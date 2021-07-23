import asyncio
from contextlib import (
    asynccontextmanager, AbstractAsyncContextManager,
    AsyncExitStack, nullcontext, aclosing)
import functools
from test import support
import unittest

from test.test_contextlib import TestBaseExitStack


def _async_test(func):
    """Decorator to turn an async function into a test case."""
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        coro = func(*args, **kwargs)
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        try:
            return loop.run_until_complete(coro)
        finally:
            loop.close()
            asyncio.set_event_loop_policy(None)
    return wrapper


class TestAbstractAsyncContextManager(unittest.TestCase):

    @_async_test
    async def test_enter(self):
        class DefaultEnter(AbstractAsyncContextManager):
            async def __aexit__(self, *args):
                await super().__aexit__(*args)

        manager = DefaultEnter()
        self.assertIs(await manager.__aenter__(), manager)

        async with manager as context:
            self.assertIs(manager, context)

    @_async_test
    async def test_async_gen_propagates_generator_exit(self):
        # A regression test for https://bugs.python.org/issue33786.

        @asynccontextmanager
        async def ctx():
            yield

        async def gen():
            async with ctx():
                yield 11

        ret = []
        exc = ValueError(22)
        with self.assertRaises(ValueError):
            async with ctx():
                async for val in gen():
                    ret.append(val)
                    raise exc

        self.assertEqual(ret, [11])

    def test_exit_is_abstract(self):
        class MissingAexit(AbstractAsyncContextManager):
            pass

        with self.assertRaises(TypeError):
            MissingAexit()

    def test_structural_subclassing(self):
        class ManagerFromScratch:
            async def __aenter__(self):
                return self
            async def __aexit__(self, exc_type, exc_value, traceback):
                return None

        self.assertTrue(issubclass(ManagerFromScratch, AbstractAsyncContextManager))

        class DefaultEnter(AbstractAsyncContextManager):
            async def __aexit__(self, *args):
                await super().__aexit__(*args)

        self.assertTrue(issubclass(DefaultEnter, AbstractAsyncContextManager))

        class NoneAenter(ManagerFromScratch):
            __aenter__ = None

        self.assertFalse(issubclass(NoneAenter, AbstractAsyncContextManager))

        class NoneAexit(ManagerFromScratch):
            __aexit__ = None

        self.assertFalse(issubclass(NoneAexit, AbstractAsyncContextManager))


