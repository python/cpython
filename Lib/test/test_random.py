import unittest
import unittest.mock
import random
import os
import time
import pickle
import warnings
from functools import partial
from math import log, exp, pi, fsum, sin, factorial
from test import support
from fractions import Fraction
from collections import Counter

class TestBasicOps:
    # Superclass with tests common to all generators.
    # Subclasses must arrange for self.gen to retrieve the Random instance
    # to be tested.

    def randomlist(self, n):
        """Helper function to make a list of random numbers"""
        return [self.gen.random() for i in range(n)]

    def test_autoseed(self):
        self.gen.seed()
        state1 = self.gen.getstate()
        time.sleep(0.1)
        self.gen.seed()      # different seeds at different times
        state2 = self.gen.getstate()
        self.assertNotEqual(state1, state2)

    def test_saverestore(self):
        N = 1000
        self.gen.seed()
        state = self.gen.getstate()
        randseq = self.randomlist(N)
        self.gen.setstate(state)    # should regenerate the same sequence
        self.assertEqual(randseq, self.randomlist(N))

    def test_seedargs(self):
        # Seed value with a negative hash.
        class MySeed(object):
            def __hash__(self):
                return -1729
        for arg in [None, 0, 1, -1, 10**20, -(10**20),
                    False, True, 3.14, 'a']:
            self.gen.seed(arg)

        for arg in [1+2j, tuple('abc'), MySeed()]:
            with self.assertWarns(DeprecationWarning):
                self.gen.seed(arg)

        for arg in [list(range(3)), dict(one=1)]:
            with self.assertWarns(DeprecationWarning):
                self.assertRaises(TypeError, self.gen.seed, arg)
        self.assertRaises(TypeError, self.gen.seed, 1, 2, 3, 4)
        self.assertRaises(TypeError, type(self.gen), [])

    @unittest.mock.patch('random._urandom') # os.urandom
    def test_seed_when_randomness_source_not_found(self, urandom_mock):
        # Random.seed() uses time.time() when an operating system specific
        # randomness source is not found. To test this on machines where it
        # exists, run the above test, test_seedargs(), again after mocking
        # os.urandom() so that it raises the exception expected when the
        # randomness source is not available.
        urandom_mock.side_effect = NotImplementedError
        self.test_seedargs()

    def test_shuffle(self):
        shuffle = self.gen.shuffle
        lst = []
        shuffle(lst)
        self.assertEqual(lst, [])
        lst = [37]
        shuffle(lst)
        self.assertEqual(lst, [37])
        seqs = [list(range(n)) for n in range(10)]
        shuffled_seqs = [list(range(n)) for n in range(10)]
        for shuffled_seq in shuffled_seqs:
            shuffle(shuffled_seq)
        for (seq, shuffled_seq) in zip(seqs, shuffled_seqs):
            self.assertEqual(len(seq), len(shuffled_seq))
            self.assertEqual(set(seq), set(shuffled_seq))
        # The above tests all would pass if the shuffle was a
        # no-op. The following non-deterministic test covers that.  It
        # asserts that the shuffled sequence of 1000 distinct elements
        # must be different from the original one. Although there is
        # mathematically a non-zero probability that this could
        # actually happen in a genuinely random shuffle, it is
        # completely negligible, given that the number of possible
        # permutations of 1000 objects is 1000! (factorial of 1000),
        # which is considerably larger than the number of atoms in the
        # universe...
        lst = list(range(1000))
        shuffled_lst = list(range(1000))
        shuffle(shuffled_lst)
        self.assertTrue(lst != shuffled_lst)
        shuffle(lst)
        self.assertTrue(lst != shuffled_lst)
        self.assertRaises(TypeError, shuffle, (1, 2, 3))

    def test_shuffle_random_argument(self):
        # Test random argument to shuffle.
        shuffle = self.gen.shuffle
        mock_random = unittest.mock.Mock(return_value=0.5)
        seq = bytearray(b'abcdefghijk')
        with self.assertWarns(DeprecationWarning):
            shuffle(seq, mock_random)
        mock_random.assert_called_with()

    def test_choice(self):
        choice = self.gen.choice
        with self.assertRaises(IndexError):
            choice([])
        self.assertEqual(choice([50]), 50)
        self.assertIn(choice([25, 75]), [25, 75])

    def test_sample(self):
        # For the entire allowable range of 0 <= k <= N, validate that
        # the sample is of the correct length and contains only unique items
        N = 100
        population = range(N)
        for k in range(N+1):
            s = self.gen.sample(population, k)
            self.assertEqual(len(s), k)
            uniq = set(s)
            self.assertEqual(len(uniq), k)
            self.assertTrue(uniq <= set(population))
        self.assertEqual(self.gen.sample([], 0), [])  # test edge case N==k==0
        # Exception raised if size of sample exceeds that of population
        self.assertRaises(ValueError, self.gen.sample, population, N+1)
        self.assertRaises(ValueError, self.gen.sample, [], -1)

    def test_sample_distribution(self):
        # For the entire allowable range of 0 <= k <= N, validate that
        # sample generates all possible permutations
        n = 5
        pop = range(n)
        trials = 10000  # large num prevents false negatives without slowing normal case
        for k in range(n):
            expected = factorial(n) // factorial(n-k)
            perms = {}
            for i in range(trials):
                perms[tuple(self.gen.sample(pop, k))] = None
                if len(perms) == expected:
                    break
            else:
                self.fail()

    def test_sample_inputs(self):
        # SF bug #801342 -- population can be any iterable defining __len__()
        self.gen.sample(range(20), 2)
        self.gen.sample(range(20), 2)
        self.gen.sample(str('abcdefghijklmnopqrst'), 2)
        self.gen.sample(tuple('abcdefghijklmnopqrst'), 2)

    def test_sample_on_dicts(self):
        self.assertRaises(TypeError, self.gen.sample, dict.fromkeys('abcdef'), 2)

    def test_sample_on_sets(self):
        with self.assertWarns(DeprecationWarning):
            population = {10, 20, 30, 40, 50, 60, 70}
            self.gen.sample(population, k=5)

    def test_sample_with_counts(self):
        sample = self.gen.sample

        # General case
        colors = ['red', 'green', 'blue', 'orange', 'black', 'brown', 'amber']
        counts = [500,      200,     20,       10,       5,       0,       1 ]
        k = 700
        summary = Counter(sample(colors, counts=counts, k=k))
        self.assertEqual(sum(summary.values()), k)
        for color, weight in zip(colors, counts):
            self.assertLessEqual(summary[color], weight)
        self.assertNotIn('brown', summary)

        # Case that exhausts the population
        k = sum(counts)
        summary = Counter(sample(colors, counts=counts, k=k))
        self.assertEqual(sum(summary.values()), k)
        for color, weight in zip(colors, counts):
            self.assertLessEqual(summary[color], weight)
        self.assertNotIn('brown', summary)

        # Case with population size of 1
        summary = Counter(sample(['x'], counts=[10], k=8))
        self.assertEqual(summary, Counter(x=8))

        # Case with all counts equal.
        nc = len(colors)
        summary = Counter(sample(colors, counts=[10]*nc, k=10*nc))
        self.assertEqual(summary, Counter(10*colors))

        # Test error handling
        with self.assertRaises(TypeError):
            sample(['red', 'green', 'blue'], counts=10, k=10)               # counts not iterable
        with self.assertRaises(ValueError):
            sample(['red', 'green', 'blue'], counts=[-3, -7, -8], k=2)      # counts are negative
        with self.assertRaises(ValueError):
            sample(['red', 'green', 'blue'], counts=[0, 0, 0], k=2)         # counts are zero
        with self.assertRaises(ValueError):
            sample(['red', 'green'], counts=[10, 10], k=21)                 # population too small
        with self.assertRaises(ValueError):
            sample(['red', 'green', 'blue'], counts=[1, 2], k=2)            # too few counts
        with self.assertRaises(ValueError):
            sample(['red', 'green', 'blue'], counts=[1, 2, 3, 4], k=2)      # too many counts

    def test_sample_counts_equivalence(self):
        # Test the documented strong equivalence to a sample with repeated elements.
        # We run this test on random.Random() which makes deterministic selections
        # for a given seed value.
        sample = random.sample
        seed = random.seed

        colors =  ['red', 'green', 'blue', 'orange', 'black', 'amber']
        counts = [500,      200,     20,       10,       5,       1 ]
        k = 700
        seed(8675309)
        s1 = sample(colors, counts=counts, k=k)
        seed(8675309)
        expanded = [color for (color, count) in zip(colors, counts) for i in range(count)]
        self.assertEqual(len(expanded), sum(counts))
        s2 = sample(expanded, k=k)
        self.assertEqual(s1, s2)

        pop = 'abcdefghi'
        counts = [10, 9, 8, 7, 6, 5, 4, 3, 2]
        seed(8675309)
        s1 = ''.join(sample(pop, counts=counts, k=30))
        expanded = ''.join([letter for (letter, count) in zip(pop, counts) for i in range(count)])
        seed(8675309)
        s2 = ''.join(sample(expanded, k=30))
        self.assertEqual(s1, s2)

    def test_choices(self):
        choices = self.gen.choices
        data = ['red', 'green', 'blue', 'yellow']
        str_data = 'abcd'
        range_data = range(4)
        set_data = set(range(4))

        # basic functionality
        for sample in [
            choices(data, k=5),
            choices(data, range(4), k=5),
            choices(k=5, population=data, weights=range(4)),
            choices(k=5, population=data, cum_weights=range(4)),
        ]:
            self.assertEqual(len(sample), 5)
            self.assertEqual(type(sample), list)
            self.assertTrue(set(sample) <= set(data))

        # test argument handling
        with self.assertRaises(TypeError):                               # missing arguments
            choices(2)

        self.assertEqual(choices(data, k=0), [])                         # k == 0
        self.assertEqual(choices(data, k=-1), [])                        # negative k behaves like ``[0] * -1``
        with self.assertRaises(TypeError):
            choices(data, k=2.5)                                         # k is a float

        self.assertTrue(set(choices(str_data, k=5)) <= set(str_data))    # population is a string sequence
        self.assertTrue(set(choices(range_data, k=5)) <= set(range_data))  # population is a range
        with self.assertRaises(TypeError):
            choices(set_data, k=2)                                       # population is not a sequence

        self.assertTrue(set(choices(data, None, k=5)) <= set(data))      # weights is None
        self.assertTrue(set(choices(data, weights=None, k=5)) <= set(data))
        with self.assertRaises(ValueError):
            choices(data, [1,2], k=5)                                    # len(weights) != len(population)
        with self.assertRaises(TypeError):
            choices(data, 10, k=5)                                       # non-iterable weights
        with self.assertRaises(TypeError):
            choices(data, [None]*4, k=5)                                 # non-numeric weights
        for weights in [
                [15, 10, 25, 30],                                                 # integer weights
                [15.1, 10.2, 25.2, 30.3],                                         # float weights
                [Fraction(1, 3), Fraction(2, 6), Fraction(3, 6), Fraction(4, 6)], # fractional weights
                [True, False, True, False]                                        # booleans (include / exclude)
        ]:
            self.assertTrue(set(choices(data, weights, k=5)) <= set(data))

        with self.assertRaises(ValueError):
            choices(data, cum_weights=[1,2], k=5)                        # len(weights) != len(population)
        with self.assertRaises(TypeError):
            choices(data, cum_weights=10, k=5)                           # non-iterable cum_weights
        with self.assertRaises(TypeError):
            choices(data, cum_weights=[None]*4, k=5)                     # non-numeric cum_weights
        with self.assertRaises(TypeError):
            choices(data, range(4), cum_weights=range(4), k=5)           # both weights and cum_weights
        for weights in [
                [15, 10, 25, 30],                                                 # integer cum_weights
                [15.1, 10.2, 25.2, 30.3],                                         # float cum_weights
                [Fraction(1, 3), Fraction(2, 6), Fraction(3, 6), Fraction(4, 6)], # fractional cum_weights
        ]:
            self.assertTrue(set(choices(data, cum_weights=weights, k=5)) <= set(data))

        # Test weight focused on a single element of the population
        self.assertEqual(choices('abcd', [1, 0, 0, 0]), ['a'])
        self.assertEqual(choices('abcd', [0, 1, 0, 0]), ['b'])
        self.assertEqual(choices('abcd', [0, 0, 1, 0]), ['c'])
        self.assertEqual(choices('abcd', [0, 0, 0, 1]), ['d'])

        # Test consistency with random.choice() for empty population
        with self.assertRaises(IndexError):
            choices([], k=1)
        with self.assertRaises(IndexError):
            choices([], weights=[], k=1)
        with self.assertRaises(IndexError):
            choices([], cum_weights=[], k=5)

    def test_choices_subnormal(self):
        # Subnormal weights would occasionally trigger an IndexError
        # in choices() when the value returned by random() was large
        # enough to make `random() * total` round up to the total.
        # See https://bugs.python.org/msg275594 for more detail.
        choices = self.gen.choices
        choices(population=[1, 2], weights=[1e-323, 1e-323], k=5000)

    def test_choices_with_all_zero_weights(self):
        # See issue #38881
        with self.assertRaises(ValueError):
            self.gen.choices('AB', [0.0, 0.0])

    def test_gauss(self):
        # Ensure that the seed() method initializes all the hidden state.  In
        # particular, through 2.2.1 it failed to reset a piece of state used
        # by (and only by) the .gauss() method.

        for seed in 1, 12, 123, 1234, 12345, 123456, 654321:
            self.gen.seed(seed)
            x1 = self.gen.random()
            y1 = self.gen.gauss(0, 1)

            self.gen.seed(seed)
            x2 = self.gen.random()
            y2 = self.gen.gauss(0, 1)

            self.assertEqual(x1, x2)
            self.assertEqual(y1, y2)

    def test_getrandbits(self):
        # Verify ranges
        for k in range(1, 1000):
            self.assertTrue(0 <= self.gen.getrandbits(k) < 2**k)
        self.assertEqual(self.gen.getrandbits(0), 0)

        # Verify all bits active
        getbits = self.gen.getrandbits
        for span in [1, 2, 3, 4, 31, 32, 32, 52, 53, 54, 119, 127, 128, 129]:
            all_bits = 2**span-1
            cum = 0
            cpl_cum = 0
            for i in range(100):
                v = getbits(span)
                cum |= v
                cpl_cum |= all_bits ^ v
            self.assertEqual(cum, all_bits)
            self.assertEqual(cpl_cum, all_bits)

        # Verify argument checking
        self.assertRaises(TypeError, self.gen.getrandbits)
        self.assertRaises(TypeError, self.gen.getrandbits, 1, 2)
        self.assertRaises(ValueError, self.gen.getrandbits, -1)
        self.assertRaises(TypeError, self.gen.getrandbits, 10.1)

    def test_pickling(self):
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            state = pickle.dumps(self.gen, proto)
            origseq = [self.gen.random() for i in range(10)]
            newgen = pickle.loads(state)
            restoredseq = [newgen.random() for i in range(10)]
            self.assertEqual(origseq, restoredseq)

    def test_bug_1727780(self):
        # verify that version-2-pickles can be loaded
        # fine, whether they are created on 32-bit or 64-bit
        # platforms, and that version-3-pickles load fine.
        files = [("randv2_32.pck", 780),
                 ("randv2_64.pck", 866),
                 ("randv3.pck", 343)]
        for file, value in files:
            with open(support.findfile(file),"rb") as f:
                r = pickle.load(f)
            self.assertEqual(int(r.random()*1000), value)

    def test_bug_9025(self):
        # Had problem with an uneven distribution in int(n*random())
        # Verify the fix by checking that distributions fall within expectations.
        n = 100000
        randrange = self.gen.randrange
        k = sum(randrange(6755399441055744) % 3 == 2 for i in range(n))
        self.assertTrue(0.30 < k/n < .37, (k/n))

    def test_randbytes(self):
        # Verify ranges
        for n in range(1, 10):
            data = self.gen.randbytes(n)
            self.assertEqual(type(data), bytes)
            self.assertEqual(len(data), n)

        self.assertEqual(self.gen.randbytes(0), b'')

        # Verify argument checking
        self.assertRaises(TypeError, self.gen.randbytes)
        self.assertRaises(TypeError, self.gen.randbytes, 1, 2)
        self.assertRaises(ValueError, self.gen.randbytes, -1)
        self.assertRaises(TypeError, self.gen.randbytes, 1.0)


