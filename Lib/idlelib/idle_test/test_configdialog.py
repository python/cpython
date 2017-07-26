"""Test idlelib.configdialog.

Half the class creates dialog, half works with user customizations.
Coverage: 46% just by creating dialog, 60% with current tests.
"""
from idlelib.configdialog import ConfigDialog, idleConf, changes, VarTrace
from test.support import requires
requires('gui')
from tkinter import Tk, IntVar, BooleanVar
import unittest
from unittest import mock
import idlelib.config as config
from idlelib.idle_test.mock_idle import Func

# Tests should not depend on fortuitous user configurations.
# They must not affect actual user .cfg files.
# Use solution from test_config: empty parsers with no filename.
usercfg = idleConf.userCfg
testcfg = {
    'main': config.IdleUserConfParser(''),
    'highlight': config.IdleUserConfParser(''),
    'keys': config.IdleUserConfParser(''),
    'extensions': config.IdleUserConfParser(''),
}

root = None
dialog = None
mainpage = changes['main']
highpage = changes['highlight']
keyspage = changes['keys']

def setUpModule():
    global root, dialog
    idleConf.userCfg = testcfg
    root = Tk()
    # root.withdraw()    # Comment out, see issue 30870
    dialog = ConfigDialog(root, 'Test', _utest=True)

def tearDownModule():
    global root, dialog
    idleConf.userCfg = usercfg
    dialog.remove_var_callbacks()
    del dialog
    root.update_idletasks()
    root.destroy()
    del root


class FontTest(unittest.TestCase):
    """Test that font widgets enable users to make font changes.

    Test that widget actions set vars, that var changes add three
    options to changes and call set_samples, and that set_samples
    changes the font of both sample boxes.
    """
    @classmethod
    def setUpClass(cls):
        dialog.set_samples = Func()  # Mask instance method.

    @classmethod
    def tearDownClass(cls):
        del dialog.set_samples  # Unmask instance method.

    def setUp(self):
        changes.clear()

    def test_load_font_cfg(self):
        # Leave widget load test to human visual check.
        # TODO Improve checks when add IdleConf.get_font_values.
        d = dialog
        d.font_name.set('Fake')
        d.font_size.set('1')
        d.font_bold.set(True)
        d.set_samples.called = 0
        d.load_font_cfg()
        self.assertNotEqual(d.font_name.get(), 'Fake')
        self.assertNotEqual(d.font_size.get(), '1')
        self.assertFalse(d.font_bold.get())
        self.assertEqual(d.set_samples.called, 3)

    def test_fontlist_key(self):
        # Up and Down keys should select a new font.

        if dialog.fontlist.size() < 2:
            cls.skipTest('need at least 2 fonts')
        fontlist = dialog.fontlist
        fontlist.activate(0)
        font = dialog.fontlist.get('active')

        # Test Down key.
        fontlist.focus_force()
        fontlist.update()
        fontlist.event_generate('<Key-Down>')
        fontlist.event_generate('<KeyRelease-Down>')

        down_font = fontlist.get('active')
        self.assertNotEqual(down_font, font)
        self.assertIn(dialog.font_name.get(), down_font.lower())

        # Test Up key.
        fontlist.focus_force()
        fontlist.update()
        fontlist.event_generate('<Key-Up>')
        fontlist.event_generate('<KeyRelease-Up>')

        up_font = fontlist.get('active')
        self.assertEqual(up_font, font)
        self.assertIn(dialog.font_name.get(), up_font.lower())

    def test_fontlist_mouse(self):
        # Click on item should select that item.

        if dialog.fontlist.size() < 2:
            cls.skipTest('need at least 2 fonts')
        fontlist = dialog.fontlist
        fontlist.activate(0)

        # Select next item in listbox
        fontlist.focus_force()
        fontlist.see(1)
        fontlist.update()
        x, y, dx, dy = fontlist.bbox(1)
        x += dx // 2
        y += dy // 2
        fontlist.event_generate('<Button-1>', x=x, y=y)
        fontlist.event_generate('<ButtonRelease-1>', x=x, y=y)

        font1 = fontlist.get(1)
        select_font = fontlist.get('anchor')
        self.assertEqual(select_font, font1)
        self.assertIn(dialog.font_name.get(), font1.lower())

    def test_sizelist(self):
        # Click on number shouod select that number
        d = dialog
        d.sizelist.variable.set(40)
        self.assertEqual(d.font_size.get(), '40')

    def test_bold_toggle(self):
        # Click on checkbutton should invert it.
        d = dialog
        d.font_bold.set(False)
        d.bold_toggle.invoke()
        self.assertTrue(d.font_bold.get())
        d.bold_toggle.invoke()
        self.assertFalse(d.font_bold.get())

    def test_font_set(self):
        # Test that setting a font Variable results in 3 provisional
        # change entries and a call to set_samples. Use values sure to
        # not be defaults.

        default_font = idleConf.GetFont(root, 'main', 'EditorWindow')
        default_size = str(default_font[1])
        default_bold = default_font[2] == 'bold'
        d = dialog
        d.font_size.set(default_size)
        d.font_bold.set(default_bold)
        d.set_samples.called = 0

        d.font_name.set('Test Font')
        expected = {'EditorWindow': {'font': 'Test Font',
                                     'font-size': default_size,
                                     'font-bold': str(default_bold)}}
        self.assertEqual(mainpage, expected)
        self.assertEqual(d.set_samples.called, 1)
        changes.clear()

        d.font_size.set('20')
        expected = {'EditorWindow': {'font': 'Test Font',
                                     'font-size': '20',
                                     'font-bold': str(default_bold)}}
        self.assertEqual(mainpage, expected)
        self.assertEqual(d.set_samples.called, 2)
        changes.clear()

        d.font_bold.set(not default_bold)
        expected = {'EditorWindow': {'font': 'Test Font',
                                     'font-size': '20',
                                     'font-bold': str(not default_bold)}}
        self.assertEqual(mainpage, expected)
        self.assertEqual(d.set_samples.called, 3)

    def test_set_samples(self):
        d = dialog
        del d.set_samples  # Unmask method for test
        d.font_sample, d.highlight_sample = {}, {}
        d.font_name.set('test')
        d.font_size.set('5')
        d.font_bold.set(1)
        expected = {'font': ('test', '5', 'bold')}

        # Test set_samples.
        d.set_samples()
        self.assertTrue(d.font_sample == d.highlight_sample == expected)

        del d.font_sample, d.highlight_sample
        d.set_samples = Func()  # Re-mask for other tests.


