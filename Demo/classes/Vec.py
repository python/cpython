# A simple vector class


def vec(*v):
	return apply(Vec, v)


class Vec:

	def __init__(self, *v):
		self.v = []
		for x in v:
			self.v.append(x)


	def fromlist(self, v):
		self.v = []
		if type(v) <> type([]):
			raise TypeError
		self.v = v[:]
		return self


	def __repr__(self):
		return 'vec(' + `self.v`[1:-1] + ')'

	def __len__(self):
		return len(self.v)

	def __getitem__(self, i):
		return self.v[i]

	def __add__(a, b):
		# Element-wise addition
		v = []
		for i in range(len(a)):
			v.append(a[i] + b[i])
		return Vec().fromlist(v)

	def __sub__(a, b):
		# Element-wise subtraction
		v = []
		for i in range(len(a)):
			v.append(a[i] - b[i])
		return Vec().fromlist(v)

	def __mul__(self, scalar):
		# Multiply by scalar
		v = []
		for i in range(len(self.v)):
			v.append(self.v[i]*scalar)
		return Vec().fromlist(v)



def test():
	a = vec(1, 2, 3)
	b = vec(3, 2, 1)
	print a
	print b
	print a+b
	print a*3.0

test()
