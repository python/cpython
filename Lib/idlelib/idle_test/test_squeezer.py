from collections import namedtuple
from tkinter import Text, Tk
import unittest
from unittest.mock import Mock, NonCallableMagicMock, patch, sentinel, ANY
from test.support import requires

from idlelib.config import idleConf
from idlelib.squeezer import count_lines_with_wrapping, ExpandingButton, \
    Squeezer
from idlelib import macosx
from idlelib.textview import view_text
from idlelib.tooltip import Hovertip
from idlelib.pyshell import PyShell


SENTINEL_VALUE = sentinel.SENTINEL_VALUE


def get_test_tk_root(test_instance):
    """Helper for tests: Create a root Tk object."""
    requires('gui')
    root = Tk()
    root.withdraw()

    def cleanup_root():
        root.update_idletasks()
        root.destroy()
    test_instance.addCleanup(cleanup_root)

    return root


class CountLinesTest(unittest.TestCase):
    """Tests for the count_lines_with_wrapping function."""
    def check(self, expected, text, linewidth, tabwidth):
        return self.assertEqual(
            expected,
            count_lines_with_wrapping(text, linewidth, tabwidth),
        )

    def test_count_empty(self):
        """Test with an empty string."""
        self.assertEqual(count_lines_with_wrapping(""), 0)

    def test_count_begins_with_empty_line(self):
        """Test with a string which begins with a newline."""
        self.assertEqual(count_lines_with_wrapping("\ntext"), 2)

    def test_count_ends_with_empty_line(self):
        """Test with a string which ends with a newline."""
        self.assertEqual(count_lines_with_wrapping("text\n"), 1)

    def test_count_several_lines(self):
        """Test with several lines of text."""
        self.assertEqual(count_lines_with_wrapping("1\n2\n3\n"), 3)

    def test_tab_width(self):
        """Test with various tab widths and line widths."""
        self.check(expected=1, text='\t' * 1, linewidth=8, tabwidth=4)
        self.check(expected=1, text='\t' * 2, linewidth=8, tabwidth=4)
        self.check(expected=2, text='\t' * 3, linewidth=8, tabwidth=4)
        self.check(expected=2, text='\t' * 4, linewidth=8, tabwidth=4)
        self.check(expected=3, text='\t' * 5, linewidth=8, tabwidth=4)

        # test longer lines and various tab widths
        self.check(expected=4, text='\t' * 10, linewidth=12, tabwidth=4)
        self.check(expected=10, text='\t' * 10, linewidth=12, tabwidth=8)
        self.check(expected=2, text='\t' * 4, linewidth=10, tabwidth=3)

        # test tabwidth=1
        self.check(expected=2, text='\t' * 9, linewidth=5, tabwidth=1)
        self.check(expected=2, text='\t' * 10, linewidth=5, tabwidth=1)
        self.check(expected=3, text='\t' * 11, linewidth=5, tabwidth=1)

        # test for off-by-one errors
        self.check(expected=2, text='\t' * 6, linewidth=12, tabwidth=4)
        self.check(expected=3, text='\t' * 6, linewidth=11, tabwidth=4)
        self.check(expected=2, text='\t' * 6, linewidth=13, tabwidth=4)


