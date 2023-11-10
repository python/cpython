"""This is a test"""

# Import the name nested_scopes twice to trigger SF bug #407394 (regression).
from __future__ import nested_scopes, nested_scopes

def f(x):
    def g(y):
        return x + y
    return g

result = f(2)(4)
