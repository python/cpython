"Test stackviewer, coverage 19%."

from idlelib import stackviewer
import unittest
from test.support import requires
from tkinter import Tk


class Test(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.update_idletasks()
##        for id in cls.root.tk.call('after', 'info'):
##            cls.root.after_cancel(id)  # Need for EditorWindow.
        cls.root.destroy()
        del cls.root

    def test_init(self):
        try:
            zzz
        except NameError as e:
            ex = e
# Test runners suppress setting of sys.last_xyx, which stackviewer needs.
# Revise stackviewer so following works.
#        stackviewer.StackBrowser(self.root, ex=exc)


if __name__ == '__main__':
    unittest.main(verbosity=2)