class SqueezerTest(unittest.TestCase):
    """Tests for the Squeezer class."""
    def make_mock_editor_window(self):
        """Create a mock EditorWindow instance."""
        editwin = NonCallableMagicMock()
        # isinstance(editwin, PyShell) must be true for Squeezer to enable
        # auto-squeezing; in practice this will always be true
        editwin.__class__ = PyShell
        return editwin

    def make_squeezer_instance(self, editor_window=None):
        """Create an actual Squeezer instance with a mock EditorWindow."""
        if editor_window is None:
            editor_window = self.make_mock_editor_window()
        return Squeezer(editor_window)

    def test_count_lines(self):
        """Test Squeezer.count_lines() with various inputs.

        This checks that Squeezer.count_lines() calls the
        count_lines_with_wrapping() function with the appropriate parameters.
        """
        for tabwidth, linewidth in [(4, 80), (1, 79), (8, 80), (3, 120)]:
            self._test_count_lines_helper(linewidth=linewidth,
                                          tabwidth=tabwidth)

    def _prepare_mock_editwin_for_count_lines(self, editwin,
                                              linewidth, tabwidth):
        """Prepare a mock EditorWindow object for Squeezer.count_lines."""
        CHAR_WIDTH = 10
        BORDER_WIDTH = 2
        PADDING_WIDTH = 1

        # Prepare all the required functionality on the mock EditorWindow object
        # so that the calculations in Squeezer.count_lines() can run.
        editwin.get_tk_tabwidth.return_value = tabwidth
        editwin.text.winfo_width.return_value = \
            linewidth * CHAR_WIDTH + 2 * (BORDER_WIDTH + PADDING_WIDTH)
        text_opts = {
            'border': BORDER_WIDTH,
            'padx': PADDING_WIDTH,
            'font': None,
        }
        editwin.text.cget = lambda opt: text_opts[opt]

        # monkey-path tkinter.font.Font with a mock object, so that
        # Font.measure('0') returns CHAR_WIDTH
        mock_font = Mock()
        def measure(char):
            if char == '0':
                return CHAR_WIDTH
            raise ValueError("measure should only be called on '0'!")
        mock_font.return_value.measure = measure
        patcher = patch('idlelib.squeezer.Font', mock_font)
        patcher.start()
        self.addCleanup(patcher.stop)

    def _test_count_lines_helper(self, linewidth, tabwidth):
        """Helper for test_count_lines."""
        editwin = self.make_mock_editor_window()
        self._prepare_mock_editwin_for_count_lines(editwin, linewidth, tabwidth)
        squeezer = self.make_squeezer_instance(editwin)

        mock_count_lines = Mock(return_value=SENTINEL_VALUE)
        text = 'TEXT'
        with patch('idlelib.squeezer.count_lines_with_wrapping',
                   mock_count_lines):
            self.assertIs(squeezer.count_lines(text), SENTINEL_VALUE)
            mock_count_lines.assert_called_with(text, linewidth, tabwidth)

    def test_init(self):
        """Test the creation of Squeezer instances."""
        editwin = self.make_mock_editor_window()
        squeezer = self.make_squeezer_instance(editwin)
        self.assertIs(squeezer.editwin, editwin)
        self.assertEqual(squeezer.expandingbuttons, [])

    def test_write_no_tags(self):
        """Test Squeezer's overriding of the EditorWindow's write() method."""
        editwin = self.make_mock_editor_window()
        for text in ['', 'TEXT', 'LONG TEXT' * 1000, 'MANY_LINES\n' * 100]:
            editwin.write = orig_write = Mock(return_value=SENTINEL_VALUE)
            squeezer = self.make_squeezer_instance(editwin)

            self.assertEqual(squeezer.editwin.write(text, ()), SENTINEL_VALUE)
            self.assertEqual(orig_write.call_count, 1)
            orig_write.assert_called_with(text, ())
            self.assertEqual(len(squeezer.expandingbuttons), 0)

    def test_write_not_stdout(self):
        """Test Squeezer's overriding of the EditorWindow's write() method."""
        for text in ['', 'TEXT', 'LONG TEXT' * 1000, 'MANY_LINES\n' * 100]:
            editwin = self.make_mock_editor_window()
            editwin.write.return_value = SENTINEL_VALUE
            orig_write = editwin.write
            squeezer = self.make_squeezer_instance(editwin)

            self.assertEqual(squeezer.editwin.write(text, "stderr"),
                              SENTINEL_VALUE)
            self.assertEqual(orig_write.call_count, 1)
            orig_write.assert_called_with(text, "stderr")
            self.assertEqual(len(squeezer.expandingbuttons), 0)

    def test_write_stdout(self):
        """Test Squeezer's overriding of the EditorWindow's write() method."""
        editwin = self.make_mock_editor_window()
        self._prepare_mock_editwin_for_count_lines(editwin,
                                                   linewidth=80, tabwidth=8)

        for text in ['', 'TEXT']:
            editwin.write = orig_write = Mock(return_value=SENTINEL_VALUE)
            squeezer = self.make_squeezer_instance(editwin)
            squeezer.auto_squeeze_min_lines = 50

            self.assertEqual(squeezer.editwin.write(text, "stdout"),
                             SENTINEL_VALUE)
            self.assertEqual(orig_write.call_count, 1)
            orig_write.assert_called_with(text, "stdout")
            self.assertEqual(len(squeezer.expandingbuttons), 0)

        for text in ['LONG TEXT' * 1000, 'MANY_LINES\n' * 100]:
            editwin.write = orig_write = Mock(return_value=SENTINEL_VALUE)
            squeezer = self.make_squeezer_instance(editwin)
            squeezer.auto_squeeze_min_lines = 50

            self.assertEqual(squeezer.editwin.write(text, "stdout"), None)
            self.assertEqual(orig_write.call_count, 0)
            self.assertEqual(len(squeezer.expandingbuttons), 1)

    def test_auto_squeeze(self):
        """Test that the auto-squeezing creates an ExpandingButton properly."""
        root = get_test_tk_root(self)
        text_widget = Text(root)
        text_widget.mark_set("iomark", "1.0")

        editwin = self.make_mock_editor_window()
        editwin.text = text_widget
        squeezer = self.make_squeezer_instance(editwin)
        squeezer.auto_squeeze_min_lines = 5
        squeezer.count_lines = Mock(return_value=6)

        editwin.write('TEXT\n'*6, "stdout")
        self.assertEqual(text_widget.get('1.0', 'end'), '\n')
        self.assertEqual(len(squeezer.expandingbuttons), 1)

    def test_squeeze_current_text_event(self):
        """Test the squeeze_current_text event."""
        root = get_test_tk_root(self)

        # squeezing text should work for both stdout and stderr
        for tag_name in ["stdout", "stderr"]:
            text_widget = Text(root)
            text_widget.mark_set("iomark", "1.0")

            editwin = self.make_mock_editor_window()
            editwin.text = editwin.per.bottom = text_widget
            squeezer = self.make_squeezer_instance(editwin)
            squeezer.count_lines = Mock(return_value=6)

            # prepare some text in the Text widget
            text_widget.insert("1.0", "SOME\nTEXT\n", tag_name)
            text_widget.mark_set("insert", "1.0")
            self.assertEqual(text_widget.get('1.0', 'end'), 'SOME\nTEXT\n\n')

            self.assertEqual(len(squeezer.expandingbuttons), 0)

            # test squeezing the current text
            retval = squeezer.squeeze_current_text_event(event=Mock())
            self.assertEqual(retval, "break")
            self.assertEqual(text_widget.get('1.0', 'end'), '\n\n')
            self.assertEqual(len(squeezer.expandingbuttons), 1)
            self.assertEqual(squeezer.expandingbuttons[0].s, 'SOME\nTEXT')

            # test that expanding the squeezed text works and afterwards the
            # Text widget contains the original text
            squeezer.expandingbuttons[0].expand(event=Mock())
            self.assertEqual(text_widget.get('1.0', 'end'), 'SOME\nTEXT\n\n')
            self.assertEqual(len(squeezer.expandingbuttons), 0)

    def test_squeeze_current_text_event_no_allowed_tags(self):
        """Test that the event doesn't squeeze text without a relevant tag."""
        root = get_test_tk_root(self)

        text_widget = Text(root)
        text_widget.mark_set("iomark", "1.0")

        editwin = self.make_mock_editor_window()
        editwin.text = editwin.per.bottom = text_widget
        squeezer = self.make_squeezer_instance(editwin)
        squeezer.count_lines = Mock(return_value=6)

        # prepare some text in the Text widget
        text_widget.insert("1.0", "SOME\nTEXT\n", "TAG")
        text_widget.mark_set("insert", "1.0")
        self.assertEqual(text_widget.get('1.0', 'end'), 'SOME\nTEXT\n\n')

        self.assertEqual(len(squeezer.expandingbuttons), 0)

        # test squeezing the current text
        retval = squeezer.squeeze_current_text_event(event=Mock())
        self.assertEqual(retval, "break")
        self.assertEqual(text_widget.get('1.0', 'end'), 'SOME\nTEXT\n\n')
        self.assertEqual(len(squeezer.expandingbuttons), 0)

    def test_squeeze_text_before_existing_squeezed_text(self):
        """Test squeezing text before existing squeezed text."""
        root = get_test_tk_root(self)

        text_widget = Text(root)
        text_widget.mark_set("iomark", "1.0")

        editwin = self.make_mock_editor_window()
        editwin.text = editwin.per.bottom = text_widget
        squeezer = self.make_squeezer_instance(editwin)
        squeezer.count_lines = Mock(return_value=6)

        # prepare some text in the Text widget and squeeze it
        text_widget.insert("1.0", "SOME\nTEXT\n", "stdout")
        text_widget.mark_set("insert", "1.0")
        squeezer.squeeze_current_text_event(event=Mock())
        self.assertEqual(len(squeezer.expandingbuttons), 1)

        # test squeezing the current text
        text_widget.insert("1.0", "MORE\nSTUFF\n", "stdout")
        text_widget.mark_set("insert", "1.0")
        retval = squeezer.squeeze_current_text_event(event=Mock())
        self.assertEqual(retval, "break")
        self.assertEqual(text_widget.get('1.0', 'end'), '\n\n\n')
        self.assertEqual(len(squeezer.expandingbuttons), 2)
        self.assertTrue(text_widget.compare(
            squeezer.expandingbuttons[0],
            '<',
            squeezer.expandingbuttons[1],
        ))

    GetOptionSignature = namedtuple('GetOptionSignature',
        'configType section option default type warn_on_default raw')
    @classmethod
    def _make_sig(cls, configType, section, option, default=sentinel.NOT_GIVEN,
                  type=sentinel.NOT_GIVEN,
                  warn_on_default=sentinel.NOT_GIVEN,
                  raw=sentinel.NOT_GIVEN):
        return cls.GetOptionSignature(configType, section, option, default,
                                      type, warn_on_default, raw)

    @classmethod
    def get_GetOption_signature(cls, mock_call_obj):
        args, kwargs = mock_call_obj[-2:]
        return cls._make_sig(*args, **kwargs)

    def test_reload(self):
        """Test the reload() class-method."""
        self.assertIsInstance(Squeezer.auto_squeeze_min_lines, int)
        idleConf.SetOption('main', 'PyShell', 'auto-squeeze-min-lines', '42')
        Squeezer.reload()
        self.assertEqual(Squeezer.auto_squeeze_min_lines, 42)


