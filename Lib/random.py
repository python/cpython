"""Random variable generators.

    integers
    --------
           uniform within range

    sequences
    ---------
           pick random element
           generate random permutation

    distributions on the real line:
    ------------------------------
           uniform
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

Multi-threading note:  the random number generator used here is not thread-
safe; it is possible that two calls return the same random value.  However,
you can instantiate a different instance of Random() in each thread to get
generators that don't share state, then use .setstate() and .jumpahead() to
move the generators to disjoint segments of the full period.  For example,

def create_generators(num, delta, firstseed=None):
    ""\"Return list of num distinct generators.
    Each generator has its own unique segment of delta elements from
    Random.random()'s full period.
    Seed the first generator with optional arg firstseed (default is
    None, to seed from current time).
    ""\"

    from random import Random
    g = Random(firstseed)
    result = [g]
    for i in range(num - 1):
        laststate = g.getstate()
        g = Random()
        g.setstate(laststate)
        g.jumpahead(delta)
        result.append(g)
    return result

gens = create_generators(10, 1000000)

That creates 10 distinct generators, which can be passed out to 10 distinct
threads.  The generators don't share state so can be called safely in
parallel.  So long as no thread calls its g.random() more than a million
times (the second argument to create_generators), the sequences seen by
each thread will not overlap.

The period of the underlying Wichmann-Hill generator is 6,953,607,871,644,
and that limits how far this technique can be pushed.

Just for fun, note that since we know the period, .jumpahead() can also be
used to "move backward in time":

>>> g = Random(42)  # arbitrary
>>> g.random()
0.25420336316883324
>>> g.jumpahead(6953607871644L - 1) # move *back* one
>>> g.random()
0.25420336316883324
"""
# XXX The docstring sucks.

from math import log as _log, exp as _exp, pi as _pi, e as _e
from math import sqrt as _sqrt, acos as _acos, cos as _cos, sin as _sin

__all__ = ["Random","seed","random","uniform","randint","choice",
           "randrange","shuffle","normalvariate","lognormvariate",
           "cunifvariate","expovariate","vonmisesvariate","gammavariate",
           "stdgamma","gauss","betavariate","paretovariate","weibullvariate",
           "getstate","setstate","jumpahead","whseed"]

def _verify(name, computed, expected):
    if abs(computed - expected) > 1e-7:
        raise ValueError(
            "computed value for %s deviates too much "
            "(computed %g, expected %g)" % (name, computed, expected))

NV_MAGICCONST = 4 * _exp(-0.5)/_sqrt(2.0)
_verify('NV_MAGICCONST', NV_MAGICCONST, 1.71552776992141)

TWOPI = 2.0*_pi
_verify('TWOPI', TWOPI, 6.28318530718)

LOG4 = _log(4.0)
_verify('LOG4', LOG4, 1.38629436111989)

SG_MAGICCONST = 1.0 + _log(4.5)
_verify('SG_MAGICCONST', SG_MAGICCONST, 2.50407739677627)

del _verify

# Translated by Guido van Rossum from C source provided by
# Adrian Baddeley.

class Random:

    VERSION = 1     # used by getstate/setstate

    def __init__(self, x=None):
        """Initialize an instance.

        Optional argument x controls seeding, as for Random.seed().
        """

        self.seed(x)
        self.gauss_next = None

