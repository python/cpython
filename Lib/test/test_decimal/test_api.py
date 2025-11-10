import unittest
import math
import sys
import pickle
import numbers
from test.support import cpython_only
import random
from . import (C, P, load_tests_for_base_classes, assert_signals,
               RoundingModes, OrderedSignals,
               setUpModule, tearDownModule)


class PythonAPItests:
    def test_abc(self):
        Decimal = self.decimal.Decimal

        self.assertIsSubclass(Decimal, numbers.Number)
        self.assertNotIsSubclass(Decimal, numbers.Real)
        self.assertIsInstance(Decimal(0), numbers.Number)
        self.assertNotIsInstance(Decimal(0), numbers.Real)

    def test_pickle(self):
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            Decimal = self.decimal.Decimal

            savedecimal = sys.modules['decimal']

            # Round trip
            sys.modules['decimal'] = self.decimal
            d = Decimal('-3.141590000')
            p = pickle.dumps(d, proto)
            e = pickle.loads(p)
            self.assertEqual(d, e)

            if C:
                # Test interchangeability
                x = C.Decimal('-3.123e81723')
                y = P.Decimal('-3.123e81723')

                sys.modules['decimal'] = C
                sx = pickle.dumps(x, proto)
                sys.modules['decimal'] = P
                r = pickle.loads(sx)
                self.assertIsInstance(r, P.Decimal)
                self.assertEqual(r, y)

                sys.modules['decimal'] = P
                sy = pickle.dumps(y, proto)
                sys.modules['decimal'] = C
                r = pickle.loads(sy)
                self.assertIsInstance(r, C.Decimal)
                self.assertEqual(r, x)

                x = C.Decimal('-3.123e81723').as_tuple()
                y = P.Decimal('-3.123e81723').as_tuple()

                sys.modules['decimal'] = C
                sx = pickle.dumps(x, proto)
                sys.modules['decimal'] = P
                r = pickle.loads(sx)
                self.assertIsInstance(r, P.DecimalTuple)
                self.assertEqual(r, y)

                sys.modules['decimal'] = P
                sy = pickle.dumps(y, proto)
                sys.modules['decimal'] = C
                r = pickle.loads(sy)
                self.assertIsInstance(r, C.DecimalTuple)
                self.assertEqual(r, x)

            sys.modules['decimal'] = savedecimal

    def test_int(self):
        Decimal = self.decimal.Decimal

        for x in range(-250, 250):
            s = '%0.2f' % (x / 100.0)
            # should work the same as for floats
            self.assertEqual(int(Decimal(s)), int(float(s)))
            # should work the same as to_integral in the P.ROUND_DOWN mode
            d = Decimal(s)
            r = d.to_integral(P.ROUND_DOWN)
            self.assertEqual(Decimal(int(d)), r)

        self.assertRaises(ValueError, int, Decimal('-nan'))
        self.assertRaises(ValueError, int, Decimal('snan'))
        self.assertRaises(OverflowError, int, Decimal('inf'))
        self.assertRaises(OverflowError, int, Decimal('-inf'))

    @cpython_only
    def test_small_ints(self):
        Decimal = self.decimal.Decimal
        # bpo-46361
        for x in range(-5, 257):
            self.assertIs(int(Decimal(x)), x)

    def test_trunc(self):
        Decimal = self.decimal.Decimal

        for x in range(-250, 250):
            s = '%0.2f' % (x / 100.0)
            # should work the same as for floats
            self.assertEqual(int(Decimal(s)), int(float(s)))
            # should work the same as to_integral in the P.ROUND_DOWN mode
            d = Decimal(s)
            r = d.to_integral(P.ROUND_DOWN)
            self.assertEqual(Decimal(math.trunc(d)), r)

    def test_from_float(self):

        Decimal = self.decimal.Decimal

        class MyDecimal(Decimal):
            def __init__(self, _):
                self.x = 'y'

        self.assertIsSubclass(MyDecimal, Decimal)

        r = MyDecimal.from_float(0.1)
        self.assertEqual(type(r), MyDecimal)
        self.assertEqual(str(r),
                '0.1000000000000000055511151231257827021181583404541015625')
        self.assertEqual(r.x, 'y')

        bigint = 12345678901234567890123456789
        self.assertEqual(MyDecimal.from_float(bigint), MyDecimal(bigint))
        self.assertTrue(MyDecimal.from_float(float('nan')).is_qnan())
        self.assertTrue(MyDecimal.from_float(float('inf')).is_infinite())
        self.assertTrue(MyDecimal.from_float(float('-inf')).is_infinite())
        self.assertEqual(str(MyDecimal.from_float(float('nan'))),
                         str(Decimal('NaN')))
        self.assertEqual(str(MyDecimal.from_float(float('inf'))),
                         str(Decimal('Infinity')))
        self.assertEqual(str(MyDecimal.from_float(float('-inf'))),
                         str(Decimal('-Infinity')))
        self.assertRaises(TypeError, MyDecimal.from_float, 'abc')
        for i in range(200):
            x = random.expovariate(0.01) * (random.random() * 2.0 - 1.0)
            self.assertEqual(x, float(MyDecimal.from_float(x))) # roundtrip

    def test_create_decimal_from_float(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context
        Inexact = self.decimal.Inexact

        context = Context(prec=5, rounding=P.ROUND_DOWN)
        self.assertEqual(
            context.create_decimal_from_float(math.pi),
            Decimal('3.1415')
        )
        context = Context(prec=5, rounding=P.ROUND_UP)
        self.assertEqual(
            context.create_decimal_from_float(math.pi),
            Decimal('3.1416')
        )
        context = Context(prec=5, traps=[Inexact])
        self.assertRaises(
            Inexact,
            context.create_decimal_from_float,
            math.pi
        )
        self.assertEqual(repr(context.create_decimal_from_float(-0.0)),
                         "Decimal('-0')")
        self.assertEqual(repr(context.create_decimal_from_float(1.0)),
                         "Decimal('1')")
        self.assertEqual(repr(context.create_decimal_from_float(10)),
                         "Decimal('10')")

    def test_quantize(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context
        InvalidOperation = self.decimal.InvalidOperation

        c = Context(Emax=99999, Emin=-99999)
        self.assertEqual(
            Decimal('7.335').quantize(Decimal('.01')),
            Decimal('7.34')
        )
        self.assertEqual(
            Decimal('7.335').quantize(Decimal('.01'), rounding=P.ROUND_DOWN),
            Decimal('7.33')
        )
        self.assertRaises(
            InvalidOperation,
            Decimal("10e99999").quantize, Decimal('1e100000'), context=c
        )

        c = Context()
        d = Decimal("0.871831e800")
        x = d.quantize(context=c, exp=Decimal("1e797"), rounding=P.ROUND_DOWN)
        self.assertEqual(x, Decimal('8.71E+799'))

    def test_complex(self):
        Decimal = self.decimal.Decimal

        x = Decimal("9.8182731e181273")
        self.assertEqual(x.real, x)
        self.assertEqual(x.imag, 0)
        self.assertEqual(x.conjugate(), x)

        x = Decimal("1")
        self.assertEqual(complex(x), complex(float(1)))

        self.assertRaises(AttributeError, setattr, x, 'real', 100)
        self.assertRaises(AttributeError, setattr, x, 'imag', 100)
        self.assertRaises(AttributeError, setattr, x, 'conjugate', 100)
        self.assertRaises(AttributeError, setattr, x, '__complex__', 100)

    def test_named_parameters(self):
        D = self.decimal.Decimal
        Context = self.decimal.Context
        localcontext = self.decimal.localcontext
        InvalidOperation = self.decimal.InvalidOperation
        Overflow = self.decimal.Overflow

        xc = Context()
        xc.prec = 1
        xc.Emax = 1
        xc.Emin = -1

        with localcontext() as c:
            c.clear_flags()

            self.assertEqual(D(9, xc), 9)
            self.assertEqual(D(9, context=xc), 9)
            self.assertEqual(D(context=xc, value=9), 9)
            self.assertEqual(D(context=xc), 0)
            xc.clear_flags()
            self.assertRaises(InvalidOperation, D, "xyz", context=xc)
            self.assertTrue(xc.flags[InvalidOperation])
            self.assertFalse(c.flags[InvalidOperation])

            xc.clear_flags()
            self.assertEqual(D(2).exp(context=xc), 7)
            self.assertRaises(Overflow, D(8).exp, context=xc)
            self.assertTrue(xc.flags[Overflow])
            self.assertFalse(c.flags[Overflow])

            xc.clear_flags()
            self.assertEqual(D(2).ln(context=xc), D('0.7'))
            self.assertRaises(InvalidOperation, D(-1).ln, context=xc)
            self.assertTrue(xc.flags[InvalidOperation])
            self.assertFalse(c.flags[InvalidOperation])

            self.assertEqual(D(0).log10(context=xc), D('-inf'))
            self.assertEqual(D(-1).next_minus(context=xc), -2)
            self.assertEqual(D(-1).next_plus(context=xc), D('-0.9'))
            self.assertEqual(D("9.73").normalize(context=xc), D('1E+1'))
            self.assertEqual(D("9999").to_integral(context=xc), 9999)
            self.assertEqual(D("-2000").to_integral_exact(context=xc), -2000)
            self.assertEqual(D("123").to_integral_value(context=xc), 123)
            self.assertEqual(D("0.0625").sqrt(context=xc), D('0.2'))

            self.assertEqual(D("0.0625").compare(context=xc, other=3), -1)
            xc.clear_flags()
            self.assertRaises(InvalidOperation,
                              D("0").compare_signal, D('nan'), context=xc)
            self.assertTrue(xc.flags[InvalidOperation])
            self.assertFalse(c.flags[InvalidOperation])
            self.assertEqual(D("0.01").max(D('0.0101'), context=xc), D('0.0'))
            self.assertEqual(D("0.01").max(D('0.0101'), context=xc), D('0.0'))
            self.assertEqual(D("0.2").max_mag(D('-0.3'), context=xc),
                             D('-0.3'))
            self.assertEqual(D("0.02").min(D('-0.03'), context=xc), D('-0.0'))
            self.assertEqual(D("0.02").min_mag(D('-0.03'), context=xc),
                             D('0.0'))
            self.assertEqual(D("0.2").next_toward(D('-1'), context=xc), D('0.1'))
            xc.clear_flags()
            self.assertRaises(InvalidOperation,
                              D("0.2").quantize, D('1e10'), context=xc)
            self.assertTrue(xc.flags[InvalidOperation])
            self.assertFalse(c.flags[InvalidOperation])
            self.assertEqual(D("9.99").remainder_near(D('1.5'), context=xc),
                             D('-0.5'))

            self.assertEqual(D("9.9").fma(third=D('0.9'), context=xc, other=7),
                             D('7E+1'))

            self.assertRaises(TypeError, D(1).is_canonical, context=xc)
            self.assertRaises(TypeError, D(1).is_finite, context=xc)
            self.assertRaises(TypeError, D(1).is_infinite, context=xc)
            self.assertRaises(TypeError, D(1).is_nan, context=xc)
            self.assertRaises(TypeError, D(1).is_qnan, context=xc)
            self.assertRaises(TypeError, D(1).is_snan, context=xc)
            self.assertRaises(TypeError, D(1).is_signed, context=xc)
            self.assertRaises(TypeError, D(1).is_zero, context=xc)

            self.assertFalse(D("0.01").is_normal(context=xc))
            self.assertTrue(D("0.01").is_subnormal(context=xc))

            self.assertRaises(TypeError, D(1).adjusted, context=xc)
            self.assertRaises(TypeError, D(1).conjugate, context=xc)
            self.assertRaises(TypeError, D(1).radix, context=xc)

            self.assertEqual(D(-111).logb(context=xc), 2)
            self.assertEqual(D(0).logical_invert(context=xc), 1)
            self.assertEqual(D('0.01').number_class(context=xc), '+Subnormal')
            self.assertEqual(D('0.21').to_eng_string(context=xc), '0.21')

            self.assertEqual(D('11').logical_and(D('10'), context=xc), 0)
            self.assertEqual(D('11').logical_or(D('10'), context=xc), 1)
            self.assertEqual(D('01').logical_xor(D('10'), context=xc), 1)
            self.assertEqual(D('23').rotate(1, context=xc), 3)
            self.assertEqual(D('23').rotate(1, context=xc), 3)
            xc.clear_flags()
            self.assertRaises(Overflow,
                              D('23').scaleb, 1, context=xc)
            self.assertTrue(xc.flags[Overflow])
            self.assertFalse(c.flags[Overflow])
            self.assertEqual(D('23').shift(-1, context=xc), 0)

            self.assertRaises(TypeError, D.from_float, 1.1, context=xc)
            self.assertRaises(TypeError, D(0).as_tuple, context=xc)

            self.assertEqual(D(1).canonical(), 1)
            self.assertRaises(TypeError, D("-1").copy_abs, context=xc)
            self.assertRaises(TypeError, D("-1").copy_negate, context=xc)
            self.assertRaises(TypeError, D(1).canonical, context="x")
            self.assertRaises(TypeError, D(1).canonical, xyz="x")

    def test_exception_hierarchy(self):
        DecimalException = self.decimal.DecimalException
        InvalidOperation = self.decimal.InvalidOperation
        FloatOperation = self.decimal.FloatOperation
        DivisionByZero = self.decimal.DivisionByZero
        Overflow = self.decimal.Overflow
        Underflow = self.decimal.Underflow
        Subnormal = self.decimal.Subnormal
        Inexact = self.decimal.Inexact
        Rounded = self.decimal.Rounded
        Clamped = self.decimal.Clamped
        ConversionSyntax = self.decimal.ConversionSyntax
        DivisionImpossible = self.decimal.DivisionImpossible
        DivisionUndefined = self.decimal.DivisionUndefined
        InvalidContext = self.decimal.InvalidContext

        self.assertIsSubclass(DecimalException, ArithmeticError)

        self.assertIsSubclass(InvalidOperation, DecimalException)
        self.assertIsSubclass(FloatOperation, DecimalException)
        self.assertIsSubclass(FloatOperation, TypeError)
        self.assertIsSubclass(DivisionByZero, DecimalException)
        self.assertIsSubclass(DivisionByZero, ZeroDivisionError)
        self.assertIsSubclass(Overflow, Rounded)
        self.assertIsSubclass(Overflow, Inexact)
        self.assertIsSubclass(Overflow, DecimalException)
        self.assertIsSubclass(Underflow, Inexact)
        self.assertIsSubclass(Underflow, Rounded)
        self.assertIsSubclass(Underflow, Subnormal)
        self.assertIsSubclass(Underflow, DecimalException)

        self.assertIsSubclass(Subnormal, DecimalException)
        self.assertIsSubclass(Inexact, DecimalException)
        self.assertIsSubclass(Rounded, DecimalException)
        self.assertIsSubclass(Clamped, DecimalException)

        self.assertIsSubclass(ConversionSyntax, InvalidOperation)
        self.assertIsSubclass(DivisionImpossible, InvalidOperation)
        self.assertIsSubclass(DivisionUndefined, InvalidOperation)
        self.assertIsSubclass(DivisionUndefined, ZeroDivisionError)
        self.assertIsSubclass(InvalidContext, InvalidOperation)


class ContextAPItests:
    def test_none_args(self):
        Context = self.decimal.Context
        InvalidOperation = self.decimal.InvalidOperation
        DivisionByZero = self.decimal.DivisionByZero
        Overflow = self.decimal.Overflow

        c1 = Context()
        c2 = Context(prec=None, rounding=None, Emax=None, Emin=None,
                     capitals=None, clamp=None, flags=None, traps=None)
        for c in [c1, c2]:
            self.assertEqual(c.prec, 28)
            self.assertEqual(c.rounding, P.ROUND_HALF_EVEN)
            self.assertEqual(c.Emax, 999999)
            self.assertEqual(c.Emin, -999999)
            self.assertEqual(c.capitals, 1)
            self.assertEqual(c.clamp, 0)
            assert_signals(self, c, 'flags', [])
            assert_signals(self, c, 'traps', [InvalidOperation, DivisionByZero,
                                              Overflow])

    def test_pickle(self):

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            Context = self.decimal.Context

            savedecimal = sys.modules['decimal']

            # Round trip
            sys.modules['decimal'] = self.decimal
            c = Context()
            e = pickle.loads(pickle.dumps(c, proto))

            self.assertEqual(c.prec, e.prec)
            self.assertEqual(c.Emin, e.Emin)
            self.assertEqual(c.Emax, e.Emax)
            self.assertEqual(c.rounding, e.rounding)
            self.assertEqual(c.capitals, e.capitals)
            self.assertEqual(c.clamp, e.clamp)
            self.assertEqual(c.flags, e.flags)
            self.assertEqual(c.traps, e.traps)

            # Test interchangeability
            combinations = [(C, P), (P, C)] if C else [(P, P)]
            for dumper, loader in combinations:
                for ri, _ in enumerate(RoundingModes):
                    for fi, _ in enumerate(OrderedSignals[dumper]):
                        for ti, _ in enumerate(OrderedSignals[dumper]):

                            prec = random.randrange(1, 100)
                            emin = random.randrange(-100, 0)
                            emax = random.randrange(1, 100)
                            caps = random.randrange(2)
                            clamp = random.randrange(2)

                            # One module dumps
                            sys.modules['decimal'] = dumper
                            c = dumper.Context(
                                  prec=prec, Emin=emin, Emax=emax,
                                  rounding=RoundingModes[ri],
                                  capitals=caps, clamp=clamp,
                                  flags=OrderedSignals[dumper][:fi],
                                  traps=OrderedSignals[dumper][:ti]
                            )
                            s = pickle.dumps(c, proto)

                            # The other module loads
                            sys.modules['decimal'] = loader
                            d = pickle.loads(s)
                            self.assertIsInstance(d, loader.Context)

                            self.assertEqual(d.prec, prec)
                            self.assertEqual(d.Emin, emin)
                            self.assertEqual(d.Emax, emax)
                            self.assertEqual(d.rounding, RoundingModes[ri])
                            self.assertEqual(d.capitals, caps)
                            self.assertEqual(d.clamp, clamp)
                            assert_signals(self, d, 'flags', OrderedSignals[loader][:fi])
                            assert_signals(self, d, 'traps', OrderedSignals[loader][:ti])

            sys.modules['decimal'] = savedecimal

    def test_equality_with_other_types(self):
        Decimal = self.decimal.Decimal

        self.assertIn(Decimal(10), ['a', 1.0, Decimal(10), (1,2), {}])
        self.assertNotIn(Decimal(10), ['a', 1.0, (1,2), {}])

    def test_copy(self):
        # All copies should be deep
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.copy()
        self.assertNotEqual(id(c), id(d))
        self.assertNotEqual(id(c.flags), id(d.flags))
        self.assertNotEqual(id(c.traps), id(d.traps))
        k1 = set(c.flags.keys())
        k2 = set(d.flags.keys())
        self.assertEqual(k1, k2)
        self.assertEqual(c.flags, d.flags)

    def test__clamp(self):
        # In Python 3.2, the private attribute `_clamp` was made
        # public (issue 8540), with the old `_clamp` becoming a
        # property wrapping `clamp`.  For the duration of Python 3.2
        # only, the attribute should be gettable/settable via both
        # `clamp` and `_clamp`; in Python 3.3, `_clamp` should be
        # removed.
        Context = self.decimal.Context
        c = Context()
        self.assertRaises(AttributeError, getattr, c, '_clamp')

    def test_abs(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.abs(Decimal(-1))
        self.assertEqual(c.abs(-1), d)
        self.assertRaises(TypeError, c.abs, '-1')

    def test_add(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.add(Decimal(1), Decimal(1))
        self.assertEqual(c.add(1, 1), d)
        self.assertEqual(c.add(Decimal(1), 1), d)
        self.assertEqual(c.add(1, Decimal(1)), d)
        self.assertRaises(TypeError, c.add, '1', 1)
        self.assertRaises(TypeError, c.add, 1, '1')

    def test_compare(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.compare(Decimal(1), Decimal(1))
        self.assertEqual(c.compare(1, 1), d)
        self.assertEqual(c.compare(Decimal(1), 1), d)
        self.assertEqual(c.compare(1, Decimal(1)), d)
        self.assertRaises(TypeError, c.compare, '1', 1)
        self.assertRaises(TypeError, c.compare, 1, '1')

    def test_compare_signal(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.compare_signal(Decimal(1), Decimal(1))
        self.assertEqual(c.compare_signal(1, 1), d)
        self.assertEqual(c.compare_signal(Decimal(1), 1), d)
        self.assertEqual(c.compare_signal(1, Decimal(1)), d)
        self.assertRaises(TypeError, c.compare_signal, '1', 1)
        self.assertRaises(TypeError, c.compare_signal, 1, '1')

    def test_compare_total(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.compare_total(Decimal(1), Decimal(1))
        self.assertEqual(c.compare_total(1, 1), d)
        self.assertEqual(c.compare_total(Decimal(1), 1), d)
        self.assertEqual(c.compare_total(1, Decimal(1)), d)
        self.assertRaises(TypeError, c.compare_total, '1', 1)
        self.assertRaises(TypeError, c.compare_total, 1, '1')

    def test_compare_total_mag(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.compare_total_mag(Decimal(1), Decimal(1))
        self.assertEqual(c.compare_total_mag(1, 1), d)
        self.assertEqual(c.compare_total_mag(Decimal(1), 1), d)
        self.assertEqual(c.compare_total_mag(1, Decimal(1)), d)
        self.assertRaises(TypeError, c.compare_total_mag, '1', 1)
        self.assertRaises(TypeError, c.compare_total_mag, 1, '1')

    def test_copy_abs(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.copy_abs(Decimal(-1))
        self.assertEqual(c.copy_abs(-1), d)
        self.assertRaises(TypeError, c.copy_abs, '-1')

    def test_copy_decimal(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.copy_decimal(Decimal(-1))
        self.assertEqual(c.copy_decimal(-1), d)
        self.assertRaises(TypeError, c.copy_decimal, '-1')

    def test_copy_negate(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.copy_negate(Decimal(-1))
        self.assertEqual(c.copy_negate(-1), d)
        self.assertRaises(TypeError, c.copy_negate, '-1')

    def test_copy_sign(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.copy_sign(Decimal(1), Decimal(-2))
        self.assertEqual(c.copy_sign(1, -2), d)
        self.assertEqual(c.copy_sign(Decimal(1), -2), d)
        self.assertEqual(c.copy_sign(1, Decimal(-2)), d)
        self.assertRaises(TypeError, c.copy_sign, '1', -2)
        self.assertRaises(TypeError, c.copy_sign, 1, '-2')

    def test_divide(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.divide(Decimal(1), Decimal(2))
        self.assertEqual(c.divide(1, 2), d)
        self.assertEqual(c.divide(Decimal(1), 2), d)
        self.assertEqual(c.divide(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.divide, '1', 2)
        self.assertRaises(TypeError, c.divide, 1, '2')

    def test_divide_int(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.divide_int(Decimal(1), Decimal(2))
        self.assertEqual(c.divide_int(1, 2), d)
        self.assertEqual(c.divide_int(Decimal(1), 2), d)
        self.assertEqual(c.divide_int(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.divide_int, '1', 2)
        self.assertRaises(TypeError, c.divide_int, 1, '2')

    def test_divmod(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.divmod(Decimal(1), Decimal(2))
        self.assertEqual(c.divmod(1, 2), d)
        self.assertEqual(c.divmod(Decimal(1), 2), d)
        self.assertEqual(c.divmod(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.divmod, '1', 2)
        self.assertRaises(TypeError, c.divmod, 1, '2')

    def test_exp(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.exp(Decimal(10))
        self.assertEqual(c.exp(10), d)
        self.assertRaises(TypeError, c.exp, '10')

    def test_fma(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.fma(Decimal(2), Decimal(3), Decimal(4))
        self.assertEqual(c.fma(2, 3, 4), d)
        self.assertEqual(c.fma(Decimal(2), 3, 4), d)
        self.assertEqual(c.fma(2, Decimal(3), 4), d)
        self.assertEqual(c.fma(2, 3, Decimal(4)), d)
        self.assertEqual(c.fma(Decimal(2), Decimal(3), 4), d)
        self.assertRaises(TypeError, c.fma, '2', 3, 4)
        self.assertRaises(TypeError, c.fma, 2, '3', 4)
        self.assertRaises(TypeError, c.fma, 2, 3, '4')

        # Issue 12079 for Context.fma ...
        self.assertRaises(TypeError, c.fma,
                          Decimal('Infinity'), Decimal(0), "not a decimal")
        self.assertRaises(TypeError, c.fma,
                          Decimal(1), Decimal('snan'), 1.222)
        # ... and for Decimal.fma.
        self.assertRaises(TypeError, Decimal('Infinity').fma,
                          Decimal(0), "not a decimal")
        self.assertRaises(TypeError, Decimal(1).fma,
                          Decimal('snan'), 1.222)

    def test_is_finite(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.is_finite(Decimal(10))
        self.assertEqual(c.is_finite(10), d)
        self.assertRaises(TypeError, c.is_finite, '10')

    def test_is_infinite(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.is_infinite(Decimal(10))
        self.assertEqual(c.is_infinite(10), d)
        self.assertRaises(TypeError, c.is_infinite, '10')

    def test_is_nan(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.is_nan(Decimal(10))
        self.assertEqual(c.is_nan(10), d)
        self.assertRaises(TypeError, c.is_nan, '10')

    def test_is_normal(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.is_normal(Decimal(10))
        self.assertEqual(c.is_normal(10), d)
        self.assertRaises(TypeError, c.is_normal, '10')

    def test_is_qnan(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.is_qnan(Decimal(10))
        self.assertEqual(c.is_qnan(10), d)
        self.assertRaises(TypeError, c.is_qnan, '10')

    def test_is_signed(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.is_signed(Decimal(10))
        self.assertEqual(c.is_signed(10), d)
        self.assertRaises(TypeError, c.is_signed, '10')

    def test_is_snan(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.is_snan(Decimal(10))
        self.assertEqual(c.is_snan(10), d)
        self.assertRaises(TypeError, c.is_snan, '10')

    def test_is_subnormal(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.is_subnormal(Decimal(10))
        self.assertEqual(c.is_subnormal(10), d)
        self.assertRaises(TypeError, c.is_subnormal, '10')

    def test_is_zero(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.is_zero(Decimal(10))
        self.assertEqual(c.is_zero(10), d)
        self.assertRaises(TypeError, c.is_zero, '10')

    def test_ln(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.ln(Decimal(10))
        self.assertEqual(c.ln(10), d)
        self.assertRaises(TypeError, c.ln, '10')

    def test_log10(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.log10(Decimal(10))
        self.assertEqual(c.log10(10), d)
        self.assertRaises(TypeError, c.log10, '10')

    def test_logb(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.logb(Decimal(10))
        self.assertEqual(c.logb(10), d)
        self.assertRaises(TypeError, c.logb, '10')

    def test_logical_and(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.logical_and(Decimal(1), Decimal(1))
        self.assertEqual(c.logical_and(1, 1), d)
        self.assertEqual(c.logical_and(Decimal(1), 1), d)
        self.assertEqual(c.logical_and(1, Decimal(1)), d)
        self.assertRaises(TypeError, c.logical_and, '1', 1)
        self.assertRaises(TypeError, c.logical_and, 1, '1')

    def test_logical_invert(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.logical_invert(Decimal(1000))
        self.assertEqual(c.logical_invert(1000), d)
        self.assertRaises(TypeError, c.logical_invert, '1000')

    def test_logical_or(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.logical_or(Decimal(1), Decimal(1))
        self.assertEqual(c.logical_or(1, 1), d)
        self.assertEqual(c.logical_or(Decimal(1), 1), d)
        self.assertEqual(c.logical_or(1, Decimal(1)), d)
        self.assertRaises(TypeError, c.logical_or, '1', 1)
        self.assertRaises(TypeError, c.logical_or, 1, '1')

    def test_logical_xor(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.logical_xor(Decimal(1), Decimal(1))
        self.assertEqual(c.logical_xor(1, 1), d)
        self.assertEqual(c.logical_xor(Decimal(1), 1), d)
        self.assertEqual(c.logical_xor(1, Decimal(1)), d)
        self.assertRaises(TypeError, c.logical_xor, '1', 1)
        self.assertRaises(TypeError, c.logical_xor, 1, '1')

    def test_max(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.max(Decimal(1), Decimal(2))
        self.assertEqual(c.max(1, 2), d)
        self.assertEqual(c.max(Decimal(1), 2), d)
        self.assertEqual(c.max(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.max, '1', 2)
        self.assertRaises(TypeError, c.max, 1, '2')

    def test_max_mag(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.max_mag(Decimal(1), Decimal(2))
        self.assertEqual(c.max_mag(1, 2), d)
        self.assertEqual(c.max_mag(Decimal(1), 2), d)
        self.assertEqual(c.max_mag(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.max_mag, '1', 2)
        self.assertRaises(TypeError, c.max_mag, 1, '2')

    def test_min(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.min(Decimal(1), Decimal(2))
        self.assertEqual(c.min(1, 2), d)
        self.assertEqual(c.min(Decimal(1), 2), d)
        self.assertEqual(c.min(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.min, '1', 2)
        self.assertRaises(TypeError, c.min, 1, '2')

    def test_min_mag(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.min_mag(Decimal(1), Decimal(2))
        self.assertEqual(c.min_mag(1, 2), d)
        self.assertEqual(c.min_mag(Decimal(1), 2), d)
        self.assertEqual(c.min_mag(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.min_mag, '1', 2)
        self.assertRaises(TypeError, c.min_mag, 1, '2')

    def test_minus(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.minus(Decimal(10))
        self.assertEqual(c.minus(10), d)
        self.assertRaises(TypeError, c.minus, '10')

    def test_multiply(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.multiply(Decimal(1), Decimal(2))
        self.assertEqual(c.multiply(1, 2), d)
        self.assertEqual(c.multiply(Decimal(1), 2), d)
        self.assertEqual(c.multiply(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.multiply, '1', 2)
        self.assertRaises(TypeError, c.multiply, 1, '2')

    def test_next_minus(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.next_minus(Decimal(10))
        self.assertEqual(c.next_minus(10), d)
        self.assertRaises(TypeError, c.next_minus, '10')

    def test_next_plus(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.next_plus(Decimal(10))
        self.assertEqual(c.next_plus(10), d)
        self.assertRaises(TypeError, c.next_plus, '10')

    def test_next_toward(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.next_toward(Decimal(1), Decimal(2))
        self.assertEqual(c.next_toward(1, 2), d)
        self.assertEqual(c.next_toward(Decimal(1), 2), d)
        self.assertEqual(c.next_toward(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.next_toward, '1', 2)
        self.assertRaises(TypeError, c.next_toward, 1, '2')

    def test_normalize(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.normalize(Decimal(10))
        self.assertEqual(c.normalize(10), d)
        self.assertRaises(TypeError, c.normalize, '10')

    def test_number_class(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        self.assertEqual(c.number_class(123), c.number_class(Decimal(123)))
        self.assertEqual(c.number_class(0), c.number_class(Decimal(0)))
        self.assertEqual(c.number_class(-45), c.number_class(Decimal(-45)))

    def test_plus(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.plus(Decimal(10))
        self.assertEqual(c.plus(10), d)
        self.assertRaises(TypeError, c.plus, '10')

    def test_power(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.power(Decimal(1), Decimal(4))
        self.assertEqual(c.power(1, 4), d)
        self.assertEqual(c.power(Decimal(1), 4), d)
        self.assertEqual(c.power(1, Decimal(4)), d)
        self.assertEqual(c.power(Decimal(1), Decimal(4)), d)
        self.assertRaises(TypeError, c.power, '1', 4)
        self.assertRaises(TypeError, c.power, 1, '4')
        self.assertEqual(c.power(modulo=5, b=8, a=2), 1)

    def test_quantize(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.quantize(Decimal(1), Decimal(2))
        self.assertEqual(c.quantize(1, 2), d)
        self.assertEqual(c.quantize(Decimal(1), 2), d)
        self.assertEqual(c.quantize(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.quantize, '1', 2)
        self.assertRaises(TypeError, c.quantize, 1, '2')

    def test_remainder(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.remainder(Decimal(1), Decimal(2))
        self.assertEqual(c.remainder(1, 2), d)
        self.assertEqual(c.remainder(Decimal(1), 2), d)
        self.assertEqual(c.remainder(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.remainder, '1', 2)
        self.assertRaises(TypeError, c.remainder, 1, '2')

    def test_remainder_near(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.remainder_near(Decimal(1), Decimal(2))
        self.assertEqual(c.remainder_near(1, 2), d)
        self.assertEqual(c.remainder_near(Decimal(1), 2), d)
        self.assertEqual(c.remainder_near(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.remainder_near, '1', 2)
        self.assertRaises(TypeError, c.remainder_near, 1, '2')

    def test_rotate(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.rotate(Decimal(1), Decimal(2))
        self.assertEqual(c.rotate(1, 2), d)
        self.assertEqual(c.rotate(Decimal(1), 2), d)
        self.assertEqual(c.rotate(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.rotate, '1', 2)
        self.assertRaises(TypeError, c.rotate, 1, '2')

    def test_sqrt(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.sqrt(Decimal(10))
        self.assertEqual(c.sqrt(10), d)
        self.assertRaises(TypeError, c.sqrt, '10')

    def test_same_quantum(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.same_quantum(Decimal(1), Decimal(2))
        self.assertEqual(c.same_quantum(1, 2), d)
        self.assertEqual(c.same_quantum(Decimal(1), 2), d)
        self.assertEqual(c.same_quantum(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.same_quantum, '1', 2)
        self.assertRaises(TypeError, c.same_quantum, 1, '2')

    def test_scaleb(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.scaleb(Decimal(1), Decimal(2))
        self.assertEqual(c.scaleb(1, 2), d)
        self.assertEqual(c.scaleb(Decimal(1), 2), d)
        self.assertEqual(c.scaleb(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.scaleb, '1', 2)
        self.assertRaises(TypeError, c.scaleb, 1, '2')

    def test_shift(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.shift(Decimal(1), Decimal(2))
        self.assertEqual(c.shift(1, 2), d)
        self.assertEqual(c.shift(Decimal(1), 2), d)
        self.assertEqual(c.shift(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.shift, '1', 2)
        self.assertRaises(TypeError, c.shift, 1, '2')

    def test_subtract(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.subtract(Decimal(1), Decimal(2))
        self.assertEqual(c.subtract(1, 2), d)
        self.assertEqual(c.subtract(Decimal(1), 2), d)
        self.assertEqual(c.subtract(1, Decimal(2)), d)
        self.assertRaises(TypeError, c.subtract, '1', 2)
        self.assertRaises(TypeError, c.subtract, 1, '2')

    def test_to_eng_string(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.to_eng_string(Decimal(10))
        self.assertEqual(c.to_eng_string(10), d)
        self.assertRaises(TypeError, c.to_eng_string, '10')

    def test_to_sci_string(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.to_sci_string(Decimal(10))
        self.assertEqual(c.to_sci_string(10), d)
        self.assertRaises(TypeError, c.to_sci_string, '10')

    def test_to_integral_exact(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.to_integral_exact(Decimal(10))
        self.assertEqual(c.to_integral_exact(10), d)
        self.assertRaises(TypeError, c.to_integral_exact, '10')

    def test_to_integral_value(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context

        c = Context()
        d = c.to_integral_value(Decimal(10))
        self.assertEqual(c.to_integral_value(10), d)
        self.assertRaises(TypeError, c.to_integral_value, '10')
        self.assertRaises(TypeError, c.to_integral_value, 10, 'x')


def load_tests(loader, tests, pattern):
    base_classes = [PythonAPItests, ContextAPItests]
    return load_tests_for_base_classes(loader, tests, base_classes)


if __name__ == '__main__':
    unittest.main()
