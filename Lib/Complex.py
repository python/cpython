# Complex numbers
# ---------------

# This module represents complex numbers as instances of the class Complex.
# A Complex instance z has two data attribues, z.re (the real part) and z.im
# (the imaginary part).  In fact, z.re and z.im can have any value -- all
# arithmetic operators work regardless of the type of z.re and z.im (as long
# as they support numerical operations).
#
# The following functions exist (Complex is actually a class):
# Complex([re [,im]) -> creates a complex number from a real and an imaginary part
# IsComplex(z) -> true iff z is a complex number (== has .re and .im attributes)
# Polar([r [,phi [,fullcircle]]]) ->
#	the complex number z for which r == z.radius() and phi == z.angle(fullcircle)
#	(r and phi default to 0)
#
# Complex numbers have the following methods:
# z.abs() -> absolute value of z
# z.radius() == z.abs()
# z.angle([fullcircle]) -> angle from positive X axis; fullcircle gives units
# z.phi([fullcircle]) == z.angle(fullcircle)
#
# These standard functions and unary operators accept complex arguments:
# abs(z)
# -z
# +z
# not z
# repr(z) == `z`
# str(z)
# hash(z) -> a combination of hash(z.re) and hash(z.im) such that if z.im is zero
#            the result equals hash(z.re)
# Note that hex(z) and oct(z) are not defined.
#
# These conversions accept complex arguments only if their imaginary part is zero:
# int(z)
# long(z)
# float(z)
#
# The following operators accept two complex numbers, or one complex number
# and one real number (int, long or float):
# z1 + z2
# z1 - z2
# z1 * z2
# z1 / z2
# pow(z1, z2)
# cmp(z1, z2)
# Note that z1 % z2 and divmod(z1, z2) are not defined,
# nor are shift and mask operations.
#
# The standard module math does not support complex numbers.
# (I suppose it would be easy to implement a cmath module.)
#
# Idea:
# add a class Polar(r, phi) and mixed-mode arithmetic which
# chooses the most appropriate type for the result:
# Complex for +,-,cmp
# Polar   for *,/,pow


import types, math

if not hasattr(math, 'hypot'):
	def hypot(x, y):
		# XXX I know there's a way to compute this without possibly causing
		# overflow, but I can't remember what it is right now...
		return math.sqrt(x*x + y*y)
	math.hypot = hypot

twopi = math.pi*2.0
halfpi = math.pi/2.0

def IsComplex(obj):
	return hasattr(obj, 're') and hasattr(obj, 'im')

def Polar(r = 0, phi = 0, fullcircle = twopi):
	phi = phi * (twopi / fullcircle)
	return Complex(math.cos(phi)*r, math.sin(phi)*r)