## -------------------- core generator -------------------

    # Specific to Wichmann-Hill generator.  Subclasses wishing to use a
    # different core generator should override the seed(), random(),
    # getstate(), setstate() and jumpahead() methods.

    def seed(self, a=None):
        """Initialize internal state from hashable object.

        None or no argument seeds from current time.

        If a is not None or an int or long, hash(a) is used instead.

        If a is an int or long, a is used directly.  Distinct values between
        0 and 27814431486575L inclusive are guaranteed to yield distinct
        internal states (this guarantee is specific to the default
        Wichmann-Hill generator).
        """

        if a is None:
            # Initialize from current time
            import time
            a = long(time.time() * 256)

        if type(a) not in (type(3), type(3L)):
            a = hash(a)

        a, x = divmod(a, 30268)
        a, y = divmod(a, 30306)
        a, z = divmod(a, 30322)
        self._seed = int(x)+1, int(y)+1, int(z)+1

    def random(self):
        """Get the next random number in the range [0.0, 1.0)."""

        # Wichman-Hill random number generator.
        #
        # Wichmann, B. A. & Hill, I. D. (1982)
        # Algorithm AS 183:
        # An efficient and portable pseudo-random number generator
        # Applied Statistics 31 (1982) 188-190
        #
        # see also:
        #        Correction to Algorithm AS 183
        #        Applied Statistics 33 (1984) 123
        #
        #        McLeod, A. I. (1985)
        #        A remark on Algorithm AS 183
        #        Applied Statistics 34 (1985),198-200

        # This part is thread-unsafe:
        # BEGIN CRITICAL SECTION
        x, y, z = self._seed
        x = (171 * x) % 30269
        y = (172 * y) % 30307
        z = (170 * z) % 30323
        self._seed = x, y, z
        # END CRITICAL SECTION

        # Note:  on a platform using IEEE-754 double arithmetic, this can
        # never return 0.0 (asserted by Tim; proof too long for a comment).
        return (x/30269.0 + y/30307.0 + z/30323.0) % 1.0

    def getstate(self):
        """Return internal state; can be passed to setstate() later."""
        return self.VERSION, self._seed, self.gauss_next

    def setstate(self, state):
        """Restore internal state from object returned by getstate()."""
        version = state[0]
        if version == 1:
            version, self._seed, self.gauss_next = state
        else:
            raise ValueError("state with version %s passed to "
                             "Random.setstate() of version %s" %
                             (version, self.VERSION))

    def jumpahead(self, n):
        """Act as if n calls to random() were made, but quickly.

        n is an int, greater than or equal to 0.

        Example use:  If you have 2 threads and know that each will
        consume no more than a million random numbers, create two Random
        objects r1 and r2, then do
            r2.setstate(r1.getstate())
            r2.jumpahead(1000000)
        Then r1 and r2 will use guaranteed-disjoint segments of the full
        period.
        """

        if not n >= 0:
            raise ValueError("n must be >= 0")
        x, y, z = self._seed
        x = int(x * pow(171, n, 30269)) % 30269
        y = int(y * pow(172, n, 30307)) % 30307
        z = int(z * pow(170, n, 30323)) % 30323
        self._seed = x, y, z

    def __whseed(self, x=0, y=0, z=0):
        """Set the Wichmann-Hill seed from (x, y, z).

        These must be integers in the range [0, 256).
        """

        if not type(x) == type(y) == type(z) == type(0):
            raise TypeError('seeds must be integers')
        if not (0 <= x < 256 and 0 <= y < 256 and 0 <= z < 256):
            raise ValueError('seeds must be in range(0, 256)')
        if 0 == x == y == z:
            # Initialize from current time
            import time
            t = long(time.time() * 256)
            t = int((t&0xffffff) ^ (t>>24))
            t, x = divmod(t, 256)
            t, y = divmod(t, 256)
            t, z = divmod(t, 256)
        # Zero is a poor seed, so substitute 1
        self._seed = (x or 1, y or 1, z or 1)

    def whseed(self, a=None):
        """Seed from hashable object's hash code.

        None or no argument seeds from current time.  It is not guaranteed
        that objects with distinct hash codes lead to distinct internal
        states.

        This is obsolete, provided for compatibility with the seed routine
        used prior to Python 2.1.  Use the .seed() method instead.
        """

        if a is None:
            self.__whseed()
            return
        a = hash(a)
        a, x = divmod(a, 256)
        a, y = divmod(a, 256)
        a, z = divmod(a, 256)
        x = (x + a) % 256 or 1
        y = (y + a) % 256 or 1
        z = (z + a) % 256 or 1
        self.__whseed(x, y, z)

