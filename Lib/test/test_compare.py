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
            return (self.arg, other)

class Cmp:
    def __init__(self,arg):
        self.arg = arg

    def __repr__(self):
        return '<Cmp %s>' % self.arg

    def __cmp__(self, other):
        return cmp(self.arg, other)

class RCmp:
    def __init__(self,arg):
        self.arg = arg

    def __repr__(self):
        return '<RCmp %s>' % self.arg

    def __rcmp__(self, other):
        return cmp(other, self.arg)
 

candidates = [2, 2.2, 2L, 2+4j, [1], (2,), None, Empty(), Coerce(3),
                Cmp(4), RCmp(5)]

def test():
    for a in candidates:
        for b in candidates:
            print "cmp(%s, %s)" % (a, b),
            try:
                x = cmp(a, b)
            except:
                print '... %s' % sys.exc_info(0)
            else:
                print '=', x

test()
