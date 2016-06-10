''' Test idlelib.tree.

Coverage: 56%
'''
from idlelib import tree
from test.support import requires
requires('gui')
import os
import unittest
from tkinter import Tk


class TreeTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root

    def test_init(self):
        # Start with code slightly adapted from htest.
        sc = tree.ScrolledCanvas(
            self.root, bg="white", highlightthickness=0, takefocus=1)
        sc.frame.pack(expand=1, fill="both", side='left')
        item = tree.FileTreeItem(tree.ICONDIR)
        node = tree.TreeNode(sc.canvas, None, item)
        node.expand()


if __name__ == '__main__':
    unittest.main(verbosity=2)
