#!/usr/bin/env python
import sys, string
from test_support import verbose
import string_tests
# UserString is a wrapper around the native builtin string type.
# UserString instances should behave similar to builtin string objects.
# The test cases were in part derived from 'test_string.py'.
from UserString import UserString

if __name__ == "__main__":
    verbose = 0

tested_methods = {}

def test(methodname, input, *args):
    global tested_methods
    tested_methods[methodname] = 1
    if verbose:
        print '%s.%s(%s) ' % (input, methodname, args),
    u = UserString(input)
    objects = [input, u, UserString(u)]
    res = [""] * 3
    for i in range(3):
        object = objects[i]
        try:
            f = getattr(object, methodname)
            res[i] = apply(f, args)
        except:
            res[i] = sys.exc_type
    if res[0] != res[1]:
        if verbose:
            print 'no'
        print `input`, f, `res[0]`, "<>", `res[1]`
    else:
        if verbose:
            print 'yes'
    if res[1] != res[2]:
        if verbose:
            print 'no'
        print `input`, f, `res[1]`, "<>", `res[2]`
    else:
        if verbose:
            print 'yes'

string_tests.run_method_tests(test)
