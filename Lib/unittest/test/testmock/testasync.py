import asyncio
import inspect
import unittest

from unittest.mock import call, AsyncMock, patch, MagicMock

class AsyncClass:
    def __init__(self):
        pass
    async def async_method(self):
        pass
    def normal_method(self):
        pass

async def async_func():
    pass

def normal_func():
    pass

class NormalClass(object):
    def a(self):
        pass


async_foo_name = f'{__name__}.AsyncClass'
normal_foo_name = f'{__name__}.NormalClass'


class AsyncPatchDecoratorTest(unittest.TestCase):
    def setUp(self):
        # Prevents altering the execution environment
        self.old_policy = asyncio.events._event_loop_policy

    def tearDown(self):
        # Restore the original event loop policy.
        asyncio.events._event_loop_policy = self.old_policy

    def test_is_coroutine_function_patch(self):
        @patch.object(AsyncClass, 'async_method')
        def test_async(mock_method):
            self.assertTrue(asyncio.iscoroutinefunction(mock_method))
        test_async()

    def test_is_async_patch(self):
        @patch.object(AsyncClass, 'async_method')
        def test_async(mock_method):
            m = mock_method()
            self.assertTrue(inspect.isawaitable(m))
            asyncio.run(m)

        @patch(f'{async_foo_name}.async_method')
        def test_no_parent_attribute(mock_method):
            m = mock_method()
            self.assertTrue(inspect.isawaitable(m))
            asyncio.run(m)

        test_async()
        test_no_parent_attribute()

    def test_is_AsyncMock_patch(self):
        @patch.object(AsyncClass, 'async_method')
        def test_async(mock_method):
            self.assertTrue(isinstance(mock_method, AsyncMock))

        test_async()


class AsyncPatchCMTest(unittest.TestCase):
    def setUp(self):
        self.old_policy = asyncio.events._event_loop_policy

    def tearDown(self):
        # Restore the original event loop policy.
        asyncio.events._event_loop_policy = self.old_policy

    def test_is_async_function_cm(self):
        def test_async():
            with patch.object(AsyncClass, 'async_method') as mock_method:
                self.assertTrue(asyncio.iscoroutinefunction(mock_method))

        test_async()

    def test_is_async_cm(self):
        def test_async():
            with patch.object(AsyncClass, 'async_method') as mock_method:
                m = mock_method()
                self.assertTrue(inspect.isawaitable(m))
                asyncio.run(m)

        test_async()

    def test_is_AsyncMock_cm(self):
        def test_async():
            with patch.object(AsyncClass, 'async_method') as mock_method:
                self.assertTrue(isinstance(mock_method, AsyncMock))

        test_async()


class AsyncMockTest(unittest.TestCase):
    def setUp(self):
        self.old_policy = asyncio.events._event_loop_policy

    def tearDown(self):
        # Restore the original event loop policy.
        asyncio.events._event_loop_policy = self.old_policy

    def test_iscoroutinefunction_default(self):
        mock = AsyncMock()
        self.assertTrue(asyncio.iscoroutinefunction(mock))

    def test_iscoroutinefunction_function(self):
        async def foo(): pass
        mock = AsyncMock(foo)
        self.assertTrue(asyncio.iscoroutinefunction(mock))
        self.assertTrue(inspect.iscoroutinefunction(mock))

    def test_isawaitable(self):
        mock = AsyncMock()
        m = mock()
        self.assertTrue(inspect.isawaitable(m))
        asyncio.run(m)
        self.assertIn('assert_awaited', dir(mock))

    def test_iscoroutinefunction_normal_function(self):
        def foo(): pass
        mock = AsyncMock(foo)
        self.assertTrue(asyncio.iscoroutinefunction(mock))
        self.assertTrue(inspect.iscoroutinefunction(mock))


class AsyncAutospecTest(unittest.TestCase):
    def test_is_AsyncMock_patch(self):
        @patch(async_foo_name, autospec=True)
        def test_async(mock_method):
            self.assertTrue(isinstance(
                             mock_method.async_method,
                             AsyncMock))
            self.assertTrue(isinstance(mock_method, MagicMock))

        @patch(async_foo_name, autospec=True)
        def test_normal_method(mock_method):
            self.assertTrue(isinstance(
                                mock_method.normal_method,
                                MagicMock))

        test_async()
        test_normal_method()