class AsyncContextManagerTestCase(unittest.TestCase):

    @_async_test
    async def test_contextmanager_plain(self):
        state = []
        @asynccontextmanager
        async def woohoo():
            state.append(1)
            yield 42
            state.append(999)
        async with woohoo() as x:
            self.assertEqual(state, [1])
            self.assertEqual(x, 42)
            state.append(x)
        self.assertEqual(state, [1, 42, 999])

    @_async_test
    async def test_contextmanager_finally(self):
        state = []
        @asynccontextmanager
        async def woohoo():
            state.append(1)
            try:
                yield 42
            finally:
                state.append(999)
        with self.assertRaises(ZeroDivisionError):
            async with woohoo() as x:
                self.assertEqual(state, [1])
                self.assertEqual(x, 42)
                state.append(x)
                raise ZeroDivisionError()
        self.assertEqual(state, [1, 42, 999])

    @_async_test
    async def test_contextmanager_no_reraise(self):
        @asynccontextmanager
        async def whee():
            yield
        ctx = whee()
        await ctx.__aenter__()
        # Calling __aexit__ should not result in an exception
        self.assertFalse(await ctx.__aexit__(TypeError, TypeError("foo"), None))

    @_async_test
    async def test_contextmanager_trap_yield_after_throw(self):
        @asynccontextmanager
        async def whoo():
            try:
                yield
            except:
                yield
        ctx = whoo()
        await ctx.__aenter__()
        with self.assertRaises(RuntimeError):
            await ctx.__aexit__(TypeError, TypeError('foo'), None)

    @_async_test
    async def test_contextmanager_trap_no_yield(self):
        @asynccontextmanager
        async def whoo():
            if False:
                yield
        ctx = whoo()
        with self.assertRaises(RuntimeError):
            await ctx.__aenter__()

    @_async_test
    async def test_contextmanager_trap_second_yield(self):
        @asynccontextmanager
        async def whoo():
            yield
            yield
        ctx = whoo()
        await ctx.__aenter__()
        with self.assertRaises(RuntimeError):
            await ctx.__aexit__(None, None, None)

    @_async_test
    async def test_contextmanager_non_normalised(self):
        @asynccontextmanager
        async def whoo():
            try:
                yield
            except RuntimeError:
                raise SyntaxError

        ctx = whoo()
        await ctx.__aenter__()
        with self.assertRaises(SyntaxError):
            await ctx.__aexit__(RuntimeError, None, None)

    @_async_test
    async def test_contextmanager_except(self):
        state = []
        @asynccontextmanager
        async def woohoo():
            state.append(1)
            try:
                yield 42
            except ZeroDivisionError as e:
                state.append(e.args[0])
                self.assertEqual(state, [1, 42, 999])
        async with woohoo() as x:
            self.assertEqual(state, [1])
            self.assertEqual(x, 42)
            state.append(x)
            raise ZeroDivisionError(999)
        self.assertEqual(state, [1, 42, 999])

    @_async_test
    async def test_contextmanager_except_stopiter(self):
        @asynccontextmanager
        async def woohoo():
            yield

        class StopIterationSubclass(StopIteration):
            pass

        class StopAsyncIterationSubclass(StopAsyncIteration):
            pass

        for stop_exc in (
            StopIteration('spam'),
            StopAsyncIteration('ham'),
            StopIterationSubclass('spam'),
            StopAsyncIterationSubclass('spam')
        ):
            with self.subTest(type=type(stop_exc)):
                try:
                    async with woohoo():
                        raise stop_exc
                except Exception as ex:
                    self.assertIs(ex, stop_exc)
                else:
                    self.fail(f'{stop_exc} was suppressed')

    @_async_test
    async def test_contextmanager_wrap_runtimeerror(self):
        @asynccontextmanager
        async def woohoo():
            try:
                yield
            except Exception as exc:
                raise RuntimeError(f'caught {exc}') from exc

        with self.assertRaises(RuntimeError):
            async with woohoo():
                1 / 0

        # If the context manager wrapped StopAsyncIteration in a RuntimeError,
        # we also unwrap it, because we can't tell whether the wrapping was
        # done by the generator machinery or by the generator itself.
        with self.assertRaises(StopAsyncIteration):
            async with woohoo():
                raise StopAsyncIteration

    def _create_contextmanager_attribs(self):
        def attribs(**kw):
            def decorate(func):
                for k,v in kw.items():
                    setattr(func,k,v)
                return func
            return decorate
        @asynccontextmanager
        @attribs(foo='bar')
        async def baz(spam):
            """Whee!"""
            yield
        return baz

    def test_contextmanager_attribs(self):
        baz = self._create_contextmanager_attribs()
        self.assertEqual(baz.__name__,'baz')
        self.assertEqual(baz.foo, 'bar')

    @support.requires_docstrings
    def test_contextmanager_doc_attrib(self):
        baz = self._create_contextmanager_attribs()
        self.assertEqual(baz.__doc__, "Whee!")

    @support.requires_docstrings
    @_async_test
    async def test_instance_docstring_given_cm_docstring(self):
        baz = self._create_contextmanager_attribs()(None)
        self.assertEqual(baz.__doc__, "Whee!")
        async with baz:
            pass  # suppress warning

    @_async_test
    async def test_keywords(self):
        # Ensure no keyword arguments are inhibited
        @asynccontextmanager
        async def woohoo(self, func, args, kwds):
            yield (self, func, args, kwds)
        async with woohoo(self=11, func=22, args=33, kwds=44) as target:
            self.assertEqual(target, (11, 22, 33, 44))

    @_async_test
    async def test_recursive(self):
        depth = 0
        ncols = 0

        @asynccontextmanager
        async def woohoo():
            nonlocal ncols
            ncols += 1

            nonlocal depth
            before = depth
            depth += 1
            yield
            depth -= 1
            self.assertEqual(depth, before)

        @woohoo()
        async def recursive():
            if depth < 10:
                await recursive()

        await recursive()

        self.assertEqual(ncols, 10)
        self.assertEqual(depth, 0)


