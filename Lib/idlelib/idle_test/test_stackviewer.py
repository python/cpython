"Test stackviewer, coverage 63%."

from idlelib import stackviewer
import unittest
from test.support import requires
from tkinter import Tk

from idlelib.tree import TreeNode, ScrolledCanvas


class StackBrowserTest(unittest.TestCase):

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
            abc
        except NameError as exc:
            sb = stackviewer.StackBrowser(self.root, exc)
        isi = self.assertIsInstance
        isi(stackviewer.sc, ScrolledCanvas)
        isi(stackviewer.item, stackviewer.StackTreeItem)
        isi(stackviewer.node, TreeNode)


if __name__ == '__main__':
    unittest.main(verbosity=2)
