import asyncio
import inspect
import unittest

from unittest.mock import call, CoroutineMock, patch, MagicMock


class AsyncFoo(object):
    def __init__(self, a):
        pass
    async def coroutine_method(self):
        pass
    @asyncio.coroutine
    def decorated_cr_method(self):
        pass

async def coroutine_func():
    pass

class NormalFoo(object):
    def a(self):
        pass


async_foo_name = f'{__name__}.AsyncFoo'
normal_foo_name = f'{__name__}.NormalFoo'


def run_coroutine(coroutine):
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    try:
        return loop.run_until_complete(coroutine)
    finally:
        loop.close()


class CoroutinePatchDecoratorTest(unittest.TestCase):
    def test_is_coroutine_function_patch(self):
        @patch.object(AsyncFoo, 'coroutine_method')
        def test_async(mock_method):
            self.assertTrue(asyncio.iscoroutinefunction(mock_method))

        @patch.object(AsyncFoo, 'decorated_cr_method')
        def test_async_decorator(mock_g):
            self.assertTrue(asyncio.iscoroutinefunction(mock_g))

        test_async()
        test_async_decorator()

    def test_is_coroutine_patch(self):
        @patch.object(AsyncFoo, 'coroutine_method')
        def test_async(mock_method):
            self.assertTrue(asyncio.iscoroutine(mock_method()))

        @patch.object(AsyncFoo, 'decorated_cr_method')
        def test_async_decorator(mock_method):
            self.assertTrue(asyncio.iscoroutine(mock_method()))

        @patch(f'{async_foo_name}.coroutine_method')
        def test_no_parent_attribute(mock_method):
            self.assertTrue(asyncio.iscoroutine(mock_method()))

        test_async()
        test_async_decorator()
        test_no_parent_attribute()

    def test_is_coroutinemock_patch(self):
        @patch.object(AsyncFoo, 'coroutine_method')
        def test_async(mock_method):
            self.assertTrue(isinstance(mock_method, CoroutineMock))

        @patch.object(AsyncFoo, 'decorated_cr_method')
        def test_async_decorator(mock_method):
            self.assertTrue(isinstance(mock_method, CoroutineMock))

        test_async()
        test_async_decorator()


class CoroutinePatchCMTest(unittest.TestCase):
    def test_is_coroutine_function_cm(self):
        def test_async():
            with patch.object(AsyncFoo, 'coroutine_method') as mock_method:
                self.assertTrue(asyncio.iscoroutinefunction(mock_method))

        def test_async_decorator():
            with patch.object(AsyncFoo, 'decorated_cr_method') as mock_method:
                self.assertTrue(asyncio.iscoroutinefunction(mock_method))

        test_async()
        test_async_decorator()

    def test_is_coroutine_cm(self):
        def test_async():
            with patch.object(AsyncFoo, 'coroutine_method') as mock_method:
                self.assertTrue(asyncio.iscoroutine(mock_method()))

        def test_async_decorator():
            with patch.object(AsyncFoo, 'decorated_cr_method') as mock_method:
                self.assertTrue(asyncio.iscoroutine(mock_method()))
        test_async()
        test_async_decorator()

    def test_is_coroutinemock_cm(self):
        def test_async():
            with patch.object(AsyncFoo, 'coroutine_method') as mock_method:
                self.assertTrue(isinstance(mock_method, CoroutineMock))

        def test_async_decorator():
            with patch.object(AsyncFoo, 'decorated_cr_method') as mock_method:
                self.assertTrue(isinstance(mock_method, CoroutineMock))

        test_async()
        test_async_decorator()


class CoroutineMockTest(unittest.TestCase):
    def test_iscoroutinefunction_default(self):
        mock = CoroutineMock()
        self.assertTrue(asyncio.iscoroutinefunction(mock))

    def test_iscoroutinefunction_function(self):
        async def foo(): pass
        mock = CoroutineMock(foo)
        self.assertTrue(asyncio.iscoroutinefunction(mock))
        self.assertTrue(inspect.iscoroutinefunction(mock))

    def test_iscoroutine(self):
        mock = CoroutineMock()
        self.assertTrue(asyncio.iscoroutine(mock()))
        self.assertIn('assert_awaited', dir(mock))

    # Should I test making a non-coroutine a coroutine mock?


