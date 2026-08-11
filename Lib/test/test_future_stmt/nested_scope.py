"""This is a test"""

from __future__ import nested_scopes; import site  # noqa: F401

def f(x):
    def g(y):
        return x + y
    return g

result = f(2)(4)