class AsyncSpecTest(unittest.TestCase):
    def setUp(self):
        self.old_policy = asyncio.events._event_loop_policy

    def tearDown(self):
        # Restore the original event loop policy.
        asyncio.events._event_loop_policy = self.old_policy
    def test_spec_as_async_positional_magicmock(self):
        mock = MagicMock(async_func)
        self.assertIsInstance(mock, MagicMock)
        m = mock()
        self.assertTrue(inspect.isawaitable(m))
        asyncio.run(m)

    def test_spec_as_async_kw_magicmock(self):
        mock = MagicMock(spec=async_func)
        self.assertIsInstance(mock, MagicMock)
        m = mock()
        self.assertTrue(inspect.isawaitable(m))
        asyncio.run(m)

    def test_spec_as_async_kw_AsyncMock(self):
        mock = AsyncMock(spec=async_func)
        self.assertIsInstance(mock, AsyncMock)
        m = mock()
        self.assertTrue(inspect.isawaitable(m))
        asyncio.run(m)

    def test_spec_as_async_positional_AsyncMock(self):
        mock = AsyncMock(async_func)
        self.assertIsInstance(mock, AsyncMock)
        m = mock()
        self.assertTrue(inspect.isawaitable(m))
        asyncio.run(m)

    def test_spec_as_normal_kw_AsyncMock(self):
        mock = AsyncMock(spec=normal_func)
        self.assertIsInstance(mock, AsyncMock)
        m = mock()
        self.assertTrue(inspect.isawaitable(m))
        asyncio.run(m)

    def test_spec_as_normal_positional_AsyncMock(self):
        mock = AsyncMock(normal_func)
        self.assertIsInstance(mock, AsyncMock)
        m = mock()
        self.assertTrue(inspect.isawaitable(m))
        asyncio.run(m)

    def test_spec_async_mock(self):
        @patch.object(AsyncClass, 'async_method', spec=True)
        def test_async(mock_method):
            self.assertTrue(isinstance(mock_method, AsyncMock))

        test_async()

    def test_spec_parent_not_async_attribute_is(self):
        @patch(async_foo_name, spec=True)
        def test_async(mock_method):
            self.assertTrue(isinstance(mock_method, MagicMock))
            self.assertTrue(isinstance(mock_method.async_method,
                                       AsyncMock))

        test_async()

    def test_target_async_spec_not(self):
        @patch.object(AsyncClass, 'async_method', spec=NormalClass.a)
        def test_async_attribute(mock_method):
            self.assertTrue(isinstance(mock_method, MagicMock))
            self.assertFalse(inspect.iscoroutine(mock_method))
            self.assertFalse(inspect.isawaitable(mock_method))

        test_async_attribute()

    def test_target_not_async_spec_is(self):
        @patch.object(NormalClass, 'a', spec=async_func)
        def test_attribute_not_async_spec_is(mock_async_func):
            self.assertTrue(isinstance(mock_async_func, AsyncMock))
        test_attribute_not_async_spec_is()

    def test_spec_async_attributes(self):
        @patch(normal_foo_name, spec=AsyncClass)
        def test_async_attributes_coroutines(MockNormalClass):
            self.assertTrue(isinstance(MockNormalClass.async_method,
                                       AsyncMock))
            self.assertTrue(isinstance(MockNormalClass, MagicMock))

        test_async_attributes_coroutines()


class AsyncSpecSetTest(unittest.TestCase):
    def test_is_AsyncMock_patch(self):
        @patch.object(AsyncClass, 'async_method', spec_set=True)
        def test_async(async_method):
            self.assertTrue(isinstance(async_method, AsyncMock))

    def test_is_async_AsyncMock(self):
        mock = AsyncMock(spec_set=AsyncClass.async_method)
        self.assertTrue(asyncio.iscoroutinefunction(mock))
        self.assertTrue(isinstance(mock, AsyncMock))

    def test_is_child_AsyncMock(self):
        mock = MagicMock(spec_set=AsyncClass)
        self.assertTrue(asyncio.iscoroutinefunction(mock.async_method))
        self.assertFalse(asyncio.iscoroutinefunction(mock.normal_method))
        self.assertTrue(isinstance(mock.async_method, AsyncMock))
        self.assertTrue(isinstance(mock.normal_method, MagicMock))
        self.assertTrue(isinstance(mock, MagicMock))


class AsyncArguments(unittest.TestCase):
    def setUp(self):
        self.old_policy = asyncio.events._event_loop_policy

    def tearDown(self):
        # Restore the original event loop policy.
        asyncio.events._event_loop_policy = self.old_policy

    def test_add_return_value(self):
        async def addition(self, var):
            return var + 1

        mock = AsyncMock(addition, return_value=10)
        output = asyncio.run(mock(5))

        self.assertEqual(output, 10)

    def test_add_side_effect_exception(self):
        async def addition(self, var):
            return var + 1
        mock = AsyncMock(addition, side_effect=Exception('err'))
        with self.assertRaises(Exception):
            asyncio.run(mock(5))


