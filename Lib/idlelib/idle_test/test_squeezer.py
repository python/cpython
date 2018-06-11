import os
from collections import namedtuple
from tkinter import Text
import unittest
from unittest.mock import Mock, NonCallableMagicMock, patch, sentinel, ANY
from test.support import captured_stderr, requires

from idlelib.squeezer import count_lines_with_wrapping, ExpandingButton, \
    Squeezer
from idlelib.pyshell import PyShell


SENTINEL_VALUE = sentinel.SENTINEL_VALUE


class TestCountLines(unittest.TestCase):
    """tests for the count_lines_with_wrapping function"""
    def check(self, expected, text, linewidth, tabwidth):
        return self.assertEqual(
            expected,
            count_lines_with_wrapping(text, linewidth, tabwidth),
        )

    def test_count_empty(self):
        """test with an empty string"""
        self.assertEqual(count_lines_with_wrapping(""), 0)

    def test_count_begins_with_empty_line(self):
        """test with a string which begins with a newline"""
        self.assertEqual(count_lines_with_wrapping("\ntext"), 2)

    def test_count_ends_with_empty_line(self):
        """test with a string which ends with a newline"""
        self.assertEqual(count_lines_with_wrapping("text\n"), 1)

    def test_count_several_lines(self):
        """test with several lines of text"""
        self.assertEqual(count_lines_with_wrapping("1\n2\n3\n"), 3)

    def test_tab_width(self):
        """test with various tab widths and line widths"""
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


