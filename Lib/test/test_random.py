#!/usr/bin/env python

import unittest
import random
import time
from test import test_support

class TestBasicOps(unittest.TestCase):
    # Superclass with tests common to all generators.
    # Subclasses must arrange for self.gen to retrieve the Random instance
    # to be tested.

    def randomlist(self, n):
        """Helper function to make a list of random numbers"""
        return [self.gen.random() for i in xrange(n)]

    def test_autoseed(self):
        self.gen.seed()
        state1 = self.gen.getstate()
        time.sleep(1)
        self.gen.seed()      # diffent seeds at different times
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
        for arg in [None, 0, 0L, 1, 1L, -1, -1L, 10**20, -(10**20),
                    3.14, 1+2j, 'a', tuple('abc')]:
            self.gen.seed(arg)
        for arg in [range(3), dict(one=1)]:
            self.assertRaises(TypeError, self.gen.seed, arg)

    def test_jumpahead(self):
        self.gen.seed()
        state1 = self.gen.getstate()
        self.gen.jumpahead(100)
        state2 = self.gen.getstate()    # s/b distinct from state1
        self.assertNotEqual(state1, state2)
        self.gen.jumpahead(100)
        state3 = self.gen.getstate()    # s/b distinct from state2
        self.assertNotEqual(state2, state3)

        self.assertRaises(TypeError, self.gen.jumpahead)  # needs an arg
        self.assertRaises(TypeError, self.gen.jumpahead, "ick")  # wrong type
        self.assertRaises(TypeError, self.gen.jumpahead, 2.3)  # wrong type
        self.assertRaises(TypeError, self.gen.jumpahead, 2, 3)  # too many

    def test_sample(self):
        # For the entire allowable range of 0 <= k <= N, validate that
        # the sample is of the correct length and contains only unique items
        N = 100
        population = xrange(N)
        for k in xrange(N+1):
            s = self.gen.sample(population, k)
            self.assertEqual(len(s), k)
            uniq = dict.fromkeys(s)
            self.assertEqual(len(uniq), k)
            self.failIf(None in uniq)

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


class WichmannHill_TestBasicOps(TestBasicOps):
    gen = random.WichmannHill()

    def test_strong_jumpahead(self):
        # tests that jumpahead(n) semantics correspond to n calls to random()
        N = 1000
        s = self.gen.getstate()
        self.gen.jumpahead(N)
        r1 = self.gen.random()
        # now do it the slow way
        self.gen.setstate(s)
        for i in xrange(N):
            self.gen.random()
        r2 = self.gen.random()
        self.assertEqual(r1, r2)

    def test_gauss_with_whseed(self):
        # Ensure that the seed() method initializes all the hidden state.  In
        # particular, through 2.2.1 it failed to reset a piece of state used
        # by (and only by) the .gauss() method.

        for seed in 1, 12, 123, 1234, 12345, 123456, 654321:
            self.gen.whseed(seed)
            x1 = self.gen.random()
            y1 = self.gen.gauss(0, 1)

            self.gen.whseed(seed)
            x2 = self.gen.random()
            y2 = self.gen.gauss(0, 1)

            self.assertEqual(x1, x2)
            self.assertEqual(y1, y2)

class MersenneTwister_TestBasicOps(TestBasicOps):
    gen = random.Random()

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

        self.gen.seed(61731L + (24903L<<32) + (614L<<64) + (42143L<<96))
        actual = self.randomlist(2000)[-10:]
        for a, e in zip(actual, expected):
            self.assertAlmostEqual(a,e,places=14)

    def test_strong_reference_implementation(self):
        # Like test_referenceImplementation, but checks for exact bit-level
        # equality.  This should pass on any box where C double contains
        # at least 53 bits of precision (the underlying algorithm suffers
        # no rounding errors -- all results are exact).
        from math import ldexp

        expected = [0x0eab3258d2231fL,
                    0x1b89db315277a5L,
                    0x1db622a5518016L,
                    0x0b7f9af0d575bfL,
                    0x029e4c4db82240L,
                    0x04961892f5d673L,
                    0x02b291598e4589L,
                    0x11388382c15694L,
                    0x02dad977c9e1feL,
                    0x191d96d4d334c6L]

        self.gen.seed(61731L + (24903L<<32) + (614L<<64) + (42143L<<96))
        actual = self.randomlist(2000)[-10:]
        for a, e in zip(actual, expected):
            self.assertEqual(long(ldexp(a, 53)), e)

    def test_long_seed(self):
        # This is most interesting to run in debug mode, just to make sure
        # nothing blows up.  Under the covers, a dynamically resized array
        # is allocated, consuming space proportional to the number of bits
        # in the seed.  Unfortunately, that's a quadratic-time algorithm,
        # so don't make this horribly big.
        seed = (1L << (10000 * 8)) - 1  # about 10K bytes
        self.gen.seed(seed)

class TestModule(unittest.TestCase):
    def testMagicConstants(self):
        self.assertAlmostEqual(random.NV_MAGICCONST, 1.71552776992141)
        self.assertAlmostEqual(random.TWOPI, 6.28318530718)
        self.assertAlmostEqual(random.LOG4, 1.38629436111989)
        self.assertAlmostEqual(random.SG_MAGICCONST, 2.50407739677627)

    def test__all__(self):
        # tests validity but not completeness of the __all__ list
        defined = dict.fromkeys(dir(random))
        for entry in random.__all__:
            self.failUnless(entry in defined)

def test_main():
    suite = unittest.TestSuite()
    for testclass in (WichmannHill_TestBasicOps,
                      MersenneTwister_TestBasicOps,
                      TestModule):
        suite.addTest(unittest.makeSuite(testclass))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
