#!/usr/bin/env python
import sys
from test_support import verbose
import string_tests
# UserString is a wrapper around the native builtin string type.
# UserString instances should behave similar to builtin string objects.
# The test cases were in part derived from 'test_string.py'.
from UserString import UserString

if __name__ == "__main__":
    verbose = '-v' in sys.argv

tested_methods = {}

def test(methodname, input, output, *args):
    global tested_methods
    tested_methods[methodname] = 1
    if verbose:
        print '%r.%s(%s)' % (input, methodname, ", ".join(map(repr, args))),
    u = UserString(input)
    objects = [input, u, UserString(u)]
    res = [""] * 3
    for i in range(3):
        object = objects[i]
        try:
            f = getattr(object, methodname)
        except AttributeError:
            f = None
            res[i] = AttributeError
        else:
            try:
                res[i] = apply(f, args)
            except:
                res[i] = sys.exc_type
    if res[0] == res[1] == res[2] == output:
        if verbose:
            print 'yes'
    else:
        if verbose:
            print 'no'
        print (methodname, input, output, args, res[0], res[1], res[2])

string_tests.run_method_tests(test)
