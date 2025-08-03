"Test runscript, coverage 16%."

import unittest
from idlelib import runscript
from idlelib.editor import EditorWindow
from tkinter import Tk

from test.support import requires


class ScriptBindingTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.update_idletasks()
        for id in cls.root.tk.call('after', 'info'):
            cls.root.after_cancel(id)  # Need for EditorWindow.
        cls.root.destroy()
        del cls.root

    def test_init(self):
        ew = EditorWindow(root=self.root)
        sb = runscript.ScriptBinding(ew)
        ew._close()


if __name__ == '__main__':
    unittest.main(verbosity=2)
