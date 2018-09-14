import asyncio
import inspect
import sys
import unittest

from unittest.mock import call, CoroutineMock, patch, MagicMock


class AsyncClass(object):
    def __init__(self, a):
        pass
    async def coroutine_method(self):
        pass
    @asyncio.coroutine
    def decorated_cr_method(self):
        pass
    def normal_method(self):
        pass

async def coroutine_func():
    pass

def normal_func():
    pass

class NormalClass(object):
    def a(self):
        pass


async_foo_name = f'{__name__}.AsyncClass'
normal_foo_name = f'{__name__}.NormalClass'


def run_coroutine(coroutine):
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    try:
        return loop.run_until_complete(coroutine)
    finally:
        loop.close()


class CoroutinePatchDecoratorTest(unittest.TestCase):
    def test_is_coroutine_function_patch(self):
        @patch.object(AsyncClass, 'coroutine_method')
        def test_async(mock_method):
            self.assertTrue(asyncio.iscoroutinefunction(mock_method))

        @patch.object(AsyncClass, 'decorated_cr_method')
        def test_async_decorator(mock_g):
            self.assertTrue(asyncio.iscoroutinefunction(mock_g))

        test_async()
        test_async_decorator()

    def test_is_coroutine_patch(self):
        @patch.object(AsyncClass, 'coroutine_method')
        def test_async(mock_method):
            self.assertTrue(asyncio.iscoroutine(mock_method()))

        @patch.object(AsyncClass, 'decorated_cr_method')
        def test_async_decorator(mock_method):
            self.assertTrue(asyncio.iscoroutine(mock_method()))

        @patch(f'{async_foo_name}.coroutine_method')
        def test_no_parent_attribute(mock_method):
            self.assertTrue(asyncio.iscoroutine(mock_method()))

        test_async()
        test_async_decorator()
        test_no_parent_attribute()

    def test_is_coroutinemock_patch(self):
        @patch.object(AsyncClass, 'coroutine_method')
        def test_async(mock_method):
            self.assertTrue(isinstance(mock_method, CoroutineMock))

        @patch.object(AsyncClass, 'decorated_cr_method')
        def test_async_decorator(mock_method):
            self.assertTrue(isinstance(mock_method, CoroutineMock))

        test_async()
        test_async_decorator()


class CoroutinePatchCMTest(unittest.TestCase):
    def test_is_coroutine_function_cm(self):
        def test_async():
            with patch.object(AsyncClass, 'coroutine_method') as mock_method:
                self.assertTrue(asyncio.iscoroutinefunction(mock_method))

        def test_async_decorator():
            with patch.object(AsyncClass, 'decorated_cr_method') as mock_method:
                self.assertTrue(asyncio.iscoroutinefunction(mock_method))

        test_async()
        test_async_decorator()

    def test_is_coroutine_cm(self):
        def test_async():
            with patch.object(AsyncClass, 'coroutine_method') as mock_method:
                self.assertTrue(asyncio.iscoroutine(mock_method()))

        def test_async_decorator():
            with patch.object(AsyncClass, 'decorated_cr_method') as mock_method:
                self.assertTrue(asyncio.iscoroutine(mock_method()))
        test_async()
        test_async_decorator()

    def test_is_coroutinemock_cm(self):
        def test_async():
            with patch.object(AsyncClass, 'coroutine_method') as mock_method:
                self.assertTrue(isinstance(mock_method, CoroutineMock))

        def test_async_decorator():
            with patch.object(AsyncClass, 'decorated_cr_method') as mock_method:
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

    def test_iscoroutinefunction_normal_function(self):
        def foo(): pass
        mock = CoroutineMock(foo)
        self.assertTrue(asyncio.iscoroutinefunction(mock))
        self.assertTrue(inspect.iscoroutinefunction(mock))


class CoroutineAutospecTest(unittest.TestCase):
    def test_is_coroutinemock_patch(self):
        @patch(async_foo_name, autospec=True)
        def test_async(mock_method):
            self.assertTrue(isinstance(
                             mock_method.coroutine_method,
                             CoroutineMock))
            self.assertTrue(isinstance(mock_method, MagicMock))

        @patch(async_foo_name, autospec=True)
        def test_async_decorator(mock_method):
            self.assertTrue(isinstance(
                                mock_method.decorated_cr_method,
                                CoroutineMock))
            self.assertTrue(isinstance(mock_method, MagicMock))

        @patch(async_foo_name, autospec=True)
        def test_normal_method(mock_method):
            self.assertTrue(isinstance(
                                mock_method.normal_method,
                                MagicMock))

        test_async()
        test_async_decorator()
        test_normal_method()