class AsyncMagicMethods(unittest.TestCase):
    class AsyncContextManager:
        def __init__(self):
            self.entered = False
            self.exited = False

        async def __aenter__(self, *args, **kwargs):
            self.entered = True
            return self

        async def __aexit__(self, *args, **kwargs):
            self.exited = True

    class AsyncIterator(object):
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


class AsyncMockAssert(unittest.TestCase):

    def setUp(self):
        self.mock = AsyncMock()
        self.old_policy = asyncio.events._event_loop_policy

    def tearDown(self):
        # Restore the original event loop policy.
        asyncio.events._event_loop_policy = self.old_policy

    async def _runnable_test(self, *args):
        if not args:
            await self.mock()
        else:
            await self.mock(*args)

    def test_assert_awaited(self):
        with self.assertRaises(AssertionError):
            self.mock.assert_awaited()

        asyncio.run(self._runnable_test())
        self.mock.assert_awaited()

    def test_assert_awaited_once(self):
        with self.assertRaises(AssertionError):
            self.mock.assert_awaited_once()

        asyncio.run(self._runnable_test())
        self.mock.assert_awaited_once()

        asyncio.run(self._runnable_test())
        with self.assertRaises(AssertionError):
            self.mock.assert_awaited_once()

    def test_assert_awaited_with(self):
        asyncio.run(self._runnable_test())
        with self.assertRaises(AssertionError):
            self.mock.assert_awaited_with('foo')

        asyncio.run(self._runnable_test('foo'))
        self.mock.assert_awaited_with('foo')

        asyncio.run(self._runnable_test('SomethingElse'))
        with self.assertRaises(AssertionError):
            self.mock.assert_awaited_with('foo')

    def test_assert_awaited_once_with(self):
        with self.assertRaises(AssertionError):
            self.mock.assert_awaited_once_with('foo')

        asyncio.run(self._runnable_test('foo'))
        self.mock.assert_awaited_once_with('foo')

        asyncio.run(self._runnable_test('foo'))
        with self.assertRaises(AssertionError):
            self.mock.assert_awaited_once_with('foo')

    def test_assert_any_wait(self):
        with self.assertRaises(AssertionError):
            self.mock.assert_any_await('NormalFoo')

        asyncio.run(self._runnable_test('foo'))
        with self.assertRaises(AssertionError):
            self.mock.assert_any_await('NormalFoo')

        asyncio.run(self._runnable_test('NormalFoo'))
        self.mock.assert_any_await('NormalFoo')

        asyncio.run(self._runnable_test('SomethingElse'))
        self.mock.assert_any_await('NormalFoo')

    def test_assert_has_awaits_no_order(self):
        calls = [call('NormalFoo'), call('baz')]

        with self.assertRaises(AssertionError):
            self.mock.assert_has_awaits(calls)

        asyncio.run(self._runnable_test('foo'))
        with self.assertRaises(AssertionError):
            self.mock.assert_has_awaits(calls)

        asyncio.run(self._runnable_test('NormalFoo'))
        with self.assertRaises(AssertionError):
            self.mock.assert_has_awaits(calls)

        asyncio.run(self._runnable_test('baz'))
        self.mock.assert_has_awaits(calls)

        asyncio.run(self._runnable_test('SomethingElse'))
        self.mock.assert_has_awaits(calls)

    def test_assert_has_awaits_ordered(self):
        calls = [call('NormalFoo'), call('baz')]
        with self.assertRaises(AssertionError):
            self.mock.assert_has_awaits(calls, any_order=True)

        asyncio.run(self._runnable_test('baz'))
        with self.assertRaises(AssertionError):
            self.mock.assert_has_awaits(calls, any_order=True)

        asyncio.run(self._runnable_test('foo'))
        with self.assertRaises(AssertionError):
            self.mock.assert_has_awaits(calls, any_order=True)

        asyncio.run(self._runnable_test('NormalFoo'))
        self.mock.assert_has_awaits(calls, any_order=True)

        asyncio.run(self._runnable_test('qux'))
        self.mock.assert_has_awaits(calls, any_order=True)

    def test_assert_not_awaited(self):
        self.mock.assert_not_awaited()

        asyncio.run(self._runnable_test())
        with self.assertRaises(AssertionError):
            self.mock.assert_not_awaited()
