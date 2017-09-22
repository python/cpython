""" Test idlelib.browser.
"""

import os.path
import unittest
import pyclbr

from idlelib import browser, filelist
from idlelib.tree import TreeNode
from test.support import requires
from unittest import mock
from tkinter import Tk
from idlelib.idle_test.mock_idle import Func
from collections import deque


class ClassBrowserTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = Tk()
        cls.root.withdraw()
        cls.flist = filelist.FileList(cls.root)
        cls.file = __file__
        cls.path = os.path.dirname(cls.file)
        cls.module = os.path.basename(cls.file).rstrip('.py')
        cls.cb = browser.ClassBrowser(cls.flist, cls.module, [cls.path], _utest=True)

    @classmethod
    def tearDownClass(cls):
        cls.cb.close()
        cls.root.destroy()
        del cls.root, cls.flist, cls.cb

    def test_init(self):
        cb = self.cb
        eq = self.assertEqual
        eq(cb.name, self.module)
        eq(cb.file, self.file)
        eq(cb.flist, self.flist)
        eq(pyclbr._modules, {})
        self.assertIsInstance(cb.node, TreeNode)

    def test_settitle(self):
        cb = self.cb
        self.assertIn(self.module, cb.top.title())
        self.assertEqual(cb.top.iconname(), 'Class Browser')

    def test_rootnode(self):
        cb = self.cb
        rn = cb.rootnode()
        self.assertIsInstance(rn, browser.ModuleBrowserTreeItem)

    def test_close(self):
        cb = self.cb
        cb.top.destroy = Func()
        cb.node.destroy = Func()
        cb.close()
        self.assertTrue(cb.top.destroy.called)
        self.assertTrue(cb.node.destroy.called)
        del cb.top.destroy, cb.node.destroy


# Same nested tree creation as in test_pyclbr.py except for super on C0.
mb = pyclbr
module, fname = 'test', 'test.py'
f0 = mb.Function(module, 'f0', fname, 1)
f1 = mb._nest_function(f0, 'f1', 2)
f2 = mb._nest_function(f1, 'f2', 3)
c1 = mb._nest_class(f0, 'c1', 5)
C0 = mb.Class(module, 'C0', ['base'], fname, 6)
F1 = mb._nest_function(C0, 'F1', 8)
C1 = mb._nest_class(C0, 'C1', 11, [''])
C2 = mb._nest_class(C1, 'C2', 12)
F3 = mb._nest_function(C2, 'F3', 14)
mock_pyclbr_tree = {'f0': f0, 'C0': C0}


class TraverseNodeTest(unittest.TestCase):

    def test__traverse_node(self):
        # Nothing to traverse if parameter name isn't same as tree module.
        tn = browser._traverse_node(mock_pyclbr_tree, 'different name')
        self.assertEqual(tn, ({}, []))

        # Parameter matches tree module.
        tn = browser._traverse_node(mock_pyclbr_tree, 'test')
        expected = ({'f0': f0, 'C0(base)': C0}, [(1, 'f0'), (6, 'C0(base)')])
        self.assertEqual(tn, expected)

        # No name parameter.
        tn = browser._traverse_node({'f1': f1})
        expected = ({'f1': f1}, [(2, 'f1')])
        self.assertEqual(tn, expected)


class ModuleBrowserTreeItemTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.orig_readmodule_ex = browser.pyclbr.readmodule_ex
        browser.pyclbr.readmodule_ex = mock.Mock(return_value=mock_pyclbr_tree)
        cls.mbt = browser.ModuleBrowserTreeItem(fname)

    @classmethod
    def tearDownClass(cls):
        browser.pyclbr.readmodule_ex = cls.orig_readmodule_ex
        del cls.orig_readmodule_ex, cls.mbt

    def test_init(self):
        mbt = self.mbt
        self.assertEqual(mbt.file, fname)

    def test_gettext(self):
        mbt = self.mbt
        self.assertEqual(mbt.GetText(), fname)

    def test_geticonname(self):
        mbt = self.mbt
        self.assertEqual(mbt.GetIconName(), 'python')

    def test_isexpandable(self):
        mbt = self.mbt
        self.assertTrue(mbt.IsExpandable())

    def test_listchildren(self):
        mbt = self.mbt
        expected_names = ['f0', 'C0(base)']
        expected_classes = {'f0': f0, 'C0(base)': C0}
        self.assertNotEqual(mbt.classes, expected_classes)

        items = mbt.listchildren()
        self.assertEqual(items, expected_names)
        # self.classes is set in listchildren.
        self.assertEqual(mbt.classes, expected_classes)

    def test_getsublist(self):
        mbt = self.mbt
        expected_names = ['f0', 'C0(base)']
        expected_classes = {'f0': f0, 'C0(base)': C0}
        sublist = mbt.GetSubList()
        for index, item in enumerate(sublist):
            self.assertIsInstance(item, browser.ChildBrowserTreeItem)
            self.assertEqual(sublist[index].name, expected_names[index])
            self.assertEqual(sublist[index].classes, expected_classes)
            self.assertEqual(sublist[index].file, fname)
        mbt.classes.clear()

    def test_ondoubleclick(self):
        mbt = self.mbt
        fopen = browser.file_open = mock.Mock()

        with mock.patch('os.path.exists', return_value=False):
            mbt.OnDoubleClick()
            fopen.assert_not_called()

        with mock.patch('os.path.exists', return_value=True):
            mbt.OnDoubleClick()
            fopen.assert_called()
            fopen.called_with(fname)

        del browser.file_open


class ChildBrowserTreeItemTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.cbt_C0 = browser.ChildBrowserTreeItem('C0', mock_pyclbr_tree, fname)
        cls.f1_classes = {'f1': f1}
        cls.cbt_f1 = browser.ChildBrowserTreeItem('f1', cls.f1_classes, fname)
        cls.F1_classes = {'F1': F1}
        cls.cbt_F1 = browser.ChildBrowserTreeItem('F1', cls.F1_classes, fname)
        cls.cbt_C1 = browser.ChildBrowserTreeItem('F1', {'C1': C1}, fname)

    @classmethod
    def tearDownClass(cls):
        del cls.cbt_C0, cls.cbt_f1, cls.cbt_F1, cls.cbt_C1

    def test_init(self):
        cbt_C0 = self.cbt_C0
        cbt_f1 = self.cbt_f1
        eq = self.assertEqual

        eq(cbt_C0.name, 'C0')
        eq(cbt_C0.classes, mock_pyclbr_tree)
        eq(cbt_C0.file, fname)
        eq(cbt_C0.cl, mock_pyclbr_tree['C0'])
        self.assertFalse(cbt_C0.isfunction)

        eq(cbt_f1.name, 'f1')
        eq(cbt_f1.classes, self.f1_classes)
        eq(cbt_f1.file, fname)
        eq(cbt_f1.cl, self.f1_classes['f1'])
        self.assertTrue(cbt_f1.isfunction)

    def test_gettext(self):
        self.assertEqual(self.cbt_C0.GetText(), 'class C0')
        self.assertEqual(self.cbt_f1.GetText(), 'def f1(...)')

    def test_geticonname(self):
        self.assertEqual(self.cbt_C0.GetIconName(), 'folder')
        self.assertEqual(self.cbt_f1.GetIconName(), 'python')

    def test_isexpandable(self):
        self.assertTrue(self.cbt_C0.IsExpandable())
        self.assertTrue(self.cbt_f1.IsExpandable())
        self.assertFalse(self.cbt_F1.IsExpandable())

    def test_listchildren(self):
        self.assertEqual(self.cbt_C0.listchildren(), [({'F1': F1}, 'F1'),
                                                      ({'C1()': C1}, 'C1()')])
        self.assertEqual(self.cbt_f1.listchildren(), [({'f2': f2}, 'f2')])
        self.assertEqual(self.cbt_F1.listchildren(), [])

    def test_getsublist(self):
        eq = self.assertEqual
        # When GetSubList is called, additional ChildBrowserTreeItem
        # instances are created for children of the current node.
        # C0 has childen F1 and C1()   (test class with children)
        C0_expected = [({'F1': F1}, 'F1'), ({'C1()': C1}, 'C1()')]
        for i, sublist in enumerate(self.cbt_C0.GetSubList()):
            self.assertIsInstance(sublist, browser.ChildBrowserTreeItem)
            eq(sublist.name, C0_expected[i][1])
            eq(sublist.classes, C0_expected[i][0])
            eq(sublist.file, fname)
            eq(sublist.cl, C0_expected[i][0][sublist.name])

        # f1 has children f2.   (test function with children)
        for sublist in self.cbt_f1.GetSubList():
            self.assertIsInstance(sublist, browser.ChildBrowserTreeItem)
            eq(sublist.name, 'f2')
            eq(sublist.classes, {'f2': f2})
            eq(sublist.file, fname)
            eq(sublist.cl, f2)

        # F1 has no children.
        eq(self.cbt_F1.GetSubList(), [])

    def test_ondoubleclick(self):
        fopen = browser.file_open = mock.Mock()
        goto = fopen.return_value.gotoline = mock.Mock()

        with mock.patch('os.path.exists', return_value=False):
            self.cbt_C0.OnDoubleClick()
            fopen.assert_not_called()

        with mock.patch('os.path.exists', return_value=True):
            self.cbt_F1.OnDoubleClick()
            fopen.assert_called()
            goto.assert_called()
            goto.assert_called_with(self.cbt_F1.cl.lineno)

        del browser.file_open


##class NestedChildrenTest(unittest.TestCase):
##    "Test that all the nodes in a nested tree are added to the BrowserTree."
##
##    @classmethod
##    def setUpClass(cls):
##        cls.orig_readmodule_ex = browser.pyclbr.readmodule_ex
##        browser.pyclbr.readmodule_ex = mock.Mock(return_value=mock_pyclbr_tree)
##        cls.sublist = browser.ModuleBrowserTreeItem(fname).GetSubList()
##
##    @classmethod
##    def tearDownClass(cls):
##        browser.pyclbr.readmodule_ex = cls.orig_readmodule_ex
##        del cls.orig_readmodule_ex, cls.sublist
##
##    def test_nested(self):
##        queue = deque()
##        actual_names = []
##        # The tree items are processed in breadth first order.
##        # Verify that processing each sublist hits every node and
##        # in the right order.
##        expected_names = ['f0', 'C0(base)',
##                          'f1', 'c1', 'F1', 'C1()',
##                          'f2', 'C2',
##                          'F3']
##        queue.extend(self.sublist)
##        while queue:
##            cb = queue.popleft()
##            sublist = cb.GetSubList()
##            queue.extend(sublist)
##            self.assertIn(cb.name, cb.GetText())
##            self.assertIn(cb.GetIconName(), ('python', 'folder'))
##            self.assertIs(cb.IsExpandable(), sublist != [])
##            actual_names.append(cb.name)
##        self.assertEqual(actual_names, expected_names)


if __name__ == '__main__':
    unittest.main(verbosity=2)