class Complex:

	def __init__(self, re=0, im=0):
		if IsComplex(re):
			im = im + re.im
			re = re.re
		if IsComplex(im):
			re = re - im.im
			im = im.re
		self.re = re
		self.im = im

	def __setattr__(self, name, value):
		if hasattr(self, name):
			raise TypeError, "Complex numbers have set-once attributes"
		self.__dict__[name] = value

	def __repr__(self):
		if not self.im:
			return 'Complex(%s)' % `self.re`
		else:
			return 'Complex(%s, %s)' % (`self.re`, `self.im`)

	def __str__(self):
		if not self.im:
			return `self.re`
		else:
			return 'Complex(%s, %s)' % (`self.re`, `self.im`)

	def __coerce__(self, other):
		if IsComplex(other):
			return self, other
		return self, Complex(other)	# May fail

	def __cmp__(self, other):
		return cmp(self.re, other.re) or cmp(self.im, other.im)

	def __hash__(self):
		if not self.im: return hash(self.re)
		mod = sys.maxint + 1L
		return int((hash(self.re) + 2L*hash(self.im) + mod) % (2L*mod) - mod)

	def __neg__(self):
		return Complex(-self.re, -self.im)

	def __pos__(self):
		return self

	def __abs__(self):
		return math.hypot(self.re, self.im)
		##return math.sqrt(self.re*self.re + self.im*self.im)


	def __int__(self):
		if self.im:
			raise ValueError, "can't convert Complex with nonzero im to int"
		return int(self.re)

	def __long__(self):
		if self.im:
			raise ValueError, "can't convert Complex with nonzero im to long"
		return long(self.re)

	def __float__(self):
		if self.im:
			raise ValueError, "can't convert Complex with nonzero im to float"
		return float(self.re)

	def __nonzero__(self):
		return not (self.re == self.im == 0)

	abs = radius = __abs__

	def angle(self, fullcircle = twopi):
		return (fullcircle/twopi) * ((halfpi - math.atan2(self.re, self.im)) % twopi)

	phi = angle

	def __add__(self, other):
		return Complex(self.re + other.re, self.im + other.im)

	__radd__ = __add__

	def __sub__(self, other):
		return Complex(self.re - other.re, self.im - other.im)

	def __rsub__(self, other):
		return Complex(other.re - self.re, other.im - self.im)

	def __mul__(self, other):
		return Complex(self.re*other.re - self.im*other.im,
		               self.re*other.im + self.im*other.re)

	__rmul__ = __mul__

	def __div__(self, other):
		# Deviating from the general principle of not forcing re or im
		# to be floats, we cast to float here, otherwise division
		# of Complex numbers with integer re and im parts would use
		# the (truncating) integer division
		d = float(other.re*other.re + other.im*other.im)
		if not d: raise ZeroDivisionError, 'Complex division'
		return Complex((self.re*other.re + self.im*other.im) / d,
		               (self.im*other.re - self.re*other.im) / d)

	def __rdiv__(self, other):
		return other / self

	def __pow__(self, n, z=None):
		if z is not None:
			raise TypeError, 'Complex does not support ternary pow()'
		if IsComplex(n):
			if n.im: raise TypeError, 'Complex to the Complex power'
			n = n.re
		r = pow(self.abs(), n)
		phi = n*self.angle()
		return Complex(math.cos(phi)*r, math.sin(phi)*r)
	
	def __rpow__(self, base):
		return pow(base, self)


# Everything below this point is part of the test suite

def checkop(expr, a, b, value, fuzz = 1e-6):
	import sys
	print '       ', a, 'and', b,
	try:
		result = eval(expr)
	except:
		result = sys.exc_type
	print '->', result
	if (type(result) == type('') or type(value) == type('')):
		ok = result == value
	else:
		ok = abs(result - value) <= fuzz
	if not ok:
		print '!!\t!!\t!! should be', value, 'diff', abs(result - value)


def test():
	testsuite = {
		'a+b': [
			(1, 10, 11),
			(1, Complex(0,10), Complex(1,10)),
			(Complex(0,10), 1, Complex(1,10)),
			(Complex(0,10), Complex(1), Complex(1,10)),
			(Complex(1), Complex(0,10), Complex(1,10)),
		],
		'a-b': [
			(1, 10, -9),
			(1, Complex(0,10), Complex(1,-10)),
			(Complex(0,10), 1, Complex(-1,10)),
			(Complex(0,10), Complex(1), Complex(-1,10)),
			(Complex(1), Complex(0,10), Complex(1,-10)),
		],
		'a*b': [
			(1, 10, 10),
			(1, Complex(0,10), Complex(0, 10)),
			(Complex(0,10), 1, Complex(0,10)),
			(Complex(0,10), Complex(1), Complex(0,10)),
			(Complex(1), Complex(0,10), Complex(0,10)),
		],
		'a/b': [
			(1., 10, 0.1),
			(1, Complex(0,10), Complex(0, -0.1)),
			(Complex(0, 10), 1, Complex(0, 10)),
			(Complex(0, 10), Complex(1), Complex(0, 10)),
			(Complex(1), Complex(0,10), Complex(0, -0.1)),
		],
		'pow(a,b)': [
			(1, 10, 1),
			(1, Complex(0,10), 'TypeError'),
			(Complex(0,10), 1, Complex(0,10)),
			(Complex(0,10), Complex(1), Complex(0,10)),
			(Complex(1), Complex(0,10), 'TypeError'),
			(2, Complex(4,0), 16),
		],
		'cmp(a,b)': [
			(1, 10, -1),
			(1, Complex(0,10), 1),
			(Complex(0,10), 1, -1),
			(Complex(0,10), Complex(1), -1),
			(Complex(1), Complex(0,10), 1),
		],
	}
	exprs = testsuite.keys()
	exprs.sort()
	for expr in exprs:
		print expr + ':'
		t = (expr,)
		for item in testsuite[expr]:
			apply(checkop, t+item)
	

if __name__ == '__main__':
	test()
