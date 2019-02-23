"Test , coverage 16%."

from idlelib import iomenu
import unittest
from test.support import requires
from tkinter import Tk

from idlelib.editor import EditorWindow


class IOBindigTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.editwin = EditorWindow(root=cls.root)

    @classmethod
    def tearDownClass(cls):
        cls.editwin._close()
        del cls.editwin
        cls.root.update_idletasks()
        for id in cls.root.tk.call('after', 'info'):
            cls.root.after_cancel(id)  # Need for EditorWindow.
        cls.root.destroy()
        del cls.root

    def test_init(self):
        io = iomenu.IOBinding(self.editwin)
        self.assertIs(io.editwin, self.editwin)
        io.close


if __name__ == '__main__':
    unittest.main(verbosity=2)
