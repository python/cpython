:mod:`random` --- Generate pseudo-random numbers
================================================

.. module:: random
   :synopsis: Generate pseudo-random numbers with various common distributions.

**Source code:** :source:`Lib/random.py`

--------------

This module implements pseudo-random number generators for various
distributions.

For integers, there is uniform selection from a range. For sequences, there is
uniform selection of a random element, a function to generate a random
permutation of a list in-place, and a function for random sampling without
replacement.

On the real line, there are functions to compute uniform, normal (Gaussian),
lognormal, negative exponential, gamma, and beta distributions. For generating
distributions of angles, the von Mises distribution is available.

Almost all module functions depend on the basic function :func:`.random`, which
generates a random float uniformly in the half-open range ``0.0 <= X < 1.0``.
Python uses the Mersenne Twister as the core generator.  It produces 53-bit precision
floats and has a period of 2\*\*19937-1.  The underlying implementation in C is
both fast and threadsafe.  The Mersenne Twister is one of the most extensively
tested random number generators in existence.  However, being completely
deterministic, it is not suitable for all purposes, and is completely unsuitable
for cryptographic purposes.

The functions supplied by this module are actually bound methods of a hidden
instance of the :class:`random.Random` class.  You can instantiate your own
instances of :class:`Random` to get generators that don't share state.

Class :class:`Random` can also be subclassed if you want to use a different
basic generator of your own devising: see the documentation on that class for
more details.

The :mod:`random` module also provides the :class:`SystemRandom` class which
uses the system function :func:`os.urandom` to generate random numbers
from sources provided by the operating system.

.. warning::

   The pseudo-random generators of this module should not be used for
   security purposes.  For security or cryptographic uses, see the
   :mod:`secrets` module.

.. seealso::

   M. Matsumoto and T. Nishimura, "Mersenne Twister: A 623-dimensionally
   equidistributed uniform pseudorandom number generator", ACM Transactions on
   Modeling and Computer Simulation Vol. 8, No. 1, January pp.3--30 1998.


   `Complementary-Multiply-with-Carry recipe
   <https://code.activestate.com/recipes/576707/>`_ for a compatible alternative
   random number generator with a long period and comparatively simple update
   operations.


Bookkeeping functions
---------------------

.. function:: seed(a=None, version=2)

   Initialize the random number generator.

   If *a* is omitted or ``None``, the current system time is used.  If
   randomness sources are provided by the operating system, they are used
   instead of the system time (see the :func:`os.urandom` function for details
   on availability).

   If *a* is an int, it is used directly.

   With version 2 (the default), a :class:`str`, :class:`bytes`, or :class:`bytearray`
   object gets converted to an :class:`int` and all of its bits are used.

   With version 1 (provided for reproducing random sequences from older versions
   of Python), the algorithm for :class:`str` and :class:`bytes` generates a
   narrower range of seeds.

   .. versionchanged:: 3.2
      Moved to the version 2 scheme which uses all of the bits in a string seed.

   .. versionchanged:: 3.11
      The *seed* must be one of the following types:
      ``None``, :class:`int`, :class:`float`, :class:`str`,
      :class:`bytes`, or :class:`bytearray`.

.. function:: getstate()

   Return an object capturing the current internal state of the generator.  This
   object can be passed to :func:`setstate` to restore the state.


.. function:: setstate(state)

   *state* should have been obtained from a previous call to :func:`getstate`, and
   :func:`setstate` restores the internal state of the generator to what it was at
   the time :func:`getstate` was called.


Functions for bytes
-------------------

.. function:: randbytes(n)

   Generate *n* random bytes.

   This method should not be used for generating security tokens.
   Use :func:`secrets.token_bytes` instead.

   .. versionadded:: 3.9


Functions for integers
----------------------

