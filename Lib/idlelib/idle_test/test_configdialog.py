'''Test idlelib.configDialog.

Coverage: 46% just by creating dialog.
The other half is code for working with user customizations.
'''
from idlelib.configDialog import ConfigDialog  # always test import
from test.support import requires
requires('gui')
from tkinter import Tk
import unittest
from idlelib import macosxSupport as macosx

class ConfigDialogTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Tk()
        cls.root.withdraw()
        macosx._initializeTkVariantTests(cls.root)

    @classmethod
    def tearDownClass(cls):
        cls.root.update_idletasks()
        cls.root.destroy()
        del cls.root

    def test_dialog(self):
        d = ConfigDialog(self.root, 'Test', _utest=True)
        d.remove_var_callbacks()


if __name__ == '__main__':
    unittest.main(verbosity=2)
