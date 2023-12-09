"""This is a test"""
"this isn't a doc string"
from __future__ import nested_scopes

def f(x):
    def g(y):
        return x + y
    return g

result = f(2)(4)