class ExpandingButtonTest(unittest.TestCase):
    """Tests for the ExpandingButton class."""
    # In these tests the squeezer instance is a mock, but actual tkinter
    # Text and Button instances are created.
    def make_mock_squeezer(self):
        """Helper for tests: Create a mock Squeezer object."""
        root = get_test_tk_root(self)
        squeezer = Mock()
        squeezer.editwin.text = Text(root)

        # Set default values for the configuration settings
        squeezer.auto_squeeze_min_lines = 50
        return squeezer

    @patch('idlelib.squeezer.Hovertip', autospec=Hovertip)
    def test_init(self, MockHovertip):
        """Test the simplest creation of an ExpandingButton."""
        squeezer = self.make_mock_squeezer()
        text_widget = squeezer.editwin.text

        expandingbutton = ExpandingButton('TEXT', 'TAGS', 50, squeezer)
        self.assertEqual(expandingbutton.s, 'TEXT')

        # check that the underlying tkinter.Button is properly configured
        self.assertEqual(expandingbutton.master, text_widget)
        self.assertTrue('50 lines' in expandingbutton.cget('text'))

        # check that the text widget still contains no text
        self.assertEqual(text_widget.get('1.0', 'end'), '\n')

        # check that the mouse events are bound
        self.assertIn('<Double-Button-1>', expandingbutton.bind())
        right_button_code = '<Button-%s>' % ('2' if macosx.isAquaTk() else '3')
        self.assertIn(right_button_code, expandingbutton.bind())

        # check that ToolTip was called once, with appropriate values
        self.assertEqual(MockHovertip.call_count, 1)
        MockHovertip.assert_called_with(expandingbutton, ANY, hover_delay=ANY)

        # check that 'right-click' appears in the tooltip text
        tooltip_text = MockHovertip.call_args[0][1]
        self.assertIn('right-click', tooltip_text.lower())

    def test_expand(self):
        """Test the expand event."""
        squeezer = self.make_mock_squeezer()
        expandingbutton = ExpandingButton('TEXT', 'TAGS', 50, squeezer)

        # insert the button into the text widget
        # (this is normally done by the Squeezer class)
        text_widget = expandingbutton.text
        text_widget.window_create("1.0", window=expandingbutton)

        # set base_text to the text widget, so that changes are actually made
        # to it (by ExpandingButton) and we can inspect these changes afterwards
        expandingbutton.base_text = expandingbutton.text

        # trigger the expand event
        retval = expandingbutton.expand(event=Mock())
        self.assertEqual(retval, None)

        # check that the text was inserted into the text widget
        self.assertEqual(text_widget.get('1.0', 'end'), 'TEXT\n')

        # check that the 'TAGS' tag was set on the inserted text
        text_end_index = text_widget.index('end-1c')
        self.assertEqual(text_widget.get('1.0', text_end_index), 'TEXT')
        self.assertEqual(text_widget.tag_nextrange('TAGS', '1.0'),
                          ('1.0', text_end_index))

        # check that the button removed itself from squeezer.expandingbuttons
        self.assertEqual(squeezer.expandingbuttons.remove.call_count, 1)
        squeezer.expandingbuttons.remove.assert_called_with(expandingbutton)

    def test_expand_dangerous_oupput(self):
        """Test that expanding very long output asks user for confirmation."""
        squeezer = self.make_mock_squeezer()
        text = 'a' * 10**5
        expandingbutton = ExpandingButton(text, 'TAGS', 50, squeezer)
        expandingbutton.set_is_dangerous()
        self.assertTrue(expandingbutton.is_dangerous)

        # insert the button into the text widget
        # (this is normally done by the Squeezer class)
        text_widget = expandingbutton.text
        text_widget.window_create("1.0", window=expandingbutton)

        # set base_text to the text widget, so that changes are actually made
        # to it (by ExpandingButton) and we can inspect these changes afterwards
        expandingbutton.base_text = expandingbutton.text

        # patch the message box module to always return False
        with patch('idlelib.squeezer.tkMessageBox') as mock_msgbox:
            mock_msgbox.askokcancel.return_value = False
            mock_msgbox.askyesno.return_value = False

            # trigger the expand event
            retval = expandingbutton.expand(event=Mock())

        # check that the event chain was broken and no text was inserted
        self.assertEqual(retval, 'break')
        self.assertEqual(expandingbutton.text.get('1.0', 'end-1c'), '')

        # patch the message box module to always return True
        with patch('idlelib.squeezer.tkMessageBox') as mock_msgbox:
            mock_msgbox.askokcancel.return_value = True
            mock_msgbox.askyesno.return_value = True

            # trigger the expand event
            retval = expandingbutton.expand(event=Mock())

        # check that the event chain wasn't broken and the text was inserted
        self.assertEqual(retval, None)
        self.assertEqual(expandingbutton.text.get('1.0', 'end-1c'), text)

    def test_copy(self):
        """Test the copy event."""
        # testing with the actual clipboard proved problematic, so this test
        # replaces the clipboard manipulation functions with mocks and checks
        # that they are called appropriately
        squeezer = self.make_mock_squeezer()
        expandingbutton = ExpandingButton('TEXT', 'TAGS', 50, squeezer)
        expandingbutton.clipboard_clear = Mock()
        expandingbutton.clipboard_append = Mock()

        # trigger the copy event
        retval = expandingbutton.copy(event=Mock())
        self.assertEqual(retval, None)

        # check that the expanding button called clipboard_clear() and
        # clipboard_append('TEXT') once each
        self.assertEqual(expandingbutton.clipboard_clear.call_count, 1)
        self.assertEqual(expandingbutton.clipboard_append.call_count, 1)
        expandingbutton.clipboard_append.assert_called_with('TEXT')

    def test_view(self):
        """Test the view event."""
        squeezer = self.make_mock_squeezer()
        expandingbutton = ExpandingButton('TEXT', 'TAGS', 50, squeezer)
        expandingbutton.selection_own = Mock()

        with patch('idlelib.squeezer.view_text', autospec=view_text)\
                as mock_view_text:
            # trigger the view event
            expandingbutton.view(event=Mock())

            # check that the expanding button called view_text
            self.assertEqual(mock_view_text.call_count, 1)

            # check that the proper text was passed
            self.assertEqual(mock_view_text.call_args[0][2], 'TEXT')

    def test_rmenu(self):
        """Test the context menu."""
        squeezer = self.make_mock_squeezer()
        expandingbutton = ExpandingButton('TEXT', 'TAGS', 50, squeezer)
        with patch('tkinter.Menu') as mock_Menu:
            mock_menu = Mock()
            mock_Menu.return_value = mock_menu
            mock_event = Mock()
            mock_event.x = 10
            mock_event.y = 10
            expandingbutton.context_menu_event(event=mock_event)
            self.assertEqual(mock_menu.add_command.call_count,
                             len(expandingbutton.rmenu_specs))
            for label, *data in expandingbutton.rmenu_specs:
                mock_menu.add_command.assert_any_call(label=label, command=ANY)


if __name__ == '__main__':
    unittest.main(verbosity=2)
