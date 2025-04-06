"""This is a test"""
import __future__
from __future__ import nested_scopes

def f(x):
    def g(y):
        return x + y
    return g

result = f(2)(4)
