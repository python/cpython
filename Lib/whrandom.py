#	WICHMANN-HILL RANDOM NUMBER GENERATOR
#
#	Wichmann, B. A. & Hill, I. D. (1982)
#	Algorithm AS 183: 
#	An efficient and portable pseudo-random number generator
#	Applied Statistics 31 (1982) 188-190
#
#	see also: 
#		Correction to Algorithm AS 183
#		Applied Statistics 33 (1984) 123  
#
#		McLeod, A. I. (1985)
#		A remark on Algorithm AS 183 
#		Applied Statistics 34 (1985),198-200
#
#
#	USE:
#	whrandom.random()	yields double precision random numbers 
#				uniformly distributed between 0 and 1.
#
#	whrandom.seed(x, y, z)	must be called before whrandom.random()
#				to seed the generator
#
#	There is also an interface to create multiple independent
#	random generators, and to choose from other ranges.


#	Translated by Guido van Rossum from C source provided by
#	Adrian Baddeley.


class whrandom:
	#
	# Initialize an instance.
	# Without arguments, initialize from current time.
	# With arguments (x, y, z), initialize from them.
	#
	def init(self, *xyz):
		if not xyz:
			# Initialize from current time
			import time
			t = time.time()
			t, x = divmod(t, 256)
			t, y = divmod(t, 256)
			t, z = divmod(t, 256)
		else:
			# Initialize from arguments (x, y, z)
			x, y, z = xyz
		self.seed(x, y, z)
		return self
	#
	# Set the seed from (x, y, z).
	# These must be integers in the range [0, 256).
	#
	def seed(self, *xyz):
		if type(xyz) <> type(()) or len(xyz) <> 3:
			raise TypeError, '3 seeds required'
		x, y, z = xyz
		if not type(x) == type(y) == type(z) == type(0):
			raise TypeError, 'seeds must be integers'
		if not 0 <= x < 256 and 0 <= y < 256 and 0 <= z < 256:
			raise ValueError, 'seeds must be in range(0, 256)'
		self._seed = xyz
	#
	# Get the next random number in the range [0.0, 1.0).
	#
	def random(self):
		x, y, z = self._seed
		#
		x1, x2 = divmod(x, 177)
		y1, y2 = divmod(y, 176)
		z1, z2 = divmod(z, 178)
		#
		x = (171 * x2 -  2 * x1) % 30269
		y = (172 * y2 - 35 * y1) % 30307
		z = (170 * z2 - 63 * z1) % 30323
		#
		self._seed = x, y, z
		#
		return (x/30269.0 + y/30307.0 + z/30323.0) % 1.0
	#
	# Get a random number in the range [a, b).
	#
	def uniform(self, a, b):
		return a + (b-a) * self.random()
	#
	# Get a random integer in the range [a, b] including both end points.
	#
	def randint(self, a, b):
		return a + int(self.random() * (b+1-a))
	#
	# Choose a random element from a non-empty sequence.
	#
	def choice(self, seq):
		return seq[int(self.random() * len(seq))]


# Initialize from the current time
#
_inst = whrandom().init()
seed = _inst.seed
random = _inst.random
uniform = _inst.uniform
randint = _inst.randint
choice = _inst.choice
