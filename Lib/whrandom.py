"""Wichman-Hill random number generator.

Wichmann, B. A. & Hill, I. D. (1982)
Algorithm AS 183:
An efficient and portable pseudo-random number generator
Applied Statistics 31 (1982) 188-190

see also:
        Correction to Algorithm AS 183
        Applied Statistics 33 (1984) 123

        McLeod, A. I. (1985)
        A remark on Algorithm AS 183
        Applied Statistics 34 (1985),198-200


USE:
whrandom.random()       yields double precision random numbers
                        uniformly distributed between 0 and 1.

whrandom.seed(x, y, z)  must be called before whrandom.random()
                        to seed the generator

There is also an interface to create multiple independent
random generators, and to choose from other ranges.



Multi-threading note: the random number generator used here is not
thread-safe; it is possible that nearly simultaneous calls in
different theads return the same random value.  To avoid this, you
have to use a lock around all calls.  (I didn't want to slow this
down in the serial case by using a lock here.)
"""

import warnings
warnings.warn("the whrandom module is deprecated; please use the random module",
              DeprecationWarning)

# Translated by Guido van Rossum from C source provided by
# Adrian Baddeley.


class whrandom:
    def __init__(self, x = 0, y = 0, z = 0):
        """Initialize an instance.
        Without arguments, initialize from current time.
        With arguments (x, y, z), initialize from them."""
        self.seed(x, y, z)

    def seed(self, x = 0, y = 0, z = 0):
        """Set the seed from (x, y, z).
        These must be integers in the range [0, 256)."""
        if not type(x) == type(y) == type(z) == type(0):
            raise TypeError, 'seeds must be integers'
        if not (0 <= x < 256 and 0 <= y < 256 and 0 <= z < 256):
            raise ValueError, 'seeds must be in range(0, 256)'
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

    def random(self):
        """Get the next random number in the range [0.0, 1.0)."""
        # This part is thread-unsafe:
        # BEGIN CRITICAL SECTION
        x, y, z = self._seed
        #
        x = (171 * x) % 30269
        y = (172 * y) % 30307
        z = (170 * z) % 30323
        #
        self._seed = x, y, z
        # END CRITICAL SECTION
        #
        return (x/30269.0 + y/30307.0 + z/30323.0) % 1.0

    def uniform(self, a, b):
        """Get a random number in the range [a, b)."""
        return a + (b-a) * self.random()

    def randint(self, a, b):
        """Get a random integer in the range [a, b] including
        both end points.

        (Deprecated; use randrange below.)"""
        return self.randrange(a, b+1)

    def choice(self, seq):
        """Choose a random element from a non-empty sequence."""
        return seq[int(self.random() * len(seq))]

    def randrange(self, start, stop=None, step=1, int=int, default=None):
        """Choose a random item from range(start, stop[, step]).

        This fixes the problem with randint() which includes the
        endpoint; in Python this is usually not what you want.
        Do not supply the 'int' and 'default' arguments."""
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


# Initialize from the current time
_inst = whrandom()
seed = _inst.seed
random = _inst.random
uniform = _inst.uniform
randint = _inst.randint
choice = _inst.choice
randrange = _inst.randrange
