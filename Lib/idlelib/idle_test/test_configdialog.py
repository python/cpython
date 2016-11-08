"""Test idlelib.configdialog.

Half the class creates dialog, half works with user customizations.
Coverage: 46% just by creating dialog, 56% with current tests.
"""
from idlelib.configdialog import ConfigDialog, idleConf  # test import
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

# ConfigDialog.changedItems is a 3-level hierarchical dictionary of
# pending changes that mirrors the multilevel user config dict.
# For testing, record args in a list for comparison with expected.
changes = []
class TestDialog(ConfigDialog):
    def AddChangedItem(self, *args):
        changes.append(args)

def setUpModule():
    global root, configure
    idleConf.userCfg = testcfg
    root = Tk()
    root.withdraw()
    configure = TestDialog(root, 'Test', _utest=True)


def tearDownModule():
    global root, configure
    idleConf.userCfg = testcfg
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
        dfont = idleConf.GetFont(root, 'main', 'EditorWindow')
        dsize = str(dfont[1])
        dbold = dfont[2] == 'bold'
        configure.fontName.set('Test Font')
        expected = [
            ('main', 'EditorWindow', 'font', 'Test Font'),
            ('main', 'EditorWindow', 'font-size', dsize),
            ('main', 'EditorWindow', 'font-bold', dbold)]
        self.assertEqual(changes, expected)
        changes.clear()
        configure.fontSize.set(20)
        expected = [
            ('main', 'EditorWindow', 'font', 'Test Font'),
            ('main', 'EditorWindow', 'font-size', '20'),
            ('main', 'EditorWindow', 'font-bold', dbold)]
        self.assertEqual(changes, expected)
        changes.clear()
        configure.fontBold.set(not dbold)
        expected = [
            ('main', 'EditorWindow', 'font', 'Test Font'),
            ('main', 'EditorWindow', 'font-size', '20'),
            ('main', 'EditorWindow', 'font-bold', not dbold)]
        self.assertEqual(changes, expected)

    #def test_sample(self): pass  # TODO

    def test_tabspace(self):
        configure.spaceNum.set(6)
        self.assertEqual(changes, [('main', 'Indent', 'num-spaces', 6)])


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
        configure.radioStartupEdit.invoke()
        self.assertEqual(changes,
                         [('main', 'General', 'editor-on-startup', 1)])

    def test_autosave(self):
        configure.radioSaveAuto.invoke()
        self.assertEqual(changes, [('main', 'General', 'autosave', 1)])

    def test_editor_size(self):
        configure.entryWinHeight.insert(0, '1')
        self.assertEqual(changes, [('main', 'EditorWindow', 'height', '140')])
        changes.clear()
        configure.entryWinWidth.insert(0, '1')
        self.assertEqual(changes, [('main', 'EditorWindow', 'width', '180')])

    #def test_help_sources(self): pass  # TODO


if __name__ == '__main__':
    unittest.main(verbosity=2)