try:
    random.SystemRandom().random()
except NotImplementedError:
    SystemRandom_available = False
else:
    SystemRandom_available = True

@unittest.skipUnless(SystemRandom_available, "random.SystemRandom not available")
class SystemRandom_TestBasicOps(TestBasicOps, unittest.TestCase):
    gen = random.SystemRandom()

    def test_autoseed(self):
        # Doesn't need to do anything except not fail
        self.gen.seed()

    def test_saverestore(self):
        self.assertRaises(NotImplementedError, self.gen.getstate)
        self.assertRaises(NotImplementedError, self.gen.setstate, None)

    def test_seedargs(self):
        # Doesn't need to do anything except not fail
        self.gen.seed(100)

    def test_gauss(self):
        self.gen.gauss_next = None
        self.gen.seed(100)
        self.assertEqual(self.gen.gauss_next, None)

    def test_pickling(self):
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            self.assertRaises(NotImplementedError, pickle.dumps, self.gen, proto)

    def test_53_bits_per_float(self):
        # This should pass whenever a C double has 53 bit precision.
        span = 2 ** 53
        cum = 0
        for i in range(100):
            cum |= int(self.gen.random() * span)
        self.assertEqual(cum, span-1)

    def test_bigrand(self):
        # The randrange routine should build-up the required number of bits
        # in stages so that all bit positions are active.
        span = 2 ** 500
        cum = 0
        for i in range(100):
            r = self.gen.randrange(span)
            self.assertTrue(0 <= r < span)
            cum |= r
        self.assertEqual(cum, span-1)

    def test_bigrand_ranges(self):
        for i in [40,80, 160, 200, 211, 250, 375, 512, 550]:
            start = self.gen.randrange(2 ** (i-2))
            stop = self.gen.randrange(2 ** i)
            if stop <= start:
                continue
            self.assertTrue(start <= self.gen.randrange(start, stop) < stop)

    def test_rangelimits(self):
        for start, stop in [(-2,0), (-(2**60)-2,-(2**60)), (2**60,2**60+2)]:
            self.assertEqual(set(range(start,stop)),
                set([self.gen.randrange(start,stop) for i in range(100)]))

    def test_randrange_nonunit_step(self):
        rint = self.gen.randrange(0, 10, 2)
        self.assertIn(rint, (0, 2, 4, 6, 8))
        rint = self.gen.randrange(0, 2, 2)
        self.assertEqual(rint, 0)

    def test_randrange_errors(self):
        raises = partial(self.assertRaises, ValueError, self.gen.randrange)
        # Empty range
        raises(3, 3)
        raises(-721)
        raises(0, 100, -12)
        # Non-integer start/stop
        raises(3.14159)
        raises(0, 2.71828)
        # Zero and non-integer step
        raises(0, 42, 0)
        raises(0, 42, 3.14159)

    def test_randbelow_logic(self, _log=log, int=int):
        # check bitcount transition points:  2**i and 2**(i+1)-1
        # show that: k = int(1.001 + _log(n, 2))
        # is equal to or one greater than the number of bits in n
        for i in range(1, 1000):
            n = 1 << i # check an exact power of two
            numbits = i+1
            k = int(1.00001 + _log(n, 2))
            self.assertEqual(k, numbits)
            self.assertEqual(n, 2**(k-1))

            n += n - 1      # check 1 below the next power of two
            k = int(1.00001 + _log(n, 2))
            self.assertIn(k, [numbits, numbits+1])
            self.assertTrue(2**k > n > 2**(k-2))

            n -= n >> 15     # check a little farther below the next power of two
            k = int(1.00001 + _log(n, 2))
            self.assertEqual(k, numbits)        # note the stronger assertion
            self.assertTrue(2**k > n > 2**(k-1))   # note the stronger assertion