class CoroutineAutospecTest(unittest.TestCase):
    def test_is_coroutinemock_patch(self):
        @patch(async_foo_name, autospec=True)
        def test_async(mock_method):
            self.assertTrue(isinstance(
                             mock_method.decorated_cr_method,
                             CoroutineMock))
            self.assertTrue(isinstance(mock_method, MagicMock))

        @patch(async_foo_name, autospec=True)
        def test_async_decorator(mock_method):
            self.assertTrue(isinstance(
                                mock_method.decorated_cr_method,
                                CoroutineMock))
            self.assertTrue(isinstance(mock_method, MagicMock))

        test_async()
        test_async_decorator()


class CoroutineSpecTest(unittest.TestCase):
    def test_spec_as_coroutine_positional(self):
        mock = MagicMock(coroutine_func)
        self.assertIsInstance(mock, MagicMock)  # Is this what we want?
        self.assertTrue(asyncio.iscoroutine(mock()))

    def test_spec_as_coroutine_kw(self):
        mock = MagicMock(spec=coroutine_func)
        self.assertIsInstance(mock, MagicMock)
        self.assertTrue(asyncio.iscoroutine(mock()))

    def test_spec_coroutine_mock(self):
        @patch.object(AsyncFoo, 'coroutine_method', spec=True)
        def test_async(mock_method):
            self.assertTrue(isinstance(mock_method, CoroutineMock))

        test_async()

    def test_spec_parent_not_coroutine_attribute_is(self):
        @patch(async_foo_name, spec=True)
        def test_async(mock_method):
            self.assertTrue(isinstance(mock_method, MagicMock))
            self.assertTrue(isinstance(mock_method.coroutine_method,
                                       CoroutineMock))

        test_async()

    def test_target_coroutine_spec_not(self):
        @patch.object(AsyncFoo, 'coroutine_method', spec=NormalFoo.a)
        def test_async_attribute(mock_method):
            self.assertTrue(isinstance(mock_method, MagicMock))
            self.assertFalse(inspect.iscoroutinefunction(mock_method))

        test_async_attribute()

    def test_target_not_coroutine_spec_is(self):
        @patch.object(NormalFoo, 'a', spec=coroutine_func)
        def test_attribute_not_coroutine_spec_is(mock_coroutine_func):
            self.assertTrue(isinstance(mock_coroutine_func, CoroutineMock))
            # should check here is there is coroutine functionality
        test_attribute_not_coroutine_spec_is()

    def test_spec_coroutine_attributes(self):
        @patch(normal_foo_name, spec=AsyncFoo)
        def test_coroutine_attributes_coroutines(MockNormalFoo):
            self.assertTrue(isinstance(MockNormalFoo.coroutine_method,
                                       CoroutineMock))
            self.assertTrue(isinstance(MockNormalFoo, MagicMock))

        test_coroutine_attributes_coroutines()


class CoroutineSpecSetTest(unittest.TestCase):
    def test_is_coroutinemock_patch(self):
        @patch.object(AsyncFoo, 'coroutine_method', spec_set=True)
        def test_async(coroutine_method):
            self.assertTrue(isinstance(coroutine_method, CoroutineMock))

    def test_is_coroutine_coroutinemock(self):
        mock = CoroutineMock(spec_set=AsyncFoo.coroutine_method)
        self.assertTrue(asyncio.iscoroutinefunction(mock))
        self.assertTrue(isinstance(mock, CoroutineMock))

    def test_is_child_coroutinemock(self):
        mock = MagicMock(spec_set=AsyncFoo)
        self.assertTrue(asyncio.iscoroutinefunction(mock.coroutine_method))
        self.assertTrue(isinstance(mock.coroutine_method, CoroutineMock))
        self.assertTrue(isinstance(mock, MagicMock))


