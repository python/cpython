import unittest
import sys
from test.support import check_disallow_instantiation
import random
from . import (C, P, requires_cdecimal, requires_extra_functionality,
               RoundingModes, OrderedSignals,
               setUpModule, tearDownModule)


class CFunctionality(unittest.TestCase):
    """Extra functionality in _decimal"""

    @requires_extra_functionality
    def test_c_context(self):
        Context = C.Context

        c = Context(flags=C.DecClamped, traps=C.DecRounded)
        self.assertEqual(c._flags, C.DecClamped)
        self.assertEqual(c._traps, C.DecRounded)

    @requires_extra_functionality
    def test_constants(self):
        # Condition flags
        cond = (
            C.DecClamped, C.DecConversionSyntax, C.DecDivisionByZero,
            C.DecDivisionImpossible, C.DecDivisionUndefined,
            C.DecFpuError, C.DecInexact, C.DecInvalidContext,
            C.DecInvalidOperation, C.DecMallocError,
            C.DecFloatOperation, C.DecOverflow, C.DecRounded,
            C.DecSubnormal, C.DecUnderflow
        )

        # Conditions
        for i, v in enumerate(cond):
            self.assertEqual(v, 1<<i)

        self.assertEqual(C.DecIEEEInvalidOperation,
                         C.DecConversionSyntax|
                         C.DecDivisionImpossible|
                         C.DecDivisionUndefined|
                         C.DecFpuError|
                         C.DecInvalidContext|
                         C.DecInvalidOperation|
                         C.DecMallocError)

        self.assertEqual(C.DecErrors,
                         C.DecIEEEInvalidOperation|
                         C.DecDivisionByZero)

        self.assertEqual(C.DecTraps,
                         C.DecErrors|C.DecOverflow|C.DecUnderflow)