class IndentTest(unittest.TestCase):

    def test_load_tab_cfg(self):
        d = dialog
        d.space_num.set(16)
        d.load_tab_cfg()
        self.assertEqual(d.space_num.get(), 4)

    def test_indent_scale(self):
        changes.clear()
        dialog.indent_scale.set(26)
        self.assertEqual(dialog.space_num.get(), 16)
        self.assertEqual(mainpage, {'Indent': {'num-spaces': '16'}})


class HighlightTest(unittest.TestCase):

    def setUp(self):
        changes.clear()


class KeysTest(unittest.TestCase):

    def setUp(self):
        changes.clear()


class GeneralTest(unittest.TestCase):

    def setUp(self):
        changes.clear()

    def test_startup(self):
        dialog.radio_startup_edit.invoke()
        self.assertEqual(mainpage,
                         {'General': {'editor-on-startup': '1'}})

    def test_autosave(self):
        dialog.radio_save_auto.invoke()
        self.assertEqual(mainpage, {'General': {'autosave': '1'}})

    def test_editor_size(self):
        dialog.entry_win_height.insert(0, '1')
        self.assertEqual(mainpage, {'EditorWindow': {'height': '140'}})
        changes.clear()
        dialog.entry_win_width.insert(0, '1')
        self.assertEqual(mainpage, {'EditorWindow': {'width': '180'}})

    #def test_help_sources(self): pass  # TODO


class TestVarTrace(unittest.TestCase):

    def setUp(self):
        changes.clear()
        self.v1 = IntVar(root)
        self.v2 = BooleanVar(root)
        self.called = 0
        self.tracers = VarTrace()

    def tearDown(self):
        del self.v1, self.v2

    def var_changed_increment(self, *params):
        self.called += 13

    def var_changed_boolean(self, *params):
        pass

    def test_init(self):
        self.assertEqual(self.tracers.untraced, [])
        self.assertEqual(self.tracers.traced, [])

    def test_add(self):
        tr = self.tracers
        func = Func()
        cb = tr.make_callback = mock.Mock(return_value=func)

        v1 = tr.add(self.v1, self.var_changed_increment)
        self.assertIsInstance(v1, IntVar)
        v2 = tr.add(self.v2, self.var_changed_boolean)
        self.assertIsInstance(v2, BooleanVar)

        v3 = IntVar(root)
        v3 = tr.add(v3, ('main', 'section', 'option'))
        cb.assert_called_once()
        cb.assert_called_with(v3, ('main', 'section', 'option'))

        expected = [(v1, self.var_changed_increment),
                    (v2, self.var_changed_boolean),
                    (v3, func)]
        self.assertEqual(tr.traced, [])
        self.assertEqual(tr.untraced, expected)

        del tr.make_callback

    def test_make_callback(self):
        tr = self.tracers
        cb = tr.make_callback(self.v1, ('main', 'section', 'option'))
        self.assertTrue(callable(cb))
        self.v1.set(42)
        # Not attached, so set didn't invoke the callback.
        self.assertNotIn('section', changes['main'])
        # Invoke callback manually.
        cb()
        self.assertIn('section', changes['main'])
        self.assertEqual(changes['main']['section']['option'], '42')

    def test_attach_detach(self):
        tr = self.tracers
        v1 = tr.add(self.v1, self.var_changed_increment)
        v2 = tr.add(self.v2, self.var_changed_boolean)
        expected = [(v1, self.var_changed_increment),
                    (v2, self.var_changed_boolean)]

        # Attach callbacks and test call increment.
        tr.attach()
        self.assertEqual(tr.untraced, [])
        self.assertCountEqual(tr.traced, expected)
        v1.set(1)
        self.assertEqual(v1.get(), 1)
        self.assertEqual(self.called, 13)

        # Check that only one callback is attached to a variable.
        # If more than one callback were attached, then var_changed_increment
        # would be called twice and the counter would be 2.
        self.called = 0
        tr.attach()
        v1.set(1)
        self.assertEqual(self.called, 13)

        # Detach callbacks.
        self.called = 0
        tr.detach()
        self.assertEqual(tr.traced, [])
        self.assertCountEqual(tr.untraced, expected)
        v1.set(1)
        self.assertEqual(self.called, 0)


if __name__ == '__main__':
    unittest.main(verbosity=2)
