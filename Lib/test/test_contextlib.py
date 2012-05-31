"""Unit tests for contextlib.py, and other context managers."""

import sys
import tempfile
import unittest
from contextlib import *  # Tests __all__
from test import support
try:
    import threading
except ImportError:
    threading = None


class ContextManagerTestCase(unittest.TestCase):

    def test_contextmanager_plain(self):
        state = []
        @contextmanager
        def woohoo():
            state.append(1)
            yield 42
            state.append(999)
        with woohoo() as x:
            self.assertEqual(state, [1])
            self.assertEqual(x, 42)
            state.append(x)
        self.assertEqual(state, [1, 42, 999])

    def test_contextmanager_finally(self):
        state = []
        @contextmanager
        def woohoo():
            state.append(1)
            try:
                yield 42
            finally:
                state.append(999)
        with self.assertRaises(ZeroDivisionError):
            with woohoo() as x:
                self.assertEqual(state, [1])
                self.assertEqual(x, 42)
                state.append(x)
                raise ZeroDivisionError()
        self.assertEqual(state, [1, 42, 999])

    def test_contextmanager_no_reraise(self):
        @contextmanager
        def whee():
            yield
        ctx = whee()
        ctx.__enter__()
        # Calling __exit__ should not result in an exception
        self.assertFalse(ctx.__exit__(TypeError, TypeError("foo"), None))

    def test_contextmanager_trap_yield_after_throw(self):
        @contextmanager
        def whoo():
            try:
                yield
            except:
                yield
        ctx = whoo()
        ctx.__enter__()
        self.assertRaises(
            RuntimeError, ctx.__exit__, TypeError, TypeError("foo"), None
        )

    def test_contextmanager_except(self):
        state = []
        @contextmanager
        def woohoo():
            state.append(1)
            try:
                yield 42
            except ZeroDivisionError as e:
                state.append(e.args[0])
                self.assertEqual(state, [1, 42, 999])
        with woohoo() as x:
            self.assertEqual(state, [1])
            self.assertEqual(x, 42)
            state.append(x)
            raise ZeroDivisionError(999)
        self.assertEqual(state, [1, 42, 999])

    def _create_contextmanager_attribs(self):
        def attribs(**kw):
            def decorate(func):
                for k,v in kw.items():
                    setattr(func,k,v)
                return func
            return decorate
        @contextmanager
        @attribs(foo='bar')
        def baz(spam):
            """Whee!"""
        return baz

    def test_contextmanager_attribs(self):
        baz = self._create_contextmanager_attribs()
        self.assertEqual(baz.__name__,'baz')
        self.assertEqual(baz.foo, 'bar')

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def test_contextmanager_doc_attrib(self):
        baz = self._create_contextmanager_attribs()
        self.assertEqual(baz.__doc__, "Whee!")

class ClosingTestCase(unittest.TestCase):

    # XXX This needs more work

    def test_closing(self):
        state = []
        class C:
            def close(self):
                state.append(1)
        x = C()
        self.assertEqual(state, [])
        with closing(x) as y:
            self.assertEqual(x, y)
        self.assertEqual(state, [1])

    def test_closing_error(self):
        state = []
        class C:
            def close(self):
                state.append(1)
        x = C()
        self.assertEqual(state, [])
        with self.assertRaises(ZeroDivisionError):
            with closing(x) as y:
                self.assertEqual(x, y)
                1 / 0
        self.assertEqual(state, [1])

class FileContextTestCase(unittest.TestCase):

    def testWithOpen(self):
        tfn = tempfile.mktemp()
        try:
            f = None
            with open(tfn, "w") as f:
                self.assertFalse(f.closed)
                f.write("Booh\n")
            self.assertTrue(f.closed)
            f = None
            with self.assertRaises(ZeroDivisionError):
                with open(tfn, "r") as f:
                    self.assertFalse(f.closed)
                    self.assertEqual(f.read(), "Booh\n")
                    1 / 0
            self.assertTrue(f.closed)
        finally:
            support.unlink(tfn)

