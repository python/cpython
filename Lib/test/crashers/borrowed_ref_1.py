"""
_PyType_Lookup() returns a borrowed reference.
This attacks the call in dictobject.c.
"""

class A(object):
    pass

class B(object):
    def __del__(self):
        print('hi')
        del D.__missing__

class D(dict):
    class __missing__:
        def __init__(self, *args):
            pass


d = D()
a = A()
a.cycle = a
a.other = B()
del a

prev = None
while 1:
    d[5]
    prev = (prev,)