class AclosingTestCase(unittest.TestCase):

    @support.requires_docstrings
    def test_instance_docs(self):
        cm_docstring = aclosing.__doc__
        obj = aclosing(None)
        self.assertEqual(obj.__doc__, cm_docstring)

    @_async_test
    async def test_aclosing(self):
        state = []
        class C:
            async def aclose(self):
                state.append(1)
        x = C()
        self.assertEqual(state, [])
        async with aclosing(x) as y:
            self.assertEqual(x, y)
        self.assertEqual(state, [1])

    @_async_test
    async def test_aclosing_error(self):
        state = []
        class C:
            async def aclose(self):
                state.append(1)
        x = C()
        self.assertEqual(state, [])
        with self.assertRaises(ZeroDivisionError):
            async with aclosing(x) as y:
                self.assertEqual(x, y)
                1 / 0
        self.assertEqual(state, [1])

    @_async_test
    async def test_aclosing_bpo41229(self):
        state = []

        class Resource:
            def __del__(self):
                state.append(1)

        async def agenfunc():
            r = Resource()
            yield -1
            yield -2

        x = agenfunc()
        self.assertEqual(state, [])
        with self.assertRaises(ZeroDivisionError):
            async with aclosing(x) as y:
                self.assertEqual(x, y)
                self.assertEqual(-1, await x.__anext__())
                1 / 0
        self.assertEqual(state, [1])


