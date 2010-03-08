# A simple vector class


def vec(*v):
    return Vec(*v)


class Vec:

    def __init__(self, *v):
        self.v = list(v)

    def fromlist(self, v):
        if not isinstance(v, list):
            raise TypeError
        self.v = v[:]
        return self

    def __repr__(self):
        return 'vec(' + repr(self.v)[1:-1] + ')'

    def __len__(self):
        return len(self.v)

    def __getitem__(self, i):
        return self.v[i]

    def __add__(self, other):
        # Element-wise addition
        v = map(lambda x, y: x+y, self, other)
        return Vec().fromlist(v)

    def __sub__(self, other):
        # Element-wise subtraction
        v = map(lambda x, y: x-y, self, other)
        return Vec().fromlist(v)

    def __mul__(self, scalar):
        # Multiply by scalar
        v = map(lambda x: x*scalar, self.v)
        return Vec().fromlist(v)



def test():
    a = vec(1, 2, 3)
    b = vec(3, 2, 1)
    print a
    print b
    print a+b
    print a-b
    print a*3.0

test()
