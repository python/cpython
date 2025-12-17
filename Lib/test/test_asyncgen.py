import inspect
import types
import unittest
import contextlib

from test.support.import_helper import import_module
from test.support import gc_collect, requires_working_socket
asyncio = import_module("asyncio")


requires_working_socket(module=True)

_no_default = object()


class AwaitException(Exception):
    pass


@types.coroutine
def awaitable(*, throw=False):
    if throw:
        yield ('throw',)
    else:
        yield ('result',)


def run_until_complete(coro):
    exc = False
    while True:
        try:
            if exc:
                exc = False
                fut = coro.throw(AwaitException)
            else:
                fut = coro.send(None)
        except StopIteration as ex:
            return ex.args[0]

        if fut == ('throw',):
            exc = True


def to_list(gen):
    async def iterate():
        res = []
        async for i in gen:
            res.append(i)
        return res

    return run_until_complete(iterate())


def py_anext(iterator, default=_no_default):
    """Pure-Python implementation of anext() for testing purposes.

    Closely matches the builtin anext() C implementation.
    Can be used to compare the built-in implementation of the inner
    coroutines machinery to C-implementation of __anext__() and send()
    or throw() on the returned generator.
    """

    try:
        __anext__ = type(iterator).__anext__
    except AttributeError:
        raise TypeError(f'{iterator!r} is not an async iterator')

    if default is _no_default:
        return __anext__(iterator)

    async def anext_impl():
        try:
            # The C code is way more low-level than this, as it implements
            # all methods of the iterator protocol. In this implementation
            # we're relying on higher-level coroutine concepts, but that's
            # exactly what we want -- crosstest pure-Python high-level
            # implementation and low-level C anext() iterators.
            return await __anext__(iterator)
        except StopAsyncIteration:
            return default

    return anext_impl()


class AsyncGenSyntaxTest(unittest.TestCase):

    def test_async_gen_syntax_01(self):
        code = '''async def foo():
            await abc
            yield from 123
        '''

        with self.assertRaisesRegex(SyntaxError, 'yield from.*inside async'):
            exec(code, {}, {})

    def test_async_gen_syntax_02(self):
        code = '''async def foo():
            yield from 123
        '''

        with self.assertRaisesRegex(SyntaxError, 'yield from.*inside async'):
            exec(code, {}, {})

    def test_async_gen_syntax_03(self):
        code = '''async def foo():
            await abc
            yield
            return 123
        '''

        with self.assertRaisesRegex(SyntaxError, 'return.*value.*async gen'):
            exec(code, {}, {})

    def test_async_gen_syntax_04(self):
        code = '''async def foo():
            yield
            return 123
        '''

        with self.assertRaisesRegex(SyntaxError, 'return.*value.*async gen'):
            exec(code, {}, {})

    def test_async_gen_syntax_05(self):
        code = '''async def foo():
            if 0:
                yield
            return 12
        '''

        with self.assertRaisesRegex(SyntaxError, 'return.*value.*async gen'):
            exec(code, {}, {})


