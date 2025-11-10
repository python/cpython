import unittest
import copy
import random
from test.support import requires_IEEE_754
from . import (C, P, requires_cdecimal,
               setUpModule, tearDownModule)

# The following classes test the behaviour of Decimal according to PEP 327

class ExplicitConstructionTest:
    '''Unit tests for Explicit Construction cases of Decimal.'''

    def test_explicit_empty(self):
        Decimal = self.decimal.Decimal
        self.assertEqual(Decimal(), Decimal("0"))

    def test_explicit_from_None(self):
        Decimal = self.decimal.Decimal
        self.assertRaises(TypeError, Decimal, None)

    def test_explicit_from_int(self):
        Decimal = self.decimal.Decimal

        #positive
        d = Decimal(45)
        self.assertEqual(str(d), '45')

        #very large positive
        d = Decimal(500000123)
        self.assertEqual(str(d), '500000123')

        #negative
        d = Decimal(-45)
        self.assertEqual(str(d), '-45')

        #zero
        d = Decimal(0)
        self.assertEqual(str(d), '0')

        # single word longs
        for n in range(0, 32):
            for sign in (-1, 1):
                for x in range(-5, 5):
                    i = sign * (2**n + x)
                    d = Decimal(i)
                    self.assertEqual(str(d), str(i))

    def test_explicit_from_string(self):
        Decimal = self.decimal.Decimal
        InvalidOperation = self.decimal.InvalidOperation
        localcontext = self.decimal.localcontext

        #empty
        self.assertEqual(str(Decimal('')), 'NaN')

        #int
        self.assertEqual(str(Decimal('45')), '45')

        #float
        self.assertEqual(str(Decimal('45.34')), '45.34')

        #engineer notation
        self.assertEqual(str(Decimal('45e2')), '4.5E+3')

        #just not a number
        self.assertEqual(str(Decimal('ugly')), 'NaN')

        #leading and trailing whitespace permitted
        self.assertEqual(str(Decimal('1.3E4 \n')), '1.3E+4')
        self.assertEqual(str(Decimal('  -7.89')), '-7.89')
        self.assertEqual(str(Decimal("  3.45679  ")), '3.45679')

        # underscores
        self.assertEqual(str(Decimal('1_3.3e4_0')), '1.33E+41')
        self.assertEqual(str(Decimal('1_0_0_0')), '1000')

        # unicode whitespace
        for lead in ["", ' ', '\u00a0', '\u205f']:
            for trail in ["", ' ', '\u00a0', '\u205f']:
                self.assertEqual(str(Decimal(lead + '9.311E+28' + trail)),
                                 '9.311E+28')

        with localcontext() as c:
            c.traps[InvalidOperation] = True
            # Invalid string
            self.assertRaises(InvalidOperation, Decimal, "xyz")
            # Two arguments max
            self.assertRaises(TypeError, Decimal, "1234", "x", "y")

            # space within the numeric part
            self.assertRaises(InvalidOperation, Decimal, "1\u00a02\u00a03")
            self.assertRaises(InvalidOperation, Decimal, "\u00a01\u00a02\u00a0")

            # unicode whitespace
            self.assertRaises(InvalidOperation, Decimal, "\u00a0")
            self.assertRaises(InvalidOperation, Decimal, "\u00a0\u00a0")

            # embedded NUL
            self.assertRaises(InvalidOperation, Decimal, "12\u00003")

            # underscores don't prevent errors
            self.assertRaises(InvalidOperation, Decimal, "1_2_\u00003")

    def test_explicit_from_tuples(self):
        Decimal = self.decimal.Decimal

        #zero
        d = Decimal( (0, (0,), 0) )
        self.assertEqual(str(d), '0')

        #int
        d = Decimal( (1, (4, 5), 0) )
        self.assertEqual(str(d), '-45')

        #float
        d = Decimal( (0, (4, 5, 3, 4), -2) )
        self.assertEqual(str(d), '45.34')

        #weird
        d = Decimal( (1, (4, 3, 4, 9, 1, 3, 5, 3, 4), -25) )
        self.assertEqual(str(d), '-4.34913534E-17')

        #inf
        d = Decimal( (0, (), "F") )
        self.assertEqual(str(d), 'Infinity')

        #wrong number of items
        self.assertRaises(ValueError, Decimal, (1, (4, 3, 4, 9, 1)) )

        #bad sign
        self.assertRaises(ValueError, Decimal, (8, (4, 3, 4, 9, 1), 2) )
        self.assertRaises(ValueError, Decimal, (0., (4, 3, 4, 9, 1), 2) )
        self.assertRaises(ValueError, Decimal, (Decimal(1), (4, 3, 4, 9, 1), 2))

        #bad exp
        self.assertRaises(ValueError, Decimal, (1, (4, 3, 4, 9, 1), 'wrong!') )
        self.assertRaises(ValueError, Decimal, (1, (4, 3, 4, 9, 1), 0.) )
        self.assertRaises(ValueError, Decimal, (1, (4, 3, 4, 9, 1), '1') )

        #bad coefficients
        self.assertRaises(ValueError, Decimal, (1, "xyz", 2) )
        self.assertRaises(ValueError, Decimal, (1, (4, 3, 4, None, 1), 2) )
        self.assertRaises(ValueError, Decimal, (1, (4, -3, 4, 9, 1), 2) )
        self.assertRaises(ValueError, Decimal, (1, (4, 10, 4, 9, 1), 2) )
        self.assertRaises(ValueError, Decimal, (1, (4, 3, 4, 'a', 1), 2) )

    def test_explicit_from_list(self):
        Decimal = self.decimal.Decimal

        d = Decimal([0, [0], 0])
        self.assertEqual(str(d), '0')

        d = Decimal([1, [4, 3, 4, 9, 1, 3, 5, 3, 4], -25])
        self.assertEqual(str(d), '-4.34913534E-17')

        d = Decimal([1, (4, 3, 4, 9, 1, 3, 5, 3, 4), -25])
        self.assertEqual(str(d), '-4.34913534E-17')

        d = Decimal((1, [4, 3, 4, 9, 1, 3, 5, 3, 4], -25))
        self.assertEqual(str(d), '-4.34913534E-17')

    def test_explicit_from_bool(self):
        Decimal = self.decimal.Decimal

        self.assertIs(bool(Decimal(0)), False)
        self.assertIs(bool(Decimal(1)), True)
        self.assertEqual(Decimal(False), Decimal(0))
        self.assertEqual(Decimal(True), Decimal(1))

    def test_explicit_from_Decimal(self):
        Decimal = self.decimal.Decimal

        #positive
        d = Decimal(45)
        e = Decimal(d)
        self.assertEqual(str(e), '45')

        #very large positive
        d = Decimal(500000123)
        e = Decimal(d)
        self.assertEqual(str(e), '500000123')

        #negative
        d = Decimal(-45)
        e = Decimal(d)
        self.assertEqual(str(e), '-45')

        #zero
        d = Decimal(0)
        e = Decimal(d)
        self.assertEqual(str(e), '0')

    @requires_IEEE_754
    def test_explicit_from_float(self):
        Decimal = self.decimal.Decimal

        r = Decimal(0.1)
        self.assertEqual(type(r), Decimal)
        self.assertEqual(str(r),
                '0.1000000000000000055511151231257827021181583404541015625')
        self.assertTrue(Decimal(float('nan')).is_qnan())
        self.assertTrue(Decimal(float('inf')).is_infinite())
        self.assertTrue(Decimal(float('-inf')).is_infinite())
        self.assertEqual(str(Decimal(float('nan'))),
                         str(Decimal('NaN')))
        self.assertEqual(str(Decimal(float('inf'))),
                         str(Decimal('Infinity')))
        self.assertEqual(str(Decimal(float('-inf'))),
                         str(Decimal('-Infinity')))
        self.assertEqual(str(Decimal(float('-0.0'))),
                         str(Decimal('-0')))
        for i in range(200):
            x = random.expovariate(0.01) * (random.random() * 2.0 - 1.0)
            self.assertEqual(x, float(Decimal(x))) # roundtrip

    def test_explicit_context_create_decimal(self):
        Decimal = self.decimal.Decimal
        InvalidOperation = self.decimal.InvalidOperation
        Rounded = self.decimal.Rounded

        nc = copy.copy(self.decimal.getcontext())
        nc.prec = 3

        # empty
        d = Decimal()
        self.assertEqual(str(d), '0')
        d = nc.create_decimal()
        self.assertEqual(str(d), '0')

        # from None
        self.assertRaises(TypeError, nc.create_decimal, None)

        # from int
        d = nc.create_decimal(456)
        self.assertIsInstance(d, Decimal)
        self.assertEqual(nc.create_decimal(45678),
                         nc.create_decimal('457E+2'))

        # from string
        d = Decimal('456789')
        self.assertEqual(str(d), '456789')
        d = nc.create_decimal('456789')
        self.assertEqual(str(d), '4.57E+5')
        # leading and trailing whitespace should result in a NaN;
        # spaces are already checked in Cowlishaw's test-suite, so
        # here we just check that a trailing newline results in a NaN
        self.assertEqual(str(nc.create_decimal('3.14\n')), 'NaN')

        # from tuples
        d = Decimal( (1, (4, 3, 4, 9, 1, 3, 5, 3, 4), -25) )
        self.assertEqual(str(d), '-4.34913534E-17')
        d = nc.create_decimal( (1, (4, 3, 4, 9, 1, 3, 5, 3, 4), -25) )
        self.assertEqual(str(d), '-4.35E-17')

        # from Decimal
        prevdec = Decimal(500000123)
        d = Decimal(prevdec)
        self.assertEqual(str(d), '500000123')
        d = nc.create_decimal(prevdec)
        self.assertEqual(str(d), '5.00E+8')

        # more integers
        nc.prec = 28
        nc.traps[InvalidOperation] = True

        for v in [-2**63-1, -2**63, -2**31-1, -2**31, 0,
                   2**31-1, 2**31, 2**63-1, 2**63]:
            d = nc.create_decimal(v)
            self.assertIsInstance(d, Decimal)
            self.assertEqual(int(d), v)

        nc.prec = 3
        nc.traps[Rounded] = True
        self.assertRaises(Rounded, nc.create_decimal, 1234)

        # from string
        nc.prec = 28
        self.assertEqual(str(nc.create_decimal('0E-017')), '0E-17')
        self.assertEqual(str(nc.create_decimal('45')), '45')
        self.assertEqual(str(nc.create_decimal('-Inf')), '-Infinity')
        self.assertEqual(str(nc.create_decimal('NaN123')), 'NaN123')

        # invalid arguments
        self.assertRaises(InvalidOperation, nc.create_decimal, "xyz")
        self.assertRaises(ValueError, nc.create_decimal, (1, "xyz", -25))
        self.assertRaises(TypeError, nc.create_decimal, "1234", "5678")
        # no whitespace and underscore stripping is done with this method
        self.assertRaises(InvalidOperation, nc.create_decimal, " 1234")
        self.assertRaises(InvalidOperation, nc.create_decimal, "12_34")

        # too many NaN payload digits
        nc.prec = 3
        self.assertRaises(InvalidOperation, nc.create_decimal, 'NaN12345')
        self.assertRaises(InvalidOperation, nc.create_decimal,
                          Decimal('NaN12345'))

        nc.traps[InvalidOperation] = False
        self.assertEqual(str(nc.create_decimal('NaN12345')), 'NaN')
        self.assertTrue(nc.flags[InvalidOperation])

        nc.flags[InvalidOperation] = False
        self.assertEqual(str(nc.create_decimal(Decimal('NaN12345'))), 'NaN')
        self.assertTrue(nc.flags[InvalidOperation])

    def test_explicit_context_create_from_float(self):
        Decimal = self.decimal.Decimal

        nc = self.decimal.Context()
        r = nc.create_decimal(0.1)
        self.assertEqual(type(r), Decimal)
        self.assertEqual(str(r), '0.1000000000000000055511151231')
        self.assertTrue(nc.create_decimal(float('nan')).is_qnan())
        self.assertTrue(nc.create_decimal(float('inf')).is_infinite())
        self.assertTrue(nc.create_decimal(float('-inf')).is_infinite())
        self.assertEqual(str(nc.create_decimal(float('nan'))),
                         str(nc.create_decimal('NaN')))
        self.assertEqual(str(nc.create_decimal(float('inf'))),
                         str(nc.create_decimal('Infinity')))
        self.assertEqual(str(nc.create_decimal(float('-inf'))),
                         str(nc.create_decimal('-Infinity')))
        self.assertEqual(str(nc.create_decimal(float('-0.0'))),
                         str(nc.create_decimal('-0')))
        nc.prec = 100
        for i in range(200):
            x = random.expovariate(0.01) * (random.random() * 2.0 - 1.0)
            self.assertEqual(x, float(nc.create_decimal(x))) # roundtrip

    def test_from_number(self, cls=None):
        Decimal = self.decimal.Decimal
        if cls is None:
            cls = Decimal

        def check(arg, expected):
            d = cls.from_number(arg)
            self.assertIs(type(d), cls)
            self.assertEqual(d, expected)

        check(314, Decimal(314))
        check(3.14, Decimal.from_float(3.14))
        check(Decimal('3.14'), Decimal('3.14'))
        self.assertRaises(TypeError, cls.from_number, 3+4j)
        self.assertRaises(TypeError, cls.from_number, '314')
        self.assertRaises(TypeError, cls.from_number, (0, (3, 1, 4), 0))
        self.assertRaises(TypeError, cls.from_number, object())

    def test_from_number_subclass(self, cls=None):
        class DecimalSubclass(self.decimal.Decimal):
            pass
        self.test_from_number(DecimalSubclass)

    def test_unicode_digits(self):
        Decimal = self.decimal.Decimal

        test_values = {
            '\uff11': '1',
            '\u0660.\u0660\u0663\u0667\u0662e-\u0663' : '0.0000372',
            '-nan\u0c68\u0c6a\u0c66\u0c66' : '-NaN2400',
            }
        for input, expected in test_values.items():
            self.assertEqual(str(Decimal(input)), expected)

