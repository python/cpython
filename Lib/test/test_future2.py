"""This is a test"""

from __future__ import nested_scopes; import string

def f(x):
    def g(y):
        return x + y
    return g

result = f(2)(4)