class TestSqueezer(unittest.TestCase):
    """tests for the Squeezer class"""
    def make_mock_editor_window(self):
        """create a mock EditorWindow instance"""
        editwin = NonCallableMagicMock()
        # isinstance(editwin, PyShell) must be true for Squeezer to enable
        # auto-squeezing; in practice this will always be true
        editwin.__class__ = PyShell
        return editwin

    def make_squeezer_instance(self, editor_window=None):
        """create an actual Squeezer instance with a mock EditorWindow"""
        if editor_window is None:
            editor_window = self.make_mock_editor_window()
        return Squeezer(editor_window)

    def test_count_lines(self):
        """test Squeezer.count_lines() with various inputs

        This checks that Squeezer.count_lines() calls the
        count_lines_with_wrapping() function with the appropriate parameters.
        """
        for tabwidth, linewidth in [(4, 80), (1, 79), (8, 80), (3, 120)]:
            self._test_count_lines_helper(linewidth=linewidth,
                                          tabwidth=tabwidth)

    def _prepare_mock_editwin_for_count_lines(self, editwin,
                                              linewidth, tabwidth):
        """prepare a mock EditorWindow object so Squeezer.count_lines can run"""
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
        """helper for test_count_lines"""
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
        """test the creation of Squeezer instances"""
        editwin = self.make_mock_editor_window()
        editwin.rmenu_specs = []
        squeezer = self.make_squeezer_instance(editwin)
        self.assertIs(squeezer.editwin, editwin)
        self.assertEqual(squeezer.expandingbuttons, [])
        self.assertEqual(squeezer.text.bind.call_count, 1)
        squeezer.text.bind.assert_called_with(
            '<<squeeze-current-text>>', squeezer.squeeze_current_text_event)
        self.assertEqual(editwin.rmenu_specs, [
            ("Squeeze current text", "<<squeeze-current-text>>"),
        ])

    def test_write_no_tags(self):
        """test Squeezer's overriding of the EditorWindow's write() method"""
        editwin = self.make_mock_editor_window()
        for text in ['', 'TEXT', 'LONG TEXT' * 1000, 'MANY_LINES\n' * 100]:
            editwin.write = orig_write = Mock(return_value=SENTINEL_VALUE)
            squeezer = self.make_squeezer_instance(editwin)

            self.assertEqual(squeezer.editwin.write(text, ()), SENTINEL_VALUE)
            self.assertEqual(orig_write.call_count, 1)
            orig_write.assert_called_with(text, ())
            self.assertEqual(len(squeezer.expandingbuttons), 0)

    def test_write_not_stdout(self):
        """test Squeezer's overriding of the EditorWindow's write() method"""
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
        """test Squeezer's overriding of the EditorWindow's write() method"""
        editwin = self.make_mock_editor_window()
        self._prepare_mock_editwin_for_count_lines(editwin,
                                                   linewidth=80, tabwidth=8)

        for text in ['', 'TEXT']:
            editwin.write = orig_write = Mock(return_value=SENTINEL_VALUE)
            squeezer = self.make_squeezer_instance(editwin)
            squeezer.get_auto_squeeze_min_lines = Mock(return_value=30)

            self.assertEqual(squeezer.editwin.write(text, "stdout"),
                              SENTINEL_VALUE)
            self.assertEqual(orig_write.call_count, 1)
            orig_write.assert_called_with(text, "stdout")
            self.assertEqual(len(squeezer.expandingbuttons), 0)

        for text in ['LONG TEXT' * 1000, 'MANY_LINES\n' * 100]:
            editwin.write = orig_write = Mock(return_value=SENTINEL_VALUE)
            squeezer = self.make_squeezer_instance(editwin)
            squeezer.get_auto_squeeze_min_lines = Mock(return_value=30)

            self.assertEqual(squeezer.editwin.write(text, "stdout"), None)
            self.assertEqual(orig_write.call_count, 0)
            self.assertEqual(len(squeezer.expandingbuttons), 1)

    def test_expand_last_squeezed_event_no_squeezed(self):
        """test the expand_last_squeezed event"""
        # The tested scenario: There are no squeezed texts, therefore there
        # are no ExpandingButton instances. The expand_last_squeezed event
        # is called and should fail (i.e. call squeezer.text.bell()).
        editwin = self.make_mock_editor_window()
        squeezer = self.make_squeezer_instance(editwin)

        retval = squeezer.expand_last_squeezed_event(event=Mock())
        self.assertEqual(retval, "break")
        self.assertEqual(squeezer.text.bell.call_count, 1)

    def test_expand_last_squeezed_event(self):
        """test the expand_last_squeezed event"""
        # The tested scenario: There are two squeezed texts, therefore there
        # are two ExpandingButton instances. The expand_last_squeezed event
        # is called three times. The first time should expand the second
        # ExpandingButton; the second time should expand the first
        # ExpandingButton; the third time should fail (i.e. call
        # squeezer.text.bell()).
        editwin = self.make_mock_editor_window()
        squeezer = self.make_squeezer_instance(editwin)
        mock_expandingbutton1 = Mock()
        mock_expandingbutton2 = Mock()
        squeezer.expandingbuttons = [mock_expandingbutton1,
                                     mock_expandingbutton2]

        # check that the second expanding button is expanded
        retval = squeezer.expand_last_squeezed_event(event=SENTINEL_VALUE)
        self.assertEqual(retval, "break")
        self.assertEqual(squeezer.text.bell.call_count, 0)
        self.assertEqual(mock_expandingbutton1.expand.call_count, 0)
        self.assertEqual(mock_expandingbutton2.expand.call_count, 1)
        mock_expandingbutton2.expand.assert_called_with(SENTINEL_VALUE)

        # normally the expanded ExpandingButton would remove itself from
        # squeezer.expandingbuttons, but we used a mock instead
        squeezer.expandingbuttons.remove(mock_expandingbutton2)

        # check that the first expanding button is expanded
        retval = squeezer.expand_last_squeezed_event(event=SENTINEL_VALUE)
        self.assertEqual(retval, "break")
        self.assertEqual(squeezer.text.bell.call_count, 0)
        self.assertEqual(mock_expandingbutton1.expand.call_count, 1)
        self.assertEqual(mock_expandingbutton2.expand.call_count, 1)
        mock_expandingbutton1.expand.assert_called_with(SENTINEL_VALUE)

        # normally the expanded ExpandingButton would remove itself from
        # squeezer.expandingbuttons, but we used a mock instead
        squeezer.expandingbuttons.remove(mock_expandingbutton1)

        # no more expanding buttons -- check that squeezer.text.bell() is called
        retval = squeezer.expand_last_squeezed_event(event=Mock())
        self.assertEqual(retval, "break")
        self.assertEqual(squeezer.text.bell.call_count, 1)

    def test_preview_last_squeezed_event_no_squeezed(self):
        """test the preview_last_squeezed event"""
        # The tested scenario: There are no squeezed texts, therefore there
        # are no ExpandingButton instances. The preview_last_squeezed event
        # is called and should fail (i.e. call squeezer.text.bell()).
        editwin = self.make_mock_editor_window()
        squeezer = self.make_squeezer_instance(editwin)
        squeezer.get_preview_command = Mock(return_value='notepad.exe %(fn)s')

        retval = squeezer.preview_last_squeezed_event(event=Mock())
        self.assertEqual(retval, "break")
        self.assertEqual(squeezer.text.bell.call_count, 1)

    def test_preview_last_squeezed_event_no_preview_command(self):
        """test the preview_last_squeezed event"""
        # The tested scenario: There is one squeezed text, therefore there
        # is one ExpandingButton instance. However, no preview command has been
        # configured. The preview_last_squeezed event is called and should fail
        # (i.e. call squeezer.text.bell()).
        editwin = self.make_mock_editor_window()
        squeezer = self.make_squeezer_instance(editwin)
        squeezer.get_preview_command = Mock(return_value='')
        mock_expandingbutton = Mock()
        squeezer.expandingbuttons = [mock_expandingbutton]

        retval = squeezer.preview_last_squeezed_event(event=Mock())
        self.assertEqual(retval, "break")
        self.assertEqual(squeezer.text.bell.call_count, 1)

    def test_preview_last_squeezed_event(self):
        """test the preview_last_squeezed event"""
        # The tested scenario: There are two squeezed texts, therefore there
        # are two ExpandingButton instances. The preview_last_squeezed event
        # is called twice. Both times should call the preview() method of the
        # second ExpandingButton.
        editwin = self.make_mock_editor_window()
        squeezer = self.make_squeezer_instance(editwin)
        squeezer.get_preview_command = Mock(return_value='notepad.exe %(fn)s')
        mock_expandingbutton1 = Mock()
        mock_expandingbutton2 = Mock()
        squeezer.expandingbuttons = [mock_expandingbutton1,
                                     mock_expandingbutton2]

        # check that the second expanding button is previewed
        retval = squeezer.preview_last_squeezed_event(event=SENTINEL_VALUE)
        self.assertEqual(retval, "break")
        self.assertEqual(squeezer.text.bell.call_count, 0)
        self.assertEqual(mock_expandingbutton1.preview.call_count, 0)
        self.assertEqual(mock_expandingbutton2.preview.call_count, 1)
        mock_expandingbutton2.preview.assert_called_with(SENTINEL_VALUE)

    def test_auto_squeeze(self):
        """test that the auto-squeezing creates an ExpandingButton properly"""
        requires('gui')
        text_widget = Text()
        text_widget.mark_set("iomark", "1.0")

        editwin = self.make_mock_editor_window()
        editwin.text = text_widget
        squeezer = self.make_squeezer_instance(editwin)
        squeezer.get_auto_squeeze_min_lines = Mock(return_value=5)
        squeezer.count_lines = Mock(return_value=6)

        editwin.write('TEXT\n'*6, "stdout")
        self.assertEqual(text_widget.get('1.0', 'end'), '\n')
        self.assertEqual(len(squeezer.expandingbuttons), 1)

    def test_squeeze_current_text_event(self):
        """test the squeeze_current_text event"""
        requires('gui')

        # squeezing text should work for both stdout and stderr
        for tag_name in "stdout", "stderr":
            text_widget = Text()
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
        """test that the event doesn't squeeze text without a relevant tag"""
        requires('gui')

        text_widget = Text()
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
        """test squeezing text before existing squeezed text"""
        requires('gui')

        text_widget = Text()
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

    def test_get_auto_squeeze_min_lines(self):
        """test the auto-squeeze-min-lines config getter"""
        with patch('idlelib.config.idleConf.GetOption') as MockGetOption:
            MockGetOption.return_value = SENTINEL_VALUE
            retval = Squeezer.get_auto_squeeze_min_lines()

            self.assertEqual(retval, SENTINEL_VALUE)
            self.assertEqual(MockGetOption.call_count, 1)
            sig = self.get_GetOption_signature(MockGetOption.call_args)
            self.assertSequenceEqual(
                (sig.configType, sig.section, sig.option),
                ("extensions", "Squeezer", "auto-squeeze-min-lines"),
            )
            self.assertEqual(sig.type, "int")
            self.assertNotEqual(sig.default, sentinel.NOT_GIVEN)

    def test_get_preview_command(self):
        """test the preview-command config getter"""
        fake_cmd = 'FAKE_VIEWER_APP {filepath}'
        with patch('idlelib.config.idleConf.GetOption') as MockGetOption:
            MockGetOption.return_value = fake_cmd
            retval = Squeezer.get_preview_command()

            self.assertEqual(retval, fake_cmd)
            self.assertEqual(MockGetOption.call_count, 1)
            sig = self.get_GetOption_signature(MockGetOption.call_args)
            self.assertSequenceEqual(
                (sig.configType, sig.section, sig.option),
                ("extensions", "Squeezer",
                 Squeezer._PREVIEW_COMMAND_CONFIG_PARAM_NAME),
            )
            self.assertTrue(sig.raw)
            self.assertNotEqual(sig.default, sentinel.NOT_GIVEN)

    def test_invalid_preview_command_template(self):
        """test the preview-command config getter"""
        with patch('idlelib.config.idleConf.GetOption') as MockGetOption:
            MockGetOption.return_value = 'FAKE_VIEWER_APP {filepath'
            with captured_stderr():
                retval = Squeezer.get_preview_command()
            self.assertFalse(retval)

    def test_get_show_tooltip(self):
        """test the show-tooltip config getter"""
        with patch('idlelib.config.idleConf.GetOption') as MockGetOption:
            MockGetOption.return_value = SENTINEL_VALUE
            retval = Squeezer.get_show_tooltip()

            self.assertEqual(retval, SENTINEL_VALUE)
            self.assertEqual(MockGetOption.call_count, 1)
            sig = self.get_GetOption_signature(MockGetOption.call_args)
            self.assertSequenceEqual(
                (sig.configType, sig.section, sig.option),
                ("extensions", "Squeezer", "show-tooltip"),
            )
            self.assertEqual(sig.type, "bool")
            self.assertNotEqual(sig.default, sentinel.NOT_GIVEN)

    def test_get_tooltip_delay(self):
        """test the tooltip-delay config getter"""
        with patch('idlelib.config.idleConf.GetOption') as MockGetOption:
            MockGetOption.return_value = SENTINEL_VALUE
            retval = Squeezer.get_tooltip_delay()

            self.assertEqual(retval, SENTINEL_VALUE)
            self.assertEqual(MockGetOption.call_count, 1)
            sig = self.get_GetOption_signature(MockGetOption.call_args)
            self.assertSequenceEqual(
                (sig.configType, sig.section, sig.option),
                ("extensions", "Squeezer", "tooltip-delay"),
            )
            self.assertEqual(sig.type, "int")
            self.assertNotEqual(sig.default, sentinel.NOT_GIVEN)

    def test_conditional_add_preview_last_squeezed_text_to_edit_menu(self):
        """test conditionally adding preview-last-squeezed to the edit menu"""
        import importlib
        import idlelib.squeezer

        # cleanup -- make sure to reload idlelib.squeezer, since ths test
        # messes around with it a bit
        self.addCleanup(importlib.reload, idlelib.squeezer)

        preview_last_squeezed_menu_item = \
            ("Preview last squeezed text", "<<preview-last-squeezed>>")

        # We can't override idlelib.squeezer.Squeezer.get_preview_command()
        # in time, since what we want to test happens at module load time,
        # and such a change can only be done once the module has been loaded.
        # Instead, we'll patch idlelib.config.idleConf.GetOption which
        # is used by get_preview_command().
        with patch('idlelib.config.idleConf.GetOption') as MockGetOption:
            # First, load the module with no preview command defined, and check
            # that the preview-last-squeezed option is not added to the Edit
            # menu.
            MockGetOption.return_value = ''
            importlib.reload(idlelib.squeezer)
            edit_menu = dict(idlelib.squeezer.Squeezer.menudefs)['edit']
            self.assertNotIn(preview_last_squeezed_menu_item, edit_menu)

            # save the length of the edit menu spec, for comparison later
            edit_menu_len_without_preview_last = len(edit_menu)

            # Second, load the module with a preview command defined, and check
            # that the preview-last-squeezed option is indeed added to the Edit
            # menu.
            MockGetOption.return_value = 'notepad.exe %(fn)s'
            importlib.reload(idlelib.squeezer)
            edit_menu = dict(idlelib.squeezer.Squeezer.menudefs)['edit']
            self.assertEqual(edit_menu[-1], preview_last_squeezed_menu_item)
            self.assertEqual(len(edit_menu), edit_menu_len_without_preview_last + 1)


