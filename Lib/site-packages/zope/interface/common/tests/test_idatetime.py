##############################################################################
#
# Copyright (c) 2003 Zope Foundation and Contributors.
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
"""Test for datetime interfaces
"""

import unittest

from zope.interface.verify import verifyObject, verifyClass
from zope.interface.common.idatetime import ITimeDelta, ITimeDeltaClass
from zope.interface.common.idatetime import IDate, IDateClass
from zope.interface.common.idatetime import IDateTime, IDateTimeClass
from zope.interface.common.idatetime import ITime, ITimeClass, ITZInfo
from datetime import timedelta, date, datetime, time, tzinfo

class TestDateTimeInterfaces(unittest.TestCase):

    def test_interfaces(self):
        verifyObject(ITimeDelta, timedelta(minutes=20))
        verifyObject(IDate, date(2000, 1, 2))
        verifyObject(IDateTime, datetime(2000, 1, 2, 10, 20))
        verifyObject(ITime, time(20, 30, 15, 1234))
        verifyObject(ITZInfo, tzinfo())
        verifyClass(ITimeDeltaClass, timedelta)
        verifyClass(IDateClass, date)
        verifyClass(IDateTimeClass, datetime)
        verifyClass(ITimeClass, time)
