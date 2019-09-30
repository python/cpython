import asyncio
import inspect
import re
import unittest

from unittest.mock import (ANY, call, AsyncMock, patch, MagicMock, Mock,
                           create_autospec, sentinel, _CallList)


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class AsyncClass:
    def __init__(self):
        pass
    async def async_method(self):
        pass
    def normal_method(self):
        pass

class AwaitableClass:
    def __await__(self):
        yield

async def async_func():
    pass

async def async_func_args(a, b, *, c):
    pass

def normal_func():
    pass

class NormalClass(object):
    def a(self):
        pass


async_foo_name = f'{__name__}.AsyncClass'
normal_foo_name = f'{__name__}.NormalClass'


class AsyncPatchDecoratorTest(unittest.TestCase):
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
            self.assertIsInstance(mock_method, AsyncMock)

        test_async()

    def test_async_def_patch(self):
        @patch(f"{__name__}.async_func", AsyncMock())
        async def test_async():
            self.assertIsInstance(async_func, AsyncMock)

        asyncio.run(test_async())
        self.assertTrue(inspect.iscoroutinefunction(async_func))


class AsyncPatchCMTest(unittest.TestCase):
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
                self.assertIsInstance(mock_method, AsyncMock)

        test_async()

    def test_async_def_cm(self):
        async def test_async():
            with patch(f"{__name__}.async_func", AsyncMock()):
                self.assertIsInstance(async_func, AsyncMock)
            self.assertTrue(inspect.iscoroutinefunction(async_func))

        asyncio.run(test_async())


class AsyncMockTest(unittest.TestCase):
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

    def test_future_isfuture(self):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        fut = asyncio.Future()
        loop.stop()
        loop.close()
        mock = AsyncMock(fut)
        self.assertIsInstance(mock, asyncio.Future)


class AsyncAutospecTest(unittest.TestCase):
    def test_is_AsyncMock_patch(self):
        @patch(async_foo_name, autospec=True)
        def test_async(mock_method):
            self.assertIsInstance(mock_method.async_method, AsyncMock)
            self.assertIsInstance(mock_method, MagicMock)

        @patch(async_foo_name, autospec=True)
        def test_normal_method(mock_method):
            self.assertIsInstance(mock_method.normal_method, MagicMock)

        test_async()
        test_normal_method()

    def test_create_autospec_instance(self):
        with self.assertRaises(RuntimeError):
            create_autospec(async_func, instance=True)

    def test_create_autospec_awaitable_class(self):
        awaitable_mock = create_autospec(spec=AwaitableClass())
        self.assertIsInstance(create_autospec(awaitable_mock), AsyncMock)

    def test_create_autospec(self):
        spec = create_autospec(async_func_args)
        awaitable = spec(1, 2, c=3)
        async def main():
            await awaitable

        self.assertEqual(spec.await_count, 0)
        self.assertIsNone(spec.await_args)
        self.assertEqual(spec.await_args_list, [])
        spec.assert_not_awaited()

        asyncio.run(main())

        self.assertTrue(asyncio.iscoroutinefunction(spec))
        self.assertTrue(asyncio.iscoroutine(awaitable))
        self.assertEqual(spec.await_count, 1)
        self.assertEqual(spec.await_args, call(1, 2, c=3))
        self.assertEqual(spec.await_args_list, [call(1, 2, c=3)])
        spec.assert_awaited_once()
        spec.assert_awaited_once_with(1, 2, c=3)
        spec.assert_awaited_with(1, 2, c=3)
        spec.assert_awaited()

    def test_patch_with_autospec(self):

        async def test_async():
            with patch(f"{__name__}.async_func_args", autospec=True) as mock_method:
                awaitable = mock_method(1, 2, c=3)
                self.assertIsInstance(mock_method.mock, AsyncMock)

                self.assertTrue(asyncio.iscoroutinefunction(mock_method))
                self.assertTrue(asyncio.iscoroutine(awaitable))
                self.assertTrue(inspect.isawaitable(awaitable))

                # Verify the default values during mock setup
                self.assertEqual(mock_method.await_count, 0)
                self.assertEqual(mock_method.await_args_list, [])
                self.assertIsNone(mock_method.await_args)
                mock_method.assert_not_awaited()

                await awaitable

            self.assertEqual(mock_method.await_count, 1)
            self.assertEqual(mock_method.await_args, call(1, 2, c=3))
            self.assertEqual(mock_method.await_args_list, [call(1, 2, c=3)])
            mock_method.assert_awaited_once()
            mock_method.assert_awaited_once_with(1, 2, c=3)
            mock_method.assert_awaited_with(1, 2, c=3)
            mock_method.assert_awaited()

            mock_method.reset_mock()
            self.assertEqual(mock_method.await_count, 0)
            self.assertIsNone(mock_method.await_args)
            self.assertEqual(mock_method.await_args_list, [])

        asyncio.run(test_async())