## ---- Methods below this point do not need to be overridden when
## ---- subclassing for the purpose of using a different core generator.

## -------------------- pickle support  -------------------

    def __getstate__(self): # for pickle
        return self.getstate()

    def __setstate__(self, state):  # for pickle
        self.setstate(state)

## -------------------- integer methods  -------------------

    def randrange(self, start, stop=None, step=1, int=int, default=None):
        """Choose a random item from range(start, stop[, step]).

        This fixes the problem with randint() which includes the
        endpoint; in Python this is usually not what you want.
        Do not supply the 'int' and 'default' arguments.
        """

        # This code is a bit messy to make it fast for the
        # common case while still doing adequate error checking
        istart = int(start)
        if istart != start:
            raise ValueError, "non-integer arg 1 for randrange()"
        if stop is default:
            if istart > 0:
                return int(self.random() * istart)
            raise ValueError, "empty range for randrange()"
        istop = int(stop)
        if istop != stop:
            raise ValueError, "non-integer stop for randrange()"
        if step == 1:
            if istart < istop:
                return istart + int(self.random() *
                                   (istop - istart))
            raise ValueError, "empty range for randrange()"
        istep = int(step)
        if istep != step:
            raise ValueError, "non-integer step for randrange()"
        if istep > 0:
            n = (istop - istart + istep - 1) / istep
        elif istep < 0:
            n = (istop - istart + istep + 1) / istep
        else:
            raise ValueError, "zero step for randrange()"

        if n <= 0:
            raise ValueError, "empty range for randrange()"
        return istart + istep*int(self.random() * n)

    def randint(self, a, b):
        """Return random integer in range [a, b], including both end points.

        (Deprecated; use randrange(a, b+1).)
        """

        return self.randrange(a, b+1)

## -------------------- sequence methods  -------------------

    def choice(self, seq):
        """Choose a random element from a non-empty sequence."""
        return seq[int(self.random() * len(seq))]

    def shuffle(self, x, random=None, int=int):
        """x, random=random.random -> shuffle list x in place; return None.

        Optional arg random is a 0-argument function returning a random
        float in [0.0, 1.0); by default, the standard random.random.

        Note that for even rather small len(x), the total number of
        permutations of x is larger than the period of most random number
        generators; this implies that "most" permutations of a long
        sequence can never be generated.
        """

        if random is None:
            random = self.random
        for i in xrange(len(x)-1, 0, -1):
            # pick an element in x[:i+1] with which to exchange x[i]
            j = int(random() * (i+1))
            x[i], x[j] = x[j], x[i]

## -------------------- real-valued distributions  -------------------

## -------------------- uniform distribution -------------------

    def uniform(self, a, b):
        """Get a random number in the range [a, b)."""
        return a + (b-a) * self.random()

## -------------------- normal distribution --------------------

    def normalvariate(self, mu, sigma):
        # mu = mean, sigma = standard deviation

        # Uses Kinderman and Monahan method. Reference: Kinderman,
        # A.J. and Monahan, J.F., "Computer generation of random
        # variables using the ratio of uniform deviates", ACM Trans
        # Math Software, 3, (1977), pp257-260.

        random = self.random
        while 1:
            u1 = random()
            u2 = random()
            z = NV_MAGICCONST*(u1-0.5)/u2
            zz = z*z/4.0
            if zz <= -_log(u2):
                break
        return mu + z*sigma

## -------------------- lognormal distribution --------------------

    def lognormvariate(self, mu, sigma):
        return _exp(self.normalvariate(mu, sigma))

## -------------------- circular uniform --------------------

    def cunifvariate(self, mean, arc):
        # mean: mean angle (in radians between 0 and pi)
        # arc:  range of distribution (in radians between 0 and pi)

        return (mean + arc * (self.random() - 0.5)) % _pi

