import asyncio
from contextlib import asynccontextmanager
from test import support
import unittest


def _async_test(func):
    """Decorator to turn an async function into a test case."""
    def wrapper(*args, **kwargs):
        loop = asyncio.new_event_loop()
        coro = func(*args, **kwargs)
        return loop.run_until_complete(coro)
    return wrapper


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
        # Calling __exit__ should not result in an exception
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
        stop_exc = StopIteration('spam')
        @asynccontextmanager
        async def woohoo():
            yield
        try:
            async with woohoo():
                raise stop_exc
        except Exception as ex:
            self.assertIs(ex, stop_exc)
        else:
            self.fail('StopIteration was suppressed')

    @_async_test
    async def test_contextmanager_except_stopasynciter(self):
        stop_exc = StopAsyncIteration('spam')
        @asynccontextmanager
        async def woohoo():
            yield
        try:
            async with woohoo():
                raise stop_exc
        except Exception as ex:
            self.assertIs(ex, stop_exc)
        else:
            self.fail('StopAsyncIteration was suppressed')

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