class AsyncSpecTest(unittest.TestCase):
    def test_spec_normal_methods_on_class(self):
        def inner_test(mock_type):
            mock = mock_type(AsyncClass)
            self.assertIsInstance(mock.async_method, AsyncMock)
            self.assertIsInstance(mock.normal_method, MagicMock)

        for mock_type in [AsyncMock, MagicMock]:
            with self.subTest(f"test method types with {mock_type}"):
                inner_test(mock_type)

    def test_spec_normal_methods_on_class_with_mock(self):
        mock = Mock(AsyncClass)
        self.assertIsInstance(mock.async_method, AsyncMock)
        self.assertIsInstance(mock.normal_method, Mock)

    def test_spec_mock_type_kw(self):
        def inner_test(mock_type):
            async_mock = mock_type(spec=async_func)
            self.assertIsInstance(async_mock, mock_type)
            with self.assertWarns(RuntimeWarning):
                # Will raise a warning because never awaited
                self.assertTrue(inspect.isawaitable(async_mock()))

            sync_mock = mock_type(spec=normal_func)
            self.assertIsInstance(sync_mock, mock_type)

        for mock_type in [AsyncMock, MagicMock, Mock]:
            with self.subTest(f"test spec kwarg with {mock_type}"):
                inner_test(mock_type)

    def test_spec_mock_type_positional(self):
        def inner_test(mock_type):
            async_mock = mock_type(async_func)
            self.assertIsInstance(async_mock, mock_type)
            with self.assertWarns(RuntimeWarning):
                # Will raise a warning because never awaited
                self.assertTrue(inspect.isawaitable(async_mock()))

            sync_mock = mock_type(normal_func)
            self.assertIsInstance(sync_mock, mock_type)

        for mock_type in [AsyncMock, MagicMock, Mock]:
            with self.subTest(f"test spec positional with {mock_type}"):
                inner_test(mock_type)

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
            self.assertIsInstance(mock_method, AsyncMock)

        test_async()

    def test_spec_parent_not_async_attribute_is(self):
        @patch(async_foo_name, spec=True)
        def test_async(mock_method):
            self.assertIsInstance(mock_method, MagicMock)
            self.assertIsInstance(mock_method.async_method, AsyncMock)

        test_async()

    def test_target_async_spec_not(self):
        @patch.object(AsyncClass, 'async_method', spec=NormalClass.a)
        def test_async_attribute(mock_method):
            self.assertIsInstance(mock_method, MagicMock)
            self.assertFalse(inspect.iscoroutine(mock_method))
            self.assertFalse(inspect.isawaitable(mock_method))

        test_async_attribute()

    def test_target_not_async_spec_is(self):
        @patch.object(NormalClass, 'a', spec=async_func)
        def test_attribute_not_async_spec_is(mock_async_func):
            self.assertIsInstance(mock_async_func, AsyncMock)
        test_attribute_not_async_spec_is()

    def test_spec_async_attributes(self):
        @patch(normal_foo_name, spec=AsyncClass)
        def test_async_attributes_coroutines(MockNormalClass):
            self.assertIsInstance(MockNormalClass.async_method, AsyncMock)
            self.assertIsInstance(MockNormalClass, MagicMock)

        test_async_attributes_coroutines()