@requires_cdecimal
class CWhitebox(unittest.TestCase):
    """Whitebox testing for _decimal"""

    def test_bignum(self):
        # Not exactly whitebox, but too slow with pydecimal.

        Decimal = C.Decimal
        localcontext = C.localcontext

        b1 = 10**35
        b2 = 10**36
        with localcontext() as c:
            c.prec = 1000000
            for i in range(5):
                a = random.randrange(b1, b2)
                b = random.randrange(1000, 1200)
                x = a ** b
                y = Decimal(a) ** Decimal(b)
                self.assertEqual(x, y)

    def test_invalid_construction(self):
        self.assertRaises(TypeError, C.Decimal, 9, "xyz")

    def test_c_input_restriction(self):
        # Too large for _decimal to be converted exactly
        Decimal = C.Decimal
        InvalidOperation = C.InvalidOperation
        Context = C.Context
        localcontext = C.localcontext

        with localcontext(Context()):
            self.assertRaises(InvalidOperation, Decimal,
                              "1e9999999999999999999")

    def test_c_context_repr(self):
        # This test is _decimal-only because flags are not printed
        # in the same order.
        DefaultContext = C.DefaultContext
        FloatOperation = C.FloatOperation

        c = DefaultContext.copy()

        c.prec = 425000000
        c.Emax = 425000000
        c.Emin = -425000000
        c.rounding = P.ROUND_HALF_DOWN
        c.capitals = 0
        c.clamp = 1
        for sig in OrderedSignals[C]:
            c.flags[sig] = True
            c.traps[sig] = True
        c.flags[FloatOperation] = True
        c.traps[FloatOperation] = True

        s = c.__repr__()
        t = "Context(prec=425000000, rounding=ROUND_HALF_DOWN, " \
            "Emin=-425000000, Emax=425000000, capitals=0, clamp=1, " \
            "flags=[Clamped, InvalidOperation, DivisionByZero, Inexact, " \
                   "FloatOperation, Overflow, Rounded, Subnormal, Underflow], " \
            "traps=[Clamped, InvalidOperation, DivisionByZero, Inexact, " \
                   "FloatOperation, Overflow, Rounded, Subnormal, Underflow])"
        self.assertEqual(s, t)

    def test_c_context_errors(self):
        Context = C.Context
        InvalidOperation = C.InvalidOperation
        Overflow = C.Overflow
        FloatOperation = C.FloatOperation
        localcontext = C.localcontext
        getcontext = C.getcontext
        setcontext = C.setcontext
        HAVE_CONFIG_64 = (C.MAX_PREC > 425000000)

        c = Context()

        # SignalDict: input validation
        self.assertRaises(KeyError, c.flags.__setitem__, 801, 0)
        self.assertRaises(KeyError, c.traps.__setitem__, 801, 0)
        self.assertRaises(ValueError, c.flags.__delitem__, Overflow)
        self.assertRaises(ValueError, c.traps.__delitem__, InvalidOperation)
        self.assertRaises(TypeError, setattr, c, 'flags', ['x'])
        self.assertRaises(TypeError, setattr, c,'traps', ['y'])
        self.assertRaises(KeyError, setattr, c, 'flags', {0:1})
        self.assertRaises(KeyError, setattr, c, 'traps', {0:1})

        # Test assignment from a signal dict with the correct length but
        # one invalid key.
        d = c.flags.copy()
        del d[FloatOperation]
        d["XYZ"] = 91283719
        self.assertRaises(KeyError, setattr, c, 'flags', d)
        self.assertRaises(KeyError, setattr, c, 'traps', d)

        # Input corner cases
        int_max = 2**63-1 if HAVE_CONFIG_64 else 2**31-1
        gt_max_emax = 10**18 if HAVE_CONFIG_64 else 10**9

        # prec, Emax, Emin
        for attr in ['prec', 'Emax']:
            self.assertRaises(ValueError, setattr, c, attr, gt_max_emax)
        self.assertRaises(ValueError, setattr, c, 'Emin', -gt_max_emax)

        # prec, Emax, Emin in context constructor
        self.assertRaises(ValueError, Context, prec=gt_max_emax)
        self.assertRaises(ValueError, Context, Emax=gt_max_emax)
        self.assertRaises(ValueError, Context, Emin=-gt_max_emax)

        # Overflow in conversion
        self.assertRaises(OverflowError, Context, prec=int_max+1)
        self.assertRaises(OverflowError, Context, Emax=int_max+1)
        self.assertRaises(OverflowError, Context, Emin=-int_max-2)
        self.assertRaises(OverflowError, Context, clamp=int_max+1)
        self.assertRaises(OverflowError, Context, capitals=int_max+1)

        # OverflowError, general ValueError
        for attr in ('prec', 'Emin', 'Emax', 'capitals', 'clamp'):
            self.assertRaises(OverflowError, setattr, c, attr, int_max+1)
            self.assertRaises(OverflowError, setattr, c, attr, -int_max-2)
            if sys.platform != 'win32':
                self.assertRaises(ValueError, setattr, c, attr, int_max)
                self.assertRaises(ValueError, setattr, c, attr, -int_max-1)

        # OverflowError: _unsafe_setprec, _unsafe_setemin, _unsafe_setemax
        if C.MAX_PREC == 425000000:
            self.assertRaises(OverflowError, getattr(c, '_unsafe_setprec'),
                              int_max+1)
            self.assertRaises(OverflowError, getattr(c, '_unsafe_setemax'),
                              int_max+1)
            self.assertRaises(OverflowError, getattr(c, '_unsafe_setemin'),
                              -int_max-2)

        # ValueError: _unsafe_setprec, _unsafe_setemin, _unsafe_setemax
        if C.MAX_PREC == 425000000:
            self.assertRaises(ValueError, getattr(c, '_unsafe_setprec'), 0)
            self.assertRaises(ValueError, getattr(c, '_unsafe_setprec'),
                              1070000001)
            self.assertRaises(ValueError, getattr(c, '_unsafe_setemax'), -1)
            self.assertRaises(ValueError, getattr(c, '_unsafe_setemax'),
                              1070000001)
            self.assertRaises(ValueError, getattr(c, '_unsafe_setemin'),
                              -1070000001)
            self.assertRaises(ValueError, getattr(c, '_unsafe_setemin'), 1)

        # capitals, clamp
        for attr in ['capitals', 'clamp']:
            self.assertRaises(ValueError, setattr, c, attr, -1)
            self.assertRaises(ValueError, setattr, c, attr, 2)
            self.assertRaises(TypeError, setattr, c, attr, [1,2,3])
            if HAVE_CONFIG_64:
                self.assertRaises(ValueError, setattr, c, attr, 2**32)
                self.assertRaises(ValueError, setattr, c, attr, 2**32+1)

        # Invalid local context
        self.assertRaises(TypeError, exec, 'with localcontext("xyz"): pass',
                          locals())
        self.assertRaises(TypeError, exec,
                          'with localcontext(context=getcontext()): pass',
                          locals())

        # setcontext
        saved_context = getcontext()
        self.assertRaises(TypeError, setcontext, "xyz")
        setcontext(saved_context)

    def test_rounding_strings_interned(self):
        self.assertIs(C.ROUND_UP, P.ROUND_UP)
        self.assertIs(C.ROUND_DOWN, P.ROUND_DOWN)
        self.assertIs(C.ROUND_CEILING, P.ROUND_CEILING)
        self.assertIs(C.ROUND_FLOOR, P.ROUND_FLOOR)
        self.assertIs(C.ROUND_HALF_UP, P.ROUND_HALF_UP)
        self.assertIs(C.ROUND_HALF_DOWN, P.ROUND_HALF_DOWN)
        self.assertIs(C.ROUND_HALF_EVEN, P.ROUND_HALF_EVEN)
        self.assertIs(C.ROUND_05UP, P.ROUND_05UP)

    @requires_extra_functionality
    def test_c_context_errors_extra(self):
        Context = C.Context
        InvalidOperation = C.InvalidOperation
        Overflow = C.Overflow
        localcontext = C.localcontext
        getcontext = C.getcontext
        setcontext = C.setcontext
        HAVE_CONFIG_64 = (C.MAX_PREC > 425000000)

        c = Context()

        # Input corner cases
        int_max = 2**63-1 if HAVE_CONFIG_64 else 2**31-1

        # OverflowError, general ValueError
        self.assertRaises(OverflowError, setattr, c, '_allcr', int_max+1)
        self.assertRaises(OverflowError, setattr, c, '_allcr', -int_max-2)
        if sys.platform != 'win32':
            self.assertRaises(ValueError, setattr, c, '_allcr', int_max)
            self.assertRaises(ValueError, setattr, c, '_allcr', -int_max-1)

        # OverflowError, general TypeError
        for attr in ('_flags', '_traps'):
            self.assertRaises(OverflowError, setattr, c, attr, int_max+1)
            self.assertRaises(OverflowError, setattr, c, attr, -int_max-2)
            if sys.platform != 'win32':
                self.assertRaises(TypeError, setattr, c, attr, int_max)
                self.assertRaises(TypeError, setattr, c, attr, -int_max-1)

        # _allcr
        self.assertRaises(ValueError, setattr, c, '_allcr', -1)
        self.assertRaises(ValueError, setattr, c, '_allcr', 2)
        self.assertRaises(TypeError, setattr, c, '_allcr', [1,2,3])
        if HAVE_CONFIG_64:
            self.assertRaises(ValueError, setattr, c, '_allcr', 2**32)
            self.assertRaises(ValueError, setattr, c, '_allcr', 2**32+1)

        # _flags, _traps
        for attr in ['_flags', '_traps']:
            self.assertRaises(TypeError, setattr, c, attr, 999999)
            self.assertRaises(TypeError, setattr, c, attr, 'x')

    def test_c_valid_context(self):
        # These tests are for code coverage in _decimal.
        DefaultContext = C.DefaultContext
        Clamped = C.Clamped
        Underflow = C.Underflow
        Inexact = C.Inexact
        Rounded = C.Rounded
        Subnormal = C.Subnormal

        c = DefaultContext.copy()

        # Exercise all getters and setters
        c.prec = 34
        c.rounding = P.ROUND_HALF_UP
        c.Emax = 3000
        c.Emin = -3000
        c.capitals = 1
        c.clamp = 0

        self.assertEqual(c.prec, 34)
        self.assertEqual(c.rounding, P.ROUND_HALF_UP)
        self.assertEqual(c.Emin, -3000)
        self.assertEqual(c.Emax, 3000)
        self.assertEqual(c.capitals, 1)
        self.assertEqual(c.clamp, 0)

        self.assertEqual(c.Etiny(), -3033)
        self.assertEqual(c.Etop(), 2967)

        # Exercise all unsafe setters
        if C.MAX_PREC == 425000000:
            c._unsafe_setprec(999999999)
            c._unsafe_setemax(999999999)
            c._unsafe_setemin(-999999999)
            self.assertEqual(c.prec, 999999999)
            self.assertEqual(c.Emax, 999999999)
            self.assertEqual(c.Emin, -999999999)

    @requires_extra_functionality
    def test_c_valid_context_extra(self):
        DefaultContext = C.DefaultContext

        c = DefaultContext.copy()
        self.assertEqual(c._allcr, 1)
        c._allcr = 0
        self.assertEqual(c._allcr, 0)

    def test_c_round(self):
        # Restricted input.
        Decimal = C.Decimal
        InvalidOperation = C.InvalidOperation
        localcontext = C.localcontext
        MAX_EMAX = C.MAX_EMAX
        MIN_ETINY = C.MIN_ETINY
        int_max = 2**63-1 if C.MAX_PREC > 425000000 else 2**31-1

        with localcontext() as c:
            c.traps[InvalidOperation] = True
            self.assertRaises(InvalidOperation, Decimal("1.23").__round__,
                              -int_max-1)
            self.assertRaises(InvalidOperation, Decimal("1.23").__round__,
                              int_max)
            self.assertRaises(InvalidOperation, Decimal("1").__round__,
                              int(MAX_EMAX+1))
            self.assertRaises(C.InvalidOperation, Decimal("1").__round__,
                              -int(MIN_ETINY-1))
            self.assertRaises(OverflowError, Decimal("1.23").__round__,
                              -int_max-2)
            self.assertRaises(OverflowError, Decimal("1.23").__round__,
                              int_max+1)

    def test_c_format(self):
        # Restricted input
        Decimal = C.Decimal
        HAVE_CONFIG_64 = (C.MAX_PREC > 425000000)

        self.assertRaises(TypeError, Decimal(1).__format__, "=10.10", [], 9)
        self.assertRaises(TypeError, Decimal(1).__format__, "=10.10", 9)
        self.assertRaises(TypeError, Decimal(1).__format__, [])

        self.assertRaises(ValueError, Decimal(1).__format__, "<>=10.10")
        maxsize = 2**63-1 if HAVE_CONFIG_64 else 2**31-1
        self.assertRaises(ValueError, Decimal("1.23456789").__format__,
                          "=%d.1" % maxsize)

    def test_c_integral(self):
        Decimal = C.Decimal
        Inexact = C.Inexact
        localcontext = C.localcontext

        x = Decimal(10)
        self.assertEqual(x.to_integral(), 10)
        self.assertRaises(TypeError, x.to_integral, '10')
        self.assertRaises(TypeError, x.to_integral, 10, 'x')
        self.assertRaises(TypeError, x.to_integral, 10)

        self.assertEqual(x.to_integral_value(), 10)
        self.assertRaises(TypeError, x.to_integral_value, '10')
        self.assertRaises(TypeError, x.to_integral_value, 10, 'x')
        self.assertRaises(TypeError, x.to_integral_value, 10)

        self.assertEqual(x.to_integral_exact(), 10)
        self.assertRaises(TypeError, x.to_integral_exact, '10')
        self.assertRaises(TypeError, x.to_integral_exact, 10, 'x')
        self.assertRaises(TypeError, x.to_integral_exact, 10)

        with localcontext() as c:
            x = Decimal("99999999999999999999999999.9").to_integral_value(P.ROUND_UP)
            self.assertEqual(x, Decimal('100000000000000000000000000'))

            x = Decimal("99999999999999999999999999.9").to_integral_exact(P.ROUND_UP)
            self.assertEqual(x, Decimal('100000000000000000000000000'))

            c.traps[Inexact] = True
            self.assertRaises(Inexact, Decimal("999.9").to_integral_exact, P.ROUND_UP)

    def test_c_funcs(self):
        # Invalid arguments
        Decimal = C.Decimal
        InvalidOperation = C.InvalidOperation
        DivisionByZero = C.DivisionByZero
        getcontext = C.getcontext
        localcontext = C.localcontext

        self.assertEqual(Decimal('9.99e10').to_eng_string(), '99.9E+9')

        self.assertRaises(TypeError, pow, Decimal(1), 2, "3")
        self.assertRaises(TypeError, Decimal(9).number_class, "x", "y")
        self.assertRaises(TypeError, Decimal(9).same_quantum, 3, "x", "y")

        self.assertRaises(
            TypeError,
            Decimal("1.23456789").quantize, Decimal('1e-100000'), []
        )
        self.assertRaises(
            TypeError,
            Decimal("1.23456789").quantize, Decimal('1e-100000'), getcontext()
        )
        self.assertRaises(
            TypeError,
            Decimal("1.23456789").quantize, Decimal('1e-100000'), 10
        )
        self.assertRaises(
            TypeError,
            Decimal("1.23456789").quantize, Decimal('1e-100000'), P.ROUND_UP, 1000
        )

        with localcontext() as c:
            c.clear_traps()

            # Invalid arguments
            self.assertRaises(TypeError, c.copy_sign, Decimal(1), "x", "y")
            self.assertRaises(TypeError, c.canonical, 200)
            self.assertRaises(TypeError, c.is_canonical, 200)
            self.assertRaises(TypeError, c.divmod, 9, 8, "x", "y")
            self.assertRaises(TypeError, c.same_quantum, 9, 3, "x", "y")

            self.assertEqual(str(c.canonical(Decimal(200))), '200')
            self.assertEqual(c.radix(), 10)

            c.traps[DivisionByZero] = True
            self.assertRaises(DivisionByZero, Decimal(9).__divmod__, 0)
            self.assertRaises(DivisionByZero, c.divmod, 9, 0)
            self.assertTrue(c.flags[InvalidOperation])

            c.clear_flags()
            c.traps[InvalidOperation] = True
            self.assertRaises(InvalidOperation, Decimal(9).__divmod__, 0)
            self.assertRaises(InvalidOperation, c.divmod, 9, 0)
            self.assertTrue(c.flags[DivisionByZero])

            c.traps[InvalidOperation] = True
            c.prec = 2
            self.assertRaises(InvalidOperation, pow, Decimal(1000), 1, 501)

    def test_va_args_exceptions(self):
        Decimal = C.Decimal
        Context = C.Context

        x = Decimal("10001111111")

        for attr in ['exp', 'is_normal', 'is_subnormal', 'ln', 'log10',
                     'logb', 'logical_invert', 'next_minus', 'next_plus',
                     'normalize', 'number_class', 'sqrt', 'to_eng_string']:
            func = getattr(x, attr)
            self.assertRaises(TypeError, func, context="x")
            self.assertRaises(TypeError, func, "x", context=None)

        for attr in ['compare', 'compare_signal', 'logical_and',
                     'logical_or', 'max', 'max_mag', 'min', 'min_mag',
                     'remainder_near', 'rotate', 'scaleb', 'shift']:
            func = getattr(x, attr)
            self.assertRaises(TypeError, func, context="x")
            self.assertRaises(TypeError, func, "x", context=None)

        self.assertRaises(TypeError, x.to_integral, rounding=None, context=[])
        self.assertRaises(TypeError, x.to_integral, rounding={}, context=[])
        self.assertRaises(TypeError, x.to_integral, [], [])

        self.assertRaises(TypeError, x.to_integral_value, rounding=None, context=[])
        self.assertRaises(TypeError, x.to_integral_value, rounding={}, context=[])
        self.assertRaises(TypeError, x.to_integral_value, [], [])

        self.assertRaises(TypeError, x.to_integral_exact, rounding=None, context=[])
        self.assertRaises(TypeError, x.to_integral_exact, rounding={}, context=[])
        self.assertRaises(TypeError, x.to_integral_exact, [], [])

        self.assertRaises(TypeError, x.fma, 1, 2, context="x")
        self.assertRaises(TypeError, x.fma, 1, 2, "x", context=None)

        self.assertRaises(TypeError, x.quantize, 1, [], context=None)
        self.assertRaises(TypeError, x.quantize, 1, [], rounding=None)
        self.assertRaises(TypeError, x.quantize, 1, [], [])

        c = Context()
        self.assertRaises(TypeError, c.power, 1, 2, mod="x")
        self.assertRaises(TypeError, c.power, 1, "x", mod=None)
        self.assertRaises(TypeError, c.power, "x", 2, mod=None)

    @requires_extra_functionality
    def test_c_context_templates(self):
        self.assertEqual(
            C.BasicContext._traps,
            C.DecIEEEInvalidOperation|C.DecDivisionByZero|C.DecOverflow|
            C.DecUnderflow|C.DecClamped
        )
        self.assertEqual(
            C.DefaultContext._traps,
            C.DecIEEEInvalidOperation|C.DecDivisionByZero|C.DecOverflow
        )

    @requires_extra_functionality
    def test_c_signal_dict(self):

        # SignalDict coverage
        Context = C.Context
        DefaultContext = C.DefaultContext

        InvalidOperation = C.InvalidOperation
        FloatOperation = C.FloatOperation
        DivisionByZero = C.DivisionByZero
        Overflow = C.Overflow
        Subnormal = C.Subnormal
        Underflow = C.Underflow
        Rounded = C.Rounded
        Inexact = C.Inexact
        Clamped = C.Clamped

        DecClamped = C.DecClamped
        DecInvalidOperation = C.DecInvalidOperation
        DecIEEEInvalidOperation = C.DecIEEEInvalidOperation

        def assertIsExclusivelySet(signal, signal_dict):
            for sig in signal_dict:
                if sig == signal:
                    self.assertTrue(signal_dict[sig])
                else:
                    self.assertFalse(signal_dict[sig])

        c = DefaultContext.copy()

        # Signal dict methods
        self.assertTrue(Overflow in c.traps)
        c.clear_traps()
        for k in c.traps.keys():
            c.traps[k] = True
        for v in c.traps.values():
            self.assertTrue(v)
        c.clear_traps()
        for k, v in c.traps.items():
            self.assertFalse(v)

        self.assertFalse(c.flags.get(Overflow))
        self.assertIs(c.flags.get("x"), None)
        self.assertEqual(c.flags.get("x", "y"), "y")
        self.assertRaises(TypeError, c.flags.get, "x", "y", "z")

        self.assertEqual(len(c.flags), len(c.traps))
        s = sys.getsizeof(c.flags)
        s = sys.getsizeof(c.traps)
        s = c.flags.__repr__()

        # Set flags/traps.
        c.clear_flags()
        c._flags = DecClamped
        self.assertTrue(c.flags[Clamped])

        c.clear_traps()
        c._traps = DecInvalidOperation
        self.assertTrue(c.traps[InvalidOperation])

        # Set flags/traps from dictionary.
        c.clear_flags()
        d = c.flags.copy()
        d[DivisionByZero] = True
        c.flags = d
        assertIsExclusivelySet(DivisionByZero, c.flags)

        c.clear_traps()
        d = c.traps.copy()
        d[Underflow] = True
        c.traps = d
        assertIsExclusivelySet(Underflow, c.traps)

        # Random constructors
        IntSignals = {
          Clamped: C.DecClamped,
          Rounded: C.DecRounded,
          Inexact: C.DecInexact,
          Subnormal: C.DecSubnormal,
          Underflow: C.DecUnderflow,
          Overflow: C.DecOverflow,
          DivisionByZero: C.DecDivisionByZero,
          FloatOperation: C.DecFloatOperation,
          InvalidOperation: C.DecIEEEInvalidOperation
        }
        IntCond = [
          C.DecDivisionImpossible, C.DecDivisionUndefined, C.DecFpuError,
          C.DecInvalidContext, C.DecInvalidOperation, C.DecMallocError,
          C.DecConversionSyntax,
        ]

        lim = len(OrderedSignals[C])
        for r in range(lim):
            for t in range(lim):
                for round in RoundingModes:
                    flags = random.sample(OrderedSignals[C], r)
                    traps = random.sample(OrderedSignals[C], t)
                    prec = random.randrange(1, 10000)
                    emin = random.randrange(-10000, 0)
                    emax = random.randrange(0, 10000)
                    clamp = random.randrange(0, 2)
                    caps = random.randrange(0, 2)
                    cr = random.randrange(0, 2)
                    c = Context(prec=prec, rounding=round, Emin=emin, Emax=emax,
                                capitals=caps, clamp=clamp, flags=list(flags),
                                traps=list(traps))

                    self.assertEqual(c.prec, prec)
                    self.assertEqual(c.rounding, round)
                    self.assertEqual(c.Emin, emin)
                    self.assertEqual(c.Emax, emax)
                    self.assertEqual(c.capitals, caps)
                    self.assertEqual(c.clamp, clamp)

                    f = 0
                    for x in flags:
                        f |= IntSignals[x]
                    self.assertEqual(c._flags, f)

                    f = 0
                    for x in traps:
                        f |= IntSignals[x]
                    self.assertEqual(c._traps, f)

        for cond in IntCond:
            c._flags = cond
            self.assertTrue(c._flags&DecIEEEInvalidOperation)
            assertIsExclusivelySet(InvalidOperation, c.flags)

        for cond in IntCond:
            c._traps = cond
            self.assertTrue(c._traps&DecIEEEInvalidOperation)
            assertIsExclusivelySet(InvalidOperation, c.traps)

    def test_invalid_override(self):
        Decimal = C.Decimal

        try:
            from locale import CHAR_MAX
        except ImportError:
            self.skipTest('locale.CHAR_MAX not available')

        def make_grouping(lst):
            return ''.join([chr(x) for x in lst])

        def get_fmt(x, override=None, fmt='n'):
            return Decimal(x).__format__(fmt, override)

        invalid_grouping = {
            'decimal_point' : ',',
            'grouping' : make_grouping([255, 255, 0]),
            'thousands_sep' : ','
        }
        invalid_dot = {
            'decimal_point' : 'xxxxx',
            'grouping' : make_grouping([3, 3, 0]),
            'thousands_sep' : ','
        }
        invalid_sep = {
            'decimal_point' : '.',
            'grouping' : make_grouping([3, 3, 0]),
            'thousands_sep' : 'yyyyy'
        }

        if CHAR_MAX == 127: # negative grouping in override
            self.assertRaises(ValueError, get_fmt, 12345,
                              invalid_grouping, 'g')

        self.assertRaises(ValueError, get_fmt, 12345, invalid_dot, 'g')
        self.assertRaises(ValueError, get_fmt, 12345, invalid_sep, 'g')

    def test_exact_conversion(self):
        Decimal = C.Decimal
        localcontext = C.localcontext
        InvalidOperation = C.InvalidOperation

        with localcontext() as c:

            c.traps[InvalidOperation] = True

            # Clamped
            x = "0e%d" % sys.maxsize
            self.assertRaises(InvalidOperation, Decimal, x)

            x = "0e%d" % (-sys.maxsize-1)
            self.assertRaises(InvalidOperation, Decimal, x)

            # Overflow
            x = "1e%d" % sys.maxsize
            self.assertRaises(InvalidOperation, Decimal, x)

            # Underflow
            x = "1e%d" % (-sys.maxsize-1)
            self.assertRaises(InvalidOperation, Decimal, x)

    def test_from_tuple(self):
        Decimal = C.Decimal
        localcontext = C.localcontext
        InvalidOperation = C.InvalidOperation
        Overflow = C.Overflow
        Underflow = C.Underflow

        with localcontext() as c:

            c.prec = 9
            c.traps[InvalidOperation] = True
            c.traps[Overflow] = True
            c.traps[Underflow] = True

            # SSIZE_MAX
            x = (1, (), sys.maxsize)
            self.assertEqual(str(c.create_decimal(x)), '-0E+999999')
            self.assertRaises(InvalidOperation, Decimal, x)

            x = (1, (0, 1, 2), sys.maxsize)
            self.assertRaises(Overflow, c.create_decimal, x)
            self.assertRaises(InvalidOperation, Decimal, x)

            # SSIZE_MIN
            x = (1, (), -sys.maxsize-1)
            self.assertEqual(str(c.create_decimal(x)), '-0E-1000007')
            self.assertRaises(InvalidOperation, Decimal, x)

            x = (1, (0, 1, 2), -sys.maxsize-1)
            self.assertRaises(Underflow, c.create_decimal, x)
            self.assertRaises(InvalidOperation, Decimal, x)

            # OverflowError
            x = (1, (), sys.maxsize+1)
            self.assertRaises(OverflowError, c.create_decimal, x)
            self.assertRaises(OverflowError, Decimal, x)

            x = (1, (), -sys.maxsize-2)
            self.assertRaises(OverflowError, c.create_decimal, x)
            self.assertRaises(OverflowError, Decimal, x)

            # Specials
            x = (1, (), "N")
            self.assertEqual(str(Decimal(x)), '-sNaN')
            x = (1, (0,), "N")
            self.assertEqual(str(Decimal(x)), '-sNaN')
            x = (1, (0, 1), "N")
            self.assertEqual(str(Decimal(x)), '-sNaN1')

    def test_sizeof(self):
        Decimal = C.Decimal
        HAVE_CONFIG_64 = (C.MAX_PREC > 425000000)

        self.assertGreater(Decimal(0).__sizeof__(), 0)
        if HAVE_CONFIG_64:
            x = Decimal(10**(19*24)).__sizeof__()
            y = Decimal(10**(19*25)).__sizeof__()
            self.assertEqual(y, x+8)
        else:
            x = Decimal(10**(9*24)).__sizeof__()
            y = Decimal(10**(9*25)).__sizeof__()
            self.assertEqual(y, x+4)

    def test_internal_use_of_overridden_methods(self):
        Decimal = C.Decimal

        # Unsound subtyping
        class X(float):
            def as_integer_ratio(self):
                return 1
            def __abs__(self):
                return self

        class Y(float):
            def __abs__(self):
                return [1]*200

        class I(int):
            def bit_length(self):
                return [1]*200

        class Z(float):
            def as_integer_ratio(self):
                return (I(1), I(1))
            def __abs__(self):
                return self

        for cls in X, Y, Z:
            self.assertEqual(Decimal.from_float(cls(101.1)),
                             Decimal.from_float(101.1))

    def test_c_immutable_types(self):
        SignalDict = type(C.Context().flags)
        SignalDictMixin = SignalDict.__bases__[0]
        ContextManager = type(C.localcontext())
        types = (
            SignalDictMixin,
            ContextManager,
            C.Decimal,
            C.Context,
        )
        for tp in types:
            with self.subTest(tp=tp):
                with self.assertRaisesRegex(TypeError, "immutable"):
                    tp.foo = 1

    def test_c_disallow_instantiation(self):
        ContextManager = type(C.localcontext())
        check_disallow_instantiation(self, ContextManager)

    def test_c_signaldict_segfault(self):
        # See gh-106263 for details.
        SignalDict = type(C.Context().flags)
        sd = SignalDict()
        err_msg = "invalid signal dict"

        with self.assertRaisesRegex(ValueError, err_msg):
            len(sd)

        with self.assertRaisesRegex(ValueError, err_msg):
            iter(sd)

        with self.assertRaisesRegex(ValueError, err_msg):
            repr(sd)

        with self.assertRaisesRegex(ValueError, err_msg):
            sd[C.InvalidOperation] = True

        with self.assertRaisesRegex(ValueError, err_msg):
            sd[C.InvalidOperation]

        with self.assertRaisesRegex(ValueError, err_msg):
            sd == C.Context().flags

        with self.assertRaisesRegex(ValueError, err_msg):
            C.Context().flags == sd

        with self.assertRaisesRegex(ValueError, err_msg):
            sd.copy()

    def test_format_fallback_capitals(self):
        # Fallback to _pydecimal formatting (triggered by `#` format which
        # is unsupported by mpdecimal) should honor the current context.
        x = C.Decimal('6.09e+23')
        self.assertEqual(format(x, '#'), '6.09E+23')
        with C.localcontext(capitals=0):
            self.assertEqual(format(x, '#'), '6.09e+23')

    def test_format_fallback_rounding(self):
        y = C.Decimal('6.09')
        self.assertEqual(format(y, '#.1f'), '6.1')
        with C.localcontext(rounding=C.ROUND_DOWN):
            self.assertEqual(format(y, '#.1f'), '6.0')


if __name__ == '__main__':
    unittest.main()
