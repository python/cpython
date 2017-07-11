"""Test idlelib.configdialog.

Half the class creates dialog, half works with user customizations.
Coverage: 46% just by creating dialog, 56% with current tests.
"""
from idlelib.configdialog import ConfigDialog, idleConf, changes
from test.support import requires
requires('gui')
from tkinter import Tk
import unittest
import idlelib.config as config

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
configure = None
mainpage = changes['main']
highpage = changes['highlight']
keyspage = changes['keys']

class TestDialog(ConfigDialog): pass  # Delete?


def setUpModule():
    global root, configure
    idleConf.userCfg = testcfg
    root = Tk()
    # root.withdraw()    # Comment out, see issue 30870
    configure = TestDialog(root, 'Test', _utest=True)


def tearDownModule():
    global root, configure
    idleConf.userCfg = usercfg
    configure.remove_var_callbacks()
    del configure
    root.update_idletasks()
    root.destroy()
    del root


class FontTabTest(unittest.TestCase):

    def setUp(self):
        changes.clear()

    def test_font(self):
        # Set values guaranteed not to be defaults.
        default_font = idleConf.GetFont(root, 'main', 'EditorWindow')
        default_size = str(default_font[1])
        default_bold = default_font[2] == 'bold'
        configure.font_name.set('Test Font')
        expected = {'EditorWindow': {'font': 'Test Font',
                                     'font-size': default_size,
                                     'font-bold': str(default_bold)}}
        self.assertEqual(mainpage, expected)
        changes.clear()
        configure.font_size.set(20)
        expected = {'EditorWindow': {'font': 'Test Font',
                                     'font-size': '20',
                                     'font-bold': str(default_bold)}}
        self.assertEqual(mainpage, expected)
        changes.clear()
        configure.font_bold.set(not default_bold)
        expected = {'EditorWindow': {'font': 'Test Font',
                                     'font-size': '20',
                                     'font-bold': str(not default_bold)}}
        self.assertEqual(mainpage, expected)

    def test_key_up_down_should_change_font_and_sample(self):
        # Restore defalut value
        # XXX: Can't direct un-tuple via idleConf, will get warning
        dfont = idleConf.GetFont(root, 'main', 'EditorWindow')
        dfont, dsize, dbold = dfont
        configure.font_name.set(dfont)
        configure.font_size.set(str(dsize))
        configure.font_bold.set(bool(dbold == 'bold'))

        # Save current font and sample font
        font = configure.fontlist.get('active')
        sample_font = configure.font_sample.cget('font')

        # Key down
        configure.fontlist.update()
        configure.fontlist.event_generate('<Key-Down>')
        configure.fontlist.event_generate('<KeyRelease-Down>')

        down_font = configure.fontlist.get('active')
        down_sample_font = configure.font_sample.cget('font')

        self.assertNotEqual(down_font, font)
        self.assertNotEqual(down_sample_font, sample_font)
        self.assertEqual(configure.font_name.get(), down_font.lower())
        self.assertIn(configure.font_name.get(), down_sample_font.lower())

        # Key Up
        configure.fontlist.update()
        configure.fontlist.event_generate('<Key-Up>')
        configure.fontlist.event_generate('<KeyRelease-Up>')
        up_font = configure.fontlist.get('active')
        up_sample_font = configure.font_sample.cget('font')

        self.assertEqual(up_font, font)
        self.assertEqual(up_sample_font, sample_font)
        self.assertEqual(configure.font_name.get(), up_font.lower())
        self.assertIn(configure.font_name.get(), up_sample_font.lower())

    def test_select_font_list_should_change_font_and_sample(self):
        font = configure.fontlist.get('anchor')
        sample_font = configure.font_sample.cget('font')
        index = configure.fontlist.index('anchor')
        inc_index = index + 1

        # Select next item in listbox
        configure.fontlist.select_anchor(inc_index)
        configure.fontlist.update()
        configure.fontlist.event_generate('<ButtonRelease-1>')
        select_font = configure.fontlist.get('anchor')
        select_sample_font = configure.font_sample.cget('font')

        self.assertNotEqual(select_font, font)
        self.assertNotEqual(select_sample_font, sample_font)
        self.assertEqual(configure.font_name.get(), select_font.lower())
        self.assertIn(configure.font_name.get(), select_sample_font.lower())

    #def test_sample(self): pass  # TODO

    def test_tabspace(self):
        configure.space_num.set(6)
        self.assertEqual(mainpage, {'Indent': {'num-spaces': '6'}})


class HighlightTest(unittest.TestCase):

    def setUp(self):
        changes.clear()

    #def test_colorchoose(self): pass  # TODO


class KeysTest(unittest.TestCase):

    def setUp(self):
        changes.clear()


class GeneralTest(unittest.TestCase):

    def setUp(self):
        changes.clear()

    def test_startup(self):
        configure.radio_startup_edit.invoke()
        self.assertEqual(mainpage,
                         {'General': {'editor-on-startup': '1'}})

    def test_autosave(self):
        configure.radio_save_auto.invoke()
        self.assertEqual(mainpage, {'General': {'autosave': '1'}})

    def test_editor_size(self):
        configure.entry_win_height.insert(0, '1')
        self.assertEqual(mainpage, {'EditorWindow': {'height': '140'}})
        changes.clear()
        configure.entry_win_width.insert(0, '1')
        self.assertEqual(mainpage, {'EditorWindow': {'width': '180'}})

    #def test_help_sources(self): pass  # TODO


if __name__ == '__main__':
    unittest.main(verbosity=2)
