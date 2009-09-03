"""
Does not terminate: consume all memory without responding to Ctrl-C.
I am not too sure why, but you can surely find out by gdb'ing a bit...
"""

class X(object):
    pass

X.__new__ = X
X()
