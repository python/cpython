
# http://python.org/sf/1202533

class A(str):
    __get__ = getattr

if __name__ == '__main__':
    a = A('a')
    A.a = a
    a.a   # segfault: infinite recursion in C
