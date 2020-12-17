##############################################################################
#
# Copyright (c) 2002 Zope Foundation and Contributors.
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
"""Adapter-style interface registry

See Adapter class.
"""
from zope.interface import Declaration

def _flatten(implements, include_None=0):

    try:
        r = implements.flattened()
    except AttributeError:
        if implements is None:
            r=()
        else:
            r = Declaration(implements).flattened()

    if not include_None:
        return r

    r = list(r)
    r.append(None)
    return r