class AsyncSpecSetTest(unittest.TestCase):
    def test_is_AsyncMock_patch(self):
        @patch.object(AsyncClass, 'async_method', spec_set=True)
        def test_async(async_method):
            self.assertIsInstance(async_method, AsyncMock)

    def test_is_async_AsyncMock(self):
        mock = AsyncMock(spec_set=AsyncClass.async_method)
        self.assertTrue(asyncio.iscoroutinefunction(mock))
        self.assertIsInstance(mock, AsyncMock)

    def test_is_child_AsyncMock(self):
        mock = MagicMock(spec_set=AsyncClass)
        self.assertTrue(asyncio.iscoroutinefunction(mock.async_method))
        self.assertFalse(asyncio.iscoroutinefunction(mock.normal_method))
        self.assertIsInstance(mock.async_method, AsyncMock)
        self.assertIsInstance(mock.normal_method, MagicMock)
        self.assertIsInstance(mock, MagicMock)

    def test_magicmock_lambda_spec(self):
        mock_obj = MagicMock()
        mock_obj.mock_func = MagicMock(spec=lambda x: x)

        with patch.object(mock_obj, "mock_func") as cm:
            self.assertIsInstance(cm, MagicMock)


class AsyncArguments(unittest.TestCase):
    def test_add_return_value(self):
        async def addition(self, var):
            return var + 1

        mock = AsyncMock(addition, return_value=10)
        output = asyncio.run(mock(5))

        self.assertEqual(output, 10)

    def test_add_side_effect_exception(self):
        async def addition(var):
            return var + 1
        mock = AsyncMock(addition, side_effect=Exception('err'))
        with self.assertRaises(Exception):
            asyncio.run(mock(5))

    def test_add_side_effect_function(self):
        async def addition(var):
            return var + 1
        mock = AsyncMock(side_effect=addition)
        result = asyncio.run(mock(5))
        self.assertEqual(result, 6)

    def test_add_side_effect_iterable(self):
        vals = [1, 2, 3]
        mock = AsyncMock(side_effect=vals)
        for item in vals:
            self.assertEqual(item, asyncio.run(mock()))

        with self.assertRaises(RuntimeError) as e:
            asyncio.run(mock())
            self.assertEqual(
                e.exception,
                RuntimeError('coroutine raised StopIteration')
            )

class AsyncMagicMethods(unittest.TestCase):
    def test_async_magic_methods_return_async_mocks(self):
        m_mock = MagicMock()
        self.assertIsInstance(m_mock.__aenter__, AsyncMock)
        self.assertIsInstance(m_mock.__aexit__, AsyncMock)
        self.assertIsInstance(m_mock.__anext__, AsyncMock)
        # __aiter__ is actually a synchronous object
        # so should return a MagicMock
        self.assertIsInstance(m_mock.__aiter__, MagicMock)

    def test_sync_magic_methods_return_magic_mocks(self):
        a_mock = AsyncMock()
        self.assertIsInstance(a_mock.__enter__, MagicMock)
        self.assertIsInstance(a_mock.__exit__, MagicMock)
        self.assertIsInstance(a_mock.__next__, MagicMock)
        self.assertIsInstance(a_mock.__len__, MagicMock)

    def test_magicmock_has_async_magic_methods(self):
        m_mock = MagicMock()
        self.assertTrue(hasattr(m_mock, "__aenter__"))
        self.assertTrue(hasattr(m_mock, "__aexit__"))
        self.assertTrue(hasattr(m_mock, "__anext__"))

    def test_asyncmock_has_sync_magic_methods(self):
        a_mock = AsyncMock()
        self.assertTrue(hasattr(a_mock, "__enter__"))
        self.assertTrue(hasattr(a_mock, "__exit__"))
        self.assertTrue(hasattr(a_mock, "__next__"))
        self.assertTrue(hasattr(a_mock, "__len__"))

    def test_magic_methods_are_async_functions(self):
        m_mock = MagicMock()
        self.assertIsInstance(m_mock.__aenter__, AsyncMock)
        self.assertIsInstance(m_mock.__aexit__, AsyncMock)
        # AsyncMocks are also coroutine functions
        self.assertTrue(asyncio.iscoroutinefunction(m_mock.__aenter__))
        self.assertTrue(asyncio.iscoroutinefunction(m_mock.__aexit__))