class MersenneTwister_TestBasicOps(TestBasicOps, unittest.TestCase):
    gen = random.Random()

    def test_guaranteed_stable(self):
        # These sequences are guaranteed to stay the same across versions of python
        self.gen.seed(3456147, version=1)
        self.assertEqual([self.gen.random().hex() for i in range(4)],
            ['0x1.ac362300d90d2p-1', '0x1.9d16f74365005p-1',
             '0x1.1ebb4352e4c4dp-1', '0x1.1a7422abf9c11p-1'])
        self.gen.seed("the quick brown fox", version=2)
        self.assertEqual([self.gen.random().hex() for i in range(4)],
            ['0x1.1239ddfb11b7cp-3', '0x1.b3cbb5c51b120p-4',
             '0x1.8c4f55116b60fp-1', '0x1.63eb525174a27p-1'])

    def test_bug_27706(self):
        # Verify that version 1 seeds are unaffected by hash randomization

        self.gen.seed('nofar', version=1)   # hash('nofar') == 5990528763808513177
        self.assertEqual([self.gen.random().hex() for i in range(4)],
            ['0x1.8645314505ad7p-1', '0x1.afb1f82e40a40p-5',
             '0x1.2a59d2285e971p-1', '0x1.56977142a7880p-6'])

        self.gen.seed('rachel', version=1)  # hash('rachel') == -9091735575445484789
        self.assertEqual([self.gen.random().hex() for i in range(4)],
            ['0x1.0b294cc856fcdp-1', '0x1.2ad22d79e77b8p-3',
             '0x1.3052b9c072678p-2', '0x1.578f332106574p-3'])

        self.gen.seed('', version=1)        # hash('') == 0
        self.assertEqual([self.gen.random().hex() for i in range(4)],
            ['0x1.b0580f98a7dbep-1', '0x1.84129978f9c1ap-1',
             '0x1.aeaa51052e978p-2', '0x1.092178fb945a6p-2'])

    def test_bug_31478(self):
        # There shouldn't be an assertion failure in _random.Random.seed() in
        # case the argument has a bad __abs__() method.
        class BadInt(int):
            def __abs__(self):
                return None
        try:
            self.gen.seed(BadInt())
        except TypeError:
            pass

    def test_bug_31482(self):
        # Verify that version 1 seeds are unaffected by hash randomization
        # when the seeds are expressed as bytes rather than strings.
        # The hash(b) values listed are the Python2.7 hash() values
        # which were used for seeding.

        self.gen.seed(b'nofar', version=1)   # hash('nofar') == 5990528763808513177
        self.assertEqual([self.gen.random().hex() for i in range(4)],
            ['0x1.8645314505ad7p-1', '0x1.afb1f82e40a40p-5',
             '0x1.2a59d2285e971p-1', '0x1.56977142a7880p-6'])

        self.gen.seed(b'rachel', version=1)  # hash('rachel') == -9091735575445484789
        self.assertEqual([self.gen.random().hex() for i in range(4)],
            ['0x1.0b294cc856fcdp-1', '0x1.2ad22d79e77b8p-3',
             '0x1.3052b9c072678p-2', '0x1.578f332106574p-3'])

        self.gen.seed(b'', version=1)        # hash('') == 0
        self.assertEqual([self.gen.random().hex() for i in range(4)],
            ['0x1.b0580f98a7dbep-1', '0x1.84129978f9c1ap-1',
             '0x1.aeaa51052e978p-2', '0x1.092178fb945a6p-2'])

        b = b'\x00\x20\x40\x60\x80\xA0\xC0\xE0\xF0'
        self.gen.seed(b, version=1)         # hash(b) == 5015594239749365497
        self.assertEqual([self.gen.random().hex() for i in range(4)],
            ['0x1.52c2fde444d23p-1', '0x1.875174f0daea4p-2',
             '0x1.9e9b2c50e5cd2p-1', '0x1.fa57768bd321cp-2'])

    def test_setstate_first_arg(self):
        self.assertRaises(ValueError, self.gen.setstate, (1, None, None))

    def test_setstate_middle_arg(self):
        start_state = self.gen.getstate()
        # Wrong type, s/b tuple
        self.assertRaises(TypeError, self.gen.setstate, (2, None, None))
        # Wrong length, s/b 625
        self.assertRaises(ValueError, self.gen.setstate, (2, (1,2,3), None))
        # Wrong type, s/b tuple of 625 ints
        self.assertRaises(TypeError, self.gen.setstate, (2, ('a',)*625, None))
        # Last element s/b an int also
        self.assertRaises(TypeError, self.gen.setstate, (2, (0,)*624+('a',), None))
        # Last element s/b between 0 and 624
        with self.assertRaises((ValueError, OverflowError)):
            self.gen.setstate((2, (1,)*624+(625,), None))
        with self.assertRaises((ValueError, OverflowError)):
            self.gen.setstate((2, (1,)*624+(-1,), None))
        # Failed calls to setstate() should not have changed the state.
        bits100 = self.gen.getrandbits(100)
        self.gen.setstate(start_state)
        self.assertEqual(self.gen.getrandbits(100), bits100)

        # Little trick to make "tuple(x % (2**32) for x in internalstate)"
        # raise ValueError. I cannot think of a simple way to achieve this, so
        # I am opting for using a generator as the middle argument of setstate
        # which attempts to cast a NaN to integer.
        state_values = self.gen.getstate()[1]
        state_values = list(state_values)
        state_values[-1] = float('nan')
        state = (int(x) for x in state_values)
        self.assertRaises(TypeError, self.gen.setstate, (2, state, None))

    def test_referenceImplementation(self):
        # Compare the python implementation with results from the original
        # code.  Create 2000 53-bit precision random floats.  Compare only
        # the last ten entries to show that the independent implementations
        # are tracking.  Here is the main() function needed to create the
        # list of expected random numbers:
        #    void main(void){
        #         int i;
        #         unsigned long init[4]={61731, 24903, 614, 42143}, length=4;
        #         init_by_array(init, length);
        #         for (i=0; i<2000; i++) {
        #           printf("%.15f ", genrand_res53());
        #           if (i%5==4) printf("\n");
        #         }
        #     }
        expected = [0.45839803073713259,
                    0.86057815201978782,
                    0.92848331726782152,
                    0.35932681119782461,
                    0.081823493762449573,
                    0.14332226470169329,
                    0.084297823823520024,
                    0.53814864671831453,
                    0.089215024911993401,
                    0.78486196105372907]

        self.gen.seed(61731 + (24903<<32) + (614<<64) + (42143<<96))
        actual = self.randomlist(2000)[-10:]
        for a, e in zip(actual, expected):
            self.assertAlmostEqual(a,e,places=14)

    def test_strong_reference_implementation(self):
        # Like test_referenceImplementation, but checks for exact bit-level
        # equality.  This should pass on any box where C double contains
        # at least 53 bits of precision (the underlying algorithm suffers
        # no rounding errors -- all results are exact).
        from math import ldexp

        expected = [0x0eab3258d2231f,
                    0x1b89db315277a5,
                    0x1db622a5518016,
                    0x0b7f9af0d575bf,
                    0x029e4c4db82240,
                    0x04961892f5d673,
                    0x02b291598e4589,
                    0x11388382c15694,
                    0x02dad977c9e1fe,
                    0x191d96d4d334c6]
        self.gen.seed(61731 + (24903<<32) + (614<<64) + (42143<<96))
        actual = self.randomlist(2000)[-10:]
        for a, e in zip(actual, expected):
            self.assertEqual(int(ldexp(a, 53)), e)

    def test_long_seed(self):
        # This is most interesting to run in debug mode, just to make sure
        # nothing blows up.  Under the covers, a dynamically resized array
        # is allocated, consuming space proportional to the number of bits
        # in the seed.  Unfortunately, that's a quadratic-time algorithm,
        # so don't make this horribly big.
        seed = (1 << (10000 * 8)) - 1  # about 10K bytes
        self.gen.seed(seed)

    def test_53_bits_per_float(self):
        # This should pass whenever a C double has 53 bit precision.
        span = 2 ** 53
        cum = 0
        for i in range(100):
            cum |= int(self.gen.random() * span)
        self.assertEqual(cum, span-1)

    def test_bigrand(self):
        # The randrange routine should build-up the required number of bits
        # in stages so that all bit positions are active.
        span = 2 ** 500
        cum = 0
        for i in range(100):
            r = self.gen.randrange(span)
            self.assertTrue(0 <= r < span)
            cum |= r
        self.assertEqual(cum, span-1)

    def test_bigrand_ranges(self):
        for i in [40,80, 160, 200, 211, 250, 375, 512, 550]:
            start = self.gen.randrange(2 ** (i-2))
            stop = self.gen.randrange(2 ** i)
            if stop <= start:
                continue
            self.assertTrue(start <= self.gen.randrange(start, stop) < stop)

    def test_rangelimits(self):
        for start, stop in [(-2,0), (-(2**60)-2,-(2**60)), (2**60,2**60+2)]:
            self.assertEqual(set(range(start,stop)),
                set([self.gen.randrange(start,stop) for i in range(100)]))

    def test_getrandbits(self):
        super().test_getrandbits()

        # Verify cross-platform repeatability
        self.gen.seed(1234567)
        self.assertEqual(self.gen.getrandbits(100),
                         97904845777343510404718956115)

    def test_randrange_uses_getrandbits(self):
        # Verify use of getrandbits by randrange
        # Use same seed as in the cross-platform repeatability test
        # in test_getrandbits above.
        self.gen.seed(1234567)
        # If randrange uses getrandbits, it should pick getrandbits(100)
        # when called with a 100-bits stop argument.
        self.assertEqual(self.gen.randrange(2**99),
                         97904845777343510404718956115)

    def test_randbelow_logic(self, _log=log, int=int):
        # check bitcount transition points:  2**i and 2**(i+1)-1
        # show that: k = int(1.001 + _log(n, 2))
        # is equal to or one greater than the number of bits in n
        for i in range(1, 1000):
            n = 1 << i # check an exact power of two
            numbits = i+1
            k = int(1.00001 + _log(n, 2))
            self.assertEqual(k, numbits)
            self.assertEqual(n, 2**(k-1))

            n += n - 1      # check 1 below the next power of two
            k = int(1.00001 + _log(n, 2))
            self.assertIn(k, [numbits, numbits+1])
            self.assertTrue(2**k > n > 2**(k-2))

            n -= n >> 15     # check a little farther below the next power of two
            k = int(1.00001 + _log(n, 2))
            self.assertEqual(k, numbits)        # note the stronger assertion
            self.assertTrue(2**k > n > 2**(k-1))   # note the stronger assertion

    def test_randbelow_without_getrandbits(self):
        # Random._randbelow() can only use random() when the built-in one
        # has been overridden but no new getrandbits() method was supplied.
        maxsize = 1<<random.BPF
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", UserWarning)
            # Population range too large (n >= maxsize)
            self.gen._randbelow_without_getrandbits(
                maxsize+1, maxsize=maxsize
            )
        self.gen._randbelow_without_getrandbits(5640, maxsize=maxsize)
        # issue 33203: test that _randbelow returns zero on
        # n == 0 also in its getrandbits-independent branch.
        x = self.gen._randbelow_without_getrandbits(0, maxsize=maxsize)
        self.assertEqual(x, 0)

        # This might be going too far to test a single line, but because of our
        # noble aim of achieving 100% test coverage we need to write a case in
        # which the following line in Random._randbelow() gets executed:
        #
        # rem = maxsize % n
        # limit = (maxsize - rem) / maxsize
        # r = random()
        # while r >= limit:
        #     r = random() # <== *This line* <==<
        #
        # Therefore, to guarantee that the while loop is executed at least
        # once, we need to mock random() so that it returns a number greater
        # than 'limit' the first time it gets called.

        n = 42
        epsilon = 0.01
        limit = (maxsize - (maxsize % n)) / maxsize
        with unittest.mock.patch.object(random.Random, 'random') as random_mock:
            random_mock.side_effect = [limit + epsilon, limit - epsilon]
            self.gen._randbelow_without_getrandbits(n, maxsize=maxsize)
            self.assertEqual(random_mock.call_count, 2)

    def test_randrange_bug_1590891(self):
        start = 1000000000000
        stop = -100000000000000000000
        step = -200
        x = self.gen.randrange(start, stop, step)
        self.assertTrue(stop < x <= start)
        self.assertEqual((x+stop)%step, 0)

    def test_choices_algorithms(self):
        # The various ways of specifying weights should produce the same results
        choices = self.gen.choices
        n = 104729

        self.gen.seed(8675309)
        a = self.gen.choices(range(n), k=10000)

        self.gen.seed(8675309)
        b = self.gen.choices(range(n), [1]*n, k=10000)
        self.assertEqual(a, b)

        self.gen.seed(8675309)
        c = self.gen.choices(range(n), cum_weights=range(1, n+1), k=10000)
        self.assertEqual(a, c)

        # American Roulette
        population = ['Red', 'Black', 'Green']
        weights = [18, 18, 2]
        cum_weights = [18, 36, 38]
        expanded_population = ['Red'] * 18 + ['Black'] * 18 + ['Green'] * 2

        self.gen.seed(9035768)
        a = self.gen.choices(expanded_population, k=10000)

        self.gen.seed(9035768)
        b = self.gen.choices(population, weights, k=10000)
        self.assertEqual(a, b)

        self.gen.seed(9035768)
        c = self.gen.choices(population, cum_weights=cum_weights, k=10000)
        self.assertEqual(a, c)

    def test_randbytes(self):
        super().test_randbytes()

        # Mersenne Twister randbytes() is deterministic
        # and does not depend on the endian and bitness.
        seed = 8675309
        expected = b'3\xa8\xf9f\xf4\xa4\xd06\x19\x8f\x9f\x82\x02oe\xf0'

        self.gen.seed(seed)
        self.assertEqual(self.gen.randbytes(16), expected)

        # randbytes(0) must not consume any entropy
        self.gen.seed(seed)
        self.assertEqual(self.gen.randbytes(0), b'')
        self.assertEqual(self.gen.randbytes(16), expected)

        # Four randbytes(4) calls give the same output than randbytes(16)
        self.gen.seed(seed)
        self.assertEqual(b''.join([self.gen.randbytes(4) for _ in range(4)]),
                         expected)

        # Each randbytes(1), randbytes(2) or randbytes(3) call consumes
        # 4 bytes of entropy
        self.gen.seed(seed)
        expected1 = expected[3::4]
        self.assertEqual(b''.join(self.gen.randbytes(1) for _ in range(4)),
                         expected1)

        self.gen.seed(seed)
        expected2 = b''.join(expected[i + 2: i + 4]
                             for i in range(0, len(expected), 4))
        self.assertEqual(b''.join(self.gen.randbytes(2) for _ in range(4)),
                         expected2)

        self.gen.seed(seed)
        expected3 = b''.join(expected[i + 1: i + 4]
                             for i in range(0, len(expected), 4))
        self.assertEqual(b''.join(self.gen.randbytes(3) for _ in range(4)),
                         expected3)

    def test_randbytes_getrandbits(self):
        # There is a simple relation between randbytes() and getrandbits()
        seed = 2849427419
        gen2 = random.Random()
        self.gen.seed(seed)
        gen2.seed(seed)
        for n in range(9):
            self.assertEqual(self.gen.randbytes(n),
                             gen2.getrandbits(n * 8).to_bytes(n, 'little'))


