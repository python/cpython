
# http://python.org/sf/1202533

import new, operator

class A:
    pass
A.__mul__ = new.instancemethod(operator.mul, None, A)

if __name__ == '__main__':
    A()*2   # segfault: infinite recursion in C