class AsyncContextManagerTest(unittest.TestCase):
    class WithAsyncContextManager:
        async def __aenter__(self, *args, **kwargs):
            self.entered = True
            return self

        async def __aexit__(self, *args, **kwargs):
            self.exited = True

    class WithSyncContextManager:
        def __enter__(self, *args, **kwargs):
            return self

        def __exit__(self, *args, **kwargs):
            pass

    class ProductionCode:
        # Example real-world(ish) code
        def __init__(self):
            self.session = None

        async def main(self):
            async with self.session.post('https://python.org') as response:
                val = await response.json()
                return val

    def test_set_return_value_of_aenter(self):
        def inner_test(mock_type):
            pc = self.ProductionCode()
            pc.session = MagicMock(name='sessionmock')
            cm = mock_type(name='magic_cm')
            response = AsyncMock(name='response')
            response.json = AsyncMock(return_value={'json': 123})
            cm.__aenter__.return_value = response
            pc.session.post.return_value = cm
            result = asyncio.run(pc.main())
            self.assertEqual(result, {'json': 123})

        for mock_type in [AsyncMock, MagicMock]:
            with self.subTest(f"test set return value of aenter with {mock_type}"):
                inner_test(mock_type)

    def test_mock_supports_async_context_manager(self):
        def inner_test(mock_type):
            called = False
            cm = self.WithAsyncContextManager()
            cm_mock = mock_type(cm)

            async def use_context_manager():
                nonlocal called
                async with cm_mock as result:
                    called = True
                return result

            cm_result = asyncio.run(use_context_manager())
            self.assertTrue(called)
            self.assertTrue(cm_mock.__aenter__.called)
            self.assertTrue(cm_mock.__aexit__.called)
            cm_mock.__aenter__.assert_awaited()
            cm_mock.__aexit__.assert_awaited()
            # We mock __aenter__ so it does not return self
            self.assertIsNot(cm_mock, cm_result)

        for mock_type in [AsyncMock, MagicMock]:
            with self.subTest(f"test context manager magics with {mock_type}"):
                inner_test(mock_type)

    def test_mock_customize_async_context_manager(self):
        instance = self.WithAsyncContextManager()
        mock_instance = MagicMock(instance)

        expected_result = object()
        mock_instance.__aenter__.return_value = expected_result

        async def use_context_manager():
            async with mock_instance as result:
                return result

        self.assertIs(asyncio.run(use_context_manager()), expected_result)

    def test_mock_customize_async_context_manager_with_coroutine(self):
        enter_called = False
        exit_called = False

        async def enter_coroutine(*args):
            nonlocal enter_called
            enter_called = True

        async def exit_coroutine(*args):
            nonlocal exit_called
            exit_called = True

        instance = self.WithAsyncContextManager()
        mock_instance = MagicMock(instance)

        mock_instance.__aenter__ = enter_coroutine
        mock_instance.__aexit__ = exit_coroutine

        async def use_context_manager():
            async with mock_instance:
                pass

        asyncio.run(use_context_manager())
        self.assertTrue(enter_called)
        self.assertTrue(exit_called)

    def test_context_manager_raise_exception_by_default(self):
        async def raise_in(context_manager):
            async with context_manager:
                raise TypeError()

        instance = self.WithAsyncContextManager()
        mock_instance = MagicMock(instance)
        with self.assertRaises(TypeError):
            asyncio.run(raise_in(mock_instance))