def gamma(z, sqrt2pi=(2.0*pi)**0.5):
    # Reflection to right half of complex plane
    if z < 0.5:
        return pi / sin(pi*z) / gamma(1.0-z)
    # Lanczos approximation with g=7
    az = z + (7.0 - 0.5)
    return az ** (z-0.5) / exp(az) * sqrt2pi * fsum([
        0.9999999999995183,
        676.5203681218835 / z,
        -1259.139216722289 / (z+1.0),
        771.3234287757674 / (z+2.0),
        -176.6150291498386 / (z+3.0),
        12.50734324009056 / (z+4.0),
        -0.1385710331296526 / (z+5.0),
        0.9934937113930748e-05 / (z+6.0),
        0.1659470187408462e-06 / (z+7.0),
    ])

class TestDistributions(unittest.TestCase):
    def test_zeroinputs(self):
        # Verify that distributions can handle a series of zero inputs'
        g = random.Random()
        x = [g.random() for i in range(50)] + [0.0]*5
        g.random = x[:].pop; g.uniform(1,10)
        g.random = x[:].pop; g.paretovariate(1.0)
        g.random = x[:].pop; g.expovariate(1.0)
        g.random = x[:].pop; g.weibullvariate(1.0, 1.0)
        g.random = x[:].pop; g.vonmisesvariate(1.0, 1.0)
        g.random = x[:].pop; g.normalvariate(0.0, 1.0)
        g.random = x[:].pop; g.gauss(0.0, 1.0)
        g.random = x[:].pop; g.lognormvariate(0.0, 1.0)
        g.random = x[:].pop; g.vonmisesvariate(0.0, 1.0)
        g.random = x[:].pop; g.gammavariate(0.01, 1.0)
        g.random = x[:].pop; g.gammavariate(1.0, 1.0)
        g.random = x[:].pop; g.gammavariate(200.0, 1.0)
        g.random = x[:].pop; g.betavariate(3.0, 3.0)
        g.random = x[:].pop; g.triangular(0.0, 1.0, 1.0/3.0)

    def test_avg_std(self):
        # Use integration to test distribution average and standard deviation.
        # Only works for distributions which do not consume variates in pairs
        g = random.Random()
        N = 5000
        x = [i/float(N) for i in range(1,N)]
        for variate, args, mu, sigmasqrd in [
                (g.uniform, (1.0,10.0), (10.0+1.0)/2, (10.0-1.0)**2/12),
                (g.triangular, (0.0, 1.0, 1.0/3.0), 4.0/9.0, 7.0/9.0/18.0),
                (g.expovariate, (1.5,), 1/1.5, 1/1.5**2),
                (g.vonmisesvariate, (1.23, 0), pi, pi**2/3),
                (g.paretovariate, (5.0,), 5.0/(5.0-1),
                                  5.0/((5.0-1)**2*(5.0-2))),
                (g.weibullvariate, (1.0, 3.0), gamma(1+1/3.0),
                                  gamma(1+2/3.0)-gamma(1+1/3.0)**2) ]:
            g.random = x[:].pop
            y = []
            for i in range(len(x)):
                try:
                    y.append(variate(*args))
                except IndexError:
                    pass
            s1 = s2 = 0
            for e in y:
                s1 += e
                s2 += (e - mu) ** 2
            N = len(y)
            self.assertAlmostEqual(s1/N, mu, places=2,
                                   msg='%s%r' % (variate.__name__, args))
            self.assertAlmostEqual(s2/(N-1), sigmasqrd, places=2,
                                   msg='%s%r' % (variate.__name__, args))

    def test_constant(self):
        g = random.Random()
        N = 100
        for variate, args, expected in [
                (g.uniform, (10.0, 10.0), 10.0),
                (g.triangular, (10.0, 10.0), 10.0),
                (g.triangular, (10.0, 10.0, 10.0), 10.0),
                (g.expovariate, (float('inf'),), 0.0),
                (g.vonmisesvariate, (3.0, float('inf')), 3.0),
                (g.gauss, (10.0, 0.0), 10.0),
                (g.lognormvariate, (0.0, 0.0), 1.0),
                (g.lognormvariate, (-float('inf'), 0.0), 0.0),
                (g.normalvariate, (10.0, 0.0), 10.0),
                (g.paretovariate, (float('inf'),), 1.0),
                (g.weibullvariate, (10.0, float('inf')), 10.0),
                (g.weibullvariate, (0.0, 10.0), 0.0),
            ]:
            for i in range(N):
                self.assertEqual(variate(*args), expected)

    def test_von_mises_range(self):
        # Issue 17149: von mises variates were not consistently in the
        # range [0, 2*PI].
        g = random.Random()
        N = 100
        for mu in 0.0, 0.1, 3.1, 6.2:
            for kappa in 0.0, 2.3, 500.0:
                for _ in range(N):
                    sample = g.vonmisesvariate(mu, kappa)
                    self.assertTrue(
                        0 <= sample <= random.TWOPI,
                        msg=("vonmisesvariate({}, {}) produced a result {} out"
                             " of range [0, 2*pi]").format(mu, kappa, sample))

    def test_von_mises_large_kappa(self):
        # Issue #17141: vonmisesvariate() was hang for large kappas
        random.vonmisesvariate(0, 1e15)
        random.vonmisesvariate(0, 1e100)

    def test_gammavariate_errors(self):
        # Both alpha and beta must be > 0.0
        self.assertRaises(ValueError, random.gammavariate, -1, 3)
        self.assertRaises(ValueError, random.gammavariate, 0, 2)
        self.assertRaises(ValueError, random.gammavariate, 2, 0)
        self.assertRaises(ValueError, random.gammavariate, 1, -3)

    # There are three different possibilities in the current implementation
    # of random.gammavariate(), depending on the value of 'alpha'. What we
    # are going to do here is to fix the values returned by random() to
    # generate test cases that provide 100% line coverage of the method.
    @unittest.mock.patch('random.Random.random')
    def test_gammavariate_alpha_greater_one(self, random_mock):

        # #1: alpha > 1.0.
        # We want the first random number to be outside the
        # [1e-7, .9999999] range, so that the continue statement executes
        # once. The values of u1 and u2 will be 0.5 and 0.3, respectively.
        random_mock.side_effect = [1e-8, 0.5, 0.3]
        returned_value = random.gammavariate(1.1, 2.3)
        self.assertAlmostEqual(returned_value, 2.53)

    @unittest.mock.patch('random.Random.random')
    def test_gammavariate_alpha_equal_one(self, random_mock):

        # #2.a: alpha == 1.
        # The execution body of the while loop executes once.
        # Then random.random() returns 0.45,
        # which causes while to stop looping and the algorithm to terminate.
        random_mock.side_effect = [0.45]
        returned_value = random.gammavariate(1.0, 3.14)
        self.assertAlmostEqual(returned_value, 1.877208182372648)

    @unittest.mock.patch('random.Random.random')
    def test_gammavariate_alpha_equal_one_equals_expovariate(self, random_mock):

        # #2.b: alpha == 1.
        # It must be equivalent of calling expovariate(1.0 / beta).
        beta = 3.14
        random_mock.side_effect = [1e-8, 1e-8]
        gammavariate_returned_value = random.gammavariate(1.0, beta)
        expovariate_returned_value = random.expovariate(1.0 / beta)
        self.assertAlmostEqual(gammavariate_returned_value, expovariate_returned_value)

    @unittest.mock.patch('random.Random.random')
    def test_gammavariate_alpha_between_zero_and_one(self, random_mock):

        # #3: 0 < alpha < 1.
        # This is the most complex region of code to cover,
        # as there are multiple if-else statements. Let's take a look at the
        # source code, and determine the values that we need accordingly:
        #
        # while 1:
        #     u = random()
        #     b = (_e + alpha)/_e
        #     p = b*u
        #     if p <= 1.0: # <=== (A)
        #         x = p ** (1.0/alpha)
        #     else: # <=== (B)
        #         x = -_log((b-p)/alpha)
        #     u1 = random()
        #     if p > 1.0: # <=== (C)
        #         if u1 <= x ** (alpha - 1.0): # <=== (D)
        #             break
        #     elif u1 <= _exp(-x): # <=== (E)
        #         break
        # return x * beta
        #
        # First, we want (A) to be True. For that we need that:
        # b*random() <= 1.0
        # r1 = random() <= 1.0 / b
        #
        # We now get to the second if-else branch, and here, since p <= 1.0,
        # (C) is False and we take the elif branch, (E). For it to be True,
        # so that the break is executed, we need that:
        # r2 = random() <= _exp(-x)
        # r2 <= _exp(-(p ** (1.0/alpha)))
        # r2 <= _exp(-((b*r1) ** (1.0/alpha)))

        _e = random._e
        _exp = random._exp
        _log = random._log
        alpha = 0.35
        beta = 1.45
        b = (_e + alpha)/_e
        epsilon = 0.01

        r1 = 0.8859296441566 # 1.0 / b
        r2 = 0.3678794411714 # _exp(-((b*r1) ** (1.0/alpha)))

        # These four "random" values result in the following trace:
        # (A) True, (E) False --> [next iteration of while]
        # (A) True, (E) True --> [while loop breaks]
        random_mock.side_effect = [r1, r2 + epsilon, r1, r2]
        returned_value = random.gammavariate(alpha, beta)
        self.assertAlmostEqual(returned_value, 1.4499999999997544)

        # Let's now make (A) be False. If this is the case, when we get to the
        # second if-else 'p' is greater than 1, so (C) evaluates to True. We
        # now encounter a second if statement, (D), which in order to execute
        # must satisfy the following condition:
        # r2 <= x ** (alpha - 1.0)
        # r2 <= (-_log((b-p)/alpha)) ** (alpha - 1.0)
        # r2 <= (-_log((b-(b*r1))/alpha)) ** (alpha - 1.0)
        r1 = 0.8959296441566 # (1.0 / b) + epsilon -- so that (A) is False
        r2 = 0.9445400408898141

        # And these four values result in the following trace:
        # (B) and (C) True, (D) False --> [next iteration of while]
        # (B) and (C) True, (D) True [while loop breaks]
        random_mock.side_effect = [r1, r2 + epsilon, r1, r2]
        returned_value = random.gammavariate(alpha, beta)
        self.assertAlmostEqual(returned_value, 1.5830349561760781)

    @unittest.mock.patch('random.Random.gammavariate')
    def test_betavariate_return_zero(self, gammavariate_mock):
        # betavariate() returns zero when the Gamma distribution
        # that it uses internally returns this same value.
        gammavariate_mock.return_value = 0.0
        self.assertEqual(0.0, random.betavariate(2.71828, 3.14159))


