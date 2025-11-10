import unittest
from test.support import requires_IEEE_754
from . import (C, P, load_tests_for_base_classes,
    assert_signals, OrderedSignals,
    setUpModule, tearDownModule)


class ContextWithStatement:
    # Can't do these as docstrings until Python 2.6
    # as doctest can't handle __future__ statements

    def test_localcontext(self):
        # Use a copy of the current context in the block
        getcontext = self.decimal.getcontext
        localcontext = self.decimal.localcontext

        orig_ctx = getcontext()
        with localcontext() as enter_ctx:
            set_ctx = getcontext()
        final_ctx = getcontext()
        self.assertIs(orig_ctx, final_ctx, 'did not restore context correctly')
        self.assertIsNot(orig_ctx, set_ctx, 'did not copy the context')
        self.assertIs(set_ctx, enter_ctx, '__enter__ returned wrong context')

    def test_localcontextarg(self):
        # Use a copy of the supplied context in the block
        Context = self.decimal.Context
        getcontext = self.decimal.getcontext
        localcontext = self.decimal.localcontext

        orig_ctx = getcontext()
        new_ctx = Context(prec=42)
        with localcontext(new_ctx) as enter_ctx:
            set_ctx = getcontext()
        final_ctx = getcontext()
        self.assertIs(orig_ctx, final_ctx, 'did not restore context correctly')
        self.assertEqual(set_ctx.prec, new_ctx.prec, 'did not set correct context')
        self.assertIsNot(new_ctx, set_ctx, 'did not copy the context')
        self.assertIs(set_ctx, enter_ctx, '__enter__ returned wrong context')

    def test_localcontext_kwargs(self):
        with self.decimal.localcontext(
            prec=10, rounding=P.ROUND_HALF_DOWN,
            Emin=-20, Emax=20, capitals=0,
            clamp=1
        ) as ctx:
            self.assertEqual(ctx.prec, 10)
            self.assertEqual(ctx.rounding, self.decimal.ROUND_HALF_DOWN)
            self.assertEqual(ctx.Emin, -20)
            self.assertEqual(ctx.Emax, 20)
            self.assertEqual(ctx.capitals, 0)
            self.assertEqual(ctx.clamp, 1)

        self.assertRaises(TypeError, self.decimal.localcontext, precision=10)

        self.assertRaises(ValueError, self.decimal.localcontext, Emin=1)
        self.assertRaises(ValueError, self.decimal.localcontext, Emax=-1)
        self.assertRaises(ValueError, self.decimal.localcontext, capitals=2)
        self.assertRaises(ValueError, self.decimal.localcontext, clamp=2)

        self.assertRaises(TypeError, self.decimal.localcontext, rounding="")
        self.assertRaises(TypeError, self.decimal.localcontext, rounding=1)

        self.assertRaises(TypeError, self.decimal.localcontext, flags="")
        self.assertRaises(TypeError, self.decimal.localcontext, traps="")
        self.assertRaises(TypeError, self.decimal.localcontext, Emin="")
        self.assertRaises(TypeError, self.decimal.localcontext, Emax="")

    def test_local_context_kwargs_does_not_overwrite_existing_argument(self):
        ctx = self.decimal.getcontext()
        orig_prec = ctx.prec
        with self.decimal.localcontext(prec=10) as ctx2:
            self.assertEqual(ctx2.prec, 10)
            self.assertEqual(ctx.prec, orig_prec)
        with self.decimal.localcontext(prec=20) as ctx2:
            self.assertEqual(ctx2.prec, 20)
            self.assertEqual(ctx.prec, orig_prec)

    def test_nested_with_statements(self):
        # Use a copy of the supplied context in the block
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context
        getcontext = self.decimal.getcontext
        localcontext = self.decimal.localcontext
        Clamped = self.decimal.Clamped
        Overflow = self.decimal.Overflow

        orig_ctx = getcontext()
        orig_ctx.clear_flags()
        new_ctx = Context(Emax=384)
        with localcontext() as c1:
            self.assertEqual(c1.flags, orig_ctx.flags)
            self.assertEqual(c1.traps, orig_ctx.traps)
            c1.traps[Clamped] = True
            c1.Emin = -383
            self.assertNotEqual(orig_ctx.Emin, -383)
            self.assertRaises(Clamped, c1.create_decimal, '0e-999')
            self.assertTrue(c1.flags[Clamped])
            with localcontext(new_ctx) as c2:
                self.assertEqual(c2.flags, new_ctx.flags)
                self.assertEqual(c2.traps, new_ctx.traps)
                self.assertRaises(Overflow, c2.power, Decimal('3.4e200'), 2)
                self.assertFalse(c2.flags[Clamped])
                self.assertTrue(c2.flags[Overflow])
                del c2
            self.assertFalse(c1.flags[Overflow])
            del c1
        self.assertNotEqual(orig_ctx.Emin, -383)
        self.assertFalse(orig_ctx.flags[Clamped])
        self.assertFalse(orig_ctx.flags[Overflow])
        self.assertFalse(new_ctx.flags[Clamped])
        self.assertFalse(new_ctx.flags[Overflow])

    def test_with_statements_gc1(self):
        localcontext = self.decimal.localcontext

        with localcontext() as c1:
            del c1
            with localcontext() as c2:
                del c2
                with localcontext() as c3:
                    del c3
                    with localcontext() as c4:
                        del c4

    def test_with_statements_gc2(self):
        localcontext = self.decimal.localcontext

        with localcontext() as c1:
            with localcontext(c1) as c2:
                del c1
                with localcontext(c2) as c3:
                    del c2
                    with localcontext(c3) as c4:
                        del c3
                        del c4

    def test_with_statements_gc3(self):
        Context = self.decimal.Context
        localcontext = self.decimal.localcontext
        getcontext = self.decimal.getcontext
        setcontext = self.decimal.setcontext

        with localcontext() as c1:
            del c1
            n1 = Context(prec=1)
            setcontext(n1)
            with localcontext(n1) as c2:
                del n1
                self.assertEqual(c2.prec, 1)
                del c2
                n2 = Context(prec=2)
                setcontext(n2)
                del n2
                self.assertEqual(getcontext().prec, 2)
                n3 = Context(prec=3)
                setcontext(n3)
                self.assertEqual(getcontext().prec, 3)
                with localcontext(n3) as c3:
                    del n3
                    self.assertEqual(c3.prec, 3)
                    del c3
                    n4 = Context(prec=4)
                    setcontext(n4)
                    del n4
                    self.assertEqual(getcontext().prec, 4)
                    with localcontext() as c4:
                        self.assertEqual(c4.prec, 4)
                        del c4


