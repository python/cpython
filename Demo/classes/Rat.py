# Rational numbers


def rat(num, den):
	return Rat().init(num, den)


def gcd(a, b):
	while b:
		a, b = b, a%b
	return a


class Rat:

	def init(self, num, den):
		if den == 0:
			raise ZeroDivisionError, 'rat(x, 0)'
		g = gcd(num, den)
		self.num = num/g
		self.den = den/g
		return self

	def __repr__(self):
		return 'rat' + `self.num, self.den`

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
		if t == type(0):
			return a, rat(b, 1)
		if t == type(0L):
			return a, rat(b, 1L)
		if t == type(0.0):
			return a.__float__(), b
		raise TypeError, 'Rat.__coerce__: bad other arg'
			
	def __add__(a, b):
		if type(b) <> type(a):
			a, b = a.__coerce__(b)
			return a + b
		return rat(a.num*b.den + b.num*a.den, a.den*b.den)

	def __sub__(a, b):
		return rat(a.num*b.den - b.num*a.den, a.den*b.den)

	def __mul__(a, b):
		if type(b) <> type(a):
			a, b = a.__coerce__(b)
			return a * b
		return rat(a.num*b.num, a.den*b.den)

	def __div__(a, b):
		return rat(a.num*b.den, a.den*b.num)

	def __neg__(self):
		return rat(-self.num, self.den)


def test():
	print rat(-1L, 1)
	print rat(1, -1)
	a = rat(1, 10)
	print int(a), long(a), float(a)
	b = rat(2, 5)
	l = [a+b, a-b, a*b, a/b]
	print l
	l.sort()
	print l
	print rat(0, 1)
	print a+1
	print a+1L
	print a+1.0
	try:
		print rat(1, 0)
		raise SystemError, 'should have been ZeroDivisionError'
	except ZeroDivisionError:
		print 'OK'

#test()
