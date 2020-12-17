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
import sys

from zope.interface.advice import addClassAdvisor
from zope.interface.advice import getFrameInfo

my_globals = globals()

def ping(log, value):

    def pong(klass):
        log.append((value,klass))
        return [klass]

    addClassAdvisor(pong)

try:
    from types import ClassType

    class ClassicClass:
        __metaclass__ = ClassType
        classLevelFrameInfo = getFrameInfo(sys._getframe())
except ImportError:
    ClassicClass = None

class NewStyleClass:
    __metaclass__ = type
    classLevelFrameInfo = getFrameInfo(sys._getframe())

moduleLevelFrameInfo = getFrameInfo(sys._getframe())
