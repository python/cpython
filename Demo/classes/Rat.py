# Rational numbers

from types import *

def rat(num, den):
	if type(num) == FloatType or type(den) == FloatType:
		return num/den
	return Rat(num, den)


def gcd(a, b):
	while b:
		a, b = b, a%b
	return a


class Rat:

	def __init__(self, num, den):
		if den == 0:
			raise ZeroDivisionError, 'rat(x, 0)'
		if type(den) == FloatType or type(num) == FloatType:
			g = float(den)
		else:
			g = gcd(num, den)
		self.num = num/g
		self.den = den/g

	def __repr__(self):
		return 'Rat(%s, %s)' % (self.num, self.den)

	def __str__(self):
		if self.den == 1:
			return str(self.num)
		else:
			return '%s/%s' % (self.num, self.den)

	def __cmp__(a, b):
		c = a-b
		if c.num < 0:
			return -1
		if c.num > 0:
			return 1
		return 0

	def __float__(self):
		return float(self.num) / float(self.den)

	def __long__(self):
		return long(self.num) / long(self.den)

	def __int__(self):
		return int(self.num / self.den)

	def __coerce__(a, b):
		t = type(b)
		if t == IntType:
			return a, Rat(b, 1)
		if t == LongType:
			return a, Rat(b, 1L)
		if t == FloatType:
			return a, Rat(b, 1.0)
		if t == InstanceType and a.__class__ == b.__class__:
			return a, b
		raise TypeError, 'Rat.__coerce__: bad other arg'

	def __add__(a, b):
		return rat(a.num*b.den + b.num*a.den, a.den*b.den)

	def __sub__(a, b):
		return rat(a.num*b.den - b.num*a.den, a.den*b.den)

	def __mul__(a, b):
		return rat(a.num*b.num, a.den*b.den)

	def __div__(a, b):
		return rat(a.num*b.den, a.den*b.num)

	def __neg__(self):
		return rat(-self.num, self.den)


def test():
	print Rat(-1L, 1)
	print Rat(1, -1)
	a = Rat(1, 10)
	print int(a), long(a), float(a)
	b = Rat(2, 5)
	l = [a+b, a-b, a*b, a/b]
	print l
	l.sort()
	print l
	print Rat(0, 1)
	print a+1
	print a+1L
	print a+1.0
	try:
		print Rat(1, 0)
		raise SystemError, 'should have been ZeroDivisionError'
	except ZeroDivisionError:
		print 'OK'

test()