class TestRandomSubclassing(unittest.TestCase):
    def test_random_subclass_with_kwargs(self):
        # SF bug #1486663 -- this used to erroneously raise a TypeError
        class Subclass(random.Random):
            def __init__(self, newarg=None):
                random.Random.__init__(self)
        Subclass(newarg=1)

    def test_subclasses_overriding_methods(self):
        # Subclasses with an overridden random, but only the original
        # getrandbits method should not rely on getrandbits in for randrange,
        # but should use a getrandbits-independent implementation instead.

        # subclass providing its own random **and** getrandbits methods
        # like random.SystemRandom does => keep relying on getrandbits for
        # randrange
        class SubClass1(random.Random):
            def random(self):
                called.add('SubClass1.random')
                return random.Random.random(self)

            def getrandbits(self, n):
                called.add('SubClass1.getrandbits')
                return random.Random.getrandbits(self, n)
        called = set()
        SubClass1().randrange(42)
        self.assertEqual(called, {'SubClass1.getrandbits'})

        # subclass providing only random => can only use random for randrange
        class SubClass2(random.Random):
            def random(self):
                called.add('SubClass2.random')
                return random.Random.random(self)
        called = set()
        SubClass2().randrange(42)
        self.assertEqual(called, {'SubClass2.random'})

        # subclass defining getrandbits to complement its inherited random
        # => can now rely on getrandbits for randrange again
        class SubClass3(SubClass2):
            def getrandbits(self, n):
                called.add('SubClass3.getrandbits')
                return random.Random.getrandbits(self, n)
        called = set()
        SubClass3().randrange(42)
        self.assertEqual(called, {'SubClass3.getrandbits'})

        # subclass providing only random and inherited getrandbits
        # => random takes precedence
        class SubClass4(SubClass3):
            def random(self):
                called.add('SubClass4.random')
                return random.Random.random(self)
        called = set()
        SubClass4().randrange(42)
        self.assertEqual(called, {'SubClass4.random'})

        # Following subclasses don't define random or getrandbits directly,
        # but inherit them from classes which are not subclasses of Random
        class Mixin1:
            def random(self):
                called.add('Mixin1.random')
                return random.Random.random(self)
        class Mixin2:
            def getrandbits(self, n):
                called.add('Mixin2.getrandbits')
                return random.Random.getrandbits(self, n)

        class SubClass5(Mixin1, random.Random):
            pass
        called = set()
        SubClass5().randrange(42)
        self.assertEqual(called, {'Mixin1.random'})

        class SubClass6(Mixin2, random.Random):
            pass
        called = set()
        SubClass6().randrange(42)
        self.assertEqual(called, {'Mixin2.getrandbits'})

        class SubClass7(Mixin1, Mixin2, random.Random):
            pass
        called = set()
        SubClass7().randrange(42)
        self.assertEqual(called, {'Mixin1.random'})

        class SubClass8(Mixin2, Mixin1, random.Random):
            pass
        called = set()
        SubClass8().randrange(42)
        self.assertEqual(called, {'Mixin2.getrandbits'})


class TestModule(unittest.TestCase):
    def testMagicConstants(self):
        self.assertAlmostEqual(random.NV_MAGICCONST, 1.71552776992141)
        self.assertAlmostEqual(random.TWOPI, 6.28318530718)
        self.assertAlmostEqual(random.LOG4, 1.38629436111989)
        self.assertAlmostEqual(random.SG_MAGICCONST, 2.50407739677627)

    def test__all__(self):
        # tests validity but not completeness of the __all__ list
        self.assertTrue(set(random.__all__) <= set(dir(random)))

    @unittest.skipUnless(hasattr(os, "fork"), "fork() required")
    def test_after_fork(self):
        # Test the global Random instance gets reseeded in child
        r, w = os.pipe()
        pid = os.fork()
        if pid == 0:
            # child process
            try:
                val = random.getrandbits(128)
                with open(w, "w") as f:
                    f.write(str(val))
            finally:
                os._exit(0)
        else:
            # parent process
            os.close(w)
            val = random.getrandbits(128)
            with open(r, "r") as f:
                child_val = eval(f.read())
            self.assertNotEqual(val, child_val)

            support.wait_process(pid, exitcode=0)


if __name__ == "__main__":
    unittest.main()