class AsyncGenTest(unittest.TestCase):

    def compare_generators(self, sync_gen, async_gen):
        def sync_iterate(g):
            res = []
            while True:
                try:
                    res.append(g.__next__())
                except StopIteration:
                    res.append('STOP')
                    break
                except Exception as ex:
                    res.append(str(type(ex)))
            return res

        def async_iterate(g):
            res = []
            while True:
                an = g.__anext__()
                try:
                    while True:
                        try:
                            an.__next__()
                        except StopIteration as ex:
                            if ex.args:
                                res.append(ex.args[0])
                                break
                            else:
                                res.append('EMPTY StopIteration')
                                break
                        except StopAsyncIteration:
                            raise
                        except Exception as ex:
                            res.append(str(type(ex)))
                            break
                except StopAsyncIteration:
                    res.append('STOP')
                    break
            return res

        sync_gen_result = sync_iterate(sync_gen)
        async_gen_result = async_iterate(async_gen)
        self.assertEqual(sync_gen_result, async_gen_result)
        return async_gen_result

    def test_async_gen_iteration_01(self):
        async def gen():
            await awaitable()
            a = yield 123
            self.assertIs(a, None)
            await awaitable()
            yield 456
            await awaitable()
            yield 789

        self.assertEqual(to_list(gen()), [123, 456, 789])

    def test_async_gen_iteration_02(self):
        async def gen():
            await awaitable()
            yield 123
            await awaitable()

        g = gen()
        ai = g.__aiter__()

        an = ai.__anext__()
        self.assertEqual(an.__next__(), ('result',))

        try:
            an.__next__()
        except StopIteration as ex:
            self.assertEqual(ex.args[0], 123)
        else:
            self.fail('StopIteration was not raised')

        an = ai.__anext__()
        self.assertEqual(an.__next__(), ('result',))

        try:
            an.__next__()
        except StopAsyncIteration as ex:
            self.assertFalse(ex.args)
        else:
            self.fail('StopAsyncIteration was not raised')

    def test_async_gen_exception_03(self):
        async def gen():
            await awaitable()
            yield 123
            await awaitable(throw=True)
            yield 456

        with self.assertRaises(AwaitException):
            to_list(gen())

    def test_async_gen_exception_04(self):
        async def gen():
            await awaitable()
            yield 123
            1 / 0

        g = gen()
        ai = g.__aiter__()
        an = ai.__anext__()
        self.assertEqual(an.__next__(), ('result',))

        try:
            an.__next__()
        except StopIteration as ex:
            self.assertEqual(ex.args[0], 123)
        else:
            self.fail('StopIteration was not raised')

        with self.assertRaises(ZeroDivisionError):
            ai.__anext__().__next__()

    def test_async_gen_exception_05(self):
        async def gen():
            yield 123
            raise StopAsyncIteration

        with self.assertRaisesRegex(RuntimeError,
                                    'async generator.*StopAsyncIteration'):
            to_list(gen())

    def test_async_gen_exception_06(self):
        async def gen():
            yield 123
            raise StopIteration

        with self.assertRaisesRegex(RuntimeError,
                                    'async generator.*StopIteration'):
            to_list(gen())

    def test_async_gen_exception_07(self):
        def sync_gen():
            try:
                yield 1
                1 / 0
            finally:
                yield 2
                yield 3

            yield 100

        async def async_gen():
            try:
                yield 1
                1 / 0
            finally:
                yield 2
                yield 3

            yield 100

        self.compare_generators(sync_gen(), async_gen())

    def test_async_gen_exception_08(self):
        def sync_gen():
            try:
                yield 1
            finally:
                yield 2
                1 / 0
                yield 3

            yield 100

        async def async_gen():
            try:
                yield 1
                await awaitable()
            finally:
                await awaitable()
                yield 2
                1 / 0
                yield 3

            yield 100

        self.compare_generators(sync_gen(), async_gen())

    def test_async_gen_exception_09(self):
        def sync_gen():
            try:
                yield 1
                1 / 0
            finally:
                yield 2
                yield 3

            yield 100

        async def async_gen():
            try:
                await awaitable()
                yield 1
                1 / 0
            finally:
                yield 2
                await awaitable()
                yield 3

            yield 100

        self.compare_generators(sync_gen(), async_gen())

    def test_async_gen_exception_10(self):
        async def gen():
            yield 123
        with self.assertRaisesRegex(TypeError,
                                    "non-None value .* async generator"):
            gen().__anext__().send(100)

    def test_async_gen_exception_11(self):
        def sync_gen():
            yield 10
            yield 20

        def sync_gen_wrapper():
            yield 1
            sg = sync_gen()
            sg.send(None)
            try:
                sg.throw(GeneratorExit())
            except GeneratorExit:
                yield 2
            yield 3

        async def async_gen():
            yield 10
            yield 20

        async def async_gen_wrapper():
            yield 1
            asg = async_gen()
            await asg.asend(None)
            try:
                await asg.athrow(GeneratorExit())
            except GeneratorExit:
                yield 2
            yield 3

        self.compare_generators(sync_gen_wrapper(), async_gen_wrapper())

    def test_async_gen_exception_12(self):
        async def gen():
            with self.assertWarnsRegex(RuntimeWarning,
                    f"coroutine method 'asend' of '{gen.__qualname__}' "
                    f"was never awaited"):
                await anext(me)
            yield 123

        me = gen()
        ai = me.__aiter__()
        an = ai.__anext__()

        with self.assertRaisesRegex(RuntimeError,
                r'anext\(\): asynchronous generator is already running'):
            an.__next__()

        with self.assertRaisesRegex(RuntimeError,
                r"cannot reuse already awaited __anext__\(\)/asend\(\)"):
            an.send(None)

    def test_async_gen_asend_throw_concurrent_with_send(self):
        import types

        @types.coroutine
        def _async_yield(v):
            return (yield v)

        class MyExc(Exception):
            pass

        async def agenfn():
            while True:
                try:
                    await _async_yield(None)
                except MyExc:
                    pass
            return
            yield


        agen = agenfn()
        gen = agen.asend(None)
        gen.send(None)
        gen2 = agen.asend(None)

        with self.assertRaisesRegex(RuntimeError,
                r'anext\(\): asynchronous generator is already running'):
            gen2.throw(MyExc)

        with self.assertRaisesRegex(RuntimeError,
                r"cannot reuse already awaited __anext__\(\)/asend\(\)"):
            gen2.send(None)

    def test_async_gen_athrow_throw_concurrent_with_send(self):
        import types

        @types.coroutine
        def _async_yield(v):
            return (yield v)

        class MyExc(Exception):
            pass

        async def agenfn():
            while True:
                try:
                    await _async_yield(None)
                except MyExc:
                    pass
            return
            yield


        agen = agenfn()
        gen = agen.asend(None)
        gen.send(None)
        gen2 = agen.athrow(MyExc)

        with self.assertRaisesRegex(RuntimeError,
                r'athrow\(\): asynchronous generator is already running'):
            gen2.throw(MyExc)

        with self.assertRaisesRegex(RuntimeError,
                r"cannot reuse already awaited aclose\(\)/athrow\(\)"):
            gen2.send(None)

    def test_async_gen_asend_throw_concurrent_with_throw(self):
        import types

        @types.coroutine
        def _async_yield(v):
            return (yield v)

        class MyExc(Exception):
            pass

        async def agenfn():
            try:
                yield
            except MyExc:
                pass
            while True:
                try:
                    await _async_yield(None)
                except MyExc:
                    pass


        agen = agenfn()
        with self.assertRaises(StopIteration):
            agen.asend(None).send(None)

        gen = agen.athrow(MyExc)
        gen.throw(MyExc)
        gen2 = agen.asend(MyExc)

        with self.assertRaisesRegex(RuntimeError,
                r'anext\(\): asynchronous generator is already running'):
            gen2.throw(MyExc)

        with self.assertRaisesRegex(RuntimeError,
                r"cannot reuse already awaited __anext__\(\)/asend\(\)"):
            gen2.send(None)

    def test_async_gen_athrow_throw_concurrent_with_throw(self):
        import types

        @types.coroutine
        def _async_yield(v):
            return (yield v)

        class MyExc(Exception):
            pass

        async def agenfn():
            try:
                yield
            except MyExc:
                pass
            while True:
                try:
                    await _async_yield(None)
                except MyExc:
                    pass

        agen = agenfn()
        with self.assertRaises(StopIteration):
            agen.asend(None).send(None)

        gen = agen.athrow(MyExc)
        gen.throw(MyExc)
        gen2 = agen.athrow(None)

        with self.assertRaisesRegex(RuntimeError,
                r'athrow\(\): asynchronous generator is already running'):
            gen2.throw(MyExc)

        with self.assertRaisesRegex(RuntimeError,
                r"cannot reuse already awaited aclose\(\)/athrow\(\)"):
            gen2.send(None)

    def test_async_gen_3_arg_deprecation_warning(self):
        async def gen():
            yield 123

        with self.assertWarns(DeprecationWarning):
            x = gen().athrow(GeneratorExit, GeneratorExit(), None)
        with self.assertRaises(GeneratorExit):
            x.send(None)
            del x
            gc_collect()

    def test_async_gen_api_01(self):
        async def gen():
            yield 123

        g = gen()

        self.assertEqual(g.__name__, 'gen')
        g.__name__ = '123'
        self.assertEqual(g.__name__, '123')

        self.assertIn('.gen', g.__qualname__)
        g.__qualname__ = '123'
        self.assertEqual(g.__qualname__, '123')

        self.assertIsNone(g.ag_await)
        self.assertIsInstance(g.ag_frame, types.FrameType)
        self.assertFalse(g.ag_running)
        self.assertIsInstance(g.ag_code, types.CodeType)
        aclose = g.aclose()
        self.assertTrue(inspect.isawaitable(aclose))
        aclose.close()

    def test_async_gen_asend_close_runtime_error(self):
        import types

        @types.coroutine
        def _async_yield(v):
            return (yield v)

        async def agenfn():
            try:
                await _async_yield(None)
            except GeneratorExit:
                await _async_yield(None)
            return
            yield

        agen = agenfn()
        gen = agen.asend(None)
        gen.send(None)
        with self.assertRaisesRegex(RuntimeError, "coroutine ignored GeneratorExit"):
            gen.close()

    def test_async_gen_athrow_close_runtime_error(self):
        import types

        @types.coroutine
        def _async_yield(v):
            return (yield v)

        class MyExc(Exception):
            pass

        async def agenfn():
            try:
                yield
            except MyExc:
                try:
                    await _async_yield(None)
                except GeneratorExit:
                    await _async_yield(None)

        agen = agenfn()
        with self.assertRaises(StopIteration):
            agen.asend(None).send(None)
        gen = agen.athrow(MyExc)
        gen.send(None)
        with self.assertRaisesRegex(RuntimeError, "coroutine ignored GeneratorExit"):
            gen.close()


