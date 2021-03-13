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
    
    lenght of Vector
    >>> len(a)
    3
    
    printing argument of Vector by index
    >>> a[1]
    2
    
    New!!
    >>> a * b
    Vec(3, 4, 3)
    
    >>> a / b
    Vec(0.3333333333333333, 1.0, 3.0)
    
    >>> a // b
    Vec(1, 0, 0)
    
    >>> a % 2
    Vec(1, 0, 1)
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
        args = ', '.join(repr(x) for x in self.v)
        return 'Vec({})'.format(args)

    def __len__(self):
        return len(self.v)

    def __getitem__(self, i):
        return self.v[i]

    def __add__(self, other):
        # Element-wise addition
        v = [x + y for x, y in zip(self.v, other.v)]
        return Vec.fromlist(v)
 
    def __sub__(self, other):
        # Element-wise subtraction
        v = [x - y for x, y in zip(self.v, other.v)]
        return Vec.fromlist(v)

	def __mul__(self, other):
		if isinstance(other, int) or isinstance(other, float):
			m = [x * other for x in self.matrix]
		else:
			m = [x * y for x, y in zip(self.matrix, other.matrix)]

		return Matrix.fromlist(m)

	def __truediv__(self, other):
		if isinstance(other, int) or isinstance(other, float):
			m = [x / other for x in self.matrix]
		else:
			m = [x / y for x, y in zip(self.matrix, other.matrix)]

		return Matrix.fromlist(m)

	def __floordiv__(self, other):
		if isinstance(other, int) or isinstance(other, float):
			m = [x // other for x in self.matrix]
		else:
			m = [x // y for x, y in zip(self.matrix, other.matrix)]

		return Matrix.fromlist(m)

	def __mod__(self, other):
		if isinstance(other, int) or isinstance(other, float):
			m = [x % other for x in self.matrix]
		else:
			m = [x % y for x, y in zip(self.matrix, other.matrix)]

		return Matrix.fromlist(m)

	__rmul__ = __mul__


def test():
    import doctest
    doctest.testmod()

test()