class AsyncIteratorTest(unittest.TestCase):
    class WithAsyncIterator(object):
        def __init__(self):
            self.items = ["foo", "NormalFoo", "baz"]

        def __aiter__(self):
            return self

        async def __anext__(self):
            try:
                return self.items.pop()
            except IndexError:
                pass

            raise StopAsyncIteration

    def test_aiter_set_return_value(self):
        mock_iter = AsyncMock(name="tester")
        mock_iter.__aiter__.return_value = [1, 2, 3]
        async def main():
            return [i async for i in mock_iter]
        result = asyncio.run(main())
        self.assertEqual(result, [1, 2, 3])

    def test_mock_aiter_and_anext_asyncmock(self):
        def inner_test(mock_type):
            instance = self.WithAsyncIterator()
            mock_instance = mock_type(instance)
            # Check that the mock and the real thing bahave the same
            # __aiter__ is not actually async, so not a coroutinefunction
            self.assertFalse(asyncio.iscoroutinefunction(instance.__aiter__))
            self.assertFalse(asyncio.iscoroutinefunction(mock_instance.__aiter__))
            # __anext__ is async
            self.assertTrue(asyncio.iscoroutinefunction(instance.__anext__))
            self.assertTrue(asyncio.iscoroutinefunction(mock_instance.__anext__))

        for mock_type in [AsyncMock, MagicMock]:
            with self.subTest(f"test aiter and anext corourtine with {mock_type}"):
                inner_test(mock_type)


    def test_mock_async_for(self):
        async def iterate(iterator):
            accumulator = []
            async for item in iterator:
                accumulator.append(item)

            return accumulator

        expected = ["FOO", "BAR", "BAZ"]
        def test_default(mock_type):
            mock_instance = mock_type(self.WithAsyncIterator())
            self.assertEqual(asyncio.run(iterate(mock_instance)), [])


        def test_set_return_value(mock_type):
            mock_instance = mock_type(self.WithAsyncIterator())
            mock_instance.__aiter__.return_value = expected[:]
            self.assertEqual(asyncio.run(iterate(mock_instance)), expected)

        def test_set_return_value_iter(mock_type):
            mock_instance = mock_type(self.WithAsyncIterator())
            mock_instance.__aiter__.return_value = iter(expected[:])
            self.assertEqual(asyncio.run(iterate(mock_instance)), expected)

        for mock_type in [AsyncMock, MagicMock]:
            with self.subTest(f"default value with {mock_type}"):
                test_default(mock_type)

            with self.subTest(f"set return_value with {mock_type}"):
                test_set_return_value(mock_type)

            with self.subTest(f"set return_value iterator with {mock_type}"):
                test_set_return_value_iter(mock_type)