@unittest.skipUnless(threading, 'Threading required for this test.')
class LockContextTestCase(unittest.TestCase):

    def boilerPlate(self, lock, locked):
        self.assertFalse(locked())
        with lock:
            self.assertTrue(locked())
        self.assertFalse(locked())
        with self.assertRaises(ZeroDivisionError):
            with lock:
                self.assertTrue(locked())
                1 / 0
        self.assertFalse(locked())

    def testWithLock(self):
        lock = threading.Lock()
        self.boilerPlate(lock, lock.locked)

    def testWithRLock(self):
        lock = threading.RLock()
        self.boilerPlate(lock, lock._is_owned)

    def testWithCondition(self):
        lock = threading.Condition()
        def locked():
            return lock._is_owned()
        self.boilerPlate(lock, locked)

    def testWithSemaphore(self):
        lock = threading.Semaphore()
        def locked():
            if lock.acquire(False):
                lock.release()
                return False
            else:
                return True
        self.boilerPlate(lock, locked)

    def testWithBoundedSemaphore(self):
        lock = threading.BoundedSemaphore()
        def locked():
            if lock.acquire(False):
                lock.release()
                return False
            else:
                return True
        self.boilerPlate(lock, locked)


class mycontext(ContextDecorator):
    started = False
    exc = None
    catch = False

    def __enter__(self):
        self.started = True
        return self

    def __exit__(self, *exc):
        self.exc = exc
        return self.catch


class TestContextDecorator(unittest.TestCase):

    def test_contextdecorator(self):
        context = mycontext()
        with context as result:
            self.assertIs(result, context)
            self.assertTrue(context.started)

        self.assertEqual(context.exc, (None, None, None))


    def test_contextdecorator_with_exception(self):
        context = mycontext()

        with self.assertRaisesRegex(NameError, 'foo'):
            with context:
                raise NameError('foo')
        self.assertIsNotNone(context.exc)
        self.assertIs(context.exc[0], NameError)

        context = mycontext()
        context.catch = True
        with context:
            raise NameError('foo')
        self.assertIsNotNone(context.exc)
        self.assertIs(context.exc[0], NameError)


    def test_decorator(self):
        context = mycontext()

        @context
        def test():
            self.assertIsNone(context.exc)
            self.assertTrue(context.started)
        test()
        self.assertEqual(context.exc, (None, None, None))


    def test_decorator_with_exception(self):
        context = mycontext()

        @context
        def test():
            self.assertIsNone(context.exc)
            self.assertTrue(context.started)
            raise NameError('foo')

        with self.assertRaisesRegex(NameError, 'foo'):
            test()
        self.assertIsNotNone(context.exc)
        self.assertIs(context.exc[0], NameError)


    def test_decorating_method(self):
        context = mycontext()

        class Test(object):

            @context
            def method(self, a, b, c=None):
                self.a = a
                self.b = b
                self.c = c

        # these tests are for argument passing when used as a decorator
        test = Test()
        test.method(1, 2)
        self.assertEqual(test.a, 1)
        self.assertEqual(test.b, 2)
        self.assertEqual(test.c, None)

        test = Test()
        test.method('a', 'b', 'c')
        self.assertEqual(test.a, 'a')
        self.assertEqual(test.b, 'b')
        self.assertEqual(test.c, 'c')

        test = Test()
        test.method(a=1, b=2)
        self.assertEqual(test.a, 1)
        self.assertEqual(test.b, 2)


    def test_typo_enter(self):
        class mycontext(ContextDecorator):
            def __unter__(self):
                pass
            def __exit__(self, *exc):
                pass

        with self.assertRaises(AttributeError):
            with mycontext():
                pass


    def test_typo_exit(self):
        class mycontext(ContextDecorator):
            def __enter__(self):
                pass
            def __uxit__(self, *exc):
                pass

        with self.assertRaises(AttributeError):
            with mycontext():
                pass


    def test_contextdecorator_as_mixin(self):
        class somecontext(object):
            started = False
            exc = None

            def __enter__(self):
                self.started = True
                return self

            def __exit__(self, *exc):
                self.exc = exc

        class mycontext(somecontext, ContextDecorator):
            pass

        context = mycontext()
        @context
        def test():
            self.assertIsNone(context.exc)
            self.assertTrue(context.started)
        test()
        self.assertEqual(context.exc, (None, None, None))


    def test_contextmanager_as_decorator(self):
        @contextmanager
        def woohoo(y):
            state.append(y)
            yield
            state.append(999)

        state = []
        @woohoo(1)
        def test(x):
            self.assertEqual(state, [1])
            state.append(x)
        test('something')
        self.assertEqual(state, [1, 'something', 999])

        # Issue #11647: Ensure the decorated function is 'reusable'
        state = []
        test('something else')
        self.assertEqual(state, [1, 'something else', 999])


