#!/usr/bin/env python3

"""
A demonstration of classes and their special methods in Python.
"""

class Vec:
    """A simple vector class.

    Instances of the Vec class can be constructed from numbers

    >>> a = Vec(1, 2, 3)
    >>> b = Vec(3, 2, 1)

    added
    >>> a + b
    Vec(4, 4, 4)

    subtracted
    >>> a - b
    Vec(-2, 0, 2)

    and multiplied by a scalar on the left
    >>> 3.0 * a
    Vec(3.0, 6.0, 9.0)

    or on the right
    >>> a * 3.0
    Vec(3.0, 6.0, 9.0)

    and dot product
    >>> a.dot(b)
    10

    and printed in vector notation
    >>> print(a)
    <1 2 3>

    """

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
        args = ', '.join([repr(x) for x in self.v])
        return f'{type(self).__name__}({args})'

    def __str__(self):
        components = ' '.join([str(x) for x in self.v])
        return f'<{components}>'

    def __len__(self):
        return len(self.v)

    def __getitem__(self, i):
        return self.v[i]

    def __add__(self, other):
        "Element-wise addition"
        v = [x + y for x, y in zip(self.v, other.v)]
        return Vec.fromlist(v)

    def __sub__(self, other):
        "Element-wise subtraction"
        v = [x - y for x, y in zip(self.v, other.v)]
        return Vec.fromlist(v)

    def __mul__(self, scalar):
        "Multiply by scalar"
        v = [x * scalar for x in self.v]
        return Vec.fromlist(v)

    __rmul__ = __mul__

    def dot(self, other):
        "Vector dot product"
        if not isinstance(other, Vec):
            raise TypeError
        return sum(x_i * y_i for (x_i, y_i) in zip(self, other))


def test():
    import doctest
    doctest.testmod()

test()