@requires_cdecimal
class CExplicitConstructionTest(ExplicitConstructionTest, unittest.TestCase):
    decimal = C
class PyExplicitConstructionTest(ExplicitConstructionTest, unittest.TestCase):
    decimal = P

class ImplicitConstructionTest:
    '''Unit tests for Implicit Construction cases of Decimal.'''

    def test_implicit_from_None(self):
        Decimal = self.decimal.Decimal
        self.assertRaises(TypeError, eval, 'Decimal(5) + None', locals())

    def test_implicit_from_int(self):
        Decimal = self.decimal.Decimal

        #normal
        self.assertEqual(str(Decimal(5) + 45), '50')
        #exceeding precision
        self.assertEqual(Decimal(5) + 123456789000, Decimal(123456789000))

    def test_implicit_from_string(self):
        Decimal = self.decimal.Decimal
        self.assertRaises(TypeError, eval, 'Decimal(5) + "3"', locals())

    def test_implicit_from_float(self):
        Decimal = self.decimal.Decimal
        self.assertRaises(TypeError, eval, 'Decimal(5) + 2.2', locals())

    def test_implicit_from_Decimal(self):
        Decimal = self.decimal.Decimal
        self.assertEqual(Decimal(5) + Decimal(45), Decimal(50))

    def test_rop(self):
        Decimal = self.decimal.Decimal

        # Allow other classes to be trained to interact with Decimals
        class E:
            def __divmod__(self, other):
                return 'divmod ' + str(other)
            def __rdivmod__(self, other):
                return str(other) + ' rdivmod'
            def __lt__(self, other):
                return 'lt ' + str(other)
            def __gt__(self, other):
                return 'gt ' + str(other)
            def __le__(self, other):
                return 'le ' + str(other)
            def __ge__(self, other):
                return 'ge ' + str(other)
            def __eq__(self, other):
                return 'eq ' + str(other)
            def __ne__(self, other):
                return 'ne ' + str(other)

        self.assertEqual(divmod(E(), Decimal(10)), 'divmod 10')
        self.assertEqual(divmod(Decimal(10), E()), '10 rdivmod')
        self.assertEqual(eval('Decimal(10) < E()'), 'gt 10')
        self.assertEqual(eval('Decimal(10) > E()'), 'lt 10')
        self.assertEqual(eval('Decimal(10) <= E()'), 'ge 10')
        self.assertEqual(eval('Decimal(10) >= E()'), 'le 10')
        self.assertEqual(eval('Decimal(10) == E()'), 'eq 10')
        self.assertEqual(eval('Decimal(10) != E()'), 'ne 10')

        # insert operator methods and then exercise them
        oplist = [
            ('+', '__add__', '__radd__'),
            ('-', '__sub__', '__rsub__'),
            ('*', '__mul__', '__rmul__'),
            ('/', '__truediv__', '__rtruediv__'),
            ('%', '__mod__', '__rmod__'),
            ('//', '__floordiv__', '__rfloordiv__'),
            ('**', '__pow__', '__rpow__')
        ]

        for sym, lop, rop in oplist:
            setattr(E, lop, lambda self, other: 'str' + lop + str(other))
            setattr(E, rop, lambda self, other: str(other) + rop + 'str')
            self.assertEqual(eval('E()' + sym + 'Decimal(10)'),
                             'str' + lop + '10')
            self.assertEqual(eval('Decimal(10)' + sym + 'E()'),
                             '10' + rop + 'str')

@requires_cdecimal
class CImplicitConstructionTest(ImplicitConstructionTest, unittest.TestCase):
    decimal = C
class PyImplicitConstructionTest(ImplicitConstructionTest, unittest.TestCase):
    decimal = P

if __name__ == '__main__':
    unittest.main()
