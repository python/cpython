# -*- Mode: Python; tab-width: 4 -*-
#
# Secret Labs' Regular Expression Engine
# $Id$
#
# re-compatible interface for the sre matching engine
#
# Copyright (c) 1998-2000 by Secret Labs AB.  All rights reserved.
#
# This code can only be used for 1.6 alpha testing.  All other use
# require explicit permission from Secret Labs AB.
#
# Portions of this engine have been developed in cooperation with
# CNRI.  Hewlett-Packard provided funding for 1.6 integration and
# other compatibility work.
#

"""
this is a long string
"""

import sre_compile

# --------------------------------------------------------------------
# public interface

def compile(pattern, flags=0):
    return sre_compile.compile(pattern, _fixflags(flags))

def match(pattern, string, flags=0):
    return compile(pattern, _fixflags(flags)).match(string)

def search(pattern, string, flags=0):
    return compile(pattern, _fixflags(flags)).search(string)

# FIXME: etc

# --------------------------------------------------------------------
# helpers

def _fixflags(flags):
    # convert flag bitmask to sequence
    assert not flags
    return ()