class CoroutineMagicMethods(unittest.TestCase):
    class AsyncContextManager:
        def __init__(self):
            self.entered = False
            self.exited = False

        async def __aenter__(self, *args, **kwargs):
            self.entered = True
            return self

        async def __aexit__(self, *args, **kwargs):
            self.exited = True

    class AsyncIterator:
        def __init__(self):
            self.iter_called = False
            self.next_called = False
            self.items = ["foo", "NormalFoo", "baz"]

        def __aiter__(self):
            return self

        async def __anext__(self):
            try:
                return self.items.pop()
            except IndexError:
                pass

            raise StopAsyncIteration

    def test_mock_magic_methods_are_coroutine_mocks(self):
        mock_instance = CoroutineMock(spec=self.AsyncContextManager())
        self.assertIsInstance(mock_instance.__aenter__,
                              CoroutineMock)
        self.assertIsInstance(mock_instance.__aexit__,
                              CoroutineMock)

    def test_mock_aiter_and_anext(self):
        instance = self.AsyncIterator()
        mock_instance = CoroutineMock(instance)

        self.assertEqual(asyncio.iscoroutine(instance.__aiter__),
                         asyncio.iscoroutine(mock_instance.__aiter__))
        self.assertEqual(asyncio.iscoroutine(instance.__anext__),
                         asyncio.iscoroutine(mock_instance.__anext__))

        iterator = instance.__aiter__()
        if asyncio.iscoroutine(iterator):
            iterator = run_coroutine(iterator)

        mock_iterator = mock_instance.__aiter__()
        if asyncio.iscoroutine(mock_iterator):
            mock_iterator = run_coroutine(mock_iterator)

        self.assertEqual(asyncio.iscoroutine(iterator.__aiter__),
                         asyncio.iscoroutine(mock_iterator.__aiter__))
        self.assertEqual(asyncio.iscoroutine(iterator.__anext__),
                         asyncio.iscoroutine(mock_iterator.__anext__))


class CoroutineMockAssert(unittest.TestCase):
    @asyncio.coroutine
    def test_assert_awaited(self):
        mock = CoroutineMock()

        with self.assertRaises(AssertionError):
            mock.assert_awaited()

        yield from mock()
        mock.assert_awaited()

    @asyncio.coroutine
    def test_assert_awaited_once(self):
        mock = CoroutineMock()

        with self.assertRaises(AssertionError):
            mock.assert_awaited_once()

        yield from mock()
        mock.assert_awaited_once()

        yield from mock()
        with self.assertRaises(AssertionError):
            mock.assert_awaited_once()

    @asyncio.coroutine
    def test_assert_awaited_with(self):
        mock = CoroutineMock()

        with self.assertRaises(AssertionError):
            mock.assert_awaited_with('foo')

        yield from mock('foo')
        mock.assert_awaited_with('foo')

        yield from mock('NormalFoo')
        with self.assertRaises(AssertionError):
            mock.assert_awaited_with('foo')

    @asyncio.coroutine
    def test_assert_awaited_once_with(self):
        mock = CoroutineMock()

        with self.assertRaises(AssertionError):
            mock.assert_awaited_once_with('foo')

        yield from mock('foo')
        mock.assert_awaited_once_with('foo')

        yield from mock('foo')
        with self.assertRaises(AssertionError):
            mock.assert_awaited_once_with('foo')

    @asyncio.coroutine
    def test_assert_any_wait(self):
        mock = CoroutineMock()

        with self.assertRaises(AssertionError):
            mock.assert_any_await('NormalFoo')

        yield from mock('foo')
        with self.assertRaises(AssertionError):
            mock.assert_any_await('NormalFoo')

        yield from mock('NormalFoo')
        mock.assert_any_await('NormalFoo')

        yield from mock('baz')
        mock.assert_any_await('NormalFoo')

    @asyncio.coroutine
    def test_assert_has_awaits(self):
        calls = [call('NormalFoo'), call('baz')]

        with self.subTest('any_order=False'):
            mock = CoroutineMock()

            with self.assertRaises(AssertionError):
                mock.assert_has_awaits(calls)

            yield from mock('foo')
            with self.assertRaises(AssertionError):
                mock.assert_has_awaits(calls)

            yield from mock('NormalFoo')
            with self.assertRaises(AssertionError):
                mock.assert_has_awaits(calls)

            yield from mock('baz')
            mock.assert_has_awaits(calls)

            yield from mock('qux')
            mock.assert_has_awaits(calls)

        with self.subTest('any_order=True'):
            mock = CoroutineMock()

            with self.assertRaises(AssertionError):
                mock.assert_has_awaits(calls, any_order=True)

            yield from mock('baz')
            with self.assertRaises(AssertionError):
                mock.assert_has_awaits(calls, any_order=True)

            yield from mock('foo')
            with self.assertRaises(AssertionError):
                mock.assert_has_awaits(calls, any_order=True)

            yield from mock('NormalFoo')
            mock.assert_has_awaits(calls, any_order=True)

            yield from mock('qux')
            mock.assert_has_awaits(calls, any_order=True)

    @asyncio.coroutine
    def test_assert_not_awaited(self):
        mock = CoroutineMock()

        mock.assert_not_awaited()

        yield from mock()
        with self.assertRaises(AssertionError):
            mock.assert_not_awaited()
