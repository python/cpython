##############################################################################
# Copyright (c) 2020 Zope Foundation and Contributors.
# All Rights Reserved.
#
# This software is subject to the provisions of the Zope Public License,
# Version 2.1 (ZPL).  A copy of the ZPL should accompany this distribution.
# THIS SOFTWARE IS PROVIDED "AS IS" AND ANY AND ALL EXPRESS OR IMPLIED
# WARRANTIES ARE DISCLAIMED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF TITLE, MERCHANTABILITY, AGAINST INFRINGEMENT, AND FITNESS
# FOR A PARTICULAR PURPOSE.
##############################################################################


import unittest
import numbers as abc

# Note that importing z.i.c.numbers does work on import.
from zope.interface.common import numbers

from . import add_abc_interface_tests
from . import VerifyClassMixin
from . import VerifyObjectMixin


class TestVerifyClass(VerifyClassMixin,
                      unittest.TestCase):

    def test_int(self):
        self.assertIsInstance(int(), abc.Integral)
        self.assertTrue(self.verify(numbers.IIntegral, int))

    def test_float(self):
        self.assertIsInstance(float(), abc.Real)
        self.assertTrue(self.verify(numbers.IReal, float))

add_abc_interface_tests(TestVerifyClass, numbers.INumber.__module__)


class TestVerifyObject(VerifyObjectMixin,
                       TestVerifyClass):
    pass
