#! /usr/bin/env python
"""Find the maximum recursion limit that prevents core dumps

This script finds the maximum safe recursion limit on a particular
platform.  If you need to change the recursion limit on your system,
this script will tell you a safe upper bound.  To use the new limit,
call sys.setrecursionlimit.

This module implements several ways to create infinite recursion in
Python.  Different implementations end up pushing different numbers of
C stack frames, depending on how many calls through Python's abstract
C API occur.

After each round of tests, it prints a message
Limit of NNNN is fine.

It ends when Python causes a segmentation fault because the limit is
too high.  On platforms like Mac and Windows, it should exit with a
MemoryError.

NB: A program that does not use __methods__ can set a higher limit.
"""

import sys

class RecursiveBlowup1:
    def __init__(self):
        self.__init__()

def test_init():
    return RecursiveBlowup1()

class RecursiveBlowup2:
    def __repr__(self):
        return repr(self)

def test_repr():
    return repr(RecursiveBlowup2())

class RecursiveBlowup4:
    def __add__(self, x):
        return x + self

def test_add():
    return RecursiveBlowup4() + RecursiveBlowup4()

class RecursiveBlowup5:
    def __getattr__(self, attr):
        return getattr(self, attr)

def test_getattr():
    return RecursiveBlowup5().attr

class RecursiveBlowup6:
    def __getitem__(self, item):
        return self[item - 2] + self[item - 1]

def test_getitem():
    return RecursiveBlowup6()[5]
    
def test_recurse():
    return test_recurse()

def check_limit(n, test_func_name):
    sys.setrecursionlimit(n)
    if test_func_name.startswith("test_"):
        print test_func_name[5:]
    else:
        print test_func_name
    test_func = globals()[test_func_name]
    try:
        test_func()
    except RuntimeError:
        pass
    else:
        print "Yikes!"

limit = 1000
while 1:
    check_limit(limit, "test_recurse")
    check_limit(limit, "test_add")
    check_limit(limit, "test_repr")
    check_limit(limit, "test_init")
    check_limit(limit, "test_getattr")
    check_limit(limit, "test_getitem")
    print "Limit of %d is fine" % limit
    limit = limit + 100
    