class TestExpandingButton(unittest.TestCase):
    """tests for the ExpandingButton class"""
    # In these tests the squeezer instance is a mock, but actual tkinter
    # Text and Button instances are created.
    def make_mock_squeezer(self):
        """helper for tests"""
        requires('gui')
        squeezer = Mock()
        squeezer.editwin.text = Text()

        # Set default values for the configuration settings
        squeezer.get_max_num_of_lines = Mock(return_value=30)
        squeezer.get_preview_command = Mock(return_value='')
        squeezer.get_show_tooltip = Mock(return_value=False)
        squeezer.get_tooltip_delay = Mock(return_value=1500)
        return squeezer

    @patch('idlelib.squeezer.ToolTip')
    def test_init_no_preview_command_nor_tooltip(self, MockToolTip):
        """Test the simplest creation of an ExpandingButton"""
        squeezer = self.make_mock_squeezer()
        squeezer.get_show_tooltip.return_value = False
        squeezer.get_preview_command.return_value = ''
        text_widget = squeezer.editwin.text

        expandingbutton = ExpandingButton('TEXT', 'TAGS', 30, squeezer)
        self.assertEqual(expandingbutton.s, 'TEXT')

        # check that the underlying tkinter.Button is properly configured
        self.assertEqual(expandingbutton.master, text_widget)
        self.assertTrue('30 lines' in expandingbutton.cget('text'))

        # check that the text widget still contains no text
        self.assertEqual(text_widget.get('1.0', 'end'), '\n')

        # check that no tooltip was created
        self.assertEqual(MockToolTip.call_count, 0)

    def test_bindings_with_preview_command(self):
        """test tooltip creation with a preview command configured"""
        squeezer = self.make_mock_squeezer()
        squeezer.get_preview_command.return_value = 'notepad.exe %(fn)s'
        expandingbutton = ExpandingButton('TEXT', 'TAGS', 30, squeezer)

        # check that when a preview command is configured, an event is bound
        # on the button for middle-click
        self.assertIn('<Double-Button-1>', expandingbutton.bind())
        self.assertIn('<Button-2>', expandingbutton.bind())
        self.assertIn('<Button-3>', expandingbutton.bind())

    def test_bindings_without_preview_command(self):
        """test tooltip creation without a preview command configured"""
        squeezer = self.make_mock_squeezer()
        squeezer.get_preview_command.return_value = ''
        expandingbutton = ExpandingButton('TEXT', 'TAGS', 30, squeezer)

        # check button's event bindings: double-click, right-click, middle-click
        self.assertIn('<Double-Button-1>', expandingbutton.bind())
        self.assertIn('<Button-2>', expandingbutton.bind())
        self.assertNotIn('<Button-3>', expandingbutton.bind())

    @patch('idlelib.squeezer.ToolTip')
    def test_init_tooltip_with_preview_command(self, MockToolTip):
        """test tooltip creation with a preview command configured"""
        squeezer = self.make_mock_squeezer()
        squeezer.get_show_tooltip.return_value = True
        squeezer.get_tooltip_delay.return_value = SENTINEL_VALUE
        squeezer.get_preview_command.return_value = 'notepad.exe %(fn)s'
        expandingbutton = ExpandingButton('TEXT', 'TAGS', 30, squeezer)

        # check that ToolTip was called once, with appropriate values
        self.assertEqual(MockToolTip.call_count, 1)
        MockToolTip.assert_called_with(expandingbutton, ANY,
                                       delay=SENTINEL_VALUE)

        # check that 'right-click' appears in the tooltip text, since we
        # configured a non-empty preview command
        tooltip_text = MockToolTip.call_args[0][1]
        self.assertIn('right-click', tooltip_text.lower())

    @patch('idlelib.squeezer.ToolTip')
    def test_init_tooltip_without_preview_command(self, MockToolTip):
        """test tooltip creation without a preview command configured"""
        squeezer = self.make_mock_squeezer()
        squeezer.get_show_tooltip.return_value = True
        squeezer.get_tooltip_delay.return_value = SENTINEL_VALUE
        squeezer.get_preview_command.return_value = ''
        expandingbutton = ExpandingButton('TEXT', 'TAGS', 30, squeezer)

        # check that ToolTip was called once, with appropriate values
        self.assertEqual(MockToolTip.call_count, 1)
        MockToolTip.assert_called_with(expandingbutton, ANY,
                                       delay=SENTINEL_VALUE)

        # check that 'right-click' doesn't appear in the tooltip text, since
        # we configured an empty preview command
        tooltip_text = MockToolTip.call_args[0][1]
        self.assertNotIn('right-click', tooltip_text.lower())

    def test_expand(self):
        """test the expand event"""
        squeezer = self.make_mock_squeezer()
        expandingbutton = ExpandingButton('TEXT', 'TAGS', 30, squeezer)

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

        # check that the text was inserting into the text widget
        self.assertEqual(text_widget.get('1.0', 'end'), 'TEXT\n')

        # check that the 'TAGS' tag was set on the inserted text
        text_end_index = text_widget.index('end-1c')
        self.assertEqual(text_widget.get('1.0', text_end_index), 'TEXT')
        self.assertEqual(text_widget.tag_nextrange('TAGS', '1.0'),
                          ('1.0', text_end_index))

        # check that the button removed itself from squeezer.expandingbuttons
        self.assertEqual(squeezer.expandingbuttons.remove.call_count, 1)
        squeezer.expandingbuttons.remove.assert_called_with(expandingbutton)

    def test_copy(self):
        """test the copy event"""
        # testing with the actual clipboard proved problematic, so this test
        # replaces the clipboard manipulation functions with mocks and checks
        # that they are called appropriately
        squeezer = self.make_mock_squeezer()
        expandingbutton = ExpandingButton('TEXT', 'TAGS', 30, squeezer)
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

    def _file_cleanup(self, filename):
        if os.path.exists(filename):
            try:
                os.remove(filename)
            except OSError:
                pass

    def test_preview(self):
        """test the preview event"""
        squeezer = self.make_mock_squeezer()
        squeezer.get_preview_command.return_value = 'FAKE_VIEWER_APP {filepath}'
        expandingbutton = ExpandingButton('TEXT', 'TAGS', 30, squeezer)
        expandingbutton.selection_own = Mock()

        with patch('subprocess.Popen') as mock_popen:
            # trigger the preview event
            expandingbutton.preview(event=Mock())

            # check that the expanding button called Popen once
            self.assertEqual(mock_popen.call_count, 1)

            command = mock_popen.call_args[0][0]
            viewer, filename = command.split(' ', 1)

            # check that the command line was created using the configured
            # preview command, and that a temporary file was actually created
            self.assertEqual(viewer, 'FAKE_VIEWER_APP')
            self.assertTrue(os.path.isfile(filename))

            # cleanup - remove the temporary file after this test
            self.addCleanup(self._file_cleanup, filename)

            # check that the temporary file contains the squeezed text
            with open(filename, 'r') as f:
                self.assertEqual(f.read(), 'TEXT')