## -------------------- exponential distribution --------------------

    def expovariate(self, lambd):
        # lambd: rate lambd = 1/mean
        # ('lambda' is a Python reserved word)

        random = self.random
        u = random()
        while u <= 1e-7:
            u = random()
        return -_log(u)/lambd

## -------------------- von Mises distribution --------------------

    def vonmisesvariate(self, mu, kappa):
        # mu:    mean angle (in radians between 0 and 2*pi)
        # kappa: concentration parameter kappa (>= 0)
        # if kappa = 0 generate uniform random angle

        # Based upon an algorithm published in: Fisher, N.I.,
        # "Statistical Analysis of Circular Data", Cambridge
        # University Press, 1993.

        # Thanks to Magnus Kessler for a correction to the
        # implementation of step 4.

        random = self.random
        if kappa <= 1e-6:
            return TWOPI * random()

        a = 1.0 + _sqrt(1.0 + 4.0 * kappa * kappa)
        b = (a - _sqrt(2.0 * a))/(2.0 * kappa)
        r = (1.0 + b * b)/(2.0 * b)

        while 1:
            u1 = random()

            z = _cos(_pi * u1)
            f = (1.0 + r * z)/(r + z)
            c = kappa * (r - f)

            u2 = random()

            if not (u2 >= c * (2.0 - c) and u2 > c * _exp(1.0 - c)):
                break

        u3 = random()
        if u3 > 0.5:
            theta = (mu % TWOPI) + _acos(f)
        else:
            theta = (mu % TWOPI) - _acos(f)

        return theta

## -------------------- gamma distribution --------------------

    def gammavariate(self, alpha, beta):
        # beta times standard gamma
        ainv = _sqrt(2.0 * alpha - 1.0)
        return beta * self.stdgamma(alpha, ainv, alpha - LOG4, alpha + ainv)

    def stdgamma(self, alpha, ainv, bbb, ccc):
        # ainv = sqrt(2 * alpha - 1)
        # bbb = alpha - log(4)
        # ccc = alpha + ainv

        random = self.random
        if alpha <= 0.0:
            raise ValueError, 'stdgamma: alpha must be > 0.0'

        if alpha > 1.0:

            # Uses R.C.H. Cheng, "The generation of Gamma
            # variables with non-integral shape parameters",
            # Applied Statistics, (1977), 26, No. 1, p71-74

            while 1:
                u1 = random()
                u2 = random()
                v = _log(u1/(1.0-u1))/ainv
                x = alpha*_exp(v)
                z = u1*u1*u2
                r = bbb+ccc*v-x
                if r + SG_MAGICCONST - 4.5*z >= 0.0 or r >= _log(z):
                    return x

        elif alpha == 1.0:
            # expovariate(1)
            u = random()
            while u <= 1e-7:
                u = random()
            return -_log(u)

        else:   # alpha is between 0 and 1 (exclusive)

            # Uses ALGORITHM GS of Statistical Computing - Kennedy & Gentle

            while 1:
                u = random()
                b = (_e + alpha)/_e
                p = b*u
                if p <= 1.0:
                    x = pow(p, 1.0/alpha)
                else:
                    # p > 1
                    x = -_log((b-p)/alpha)
                u1 = random()
                if not (((p <= 1.0) and (u1 > _exp(-x))) or
                          ((p > 1)  and  (u1 > pow(x, alpha - 1.0)))):
                    break
            return x


## -------------------- Gauss (faster alternative) --------------------

    def gauss(self, mu, sigma):

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

        random = self.random
        z = self.gauss_next
        self.gauss_next = None
        if z is None:
            x2pi = random() * TWOPI
            g2rad = _sqrt(-2.0 * _log(1.0 - random()))
            z = _cos(x2pi) * g2rad
            self.gauss_next = _sin(x2pi) * g2rad

        return mu + z*sigma