.. function:: randrange(stop)
              randrange(start, stop[, step])

   Return a randomly selected element from ``range(start, stop, step)``.  This is
   equivalent to ``choice(range(start, stop, step))``, but doesn't actually build a
   range object.

   The positional argument pattern matches that of :func:`range`.  Keyword arguments
   should not be used because the function may use them in unexpected ways.

   .. versionchanged:: 3.2
      :meth:`randrange` is more sophisticated about producing equally distributed
      values.  Formerly it used a style like ``int(random()*n)`` which could produce
      slightly uneven distributions.

   .. deprecated:: 3.10
      The automatic conversion of non-integer types to equivalent integers is
      deprecated.  Currently ``randrange(10.0)`` is losslessly converted to
      ``randrange(10)``.  In the future, this will raise a :exc:`TypeError`.

   .. deprecated:: 3.10
      The exception raised for non-integer values such as ``randrange(10.5)``
      or ``randrange('10')`` will be changed from :exc:`ValueError` to
      :exc:`TypeError`.

.. function:: randint(a, b)

   Return a random integer *N* such that ``a <= N <= b``.  Alias for
   ``randrange(a, b+1)``.

.. function:: getrandbits(k)

   Returns a non-negative Python integer with *k* random bits. This method
   is supplied with the MersenneTwister generator and some other generators
   may also provide it as an optional part of the API. When available,
   :meth:`getrandbits` enables :meth:`randrange` to handle arbitrarily large
   ranges.

   .. versionchanged:: 3.9
      This method now accepts zero for *k*.


Functions for sequences
-----------------------

.. function:: choice(seq)

   Return a random element from the non-empty sequence *seq*. If *seq* is empty,
   raises :exc:`IndexError`.

.. function:: choices(population, weights=None, *, cum_weights=None, k=1)

   Return a *k* sized list of elements chosen from the *population* with replacement.
   If the *population* is empty, raises :exc:`IndexError`.

   If a *weights* sequence is specified, selections are made according to the
   relative weights.  Alternatively, if a *cum_weights* sequence is given, the
   selections are made according to the cumulative weights (perhaps computed
   using :func:`itertools.accumulate`).  For example, the relative weights
   ``[10, 5, 30, 5]`` are equivalent to the cumulative weights
   ``[10, 15, 45, 50]``.  Internally, the relative weights are converted to
   cumulative weights before making selections, so supplying the cumulative
   weights saves work.

   If neither *weights* nor *cum_weights* are specified, selections are made
   with equal probability.  If a weights sequence is supplied, it must be
   the same length as the *population* sequence.  It is a :exc:`TypeError`
   to specify both *weights* and *cum_weights*.

   The *weights* or *cum_weights* can use any numeric type that interoperates
   with the :class:`float` values returned by :func:`random` (that includes
   integers, floats, and fractions but excludes decimals).  Weights are assumed
   to be non-negative and finite.  A :exc:`ValueError` is raised if all
   weights are zero.

   For a given seed, the :func:`choices` function with equal weighting
   typically produces a different sequence than repeated calls to
   :func:`choice`.  The algorithm used by :func:`choices` uses floating
   point arithmetic for internal consistency and speed.  The algorithm used
   by :func:`choice` defaults to integer arithmetic with repeated selections
   to avoid small biases from round-off error.

   .. versionadded:: 3.6

   .. versionchanged:: 3.9
      Raises a :exc:`ValueError` if all weights are zero.


.. function:: shuffle(x)

   Shuffle the sequence *x* in place.

   To shuffle an immutable sequence and return a new shuffled list, use
   ``sample(x, k=len(x))`` instead.

   Note that even for small ``len(x)``, the total number of permutations of *x*
   can quickly grow larger than the period of most random number generators.
   This implies that most permutations of a long sequence can never be
   generated.  For example, a sequence of length 2080 is the largest that
   can fit within the period of the Mersenne Twister random number generator.

   .. versionchanged:: 3.11
      Removed the optional parameter *random*.


