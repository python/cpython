
# http://python.org/sf/1202533

class A(object):
    pass
A.__call__ = A()

if __name__ == '__main__':
    A()()   # segfault: infinite recursion in C
