##############################################################################
#
# Copyright (c) 2001, 2002 Zope Foundation and Contributors.
# All Rights Reserved.
#
# This software is subject to the provisions of the Zope Public License,
# Version 2.1 (ZPL).  A copy of the ZPL should accompany this distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY AND ALL EXPRESS OR IMPLIED
# WARRANTIES ARE DISCLAIMED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF TITLE, MERCHANTABILITY, AGAINST INFRINGEMENT, AND FITNESS
# FOR A PARTICULAR PURPOSE.
#
##############################################################################
"""Base Mapping tests
"""
from operator import __getitem__

def testIReadMapping(self, inst, state, absent):
    for key in state:
        self.assertEqual(inst[key], state[key])
        self.assertEqual(inst.get(key, None), state[key])
        self.assertTrue(key in inst)

    for key in absent:
        self.assertEqual(inst.get(key, None), None)
        self.assertEqual(inst.get(key), None)
        self.assertEqual(inst.get(key, self), self)
        self.assertRaises(KeyError, __getitem__, inst, key)


def test_keys(self, inst, state):
    # Return the keys of the mapping object
    inst_keys = list(inst.keys()); inst_keys.sort()
    state_keys = list(state.keys()) ; state_keys.sort()
    self.assertEqual(inst_keys, state_keys)

def test_iter(self, inst, state):
    # Return the keys of the mapping object
    inst_keys = list(inst); inst_keys.sort()
    state_keys = list(state.keys()) ; state_keys.sort()
    self.assertEqual(inst_keys, state_keys)

def test_values(self, inst, state):
    # Return the values of the mapping object
    inst_values = list(inst.values()); inst_values.sort()
    state_values = list(state.values()) ; state_values.sort()
    self.assertEqual(inst_values, state_values)

def test_items(self, inst, state):
    # Return the items of the mapping object
    inst_items = list(inst.items()); inst_items.sort()
    state_items = list(state.items()) ; state_items.sort()
    self.assertEqual(inst_items, state_items)

def test___len__(self, inst, state):
    # Return the number of items
    self.assertEqual(len(inst), len(state))

def testIEnumerableMapping(self, inst, state):
    test_keys(self, inst, state)
    test_items(self, inst, state)
    test_values(self, inst, state)
    test___len__(self, inst, state)


class BaseTestIReadMapping(object):
    def testIReadMapping(self):
        inst = self._IReadMapping__sample()
        state = self._IReadMapping__stateDict()
        absent = self._IReadMapping__absentKeys()
        testIReadMapping(self, inst, state, absent)


class BaseTestIEnumerableMapping(BaseTestIReadMapping):
    # Mapping objects whose items can be enumerated
    def test_keys(self):
        # Return the keys of the mapping object
        inst = self._IEnumerableMapping__sample()
        state = self._IEnumerableMapping__stateDict()
        test_keys(self, inst, state)

    def test_values(self):
        # Return the values of the mapping object
        inst = self._IEnumerableMapping__sample()
        state = self._IEnumerableMapping__stateDict()
        test_values(self, inst, state)

    def test_items(self):
        # Return the items of the mapping object
        inst = self._IEnumerableMapping__sample()
        state = self._IEnumerableMapping__stateDict()
        test_items(self, inst, state)

    def test___len__(self):
        # Return the number of items
        inst = self._IEnumerableMapping__sample()
        state = self._IEnumerableMapping__stateDict()
        test___len__(self, inst, state)

    def _IReadMapping__stateDict(self):
        return self._IEnumerableMapping__stateDict()

    def _IReadMapping__sample(self):
        return self._IEnumerableMapping__sample()

    def _IReadMapping__absentKeys(self):
        return self._IEnumerableMapping__absentKeys()
