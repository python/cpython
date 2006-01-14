
# http://python.org/sf/1267884

import types

class C:
    __str__ = types.InstanceType.__str__

if __name__ == '__main__':
    str(C())   # segfault: infinite recursion in C