class CoroutineSpecTest(unittest.TestCase):
    def test_spec_as_coroutine_positional_magicmock(self):
        mock = MagicMock(coroutine_func)
        self.assertIsInstance(mock, MagicMock)
        self.assertTrue(asyncio.iscoroutine(mock()))

    def test_spec_as_coroutine_kw_magicmock(self):
        mock = MagicMock(spec=coroutine_func)
        self.assertIsInstance(mock, MagicMock)
        self.assertTrue(asyncio.iscoroutine(mock()))

    def test_spec_as_coroutine_kw_coroutinemock(self):
        mock = CoroutineMock(spec=coroutine_func)
        self.assertIsInstance(mock, CoroutineMock)
        self.assertTrue(asyncio.iscoroutine(mock()))

    def test_spec_as_coroutine_positional_coroutinemock(self):
        mock = CoroutineMock(coroutine_func)
        self.assertIsInstance(mock, CoroutineMock)
        self.assertTrue(asyncio.iscoroutine(mock()))

    def test_spec_as_normal_kw_coroutinemock(self):
        mock = CoroutineMock(spec=normal_func)
        self.assertIsInstance(mock, CoroutineMock)
        self.assertTrue(asyncio.iscoroutine(mock()))

    def test_spec_as_normal_positional_coroutinemock(self):
        mock = CoroutineMock(normal_func)
        self.assertIsInstance(mock, CoroutineMock)
        self.assertTrue(asyncio.iscoroutine(mock()))

    def test_spec_coroutine_mock(self):
        @patch.object(AsyncClass, 'coroutine_method', spec=True)
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
        @patch.object(AsyncClass, 'coroutine_method', spec=NormalClass.a)
        def test_async_attribute(mock_method):
            self.assertTrue(isinstance(mock_method, MagicMock))
            self.assertFalse(inspect.iscoroutine(mock_method))
            self.assertFalse(asyncio.iscoroutine(mock_method))

        test_async_attribute()

    def test_target_not_coroutine_spec_is(self):
        @patch.object(NormalClass, 'a', spec=coroutine_func)
        def test_attribute_not_coroutine_spec_is(mock_coroutine_func):
            self.assertTrue(isinstance(mock_coroutine_func, CoroutineMock))
        test_attribute_not_coroutine_spec_is()

    def test_spec_coroutine_attributes(self):
        @patch(normal_foo_name, spec=AsyncClass)
        def test_coroutine_attributes_coroutines(MockNormalClass):
            self.assertTrue(isinstance(MockNormalClass.coroutine_method,
                                       CoroutineMock))
            self.assertTrue(isinstance(MockNormalClass, MagicMock))

        test_coroutine_attributes_coroutines()


class CoroutineSpecSetTest(unittest.TestCase):
    def test_is_coroutinemock_patch(self):
        @patch.object(AsyncClass, 'coroutine_method', spec_set=True)
        def test_async(coroutine_method):
            self.assertTrue(isinstance(coroutine_method, CoroutineMock))

    def test_is_coroutine_coroutinemock(self):
        mock = CoroutineMock(spec_set=AsyncClass.coroutine_method)
        self.assertTrue(asyncio.iscoroutinefunction(mock))
        self.assertTrue(isinstance(mock, CoroutineMock))

    def test_is_child_coroutinemock(self):
        mock = MagicMock(spec_set=AsyncClass)
        self.assertTrue(asyncio.iscoroutinefunction(mock.coroutine_method))
        self.assertTrue(asyncio.iscoroutinefunction(mock.decorated_cr_method))
        self.assertFalse(asyncio.iscoroutinefunction(mock.normal_method))
        self.assertTrue(isinstance(mock.coroutine_method, CoroutineMock))
        self.assertTrue(isinstance(mock.decorated_cr_method, CoroutineMock))
        self.assertTrue(isinstance(mock.normal_method, MagicMock))
        self.assertTrue(isinstance(mock, MagicMock))


class CoroutineArguments(unittest.TestCase):
    # I want to add more tests here with more complicate use-cases.
    def setUp(self):
        self.old_policy = asyncio.events._event_loop_policy

    def tearDown(self):
        # Restore the original event loop policy.
        asyncio.events._event_loop_policy = self.old_policy

    def test_add_return_value(self):
        async def addition(self, var):
            return var + 1

        mock = CoroutineMock(addition, return_value=10)
        output = asyncio.run(mock(5))

        self.assertEqual(output, 10)

    def test_add_side_effect_exception(self):
        async def addition(self, var):
            return var + 1

        mock = CoroutineMock(addition, side_effect=Exception('err'))
        with self.assertRaises(Exception):
            asyncio.run(mock(5))


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

    # Before 3.7 __aiter__ was a coroutine
    class AsyncItertorDeprecated(AsyncIterator):
        async def __aiter__(self):
            return super().__aiter__()

    def test_mock_magic_methods_are_coroutine_mocks(self):
        mock_instance = CoroutineMock(spec=self.AsyncContextManager())
        self.assertIsInstance(mock_instance.__aenter__,
                              CoroutineMock)
        self.assertIsInstance(mock_instance.__aexit__,
                              CoroutineMock)

    def test_mock_aiter_and_anext(self):
        if sys.version_info < (3, 7):
            instance = self.AsyncItertorDeprecated()
        else:
            instance = self.AsyncIterator()
        mock_instance = CoroutineMock(instance)

        self.assertEqual(asyncio.iscoroutine(instance.__aiter__),
                         asyncio.iscoroutine(mock_instance.__aiter__))
        self.assertEqual(asyncio.iscoroutine(instance.__anext__),
                         asyncio.iscoroutine(mock_instance.__anext__))
        if sys.version_info < (3, 7):
            iterator = instance.__aiter__()
            iterator = run_coroutine(iterator)

            mock_iterator = mock_instance.__aiter__()
            mock_iterator = run_coroutine(mock_iterator)

            self.assertEqual(asyncio.iscoroutine(iterator.__aiter__),
                             asyncio.iscoroutine(mock_iterator.__aiter__))
            self.assertEqual(asyncio.iscoroutine(iterator.__anext__),
                             asyncio.iscoroutine(mock_iterator.__anext__))


class CoroutineMockAssert(unittest.TestCase):

    def setUp(self):
        self.mock = CoroutineMock()
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