.. function:: sample(population, k, *, counts=None)

   Return a *k* length list of unique elements chosen from the population
   sequence.  Used for random sampling without replacement.

   Returns a new list containing elements from the population while leaving the
   original population unchanged.  The resulting list is in selection order so that
   all sub-slices will also be valid random samples.  This allows raffle winners
   (the sample) to be partitioned into grand prize and second place winners (the
   subslices).

   Members of the population need not be :term:`hashable` or unique.  If the population
   contains repeats, then each occurrence is a possible selection in the sample.

   Repeated elements can be specified one at a time or with the optional
   keyword-only *counts* parameter.  For example, ``sample(['red', 'blue'],
   counts=[4, 2], k=5)`` is equivalent to ``sample(['red', 'red', 'red', 'red',
   'blue', 'blue'], k=5)``.

   To choose a sample from a range of integers, use a :func:`range` object as an
   argument.  This is especially fast and space efficient for sampling from a large
   population:  ``sample(range(10000000), k=60)``.

   If the sample size is larger than the population size, a :exc:`ValueError`
   is raised.

   .. versionchanged:: 3.9
      Added the *counts* parameter.

   .. versionchanged:: 3.11

      The *population* must be a sequence.  Automatic conversion of sets
      to lists is no longer supported.


.. _real-valued-distributions:

Real-valued distributions
-------------------------

The following functions generate specific real-valued distributions. Function
parameters are named after the corresponding variables in the distribution's
equation, as used in common mathematical practice; most of these equations can
be found in any statistics text.


.. function:: random()

   Return the next random floating point number in the range ``0.0 <= X < 1.0``


.. function:: uniform(a, b)

   Return a random floating point number *N* such that ``a <= N <= b`` for
   ``a <= b`` and ``b <= N <= a`` for ``b < a``.

   The end-point value ``b`` may or may not be included in the range
   depending on floating-point rounding in the expression
   ``a + (b-a) * random()``.


.. function:: triangular(low, high, mode)

   Return a random floating point number *N* such that ``low <= N <= high`` and
   with the specified *mode* between those bounds.  The *low* and *high* bounds
   default to zero and one.  The *mode* argument defaults to the midpoint
   between the bounds, giving a symmetric distribution.


.. function:: betavariate(alpha, beta)

   Beta distribution.  Conditions on the parameters are ``alpha > 0`` and
   ``beta > 0``. Returned values range between 0 and 1.


.. function:: expovariate(lambd)

   Exponential distribution.  *lambd* is 1.0 divided by the desired
   mean.  It should be nonzero.  (The parameter would be called
   "lambda", but that is a reserved word in Python.)  Returned values
   range from 0 to positive infinity if *lambd* is positive, and from
   negative infinity to 0 if *lambd* is negative.


.. function:: gammavariate(alpha, beta)

   Gamma distribution.  (*Not* the gamma function!)  The shape and
   scale parameters, *alpha* and *beta*, must have positive values.
   (Calling conventions vary and some sources define 'beta'
   as the inverse of the scale).

   The probability distribution function is::

                 x ** (alpha - 1) * math.exp(-x / beta)
       pdf(x) =  --------------------------------------
                   math.gamma(alpha) * beta ** alpha


.. function:: gauss(mu=0.0, sigma=1.0)

   Normal distribution, also called the Gaussian distribution.
   *mu* is the mean,
   and *sigma* is the standard deviation.  This is slightly faster than
   the :func:`normalvariate` function defined below.

   Multithreading note:  When two threads call this function
   simultaneously, it is possible that they will receive the
   same return value.  This can be avoided in three ways.
   1) Have each thread use a different instance of the random
   number generator. 2) Put locks around all calls. 3) Use the
   slower, but thread-safe :func:`normalvariate` function instead.

   .. versionchanged:: 3.11
      *mu* and *sigma* now have default arguments.


.. function:: lognormvariate(mu, sigma)

   Log normal distribution.  If you take the natural logarithm of this
   distribution, you'll get a normal distribution with mean *mu* and standard
   deviation *sigma*.  *mu* can have any value, and *sigma* must be greater than
   zero.


.. function:: normalvariate(mu=0.0, sigma=1.0)

   Normal distribution.  *mu* is the mean, and *sigma* is the standard deviation.

   .. versionchanged:: 3.11
      *mu* and *sigma* now have default arguments.