class AsyncMockAssert(unittest.TestCase):
    def setUp(self):
        self.mock = AsyncMock()

    async def _runnable_test(self, *args, **kwargs):
        await self.mock(*args, **kwargs)

    async def _await_coroutine(self, coroutine):
        return await coroutine

    def test_assert_called_but_not_awaited(self):
        mock = AsyncMock(AsyncClass)
        with self.assertWarns(RuntimeWarning):
            # Will raise a warning because never awaited
            mock.async_method()
        self.assertTrue(asyncio.iscoroutinefunction(mock.async_method))
        mock.async_method.assert_called()
        mock.async_method.assert_called_once()
        mock.async_method.assert_called_once_with()
        with self.assertRaises(AssertionError):
            mock.assert_awaited()
        with self.assertRaises(AssertionError):
            mock.async_method.assert_awaited()

    def test_assert_called_then_awaited(self):
        mock = AsyncMock(AsyncClass)
        mock_coroutine = mock.async_method()
        mock.async_method.assert_called()
        mock.async_method.assert_called_once()
        mock.async_method.assert_called_once_with()
        with self.assertRaises(AssertionError):
            mock.async_method.assert_awaited()

        asyncio.run(self._await_coroutine(mock_coroutine))
        # Assert we haven't re-called the function
        mock.async_method.assert_called_once()
        mock.async_method.assert_awaited()
        mock.async_method.assert_awaited_once()
        mock.async_method.assert_awaited_once_with()

    def test_assert_called_and_awaited_at_same_time(self):
        with self.assertRaises(AssertionError):
            self.mock.assert_awaited()

        with self.assertRaises(AssertionError):
            self.mock.assert_called()

        asyncio.run(self._runnable_test())
        self.mock.assert_called_once()
        self.mock.assert_awaited_once()

    def test_assert_called_twice_and_awaited_once(self):
        mock = AsyncMock(AsyncClass)
        coroutine = mock.async_method()
        with self.assertWarns(RuntimeWarning):
            # The first call will be awaited so no warning there
            # But this call will never get awaited, so it will warn here
            mock.async_method()
        with self.assertRaises(AssertionError):
            mock.async_method.assert_awaited()
        mock.async_method.assert_called()
        asyncio.run(self._await_coroutine(coroutine))
        mock.async_method.assert_awaited()
        mock.async_method.assert_awaited_once()

    def test_assert_called_once_and_awaited_twice(self):
        mock = AsyncMock(AsyncClass)
        coroutine = mock.async_method()
        mock.async_method.assert_called_once()
        asyncio.run(self._await_coroutine(coroutine))
        with self.assertRaises(RuntimeError):
            # Cannot reuse already awaited coroutine
            asyncio.run(self._await_coroutine(coroutine))
        mock.async_method.assert_awaited()

    def test_assert_awaited_but_not_called(self):
        with self.assertRaises(AssertionError):
            self.mock.assert_awaited()
        with self.assertRaises(AssertionError):
            self.mock.assert_called()
        with self.assertRaises(TypeError):
            # You cannot await an AsyncMock, it must be a coroutine
            asyncio.run(self._await_coroutine(self.mock))

        with self.assertRaises(AssertionError):
            self.mock.assert_awaited()
        with self.assertRaises(AssertionError):
            self.mock.assert_called()

    def test_assert_has_calls_not_awaits(self):
        kalls = [call('foo')]
        with self.assertWarns(RuntimeWarning):
            # Will raise a warning because never awaited
            self.mock('foo')
        self.mock.assert_has_calls(kalls)
        with self.assertRaises(AssertionError):
            self.mock.assert_has_awaits(kalls)

    def test_assert_has_mock_calls_on_async_mock_no_spec(self):
        with self.assertWarns(RuntimeWarning):
            # Will raise a warning because never awaited
            self.mock()
        kalls_empty = [('', (), {})]
        self.assertEqual(self.mock.mock_calls, kalls_empty)

        with self.assertWarns(RuntimeWarning):
            # Will raise a warning because never awaited
            self.mock('foo')
            self.mock('baz')
        mock_kalls = ([call(), call('foo'), call('baz')])
        self.assertEqual(self.mock.mock_calls, mock_kalls)

    def test_assert_has_mock_calls_on_async_mock_with_spec(self):
        a_class_mock = AsyncMock(AsyncClass)
        with self.assertWarns(RuntimeWarning):
            # Will raise a warning because never awaited
            a_class_mock.async_method()
        kalls_empty = [('', (), {})]
        self.assertEqual(a_class_mock.async_method.mock_calls, kalls_empty)
        self.assertEqual(a_class_mock.mock_calls, [call.async_method()])

        with self.assertWarns(RuntimeWarning):
            # Will raise a warning because never awaited
            a_class_mock.async_method(1, 2, 3, a=4, b=5)
        method_kalls = [call(), call(1, 2, 3, a=4, b=5)]
        mock_kalls = [call.async_method(), call.async_method(1, 2, 3, a=4, b=5)]
        self.assertEqual(a_class_mock.async_method.mock_calls, method_kalls)
        self.assertEqual(a_class_mock.mock_calls, mock_kalls)

    def test_async_method_calls_recorded(self):
        with self.assertWarns(RuntimeWarning):
            # Will raise warnings because never awaited
            self.mock.something(3, fish=None)
            self.mock.something_else.something(6, cake=sentinel.Cake)

        self.assertEqual(self.mock.method_calls, [
            ("something", (3,), {'fish': None}),
            ("something_else.something", (6,), {'cake': sentinel.Cake})
        ],
            "method calls not recorded correctly")
        self.assertEqual(self.mock.something_else.method_calls,
                         [("something", (6,), {'cake': sentinel.Cake})],
                         "method calls not recorded correctly")

    def test_async_arg_lists(self):
        def assert_attrs(mock):
            names = ('call_args_list', 'method_calls', 'mock_calls')
            for name in names:
                attr = getattr(mock, name)
                self.assertIsInstance(attr, _CallList)
                self.assertIsInstance(attr, list)
                self.assertEqual(attr, [])

        assert_attrs(self.mock)
        with self.assertWarns(RuntimeWarning):
            # Will raise warnings because never awaited
            self.mock()
            self.mock(1, 2)
            self.mock(a=3)

        self.mock.reset_mock()
        assert_attrs(self.mock)

        a_mock = AsyncMock(AsyncClass)
        with self.assertWarns(RuntimeWarning):
            # Will raise warnings because never awaited
            a_mock.async_method()
            a_mock.async_method(1, a=3)

        a_mock.reset_mock()
        assert_attrs(a_mock)

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
        msg = 'expected await not found'
        with self.assertRaisesRegex(AssertionError, msg):
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
            self.mock.assert_any_await('foo')

        asyncio.run(self._runnable_test('baz'))
        with self.assertRaises(AssertionError):
            self.mock.assert_any_await('foo')

        asyncio.run(self._runnable_test('foo'))
        self.mock.assert_any_await('foo')

        asyncio.run(self._runnable_test('SomethingElse'))
        self.mock.assert_any_await('foo')

    def test_assert_has_awaits_no_order(self):
        calls = [call('foo'), call('baz')]

        with self.assertRaises(AssertionError) as cm:
            self.mock.assert_has_awaits(calls)
        self.assertEqual(len(cm.exception.args), 1)

        asyncio.run(self._runnable_test('foo'))
        with self.assertRaises(AssertionError):
            self.mock.assert_has_awaits(calls)

        asyncio.run(self._runnable_test('foo'))
        with self.assertRaises(AssertionError):
            self.mock.assert_has_awaits(calls)

        asyncio.run(self._runnable_test('baz'))
        self.mock.assert_has_awaits(calls)

        asyncio.run(self._runnable_test('SomethingElse'))
        self.mock.assert_has_awaits(calls)

    def test_assert_has_awaits_ordered(self):
        calls = [call('foo'), call('baz')]
        with self.assertRaises(AssertionError):
            self.mock.assert_has_awaits(calls, any_order=True)

        asyncio.run(self._runnable_test('baz'))
        with self.assertRaises(AssertionError):
            self.mock.assert_has_awaits(calls, any_order=True)

        asyncio.run(self._runnable_test('bamf'))
        with self.assertRaises(AssertionError):
            self.mock.assert_has_awaits(calls, any_order=True)

        asyncio.run(self._runnable_test('foo'))
        self.mock.assert_has_awaits(calls, any_order=True)

        asyncio.run(self._runnable_test('qux'))
        self.mock.assert_has_awaits(calls, any_order=True)

    def test_assert_not_awaited(self):
        self.mock.assert_not_awaited()

        asyncio.run(self._runnable_test())
        with self.assertRaises(AssertionError):
            self.mock.assert_not_awaited()

    def test_assert_has_awaits_not_matching_spec_error(self):
        async def f(x=None): pass

        self.mock = AsyncMock(spec=f)
        asyncio.run(self._runnable_test(1))

        with self.assertRaisesRegex(
                AssertionError,
                '^{}$'.format(
                    re.escape('Awaits not found.\n'
                              'Expected: [call()]\n'
                              'Actual: [call(1)]'))) as cm:
            self.mock.assert_has_awaits([call()])
        self.assertIsNone(cm.exception.__cause__)

        with self.assertRaisesRegex(
                AssertionError,
                '^{}$'.format(
                    re.escape(
                        'Error processing expected awaits.\n'
                        "Errors: [None, TypeError('too many positional "
                        "arguments')]\n"
                        'Expected: [call(), call(1, 2)]\n'
                        'Actual: [call(1)]'))) as cm:
            self.mock.assert_has_awaits([call(), call(1, 2)])
        self.assertIsInstance(cm.exception.__cause__, TypeError)
