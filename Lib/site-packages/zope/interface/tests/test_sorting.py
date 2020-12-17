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
"""Test interface sorting
"""

import unittest

from zope.interface import Interface

class I1(Interface): pass
class I2(I1): pass
class I3(I1): pass
class I4(Interface): pass
class I5(I4): pass
class I6(I2): pass


class Test(unittest.TestCase):

    def test(self):
        l = [I1, I3, I5, I6, I4, I2]
        l.sort()
        self.assertEqual(l, [I1, I2, I3, I4, I5, I6])

    def test_w_None(self):
        l = [I1, None, I3, I5, I6, I4, I2]
        l.sort()
        self.assertEqual(l, [I1, I2, I3, I4, I5, I6, None])

    def test_w_equal_names(self):
        # interfaces with equal names but different modules should sort by
        # module name
        from zope.interface.tests.m1 import I1 as m1_I1
        l = [I1, m1_I1]
        l.sort()
        self.assertEqual(l, [m1_I1, I1])

    def test_I1_I2(self):
        self.assertLess(I1.__name__, I2.__name__)
        self.assertEqual(I1.__module__, I2.__module__)
        self.assertEqual(I1.__module__, __name__)
        self.assertLess(I1, I2)

    def _makeI1(self):
        class I1(Interface):
            pass
        return I1

    def test_nested(self):
        nested_I1 = self._makeI1()
        self.assertEqual(I1, nested_I1)
        self.assertEqual(nested_I1, I1)
        self.assertEqual(hash(I1), hash(nested_I1))