.. function:: vonmisesvariate(mu, kappa)

   *mu* is the mean angle, expressed in radians between 0 and 2\*\ *pi*, and *kappa*
   is the concentration parameter, which must be greater than or equal to zero.  If
   *kappa* is equal to zero, this distribution reduces to a uniform random angle
   over the range 0 to 2\*\ *pi*.


.. function:: paretovariate(alpha)

   Pareto distribution.  *alpha* is the shape parameter.


.. function:: weibullvariate(alpha, beta)

   Weibull distribution.  *alpha* is the scale parameter and *beta* is the shape
   parameter.


Alternative Generator
---------------------

.. class:: Random([seed])

   Class that implements the default pseudo-random number generator used by the
   :mod:`random` module.

   .. versionchanged:: 3.11
      Formerly the *seed* could be any hashable object.  Now it is limited to:
      ``None``, :class:`int`, :class:`float`, :class:`str`,
      :class:`bytes`, or :class:`bytearray`.

   Subclasses of :class:`!Random` should override the following methods if they
   wish to make use of a different basic generator:

   .. method:: Random.seed(a=None, version=2)

      Override this method in subclasses to customise the :meth:`~random.seed`
      behaviour of :class:`!Random` instances.

   .. method:: Random.getstate()

      Override this method in subclasses to customise the :meth:`~random.getstate`
      behaviour of :class:`!Random` instances.

   .. method:: Random.setstate(state)

      Override this method in subclasses to customise the :meth:`~random.setstate`
      behaviour of :class:`!Random` instances.

   .. method:: Random.random()

      Override this method in subclasses to customise the :meth:`~random.random`
      behaviour of :class:`!Random` instances.

   Optionally, a custom generator subclass can also supply the following method:

   .. method:: Random.getrandbits(k)

      Override this method in subclasses to customise the
      :meth:`~random.getrandbits` behaviour of :class:`!Random` instances.


.. class:: SystemRandom([seed])

   Class that uses the :func:`os.urandom` function for generating random numbers
   from sources provided by the operating system. Not available on all systems.
   Does not rely on software state, and sequences are not reproducible. Accordingly,
   the :meth:`seed` method has no effect and is ignored.
   The :meth:`getstate` and :meth:`setstate` methods raise
   :exc:`NotImplementedError` if called.


Notes on Reproducibility
------------------------

Sometimes it is useful to be able to reproduce the sequences given by a
pseudo-random number generator.  By re-using a seed value, the same sequence should be
reproducible from run to run as long as multiple threads are not running.

Most of the random module's algorithms and seeding functions are subject to
change across Python versions, but two aspects are guaranteed not to change:

* If a new seeding method is added, then a backward compatible seeder will be
  offered.

* The generator's :meth:`~Random.random` method will continue to produce the same
  sequence when the compatible seeder is given the same seed.

.. _random-examples:

Examples
--------

Basic examples::

   >>> random()                          # Random float:  0.0 <= x < 1.0
   0.37444887175646646

   >>> uniform(2.5, 10.0)                # Random float:  2.5 <= x <= 10.0
   3.1800146073117523

   >>> expovariate(1 / 5)                # Interval between arrivals averaging 5 seconds
   5.148957571865031

   >>> randrange(10)                     # Integer from 0 to 9 inclusive
   7

   >>> randrange(0, 101, 2)              # Even integer from 0 to 100 inclusive
   26

   >>> choice(['win', 'lose', 'draw'])   # Single random element from a sequence
   'draw'

   >>> deck = 'ace two three four'.split()
   >>> shuffle(deck)                     # Shuffle a list
   >>> deck
   ['four', 'two', 'ace', 'three']

   >>> sample([10, 20, 30, 40, 50], k=4) # Four samples without replacement
   [40, 10, 50, 30]