class AsyncGenAsyncioTest(unittest.TestCase):

    def setUp(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(None)

    def tearDown(self):
        self.loop.close()
        self.loop = None
        asyncio.events._set_event_loop_policy(None)

    def check_async_iterator_anext(self, ait_class):
        with self.subTest(anext="pure-Python"):
            self._check_async_iterator_anext(ait_class, py_anext)
        with self.subTest(anext="builtin"):
            self._check_async_iterator_anext(ait_class, anext)

    def _check_async_iterator_anext(self, ait_class, anext):
        g = ait_class()
        async def consume():
            results = []
            results.append(await anext(g))
            results.append(await anext(g))
            results.append(await anext(g, 'buckle my shoe'))
            return results
        res = self.loop.run_until_complete(consume())
        self.assertEqual(res, [1, 2, 'buckle my shoe'])
        with self.assertRaises(StopAsyncIteration):
            self.loop.run_until_complete(consume())

        async def test_2():
            g1 = ait_class()
            self.assertEqual(await anext(g1), 1)
            self.assertEqual(await anext(g1), 2)
            with self.assertRaises(StopAsyncIteration):
                await anext(g1)
            with self.assertRaises(StopAsyncIteration):
                await anext(g1)

            g2 = ait_class()
            self.assertEqual(await anext(g2, "default"), 1)
            self.assertEqual(await anext(g2, "default"), 2)
            self.assertEqual(await anext(g2, "default"), "default")
            self.assertEqual(await anext(g2, "default"), "default")

            return "completed"

        result = self.loop.run_until_complete(test_2())
        self.assertEqual(result, "completed")

        def test_send():
            p = ait_class()
            obj = anext(p, "completed")
            with self.assertRaises(StopIteration):
                with contextlib.closing(obj.__await__()) as g:
                    g.send(None)

        test_send()

        async def test_throw():
            p = ait_class()
            obj = anext(p, "completed")
            self.assertRaises(SyntaxError, obj.throw, SyntaxError)
            return "completed"

        result = self.loop.run_until_complete(test_throw())
        self.assertEqual(result, "completed")

    def test_async_generator_anext(self):
        async def agen():
            yield 1
            yield 2
        self.check_async_iterator_anext(agen)

    def test_python_async_iterator_anext(self):
        class MyAsyncIter:
            """Asynchronously yield 1, then 2."""
            def __init__(self):
                self.yielded = 0
            def __aiter__(self):
                return self
            async def __anext__(self):
                if self.yielded >= 2:
                    raise StopAsyncIteration()
                else:
                    self.yielded += 1
                    return self.yielded
        self.check_async_iterator_anext(MyAsyncIter)

    def test_python_async_iterator_types_coroutine_anext(self):
        import types
        class MyAsyncIterWithTypesCoro:
            """Asynchronously yield 1, then 2."""
            def __init__(self):
                self.yielded = 0
            def __aiter__(self):
                return self
            @types.coroutine
            def __anext__(self):
                if False:
                    yield "this is a generator-based coroutine"
                if self.yielded >= 2:
                    raise StopAsyncIteration()
                else:
                    self.yielded += 1
                    return self.yielded
        self.check_async_iterator_anext(MyAsyncIterWithTypesCoro)

    def test_async_gen_aiter(self):
        async def gen():
            yield 1
            yield 2
        g = gen()
        async def consume():
            return [i async for i in aiter(g)]
        res = self.loop.run_until_complete(consume())
        self.assertEqual(res, [1, 2])

    def test_async_gen_aiter_class(self):
        results = []
        class Gen:
            async def __aiter__(self):
                yield 1
                yield 2
        g = Gen()
        async def consume():
            ait = aiter(g)
            while True:
                try:
                    results.append(await anext(ait))
                except StopAsyncIteration:
                    break
        self.loop.run_until_complete(consume())
        self.assertEqual(results, [1, 2])

    def test_aiter_idempotent(self):
        async def gen():
            yield 1
        applied_once = aiter(gen())
        applied_twice = aiter(applied_once)
        self.assertIs(applied_once, applied_twice)

    def test_anext_bad_args(self):
        async def gen():
            yield 1
        async def call_with_too_few_args():
            await anext()
        async def call_with_too_many_args():
            await anext(gen(), 1, 3)
        async def call_with_wrong_type_args():
            await anext(1, gen())
        async def call_with_kwarg():
            await anext(aiterator=gen())
        with self.assertRaises(TypeError):
            self.loop.run_until_complete(call_with_too_few_args())
        with self.assertRaises(TypeError):
            self.loop.run_until_complete(call_with_too_many_args())
        with self.assertRaises(TypeError):
            self.loop.run_until_complete(call_with_wrong_type_args())
        with self.assertRaises(TypeError):
            self.loop.run_until_complete(call_with_kwarg())

    def test_anext_bad_await(self):
        async def bad_awaitable():
            class BadAwaitable:
                def __await__(self):
                    return 42
            class MyAsyncIter:
                def __aiter__(self):
                    return self
                def __anext__(self):
                    return BadAwaitable()
            regex = r"__await__.*iterator"
            awaitable = anext(MyAsyncIter(), "default")
            with self.assertRaisesRegex(TypeError, regex):
                await awaitable
            awaitable = anext(MyAsyncIter())
            with self.assertRaisesRegex(TypeError, regex):
                await awaitable
            return "completed"
        result = self.loop.run_until_complete(bad_awaitable())
        self.assertEqual(result, "completed")

    async def check_anext_returning_iterator(self, aiter_class):
        awaitable = anext(aiter_class(), "default")
        with self.assertRaises(TypeError):
            await awaitable
        awaitable = anext(aiter_class())
        with self.assertRaises(TypeError):
            await awaitable
        return "completed"

    def test_anext_return_iterator(self):
        class WithIterAnext:
            def __aiter__(self):
                return self
            def __anext__(self):
                return iter("abc")
        result = self.loop.run_until_complete(self.check_anext_returning_iterator(WithIterAnext))
        self.assertEqual(result, "completed")

    def test_anext_return_generator(self):
        class WithGenAnext:
            def __aiter__(self):
                return self
            def __anext__(self):
                yield
        result = self.loop.run_until_complete(self.check_anext_returning_iterator(WithGenAnext))
        self.assertEqual(result, "completed")

    def test_anext_await_raises(self):
        class RaisingAwaitable:
            def __await__(self):
                raise ZeroDivisionError()
                yield
        class WithRaisingAwaitableAnext:
            def __aiter__(self):
                return self
            def __anext__(self):
                return RaisingAwaitable()
        async def do_test():
            awaitable = anext(WithRaisingAwaitableAnext())
            with self.assertRaises(ZeroDivisionError):
                await awaitable
            awaitable = anext(WithRaisingAwaitableAnext(), "default")
            with self.assertRaises(ZeroDivisionError):
                await awaitable
            return "completed"
        result = self.loop.run_until_complete(do_test())
        self.assertEqual(result, "completed")

    def test_anext_iter(self):
        @types.coroutine
        def _async_yield(v):
            return (yield v)

        class MyError(Exception):
            pass

        async def agenfn():
            try:
                await _async_yield(1)
            except MyError:
                await _async_yield(2)
            return
            yield

        def test1(anext):
            agen = agenfn()
            with contextlib.closing(anext(agen, "default").__await__()) as g:
                self.assertEqual(g.send(None), 1)
                self.assertEqual(g.throw(MyError()), 2)
                try:
                    g.send(None)
                except StopIteration as e:
                    err = e
                else:
                    self.fail('StopIteration was not raised')
                self.assertEqual(err.value, "default")

        def test2(anext):
            agen = agenfn()
            with contextlib.closing(anext(agen, "default").__await__()) as g:
                self.assertEqual(g.send(None), 1)
                self.assertEqual(g.throw(MyError()), 2)
                with self.assertRaises(MyError):
                    g.throw(MyError())

        def test3(anext):
            agen = agenfn()
            with contextlib.closing(anext(agen, "default").__await__()) as g:
                self.assertEqual(g.send(None), 1)
                g.close()
                with self.assertRaisesRegex(RuntimeError, 'cannot reuse'):
                    self.assertEqual(g.send(None), 1)

        def test4(anext):
            @types.coroutine
            def _async_yield(v):
                yield v * 10
                return (yield (v * 10 + 1))

            async def agenfn():
                try:
                    await _async_yield(1)
                except MyError:
                    await _async_yield(2)
                return
                yield

            agen = agenfn()
            with contextlib.closing(anext(agen, "default").__await__()) as g:
                self.assertEqual(g.send(None), 10)
                self.assertEqual(g.throw(MyError()), 20)
                with self.assertRaisesRegex(MyError, 'val'):
                    g.throw(MyError('val'))

        def test5(anext):
            @types.coroutine
            def _async_yield(v):
                yield v * 10
                return (yield (v * 10 + 1))

            async def agenfn():
                try:
                    await _async_yield(1)
                except MyError:
                    return
                yield 'aaa'

            agen = agenfn()
            with contextlib.closing(anext(agen, "default").__await__()) as g:
                self.assertEqual(g.send(None), 10)
                with self.assertRaisesRegex(StopIteration, 'default'):
                    g.throw(MyError())

        def test6(anext):
            @types.coroutine
            def _async_yield(v):
                yield v * 10
                return (yield (v * 10 + 1))

            async def agenfn():
                await _async_yield(1)
                yield 'aaa'

            agen = agenfn()
            with contextlib.closing(anext(agen, "default").__await__()) as g:
                with self.assertRaises(MyError):
                    g.throw(MyError())

        def run_test(test):
            with self.subTest('pure-Python anext()'):
                test(py_anext)
            with self.subTest('builtin anext()'):
                test(anext)

        run_test(test1)
        run_test(test2)
        run_test(test3)
        run_test(test4)
        run_test(test5)
        run_test(test6)

    def test_aiter_bad_args(self):
        async def gen():
            yield 1
        async def call_with_too_few_args():
            await aiter()
        async def call_with_too_many_args():
            await aiter(gen(), 1)
        async def call_with_wrong_type_arg():
            await aiter(1)
        with self.assertRaises(TypeError):
            self.loop.run_until_complete(call_with_too_few_args())
        with self.assertRaises(TypeError):
            self.loop.run_until_complete(call_with_too_many_args())
        with self.assertRaises(TypeError):
            self.loop.run_until_complete(call_with_wrong_type_arg())

    async def to_list(self, gen):
        res = []
        async for i in gen:
            res.append(i)
        return res

    def test_async_gen_asyncio_01(self):
        async def gen():
            yield 1
            await asyncio.sleep(0.01)
            yield 2
            await asyncio.sleep(0.01)
            return
            yield 3

        res = self.loop.run_until_complete(self.to_list(gen()))
        self.assertEqual(res, [1, 2])

    def test_async_gen_asyncio_02(self):
        async def gen():
            yield 1
            await asyncio.sleep(0.01)
            yield 2
            1 / 0
            yield 3

        with self.assertRaises(ZeroDivisionError):
            self.loop.run_until_complete(self.to_list(gen()))

    def test_async_gen_asyncio_03(self):
        loop = self.loop

        class Gen:
            async def __aiter__(self):
                yield 1
                await asyncio.sleep(0.01)
                yield 2

        res = loop.run_until_complete(self.to_list(Gen()))
        self.assertEqual(res, [1, 2])

    def test_async_gen_asyncio_anext_04(self):
        async def foo():
            yield 1
            await asyncio.sleep(0.01)
            try:
                yield 2
                yield 3
            except ZeroDivisionError:
                yield 1000
            await asyncio.sleep(0.01)
            yield 4

        async def run1():
            it = foo().__aiter__()

            self.assertEqual(await it.__anext__(), 1)
            self.assertEqual(await it.__anext__(), 2)
            self.assertEqual(await it.__anext__(), 3)
            self.assertEqual(await it.__anext__(), 4)
            with self.assertRaises(StopAsyncIteration):
                await it.__anext__()
            with self.assertRaises(StopAsyncIteration):
                await it.__anext__()

        async def run2():
            it = foo().__aiter__()

            self.assertEqual(await it.__anext__(), 1)
            self.assertEqual(await it.__anext__(), 2)
            try:
                it.__anext__().throw(ZeroDivisionError)
            except StopIteration as ex:
                self.assertEqual(ex.args[0], 1000)
            else:
                self.fail('StopIteration was not raised')
            self.assertEqual(await it.__anext__(), 4)
            with self.assertRaises(StopAsyncIteration):
                await it.__anext__()

        self.loop.run_until_complete(run1())
        self.loop.run_until_complete(run2())

    def test_async_gen_asyncio_anext_05(self):
        async def foo():
            v = yield 1
            v = yield v
            yield v * 100

        async def run():
            it = foo().__aiter__()

            try:
                it.__anext__().send(None)
            except StopIteration as ex:
                self.assertEqual(ex.args[0], 1)
            else:
                self.fail('StopIteration was not raised')

            try:
                it.__anext__().send(10)
            except StopIteration as ex:
                self.assertEqual(ex.args[0], 10)
            else:
                self.fail('StopIteration was not raised')

            try:
                it.__anext__().send(12)
            except StopIteration as ex:
                self.assertEqual(ex.args[0], 1200)
            else:
                self.fail('StopIteration was not raised')

            with self.assertRaises(StopAsyncIteration):
                await it.__anext__()

        self.loop.run_until_complete(run())

    def test_async_gen_asyncio_anext_06(self):
        DONE = 0

        # test synchronous generators
        def foo():
            try:
                yield
            except:
                pass
        g = foo()
        g.send(None)
        with self.assertRaises(StopIteration):
            g.send(None)

        # now with asynchronous generators

        async def gen():
            nonlocal DONE
            try:
                yield
            except:
                pass
            DONE = 1

        async def run():
            nonlocal DONE
            g = gen()
            await g.asend(None)
            with self.assertRaises(StopAsyncIteration):
                await g.asend(None)
            DONE += 10

        self.loop.run_until_complete(run())
        self.assertEqual(DONE, 11)

    def test_async_gen_asyncio_anext_tuple(self):
        async def foo():
            try:
                yield (1,)
            except ZeroDivisionError:
                yield (2,)

        async def run():
            it = foo().__aiter__()

            self.assertEqual(await it.__anext__(), (1,))
            with self.assertRaises(StopIteration) as cm:
                it.__anext__().throw(ZeroDivisionError)
            self.assertEqual(cm.exception.args[0], (2,))
            with self.assertRaises(StopAsyncIteration):
                await it.__anext__()

        self.loop.run_until_complete(run())

    def test_async_gen_asyncio_anext_tuple_no_exceptions(self):
        # StopAsyncIteration exceptions should be cleared.
        # See: https://github.com/python/cpython/issues/128078.

        async def foo():
            if False:
                yield (1, 2)

        async def run():
            it = foo().__aiter__()
            with self.assertRaises(StopAsyncIteration):
                await it.__anext__()
            res = await anext(it, ('a', 'b'))
            self.assertTupleEqual(res, ('a', 'b'))

        self.loop.run_until_complete(run())

    def test_sync_anext_raises_exception(self):
        # See: https://github.com/python/cpython/issues/131670
        msg = 'custom'
        for exc_type in [
            StopAsyncIteration,
            StopIteration,
            ValueError,
            Exception,
        ]:
            exc = exc_type(msg)
            with self.subTest(exc=exc):
                class A:
                    def __anext__(self):
                        raise exc

                with self.assertRaisesRegex(exc_type, msg):
                    anext(A())
                with self.assertRaisesRegex(exc_type, msg):
                    anext(A(), 1)

    def test_async_gen_asyncio_anext_stopiteration(self):
        async def foo():
            try:
                yield StopIteration(1)
            except ZeroDivisionError:
                yield StopIteration(3)

        async def run():
            it = foo().__aiter__()

            v = await it.__anext__()
            self.assertIsInstance(v, StopIteration)
            self.assertEqual(v.value, 1)
            with self.assertRaises(StopIteration) as cm:
                it.__anext__().throw(ZeroDivisionError)
            v = cm.exception.args[0]
            self.assertIsInstance(v, StopIteration)
            self.assertEqual(v.value, 3)
            with self.assertRaises(StopAsyncIteration):
                await it.__anext__()

        self.loop.run_until_complete(run())

    def test_async_gen_asyncio_aclose_06(self):
        async def foo():
            try:
                yield 1
                1 / 0
            finally:
                await asyncio.sleep(0.01)
                yield 12

        async def run():
            gen = foo()
            it = gen.__aiter__()
            await it.__anext__()
            await gen.aclose()

        with self.assertRaisesRegex(
                RuntimeError,
                "async generator ignored GeneratorExit"):
            self.loop.run_until_complete(run())

    def test_async_gen_asyncio_aclose_07(self):
        DONE = 0

        async def foo():
            nonlocal DONE
            try:
                yield 1
                1 / 0
            finally:
                await asyncio.sleep(0.01)
                await asyncio.sleep(0.01)
                DONE += 1
            DONE += 1000

        async def run():
            gen = foo()
            it = gen.__aiter__()
            await it.__anext__()
            await gen.aclose()

        self.loop.run_until_complete(run())
        self.assertEqual(DONE, 1)

    def test_async_gen_asyncio_aclose_08(self):
        DONE = 0

        fut = asyncio.Future(loop=self.loop)

        async def foo():
            nonlocal DONE
            try:
                yield 1
                await fut
                DONE += 1000
                yield 2
            finally:
                await asyncio.sleep(0.01)
                await asyncio.sleep(0.01)
                DONE += 1
            DONE += 1000

        async def run():
            gen = foo()
            it = gen.__aiter__()
            self.assertEqual(await it.__anext__(), 1)
            await gen.aclose()

        self.loop.run_until_complete(run())
        self.assertEqual(DONE, 1)

        # Silence ResourceWarnings
        fut.cancel()
        self.loop.run_until_complete(asyncio.sleep(0.01))

    def test_async_gen_asyncio_gc_aclose_09(self):
        DONE = 0

        async def gen():
            nonlocal DONE
            try:
                while True:
                    yield 1
            finally:
                await asyncio.sleep(0)
                DONE = 1

        async def run():
            g = gen()
            await g.__anext__()
            await g.__anext__()
            del g
            gc_collect()  # For PyPy or other GCs.

            # Starts running the aclose task
            await asyncio.sleep(0)
            # For asyncio.sleep(0) in finally block
            await asyncio.sleep(0)

        self.loop.run_until_complete(run())
        self.assertEqual(DONE, 1)

    def test_async_gen_asyncio_aclose_10(self):
        DONE = 0

        # test synchronous generators
        def foo():
            try:
                yield
            except:
                pass
        g = foo()
        g.send(None)
        g.close()

        # now with asynchronous generators

        async def gen():
            nonlocal DONE
            try:
                yield
            except:
                pass
            DONE = 1

        async def run():
            nonlocal DONE
            g = gen()
            await g.asend(None)
            await g.aclose()
            DONE += 10

        self.loop.run_until_complete(run())
        self.assertEqual(DONE, 11)

    def test_async_gen_asyncio_aclose_11(self):
        DONE = 0

        # test synchronous generators
        def foo():
            try:
                yield
            except:
                pass
            yield
        g = foo()
        g.send(None)
        with self.assertRaisesRegex(RuntimeError, 'ignored GeneratorExit'):
            g.close()

        # now with asynchronous generators

        async def gen():
            nonlocal DONE
            try:
                yield
            except:
                pass
            yield
            DONE += 1

        async def run():
            nonlocal DONE
            g = gen()
            await g.asend(None)
            with self.assertRaisesRegex(RuntimeError, 'ignored GeneratorExit'):
                await g.aclose()
            DONE += 10

        self.loop.run_until_complete(run())
        self.assertEqual(DONE, 10)

    def test_async_gen_asyncio_aclose_12(self):
        DONE = 0

        async def target():
            await asyncio.sleep(0.01)
            1 / 0

        async def foo():
            nonlocal DONE
            task = asyncio.create_task(target())
            try:
                yield 1
            finally:
                try:
                    await task
                except ZeroDivisionError:
                    DONE = 1

        async def run():
            gen = foo()
            it = gen.__aiter__()
            await it.__anext__()
            await gen.aclose()

        self.loop.run_until_complete(run())
        self.assertEqual(DONE, 1)

    def test_async_gen_asyncio_asend_01(self):
        DONE = 0

        # Sanity check:
        def sgen():
            v = yield 1
            yield v * 2
        sg = sgen()
        v = sg.send(None)
        self.assertEqual(v, 1)
        v = sg.send(100)
        self.assertEqual(v, 200)

        async def gen():
            nonlocal DONE
            try:
                await asyncio.sleep(0.01)
                v = yield 1
                await asyncio.sleep(0.01)
                yield v * 2
                await asyncio.sleep(0.01)
                return
            finally:
                await asyncio.sleep(0.01)
                await asyncio.sleep(0.01)
                DONE = 1

        async def run():
            g = gen()

            v = await g.asend(None)
            self.assertEqual(v, 1)

            v = await g.asend(100)
            self.assertEqual(v, 200)

            with self.assertRaises(StopAsyncIteration):
                await g.asend(None)

        self.loop.run_until_complete(run())
        self.assertEqual(DONE, 1)

    def test_async_gen_asyncio_asend_02(self):
        DONE = 0

        async def sleep_n_crash(delay):
            await asyncio.sleep(delay)
            1 / 0

        async def gen():
            nonlocal DONE
            try:
                await asyncio.sleep(0.01)
                v = yield 1
                await sleep_n_crash(0.01)
                DONE += 1000
                yield v * 2
            finally:
                await asyncio.sleep(0.01)
                await asyncio.sleep(0.01)
                DONE = 1

        async def run():
            g = gen()

            v = await g.asend(None)
            self.assertEqual(v, 1)

            await g.asend(100)

        with self.assertRaises(ZeroDivisionError):
            self.loop.run_until_complete(run())
        self.assertEqual(DONE, 1)

    def test_async_gen_asyncio_asend_03(self):
        DONE = 0

        async def sleep_n_crash(delay):
            fut = asyncio.ensure_future(asyncio.sleep(delay),
                                        loop=self.loop)
            self.loop.call_later(delay / 2, lambda: fut.cancel())
            return await fut

        async def gen():
            nonlocal DONE
            try:
                await asyncio.sleep(0.01)
                v = yield 1
                await sleep_n_crash(0.01)
                DONE += 1000
                yield v * 2
            finally:
                await asyncio.sleep(0.01)
                await asyncio.sleep(0.01)
                DONE = 1

        async def run():
            g = gen()

            v = await g.asend(None)
            self.assertEqual(v, 1)

            await g.asend(100)

        with self.assertRaises(asyncio.CancelledError):
            self.loop.run_until_complete(run())
        self.assertEqual(DONE, 1)

    def test_async_gen_asyncio_athrow_01(self):
        DONE = 0

        class FooEr(Exception):
            pass

        # Sanity check:
        def sgen():
            try:
                v = yield 1
            except FooEr:
                v = 1000
            yield v * 2
        sg = sgen()
        v = sg.send(None)
        self.assertEqual(v, 1)
        v = sg.throw(FooEr)
        self.assertEqual(v, 2000)
        with self.assertRaises(StopIteration):
            sg.send(None)

        async def gen():
            nonlocal DONE
            try:
                await asyncio.sleep(0.01)
                try:
                    v = yield 1
                except FooEr:
                    v = 1000
                    await asyncio.sleep(0.01)
                yield v * 2
                await asyncio.sleep(0.01)
                # return
            finally:
                await asyncio.sleep(0.01)
                await asyncio.sleep(0.01)
                DONE = 1

        async def run():
            g = gen()

            v = await g.asend(None)
            self.assertEqual(v, 1)

            v = await g.athrow(FooEr)
            self.assertEqual(v, 2000)

            with self.assertRaises(StopAsyncIteration):
                await g.asend(None)

        self.loop.run_until_complete(run())
        self.assertEqual(DONE, 1)

    def test_async_gen_asyncio_athrow_02(self):
        DONE = 0

        class FooEr(Exception):
            pass

        async def sleep_n_crash(delay):
            fut = asyncio.ensure_future(asyncio.sleep(delay),
                                        loop=self.loop)
            self.loop.call_later(delay / 2, lambda: fut.cancel())
            return await fut

        async def gen():
            nonlocal DONE
            try:
                await asyncio.sleep(0.01)
                try:
                    v = yield 1
                except FooEr:
                    await sleep_n_crash(0.01)
                yield v * 2
                await asyncio.sleep(0.01)
                # return
            finally:
                await asyncio.sleep(0.01)
                await asyncio.sleep(0.01)
                DONE = 1

        async def run():
            g = gen()

            v = await g.asend(None)
            self.assertEqual(v, 1)

            try:
                await g.athrow(FooEr)
            except asyncio.CancelledError:
                self.assertEqual(DONE, 1)
                raise
            else:
                self.fail('CancelledError was not raised')

        with self.assertRaises(asyncio.CancelledError):
            self.loop.run_until_complete(run())
        self.assertEqual(DONE, 1)

    def test_async_gen_asyncio_athrow_03(self):
        DONE = 0

        # test synchronous generators
        def foo():
            try:
                yield
            except:
                pass
        g = foo()
        g.send(None)
        with self.assertRaises(StopIteration):
            g.throw(ValueError)

        # now with asynchronous generators

        async def gen():
            nonlocal DONE
            try:
                yield
            except:
                pass
            DONE = 1

        async def run():
            nonlocal DONE
            g = gen()
            await g.asend(None)
            with self.assertRaises(StopAsyncIteration):
                await g.athrow(ValueError)
            DONE += 10

        self.loop.run_until_complete(run())
        self.assertEqual(DONE, 11)

    def test_async_gen_asyncio_athrow_tuple(self):
        async def gen():
            try:
                yield 1
            except ZeroDivisionError:
                yield (2,)

        async def run():
            g = gen()
            v = await g.asend(None)
            self.assertEqual(v, 1)
            v = await g.athrow(ZeroDivisionError)
            self.assertEqual(v, (2,))
            with self.assertRaises(StopAsyncIteration):
                await g.asend(None)

        self.loop.run_until_complete(run())

    def test_async_gen_asyncio_athrow_stopiteration(self):
        async def gen():
            try:
                yield 1
            except ZeroDivisionError:
                yield StopIteration(2)

        async def run():
            g = gen()
            v = await g.asend(None)
            self.assertEqual(v, 1)
            v = await g.athrow(ZeroDivisionError)
            self.assertIsInstance(v, StopIteration)
            self.assertEqual(v.value, 2)
            with self.assertRaises(StopAsyncIteration):
                await g.asend(None)

        self.loop.run_until_complete(run())

    def test_async_gen_asyncio_shutdown_01(self):
        finalized = 0

        async def waiter(timeout):
            nonlocal finalized
            try:
                await asyncio.sleep(timeout)
                yield 1
            finally:
                await asyncio.sleep(0)
                finalized += 1

        async def wait():
            async for _ in waiter(1):
                pass

        t1 = self.loop.create_task(wait())
        t2 = self.loop.create_task(wait())

        self.loop.run_until_complete(asyncio.sleep(0.1))

        # Silence warnings
        t1.cancel()
        t2.cancel()

        with self.assertRaises(asyncio.CancelledError):
            self.loop.run_until_complete(t1)
        with self.assertRaises(asyncio.CancelledError):
            self.loop.run_until_complete(t2)

        self.loop.run_until_complete(self.loop.shutdown_asyncgens())

        self.assertEqual(finalized, 2)

    def test_async_gen_asyncio_shutdown_02(self):
        messages = []

        def exception_handler(loop, context):
            messages.append(context)

        async def async_iterate():
            yield 1
            yield 2

        it = async_iterate()
        async def main():
            loop = asyncio.get_running_loop()
            loop.set_exception_handler(exception_handler)

            async for i in it:
                break

        asyncio.run(main())

        self.assertEqual(messages, [])

    def test_async_gen_asyncio_shutdown_exception_01(self):
        messages = []

        def exception_handler(loop, context):
            messages.append(context)

        async def async_iterate():
            try:
                yield 1
                yield 2
            finally:
                1/0

        it = async_iterate()
        async def main():
            loop = asyncio.get_running_loop()
            loop.set_exception_handler(exception_handler)

            async for i in it:
                break

        asyncio.run(main())

        message, = messages
        self.assertEqual(message['asyncgen'], it)
        self.assertIsInstance(message['exception'], ZeroDivisionError)
        self.assertIn('an error occurred during closing of asynchronous generator',
                      message['message'])

    def test_async_gen_asyncio_shutdown_exception_02(self):
        messages = []

        def exception_handler(loop, context):
            messages.append(context)

        async def async_iterate():
            try:
                yield 1
                yield 2
            finally:
                1/0

        async def main():
            loop = asyncio.get_running_loop()
            loop.set_exception_handler(exception_handler)

            async for i in async_iterate():
                break
            gc_collect()

        asyncio.run(main())

        message, = messages
        self.assertIsInstance(message['exception'], ZeroDivisionError)
        self.assertIn('unhandled exception during asyncio.run() shutdown',
                      message['message'])
        del message, messages
        gc_collect()

    def test_async_gen_expression_01(self):
        async def arange(n):
            for i in range(n):
                await asyncio.sleep(0.01)
                yield i

        def make_arange(n):
            # This syntax is legal starting with Python 3.7
            return (i * 2 async for i in arange(n))

        async def run():
            return [i async for i in make_arange(10)]

        res = self.loop.run_until_complete(run())
        self.assertEqual(res, [i * 2 for i in range(10)])

    def test_async_gen_expression_02(self):
        async def wrap(n):
            await asyncio.sleep(0.01)
            return n

        def make_arange(n):
            # This syntax is legal starting with Python 3.7
            return (i * 2 for i in range(n) if await wrap(i))

        async def run():
            return [i async for i in make_arange(10)]

        res = self.loop.run_until_complete(run())
        self.assertEqual(res, [i * 2 for i in range(1, 10)])

    def test_asyncgen_nonstarted_hooks_are_cancellable(self):
        # See https://bugs.python.org/issue38013
        messages = []

        def exception_handler(loop, context):
            messages.append(context)

        async def async_iterate():
            yield 1
            yield 2

        async def main():
            loop = asyncio.get_running_loop()
            loop.set_exception_handler(exception_handler)

            async for i in async_iterate():
                break

        asyncio.run(main())

        self.assertEqual([], messages)
        gc_collect()

    def test_async_gen_await_same_anext_coro_twice(self):
        async def async_iterate():
            yield 1
            yield 2

        async def run():
            it = async_iterate()
            nxt = it.__anext__()
            await nxt
            with self.assertRaisesRegex(
                    RuntimeError,
                    r"cannot reuse already awaited __anext__\(\)/asend\(\)"
            ):
                await nxt

            await it.aclose()  # prevent unfinished iterator warning

        self.loop.run_until_complete(run())

    def test_async_gen_await_same_aclose_coro_twice(self):
        async def async_iterate():
            yield 1
            yield 2

        async def run():
            it = async_iterate()
            nxt = it.aclose()
            await nxt
            with self.assertRaisesRegex(
                    RuntimeError,
                    r"cannot reuse already awaited aclose\(\)/athrow\(\)"
            ):
                await nxt

        self.loop.run_until_complete(run())

    def test_async_gen_throw_same_aclose_coro_twice(self):
        async def async_iterate():
            yield 1
            yield 2

        it = async_iterate()
        nxt = it.aclose()
        with self.assertRaises(StopIteration):
            nxt.throw(GeneratorExit)

        with self.assertRaisesRegex(
            RuntimeError,
            r"cannot reuse already awaited aclose\(\)/athrow\(\)"
        ):
            nxt.throw(GeneratorExit)

    def test_async_gen_throw_custom_same_aclose_coro_twice(self):
        async def async_iterate():
            yield 1
            yield 2

        it = async_iterate()

        class MyException(Exception):
            pass

        nxt = it.aclose()
        with self.assertRaises(MyException):
            nxt.throw(MyException)

        with self.assertRaisesRegex(
            RuntimeError,
            r"cannot reuse already awaited aclose\(\)/athrow\(\)"
        ):
            nxt.throw(MyException)

    def test_async_gen_throw_custom_same_athrow_coro_twice(self):
        async def async_iterate():
            yield 1
            yield 2

        it = async_iterate()

        class MyException(Exception):
            pass

        nxt = it.athrow(MyException)
        with self.assertRaises(MyException):
            nxt.throw(MyException)

        with self.assertRaisesRegex(
            RuntimeError,
            r"cannot reuse already awaited aclose\(\)/athrow\(\)"
        ):
            nxt.throw(MyException)

    def test_async_gen_aclose_twice_with_different_coros(self):
        # Regression test for https://bugs.python.org/issue39606
        async def async_iterate():
            yield 1
            yield 2

        async def run():
            it = async_iterate()
            await it.aclose()
            await it.aclose()

        self.loop.run_until_complete(run())

    def test_async_gen_aclose_after_exhaustion(self):
        # Regression test for https://bugs.python.org/issue39606
        async def async_iterate():
            yield 1
            yield 2

        async def run():
            it = async_iterate()
            async for _ in it:
                pass
            await it.aclose()

        self.loop.run_until_complete(run())

    def test_async_gen_aclose_compatible_with_get_stack(self):
        async def async_generator():
            yield object()

        async def run():
            ag = async_generator()
            asyncio.create_task(ag.aclose())
            tasks = asyncio.all_tasks()
            for task in tasks:
                # No AttributeError raised
                task.get_stack()

        self.loop.run_until_complete(run())


class TestUnawaitedWarnings(unittest.TestCase):
    def test_asend(self):
        async def gen():
            yield 1

        # gh-113753: asend objects allocated from a free-list should warn.
        # Ensure there is a finalized 'asend' object ready to be reused.
        try:
            g = gen()
            g.asend(None).send(None)
        except StopIteration:
            pass

        msg = f"coroutine method 'asend' of '{gen.__qualname__}' was never awaited"
        with self.assertWarnsRegex(RuntimeWarning, msg):
            g = gen()
            g.asend(None)
            gc_collect()

    def test_athrow(self):
        async def gen():
            yield 1

        msg = f"coroutine method 'athrow' of '{gen.__qualname__}' was never awaited"
        with self.assertWarnsRegex(RuntimeWarning, msg):
            g = gen()
            g.athrow(RuntimeError)
            gc_collect()

    def test_athrow_throws_immediately(self):
        async def gen():
            yield 1

        g = gen()
        msg = "athrow expected at least 1 argument, got 0"
        with self.assertRaisesRegex(TypeError, msg):
            g.athrow()

    def test_aclose(self):
        async def gen():
            yield 1

        msg = f"coroutine method 'aclose' of '{gen.__qualname__}' was never awaited"
        with self.assertWarnsRegex(RuntimeWarning, msg):
            g = gen()
            g.aclose()
            gc_collect()

    def test_aclose_throw(self):
        async def gen():
            return
            yield

        class MyException(Exception):
            pass

        g = gen()
        with self.assertRaises(MyException):
            g.aclose().throw(MyException)

        del g
        gc_collect()  # does not warn unawaited

    def test_asend_send_already_running(self):
        @types.coroutine
        def _async_yield(v):
            return (yield v)

        async def agenfn():
            while True:
                await _async_yield(1)
            return
            yield

        agen = agenfn()
        gen = agen.asend(None)
        gen.send(None)
        gen2 = agen.asend(None)

        with self.assertRaisesRegex(RuntimeError,
                r'anext\(\): asynchronous generator is already running'):
            gen2.send(None)

        del gen2
        gc_collect()  # does not warn unawaited


    def test_athrow_send_already_running(self):
        @types.coroutine
        def _async_yield(v):
            return (yield v)

        async def agenfn():
            while True:
                await _async_yield(1)
            return
            yield

        agen = agenfn()
        gen = agen.asend(None)
        gen.send(None)
        gen2 = agen.athrow(Exception)

        with self.assertRaisesRegex(RuntimeError,
                r'athrow\(\): asynchronous generator is already running'):
            gen2.send(None)

        del gen2
        gc_collect()  # does not warn unawaited

if __name__ == "__main__":
    unittest.main()
