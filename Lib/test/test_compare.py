import sys

from test_support import *

class Empty:
    def __repr__(self):
        return '<Empty>'

class Coerce:
    def __init__(self, arg):
        self.arg = arg

    def __repr__(self):
        return '<Coerce %s>' % self.arg

    def __coerce__(self, other):
        if isinstance(other, Coerce):
            return self.arg, other.arg
        else:
            return self.arg, other

class Cmp:
    def __init__(self,arg):
        self.arg = arg

    def __repr__(self):
        return '<Cmp %s>' % self.arg

    def __cmp__(self, other):
        return cmp(self.arg, other)


candidates = [2, 2.0, 2L, 2+0j, [1], (3,), None, Empty(), Coerce(2), Cmp(2.0)]

def test():
    for a in candidates:
        for b in candidates:
            try:
                x = a == b
            except:
                print 'cmp(%s, %s) => %s' % (a, b, sys.exc_info()[0])
            else:
                if x:
                    print "%s == %s" % (a, b)
                else:
                    print "%s != %s" % (a, b)
    # Ensure default comparison compares id() of args
    L = []
    for i in range(10):
        L.insert(len(L)//2, Empty())
    for a in L:
        for b in L:
            if cmp(a, b) != cmp(id(a), id(b)):
                print "ERROR:", cmp(a, b), cmp(id(a), id(b)), id(a), id(b)

test()
