import asyncio
from contextlib import asynccontextmanager, AbstractAsyncContextManager
import functools
from test import support
import unittest


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
            asyncio.set_event_loop(None)
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

        for stop_exc in (StopIteration('spam'), StopAsyncIteration('ham')):
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


if __name__ == '__main__':
    unittest.main()
