""" Test idlelib.browser.
"""

from idlelib import browser
from test.support import requires
requires('gui')
import unittest
from unittest import mock
from tkinter import Tk
from collections import deque
import pyclbr
from textwrap import dedent


class ClassBrowserTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.root = Tk()
        cls.root.withdraw()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root


class BrowserTreeItemTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        mb = pyclbr
        # Set arguments for descriptor creation and _create_tree call.
        # Same as test_nested in test_pyclbr with the addition of a super
        # for C0.
        m, p, cls.f, t, i = 'test', '', 'test.py', {}, None
        source = dedent("""\
        def f0:
            def f1(a,b,c):
                def f2(a=1, b=2, c=3): pass
                    return f1(a,b,d)
            class c1: pass
        class C0(base):
            "Test class."
            def F1():
                "Method."
                return 'return'
            class C1():
                class C2:
                    "Class nested within nested class."
                    def F3(): return 1+1
        """)
        mock_pyclbr_tree = mb._create_tree(m, p, cls.f, source, t, i)

        cls.save_readmodule_ex = browser.pyclbr.readmodule_ex
        browser.pyclbr.readmodule_ex = mock.Mock(return_value=mock_pyclbr_tree)
        cls.mbt = browser.ModuleBrowserTreeItem(cls.f)
        cls.sublist = cls.mbt.GetSubList()

    @classmethod
    def tearDownClass(cls):
        del cls.sublist, cls.mbt
        browser.pyclbr.readmodule_ex = cls.save_readmodule_ex
        del cls.save_readmodule_ex

    def test_ModuleBrowserTreeItem(self):
        self.assertEqual(self.mbt.GetText(), self.f)
        self.assertEqual(self.mbt.GetIconName(), 'python')
        self.assertTrue(self.mbt.IsExpandable())

        self.assertEqual('f0', self.sublist[0].name)
        self.assertEqual('C0(base)', self.sublist[1].name)
        for s in self.sublist:
            self.assertIsInstance(s, browser.ChildBrowserTreeItem)

    def test_ChildBrowserTreeItem(self):
        queue = deque()
        actual_names = []
        # The tree items are processed in breadth first order.
        # Verify that processing each sublist hits every node and
        # in the right order.
        expected_names = ['f0', 'C0(base)',
                          'f1', 'c1', 'F1', 'C1()',
                          'f2', 'C2',
                          'F3']
        queue.extend(self.sublist)
        while queue:
            cb = queue.popleft()
            sublist = cb.GetSubList()
            queue.extend(sublist)
            self.assertIn(cb.name, cb.GetText())
            self.assertIn(cb.GetIconName(), ('python', 'folder'))
            self.assertIs(cb.IsExpandable(), sublist != [])
            actual_names.append(cb.name)
        self.assertEqual(actual_names, expected_names)

if __name__ == '__main__':
    unittest.main(verbosity=2)
