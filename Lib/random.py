"""Random variable generators.

    distributions on the real line:
    ------------------------------
           normal (Gaussian)
           lognormal
           negative exponential
           gamma
           beta

    distributions on the circle (angles 0 to 2pi)
    ---------------------------------------------
           circular uniform
           von Mises

Translated from anonymously contributed C/C++ source.

Multi-threading note: the random number generator used here is not
thread-safe; it is possible that two calls return the same random
value.  See whrandom.py for more info.
"""

import whrandom
from whrandom import random, uniform, randint, choice, randrange # For export!
from math import log, exp, pi, e, sqrt, acos, cos, sin

# Interfaces to replace remaining needs for importing whrandom
# XXX TO DO: make the distribution functions below into methods.

def makeseed(a=None):
	"""Turn a hashable value into three seed values for whrandom.seed().

	None or no argument returns (0, 0, 0), to seed from current time.

	"""
	if a is None:
		return (0, 0, 0)
	a = hash(a)
	a, x = divmod(a, 256)
	a, y = divmod(a, 256)
	a, z = divmod(a, 256)
	x = (x + a) % 256 or 1
	y = (y + a) % 256 or 1
	z = (z + a) % 256 or 1
	return (x, y, z)

def seed(a=None):
	"""Seed the default generator from any hashable value.

	None or no argument seeds from current time.

	"""
	x, y, z = makeseed(a)
	whrandom.seed(x, y, z)

class generator(whrandom.whrandom):
	"""Random generator class."""

	def __init__(self, a=None):
		"""Constructor.  Seed from current time or hashable value."""
		self.seed(a)

	def seed(self, a=None):
		"""Seed the generator from current time or hashable value."""
		x, y, z = makeseed(a)
		whrandom.whrandom.seed(self, x, y, z)

def new_generator(a=None):
	"""Return a new random generator instance."""
	return generator(a)

# Housekeeping function to verify that magic constants have been
# computed correctly

def verify(name, expected):
	computed = eval(name)
	if abs(computed - expected) > 1e-7:
		raise ValueError, \
  'computed value for %s deviates too much (computed %g, expected %g)' % \
  (name, computed, expected)

# -------------------- normal distribution --------------------

NV_MAGICCONST = 4*exp(-0.5)/sqrt(2.0)
verify('NV_MAGICCONST', 1.71552776992141)
def normalvariate(mu, sigma):
	# mu = mean, sigma = standard deviation

	# Uses Kinderman and Monahan method. Reference: Kinderman,
	# A.J. and Monahan, J.F., "Computer generation of random
	# variables using the ratio of uniform deviates", ACM Trans
	# Math Software, 3, (1977), pp257-260.

	while 1:
		u1 = random()
		u2 = random()
		z = NV_MAGICCONST*(u1-0.5)/u2
		zz = z*z/4.0
		if zz <= -log(u2):
			break
	return mu+z*sigma

# -------------------- lognormal distribution --------------------

def lognormvariate(mu, sigma):
	return exp(normalvariate(mu, sigma))

# -------------------- circular uniform --------------------

def cunifvariate(mean, arc):
	# mean: mean angle (in radians between 0 and pi)
	# arc:  range of distribution (in radians between 0 and pi)

	return (mean + arc * (random() - 0.5)) % pi

# -------------------- exponential distribution --------------------

def expovariate(lambd):
	# lambd: rate lambd = 1/mean
	# ('lambda' is a Python reserved word)

	u = random()
	while u <= 1e-7:
		u = random()
	return -log(u)/lambd

# -------------------- von Mises distribution --------------------

TWOPI = 2.0*pi
verify('TWOPI', 6.28318530718)

def vonmisesvariate(mu, kappa):
	# mu:    mean angle (in radians between 0 and 2*pi)
	# kappa: concentration parameter kappa (>= 0)
	# if kappa = 0 generate uniform random angle

	# Based upon an algorithm published in: Fisher, N.I.,
	# "Statistical Analysis of Circular Data", Cambridge
	# University Press, 1993.

	# Thanks to Magnus Kessler for a correction to the
	# implementation of step 4.

	if kappa <= 1e-6:
		return TWOPI * random()

	a = 1.0 + sqrt(1.0 + 4.0 * kappa * kappa)
	b = (a - sqrt(2.0 * a))/(2.0 * kappa)
	r = (1.0 + b * b)/(2.0 * b)

	while 1:
		u1 = random()

		z = cos(pi * u1)
		f = (1.0 + r * z)/(r + z)
		c = kappa * (r - f)

		u2 = random()

		if not (u2 >= c * (2.0 - c) and u2 > c * exp(1.0 - c)):
			break

	u3 = random()
	if u3 > 0.5:
		theta = (mu % TWOPI) + acos(f)
	else:
		theta = (mu % TWOPI) - acos(f)

	return theta

# -------------------- gamma distribution --------------------

LOG4 = log(4.0)
verify('LOG4', 1.38629436111989)

def gammavariate(alpha, beta):
        # beta times standard gamma
	ainv = sqrt(2.0 * alpha - 1.0)
	return beta * stdgamma(alpha, ainv, alpha - LOG4, alpha + ainv)

SG_MAGICCONST = 1.0 + log(4.5)
verify('SG_MAGICCONST', 2.50407739677627)

