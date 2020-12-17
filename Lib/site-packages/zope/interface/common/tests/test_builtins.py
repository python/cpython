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
from __future__ import absolute_import

import unittest

from zope.interface._compat import PYTHON2 as PY2
from zope.interface.common import builtins

from . import VerifyClassMixin
from . import VerifyObjectMixin
from . import add_verify_tests


class TestVerifyClass(VerifyClassMixin,
                      unittest.TestCase):
    pass


add_verify_tests(TestVerifyClass, (
    (builtins.IList, (list,)),
    (builtins.ITuple, (tuple,)),
    (builtins.ITextString, (type(u'abc'),)),
    (builtins.IByteString, (bytes,)),
    (builtins.INativeString, (str,)),
    (builtins.IBool, (bool,)),
    (builtins.IDict, (dict,)),
    (builtins.IFile, (file,) if PY2 else ()),
))


class TestVerifyObject(VerifyObjectMixin,
                       TestVerifyClass):
    CONSTRUCTORS = {
        builtins.IFile: lambda: open(__file__)
    }
