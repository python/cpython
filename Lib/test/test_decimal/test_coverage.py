import unittest
import sys
from . import (
    C, P, requires_cdecimal, OrderedSignals,
    setUpModule, tearDownModule)


class Coverage:
    def test_adjusted(self):
        Decimal = self.decimal.Decimal

        self.assertEqual(Decimal('1234e9999').adjusted(), 10002)
        # XXX raise?
        self.assertEqual(Decimal('nan').adjusted(), 0)
        self.assertEqual(Decimal('inf').adjusted(), 0)

    def test_canonical(self):
        Decimal = self.decimal.Decimal
        getcontext = self.decimal.getcontext

        x = Decimal(9).canonical()
        self.assertEqual(x, 9)

        c = getcontext()
        x = c.canonical(Decimal(9))
        self.assertEqual(x, 9)

    def test_context_repr(self):
        c = self.decimal.DefaultContext.copy()

        c.prec = 425000000
        c.Emax = 425000000
        c.Emin = -425000000
        c.rounding = P.ROUND_HALF_DOWN
        c.capitals = 0
        c.clamp = 1
        for sig in OrderedSignals[self.decimal]:
            c.flags[sig] = False
            c.traps[sig] = False

        s = c.__repr__()
        t = "Context(prec=425000000, rounding=ROUND_HALF_DOWN, " \
            "Emin=-425000000, Emax=425000000, capitals=0, clamp=1, " \
            "flags=[], traps=[])"
        self.assertEqual(s, t)

    def test_implicit_context(self):
        Decimal = self.decimal.Decimal
        localcontext = self.decimal.localcontext

        with localcontext() as c:
            c.prec = 1
            c.Emax = 1
            c.Emin = -1

            # abs
            self.assertEqual(abs(Decimal("-10")), 10)
            # add
            self.assertEqual(Decimal("7") + 1, 8)
            # divide
            self.assertEqual(Decimal("10") / 5, 2)
            # divide_int
            self.assertEqual(Decimal("10") // 7, 1)
            # fma
            self.assertEqual(Decimal("1.2").fma(Decimal("0.01"), 1), 1)
            self.assertIs(Decimal("NaN").fma(7, 1).is_nan(), True)
            # three arg power
            self.assertEqual(pow(Decimal(10), 2, 7), 2)
            self.assertEqual(pow(10, Decimal(2), 7), 2)
            if self.decimal == C:
                self.assertEqual(pow(10, 2, Decimal(7)), 2)
            else:
                # XXX: There is no special method to dispatch on the
                # third arg of three-arg power.
                self.assertRaises(TypeError, pow, 10, 2, Decimal(7))
            # exp
            self.assertEqual(Decimal("1.01").exp(), 3)
            # is_normal
            self.assertIs(Decimal("0.01").is_normal(), False)
            # is_subnormal
            self.assertIs(Decimal("0.01").is_subnormal(), True)
            # ln
            self.assertEqual(Decimal("20").ln(), 3)
            # log10
            self.assertEqual(Decimal("20").log10(), 1)
            # logb
            self.assertEqual(Decimal("580").logb(), 2)
            # logical_invert
            self.assertEqual(Decimal("10").logical_invert(), 1)
            # minus
            self.assertEqual(-Decimal("-10"), 10)
            # multiply
            self.assertEqual(Decimal("2") * 4, 8)
            # next_minus
            self.assertEqual(Decimal("10").next_minus(), 9)
            # next_plus
            self.assertEqual(Decimal("10").next_plus(), Decimal('2E+1'))
            # normalize
            self.assertEqual(Decimal("-10").normalize(), Decimal('-1E+1'))
            # number_class
            self.assertEqual(Decimal("10").number_class(), '+Normal')
            # plus
            self.assertEqual(+Decimal("-1"), -1)
            # remainder
            self.assertEqual(Decimal("10") % 7, 3)
            # subtract
            self.assertEqual(Decimal("10") - 7, 3)
            # to_integral_exact
            self.assertEqual(Decimal("1.12345").to_integral_exact(), 1)

            # Boolean functions
            self.assertTrue(Decimal("1").is_canonical())
            self.assertTrue(Decimal("1").is_finite())
            self.assertTrue(Decimal("1").is_finite())
            self.assertTrue(Decimal("snan").is_snan())
            self.assertTrue(Decimal("-1").is_signed())
            self.assertTrue(Decimal("0").is_zero())
            self.assertTrue(Decimal("0").is_zero())

        # Copy
        with localcontext() as c:
            c.prec = 10000
            x = 1228 ** 1523
            y = -Decimal(x)

            z = y.copy_abs()
            self.assertEqual(z, x)

            z = y.copy_negate()
            self.assertEqual(z, x)

            z = y.copy_sign(Decimal(1))
            self.assertEqual(z, x)

    def test_divmod(self):
        Decimal = self.decimal.Decimal
        localcontext = self.decimal.localcontext
        InvalidOperation = self.decimal.InvalidOperation
        DivisionByZero = self.decimal.DivisionByZero

        with localcontext() as c:
            q, r = divmod(Decimal("10912837129"), 1001)
            self.assertEqual(q, Decimal('10901935'))
            self.assertEqual(r, Decimal('194'))

            q, r = divmod(Decimal("NaN"), 7)
            self.assertTrue(q.is_nan() and r.is_nan())

            c.traps[InvalidOperation] = False
            q, r = divmod(Decimal("NaN"), 7)
            self.assertTrue(q.is_nan() and r.is_nan())

            c.traps[InvalidOperation] = False
            c.clear_flags()
            q, r = divmod(Decimal("inf"), Decimal("inf"))
            self.assertTrue(q.is_nan() and r.is_nan())
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            q, r = divmod(Decimal("inf"), 101)
            self.assertTrue(q.is_infinite() and r.is_nan())
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            q, r = divmod(Decimal(0), 0)
            self.assertTrue(q.is_nan() and r.is_nan())
            self.assertTrue(c.flags[InvalidOperation])

            c.traps[DivisionByZero] = False
            c.clear_flags()
            q, r = divmod(Decimal(11), 0)
            self.assertTrue(q.is_infinite() and r.is_nan())
            self.assertTrue(c.flags[InvalidOperation] and
                            c.flags[DivisionByZero])

    def test_power(self):
        Decimal = self.decimal.Decimal
        localcontext = self.decimal.localcontext
        Overflow = self.decimal.Overflow
        Rounded = self.decimal.Rounded

        with localcontext() as c:
            c.prec = 3
            c.clear_flags()
            self.assertEqual(Decimal("1.0") ** 100, Decimal('1.00'))
            self.assertTrue(c.flags[Rounded])

            c.prec = 1
            c.Emax = 1
            c.Emin = -1
            c.clear_flags()
            c.traps[Overflow] = False
            self.assertEqual(Decimal(10000) ** Decimal("0.5"), Decimal('inf'))
            self.assertTrue(c.flags[Overflow])

    def test_quantize(self):
        Decimal = self.decimal.Decimal
        localcontext = self.decimal.localcontext
        InvalidOperation = self.decimal.InvalidOperation

        with localcontext() as c:
            c.prec = 1
            c.Emax = 1
            c.Emin = -1
            c.traps[InvalidOperation] = False
            x = Decimal(99).quantize(Decimal("1e1"))
            self.assertTrue(x.is_nan())

    def test_radix(self):
        Decimal = self.decimal.Decimal
        getcontext = self.decimal.getcontext

        c = getcontext()
        self.assertEqual(Decimal("1").radix(), 10)
        self.assertEqual(c.radix(), 10)

    def test_rop(self):
        Decimal = self.decimal.Decimal

        for attr in ('__radd__', '__rsub__', '__rmul__', '__rtruediv__',
                     '__rdivmod__', '__rmod__', '__rfloordiv__', '__rpow__'):
            self.assertIs(getattr(Decimal("1"), attr)("xyz"), NotImplemented)

    def test_round(self):
        # Python3 behavior: round() returns Decimal
        Decimal = self.decimal.Decimal
        localcontext = self.decimal.localcontext

        with localcontext() as c:
            c.prec = 28

            self.assertEqual(str(Decimal("9.99").__round__()), "10")
            self.assertEqual(str(Decimal("9.99e-5").__round__()), "0")
            self.assertEqual(str(Decimal("1.23456789").__round__(5)), "1.23457")
            self.assertEqual(str(Decimal("1.2345").__round__(10)), "1.2345000000")
            self.assertEqual(str(Decimal("1.2345").__round__(-10)), "0E+10")

            self.assertRaises(TypeError, Decimal("1.23").__round__, "5")
            self.assertRaises(TypeError, Decimal("1.23").__round__, 5, 8)

    def test_create_decimal(self):
        c = self.decimal.Context()
        self.assertRaises(ValueError, c.create_decimal, ["%"])

    def test_int(self):
        Decimal = self.decimal.Decimal
        localcontext = self.decimal.localcontext

        with localcontext() as c:
            c.prec = 9999
            x = Decimal(1221**1271) / 10**3923
            self.assertEqual(int(x), 1)
            self.assertEqual(x.to_integral(), 2)

    def test_copy(self):
        Context = self.decimal.Context

        c = Context()
        c.prec = 10000
        x = -(1172 ** 1712)

        y = c.copy_abs(x)
        self.assertEqual(y, -x)

        y = c.copy_negate(x)
        self.assertEqual(y, -x)

        y = c.copy_sign(x, 1)
        self.assertEqual(y, -x)


@requires_cdecimal
class CCoverage(Coverage, unittest.TestCase):
    decimal = C
class PyCoverage(Coverage, unittest.TestCase):
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
