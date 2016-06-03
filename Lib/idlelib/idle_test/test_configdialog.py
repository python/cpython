'''Unittests for idlelib/configHandler.py

Coverage: 46% just by creating dialog. The other half is change code.

'''
import unittest
from test.test_support import requires
from Tkinter import Tk
from idlelib.configDialog import ConfigDialog
from idlelib.macosxSupport import _initializeTkVariantTests


class ConfigDialogTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        _initializeTkVariantTests(cls.root)

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def test_dialog(self):
        d = ConfigDialog(self.root, 'Test', _utest=True)
        d.remove_var_callbacks()


if __name__ == '__main__':
    unittest.main(verbosity=2)
