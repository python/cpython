"""Test idlelib.configdialog.

Half the class creates dialog, half works with user customizations.
Coverage: 63%.
"""
from idlelib import configdialog
from test.support import requires
requires('gui')
import unittest
from unittest import mock
from idlelib.idle_test.mock_idle import Func
from tkinter import Tk, Frame, IntVar, BooleanVar, DISABLED, NORMAL
from idlelib import config
from idlelib.configdialog import idleConf, changes, tracers

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
    dialog = configdialog.ConfigDialog(root, 'Test', _utest=True)

def tearDownModule():
    global root, dialog
    idleConf.userCfg = usercfg
    tracers.detach()
    del dialog
    root.update_idletasks()
    root.destroy()
    del root


class FontPageTest(unittest.TestCase):
    """Test that font widgets enable users to make font changes.

    Test that widget actions set vars, that var changes add three
    options to changes and call set_samples, and that set_samples
    changes the font of both sample boxes.
    """
    @classmethod
    def setUpClass(cls):
        page = cls.page = dialog.fontpage
        #dialog.note.insert(0, page, text='copy')
        #dialog.note.add(page, text='copyfgfg')
        dialog.note.select(page)
        page.set_samples = Func()  # Mask instance method.

    @classmethod
    def tearDownClass(cls):
        del cls.page.set_samples  # Unmask instance method.

    def setUp(self):
        changes.clear()

    def test_load_font_cfg(self):
        # Leave widget load test to human visual check.
        # TODO Improve checks when add IdleConf.get_font_values.
        tracers.detach()
        d = self.page
        d.font_name.set('Fake')
        d.font_size.set('1')
        d.font_bold.set(True)
        d.set_samples.called = 0
        d.load_font_cfg()
        self.assertNotEqual(d.font_name.get(), 'Fake')
        self.assertNotEqual(d.font_size.get(), '1')
        self.assertFalse(d.font_bold.get())
        self.assertEqual(d.set_samples.called, 1)
        tracers.attach()

    def test_fontlist_key(self):
        # Up and Down keys should select a new font.
        d = self.page
        if d.fontlist.size() < 2:
            self.skipTest('need at least 2 fonts')
        fontlist = d.fontlist
        fontlist.activate(0)
        font = d.fontlist.get('active')

        # Test Down key.
        fontlist.focus_force()
        fontlist.update()
        fontlist.event_generate('<Key-Down>')
        fontlist.event_generate('<KeyRelease-Down>')

        down_font = fontlist.get('active')
        self.assertNotEqual(down_font, font)
        self.assertIn(d.font_name.get(), down_font.lower())

        # Test Up key.
        fontlist.focus_force()
        fontlist.update()
        fontlist.event_generate('<Key-Up>')
        fontlist.event_generate('<KeyRelease-Up>')

        up_font = fontlist.get('active')
        self.assertEqual(up_font, font)
        self.assertIn(d.font_name.get(), up_font.lower())

    def test_fontlist_mouse(self):
        # Click on item should select that item.
        d = self.page
        if d.fontlist.size() < 2:
            cls.skipTest('need at least 2 fonts')
        fontlist = d.fontlist
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
        self.assertIn(d.font_name.get(), font1.lower())

    def test_sizelist(self):
        # Click on number shouod select that number
        d = self.page
        d.sizelist.variable.set(40)
        self.assertEqual(d.font_size.get(), '40')

    def test_bold_toggle(self):
        # Click on checkbutton should invert it.
        d = self.page
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
        d = self.page
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
        d = self.page
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

    @classmethod
    def setUpClass(cls):
        cls.page = dialog.fontpage

    def test_load_tab_cfg(self):
        d = self.page
        d.space_num.set(16)
        d.load_tab_cfg()
        self.assertEqual(d.space_num.get(), 4)

    def test_indent_scale(self):
        d = self.page
        changes.clear()
        d.indent_scale.set(20)
        self.assertEqual(d.space_num.get(), 16)
        self.assertEqual(mainpage, {'Indent': {'num-spaces': '16'}})


class HighlightTest(unittest.TestCase):

    def setUp(self):
        changes.clear()


class KeysTest(unittest.TestCase):

    def setUp(self):
        changes.clear()