class TestAsyncExitStack(TestBaseExitStack, unittest.TestCase):
    class SyncAsyncExitStack(AsyncExitStack):
        @staticmethod
        def run_coroutine(coro):
            loop = asyncio.get_event_loop_policy().get_event_loop()
            t = loop.create_task(coro)
            t.add_done_callback(lambda f: loop.stop())
            loop.run_forever()

            exc = t.exception()
            if not exc:
                return t.result()
            else:
                context = exc.__context__

                try:
                    raise exc
                except:
                    exc.__context__ = context
                    raise exc

        def close(self):
            return self.run_coroutine(self.aclose())

        def __enter__(self):
            return self.run_coroutine(self.__aenter__())

        def __exit__(self, *exc_details):
            return self.run_coroutine(self.__aexit__(*exc_details))

    exit_stack = SyncAsyncExitStack

    def setUp(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        self.addCleanup(self.loop.close)
        self.addCleanup(asyncio.set_event_loop_policy, None)

    @_async_test
    async def test_async_callback(self):
        expected = [
            ((), {}),
            ((1,), {}),
            ((1,2), {}),
            ((), dict(example=1)),
            ((1,), dict(example=1)),
            ((1,2), dict(example=1)),
        ]
        result = []
        async def _exit(*args, **kwds):
            """Test metadata propagation"""
            result.append((args, kwds))

        async with AsyncExitStack() as stack:
            for args, kwds in reversed(expected):
                if args and kwds:
                    f = stack.push_async_callback(_exit, *args, **kwds)
                elif args:
                    f = stack.push_async_callback(_exit, *args)
                elif kwds:
                    f = stack.push_async_callback(_exit, **kwds)
                else:
                    f = stack.push_async_callback(_exit)
                self.assertIs(f, _exit)
            for wrapper in stack._exit_callbacks:
                self.assertIs(wrapper[1].__wrapped__, _exit)
                self.assertNotEqual(wrapper[1].__name__, _exit.__name__)
                self.assertIsNone(wrapper[1].__doc__, _exit.__doc__)

        self.assertEqual(result, expected)

        result = []
        async with AsyncExitStack() as stack:
            with self.assertRaises(TypeError):
                stack.push_async_callback(arg=1)
            with self.assertRaises(TypeError):
                self.exit_stack.push_async_callback(arg=2)
            with self.assertRaises(TypeError):
                stack.push_async_callback(callback=_exit, arg=3)
        self.assertEqual(result, [])

    @_async_test
    async def test_async_push(self):
        exc_raised = ZeroDivisionError
        async def _expect_exc(exc_type, exc, exc_tb):
            self.assertIs(exc_type, exc_raised)
        async def _suppress_exc(*exc_details):
            return True
        async def _expect_ok(exc_type, exc, exc_tb):
            self.assertIsNone(exc_type)
            self.assertIsNone(exc)
            self.assertIsNone(exc_tb)
        class ExitCM(object):
            def __init__(self, check_exc):
                self.check_exc = check_exc
            async def __aenter__(self):
                self.fail("Should not be called!")
            async def __aexit__(self, *exc_details):
                await self.check_exc(*exc_details)

        async with self.exit_stack() as stack:
            stack.push_async_exit(_expect_ok)
            self.assertIs(stack._exit_callbacks[-1][1], _expect_ok)
            cm = ExitCM(_expect_ok)
            stack.push_async_exit(cm)
            self.assertIs(stack._exit_callbacks[-1][1].__self__, cm)
            stack.push_async_exit(_suppress_exc)
            self.assertIs(stack._exit_callbacks[-1][1], _suppress_exc)
            cm = ExitCM(_expect_exc)
            stack.push_async_exit(cm)
            self.assertIs(stack._exit_callbacks[-1][1].__self__, cm)
            stack.push_async_exit(_expect_exc)
            self.assertIs(stack._exit_callbacks[-1][1], _expect_exc)
            stack.push_async_exit(_expect_exc)
            self.assertIs(stack._exit_callbacks[-1][1], _expect_exc)
            1/0

    @_async_test
    async def test_enter_async_context(self):
        class TestCM(object):
            async def __aenter__(self):
                result.append(1)
            async def __aexit__(self, *exc_details):
                result.append(3)

        result = []
        cm = TestCM()

        async with AsyncExitStack() as stack:
            @stack.push_async_callback  # Registered first => cleaned up last
            async def _exit():
                result.append(4)
            self.assertIsNotNone(_exit)
            await stack.enter_async_context(cm)
            self.assertIs(stack._exit_callbacks[-1][1].__self__, cm)
            result.append(2)

        self.assertEqual(result, [1, 2, 3, 4])

    @_async_test
    async def test_enter_async_context_errors(self):
        class LacksEnterAndExit:
            pass
        class LacksEnter:
            async def __aexit__(self, *exc_info):
                pass
        class LacksExit:
            async def __aenter__(self):
                pass

        async with self.exit_stack() as stack:
            with self.assertRaisesRegex(TypeError, 'asynchronous context manager'):
                await stack.enter_async_context(LacksEnterAndExit())
            with self.assertRaisesRegex(TypeError, 'asynchronous context manager'):
                await stack.enter_async_context(LacksEnter())
            with self.assertRaisesRegex(TypeError, 'asynchronous context manager'):
                await stack.enter_async_context(LacksExit())
            self.assertFalse(stack._exit_callbacks)

    @_async_test
    async def test_async_exit_exception_chaining(self):
        # Ensure exception chaining matches the reference behaviour
        async def raise_exc(exc):
            raise exc

        saved_details = None
        async def suppress_exc(*exc_details):
            nonlocal saved_details
            saved_details = exc_details
            return True

        try:
            async with self.exit_stack() as stack:
                stack.push_async_callback(raise_exc, IndexError)
                stack.push_async_callback(raise_exc, KeyError)
                stack.push_async_callback(raise_exc, AttributeError)
                stack.push_async_exit(suppress_exc)
                stack.push_async_callback(raise_exc, ValueError)
                1 / 0
        except IndexError as exc:
            self.assertIsInstance(exc.__context__, KeyError)
            self.assertIsInstance(exc.__context__.__context__, AttributeError)
            # Inner exceptions were suppressed
            self.assertIsNone(exc.__context__.__context__.__context__)
        else:
            self.fail("Expected IndexError, but no exception was raised")
        # Check the inner exceptions
        inner_exc = saved_details[1]
        self.assertIsInstance(inner_exc, ValueError)
        self.assertIsInstance(inner_exc.__context__, ZeroDivisionError)

    @_async_test
    async def test_instance_bypass_async(self):
        class Example(object): pass
        cm = Example()
        cm.__aenter__ = object()
        cm.__aexit__ = object()
        stack = self.exit_stack()
        with self.assertRaisesRegex(TypeError, 'asynchronous context manager'):
            await stack.enter_async_context(cm)
        stack.push_async_exit(cm)
        self.assertIs(stack._exit_callbacks[-1][1], cm)


class TestAsyncNullcontext(unittest.TestCase):
    @_async_test
    async def test_async_nullcontext(self):
        class C:
            pass
        c = C()
        async with nullcontext(c) as c_in:
            self.assertIs(c_in, c)


if __name__ == '__main__':
    unittest.main()
