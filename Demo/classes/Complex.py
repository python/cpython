# Complex numbers


from math import sqrt


class complex:

	def __init__(self, re, im):
		self.re = float(re)
		self.im = float(im)

	def __coerce__(self, other):
		if type(other) == type(self):
			if other.__class__ == self.__class__:
				return self, other
			else:
				raise TypeError, 'cannot coerce to complex'
		else:
			# The cast to float() may raise an exception!
			return self, complex(float(other), 0.0)

	def __repr__(self):
		return 'complex' + `self.re, self.im`

	def __cmp__(a, b):
		a = a.__abs__()
		b = b.__abs__()
		return (a > b) - (a < b)

	def __float__(self):
		if self.im:
			raise ValueError, 'cannot convert complex to float'
		return float(self.re)

	def __long__(self):
		return long(float(self))

	def __int__(self):
		return int(float(self))

	def __abs__(self):
		# XXX overflow?
		return sqrt(self.re*self.re + self.im*self.im)

	def __add__(a, b):
		return complex(a.re + b.re, a.im + b.im)

	def __sub__(a, b):
		return complex(a.re - b.re, a.im - b.im)

	def __mul__(a, b):
		return complex(a.re*b.re - a.im*b.im, a.re*b.im + a.im*b.re)

	def __div__(a, b):
		q = (b.re*b.re + b.im*b.im)
		re = (a.re*b.re + a.im*b.im) / q
		im = (a.im*b.re - b.im*a.re) / q
		return complex(re, im)

	def __neg__(self):
		return complex(-self.re, -self.im)


def test():
	a = complex(2, 0)
	b = complex(3, 4)
	print a
	print b
	print a+b
	print a-b
	print a*b
	print a/b
	print b+a
	print b-a
	print b*a
	print b/a
	i = complex(0, 1)
	print i, i*i, i*i*i, i*i*i*i
	j = complex(1, 1)
	print j, j*j, j*j*j, j*j*j*j
	print abs(j), abs(j*j), abs(j*j*j), abs(j*j*j*j)
	print i/-i

test()
