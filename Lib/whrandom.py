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
#	whrandom.seed()		must be called before whrandom.random()
#				to seed the generator


#	Translated by Guido van Rossum from C source provided by
#	Adrian Baddeley.


# The seed
#
_seed = [0, 0, 0]


# Set the seed
#
def seed(x, y, z):
	_seed[:] = [x, y, z]


# Return the next random number in the range [0.0 .. 1.0)
#
def random():
	from math import floor		# floor() function
	#
	[x, y, z] = _seed
	x = 171 * (x % 177) - 2 * (x/177)
	y = 172 * (y % 176) - 35 * (y/176)
	z = 170 * (z % 178) - 63 * (z/178)
	#
	if x < 0: x = x + 30269
	if y < 0: y = y + 30307
	if z < 0: z = z + 30323
	#
	_seed[:] = [x, y, z]
	#
	term = float(x)/30269.0 + float(y)/30307.0 + float(z)/30323.0
	rand = term - floor(term)
	#
	if rand >= 1.0: rand = 0.0	# floor() inaccuracy?
	#
	return rand


# Initialize from the current time
#
def init():
	import time
	t = time.time()
	seed(t%256, t/256%256, t/65536%256)


# Make sure the generator is preset to a nonzero value
#
init()