def stdgamma(alpha, ainv, bbb, ccc):
	# ainv = sqrt(2 * alpha - 1)
	# bbb = alpha - log(4)
	# ccc = alpha + ainv

	if alpha <= 0.0:
		raise ValueError, 'stdgamma: alpha must be > 0.0'

	if alpha > 1.0:

		# Uses R.C.H. Cheng, "The generation of Gamma
		# variables with non-integral shape parameters",
		# Applied Statistics, (1977), 26, No. 1, p71-74

		while 1:
			u1 = random()
			u2 = random()
			v = log(u1/(1.0-u1))/ainv
			x = alpha*exp(v)
			z = u1*u1*u2
			r = bbb+ccc*v-x
			if r + SG_MAGICCONST - 4.5*z >= 0.0 or r >= log(z):
				return x

	elif alpha == 1.0:
		# expovariate(1)
		u = random()
		while u <= 1e-7:
			u = random()
		return -log(u)

	else:	# alpha is between 0 and 1 (exclusive)

		# Uses ALGORITHM GS of Statistical Computing - Kennedy & Gentle

		while 1:
			u = random()
			b = (e + alpha)/e
			p = b*u
			if p <= 1.0:
				x = pow(p, 1.0/alpha)
			else:
				# p > 1
				x = -log((b-p)/alpha)
			u1 = random()
			if not (((p <= 1.0) and (u1 > exp(-x))) or
				  ((p > 1)  and  (u1 > pow(x, alpha - 1.0)))):
				break
		return x


# -------------------- Gauss (faster alternative) --------------------

gauss_next = None
def gauss(mu, sigma):

	# When x and y are two variables from [0, 1), uniformly
	# distributed, then
	#
	#    cos(2*pi*x)*sqrt(-2*log(1-y))
	#    sin(2*pi*x)*sqrt(-2*log(1-y))
	#
	# are two *independent* variables with normal distribution
	# (mu = 0, sigma = 1).
	# (Lambert Meertens)
	# (corrected version; bug discovered by Mike Miller, fixed by LM)

	# Multithreading note: When two threads call this function
	# simultaneously, it is possible that they will receive the
	# same return value.  The window is very small though.  To
	# avoid this, you have to use a lock around all calls.  (I
	# didn't want to slow this down in the serial case by using a
	# lock here.)

	global gauss_next

	z = gauss_next
	gauss_next = None
	if z is None:
		x2pi = random() * TWOPI
		g2rad = sqrt(-2.0 * log(1.0 - random()))
		z = cos(x2pi) * g2rad
		gauss_next = sin(x2pi) * g2rad

	return mu + z*sigma

# -------------------- beta --------------------

def betavariate(alpha, beta):

	# Discrete Event Simulation in C, pp 87-88.

	y = expovariate(alpha)
	z = expovariate(1.0/beta)
	return z/(y+z)

# -------------------- Pareto --------------------

def paretovariate(alpha):
	# Jain, pg. 495

	u = random()
	return 1.0 / pow(u, 1.0/alpha)

# -------------------- Weibull --------------------

def weibullvariate(alpha, beta):
	# Jain, pg. 499; bug fix courtesy Bill Arms

	u = random()
	return alpha * pow(-log(u), 1.0/beta)

# -------------------- shuffle --------------------
# Not quite a random distribution, but a standard algorithm.
# This implementation due to Tim Peters.

def shuffle(x, random=random, int=int):
    """x, random=random.random -> shuffle list x in place; return None.

    Optional arg random is a 0-argument function returning a random
    float in [0.0, 1.0); by default, the standard random.random.

    Note that for even rather small len(x), the total number of
    permutations of x is larger than the period of most random number
    generators; this implies that "most" permutations of a long
    sequence can never be generated.
    """

    for i in xrange(len(x)-1, 0, -1):
        # pick an element in x[:i+1] with which to exchange x[i]
        j = int(random() * (i+1))
        x[i], x[j] = x[j], x[i]

# -------------------- test program --------------------

def test(N = 200):
	print 'TWOPI         =', TWOPI
	print 'LOG4          =', LOG4
	print 'NV_MAGICCONST =', NV_MAGICCONST
	print 'SG_MAGICCONST =', SG_MAGICCONST
	test_generator(N, 'random()')
	test_generator(N, 'normalvariate(0.0, 1.0)')
	test_generator(N, 'lognormvariate(0.0, 1.0)')
	test_generator(N, 'cunifvariate(0.0, 1.0)')
	test_generator(N, 'expovariate(1.0)')
	test_generator(N, 'vonmisesvariate(0.0, 1.0)')
	test_generator(N, 'gammavariate(0.5, 1.0)')
	test_generator(N, 'gammavariate(0.9, 1.0)')
	test_generator(N, 'gammavariate(1.0, 1.0)')
	test_generator(N, 'gammavariate(2.0, 1.0)')
	test_generator(N, 'gammavariate(20.0, 1.0)')
	test_generator(N, 'gammavariate(200.0, 1.0)')
	test_generator(N, 'gauss(0.0, 1.0)')
	test_generator(N, 'betavariate(3.0, 3.0)')
	test_generator(N, 'paretovariate(1.0)')
	test_generator(N, 'weibullvariate(1.0, 1.0)')

def test_generator(n, funccall):
	import time
	print n, 'times', funccall
	code = compile(funccall, funccall, 'eval')
	sum = 0.0
	sqsum = 0.0
	smallest = 1e10
	largest = -1e10
	t0 = time.time()
	for i in range(n):
		x = eval(code)
		sum = sum + x
		sqsum = sqsum + x*x
		smallest = min(x, smallest)
		largest = max(x, largest)
	t1 = time.time()
	print round(t1-t0, 3), 'sec,', 
	avg = sum/n
	stddev = sqrt(sqsum/n - avg*avg)
	print 'avg %g, stddev %g, min %g, max %g' % \
		  (avg, stddev, smallest, largest)

if __name__ == '__main__':
	test()
