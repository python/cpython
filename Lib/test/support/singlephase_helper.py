"""Helper module for _testembed.c test_subinterpreter_finalize test.
"""

import sys

PREFIX = "shared_string_"
SUFFIX = "DByGMRJRSEDp29PkiZQNHA"

def init_sub1():
    import _testsinglephase
    # Create global object to be shared when imported a second time.
    _testsinglephase._shared_list = []
    # Create a new interned string, to be shared with the main interpreter.
    _testsinglephase._shared_string = sys.intern(PREFIX + SUFFIX)


def init_sub2():
    # This sub-interpreter will share a reference to _shared_list with the
    # first interpreter, since importing _testsinglephase will not initialize
    # the module a second time but will just copy the global dict.  This
    # situtation used to trigger a bug like gh-125286 if TraceRefs was enabled
    # for the build.
    import _testsinglephase


def init_main():
    global shared_str
    # The first sub-interpreter has already interned this string value.  The
    # return value from intern() will be the same string object created in
    # sub-interpreter 1.  Assign it to a global so it lives until the main
    # interpreter is shutdown.
    shared_string = sys.intern(PREFIX + SUFFIX)
