import unittest
import math
import sys
import copy
import random
from test.support.import_helper import import_fresh_module
from . import (C, P, requires_cdecimal, orig_sys_decimal,
               setUpModule, tearDownModule)


# fractions module must import the correct decimal module.
sys.modules['decimal'] = C
cfractions = import_fresh_module('fractions', fresh=['fractions'])
sys.modules['decimal'] = P
pfractions = import_fresh_module('fractions', fresh=['fractions'])
sys.modules['decimal'] = orig_sys_decimal

fractions = {C: cfractions, P: pfractions}

class UsabilityTest:
    '''Unit tests for Usability cases of Decimal.'''

    def test_comparison_operators(self):

        Decimal = self.decimal.Decimal

        da = Decimal('23.42')
        db = Decimal('23.42')
        dc = Decimal('45')

        #two Decimals
        self.assertGreater(dc, da)
        self.assertGreaterEqual(dc, da)
        self.assertLess(da, dc)
        self.assertLessEqual(da, dc)
        self.assertEqual(da, db)
        self.assertNotEqual(da, dc)
        self.assertLessEqual(da, db)
        self.assertGreaterEqual(da, db)

        #a Decimal and an int
        self.assertGreater(dc, 23)
        self.assertLess(23, dc)
        self.assertEqual(dc, 45)

        #a Decimal and uncomparable
        self.assertNotEqual(da, 'ugly')
        self.assertNotEqual(da, 32.7)
        self.assertNotEqual(da, object())
        self.assertNotEqual(da, object)

        # sortable
        a = list(map(Decimal, range(100)))
        b =  a[:]
        random.shuffle(a)
        a.sort()
        self.assertEqual(a, b)

    def test_decimal_float_comparison(self):
        Decimal = self.decimal.Decimal

        da = Decimal('0.25')
        db = Decimal('3.0')
        self.assertLess(da, 3.0)
        self.assertLessEqual(da, 3.0)
        self.assertGreater(db, 0.25)
        self.assertGreaterEqual(db, 0.25)
        self.assertNotEqual(da, 1.5)
        self.assertEqual(da, 0.25)
        self.assertGreater(3.0, da)
        self.assertGreaterEqual(3.0, da)
        self.assertLess(0.25, db)
        self.assertLessEqual(0.25, db)
        self.assertNotEqual(0.25, db)
        self.assertEqual(3.0, db)
        self.assertNotEqual(0.1, Decimal('0.1'))

    def test_decimal_complex_comparison(self):
        Decimal = self.decimal.Decimal

        da = Decimal('0.25')
        db = Decimal('3.0')
        self.assertNotEqual(da, (1.5+0j))
        self.assertNotEqual((1.5+0j), da)
        self.assertEqual(da, (0.25+0j))
        self.assertEqual((0.25+0j), da)
        self.assertEqual((3.0+0j), db)
        self.assertEqual(db, (3.0+0j))

        self.assertNotEqual(db, (3.0+1j))
        self.assertNotEqual((3.0+1j), db)

        self.assertIs(db.__lt__(3.0+0j), NotImplemented)
        self.assertIs(db.__le__(3.0+0j), NotImplemented)
        self.assertIs(db.__gt__(3.0+0j), NotImplemented)
        self.assertIs(db.__le__(3.0+0j), NotImplemented)

    def test_decimal_fraction_comparison(self):
        D = self.decimal.Decimal
        F = fractions[self.decimal].Fraction
        Context = self.decimal.Context
        localcontext = self.decimal.localcontext
        InvalidOperation = self.decimal.InvalidOperation


        emax = C.MAX_EMAX if C else 999999999
        emin = C.MIN_EMIN if C else -999999999
        etiny = C.MIN_ETINY if C else -1999999997
        c = Context(Emax=emax, Emin=emin)

        with localcontext(c):
            c.prec = emax
            self.assertLess(D(0), F(1,9999999999999999999999999999999999999))
            self.assertLess(F(-1,9999999999999999999999999999999999999), D(0))
            self.assertLess(F(0,1), D("1e" + str(etiny)))
            self.assertLess(D("-1e" + str(etiny)), F(0,1))
            self.assertLess(F(0,9999999999999999999999999), D("1e" + str(etiny)))
            self.assertLess(D("-1e" + str(etiny)), F(0,9999999999999999999999999))

            self.assertEqual(D("0.1"), F(1,10))
            self.assertEqual(F(1,10), D("0.1"))

            c.prec = 300
            self.assertNotEqual(D(1)/3, F(1,3))
            self.assertNotEqual(F(1,3), D(1)/3)

            self.assertLessEqual(F(120984237, 9999999999), D("9e" + str(emax)))
            self.assertGreaterEqual(D("9e" + str(emax)), F(120984237, 9999999999))

            self.assertGreater(D('inf'), F(99999999999,123))
            self.assertGreater(D('inf'), F(-99999999999,123))
            self.assertLess(D('-inf'), F(99999999999,123))
            self.assertLess(D('-inf'), F(-99999999999,123))

            self.assertRaises(InvalidOperation, D('nan').__gt__, F(-9,123))
            self.assertIs(NotImplemented, F(-9,123).__lt__(D('nan')))
            self.assertNotEqual(D('nan'), F(-9,123))
            self.assertNotEqual(F(-9,123), D('nan'))

    def test_copy_and_deepcopy_methods(self):
        Decimal = self.decimal.Decimal

        d = Decimal('43.24')
        c = copy.copy(d)
        self.assertEqual(id(c), id(d))
        dc = copy.deepcopy(d)
        self.assertEqual(id(dc), id(d))

    def test_hash_method(self):

        Decimal = self.decimal.Decimal
        localcontext = self.decimal.localcontext

        def hashit(d):
            a = hash(d)
            b = d.__hash__()
            self.assertEqual(a, b)
            return a

        #just that it's hashable
        hashit(Decimal(23))
        hashit(Decimal('Infinity'))
        hashit(Decimal('-Infinity'))
        hashit(Decimal('nan123'))
        hashit(Decimal('-NaN'))

        test_values = [Decimal(sign*(2**m + n))
                       for m in [0, 14, 15, 16, 17, 30, 31,
                                 32, 33, 61, 62, 63, 64, 65, 66]
                       for n in range(-10, 10)
                       for sign in [-1, 1]]
        test_values.extend([
                Decimal("-1"), # ==> -2
                Decimal("-0"), # zeros
                Decimal("0.00"),
                Decimal("-0.000"),
                Decimal("0E10"),
                Decimal("-0E12"),
                Decimal("10.0"), # negative exponent
                Decimal("-23.00000"),
                Decimal("1230E100"), # positive exponent
                Decimal("-4.5678E50"),
                # a value for which hash(n) != hash(n % (2**64-1))
                # in Python pre-2.6
                Decimal(2**64 + 2**32 - 1),
                # selection of values which fail with the old (before
                # version 2.6) long.__hash__
                Decimal("1.634E100"),
                Decimal("90.697E100"),
                Decimal("188.83E100"),
                Decimal("1652.9E100"),
                Decimal("56531E100"),
                ])

        # check that hash(d) == hash(int(d)) for integral values
        for value in test_values:
            self.assertEqual(hashit(value), hash(int(value)))

        # check that the hashes of a Decimal float match when they
        # represent exactly the same values
        test_strings = ['inf', '-Inf', '0.0', '-.0e1',
                        '34.0', '2.5', '112390.625', '-0.515625']
        for s in test_strings:
            f = float(s)
            d = Decimal(s)
            self.assertEqual(hashit(d), hash(f))

        with localcontext() as c:
            # check that the value of the hash doesn't depend on the
            # current context (issue #1757)
            x = Decimal("123456789.1")

            c.prec = 6
            h1 = hashit(x)
            c.prec = 10
            h2 = hashit(x)
            c.prec = 16
            h3 = hashit(x)

            self.assertEqual(h1, h2)
            self.assertEqual(h1, h3)

            c.prec = 10000
            x = 1100 ** 1248
            self.assertEqual(hashit(Decimal(x)), hashit(x))

    def test_hash_method_nan(self):
        Decimal = self.decimal.Decimal
        self.assertRaises(TypeError, hash, Decimal('sNaN'))
        value = Decimal('NaN')
        self.assertEqual(hash(value), object.__hash__(value))
        class H:
            def __hash__(self):
                return 42
        class D(Decimal, H):
            pass
        value = D('NaN')
        self.assertEqual(hash(value), object.__hash__(value))

    def test_min_and_max_methods(self):
        Decimal = self.decimal.Decimal

        d1 = Decimal('15.32')
        d2 = Decimal('28.5')
        l1 = 15
        l2 = 28

        #between Decimals
        self.assertIs(min(d1,d2), d1)
        self.assertIs(min(d2,d1), d1)
        self.assertIs(max(d1,d2), d2)
        self.assertIs(max(d2,d1), d2)

        #between Decimal and int
        self.assertIs(min(d1,l2), d1)
        self.assertIs(min(l2,d1), d1)
        self.assertIs(max(l1,d2), d2)
        self.assertIs(max(d2,l1), d2)

    def test_as_nonzero(self):
        Decimal = self.decimal.Decimal

        #as false
        self.assertFalse(Decimal(0))
        #as true
        self.assertTrue(Decimal('0.372'))

    def test_tostring_methods(self):
        #Test str and repr methods.
        Decimal = self.decimal.Decimal

        d = Decimal('15.32')
        self.assertEqual(str(d), '15.32')               # str
        self.assertEqual(repr(d), "Decimal('15.32')")   # repr

    def test_tonum_methods(self):
        #Test float and int methods.
        Decimal = self.decimal.Decimal

        d1 = Decimal('66')
        d2 = Decimal('15.32')

        #int
        self.assertEqual(int(d1), 66)
        self.assertEqual(int(d2), 15)

        #float
        self.assertEqual(float(d1), 66)
        self.assertEqual(float(d2), 15.32)

        #floor
        test_pairs = [
            ('123.00', 123),
            ('3.2', 3),
            ('3.54', 3),
            ('3.899', 3),
            ('-2.3', -3),
            ('-11.0', -11),
            ('0.0', 0),
            ('-0E3', 0),
            ('89891211712379812736.1', 89891211712379812736),
            ]
        for d, i in test_pairs:
            self.assertEqual(math.floor(Decimal(d)), i)
        self.assertRaises(ValueError, math.floor, Decimal('-NaN'))
        self.assertRaises(ValueError, math.floor, Decimal('sNaN'))
        self.assertRaises(ValueError, math.floor, Decimal('NaN123'))
        self.assertRaises(OverflowError, math.floor, Decimal('Inf'))
        self.assertRaises(OverflowError, math.floor, Decimal('-Inf'))

        #ceiling
        test_pairs = [
            ('123.00', 123),
            ('3.2', 4),
            ('3.54', 4),
            ('3.899', 4),
            ('-2.3', -2),
            ('-11.0', -11),
            ('0.0', 0),
            ('-0E3', 0),
            ('89891211712379812736.1', 89891211712379812737),
            ]
        for d, i in test_pairs:
            self.assertEqual(math.ceil(Decimal(d)), i)
        self.assertRaises(ValueError, math.ceil, Decimal('-NaN'))
        self.assertRaises(ValueError, math.ceil, Decimal('sNaN'))
        self.assertRaises(ValueError, math.ceil, Decimal('NaN123'))
        self.assertRaises(OverflowError, math.ceil, Decimal('Inf'))
        self.assertRaises(OverflowError, math.ceil, Decimal('-Inf'))

        #round, single argument
        test_pairs = [
            ('123.00', 123),
            ('3.2', 3),
            ('3.54', 4),
            ('3.899', 4),
            ('-2.3', -2),
            ('-11.0', -11),
            ('0.0', 0),
            ('-0E3', 0),
            ('-3.5', -4),
            ('-2.5', -2),
            ('-1.5', -2),
            ('-0.5', 0),
            ('0.5', 0),
            ('1.5', 2),
            ('2.5', 2),
            ('3.5', 4),
            ]
        for d, i in test_pairs:
            self.assertEqual(round(Decimal(d)), i)
        self.assertRaises(ValueError, round, Decimal('-NaN'))
        self.assertRaises(ValueError, round, Decimal('sNaN'))
        self.assertRaises(ValueError, round, Decimal('NaN123'))
        self.assertRaises(OverflowError, round, Decimal('Inf'))
        self.assertRaises(OverflowError, round, Decimal('-Inf'))

        #round, two arguments;  this is essentially equivalent
        #to quantize, which is already extensively tested
        test_triples = [
            ('123.456', -4, '0E+4'),
            ('-123.456', -4, '-0E+4'),
            ('123.456', -3, '0E+3'),
            ('-123.456', -3, '-0E+3'),
            ('123.456', -2, '1E+2'),
            ('123.456', -1, '1.2E+2'),
            ('123.456', 0, '123'),
            ('123.456', 1, '123.5'),
            ('123.456', 2, '123.46'),
            ('123.456', 3, '123.456'),
            ('123.456', 4, '123.4560'),
            ('123.455', 2, '123.46'),
            ('123.445', 2, '123.44'),
            ('Inf', 4, 'NaN'),
            ('-Inf', -23, 'NaN'),
            ('sNaN314', 3, 'NaN314'),
            ]
        for d, n, r in test_triples:
            self.assertEqual(str(round(Decimal(d), n)), r)

    def test_nan_to_float(self):
        # Test conversions of decimal NANs to float.
        # See http://bugs.python.org/issue15544
        Decimal = self.decimal.Decimal
        for s in ('nan', 'nan1234', '-nan', '-nan2468'):
            f = float(Decimal(s))
            self.assertTrue(math.isnan(f))
            sign = math.copysign(1.0, f)
            self.assertEqual(sign, -1.0 if s.startswith('-') else 1.0)

    def test_snan_to_float(self):
        Decimal = self.decimal.Decimal
        for s in ('snan', '-snan', 'snan1357', '-snan1234'):
            d = Decimal(s)
            self.assertRaises(ValueError, float, d)

    def test_eval_round_trip(self):
        Decimal = self.decimal.Decimal

        #with zero
        d = Decimal( (0, (0,), 0) )
        self.assertEqual(d, eval(repr(d)))

        #int
        d = Decimal( (1, (4, 5), 0) )
        self.assertEqual(d, eval(repr(d)))

        #float
        d = Decimal( (0, (4, 5, 3, 4), -2) )
        self.assertEqual(d, eval(repr(d)))

        #weird
        d = Decimal( (1, (4, 3, 4, 9, 1, 3, 5, 3, 4), -25) )
        self.assertEqual(d, eval(repr(d)))

    def test_as_tuple(self):
        Decimal = self.decimal.Decimal

        #with zero
        d = Decimal(0)
        self.assertEqual(d.as_tuple(), (0, (0,), 0) )

        #int
        d = Decimal(-45)
        self.assertEqual(d.as_tuple(), (1, (4, 5), 0) )

        #complicated string
        d = Decimal("-4.34913534E-17")
        self.assertEqual(d.as_tuple(), (1, (4, 3, 4, 9, 1, 3, 5, 3, 4), -25) )

        # The '0' coefficient is implementation specific to decimal.py.
        # It has no meaning in the C-version and is ignored there.
        d = Decimal("Infinity")
        self.assertEqual(d.as_tuple(), (0, (0,), 'F') )

        #leading zeros in coefficient should be stripped
        d = Decimal( (0, (0, 0, 4, 0, 5, 3, 4), -2) )
        self.assertEqual(d.as_tuple(), (0, (4, 0, 5, 3, 4), -2) )
        d = Decimal( (1, (0, 0, 0), 37) )
        self.assertEqual(d.as_tuple(), (1, (0,), 37))
        d = Decimal( (1, (), 37) )
        self.assertEqual(d.as_tuple(), (1, (0,), 37))

        #leading zeros in NaN diagnostic info should be stripped
        d = Decimal( (0, (0, 0, 4, 0, 5, 3, 4), 'n') )
        self.assertEqual(d.as_tuple(), (0, (4, 0, 5, 3, 4), 'n') )
        d = Decimal( (1, (0, 0, 0), 'N') )
        self.assertEqual(d.as_tuple(), (1, (), 'N') )
        d = Decimal( (1, (), 'n') )
        self.assertEqual(d.as_tuple(), (1, (), 'n') )

        # For infinities, decimal.py has always silently accepted any
        # coefficient tuple.
        d = Decimal( (0, (0,), 'F') )
        self.assertEqual(d.as_tuple(), (0, (0,), 'F'))
        d = Decimal( (0, (4, 5, 3, 4), 'F') )
        self.assertEqual(d.as_tuple(), (0, (0,), 'F'))
        d = Decimal( (1, (0, 2, 7, 1), 'F') )
        self.assertEqual(d.as_tuple(), (1, (0,), 'F'))

    def test_as_integer_ratio(self):
        Decimal = self.decimal.Decimal

        # exceptional cases
        self.assertRaises(OverflowError,
                          Decimal.as_integer_ratio, Decimal('inf'))
        self.assertRaises(OverflowError,
                          Decimal.as_integer_ratio, Decimal('-inf'))
        self.assertRaises(ValueError,
                          Decimal.as_integer_ratio, Decimal('-nan'))
        self.assertRaises(ValueError,
                          Decimal.as_integer_ratio, Decimal('snan123'))

        for exp in range(-4, 2):
            for coeff in range(1000):
                for sign in '+', '-':
                    d = Decimal('%s%dE%d' % (sign, coeff, exp))
                    pq = d.as_integer_ratio()
                    p, q = pq

                    # check return type
                    self.assertIsInstance(pq, tuple)
                    self.assertIsInstance(p, int)
                    self.assertIsInstance(q, int)

                    # check normalization:  q should be positive;
                    # p should be relatively prime to q.
                    self.assertGreater(q, 0)
                    self.assertEqual(math.gcd(p, q), 1)

                    # check that p/q actually gives the correct value
                    self.assertEqual(Decimal(p) / Decimal(q), d)

    def test_subclassing(self):
        # Different behaviours when subclassing Decimal
        Decimal = self.decimal.Decimal

        class MyDecimal(Decimal):
            y = None

        d1 = MyDecimal(1)
        d2 = MyDecimal(2)
        d = d1 + d2
        self.assertIs(type(d), Decimal)

        d = d1.max(d2)
        self.assertIs(type(d), Decimal)

        d = copy.copy(d1)
        self.assertIs(type(d), MyDecimal)
        self.assertEqual(d, d1)

        d = copy.deepcopy(d1)
        self.assertIs(type(d), MyDecimal)
        self.assertEqual(d, d1)

        # Decimal(Decimal)
        d = Decimal('1.0')
        x = Decimal(d)
        self.assertIs(type(x), Decimal)
        self.assertEqual(x, d)

        # MyDecimal(Decimal)
        m = MyDecimal(d)
        self.assertIs(type(m), MyDecimal)
        self.assertEqual(m, d)
        self.assertIs(m.y, None)

        # Decimal(MyDecimal)
        x = Decimal(m)
        self.assertIs(type(x), Decimal)
        self.assertEqual(x, d)

        # MyDecimal(MyDecimal)
        m.y = 9
        x = MyDecimal(m)
        self.assertIs(type(x), MyDecimal)
        self.assertEqual(x, d)
        self.assertIs(x.y, None)

    def test_implicit_context(self):
        Decimal = self.decimal.Decimal
        getcontext = self.decimal.getcontext

        # Check results when context given implicitly.  (Issue 2478)
        c = getcontext()
        self.assertEqual(str(Decimal(0).sqrt()),
                         str(c.sqrt(Decimal(0))))

    def test_none_args(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context
        localcontext = self.decimal.localcontext
        InvalidOperation = self.decimal.InvalidOperation
        DivisionByZero = self.decimal.DivisionByZero
        Overflow = self.decimal.Overflow
        Underflow = self.decimal.Underflow
        Subnormal = self.decimal.Subnormal
        Inexact = self.decimal.Inexact
        Rounded = self.decimal.Rounded
        Clamped = self.decimal.Clamped

        with localcontext(Context()) as c:
            c.prec = 7
            c.Emax = 999
            c.Emin = -999

            x = Decimal("111")
            y = Decimal("1e9999")
            z = Decimal("1e-9999")

            ##### Unary functions
            c.clear_flags()
            self.assertEqual(str(x.exp(context=None)), '1.609487E+48')
            self.assertTrue(c.flags[Inexact])
            self.assertTrue(c.flags[Rounded])
            c.clear_flags()
            self.assertRaises(Overflow, y.exp, context=None)
            self.assertTrue(c.flags[Overflow])

            self.assertIs(z.is_normal(context=None), False)
            self.assertIs(z.is_subnormal(context=None), True)

            c.clear_flags()
            self.assertEqual(str(x.ln(context=None)), '4.709530')
            self.assertTrue(c.flags[Inexact])
            self.assertTrue(c.flags[Rounded])
            c.clear_flags()
            self.assertRaises(InvalidOperation, Decimal(-1).ln, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            self.assertEqual(str(x.log10(context=None)), '2.045323')
            self.assertTrue(c.flags[Inexact])
            self.assertTrue(c.flags[Rounded])
            c.clear_flags()
            self.assertRaises(InvalidOperation, Decimal(-1).log10, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            self.assertEqual(str(x.logb(context=None)), '2')
            self.assertRaises(DivisionByZero, Decimal(0).logb, context=None)
            self.assertTrue(c.flags[DivisionByZero])

            c.clear_flags()
            self.assertEqual(str(x.logical_invert(context=None)), '1111000')
            self.assertRaises(InvalidOperation, y.logical_invert, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            self.assertEqual(str(y.next_minus(context=None)), '9.999999E+999')
            self.assertRaises(InvalidOperation, Decimal('sNaN').next_minus, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            self.assertEqual(str(y.next_plus(context=None)), 'Infinity')
            self.assertRaises(InvalidOperation, Decimal('sNaN').next_plus, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            self.assertEqual(str(z.normalize(context=None)), '0')
            self.assertRaises(Overflow, y.normalize, context=None)
            self.assertTrue(c.flags[Overflow])

            self.assertEqual(str(z.number_class(context=None)), '+Subnormal')

            c.clear_flags()
            self.assertEqual(str(z.sqrt(context=None)), '0E-1005')
            self.assertTrue(c.flags[Clamped])
            self.assertTrue(c.flags[Inexact])
            self.assertTrue(c.flags[Rounded])
            self.assertTrue(c.flags[Subnormal])
            self.assertTrue(c.flags[Underflow])
            c.clear_flags()
            self.assertRaises(Overflow, y.sqrt, context=None)
            self.assertTrue(c.flags[Overflow])

            c.capitals = 0
            self.assertEqual(str(z.to_eng_string(context=None)), '1e-9999')
            c.capitals = 1


            ##### Binary functions
            c.clear_flags()
            ans = str(x.compare(Decimal('Nan891287828'), context=None))
            self.assertEqual(ans, 'NaN1287828')
            self.assertRaises(InvalidOperation, x.compare, Decimal('sNaN'), context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            ans = str(x.compare_signal(8224, context=None))
            self.assertEqual(ans, '-1')
            self.assertRaises(InvalidOperation, x.compare_signal, Decimal('NaN'), context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            ans = str(x.logical_and(101, context=None))
            self.assertEqual(ans, '101')
            self.assertRaises(InvalidOperation, x.logical_and, 123, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            ans = str(x.logical_or(101, context=None))
            self.assertEqual(ans, '111')
            self.assertRaises(InvalidOperation, x.logical_or, 123, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            ans = str(x.logical_xor(101, context=None))
            self.assertEqual(ans, '10')
            self.assertRaises(InvalidOperation, x.logical_xor, 123, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            ans = str(x.max(101, context=None))
            self.assertEqual(ans, '111')
            self.assertRaises(InvalidOperation, x.max, Decimal('sNaN'), context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            ans = str(x.max_mag(101, context=None))
            self.assertEqual(ans, '111')
            self.assertRaises(InvalidOperation, x.max_mag, Decimal('sNaN'), context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            ans = str(x.min(101, context=None))
            self.assertEqual(ans, '101')
            self.assertRaises(InvalidOperation, x.min, Decimal('sNaN'), context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            ans = str(x.min_mag(101, context=None))
            self.assertEqual(ans, '101')
            self.assertRaises(InvalidOperation, x.min_mag, Decimal('sNaN'), context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            ans = str(x.remainder_near(101, context=None))
            self.assertEqual(ans, '10')
            self.assertRaises(InvalidOperation, y.remainder_near, 101, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            ans = str(x.rotate(2, context=None))
            self.assertEqual(ans, '11100')
            self.assertRaises(InvalidOperation, x.rotate, 101, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            ans = str(x.scaleb(7, context=None))
            self.assertEqual(ans, '1.11E+9')
            self.assertRaises(InvalidOperation, x.scaleb, 10000, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            ans = str(x.shift(2, context=None))
            self.assertEqual(ans, '11100')
            self.assertRaises(InvalidOperation, x.shift, 10000, context=None)
            self.assertTrue(c.flags[InvalidOperation])


            ##### Ternary functions
            c.clear_flags()
            ans = str(x.fma(2, 3, context=None))
            self.assertEqual(ans, '225')
            self.assertRaises(Overflow, x.fma, Decimal('1e9999'), 3, context=None)
            self.assertTrue(c.flags[Overflow])


            ##### Special cases
            c.rounding = P.ROUND_HALF_EVEN
            ans = str(Decimal('1.5').to_integral(rounding=None, context=None))
            self.assertEqual(ans, '2')
            c.rounding = P.ROUND_DOWN
            ans = str(Decimal('1.5').to_integral(rounding=None, context=None))
            self.assertEqual(ans, '1')
            ans = str(Decimal('1.5').to_integral(rounding=P.ROUND_UP, context=None))
            self.assertEqual(ans, '2')
            c.clear_flags()
            self.assertRaises(InvalidOperation, Decimal('sNaN').to_integral, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.rounding = P.ROUND_HALF_EVEN
            ans = str(Decimal('1.5').to_integral_value(rounding=None, context=None))
            self.assertEqual(ans, '2')
            c.rounding = P.ROUND_DOWN
            ans = str(Decimal('1.5').to_integral_value(rounding=None, context=None))
            self.assertEqual(ans, '1')
            ans = str(Decimal('1.5').to_integral_value(rounding=P.ROUND_UP, context=None))
            self.assertEqual(ans, '2')
            c.clear_flags()
            self.assertRaises(InvalidOperation, Decimal('sNaN').to_integral_value, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.rounding = P.ROUND_HALF_EVEN
            ans = str(Decimal('1.5').to_integral_exact(rounding=None, context=None))
            self.assertEqual(ans, '2')
            c.rounding = P.ROUND_DOWN
            ans = str(Decimal('1.5').to_integral_exact(rounding=None, context=None))
            self.assertEqual(ans, '1')
            ans = str(Decimal('1.5').to_integral_exact(rounding=P.ROUND_UP, context=None))
            self.assertEqual(ans, '2')
            c.clear_flags()
            self.assertRaises(InvalidOperation, Decimal('sNaN').to_integral_exact, context=None)
            self.assertTrue(c.flags[InvalidOperation])

            c.rounding = P.ROUND_UP
            ans = str(Decimal('1.50001').quantize(exp=Decimal('1e-3'), rounding=None, context=None))
            self.assertEqual(ans, '1.501')
            c.rounding = P.ROUND_DOWN
            ans = str(Decimal('1.50001').quantize(exp=Decimal('1e-3'), rounding=None, context=None))
            self.assertEqual(ans, '1.500')
            ans = str(Decimal('1.50001').quantize(exp=Decimal('1e-3'), rounding=P.ROUND_UP, context=None))
            self.assertEqual(ans, '1.501')
            c.clear_flags()
            self.assertRaises(InvalidOperation, y.quantize, Decimal('1e-10'), rounding=P.ROUND_UP, context=None)
            self.assertTrue(c.flags[InvalidOperation])

        with localcontext(Context()) as context:
            context.prec = 7
            context.Emax = 999
            context.Emin = -999
            with localcontext(ctx=None) as c:
                self.assertEqual(c.prec, 7)
                self.assertEqual(c.Emax, 999)
                self.assertEqual(c.Emin, -999)

    def test_conversions_from_int(self):
        # Check that methods taking a second Decimal argument will
        # always accept an integer in place of a Decimal.
        Decimal = self.decimal.Decimal

        self.assertEqual(Decimal(4).compare(3),
                         Decimal(4).compare(Decimal(3)))
        self.assertEqual(Decimal(4).compare_signal(3),
                         Decimal(4).compare_signal(Decimal(3)))
        self.assertEqual(Decimal(4).compare_total(3),
                         Decimal(4).compare_total(Decimal(3)))
        self.assertEqual(Decimal(4).compare_total_mag(3),
                         Decimal(4).compare_total_mag(Decimal(3)))
        self.assertEqual(Decimal(10101).logical_and(1001),
                         Decimal(10101).logical_and(Decimal(1001)))
        self.assertEqual(Decimal(10101).logical_or(1001),
                         Decimal(10101).logical_or(Decimal(1001)))
        self.assertEqual(Decimal(10101).logical_xor(1001),
                         Decimal(10101).logical_xor(Decimal(1001)))
        self.assertEqual(Decimal(567).max(123),
                         Decimal(567).max(Decimal(123)))
        self.assertEqual(Decimal(567).max_mag(123),
                         Decimal(567).max_mag(Decimal(123)))
        self.assertEqual(Decimal(567).min(123),
                         Decimal(567).min(Decimal(123)))
        self.assertEqual(Decimal(567).min_mag(123),
                         Decimal(567).min_mag(Decimal(123)))
        self.assertEqual(Decimal(567).next_toward(123),
                         Decimal(567).next_toward(Decimal(123)))
        self.assertEqual(Decimal(1234).quantize(100),
                         Decimal(1234).quantize(Decimal(100)))
        self.assertEqual(Decimal(768).remainder_near(1234),
                         Decimal(768).remainder_near(Decimal(1234)))
        self.assertEqual(Decimal(123).rotate(1),
                         Decimal(123).rotate(Decimal(1)))
        self.assertEqual(Decimal(1234).same_quantum(1000),
                         Decimal(1234).same_quantum(Decimal(1000)))
        self.assertEqual(Decimal('9.123').scaleb(-100),
                         Decimal('9.123').scaleb(Decimal(-100)))
        self.assertEqual(Decimal(456).shift(-1),
                         Decimal(456).shift(Decimal(-1)))

        self.assertEqual(Decimal(-12).fma(Decimal(45), 67),
                         Decimal(-12).fma(Decimal(45), Decimal(67)))
        self.assertEqual(Decimal(-12).fma(45, 67),
                         Decimal(-12).fma(Decimal(45), Decimal(67)))
        self.assertEqual(Decimal(-12).fma(45, Decimal(67)),
                         Decimal(-12).fma(Decimal(45), Decimal(67)))


@requires_cdecimal
class CUsabilityTest(UsabilityTest, unittest.TestCase):
    decimal = C


class PyUsabilityTest(UsabilityTest, unittest.TestCase):
    decimal = P

    def setUp(self):
        super().setUp()
        self._previous_int_limit = sys.get_int_max_str_digits()
        sys.set_int_max_str_digits(7000)

    def tearDown(self):
        sys.set_int_max_str_digits(self._previous_int_limit)
        super().tearDown()


if __name__ == '__main__':
    unittest.main()
