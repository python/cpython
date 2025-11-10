import unittest
from test.support import threading_helper
import threading
import contextvars
from . import (C, load_tests_for_base_classes, Signals,
               setUpModule, tearDownModule)


# The following are two functions used to test threading in the next class

def thfunc1(cls):
    Decimal = cls.decimal.Decimal
    InvalidOperation = cls.decimal.InvalidOperation
    DivisionByZero = cls.decimal.DivisionByZero
    Overflow = cls.decimal.Overflow
    Underflow = cls.decimal.Underflow
    Inexact = cls.decimal.Inexact
    getcontext = cls.decimal.getcontext
    localcontext = cls.decimal.localcontext

    d1 = Decimal(1)
    d3 = Decimal(3)
    test1 = d1/d3

    cls.finish1.set()
    cls.synchro.wait()

    test2 = d1/d3
    with localcontext() as c2:
        cls.assertTrue(c2.flags[Inexact])
        cls.assertRaises(DivisionByZero, c2.divide, d1, 0)
        cls.assertTrue(c2.flags[DivisionByZero])
        with localcontext() as c3:
            cls.assertTrue(c3.flags[Inexact])
            cls.assertTrue(c3.flags[DivisionByZero])
            cls.assertRaises(InvalidOperation, c3.compare, d1, Decimal('sNaN'))
            cls.assertTrue(c3.flags[InvalidOperation])
            del c3
        cls.assertFalse(c2.flags[InvalidOperation])
        del c2

    cls.assertEqual(test1, Decimal('0.333333333333333333333333'))
    cls.assertEqual(test2, Decimal('0.333333333333333333333333'))

    c1 = getcontext()
    cls.assertTrue(c1.flags[Inexact])
    for sig in Overflow, Underflow, DivisionByZero, InvalidOperation:
        cls.assertFalse(c1.flags[sig])

def thfunc2(cls):
    Decimal = cls.decimal.Decimal
    InvalidOperation = cls.decimal.InvalidOperation
    DivisionByZero = cls.decimal.DivisionByZero
    Overflow = cls.decimal.Overflow
    Underflow = cls.decimal.Underflow
    Inexact = cls.decimal.Inexact
    getcontext = cls.decimal.getcontext
    localcontext = cls.decimal.localcontext

    d1 = Decimal(1)
    d3 = Decimal(3)
    test1 = d1/d3

    thiscontext = getcontext()
    thiscontext.prec = 18
    test2 = d1/d3

    with localcontext() as c2:
        cls.assertTrue(c2.flags[Inexact])
        cls.assertRaises(Overflow, c2.multiply, Decimal('1e425000000'), 999)
        cls.assertTrue(c2.flags[Overflow])
        with localcontext(thiscontext) as c3:
            cls.assertTrue(c3.flags[Inexact])
            cls.assertFalse(c3.flags[Overflow])
            c3.traps[Underflow] = True
            cls.assertRaises(Underflow, c3.divide, Decimal('1e-425000000'), 999)
            cls.assertTrue(c3.flags[Underflow])
            del c3
        cls.assertFalse(c2.flags[Underflow])
        cls.assertFalse(c2.traps[Underflow])
        del c2

    cls.synchro.set()
    cls.finish2.set()

    cls.assertEqual(test1, Decimal('0.333333333333333333333333'))
    cls.assertEqual(test2, Decimal('0.333333333333333333'))

    cls.assertFalse(thiscontext.traps[Underflow])
    cls.assertTrue(thiscontext.flags[Inexact])
    for sig in Overflow, Underflow, DivisionByZero, InvalidOperation:
        cls.assertFalse(thiscontext.flags[sig])


@threading_helper.requires_working_threading()
class ThreadingTest:
    '''Unit tests for thread local contexts in Decimal.'''

    # Take care executing this test from IDLE, there's an issue in threading
    # that hangs IDLE and I couldn't find it

    def test_threading(self):
        DefaultContext = self.decimal.DefaultContext

        if self.decimal == C and not self.decimal.HAVE_THREADS:
            self.skipTest("compiled without threading")
        # Test the "threading isolation" of a Context. Also test changing
        # the DefaultContext, which acts as a template for the thread-local
        # contexts.
        save_prec = DefaultContext.prec
        save_emax = DefaultContext.Emax
        save_emin = DefaultContext.Emin
        DefaultContext.prec = 24
        DefaultContext.Emax = 425000000
        DefaultContext.Emin = -425000000

        self.synchro = threading.Event()
        self.finish1 = threading.Event()
        self.finish2 = threading.Event()

        # This test wants to start threads with an empty context, no matter
        # the setting of sys.flags.thread_inherit_context.  We pass the
        # 'context' argument explicitly with an empty context instance.
        th1 = threading.Thread(target=thfunc1, args=(self,),
                               context=contextvars.Context())
        th2 = threading.Thread(target=thfunc2, args=(self,),
                               context=contextvars.Context())

        th1.start()
        th2.start()

        self.finish1.wait()
        self.finish2.wait()

        for sig in Signals[self.decimal]:
            self.assertFalse(DefaultContext.flags[sig])

        th1.join()
        th2.join()

        DefaultContext.prec = save_prec
        DefaultContext.Emax = save_emax
        DefaultContext.Emin = save_emin


def load_tests(loader, tests, pattern):
    return load_tests_for_base_classes(loader, tests, [ThreadingTest])


if __name__ == '__main__':
    unittest.main()
