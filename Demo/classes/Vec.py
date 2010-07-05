# A simple vector class

import operator


class Vec:

    def __init__(self, *v):
        self.v = list(v)

    @classmethod
    def fromlist(cls, v):
        if not isinstance(v, list):
            raise TypeError
        inst = cls()
        inst.v = v
        return inst

    def __repr__(self):
        return 'Vec(' + repr(self.v)[1:-1] + ')'

    def __len__(self):
        return len(self.v)

    def __getitem__(self, i):
        return self.v[i]

    def __add__(self, other):
        # Element-wise addition
        v = list(map(operator.add, self, other))
        return Vec.fromlist(v)

    def __sub__(self, other):
        # Element-wise subtraction
        v = list(map(operator.sub, self, other))
        return Vec.fromlist(v)

    def __mul__(self, scalar):
        # Multiply by scalar
        v = [x*scalar for x in self.v]
        return Vec.fromlist(v)



def test():
    a = Vec(1, 2, 3)
    b = Vec(3, 2, 1)
    print(a)
    print(b)
    print(a+b)
    print(a-b)
    print(a*3.0)

test()
