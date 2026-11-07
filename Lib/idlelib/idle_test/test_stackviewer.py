"Test stackviewer, coverage 63%."

from idlelib import stackviewer
import unittest
from test.support import requires
from tkinter import Tk

from idlelib.tree import TreeWidget


class StackBrowserTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):

        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):

        cls.root.update_idletasks()
##        for id in cls.root.after_info():
##            cls.root.after_cancel(id)  # Need for EditorWindow.
        cls.root.destroy()
        del cls.root

    def test_init(self):
        try:
            abc
        except NameError as exc:
            stackviewer.StackBrowser(self.root, exc)
        widget = stackviewer.tree
        isi = self.assertIsInstance
        isi(widget, TreeWidget)
        isi(stackviewer.item, stackviewer.StackTreeItem)
        top = widget.winfo_toplevel()
        self.assertEqual(top.winfo_class(), 'Idle')
        # The root row is the exception; its children are the frames.
        self.assertEqual(widget.tree.item(widget.root, 'text'),
                         "NameError: name 'abc' is not defined")
        self.assertEqual(len(widget.tree.get_children(widget.root)), 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
