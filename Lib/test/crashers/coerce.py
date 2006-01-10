
# http://python.org/sf/992017

class foo:
    def __coerce__(self, other):
        return other, self

if __name__ == '__main__':
    foo()+1   # segfault: infinite recursion in C