Simulations::

   >>> # Six roulette wheel spins (weighted sampling with replacement)
   >>> choices(['red', 'black', 'green'], [18, 18, 2], k=6)
   ['red', 'green', 'black', 'black', 'red', 'black']

   >>> # Deal 20 cards without replacement from a deck
   >>> # of 52 playing cards, and determine the proportion of cards
   >>> # with a ten-value:  ten, jack, queen, or king.
   >>> dealt = sample(['tens', 'low cards'], counts=[16, 36], k=20)
   >>> dealt.count('tens') / 20
   0.15

   >>> # Estimate the probability of getting 5 or more heads from 7 spins
   >>> # of a biased coin that settles on heads 60% of the time.
   >>> def trial():
   ...     return choices('HT', cum_weights=(0.60, 1.00), k=7).count('H') >= 5
   ...
   >>> sum(trial() for i in range(10_000)) / 10_000
   0.4169

   >>> # Probability of the median of 5 samples being in middle two quartiles
   >>> def trial():
   ...     return 2_500 <= sorted(choices(range(10_000), k=5))[2] < 7_500
   ...
   >>> sum(trial() for i in range(10_000)) / 10_000
   0.7958

Example of `statistical bootstrapping
<https://en.wikipedia.org/wiki/Bootstrapping_(statistics)>`_ using resampling
with replacement to estimate a confidence interval for the mean of a sample::

   # https://www.thoughtco.com/example-of-bootstrapping-3126155
   from statistics import fmean as mean
   from random import choices

   data = [41, 50, 29, 37, 81, 30, 73, 63, 20, 35, 68, 22, 60, 31, 95]
   means = sorted(mean(choices(data, k=len(data))) for i in range(100))
   print(f'The sample mean of {mean(data):.1f} has a 90% confidence '
         f'interval from {means[5]:.1f} to {means[94]:.1f}')

Example of a `resampling permutation test
<https://en.wikipedia.org/wiki/Resampling_(statistics)#Permutation_tests>`_
to determine the statistical significance or `p-value
<https://en.wikipedia.org/wiki/P-value>`_ of an observed difference
between the effects of a drug versus a placebo::

    # Example from "Statistics is Easy" by Dennis Shasha and Manda Wilson
    from statistics import fmean as mean
    from random import shuffle

    drug = [54, 73, 53, 70, 73, 68, 52, 65, 65]
    placebo = [54, 51, 58, 44, 55, 52, 42, 47, 58, 46]
    observed_diff = mean(drug) - mean(placebo)

    n = 10_000
    count = 0
    combined = drug + placebo
    for i in range(n):
        shuffle(combined)
        new_diff = mean(combined[:len(drug)]) - mean(combined[len(drug):])
        count += (new_diff >= observed_diff)

    print(f'{n} label reshufflings produced only {count} instances with a difference')
    print(f'at least as extreme as the observed difference of {observed_diff:.1f}.')
    print(f'The one-sided p-value of {count / n:.4f} leads us to reject the null')
    print(f'hypothesis that there is no difference between the drug and the placebo.')

Simulation of arrival times and service deliveries for a multiserver queue::

    from heapq import heapify, heapreplace
    from random import expovariate, gauss
    from statistics import mean, quantiles

    average_arrival_interval = 5.6
    average_service_time = 15.0
    stdev_service_time = 3.5
    num_servers = 3

    waits = []
    arrival_time = 0.0
    servers = [0.0] * num_servers  # time when each server becomes available
    heapify(servers)
    for i in range(1_000_000):
        arrival_time += expovariate(1.0 / average_arrival_interval)
        next_server_available = servers[0]
        wait = max(0.0, next_server_available - arrival_time)
        waits.append(wait)
        service_duration = max(0.0, gauss(average_service_time, stdev_service_time))
        service_completed = arrival_time + wait + service_duration
        heapreplace(servers, service_completed)

    print(f'Mean wait: {mean(waits):.1f}   Max wait: {max(waits):.1f}')
    print('Quartiles:', [round(q, 1) for q in quantiles(waits)])

