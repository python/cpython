# Calls to PyIter_Next, or direct calls to tp_iternext, on an object
# which might no longer be an iterable because its 'next' method was
# removed.  These are all variants of Issue3720.

"""
Run this script with an argument between 1 and <N> to test for
different crashes.
"""
N = 8

import sys

class Foo(object):
    def __iter__(self):
        return self
    def next(self):
        del Foo.next
        return (1, 2)

def case1():
    list(enumerate(Foo()))

def case2():
    x, y = Foo()

def case3():
    filter(None, Foo())

def case4():
    map(None, Foo(), Foo())

def case5():
    max(Foo())

def case6():
    sum(Foo(), ())

def case7():
    dict(Foo())

def case8():
    sys.stdout.writelines(Foo())

# etc...


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print __doc__.replace('<N>', str(N))
    else:
        n = int(sys.argv[1])
        func = globals()['case%d' % n]
        func()