class TestExitStack(unittest.TestCase):

    def test_no_resources(self):
        with ExitStack():
            pass

    def test_callback(self):
        expected = [
            ((), {}),
            ((1,), {}),
            ((1,2), {}),
            ((), dict(example=1)),
            ((1,), dict(example=1)),
            ((1,2), dict(example=1)),
        ]
        result = []
        def _exit(*args, **kwds):
            """Test metadata propagation"""
            result.append((args, kwds))
        with ExitStack() as stack:
            for args, kwds in reversed(expected):
                if args and kwds:
                    f = stack.callback(_exit, *args, **kwds)
                elif args:
                    f = stack.callback(_exit, *args)
                elif kwds:
                    f = stack.callback(_exit, **kwds)
                else:
                    f = stack.callback(_exit)
                self.assertIs(f, _exit)
            for wrapper in stack._exit_callbacks:
                self.assertIs(wrapper.__wrapped__, _exit)
                self.assertNotEqual(wrapper.__name__, _exit.__name__)
                self.assertIsNone(wrapper.__doc__, _exit.__doc__)
        self.assertEqual(result, expected)

    def test_push(self):
        exc_raised = ZeroDivisionError
        def _expect_exc(exc_type, exc, exc_tb):
            self.assertIs(exc_type, exc_raised)
        def _suppress_exc(*exc_details):
            return True
        def _expect_ok(exc_type, exc, exc_tb):
            self.assertIsNone(exc_type)
            self.assertIsNone(exc)
            self.assertIsNone(exc_tb)
        class ExitCM(object):
            def __init__(self, check_exc):
                self.check_exc = check_exc
            def __enter__(self):
                self.fail("Should not be called!")
            def __exit__(self, *exc_details):
                self.check_exc(*exc_details)
        with ExitStack() as stack:
            stack.push(_expect_ok)
            self.assertIs(stack._exit_callbacks[-1], _expect_ok)
            cm = ExitCM(_expect_ok)
            stack.push(cm)
            self.assertIs(stack._exit_callbacks[-1].__self__, cm)
            stack.push(_suppress_exc)
            self.assertIs(stack._exit_callbacks[-1], _suppress_exc)
            cm = ExitCM(_expect_exc)
            stack.push(cm)
            self.assertIs(stack._exit_callbacks[-1].__self__, cm)
            stack.push(_expect_exc)
            self.assertIs(stack._exit_callbacks[-1], _expect_exc)
            stack.push(_expect_exc)
            self.assertIs(stack._exit_callbacks[-1], _expect_exc)
            1/0

    def test_enter_context(self):
        class TestCM(object):
            def __enter__(self):
                result.append(1)
            def __exit__(self, *exc_details):
                result.append(3)

        result = []
        cm = TestCM()
        with ExitStack() as stack:
            @stack.callback  # Registered first => cleaned up last
            def _exit():
                result.append(4)
            self.assertIsNotNone(_exit)
            stack.enter_context(cm)
            self.assertIs(stack._exit_callbacks[-1].__self__, cm)
            result.append(2)
        self.assertEqual(result, [1, 2, 3, 4])

    def test_close(self):
        result = []
        with ExitStack() as stack:
            @stack.callback
            def _exit():
                result.append(1)
            self.assertIsNotNone(_exit)
            stack.close()
            result.append(2)
        self.assertEqual(result, [1, 2])

    def test_pop_all(self):
        result = []
        with ExitStack() as stack:
            @stack.callback
            def _exit():
                result.append(3)
            self.assertIsNotNone(_exit)
            new_stack = stack.pop_all()
            result.append(1)
        result.append(2)
        new_stack.close()
        self.assertEqual(result, [1, 2, 3])

    def test_exit_raise(self):
        with self.assertRaises(ZeroDivisionError):
            with ExitStack() as stack:
                stack.push(lambda *exc: False)
                1/0

    def test_exit_suppress(self):
        with ExitStack() as stack:
            stack.push(lambda *exc: True)
            1/0

    def test_exit_exception_chaining_reference(self):
        # Sanity check to make sure that ExitStack chaining matches
        # actual nested with statements
        class RaiseExc:
            def __init__(self, exc):
                self.exc = exc
            def __enter__(self):
                return self
            def __exit__(self, *exc_details):
                raise self.exc

        class SuppressExc:
            def __enter__(self):
                return self
            def __exit__(self, *exc_details):
                type(self).saved_details = exc_details
                return True

        try:
            with RaiseExc(IndexError):
                with RaiseExc(KeyError):
                    with RaiseExc(AttributeError):
                        with SuppressExc():
                            with RaiseExc(ValueError):
                                1 / 0
        except IndexError as exc:
            self.assertIsInstance(exc.__context__, KeyError)
            self.assertIsInstance(exc.__context__.__context__, AttributeError)
            # Inner exceptions were suppressed
            self.assertIsNone(exc.__context__.__context__.__context__)
        else:
            self.fail("Expected IndexError, but no exception was raised")
        # Check the inner exceptions
        inner_exc = SuppressExc.saved_details[1]
        self.assertIsInstance(inner_exc, ValueError)
        self.assertIsInstance(inner_exc.__context__, ZeroDivisionError)

    def test_exit_exception_chaining(self):
        # Ensure exception chaining matches the reference behaviour
        def raise_exc(exc):
            raise exc

        saved_details = None
        def suppress_exc(*exc_details):
            nonlocal saved_details
            saved_details = exc_details
            return True

        try:
            with ExitStack() as stack:
                stack.callback(raise_exc, IndexError)
                stack.callback(raise_exc, KeyError)
                stack.callback(raise_exc, AttributeError)
                stack.push(suppress_exc)
                stack.callback(raise_exc, ValueError)
                1 / 0
        except IndexError as exc:
            self.assertIsInstance(exc.__context__, KeyError)
            self.assertIsInstance(exc.__context__.__context__, AttributeError)
            # Inner exceptions were suppressed, but the with statement
            # cleanup code adds the one from the body back in as the
            # context of the exception raised by the outer callbacks
            # See http://bugs.python.org/issue14969
            suite_exc = exc.__context__.__context__.__context__
            self.assertIsInstance(suite_exc, ZeroDivisionError)
        else:
            self.fail("Expected IndexError, but no exception was raised")
        # Check the inner exceptions
        inner_exc = saved_details[1]
        self.assertIsInstance(inner_exc, ValueError)
        self.assertIsInstance(inner_exc.__context__, ZeroDivisionError)

    def test_exit_exception_chaining_suppress(self):
        with ExitStack() as stack:
            stack.push(lambda *exc: True)
            stack.push(lambda *exc: 1/0)
            stack.push(lambda *exc: {}[1])

    def test_instance_bypass(self):
        class Example(object): pass
        cm = Example()
        cm.__exit__ = object()
        stack = ExitStack()
        self.assertRaises(AttributeError, stack.enter_context, cm)
        stack.push(cm)
        self.assertIs(stack._exit_callbacks[-1], cm)


# This is needed to make the test actually run under regrtest.py!
def test_main():
    support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
