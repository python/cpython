from __future__ import nested_scopes
from __future__ import division
from __future__ import nested_scopes

def f(x):
    def g(y):
        return y // x
    return g


print f(2)(5)
