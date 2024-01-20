# Tests for xml.dom.minicompat

import copy
import pickle
import unittest

import xml.dom
from xml.dom.minicompat import *


class EmptyNodeListTestCase(unittest.TestCase):
    """Tests for the EmptyNodeList class."""

    def test_emptynodelist_item(self):
        # Test item access on an EmptyNodeList.
        node_list = EmptyNodeList()

        self.assertIsNone(node_list.item(0))
        self.assertIsNone(node_list.item(-1)) # invalid item

        with self.assertRaises(IndexError):
            node_list[0]
        with self.assertRaises(IndexError):
            node_list[-1]

    def test_emptynodelist_length(self):
        node_list = EmptyNodeList()
        # Reading
        self.assertEqual(node_list.length, 0)
        # Writing
        with self.assertRaises(xml.dom.NoModificationAllowedErr):
            node_list.length = 111

    def test_emptynodelist___add__(self):
        node_list = EmptyNodeList() + NodeList()
        self.assertEqual(node_list, NodeList())

    def test_emptynodelist___radd__(self):
        node_list = [1,2] + EmptyNodeList()
        self.assertEqual(node_list, [1,2])


class NodeListTestCase(unittest.TestCase):
    """Tests for the NodeList class."""

    def test_nodelist_item(self):
        # Test items access on a NodeList.
        # First, use an empty NodeList.
        node_list = NodeList()

        self.assertIsNone(node_list.item(0))
        self.assertIsNone(node_list.item(-1))

        with self.assertRaises(IndexError):
            node_list[0]
        with self.assertRaises(IndexError):
            node_list[-1]

        # Now, use a NodeList with items.
        node_list.append(111)
        node_list.append(999)

        self.assertEqual(node_list.item(0), 111)
        self.assertIsNone(node_list.item(-1)) # invalid item

        self.assertEqual(node_list[0], 111)
        self.assertEqual(node_list[-1], 999)

    def test_nodelist_length(self):
        node_list = NodeList([1, 2])
        # Reading
        self.assertEqual(node_list.length, 2)
        # Writing
        with self.assertRaises(xml.dom.NoModificationAllowedErr):
            node_list.length = 111

    def test_nodelist___add__(self):
        node_list = NodeList([3, 4]) + [1, 2]
        self.assertEqual(node_list, NodeList([3, 4, 1, 2]))

    def test_nodelist___radd__(self):
        node_list = [1, 2] + NodeList([3, 4])
        self.assertEqual(node_list, NodeList([1, 2, 3, 4]))

    def test_nodelist_pickle_roundtrip(self):
        # Test pickling and unpickling of a NodeList.

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            # Empty NodeList.
            node_list = NodeList()
            pickled = pickle.dumps(node_list, proto)
            unpickled = pickle.loads(pickled)
            self.assertIsNot(unpickled, node_list)
            self.assertEqual(unpickled, node_list)

            # Non-empty NodeList.
            node_list.append(1)
            node_list.append(2)
            pickled = pickle.dumps(node_list, proto)
            unpickled = pickle.loads(pickled)
            self.assertIsNot(unpickled, node_list)
            self.assertEqual(unpickled, node_list)

    def test_nodelist_copy(self):
        # Empty NodeList.
        node_list = NodeList()
        copied = copy.copy(node_list)
        self.assertIsNot(copied, node_list)
        self.assertEqual(copied, node_list)

        # Non-empty NodeList.
        node_list.append([1])
        node_list.append([2])
        copied = copy.copy(node_list)
        self.assertIsNot(copied, node_list)
        self.assertEqual(copied, node_list)
        for x, y in zip(copied, node_list):
            self.assertIs(x, y)

    def test_nodelist_deepcopy(self):
        # Empty NodeList.
        node_list = NodeList()
        copied = copy.deepcopy(node_list)
        self.assertIsNot(copied, node_list)
        self.assertEqual(copied, node_list)

        # Non-empty NodeList.
        node_list.append([1])
        node_list.append([2])
        copied = copy.deepcopy(node_list)
        self.assertIsNot(copied, node_list)
        self.assertEqual(copied, node_list)
        for x, y in zip(copied, node_list):
            self.assertIsNot(x, y)
            self.assertEqual(x, y)

if __name__ == '__main__':
    unittest.main()