class GenPageTest(unittest.TestCase):
    """Test that general tab widgets enable users to make changes.

    Test that widget actions set vars, that var changes add
    options to changes and that helplist works correctly.
    """
    @classmethod
    def setUpClass(cls):
        page = cls.page = dialog.genpage
        dialog.note.select(page)
        page.set = page.set_add_delete_state = Func()
        page.upc = page.update_help_changes = Func()

    @classmethod
    def tearDownClass(cls):
        page = cls.page
        del page.set, page.set_add_delete_state
        del page.upc, page.update_help_changes
        page.helplist.delete(0, 'end')
        page.user_helplist.clear()

    def setUp(self):
        changes.clear()

    def test_load_general_cfg(self):
        # Set to wrong values, load, check right values.
        eq = self.assertEqual
        d = self.page
        d.startup_edit.set(1)
        d.autosave.set(1)
        d.win_width.set(1)
        d.win_height.set(1)
        d.helplist.insert('end', 'bad')
        d.user_helplist = ['bad', 'worse']
        idleConf.SetOption('main', 'HelpFiles', '1', 'name;file')
        d.load_general_cfg()
        eq(d.startup_edit.get(), 0)
        eq(d.autosave.get(), 0)
        eq(d.win_width.get(), '80')
        eq(d.win_height.get(), '40')
        eq(d.helplist.get(0, 'end'), ('name',))
        eq(d.user_helplist, [('name', 'file', '1')])

    def test_startup(self):
        d = self.page
        d.startup_editor_on.invoke()
        self.assertEqual(mainpage,
                         {'General': {'editor-on-startup': '1'}})
        changes.clear()
        d.startup_shell_on.invoke()
        self.assertEqual(mainpage,
                         {'General': {'editor-on-startup': '0'}})

    def test_autosave(self):
        d = self.page
        d.save_auto_on.invoke()
        self.assertEqual(mainpage, {'General': {'autosave': '1'}})
        d.save_ask_on.invoke()
        self.assertEqual(mainpage, {'General': {'autosave': '0'}})

    def test_editor_size(self):
        d = self.page
        d.win_height_int.insert(0, '1')
        self.assertEqual(mainpage, {'EditorWindow': {'height': '140'}})
        changes.clear()
        d.win_width_int.insert(0, '1')
        self.assertEqual(mainpage, {'EditorWindow': {'width': '180'}})

    def test_source_selected(self):
        d = self.page
        d.set = d.set_add_delete_state
        d.upc = d.update_help_changes
        helplist = d.helplist
        dex = 'end'
        helplist.insert(dex, 'source')
        helplist.activate(dex)

        helplist.focus_force()
        helplist.see(dex)
        helplist.update()
        x, y, dx, dy = helplist.bbox(dex)
        x += dx // 2
        y += dy // 2
        d.set.called = d.upc.called = 0
        helplist.event_generate('<Enter>', x=0, y=0)
        helplist.event_generate('<Motion>', x=x, y=y)
        helplist.event_generate('<Button-1>', x=x, y=y)
        helplist.event_generate('<ButtonRelease-1>', x=x, y=y)
        self.assertEqual(helplist.get('anchor'), 'source')
        self.assertTrue(d.set.called)
        self.assertFalse(d.upc.called)

    def test_set_add_delete_state(self):
        # Call with 0 items, 1 unselected item, 1 selected item.
        eq = self.assertEqual
        d = self.page
        del d.set_add_delete_state  # Unmask method.
        sad = d.set_add_delete_state
        h = d.helplist

        h.delete(0, 'end')
        sad()
        eq(d.button_helplist_edit['state'], DISABLED)
        eq(d.button_helplist_remove['state'], DISABLED)

        h.insert(0, 'source')
        sad()
        eq(d.button_helplist_edit['state'], DISABLED)
        eq(d.button_helplist_remove['state'], DISABLED)

        h.selection_set(0)
        sad()
        eq(d.button_helplist_edit['state'], NORMAL)
        eq(d.button_helplist_remove['state'], NORMAL)
        d.set_add_delete_state = Func()  # Mask method.

    def test_helplist_item_add(self):
        # Call without and twice with HelpSource result.
        # Double call enables check on order.
        eq = self.assertEqual
        orig_helpsource = configdialog.HelpSource
        hs = configdialog.HelpSource = Func(return_self=True)
        d = self.page
        d.helplist.delete(0, 'end')
        d.user_helplist.clear()
        d.set.called = d.upc.called = 0

        hs.result = ''
        d.helplist_item_add()
        self.assertTrue(list(d.helplist.get(0, 'end')) ==
                        d.user_helplist == [])
        self.assertFalse(d.upc.called)

        hs.result = ('name1', 'file1')
        d.helplist_item_add()
        hs.result = ('name2', 'file2')
        d.helplist_item_add()
        eq(d.helplist.get(0, 'end'), ('name1', 'name2'))
        eq(d.user_helplist, [('name1', 'file1'), ('name2', 'file2')])
        eq(d.upc.called, 2)
        self.assertFalse(d.set.called)

        configdialog.HelpSource = orig_helpsource

    def test_helplist_item_edit(self):
        # Call without and with HelpSource change.
        eq = self.assertEqual
        orig_helpsource = configdialog.HelpSource
        hs = configdialog.HelpSource = Func(return_self=True)
        d = self.page
        d.helplist.delete(0, 'end')
        d.helplist.insert(0, 'name1')
        d.helplist.selection_set(0)
        d.helplist.selection_anchor(0)
        d.user_helplist.clear()
        d.user_helplist.append(('name1', 'file1'))
        d.set.called = d.upc.called = 0

        hs.result = ''
        d.helplist_item_edit()
        hs.result = ('name1', 'file1')
        d.helplist_item_edit()
        eq(d.helplist.get(0, 'end'), ('name1',))
        eq(d.user_helplist, [('name1', 'file1')])
        self.assertFalse(d.upc.called)

        hs.result = ('name2', 'file2')
        d.helplist_item_edit()
        eq(d.helplist.get(0, 'end'), ('name2',))
        eq(d.user_helplist, [('name2', 'file2')])
        self.assertTrue(d.upc.called == d.set.called == 1)

        configdialog.HelpSource = orig_helpsource

    def test_helplist_item_remove(self):
        eq = self.assertEqual
        d = self.page
        d.helplist.delete(0, 'end')
        d.helplist.insert(0, 'name1')
        d.helplist.selection_set(0)
        d.helplist.selection_anchor(0)
        d.user_helplist.clear()
        d.user_helplist.append(('name1', 'file1'))
        d.set.called = d.upc.called = 0

        d.helplist_item_remove()
        eq(d.helplist.get(0, 'end'), ())
        eq(d.user_helplist, [])
        self.assertTrue(d.upc.called == d.set.called == 1)

    def test_update_help_changes(self):
        d = self.page
        del d.update_help_changes
        d.user_helplist.clear()
        d.user_helplist.append(('name1', 'file1'))
        d.user_helplist.append(('name2', 'file2'))

        d.update_help_changes()
        self.assertEqual(mainpage['HelpFiles'],
                         {'1': 'name1;file1', '2': 'name2;file2'})
        d.update_help_changes = Func()


class VarTraceTest(unittest.TestCase):

    def setUp(self):
        changes.clear()
        tracers.clear()
        self.v1 = IntVar(root)
        self.v2 = BooleanVar(root)
        self.called = 0

    def tearDown(self):
        del self.v1, self.v2

    def var_changed_increment(self, *params):
        self.called += 13

    def var_changed_boolean(self, *params):
        pass

    def test_init(self):
        tracers.__init__()
        self.assertEqual(tracers.untraced, [])
        self.assertEqual(tracers.traced, [])

    def test_clear(self):
        tracers.untraced.append(0)
        tracers.traced.append(1)
        tracers.clear()
        self.assertEqual(tracers.untraced, [])
        self.assertEqual(tracers.traced, [])

    def test_add(self):
        tr = tracers
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
        cb = tracers.make_callback(self.v1, ('main', 'section', 'option'))
        self.assertTrue(callable(cb))
        self.v1.set(42)
        # Not attached, so set didn't invoke the callback.
        self.assertNotIn('section', changes['main'])
        # Invoke callback manually.
        cb()
        self.assertIn('section', changes['main'])
        self.assertEqual(changes['main']['section']['option'], '42')

    def test_attach_detach(self):
        tr = tracers
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