## -------------------- beta --------------------
## See
## http://sourceforge.net/bugs/?func=detailbug&bug_id=130030&group_id=5470
## for Ivan Frohne's insightful analysis of why the original implementation:
##
##    def betavariate(self, alpha, beta):
##        # Discrete Event Simulation in C, pp 87-88.
##
##        y = self.expovariate(alpha)
##        z = self.expovariate(1.0/beta)
##        return z/(y+z)
##
## was dead wrong, and how it probably got that way.

    def betavariate(self, alpha, beta):
        # This version due to Janne Sinkkonen, and matches all the std
        # texts (e.g., Knuth Vol 2 Ed 3 pg 134 "the beta distribution").
        y = self.gammavariate(alpha, 1.)
        if y == 0:
            return 0.0
        else:
            return y / (y + self.gammavariate(beta, 1.))

## -------------------- Pareto --------------------

    def paretovariate(self, alpha):
        # Jain, pg. 495

        u = self.random()
        return 1.0 / pow(u, 1.0/alpha)

## -------------------- Weibull --------------------

    def weibullvariate(self, alpha, beta):
        # Jain, pg. 499; bug fix courtesy Bill Arms

        u = self.random()
        return alpha * pow(-_log(u), 1.0/beta)

## -------------------- test program --------------------

def _test_generator(n, funccall):
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
    stddev = _sqrt(sqsum/n - avg*avg)
    print 'avg %g, stddev %g, min %g, max %g' % \
              (avg, stddev, smallest, largest)

def _test(N=200):
    print 'TWOPI         =', TWOPI
    print 'LOG4          =', LOG4
    print 'NV_MAGICCONST =', NV_MAGICCONST
    print 'SG_MAGICCONST =', SG_MAGICCONST
    _test_generator(N, 'random()')
    _test_generator(N, 'normalvariate(0.0, 1.0)')
    _test_generator(N, 'lognormvariate(0.0, 1.0)')
    _test_generator(N, 'cunifvariate(0.0, 1.0)')
    _test_generator(N, 'expovariate(1.0)')
    _test_generator(N, 'vonmisesvariate(0.0, 1.0)')
    _test_generator(N, 'gammavariate(0.5, 1.0)')
    _test_generator(N, 'gammavariate(0.9, 1.0)')
    _test_generator(N, 'gammavariate(1.0, 1.0)')
    _test_generator(N, 'gammavariate(2.0, 1.0)')
    _test_generator(N, 'gammavariate(20.0, 1.0)')
    _test_generator(N, 'gammavariate(200.0, 1.0)')
    _test_generator(N, 'gauss(0.0, 1.0)')
    _test_generator(N, 'betavariate(3.0, 3.0)')
    _test_generator(N, 'paretovariate(1.0)')
    _test_generator(N, 'weibullvariate(1.0, 1.0)')

    # Test jumpahead.
    s = getstate()
    jumpahead(N)
    r1 = random()
    # now do it the slow way
    setstate(s)
    for i in range(N):
        random()
    r2 = random()
    if r1 != r2:
        raise ValueError("jumpahead test failed " + `(N, r1, r2)`)

# Create one instance, seeded from current time, and export its methods
# as module-level functions.  The functions are not threadsafe, and state
# is shared across all uses (both in the user's code and in the Python
# libraries), but that's fine for most programs and is easier for the
# casual user than making them instantiate their own Random() instance.
_inst = Random()
seed = _inst.seed
random = _inst.random
uniform = _inst.uniform
randint = _inst.randint
choice = _inst.choice
randrange = _inst.randrange
shuffle = _inst.shuffle
normalvariate = _inst.normalvariate
lognormvariate = _inst.lognormvariate
cunifvariate = _inst.cunifvariate
expovariate = _inst.expovariate
vonmisesvariate = _inst.vonmisesvariate
gammavariate = _inst.gammavariate
stdgamma = _inst.stdgamma
gauss = _inst.gauss
betavariate = _inst.betavariate
paretovariate = _inst.paretovariate
weibullvariate = _inst.weibullvariate
getstate = _inst.getstate
setstate = _inst.setstate
jumpahead = _inst.jumpahead
whseed = _inst.whseed

if __name__ == '__main__':
    _test()