.. seealso::

   `Statistics for Hackers <https://www.youtube.com/watch?v=Iq9DzN6mvYA>`_
   a video tutorial by
   `Jake Vanderplas <https://us.pycon.org/2016/speaker/profile/295/>`_
   on statistical analysis using just a few fundamental concepts
   including simulation, sampling, shuffling, and cross-validation.

   `Economics Simulation
   <https://nbviewer.org/url/norvig.com/ipython/Economics.ipynb>`_
   a simulation of a marketplace by
   `Peter Norvig <https://norvig.com/bio.html>`_ that shows effective
   use of many of the tools and distributions provided by this module
   (gauss, uniform, sample, betavariate, choice, triangular, and randrange).

   `A Concrete Introduction to Probability (using Python)
   <https://nbviewer.org/url/norvig.com/ipython/Probability.ipynb>`_
   a tutorial by `Peter Norvig <https://norvig.com/bio.html>`_ covering
   the basics of probability theory, how to write simulations, and
   how to perform data analysis using Python.


Recipes
-------

These recipes show how to efficiently make random selections
from the combinatoric iterators in the :mod:`itertools` module:

.. testcode::
   import random

   def random_product(*args, repeat=1):
       "Random selection from itertools.product(*args, **kwds)"
       pools = [tuple(pool) for pool in args] * repeat
       return tuple(map(random.choice, pools))

   def random_permutation(iterable, r=None):
       "Random selection from itertools.permutations(iterable, r)"
       pool = tuple(iterable)
       r = len(pool) if r is None else r
       return tuple(random.sample(pool, r))

   def random_combination(iterable, r):
       "Random selection from itertools.combinations(iterable, r)"
       pool = tuple(iterable)
       n = len(pool)
       indices = sorted(random.sample(range(n), r))
       return tuple(pool[i] for i in indices)

   def random_combination_with_replacement(iterable, r):
       "Choose r elements with replacement.  Order the result to match the iterable."
       # Result will be in set(itertools.combinations_with_replacement(iterable, r)).
       pool = tuple(iterable)
       n = len(pool)
       indices = sorted(random.choices(range(n), k=r))
       return tuple(pool[i] for i in indices)

The default :func:`.random` returns multiples of 2⁻⁵³ in the range
*0.0 ≤ x < 1.0*.  All such numbers are evenly spaced and are exactly
representable as Python floats.  However, many other representable
floats in that interval are not possible selections.  For example,
``0.05954861408025609`` isn't an integer multiple of 2⁻⁵³.

The following recipe takes a different approach.  All floats in the
interval are possible selections.  The mantissa comes from a uniform
distribution of integers in the range *2⁵² ≤ mantissa < 2⁵³*.  The
exponent comes from a geometric distribution where exponents smaller
than *-53* occur half as often as the next larger exponent.

::

    from random import Random
    from math import ldexp

    class FullRandom(Random):

        def random(self):
            mantissa = 0x10_0000_0000_0000 | self.getrandbits(52)
            exponent = -53
            x = 0
            while not x:
                x = self.getrandbits(32)
                exponent += x.bit_length() - 32
            return ldexp(mantissa, exponent)

All :ref:`real valued distributions <real-valued-distributions>`
in the class will use the new method::

    >>> fr = FullRandom()
    >>> fr.random()
    0.05954861408025609
    >>> fr.expovariate(0.25)
    8.87925541791544

The recipe is conceptually equivalent to an algorithm that chooses from
all the multiples of 2⁻¹⁰⁷⁴ in the range *0.0 ≤ x < 1.0*.  All such
numbers are evenly spaced, but most have to be rounded down to the
nearest representable Python float.  (The value 2⁻¹⁰⁷⁴ is the smallest
positive unnormalized float and is equal to ``math.ulp(0.0)``.)


.. seealso::

   `Generating Pseudo-random Floating-Point Values
   <https://allendowney.com/research/rand/downey07randfloat.pdf>`_ a
   paper by Allen B. Downey describing ways to generate more
   fine-grained floats than normally generated by :func:`.random`.
