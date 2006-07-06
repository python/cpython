"""
_PyType_Lookup() returns a borrowed reference.
This attacks PyObject_GenericSetAttr().

NB. on my machine this crashes in 2.5 debug but not release.
"""

class A(object):
    pass

class B(object):
    def __del__(self):
        print "hi"
        del C.d

class D(object):
    def __set__(self, obj, value):
        self.hello = 42

class C(object):
    d = D()

    def g():
        pass


c = C()
a = A()
a.cycle = a
a.other = B()

lst = [None] * 1000000
i = 0
del a
while 1:
    c.d = 42         # segfaults in PyMethod_New(im_func=D.__set__, im_self=d)
    lst[i] = c.g     # consume the free list of instancemethod objects
    i += 1
