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
import io as abc

# Note that importing z.i.c.io does work on import.
from zope.interface.common import io

from . import add_abc_interface_tests
from . import VerifyClassMixin
from . import VerifyObjectMixin


class TestVerifyClass(VerifyClassMixin,
                      unittest.TestCase):
    pass

add_abc_interface_tests(TestVerifyClass, io.IIOBase.__module__)


class TestVerifyObject(VerifyObjectMixin,
                       TestVerifyClass):
    CONSTRUCTORS = {
        abc.BufferedWriter: lambda: abc.BufferedWriter(abc.StringIO()),
        abc.BufferedReader: lambda: abc.BufferedReader(abc.StringIO()),
        abc.TextIOWrapper: lambda: abc.TextIOWrapper(abc.BytesIO()),
        abc.BufferedRandom: lambda: abc.BufferedRandom(abc.BytesIO()),
        abc.BufferedRWPair: lambda: abc.BufferedRWPair(abc.BytesIO(), abc.BytesIO()),
        abc.FileIO: lambda: abc.FileIO(__file__),
        '_WindowsConsoleIO': unittest.SkipTest,
    }

    try:
        import cStringIO
    except ImportError:
        pass
    else:
        CONSTRUCTORS.update({
            cStringIO.InputType: lambda cStringIO=cStringIO: cStringIO.StringIO('abc'),
            cStringIO.OutputType: cStringIO.StringIO,
        })