class ContextFlags:

    def test_flags_irrelevant(self):
        # check that the result (numeric result + flags raised) of an
        # arithmetic operation doesn't depend on the current flags
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context
        Inexact = self.decimal.Inexact
        Rounded = self.decimal.Rounded
        Underflow = self.decimal.Underflow
        Clamped = self.decimal.Clamped
        Subnormal = self.decimal.Subnormal

        def raise_error(context, flag):
            if self.decimal == C:
                context.flags[flag] = True
                if context.traps[flag]:
                    raise flag
            else:
                context._raise_error(flag)

        context = Context(prec=9, Emin = -425000000, Emax = 425000000,
                          rounding=P.ROUND_HALF_EVEN, traps=[], flags=[])

        # operations that raise various flags, in the form (function, arglist)
        operations = [
            (context._apply, [Decimal("100E-425000010")]),
            (context.sqrt, [Decimal(2)]),
            (context.add, [Decimal("1.23456789"), Decimal("9.87654321")]),
            (context.multiply, [Decimal("1.23456789"), Decimal("9.87654321")]),
            (context.subtract, [Decimal("1.23456789"), Decimal("9.87654321")]),
            ]

        # try various flags individually, then a whole lot at once
        flagsets = [[Inexact], [Rounded], [Underflow], [Clamped], [Subnormal],
                    [Inexact, Rounded, Underflow, Clamped, Subnormal]]

        for fn, args in operations:
            # find answer and flags raised using a clean context
            context.clear_flags()
            ans = fn(*args)
            flags = [k for k, v in context.flags.items() if v]

            for extra_flags in flagsets:
                # set flags, before calling operation
                context.clear_flags()
                for flag in extra_flags:
                    raise_error(context, flag)
                new_ans = fn(*args)

                # flags that we expect to be set after the operation
                expected_flags = list(flags)
                for flag in extra_flags:
                    if flag not in expected_flags:
                        expected_flags.append(flag)
                expected_flags.sort(key=id)

                # flags we actually got
                new_flags = [k for k,v in context.flags.items() if v]
                new_flags.sort(key=id)

                self.assertEqual(ans, new_ans,
                                 "operation produces different answers depending on flags set: " +
                                 "expected %s, got %s." % (ans, new_ans))
                self.assertEqual(new_flags, expected_flags,
                                  "operation raises different flags depending on flags set: " +
                                  "expected %s, got %s" % (expected_flags, new_flags))

    def test_flag_comparisons(self):
        Context = self.decimal.Context
        Inexact = self.decimal.Inexact
        Rounded = self.decimal.Rounded

        c = Context()

        # Valid SignalDict
        self.assertNotEqual(c.flags, c.traps)
        self.assertNotEqual(c.traps, c.flags)

        c.flags = c.traps
        self.assertEqual(c.flags, c.traps)
        self.assertEqual(c.traps, c.flags)

        c.flags[Rounded] = True
        c.traps = c.flags
        self.assertEqual(c.flags, c.traps)
        self.assertEqual(c.traps, c.flags)

        d = {}
        d.update(c.flags)
        self.assertEqual(d, c.flags)
        self.assertEqual(c.flags, d)

        d[Inexact] = True
        self.assertNotEqual(d, c.flags)
        self.assertNotEqual(c.flags, d)

        # Invalid SignalDict
        d = {Inexact:False}
        self.assertNotEqual(d, c.flags)
        self.assertNotEqual(c.flags, d)

        d = ["xyz"]
        self.assertNotEqual(d, c.flags)
        self.assertNotEqual(c.flags, d)

    @requires_IEEE_754
    def test_float_operation(self):
        Decimal = self.decimal.Decimal
        FloatOperation = self.decimal.FloatOperation
        localcontext = self.decimal.localcontext

        with localcontext() as c:
            ##### trap is off by default
            self.assertFalse(c.traps[FloatOperation])

            # implicit conversion sets the flag
            c.clear_flags()
            self.assertEqual(Decimal(7.5), 7.5)
            self.assertTrue(c.flags[FloatOperation])

            c.clear_flags()
            self.assertEqual(c.create_decimal(7.5), 7.5)
            self.assertTrue(c.flags[FloatOperation])

            # explicit conversion does not set the flag
            c.clear_flags()
            x = Decimal.from_float(7.5)
            self.assertFalse(c.flags[FloatOperation])
            # comparison sets the flag
            self.assertEqual(x, 7.5)
            self.assertTrue(c.flags[FloatOperation])

            c.clear_flags()
            x = c.create_decimal_from_float(7.5)
            self.assertFalse(c.flags[FloatOperation])
            self.assertEqual(x, 7.5)
            self.assertTrue(c.flags[FloatOperation])

            ##### set the trap
            c.traps[FloatOperation] = True

            # implicit conversion raises
            c.clear_flags()
            self.assertRaises(FloatOperation, Decimal, 7.5)
            self.assertTrue(c.flags[FloatOperation])

            c.clear_flags()
            self.assertRaises(FloatOperation, c.create_decimal, 7.5)
            self.assertTrue(c.flags[FloatOperation])

            # explicit conversion is silent
            c.clear_flags()
            x = Decimal.from_float(7.5)
            self.assertFalse(c.flags[FloatOperation])

            c.clear_flags()
            x = c.create_decimal_from_float(7.5)
            self.assertFalse(c.flags[FloatOperation])

    def test_float_comparison(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context
        FloatOperation = self.decimal.FloatOperation
        localcontext = self.decimal.localcontext

        def assert_attr(a, b, attr, context, signal=None):
            context.clear_flags()
            f = getattr(a, attr)
            if signal == FloatOperation:
                self.assertRaises(signal, f, b)
            else:
                self.assertIs(f(b), True)
            self.assertTrue(context.flags[FloatOperation])

        small_d = Decimal('0.25')
        big_d = Decimal('3.0')
        small_f = 0.25
        big_f = 3.0

        zero_d = Decimal('0.0')
        neg_zero_d = Decimal('-0.0')
        zero_f = 0.0
        neg_zero_f = -0.0

        inf_d = Decimal('Infinity')
        neg_inf_d = Decimal('-Infinity')
        inf_f = float('inf')
        neg_inf_f = float('-inf')

        def doit(c, signal=None):
            # Order
            for attr in '__lt__', '__le__':
                assert_attr(small_d, big_f, attr, c, signal)

            for attr in '__gt__', '__ge__':
                assert_attr(big_d, small_f, attr, c, signal)

            # Equality
            assert_attr(small_d, small_f, '__eq__', c, None)

            assert_attr(neg_zero_d, neg_zero_f, '__eq__', c, None)
            assert_attr(neg_zero_d, zero_f, '__eq__', c, None)

            assert_attr(zero_d, neg_zero_f, '__eq__', c, None)
            assert_attr(zero_d, zero_f, '__eq__', c, None)

            assert_attr(neg_inf_d, neg_inf_f, '__eq__', c, None)
            assert_attr(inf_d, inf_f, '__eq__', c, None)

            # Inequality
            assert_attr(small_d, big_f, '__ne__', c, None)

            assert_attr(Decimal('0.1'), 0.1, '__ne__', c, None)

            assert_attr(neg_inf_d, inf_f, '__ne__', c, None)
            assert_attr(inf_d, neg_inf_f, '__ne__', c, None)

            assert_attr(Decimal('NaN'), float('nan'), '__ne__', c, None)

        def test_containers(c, signal=None):
            c.clear_flags()
            s = set([100.0, Decimal('100.0')])
            self.assertEqual(len(s), 1)
            self.assertTrue(c.flags[FloatOperation])

            c.clear_flags()
            if signal:
                self.assertRaises(signal, sorted, [1.0, Decimal('10.0')])
            else:
                s = sorted([10.0, Decimal('10.0')])
            self.assertTrue(c.flags[FloatOperation])

            c.clear_flags()
            b = 10.0 in [Decimal('10.0'), 1.0]
            self.assertTrue(c.flags[FloatOperation])

            c.clear_flags()
            b = 10.0 in {Decimal('10.0'):'a', 1.0:'b'}
            self.assertTrue(c.flags[FloatOperation])

        nc = Context()
        with localcontext(nc) as c:
            self.assertFalse(c.traps[FloatOperation])
            doit(c, signal=None)
            test_containers(c, signal=None)

            c.traps[FloatOperation] = True
            doit(c, signal=FloatOperation)
            test_containers(c, signal=FloatOperation)

    def test_float_operation_default(self):
        Decimal = self.decimal.Decimal
        Context = self.decimal.Context
        Inexact = self.decimal.Inexact
        FloatOperation= self.decimal.FloatOperation

        context = Context()
        self.assertFalse(context.flags[FloatOperation])
        self.assertFalse(context.traps[FloatOperation])

        context.clear_traps()
        context.traps[Inexact] = True
        context.traps[FloatOperation] = True
        self.assertTrue(context.traps[FloatOperation])
        self.assertTrue(context.traps[Inexact])


class SpecialContexts:
    """Test the context templates."""

    def test_context_templates(self):
        BasicContext = self.decimal.BasicContext
        ExtendedContext = self.decimal.ExtendedContext
        getcontext = self.decimal.getcontext
        setcontext = self.decimal.setcontext
        InvalidOperation = self.decimal.InvalidOperation
        DivisionByZero = self.decimal.DivisionByZero
        Overflow = self.decimal.Overflow
        Underflow = self.decimal.Underflow
        Clamped = self.decimal.Clamped

        assert_signals(self, BasicContext, 'traps',
            [InvalidOperation, DivisionByZero, Overflow, Underflow, Clamped]
        )

        savecontext = getcontext().copy()
        basic_context_prec = BasicContext.prec
        extended_context_prec = ExtendedContext.prec

        ex = None
        try:
            BasicContext.prec = ExtendedContext.prec = 441
            for template in BasicContext, ExtendedContext:
                setcontext(template)
                c = getcontext()
                self.assertIsNot(c, template)
                self.assertEqual(c.prec, 441)
        except Exception as e:
            ex = e.__class__
        finally:
            BasicContext.prec = basic_context_prec
            ExtendedContext.prec = extended_context_prec
            setcontext(savecontext)
            if ex:
                raise ex

    def test_default_context(self):
        DefaultContext = self.decimal.DefaultContext
        BasicContext = self.decimal.BasicContext
        ExtendedContext = self.decimal.ExtendedContext
        getcontext = self.decimal.getcontext
        setcontext = self.decimal.setcontext
        InvalidOperation = self.decimal.InvalidOperation
        DivisionByZero = self.decimal.DivisionByZero
        Overflow = self.decimal.Overflow

        self.assertEqual(BasicContext.prec, 9)
        self.assertEqual(ExtendedContext.prec, 9)

        assert_signals(self, DefaultContext, 'traps',
            [InvalidOperation, DivisionByZero, Overflow]
        )

        savecontext = getcontext().copy()
        default_context_prec = DefaultContext.prec

        ex = None
        try:
            c = getcontext()
            saveprec = c.prec

            DefaultContext.prec = 961
            c = getcontext()
            self.assertEqual(c.prec, saveprec)

            setcontext(DefaultContext)
            c = getcontext()
            self.assertIsNot(c, DefaultContext)
            self.assertEqual(c.prec, 961)
        except Exception as e:
            ex = e.__class__
        finally:
            DefaultContext.prec = default_context_prec
            setcontext(savecontext)
            if ex:
                raise ex


class ContextInputValidation:

    def test_invalid_context(self):
        Context = self.decimal.Context
        DefaultContext = self.decimal.DefaultContext

        c = DefaultContext.copy()

        # prec, Emax
        for attr in ['prec', 'Emax']:
            setattr(c, attr, 999999)
            self.assertEqual(getattr(c, attr), 999999)
            self.assertRaises(ValueError, setattr, c, attr, -1)
            self.assertRaises(TypeError, setattr, c, attr, 'xyz')

        # Emin
        setattr(c, 'Emin', -999999)
        self.assertEqual(getattr(c, 'Emin'), -999999)
        self.assertRaises(ValueError, setattr, c, 'Emin', 1)
        self.assertRaises(TypeError, setattr, c, 'Emin', (1,2,3))

        self.assertRaises(TypeError, setattr, c, 'rounding', -1)
        self.assertRaises(TypeError, setattr, c, 'rounding', 9)
        self.assertRaises(TypeError, setattr, c, 'rounding', 1.0)
        self.assertRaises(TypeError, setattr, c, 'rounding', 'xyz')

        # capitals, clamp
        for attr in ['capitals', 'clamp']:
            self.assertRaises(ValueError, setattr, c, attr, -1)
            self.assertRaises(ValueError, setattr, c, attr, 2)
            self.assertRaises(TypeError, setattr, c, attr, [1,2,3])

        # Invalid attribute
        self.assertRaises(AttributeError, setattr, c, 'emax', 100)

        # Invalid signal dict
        self.assertRaises(TypeError, setattr, c, 'flags', [])
        self.assertRaises(KeyError, setattr, c, 'flags', {})
        self.assertRaises(KeyError, setattr, c, 'traps',
                          {'InvalidOperation':0})

        # Attributes cannot be deleted
        for attr in ['prec', 'Emax', 'Emin', 'rounding', 'capitals', 'clamp',
                     'flags', 'traps']:
            self.assertRaises(AttributeError, c.__delattr__, attr)

        # Invalid attributes
        self.assertRaises(TypeError, getattr, c, 9)
        self.assertRaises(TypeError, setattr, c, 9)

        # Invalid values in constructor
        self.assertRaises(TypeError, Context, rounding=999999)
        self.assertRaises(TypeError, Context, rounding='xyz')
        self.assertRaises(ValueError, Context, clamp=2)
        self.assertRaises(ValueError, Context, capitals=-1)
        self.assertRaises(KeyError, Context, flags=["P"])
        self.assertRaises(KeyError, Context, traps=["Q"])

        # Type error in conversion
        self.assertRaises(TypeError, Context, flags=(0,1))
        self.assertRaises(TypeError, Context, traps=(1,0))


class ContextSubclassing:

    def test_context_subclassing(self):
        decimal = self.decimal
        Decimal = decimal.Decimal
        Context = decimal.Context
        Clamped = decimal.Clamped
        DivisionByZero = decimal.DivisionByZero
        Inexact = decimal.Inexact
        Overflow = decimal.Overflow
        Rounded = decimal.Rounded
        Subnormal = decimal.Subnormal
        Underflow = decimal.Underflow
        InvalidOperation = decimal.InvalidOperation

        class MyContext(Context):
            def __init__(self, prec=None, rounding=None, Emin=None, Emax=None,
                               capitals=None, clamp=None, flags=None,
                               traps=None):
                Context.__init__(self)
                if prec is not None:
                    self.prec = prec
                if rounding is not None:
                    self.rounding = rounding
                if Emin is not None:
                    self.Emin = Emin
                if Emax is not None:
                    self.Emax = Emax
                if capitals is not None:
                    self.capitals = capitals
                if clamp is not None:
                    self.clamp = clamp
                if flags is not None:
                    if isinstance(flags, list):
                        flags = {v:(v in flags) for v in OrderedSignals[decimal] + flags}
                    self.flags = flags
                if traps is not None:
                    if isinstance(traps, list):
                        traps = {v:(v in traps) for v in OrderedSignals[decimal] + traps}
                    self.traps = traps

        c = Context()
        d = MyContext()
        for attr in ('prec', 'rounding', 'Emin', 'Emax', 'capitals', 'clamp',
                     'flags', 'traps'):
            self.assertEqual(getattr(c, attr), getattr(d, attr))

        # prec
        self.assertRaises(ValueError, MyContext, **{'prec':-1})
        c = MyContext(prec=1)
        self.assertEqual(c.prec, 1)
        self.assertRaises(InvalidOperation, c.quantize, Decimal('9e2'), 0)

        # rounding
        self.assertRaises(TypeError, MyContext, **{'rounding':'XYZ'})
        c = MyContext(rounding=P.ROUND_DOWN, prec=1)
        self.assertEqual(c.rounding, P.ROUND_DOWN)
        self.assertEqual(c.plus(Decimal('9.9')), 9)

        # Emin
        self.assertRaises(ValueError, MyContext, **{'Emin':5})
        c = MyContext(Emin=-1, prec=1)
        self.assertEqual(c.Emin, -1)
        x = c.add(Decimal('1e-99'), Decimal('2.234e-2000'))
        self.assertEqual(x, Decimal('0.0'))
        for signal in (Inexact, Underflow, Subnormal, Rounded, Clamped):
            self.assertTrue(c.flags[signal])

        # Emax
        self.assertRaises(ValueError, MyContext, **{'Emax':-1})
        c = MyContext(Emax=1, prec=1)
        self.assertEqual(c.Emax, 1)
        self.assertRaises(Overflow, c.add, Decimal('1e99'), Decimal('2.234e2000'))
        if self.decimal == C:
            for signal in (Inexact, Overflow, Rounded):
                self.assertTrue(c.flags[signal])

        # capitals
        self.assertRaises(ValueError, MyContext, **{'capitals':-1})
        c = MyContext(capitals=0)
        self.assertEqual(c.capitals, 0)
        x = c.create_decimal('1E222')
        self.assertEqual(c.to_sci_string(x), '1e+222')

        # clamp
        self.assertRaises(ValueError, MyContext, **{'clamp':2})
        c = MyContext(clamp=1, Emax=99)
        self.assertEqual(c.clamp, 1)
        x = c.plus(Decimal('1e99'))
        self.assertEqual(str(x), '1.000000000000000000000000000E+99')

        # flags
        self.assertRaises(TypeError, MyContext, **{'flags':'XYZ'})
        c = MyContext(flags=[Rounded, DivisionByZero])
        for signal in (Rounded, DivisionByZero):
            self.assertTrue(c.flags[signal])
        c.clear_flags()
        for signal in OrderedSignals[decimal]:
            self.assertFalse(c.flags[signal])

        # traps
        self.assertRaises(TypeError, MyContext, **{'traps':'XYZ'})
        c = MyContext(traps=[Rounded, DivisionByZero])
        for signal in (Rounded, DivisionByZero):
            self.assertTrue(c.traps[signal])
        c.clear_traps()
        for signal in OrderedSignals[decimal]:
            self.assertFalse(c.traps[signal])


class IEEEContexts:

    def test_ieee_context(self):
        # issue 8786: Add support for IEEE 754 contexts to decimal module.
        IEEEContext = self.decimal.IEEEContext

        def assert_rest(self, context):
            self.assertEqual(context.clamp, 1)
            assert_signals(self, context, 'traps', [])
            assert_signals(self, context, 'flags', [])

        c = IEEEContext(32)
        self.assertEqual(c.prec, 7)
        self.assertEqual(c.Emax, 96)
        self.assertEqual(c.Emin, -95)
        assert_rest(self, c)

        c = IEEEContext(64)
        self.assertEqual(c.prec, 16)
        self.assertEqual(c.Emax, 384)
        self.assertEqual(c.Emin, -383)
        assert_rest(self, c)

        c = IEEEContext(128)
        self.assertEqual(c.prec, 34)
        self.assertEqual(c.Emax, 6144)
        self.assertEqual(c.Emin, -6143)
        assert_rest(self, c)

        # Invalid values
        self.assertRaises(ValueError, IEEEContext, -1)
        self.assertRaises(ValueError, IEEEContext, 123)
        self.assertRaises(ValueError, IEEEContext, 1024)

    def test_constants(self):
        # IEEEContext
        IEEE_CONTEXT_MAX_BITS = self.decimal.IEEE_CONTEXT_MAX_BITS
        self.assertIn(IEEE_CONTEXT_MAX_BITS, {256, 512})


def load_tests(loader, tests, pattern):
    base_classes = [
        ContextWithStatement,
        ContextFlags,
        SpecialContexts,
        ContextInputValidation,
        ContextSubclassing,
        IEEEContexts,
    ]
    return load_tests_for_base_classes(loader, tests, base_classes)


if __name__ == '__main__':
    unittest.main()
