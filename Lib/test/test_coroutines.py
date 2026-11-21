import ast
import contextlib
import copy
import builtins
import gc
import inspect
import pickle
import sys
import types
import traceback
import unittest
import warnings
from test import support
from test.support import import_helper
from test.support import warnings_helper
from test.support.script_helper import assert_python_ok
try:
    import _testcapi
except ImportError:
    _testcapi = None


class AsyncYieldFrom:
    def __init__(self, obj):
        self.obj = obj

    def __await__(self):
        yield from self.obj


class AsyncYield:
    def __init__(self, value):
        self.value = value

    def __await__(self):
        yield self.value


async def asynciter(iterable):
    """Convert an iterable to an asynchronous iterator."""
    for x in iterable:
        yield x


def run_async(coro):
    assert coro.__class__ in {types.GeneratorType, types.CoroutineType}

    buffer = []
    result = None
    while True:
        try:
            buffer.append(coro.send(None))
        except StopIteration as ex:
            result = ex.args[0] if ex.args else None
            break
    return buffer, result


def run_async__await__(coro):
    assert coro.__class__ is types.CoroutineType
    aw = coro.__await__()
    buffer = []
    result = None
    i = 0
    while True:
        try:
            if i % 2:
                buffer.append(next(aw))
            else:
                buffer.append(aw.send(None))
            i += 1
        except StopIteration as ex:
            result = ex.args[0] if ex.args else None
            break
    return buffer, result


@contextlib.contextmanager
def silence_coro_gc():
    with warnings.catch_warnings():
        warnings.simplefilter("ignore")
        yield
        support.gc_collect()


class AsyncBadSyntaxTest(unittest.TestCase):

    def test_badsyntax_1(self):
        samples = [
            """def foo():
                await something()
            """,

            """await something()""",

            """async def foo():
                yield from []
            """,

            """async def foo():
                await await fut
            """,

            """async def foo(a=await something()):
                pass
            """,

            """async def foo(a:await something()):
                pass
            """,

            """async def foo():
                def bar():
                 [i async for i in els]
            """,

            """async def foo():
                def bar():
                 [await i for i in els]
            """,

            """async def foo():
                def bar():
                 [i for i in els
                    async for b in els]
            """,

            """async def foo():
                def bar():
                 [i for i in els
                    for c in b
                    async for b in els]
            """,

            """async def foo():
                def bar():
                 [i for i in els
                    async for b in els
                    for c in b]
            """,

            """async def foo():
                def bar():
                 [[async for i in b] for b in els]
            """,

            """async def foo():
                def bar():
                 [i for i in els
                    for b in await els]
            """,

            """async def foo():
                def bar():
                 [i for i in els
                    for b in els
                        if await b]
            """,

            """async def foo():
                def bar():
                 [i for i in await els]
            """,

            """async def foo():
                def bar():
                 [i for i in els if await i]
            """,

            """def bar():
                 [i async for i in els]
            """,

            """def bar():
                 {i: i async for i in els}
            """,

            """def bar():
                 {i async for i in els}
            """,

            """def bar():
                 [await i for i in els]
            """,

            """def bar():
                 [i for i in els
                    async for b in els]
            """,

            """def bar():
                 [i for i in els
                    for c in b
                    async for b in els]
            """,

            """def bar():
                 [i for i in els
                    async for b in els
                    for c in b]
            """,

            """def bar():
                 [i for i in els
                    for b in await els]
            """,

            """def bar():
                 [i for i in els
                    for b in els
                        if await b]
            """,

            """def bar():
                 [i for i in await els]
            """,

            """def bar():
                 [i for i in els if await i]
            """,

            """def bar():
                 [[i async for i in a] for a in elts]
            """,

            """[[i async for i in a] for a in elts]
            """,

            """async def foo():
                await
            """,

            """async def foo():
                   def bar(): pass
                   await = 1
            """,

            """async def foo():

                   def bar(): pass
                   await = 1
            """,

            """async def foo():
                   def bar(): pass
                   if 1:
                       await = 1
            """,

            """def foo():
                   async def bar(): pass
                   if 1:
                       await a
            """,

            """def foo():
                   async def bar(): pass
                   await a
            """,

            """def foo():
                   def baz(): pass
                   async def bar(): pass
                   await a
            """,

            """def foo():
                   def baz(): pass
                   # 456
                   async def bar(): pass
                   # 123
                   await a
            """,

            """async def foo():
                   def baz(): pass
                   # 456
                   async def bar(): pass
                   # 123
                   await = 2
            """,

            """def foo():

                   def baz(): pass

                   async def bar(): pass

                   await a
            """,

            """async def foo():

                   def baz(): pass

                   async def bar(): pass

                   await = 2
            """,

            """async def foo():
                   def async(): pass
            """,

            """async def foo():
                   def await(): pass
            """,

            """async def foo():
                   def bar():
                       await
            """,

            """async def foo():
                   return lambda async: await
            """,

            """async def foo():
                   return lambda a: await
            """,

            """await a()""",

            """async def foo(a=await b):
                   pass
            """,

            """async def foo(a:await b):
                   pass
            """,

            """def baz():
                   async def foo(a=await b):
                       pass
            """,

            """async def foo(async):
                   pass
            """,

            """async def foo():
                   def bar():
                        def baz():
                            async = 1
            """,

            """async def foo():
                   def bar():
                        def baz():
                            pass
                        async = 1
            """,

            """def foo():
                   async def bar():

                        async def baz():
                            pass

                        def baz():
                            42

                        async = 1
            """,

            """async def foo():
                   def bar():
                        def baz():
                            pass\nawait foo()
            """,

            """def foo():
                   def bar():
                        async def baz():
                            pass\nawait foo()
            """,

            """async def foo(await):
                   pass
            """,

            """def foo():

                   async def bar(): pass

                   await a
            """,

            """def foo():
                   async def bar():
                        pass\nawait a
            """,
            """def foo():
                   async for i in arange(2):
                       pass
            """,
            """def foo():
                   async with resource:
                       pass
            """,
            """async with resource:
                   pass
            """,
            """async for i in arange(2):
                   pass
            """,
            ]

        for code in samples:
            with self.subTest(code=code), self.assertRaises(SyntaxError):
                compile(code, "<test>", "exec")

    def test_async_comprehension_scope(self):
        # List/set/dict comprehensions with await or async for are allowed
        # only in async functions, or at module level with top-level await.
        allowed = [
            "async def f():\n    [await x for x in y]",
            "async def f():\n    {await x for x in y}",
            "async def f():\n    {k: await x for k, x in y}",
            "async def f():\n    [x async for x in y]",
            "async def f():\n    {x async for x in y}",
            "async def f():\n    {k: x async for k, x in y}",
            "async def f():\n    [[await x for x in y] for y in z]",
            # Defaults, bases, and genexp iterables are evaluated in the
            # enclosing scope.
            "async def outer():\n    async def f(x=[await y for y in z]): pass",
            "async def f():\n    class C([await x for x in y]): pass",
            "async def f():\n    (x for x in [await y for y in z])",
            "async def f():\n    (x for x in [y async for y in z])",
        ]
        for code in allowed:
            with self.subTest(code=code):
                compile(code, "<test>", "exec")

        # Generator expressions with await are async genexps and may appear
        # outside async functions. A listcomp nested in a genexp body is also
        # allowed (the genexp becomes an async generator).
        for code in [
            "(await x for x in y)",
            "def f():\n    (await x for x in y)",
            "class C:\n    (await x for x in y)",
            "lambda: (await x for x in y)",
            "([await x for x in y] for y in z)",
            "def f():\n    ([await x for x in y] for y in z)",
            "class C:\n    ([await x for x in y] for y in z)",
            "async def f():\n    ([await x for x in y] for y in z)",
        ]:
            with self.subTest(code=code):
                compile(code, "<test>", "exec")

        err = "asynchronous comprehension outside of an asynchronous function"
        invalid = [
            "[await x for x in y]",
            "{await x for x in y}",
            "{k: await x for k, x in y}",
            "[x async for x in y]",
            "{x async for x in y}",
            "{k: x async for k, x in y}",
            "[[await x for x in y] for y in z]",
            "[[x async for x in y] for y in z]",
            "def f():\n    [await x for x in y]",
            "def f():\n    [x async for x in y]",
            "async def f():\n    def g():\n        [await x for x in y]",
            "class C:\n    [await x for x in y]",
            "class C:\n    {await x for x in y}",
            "class C:\n    {k: await x for k, x in y}",
            "class C:\n    [x async for x in y]",
            "class C:\n    [[await x for x in y] for y in z]",
            "async def f():\n    class C:\n        x = [await y for y in z]",
            "async def f():\n    class C:\n        x = [y async for y in z]",
            # Lambdas are never async, even inside async def.
            "lambda: [await x for x in y]",
            "async def f():\n    lambda: [await x for x in y]",
            "class C:\n    f = lambda: [await x for x in y]",
            # Defaults, bases, and genexp iterables run in the enclosing scope.
            "(x for x in [await y for y in z])",
            "(x for x in [y async for y in z])",
            "def f():\n    (x for x in [await y for y in z])",
            "async def f(x=[await y for y in z]): pass",
            "def f(x=[await y for y in z]): pass",
            "class C:\n    def f(self, x=[await y for y in z]): pass",
            "class C([await x for x in y]): pass",
            # Type aliases and type-parameter scopes.
            "type T = [await x for x in y]",
            "type T = [x async for x in y]",
            "def f[T=[await x for x in y]](): pass",
            "def f[T: [await x for x in y]](): pass",
            "async def f[T=[await x for x in y]](): pass",
            "async def f(x: [await y for y in z]): pass",
        ]
        for code in invalid:
            with self.subTest(code=code):
                support.check_syntax_error(self, code, err)

        support.check_syntax_error(
            self, "await x", "'await' outside function")
        support.check_syntax_error(
            self, "class C:\n    await x", "'await' outside function")
        support.check_syntax_error(
            self, "def f():\n    await x", "'await' outside async function")

        flags = ast.PyCF_ALLOW_TOP_LEVEL_AWAIT
        for code in [
            "[await x for x in y]",
            "async def f(x=[await y for y in z]): pass",
            "class C([await x for x in y]): pass",
            "f'{[await x for x in y]}'",
            "(x for x in [await y for y in z])",
            "(x for x in [y async for y in z])",
        ]:
            with self.subTest(code=code, tla=True):
                compile(code, "<test>", "exec", flags=flags)
        still_invalid = [
            "lambda: [await x for x in y]",
            "def f():\n    (x for x in [await y for y in z])",
            "class C:\n    def f(self, x=[await y for y in z]): pass",
            "type T = [await x for x in y]",
            "async def f[T=[await x for x in y]](): pass",
        ]
        for code in still_invalid:
            with self.subTest(code=code, tla=True):
                with self.assertRaisesRegex(SyntaxError, err):
                    compile(code, "<test>", "exec", flags=flags)

    def test_badsyntax_2(self):
        samples = [
            """def foo():
                await = 1
            """,

            """class Bar:
                def async(): pass
            """,

            """class Bar:
                async = 1
            """,

            """class async:
                pass
            """,

            """class await:
                pass
            """,

            """import math as await""",

            """def async():
                pass""",

            """def foo(*, await=1):
                pass"""

            """async = 1""",

            """print(await=1)"""
        ]

        for code in samples:
            with self.subTest(code=code), self.assertRaises(SyntaxError):
                compile(code, "<test>", "exec")

    def test_badsyntax_3(self):
        with self.assertRaises(SyntaxError):
            compile("async = 1", "<test>", "exec")

    def test_badsyntax_4(self):
        samples = [
            '''def foo(await):
                async def foo(): pass
                async def foo():
                    pass
                return await + 1
            ''',

            '''def foo(await):
                async def foo(): pass
                async def foo(): pass
                return await + 1
            ''',

            '''def foo(await):

                async def foo(): pass

                async def foo(): pass

                return await + 1
            ''',

            '''def foo(await):
                """spam"""
                async def foo(): \
                    pass
                # 123
                async def foo(): pass
                # 456
                return await + 1
            ''',

            '''def foo(await):
                def foo(): pass
                def foo(): pass
                async def bar(): return await_
                await_ = await
                try:
                    bar().send(None)
                except StopIteration as ex:
                    return ex.args[0] + 1
            '''
        ]

        for code in samples:
            with self.subTest(code=code), self.assertRaises(SyntaxError):
                compile(code, "<test>", "exec")


class TokenizerRegrTest(unittest.TestCase):

    def test_oneline_defs(self):
        buf = []
        for i in range(500):
            buf.append('def i{i}(): return {i}'.format(i=i))
        buf = '\n'.join(buf)

        # Test that 500 consequent, one-line defs is OK
        ns = {}
        exec(buf, ns, ns)
        self.assertEqual(ns['i499'](), 499)

        # Test that 500 consequent, one-line defs *and*
        # one 'async def' following them is OK
        buf += '\nasync def foo():\n    return'
        ns = {}
        exec(buf, ns, ns)
        self.assertEqual(ns['i499'](), 499)
        self.assertTrue(inspect.iscoroutinefunction(ns['foo']))


class CoroutineTest(unittest.TestCase):

    def test_gen_1(self):
        def gen(): yield
        self.assertNotHasAttr(gen, '__await__')

    def test_func_1(self):
        async def foo():
            return 10

        f = foo()
        self.assertIsInstance(f, types.CoroutineType)
        self.assertTrue(bool(foo.__code__.co_flags & inspect.CO_COROUTINE))
        self.assertFalse(bool(foo.__code__.co_flags & inspect.CO_GENERATOR))
        self.assertTrue(bool(f.cr_code.co_flags & inspect.CO_COROUTINE))
        self.assertFalse(bool(f.cr_code.co_flags & inspect.CO_GENERATOR))
        self.assertEqual(run_async(f), ([], 10))

        self.assertEqual(run_async__await__(foo()), ([], 10))

        def bar(): pass
        self.assertFalse(bool(bar.__code__.co_flags & inspect.CO_COROUTINE))

    def test_func_2(self):
        async def foo():
            raise StopIteration

        with self.assertRaisesRegex(
                RuntimeError, "coroutine raised StopIteration"):

            run_async(foo())

    def test_func_3(self):
        async def foo():
            raise StopIteration

        coro = foo()
        self.assertRegex(repr(coro), '^<coroutine object.* at 0x.*>$')
        coro.close()

    def test_func_4(self):
        async def foo():
            raise StopIteration
        coro = foo()

        check = lambda: self.assertRaisesRegex(
            TypeError, "'coroutine' object is not iterable")

        with check():
            list(coro)

        with check():
            tuple(coro)

        with check():
            sum(coro)

        with check():
            iter(coro)

        with check():
            for i in coro:
                pass

        with check():
            [i for i in coro]

        coro.close()

    def test_func_5(self):
        @types.coroutine
        def bar():
            yield 1

        async def foo():
            await bar()

        check = lambda: self.assertRaisesRegex(
            TypeError, "'coroutine' object is not iterable")

        coro = foo()
        with check():
            for el in coro:
                pass
        coro.close()

        # the following should pass without an error
        for el in bar():
            self.assertEqual(el, 1)
        self.assertEqual([el for el in bar()], [1])
        self.assertEqual(tuple(bar()), (1,))
        self.assertEqual(next(iter(bar())), 1)

    def test_func_6(self):
        @types.coroutine
        def bar():
            yield 1
            yield 2

        async def foo():
            await bar()

        f = foo()
        self.assertEqual(f.send(None), 1)
        self.assertEqual(f.send(None), 2)
        with self.assertRaises(StopIteration):
            f.send(None)

    def test_func_7(self):
        async def bar():
            return 10
        coro = bar()

        def foo():
            yield from coro

        with self.assertRaisesRegex(
                TypeError,
                "cannot 'yield from' a coroutine object in "
                "a non-coroutine generator"):
            list(foo())

        coro.close()

    def test_func_8(self):
        @types.coroutine
        def bar():
            return (yield from coro)

        async def foo():
            return 'spam'

        coro = foo()
        self.assertEqual(run_async(bar()), ([], 'spam'))
        coro.close()

    def test_func_9(self):
        async def foo():
            pass

        with self.assertWarnsRegex(
                RuntimeWarning,
                r"coroutine '.*test_func_9.*foo' was never awaited"):

            foo()
            support.gc_collect()

        with self.assertWarnsRegex(
                RuntimeWarning,
                r"coroutine '.*test_func_9.*foo' was never awaited"):

            with self.assertRaises(TypeError):
                # See bpo-32703.
                for _ in foo():
                    pass

            support.gc_collect()

    def test_func_10(self):
        N = 0

        @types.coroutine
        def gen():
            nonlocal N
            try:
                a = yield
                yield (a ** 2)
            except ZeroDivisionError:
                N += 100
                raise
            finally:
                N += 1

        async def foo():
            await gen()

        coro = foo()
        aw = coro.__await__()
        self.assertIs(aw, iter(aw))
        next(aw)
        self.assertEqual(aw.send(10), 100)

        self.assertEqual(N, 0)
        aw.close()
        self.assertEqual(N, 1)

        coro = foo()
        aw = coro.__await__()
        next(aw)
        with self.assertRaises(ZeroDivisionError):
            aw.throw(ZeroDivisionError())
        self.assertEqual(N, 102)

        coro = foo()
        aw = coro.__await__()
        next(aw)
        with self.assertRaises(ZeroDivisionError):
            with self.assertWarns(DeprecationWarning):
                aw.throw(ZeroDivisionError, ZeroDivisionError(), None)

    def test_func_11(self):
        async def func(): pass
        coro = func()
        # Test that PyCoro_Type and _PyCoroWrapper_Type types were properly
        # initialized
        self.assertIn('__await__', dir(coro))
        self.assertIn('__iter__', dir(coro.__await__()))
        self.assertIn('coroutine_wrapper', repr(coro.__await__()))
        coro.close() # avoid RuntimeWarning

    def test_func_12(self):
        async def g():
            me.send(None)
            await foo
        me = g()
        with self.assertRaisesRegex(ValueError,
                                    "coroutine already executing"):
            me.send(None)

    def test_func_13(self):
        async def g():
            pass

        coro = g()
        with self.assertRaisesRegex(
                TypeError,
                "can't send non-None value to a just-started coroutine"):
            coro.send('spam')

        coro.close()

    def test_func_14(self):
        @types.coroutine
        def gen():
            yield
        async def coro():
            try:
                await gen()
            except GeneratorExit:
                await gen()
        c = coro()
        c.send(None)
        with self.assertRaisesRegex(RuntimeError,
                                    "coroutine ignored GeneratorExit"):
            c.close()

    def test_func_15(self):
        # See http://bugs.python.org/issue25887 for details

        async def spammer():
            return 'spam'
        async def reader(coro):
            return await coro

        spammer_coro = spammer()

        with self.assertRaisesRegex(StopIteration, 'spam'):
            reader(spammer_coro).send(None)

        with self.assertRaisesRegex(RuntimeError,
                                    'cannot reuse already awaited coroutine'):
            reader(spammer_coro).send(None)

    def test_func_16(self):
        # See http://bugs.python.org/issue25887 for details

        @types.coroutine
        def nop():
            yield
        async def send():
            await nop()
            return 'spam'
        async def read(coro):
            await nop()
            return await coro

        spammer = send()

        reader = read(spammer)
        reader.send(None)
        reader.send(None)
        with self.assertRaisesRegex(Exception, 'ham'):
            reader.throw(Exception('ham'))

        reader = read(spammer)
        reader.send(None)
        with self.assertRaisesRegex(RuntimeError,
                                    'cannot reuse already awaited coroutine'):
            reader.send(None)

        with self.assertRaisesRegex(RuntimeError,
                                    'cannot reuse already awaited coroutine'):
            reader.throw(Exception('wat'))

    def test_func_17(self):
        # See http://bugs.python.org/issue25887 for details

        async def coroutine():
            return 'spam'

        coro = coroutine()
        with self.assertRaisesRegex(StopIteration, 'spam'):
            coro.send(None)

        with self.assertRaisesRegex(RuntimeError,
                                    'cannot reuse already awaited coroutine'):
            coro.send(None)

        with self.assertRaisesRegex(RuntimeError,
                                    'cannot reuse already awaited coroutine'):
            coro.throw(Exception('wat'))

        # Closing a coroutine shouldn't raise any exception even if it's
        # already closed/exhausted (similar to generators)
        coro.close()
        coro.close()

    def test_func_18(self):
        # See http://bugs.python.org/issue25887 for details

        async def coroutine():
            return 'spam'

        coro = coroutine()
        await_iter = coro.__await__()
        it = iter(await_iter)

        with self.assertRaisesRegex(StopIteration, 'spam'):
            it.send(None)

        with self.assertRaisesRegex(RuntimeError,
                                    'cannot reuse already awaited coroutine'):
            it.send(None)

        with self.assertRaisesRegex(RuntimeError,
                                    'cannot reuse already awaited coroutine'):
            # Although the iterator protocol requires iterators to
            # raise another StopIteration here, we don't want to do
            # that.  In this particular case, the iterator will raise
            # a RuntimeError, so that 'yield from' and 'await'
            # expressions will trigger the error, instead of silently
            # ignoring the call.
            next(it)

        with self.assertRaisesRegex(RuntimeError,
                                    'cannot reuse already awaited coroutine'):
            it.throw(Exception('wat'))

        with self.assertRaisesRegex(RuntimeError,
                                    'cannot reuse already awaited coroutine'):
            it.throw(Exception('wat'))

        # Closing a coroutine shouldn't raise any exception even if it's
        # already closed/exhausted (similar to generators)
        it.close()
        it.close()

    def test_func_19(self):
        CHK = 0

        @types.coroutine
        def foo():
            nonlocal CHK
            yield
            try:
                yield
            except GeneratorExit:
                CHK += 1

        async def coroutine():
            await foo()

        coro = coroutine()

        coro.send(None)
        coro.send(None)

        self.assertEqual(CHK, 0)
        coro.close()
        self.assertEqual(CHK, 1)

        for _ in range(3):
            # Closing a coroutine shouldn't raise any exception even if it's
            # already closed/exhausted (similar to generators)
            coro.close()
            self.assertEqual(CHK, 1)

    def test_coro_wrapper_send_tuple(self):
        async def foo():
            return (10,)

        result = run_async__await__(foo())
        self.assertEqual(result, ([], (10,)))

    def test_coro_wrapper_send_stop_iterator(self):
        async def foo():
            return StopIteration(10)

        result = run_async__await__(foo())
        self.assertIsInstance(result[1], StopIteration)
        self.assertEqual(result[1].value, 10)

    def test_cr_await(self):
        @types.coroutine
        def a():
            self.assertEqual(inspect.getcoroutinestate(coro_b), inspect.CORO_RUNNING)
            self.assertIsNone(coro_b.cr_await)
            yield
            self.assertEqual(inspect.getcoroutinestate(coro_b), inspect.CORO_RUNNING)
            self.assertIsNone(coro_b.cr_await)

        async def c():
            await a()

        async def b():
            self.assertIsNone(coro_b.cr_await)
            await c()
            self.assertIsNone(coro_b.cr_await)

        coro_b = b()
        self.assertEqual(inspect.getcoroutinestate(coro_b), inspect.CORO_CREATED)
        self.assertIsNone(coro_b.cr_await)

        coro_b.send(None)
        self.assertEqual(inspect.getcoroutinestate(coro_b), inspect.CORO_SUSPENDED)
        self.assertEqual(coro_b.cr_await.cr_await.gi_code.co_name, 'a')

        with self.assertRaises(StopIteration):
            coro_b.send(None)  # complete coroutine
        self.assertEqual(inspect.getcoroutinestate(coro_b), inspect.CORO_CLOSED)
        self.assertIsNone(coro_b.cr_await)

    def test_corotype_1(self):
        ct = types.CoroutineType
        if not support.MISSING_C_DOCSTRINGS:
            self.assertIn('into coroutine', ct.send.__doc__)
            self.assertIn('inside coroutine', ct.close.__doc__)
            self.assertIn('in coroutine', ct.throw.__doc__)
            self.assertIn('of the coroutine', ct.__dict__['__name__'].__doc__)
            self.assertIn('of the coroutine', ct.__dict__['__qualname__'].__doc__)
        self.assertEqual(ct.__name__, 'coroutine')

        async def f(): pass
        c = f()
        self.assertIn('coroutine object', repr(c))
        c.close()

    def test_await_1(self):

        async def foo():
            await 1
        with self.assertRaisesRegex(TypeError, "'int' object can.t be awaited"):
            run_async(foo())

    def test_await_2(self):
        async def foo():
            await []
        with self.assertRaisesRegex(TypeError, "'list' object can.t be awaited"):
            run_async(foo())

    def test_await_3(self):
        async def foo():
            await AsyncYieldFrom([1, 2, 3])

        self.assertEqual(run_async(foo()), ([1, 2, 3], None))
        self.assertEqual(run_async__await__(foo()), ([1, 2, 3], None))

    def test_await_4(self):
        async def bar():
            return 42

        async def foo():
            return await bar()

        self.assertEqual(run_async(foo()), ([], 42))

    def test_await_5(self):
        class Awaitable:
            def __await__(self):
                return

        async def foo():
            return (await Awaitable())

        with self.assertRaisesRegex(
            TypeError, "__await__.*must return an iterator, not"):

            run_async(foo())

    def test_await_6(self):
        class Awaitable:
            def __await__(self):
                return iter([52])

        async def foo():
            return (await Awaitable())

        self.assertEqual(run_async(foo()), ([52], None))

    def test_await_7(self):
        class Awaitable:
            def __await__(self):
                yield 42
                return 100

        async def foo():
            return (await Awaitable())

        self.assertEqual(run_async(foo()), ([42], 100))

    def test_await_8(self):
        class Awaitable:
            pass

        async def foo(): return await Awaitable()

        with self.assertRaisesRegex(
            TypeError, "'Awaitable' object can't be awaited"):

            run_async(foo())

    def test_await_9(self):
        def wrap():
            return bar

        async def bar():
            return 42

        async def foo():
            db = {'b':  lambda: wrap}

            class DB:
                b = wrap

            return (await bar() + await wrap()() + await db['b']()()() +
                    await bar() * 1000 + await DB.b()())

        async def foo2():
            return -await bar()

        self.assertEqual(run_async(foo()), ([], 42168))
        self.assertEqual(run_async(foo2()), ([], -42))

    def test_await_10(self):
        async def baz():
            return 42

        async def bar():
            return baz()

        async def foo():
            return await (await bar())

        self.assertEqual(run_async(foo()), ([], 42))

    def test_await_11(self):
        def ident(val):
            return val

        async def bar():
            return 'spam'

        async def foo():
            return ident(val=await bar())

        async def foo2():
            return await bar(), 'ham'

        self.assertEqual(run_async(foo2()), ([], ('spam', 'ham')))

    def test_await_12(self):
        async def coro():
            return 'spam'
        c = coro()

        class Awaitable:
            def __await__(self):
                return c

        async def foo():
            return await Awaitable()

        with self.assertRaisesRegex(
                TypeError, r"__await__\(\) must return an iterator, not coroutine"):
            run_async(foo())

        c.close()

    def test_await_13(self):
        class Awaitable:
            def __await__(self):
                return self

        async def foo():
            return await Awaitable()

        with self.assertRaisesRegex(
            TypeError, "__await__.*must return an iterator, not"):

            run_async(foo())

    def test_await_14(self):
        class Wrapper:
            # Forces the interpreter to use CoroutineType.__await__
            def __init__(self, coro):
                assert coro.__class__ is types.CoroutineType
                self.coro = coro
            def __await__(self):
                return self.coro.__await__()

        class FutureLike:
            def __await__(self):
                return (yield)

        class Marker(Exception):
            pass

        async def coro1():
            try:
                return await FutureLike()
            except ZeroDivisionError:
                raise Marker
        async def coro2():
            return await Wrapper(coro1())

        c = coro2()
        c.send(None)
        with self.assertRaisesRegex(StopIteration, 'spam'):
            c.send('spam')

        c = coro2()
        c.send(None)
        with self.assertRaises(Marker):
            c.throw(ZeroDivisionError)

    def test_await_15(self):
        @types.coroutine
        def nop():
            yield

        async def coroutine():
            await nop()

        async def waiter(coro):
            await coro

        coro = coroutine()
        coro.send(None)

        with self.assertRaisesRegex(RuntimeError,
                                    "coroutine is being awaited already"):
            waiter(coro).send(None)

    def test_await_16(self):
        # See https://bugs.python.org/issue29600 for details.

        async def f():
            return ValueError()

        async def g():
            try:
                raise KeyError
            except KeyError:
                return await f()

        _, result = run_async(g())
        self.assertIsNone(result.__context__)

    def test_await_17(self):
        # See https://github.com/python/cpython/issues/131666 for details.
        class A:
            async def __anext__(self):
                raise StopAsyncIteration
            def __aiter__(self):
                return self

        with warnings.catch_warnings():
            # Closing the unstarted anext() coroutine leaves the inner
            # __anext__() coroutine never awaited.
            warnings.simplefilter("ignore", RuntimeWarning)
            with contextlib.closing(anext(A(), "a").__await__()) as anext_awaitable:
                self.assertRaises(TypeError, anext_awaitable.close, 1)

    def test_with_1(self):
        class Manager:
            def __init__(self, name):
                self.name = name

            async def __aenter__(self):
                await AsyncYieldFrom(['enter-1-' + self.name,
                                      'enter-2-' + self.name])
                return self

            async def __aexit__(self, *args):
                await AsyncYieldFrom(['exit-1-' + self.name,
                                      'exit-2-' + self.name])

                if self.name == 'B':
                    return True


        async def foo():
            async with Manager("A") as a, Manager("B") as b:
                await AsyncYieldFrom([('managers', a.name, b.name)])
                1/0

        f = foo()
        result, _ = run_async(f)

        self.assertEqual(
            result, ['enter-1-A', 'enter-2-A', 'enter-1-B', 'enter-2-B',
                     ('managers', 'A', 'B'),
                     'exit-1-B', 'exit-2-B', 'exit-1-A', 'exit-2-A']
        )

        async def foo():
            async with Manager("A") as a, Manager("C") as c:
                await AsyncYieldFrom([('managers', a.name, c.name)])
                1/0

        with self.assertRaises(ZeroDivisionError):
            run_async(foo())

    def test_with_2(self):
        class CM:
            def __aenter__(self):
                pass

        body_executed = None
        async def foo():
            nonlocal body_executed
            body_executed = False
            async with CM():
                body_executed = True

        with self.assertRaisesRegex(TypeError, 'asynchronous context manager.*__aexit__'):
            run_async(foo())
        self.assertIs(body_executed, False)

    def test_with_3(self):
        class CM:
            def __aexit__(self):
                pass

        body_executed = None
        async def foo():
            nonlocal body_executed
            body_executed = False
            async with CM():
                body_executed = True

        with self.assertRaisesRegex(TypeError, 'asynchronous context manager'):
            run_async(foo())
        self.assertIs(body_executed, False)

    def test_with_4(self):
        class CM:
            pass

        body_executed = None
        async def foo():
            nonlocal body_executed
            body_executed = False
            async with CM():
                body_executed = True

        with self.assertRaisesRegex(TypeError, 'asynchronous context manager'):
            run_async(foo())
        self.assertIs(body_executed, False)

    def test_with_5(self):
        # While this test doesn't make a lot of sense,
        # it's a regression test for an early bug with opcodes
        # generation

        class CM:
            async def __aenter__(self):
                return self

            async def __aexit__(self, *exc):
                pass

        async def func():
            async with CM():
                self.assertEqual((1, ), 1)

        with self.assertRaises(AssertionError):
            run_async(func())

    def test_with_6(self):
        class CM:
            def __aenter__(self):
                return 123

            def __aexit__(self, *e):
                return 456

        async def foo():
            async with CM():
                pass

        with self.assertRaisesRegex(
                TypeError,
                "'async with' received an object from __aenter__ "
                "that does not implement __await__: int"):
            # it's important that __aexit__ wasn't called
            run_async(foo())

    def test_with_7(self):
        class CM:
            async def __aenter__(self):
                return self

            def __aexit__(self, *e):
                return 444

        # Exit with exception
        async def foo():
            async with CM():
                1/0

        try:
            run_async(foo())
        except TypeError as exc:
            self.assertRegex(
                exc.args[0],
                "'async with' received an object from __aexit__ "
                "that does not implement __await__: int")
            self.assertTrue(exc.__context__ is not None)
            self.assertTrue(isinstance(exc.__context__, ZeroDivisionError))
        else:
            self.fail('invalid asynchronous context manager did not fail')


    def test_with_8(self):
        CNT = 0

        class CM:
            async def __aenter__(self):
                return self

            def __aexit__(self, *e):
                return 456

        # Normal exit
        async def foo():
            nonlocal CNT
            async with CM():
                CNT += 1
        with self.assertRaisesRegex(
                TypeError,
                "'async with' received an object from __aexit__ "
                "that does not implement __await__: int"):
            run_async(foo())
        self.assertEqual(CNT, 1)

        # Exit with 'break'
        async def foo():
            nonlocal CNT
            for i in range(2):
                async with CM():
                    CNT += 1
                    break
        with self.assertRaisesRegex(
                TypeError,
                "'async with' received an object from __aexit__ "
                "that does not implement __await__: int"):
            run_async(foo())
        self.assertEqual(CNT, 2)

        # Exit with 'continue'
        async def foo():
            nonlocal CNT
            for i in range(2):
                async with CM():
                    CNT += 1
                    continue
        with self.assertRaisesRegex(
                TypeError,
                "'async with' received an object from __aexit__ "
                "that does not implement __await__: int"):
            run_async(foo())
        self.assertEqual(CNT, 3)

        # Exit with 'return'
        async def foo():
            nonlocal CNT
            async with CM():
                CNT += 1
                return
        with self.assertRaisesRegex(
                TypeError,
                "'async with' received an object from __aexit__ "
                "that does not implement __await__: int"):
            run_async(foo())
        self.assertEqual(CNT, 4)


    def test_with_9(self):
        CNT = 0

        class CM:
            async def __aenter__(self):
                return self

            async def __aexit__(self, *e):
                1/0

        async def foo():
            nonlocal CNT
            async with CM():
                CNT += 1

        with self.assertRaises(ZeroDivisionError):
            run_async(foo())

        self.assertEqual(CNT, 1)

    def test_with_10(self):
        CNT = 0

        class CM:
            async def __aenter__(self):
                return self

            async def __aexit__(self, *e):
                1/0

        async def foo():
            nonlocal CNT
            async with CM():
                async with CM():
                    raise RuntimeError

        try:
            run_async(foo())
        except ZeroDivisionError as exc:
            self.assertTrue(exc.__context__ is not None)
            self.assertTrue(isinstance(exc.__context__, ZeroDivisionError))
            self.assertTrue(isinstance(exc.__context__.__context__,
                                       RuntimeError))
        else:
            self.fail('exception from __aexit__ did not propagate')

    def test_with_11(self):
        CNT = 0

        class CM:
            async def __aenter__(self):
                raise NotImplementedError

            async def __aexit__(self, *e):
                1/0

        async def foo():
            nonlocal CNT
            async with CM():
                raise RuntimeError

        try:
            run_async(foo())
        except NotImplementedError as exc:
            self.assertTrue(exc.__context__ is None)
        else:
            self.fail('exception from __aenter__ did not propagate')

    def test_with_12(self):
        CNT = 0

        class CM:
            async def __aenter__(self):
                return self

            async def __aexit__(self, *e):
                return True

        async def foo():
            nonlocal CNT
            async with CM() as cm:
                self.assertIs(cm.__class__, CM)
                raise RuntimeError

        run_async(foo())

    def test_with_13(self):
        CNT = 0

        class CM:
            async def __aenter__(self):
                1/0

            async def __aexit__(self, *e):
                return True

        async def foo():
            nonlocal CNT
            CNT += 1
            async with CM():
                CNT += 1000
            CNT += 10000

        with self.assertRaises(ZeroDivisionError):
            run_async(foo())
        self.assertEqual(CNT, 1)

    def test_for_1(self):
        aiter_calls = 0

        class AsyncIter:
            def __init__(self):
                self.i = 0

            def __aiter__(self):
                nonlocal aiter_calls
                aiter_calls += 1
                return self

            async def __anext__(self):
                self.i += 1

                if not (self.i % 10):
                    await AsyncYield(self.i * 10)

                if self.i > 100:
                    raise StopAsyncIteration

                return self.i, self.i


        buffer = []
        async def test1():
            async for i1, i2 in AsyncIter():
                buffer.append(i1 + i2)

        yielded, _ = run_async(test1())
        # Make sure that __aiter__ was called only once
        self.assertEqual(aiter_calls, 1)
        self.assertEqual(yielded, [i * 100 for i in range(1, 11)])
        self.assertEqual(buffer, [i*2 for i in range(1, 101)])


        buffer = []
        async def test2():
            nonlocal buffer
            async for i in AsyncIter():
                buffer.append(i[0])
                if i[0] == 20:
                    break
            else:
                buffer.append('what?')
            buffer.append('end')

        yielded, _ = run_async(test2())
        # Make sure that __aiter__ was called only once
        self.assertEqual(aiter_calls, 2)
        self.assertEqual(yielded, [100, 200])
        self.assertEqual(buffer, [i for i in range(1, 21)] + ['end'])


        buffer = []
        async def test3():
            nonlocal buffer
            async for i in AsyncIter():
                if i[0] > 20:
                    continue
                buffer.append(i[0])
            else:
                buffer.append('what?')
            buffer.append('end')

        yielded, _ = run_async(test3())
        # Make sure that __aiter__ was called only once
        self.assertEqual(aiter_calls, 3)
        self.assertEqual(yielded, [i * 100 for i in range(1, 11)])
        self.assertEqual(buffer, [i for i in range(1, 21)] +
                                 ['what?', 'end'])

    def test_for_2(self):
        tup = (1, 2, 3)
        refs_before = sys.getrefcount(tup)

        async def foo():
            async for i in tup:
                print('never going to happen')

        with self.assertRaisesRegex(
                TypeError, "async for' requires an object.*__aiter__.*tuple"):

            run_async(foo())

        self.assertEqual(sys.getrefcount(tup), refs_before)

    def test_for_3(self):
        class I:
            def __aiter__(self):
                return self

        aiter = I()
        refs_before = sys.getrefcount(aiter)

        async def foo():
            async for i in aiter:
                print('never going to happen')

        with self.assertRaisesRegex(
                TypeError,
                r"that does not implement __anext__"):

            run_async(foo())

        self.assertEqual(sys.getrefcount(aiter), refs_before)

    def test_for_4(self):
        class I:
            def __aiter__(self):
                return self

            def __anext__(self):
                return ()

        aiter = I()
        refs_before = sys.getrefcount(aiter)

        async def foo():
            async for i in aiter:
                print('never going to happen')

        with self.assertRaisesRegex(
                TypeError,
                "async for' received an invalid object.*__anext__.*tuple"):

            run_async(foo())

        self.assertEqual(sys.getrefcount(aiter), refs_before)

    def test_for_6(self):
        I = 0

        class Manager:
            async def __aenter__(self):
                nonlocal I
                I += 10000

            async def __aexit__(self, *args):
                nonlocal I
                I += 100000

        class Iterable:
            def __init__(self):
                self.i = 0

            def __aiter__(self):
                return self

            async def __anext__(self):
                if self.i > 10:
                    raise StopAsyncIteration
                self.i += 1
                return self.i

        ##############

        manager = Manager()
        iterable = Iterable()
        mrefs_before = sys.getrefcount(manager)
        irefs_before = sys.getrefcount(iterable)

        async def main():
            nonlocal I

            async with manager:
                async for i in iterable:
                    I += 1
            I += 1000

        with warnings.catch_warnings():
            warnings.simplefilter("error")
            # Test that __aiter__ that returns an asynchronous iterator
            # directly does not throw any warnings.
            run_async(main())
        self.assertEqual(I, 111011)

        self.assertEqual(sys.getrefcount(manager), mrefs_before)
        self.assertEqual(sys.getrefcount(iterable), irefs_before)

        ##############

        async def main():
            nonlocal I

            async with Manager():
                async for i in Iterable():
                    I += 1
            I += 1000

            async with Manager():
                async for i in Iterable():
                    I += 1
            I += 1000

        run_async(main())
        self.assertEqual(I, 333033)

        ##############

        async def main():
            nonlocal I

            async with Manager():
                I += 100
                async for i in Iterable():
                    I += 1
                else:
                    I += 10000000
            I += 1000

            async with Manager():
                I += 100
                async for i in Iterable():
                    I += 1
                else:
                    I += 10000000
            I += 1000

        run_async(main())
        self.assertEqual(I, 20555255)

    def test_for_7(self):
        CNT = 0
        class AI:
            def __aiter__(self):
                1/0
        async def foo():
            nonlocal CNT
            async for i in AI():
                CNT += 1
            CNT += 10
        with self.assertRaises(ZeroDivisionError):
            run_async(foo())
        self.assertEqual(CNT, 0)

    def test_for_8(self):
        CNT = 0
        class AI:
            def __aiter__(self):
                1/0
        async def foo():
            nonlocal CNT
            async for i in AI():
                CNT += 1
            CNT += 10
        with self.assertRaises(ZeroDivisionError):
            with warnings.catch_warnings():
                warnings.simplefilter("error")
                # Test that if __aiter__ raises an exception it propagates
                # without any kind of warning.
                run_async(foo())
        self.assertEqual(CNT, 0)

    def test_for_11(self):
        class F:
            def __aiter__(self):
                return self
            def __anext__(self):
                return self
            def __await__(self):
                1 / 0

        async def main():
            async for _ in F():
                pass

        with self.assertRaisesRegex(TypeError,
                                    'an invalid object from __anext__') as c:
            main().send(None)

        err = c.exception
        self.assertIsInstance(err.__cause__, ZeroDivisionError)

    def test_for_tuple(self):
        class Done(Exception): pass

        class AIter(tuple):
            i = 0
            def __aiter__(self):
                return self
            async def __anext__(self):
                if self.i >= len(self):
                    raise StopAsyncIteration
                self.i += 1
                return self[self.i - 1]

        result = []
        async def foo():
            async for i in AIter([42]):
                result.append(i)
            raise Done

        with self.assertRaises(Done):
            foo().send(None)
        self.assertEqual(result, [42])

    def test_for_stop_iteration(self):
        class Done(Exception): pass

        class AIter(StopIteration):
            i = 0
            def __aiter__(self):
                return self
            async def __anext__(self):
                if self.i:
                    raise StopAsyncIteration
                self.i += 1
                return self.value

        result = []
        async def foo():
            async for i in AIter(42):
                result.append(i)
            raise Done

        with self.assertRaises(Done):
            foo().send(None)
        self.assertEqual(result, [42])

    def test_comp_1(self):
        async def f(i):
            return i

        async def run_list():
            return [await c for c in [f(1), f(41)]]

        async def run_set():
            return {await c for c in [f(1), f(41)]}

        async def run_dict1():
            return {await c: 'a' for c in [f(1), f(41)]}

        async def run_dict2():
            return {i: await c for i, c in enumerate([f(1), f(41)])}

        self.assertEqual(run_async(run_list()), ([], [1, 41]))
        self.assertEqual(run_async(run_set()), ([], {1, 41}))
        self.assertEqual(run_async(run_dict1()), ([], {1: 'a', 41: 'a'}))
        self.assertEqual(run_async(run_dict2()), ([], {0: 1, 1: 41}))

    def test_comp_2(self):
        async def f(i):
            return i

        async def run_list():
            return [s for c in [f(''), f('abc'), f(''), f(['de', 'fg'])]
                    for s in await c]

        self.assertEqual(
            run_async(run_list()),
            ([], ['a', 'b', 'c', 'de', 'fg']))

        async def run_set():
            return {d
                    for c in [f([f([10, 30]),
                                 f([20])])]
                    for s in await c
                    for d in await s}

        self.assertEqual(
            run_async(run_set()),
            ([], {10, 20, 30}))

        async def run_set2():
            return {await s
                    for c in [f([f(10), f(20)])]
                    for s in await c}

        self.assertEqual(
            run_async(run_set2()),
            ([], {10, 20}))

    def test_comp_3(self):
        async def f(it):
            for i in it:
                yield i

        async def run_list():
            return [i + 1 async for i in f([10, 20])]
        self.assertEqual(
            run_async(run_list()),
            ([], [11, 21]))

        async def run_set():
            return {i + 1 async for i in f([10, 20])}
        self.assertEqual(
            run_async(run_set()),
            ([], {11, 21}))

        async def run_dict():
            return {i + 1: i + 2 async for i in f([10, 20])}
        self.assertEqual(
            run_async(run_dict()),
            ([], {11: 12, 21: 22}))

        async def run_gen():
            gen = (i + 1 async for i in f([10, 20]))
            return [g + 100 async for g in gen]
        self.assertEqual(
            run_async(run_gen()),
            ([], [111, 121]))

    def test_comp_4(self):
        async def f(it):
            for i in it:
                yield i

        async def run_list():
            return [i + 1 async for i in f([10, 20]) if i > 10]
        self.assertEqual(
            run_async(run_list()),
            ([], [21]))

        async def run_set():
            return {i + 1 async for i in f([10, 20]) if i > 10}
        self.assertEqual(
            run_async(run_set()),
            ([], {21}))

        async def run_dict():
            return {i + 1: i + 2 async for i in f([10, 20]) if i > 10}
        self.assertEqual(
            run_async(run_dict()),
            ([], {21: 22}))

        async def run_gen():
            gen = (i + 1 async for i in f([10, 20]) if i > 10)
            return [g + 100 async for g in gen]
        self.assertEqual(
            run_async(run_gen()),
            ([], [121]))

    def test_comp_4_2(self):
        async def f(it):
            for i in it:
                yield i

        async def run_list():
            return [i + 10 async for i in f(range(5)) if 0 < i < 4]
        self.assertEqual(
            run_async(run_list()),
            ([], [11, 12, 13]))

        async def run_set():
            return {i + 10 async for i in f(range(5)) if 0 < i < 4}
        self.assertEqual(
            run_async(run_set()),
            ([], {11, 12, 13}))

        async def run_dict():
            return {i + 10: i + 100 async for i in f(range(5)) if 0 < i < 4}
        self.assertEqual(
            run_async(run_dict()),
            ([], {11: 101, 12: 102, 13: 103}))

        async def run_gen():
            gen = (i + 10 async for i in f(range(5)) if 0 < i < 4)
            return [g + 100 async for g in gen]
        self.assertEqual(
            run_async(run_gen()),
            ([], [111, 112, 113]))

    def test_comp_5(self):
        async def f(it):
            for i in it:
                yield i

        async def run_list():
            return [i + 1 for pair in ([10, 20], [30, 40]) if pair[0] > 10
                    async for i in f(pair) if i > 30]
        self.assertEqual(
            run_async(run_list()),
            ([], [41]))

    def test_comp_6(self):
        async def f(it):
            for i in it:
                yield i

        async def run_list():
            return [i + 1 async for seq in f([(10, 20), (30,)])
                    for i in seq]

        self.assertEqual(
            run_async(run_list()),
            ([], [11, 21, 31]))

    def test_comp_7(self):
        async def f():
            yield 1
            yield 2
            raise Exception('aaa')

        async def run_list():
            return [i async for i in f()]

        with self.assertRaisesRegex(Exception, 'aaa'):
            run_async(run_list())

    def test_comp_8(self):
        async def f():
            return [i for i in [1, 2, 3]]

        self.assertEqual(
            run_async(f()),
            ([], [1, 2, 3]))

    def test_comp_9(self):
        async def gen():
            yield 1
            yield 2
        async def f():
            l = [i async for i in gen()]
            return [i for i in l]

        self.assertEqual(
            run_async(f()),
            ([], [1, 2]))

    def test_comp_10(self):
        async def f():
            xx = {i for i in [1, 2, 3]}
            return {x: x for x in xx}

        self.assertEqual(
            run_async(f()),
            ([], {1: 1, 2: 2, 3: 3}))

    def test_nested_comp(self):
        async def run_list_inside_list():
            return [[i + j async for i in asynciter([1, 2])] for j in [10, 20]]
        self.assertEqual(
            run_async(run_list_inside_list()),
            ([], [[11, 12], [21, 22]]))

        async def run_set_inside_list():
            return [{i + j async for i in asynciter([1, 2])} for j in [10, 20]]
        self.assertEqual(
            run_async(run_set_inside_list()),
            ([], [{11, 12}, {21, 22}]))

        async def run_list_inside_set():
            return {sum([i async for i in asynciter(range(j))]) for j in [3, 5]}
        self.assertEqual(
            run_async(run_list_inside_set()),
            ([], {3, 10}))

        async def run_dict_inside_dict():
            return {j: {i: i + j async for i in asynciter([1, 2])} for j in [10, 20]}
        self.assertEqual(
            run_async(run_dict_inside_dict()),
            ([], {10: {1: 11, 2: 12}, 20: {1: 21, 2: 22}}))

        async def run_list_inside_gen():
            gen = ([i + j async for i in asynciter([1, 2])] for j in [10, 20])
            return [x async for x in gen]
        self.assertEqual(
            run_async(run_list_inside_gen()),
            ([], [[11, 12], [21, 22]]))

        async def run_gen_inside_list():
            gens = [(i async for i in asynciter(range(j))) for j in [3, 5]]
            return [x for g in gens async for x in g]
        self.assertEqual(
            run_async(run_gen_inside_list()),
            ([], [0, 1, 2, 0, 1, 2, 3, 4]))

        async def run_gen_inside_gen():
            gens = ((i async for i in asynciter(range(j))) for j in [3, 5])
            return [x for g in gens async for x in g]
        self.assertEqual(
            run_async(run_gen_inside_gen()),
            ([], [0, 1, 2, 0, 1, 2, 3, 4]))

        async def run_list_inside_list_inside_list():
            return [[[i + j + k async for i in asynciter([1, 2])]
                     for j in [10, 20]]
                    for k in [100, 200]]
        self.assertEqual(
            run_async(run_list_inside_list_inside_list()),
            ([], [[[111, 112], [121, 122]], [[211, 212], [221, 222]]]))

    def test_copy(self):
        async def func(): pass
        coro = func()
        with self.assertRaises(TypeError):
            copy.copy(coro)

        aw = coro.__await__()
        try:
            with self.assertRaises(TypeError):
                copy.copy(aw)
        finally:
            aw.close()

    def test_pickle(self):
        async def func(): pass
        coro = func()
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.assertRaises((TypeError, pickle.PicklingError)):
                pickle.dumps(coro, proto)

        aw = coro.__await__()
        try:
            for proto in range(pickle.HIGHEST_PROTOCOL + 1):
                with self.assertRaises((TypeError, pickle.PicklingError)):
                    pickle.dumps(aw, proto)
        finally:
            aw.close()

    def test_fatal_coro_warning(self):
        # Issue 27811
        async def func(): pass
        with warnings.catch_warnings(), \
             support.catch_unraisable_exception() as cm:
            warnings.filterwarnings("error")
            coro = func()
            # only store repr() to avoid keeping the coroutine alive
            coro_repr = repr(coro)
            coro = None
            support.gc_collect()

            self.assertEqual(cm.unraisable.err_msg,
                             f"Exception ignored while finalizing "
                             f"coroutine {coro_repr}")
            self.assertIn("was never awaited", str(cm.unraisable.exc_value))

    def test_for_assign_raising_stop_async_iteration(self):
        class BadTarget:
            def __setitem__(self, key, value):
                raise StopAsyncIteration(42)
        tgt = BadTarget()
        async def source():
            yield 10

        async def run_for():
            with self.assertRaises(StopAsyncIteration) as cm:
                async for tgt[0] in source():
                    pass
            self.assertEqual(cm.exception.args, (42,))
            return 'end'
        self.assertEqual(run_async(run_for()), ([], 'end'))

        async def run_list():
            with self.assertRaises(StopAsyncIteration) as cm:
                return [0 async for tgt[0] in source()]
            self.assertEqual(cm.exception.args, (42,))
            return 'end'
        self.assertEqual(run_async(run_list()), ([], 'end'))

        async def run_gen():
            gen = (0 async for tgt[0] in source())
            a = gen.asend(None)
            with self.assertRaises(RuntimeError) as cm:
                await a
            self.assertIsInstance(cm.exception.__cause__, StopAsyncIteration)
            self.assertEqual(cm.exception.__cause__.args, (42,))
            return 'end'
        self.assertEqual(run_async(run_gen()), ([], 'end'))

    def test_for_assign_raising_stop_async_iteration_2(self):
        class BadIterable:
            def __iter__(self):
                raise StopAsyncIteration(42)
        async def badpairs():
            yield BadIterable()

        async def run_for():
            with self.assertRaises(StopAsyncIteration) as cm:
                async for i, j in badpairs():
                    pass
            self.assertEqual(cm.exception.args, (42,))
            return 'end'
        self.assertEqual(run_async(run_for()), ([], 'end'))

        async def run_list():
            with self.assertRaises(StopAsyncIteration) as cm:
                return [0 async for i, j in badpairs()]
            self.assertEqual(cm.exception.args, (42,))
            return 'end'
        self.assertEqual(run_async(run_list()), ([], 'end'))

        async def run_gen():
            gen = (0 async for i, j in badpairs())
            a = gen.asend(None)
            with self.assertRaises(RuntimeError) as cm:
                await a
            self.assertIsInstance(cm.exception.__cause__, StopAsyncIteration)
            self.assertEqual(cm.exception.__cause__.args, (42,))
            return 'end'
        self.assertEqual(run_async(run_gen()), ([], 'end'))

    def test_bpo_45813_1(self):
        'This would crash the interpreter in 3.11a2'
        async def f():
            pass
        with self.assertWarns(RuntimeWarning):
            frame = f().cr_frame
        frame.clear()

    def test_bpo_45813_2(self):
        'This would crash the interpreter in 3.11a2'
        async def f():
            pass
        gen = f()
        with self.assertWarns(RuntimeWarning):
            gen.cr_frame.clear()
        gen.close()

    def test_cr_frame_after_close(self):
        async def f():
            pass
        gen = f()
        self.assertIsNotNone(gen.cr_frame)
        gen.close()
        self.assertIsNone(gen.cr_frame)

    def test_stack_in_coroutine_throw(self):
        # Regression test for https://github.com/python/cpython/issues/93592
        async def a():
            return await b()

        async def b():
            return await c()

        @types.coroutine
        def c():
            try:
                # traceback.print_stack()
                yield len(traceback.extract_stack())
            except ZeroDivisionError:
                # traceback.print_stack()
                yield len(traceback.extract_stack())

        coro = a()
        len_send = coro.send(None)
        len_throw = coro.throw(ZeroDivisionError)
        # before fixing, visible stack from throw would be shorter than from send.
        self.assertEqual(len_send, len_throw)

    def test_call_generator_in_frame_clear(self):
        # gh-143939: Running a generator while clearing the coroutine's frame
        # should not be misinterpreted as a yield.
        class CallGeneratorOnDealloc:
            def __del__(self):
                next(x for x in [1])

        async def coro():
            obj = CallGeneratorOnDealloc()
            return 42

        yielded, result = run_async(coro())
        self.assertEqual(yielded, [])
        self.assertEqual(result, 42)

@unittest.skipIf(
    support.is_emscripten or support.is_wasi,
    "asyncio does not work under Emscripten/WASI yet."
)
class CoroAsyncIOCompatTest(unittest.TestCase):

    def test_asyncio_1(self):
        # asyncio cannot be imported when Python is compiled without thread
        # support
        asyncio = import_helper.import_module('asyncio')

        class MyException(Exception):
            pass

        buffer = []

        class CM:
            async def __aenter__(self):
                buffer.append(1)
                await asyncio.sleep(0.01)
                buffer.append(2)
                return self

            async def __aexit__(self, exc_type, exc_val, exc_tb):
                await asyncio.sleep(0.01)
                buffer.append(exc_type.__name__)

        async def f():
            async with CM():
                await asyncio.sleep(0.01)
                raise MyException
            buffer.append('unreachable')

        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        try:
            loop.run_until_complete(f())
        except MyException:
            pass
        finally:
            loop.close()
            asyncio.set_event_loop(None)

        self.assertEqual(buffer, [1, 2, 'MyException'])


class OriginTrackingTest(unittest.TestCase):
    def here(self):
        info = inspect.getframeinfo(inspect.currentframe().f_back)
        return (info.filename, info.lineno)

    def test_origin_tracking(self):
        orig_depth = sys.get_coroutine_origin_tracking_depth()
        try:
            async def corofn():
                pass

            sys.set_coroutine_origin_tracking_depth(0)
            self.assertEqual(sys.get_coroutine_origin_tracking_depth(), 0)

            with contextlib.closing(corofn()) as coro:
                self.assertIsNone(coro.cr_origin)

            sys.set_coroutine_origin_tracking_depth(1)
            self.assertEqual(sys.get_coroutine_origin_tracking_depth(), 1)

            fname, lineno = self.here()
            with contextlib.closing(corofn()) as coro:
                self.assertEqual(coro.cr_origin,
                                 ((fname, lineno + 1, "test_origin_tracking"),))

            sys.set_coroutine_origin_tracking_depth(2)
            self.assertEqual(sys.get_coroutine_origin_tracking_depth(), 2)

            def nested():
                return (self.here(), corofn())
            fname, lineno = self.here()
            ((nested_fname, nested_lineno), coro) = nested()
            with contextlib.closing(coro):
                self.assertEqual(coro.cr_origin,
                                 ((nested_fname, nested_lineno, "nested"),
                                  (fname, lineno + 1, "test_origin_tracking")))

            # Check we handle running out of frames correctly
            sys.set_coroutine_origin_tracking_depth(1000)
            with contextlib.closing(corofn()) as coro:
                self.assertTrue(2 < len(coro.cr_origin) < 1000)

            # We can't set depth negative
            with self.assertRaises(ValueError):
                sys.set_coroutine_origin_tracking_depth(-1)
            # And trying leaves it unchanged
            self.assertEqual(sys.get_coroutine_origin_tracking_depth(), 1000)

        finally:
            sys.set_coroutine_origin_tracking_depth(orig_depth)

    def test_origin_tracking_warning(self):
        async def corofn():
            pass

        a1_filename, a1_lineno = self.here()
        def a1():
            return corofn()  # comment in a1
        a1_lineno += 2

        a2_filename, a2_lineno = self.here()
        def a2():
            return a1()  # comment in a2
        a2_lineno += 2

        def check(depth, msg):
            sys.set_coroutine_origin_tracking_depth(depth)
            with self.assertWarns(RuntimeWarning) as cm:
                a2()
                support.gc_collect()
            self.assertEqual(msg, str(cm.warning))

        orig_depth = sys.get_coroutine_origin_tracking_depth()
        try:
            check(0, f"coroutine '{corofn.__qualname__}' was never awaited")
            check(1, "".join([
                f"coroutine '{corofn.__qualname__}' was never awaited\n",
                "Coroutine created at (most recent call last)\n",
                f'  File "{a1_filename}", line {a1_lineno}, in a1\n',
                "    return corofn()  # comment in a1",
            ]))
            check(2, "".join([
                f"coroutine '{corofn.__qualname__}' was never awaited\n",
                "Coroutine created at (most recent call last)\n",
                f'  File "{a2_filename}", line {a2_lineno}, in a2\n',
                "    return a1()  # comment in a2\n",
                f'  File "{a1_filename}", line {a1_lineno}, in a1\n',
                "    return corofn()  # comment in a1",
            ]))

        finally:
            sys.set_coroutine_origin_tracking_depth(orig_depth)

    def test_unawaited_warning_when_module_broken(self):
        # Make sure we don't blow up too bad if
        # warnings._warn_unawaited_coroutine is broken somehow (e.g. because
        # of shutdown problems)
        async def corofn():
            pass

        orig_wuc = warnings._warn_unawaited_coroutine
        try:
            warnings._warn_unawaited_coroutine = lambda coro: 1/0
            with support.catch_unraisable_exception() as cm, \
                 warnings_helper.check_warnings(
                         (r'coroutine .* was never awaited',
                          RuntimeWarning)):
                # only store repr() to avoid keeping the coroutine alive
                coro = corofn()
                coro_repr = repr(coro)

                # clear reference to the coroutine without awaiting for it
                del coro
                support.gc_collect()

                self.assertEqual(cm.unraisable.err_msg,
                                 f"Exception ignored while finalizing "
                                 f"coroutine {coro_repr}")
                self.assertEqual(cm.unraisable.exc_type, ZeroDivisionError)

            del warnings._warn_unawaited_coroutine
            with warnings_helper.check_warnings(
                    (r'coroutine .* was never awaited', RuntimeWarning)):
                corofn()
                support.gc_collect()

        finally:
            warnings._warn_unawaited_coroutine = orig_wuc


class UnawaitedWarningDuringShutdownTest(unittest.TestCase):
    # https://bugs.python.org/issue32591#msg310726
    def test_unawaited_warning_during_shutdown(self):
        code = ("import asyncio\n"
                "async def f(): pass\n"
                "async def t(): asyncio.gather(f())\n"
                "asyncio.run(t())\n")
        assert_python_ok("-c", code)

        code = ("import sys\n"
                "async def f(): pass\n"
                "sys.coro = f()\n")
        assert_python_ok("-c", code)

        code = ("import sys\n"
                "async def f(): pass\n"
                "sys.corocycle = [f()]\n"
                "sys.corocycle.append(sys.corocycle)\n")
        assert_python_ok("-c", code)


@support.cpython_only
class LazyCoroutineTest(unittest.TestCase):
    """A coroutine that is awaited right away (``await f()``) runs in
    the calling thread's frame and only gets a coroutine object when
    it first suspends.  This must not be observable, except that no
    coroutine object exists before the first suspension."""

    @staticmethod
    def coroutines_for(code):
        return [o for o in gc.get_objects()
                if type(o) is types.CoroutineType and o.cr_code is code]

    def test_no_object_before_first_suspension(self):
        seen = []

        async def inner():
            seen.append(len(self.coroutines_for(inner.__code__)))
            await AsyncYield(1)
            seen.append(len(self.coroutines_for(inner.__code__)))
            return 42

        async def outer():
            return await inner()

        self.assertEqual(run_async(outer()), ([1], 42))
        self.assertEqual(seen, [0, 1])

    def test_return_without_suspending(self):
        async def inner(x):
            return x + 1

        async def outer():
            total = 0
            for i in range(100):
                total += await inner(i)
            return total

        self.assertEqual(run_async(outer()), ([], sum(range(1, 101))))

    def test_nested_chain(self):
        async def chain(n):
            if n == 0:
                await AsyncYield("bottom")
                return 0
            return 1 + await chain(n - 1)

        self.assertEqual(run_async(chain(20)), (["bottom"], 20))

    def test_exception_before_first_suspension(self):
        async def inner():
            raise ValueError("boom")

        async def outer():
            try:
                await inner()
            except ValueError as exc:
                tb = exc.__traceback__
                names = []
                while tb is not None:
                    names.append(tb.tb_frame.f_code.co_name)
                    tb = tb.tb_next
                return names

        self.assertEqual(run_async(outer()), ([], ["outer", "inner"]))

    def test_stopiteration_before_first_suspension(self):
        async def inner():
            raise StopIteration

        async def outer():
            await inner()

        with self.assertRaisesRegex(RuntimeError,
                                    "coroutine raised StopIteration"):
            run_async(outer())

    def test_frame_object(self):
        async def inner():
            x = 1
            frame = sys._getframe()
            self.assertIsNone(frame.f_generator)
            self.assertEqual(frame.f_locals["x"], 1)
            self.assertEqual(frame.f_back.f_code.co_name, "outer")
            await AsyncYield(1)
            coro = frame.f_generator
            self.assertIsInstance(coro, types.CoroutineType)
            self.assertIs(coro.cr_frame, frame)
            self.assertIs(coro.cr_code, inner.__code__)
            self.assertEqual(frame.f_locals["x"], 1)
            self.assertEqual(frame.f_back.f_code.co_name, "outer")
            x = 2
            return frame

        async def outer():
            return await inner()

        _, frame = run_async(outer())
        self.assertIsNone(frame.f_generator)  # finished
        self.assertEqual(frame.f_locals["x"], 2)

    def test_exc_info_suspend_inside_handler(self):
        async def inner():
            try:
                raise KeyError
            except KeyError:
                await AsyncYield(1)
                self.assertIsInstance(sys.exc_info()[1], KeyError)
                await AsyncYield(2)
                self.assertIsInstance(sys.exc_info()[1], KeyError)
            # The caller's context is visible after the handler.
            self.assertIsInstance(sys.exc_info()[1], TypeError)
            await AsyncYield(3)
            self.assertIsInstance(sys.exc_info()[1], TypeError)
            return "ok"

        async def outer():
            try:
                raise TypeError
            except TypeError:
                result = await inner()
                self.assertIsInstance(sys.exc_info()[1], TypeError)
            self.assertIsNone(sys.exc_info()[1])
            return result

        self.assertEqual(run_async(outer()), ([1, 2, 3], "ok"))

    def test_exc_info_handler_without_suspending(self):
        async def inner():
            try:
                raise KeyError
            except KeyError:
                self.assertIsInstance(sys.exc_info()[1], KeyError)
            self.assertIsInstance(sys.exc_info()[1], TypeError)
            return "ok"

        async def outer():
            try:
                raise TypeError
            except TypeError:
                result = await inner()
                self.assertIsInstance(sys.exc_info()[1], TypeError)
            self.assertIsNone(sys.exc_info()[1])
            return result

        self.assertEqual(run_async(outer()), ([], "ok"))

    def test_throw_into_materialized_coroutine(self):
        async def inner():
            try:
                await AsyncYield(1)
            except ZeroDivisionError:
                return "caught"
            return "not caught"

        async def outer():
            return await inner()

        coro = outer()
        self.assertEqual(coro.send(None), 1)
        with self.assertRaises(StopIteration) as cm:
            coro.throw(ZeroDivisionError)
        self.assertEqual(cm.exception.value, "caught")

    def test_close_materialized_coroutine(self):
        closed = []

        async def inner():
            try:
                await AsyncYield(1)
            finally:
                closed.append(True)

        async def outer():
            await inner()

        coro = outer()
        self.assertEqual(coro.send(None), 1)
        coro.close()
        self.assertEqual(closed, [True])

    def test_async_with(self):
        log = []

        class CM:
            async def __aenter__(self):
                log.append("enter")
                return self

            async def __aexit__(self, *args):
                log.append("exit")
                return False

        async def outer():
            async with CM() as cm:
                log.append(type(cm).__name__)
                await AsyncYield(1)
            return "done"

        self.assertEqual(run_async(outer()), ([1], "done"))
        self.assertEqual(log, ["enter", "CM", "exit"])

    def test_async_with_lazy(self):
        log = []

        class CM:
            def __init__(self, suspend=False, suppress=False):
                self.suspend = suspend
                self.suppress = suppress

            async def __aenter__(self):
                log.append(("enter", sys._getframe().f_generator is None))
                if self.suspend:
                    await AsyncYield("enter")
                return self

            async def __aexit__(self, typ, *args):
                log.append(("exit", typ, sys._getframe().f_generator is None))
                if self.suspend:
                    await AsyncYield("exit")
                return self.suppress

        async def body(cm):
            async with cm:
                return "returned"

        async def loop(cm):
            for i in range(2):
                async with cm:
                    if i == 0:
                        continue
                    break
            return "looped"

        async def nested(cm):
            async with cm, cm as x:
                pass
            return x

        async def raises(cm):
            async with cm:
                raise ValueError
            return "suppressed"

        for suspend in False, True:
            with self.subTest(suspend=suspend):
                y = ["enter", "exit"] if suspend else []
                lazy = [("enter", True), ("exit", None, True)]
                log.clear()
                self.assertEqual(run_async(body(CM(suspend))), (y, "returned"))
                self.assertEqual(log, lazy)
                log.clear()
                self.assertEqual(run_async(loop(CM(suspend))), (y * 2, "looped"))
                self.assertEqual(log, lazy * 2)
                log.clear()
                cm = CM(suspend)
                self.assertEqual(run_async(nested(cm)), (y[:1] * 2 + y[1:] * 2, cm))
                self.assertEqual(log, [lazy[0], lazy[0], lazy[1], lazy[1]])
                # WITH_EXCEPT_START calls __aexit__() eagerly.
                log.clear()
                self.assertEqual(run_async(raises(CM(suspend, True))),
                                 (y, "suppressed"))
                self.assertEqual(log, [("enter", True), ("exit", ValueError, False)])
                log.clear()
                with self.assertRaises(ValueError):
                    run_async(raises(CM(suspend)))

    def test_async_comprehension(self):
        async def inner(x):
            return x * 2

        async def outer():
            return [await inner(x) for x in range(5)]

        self.assertEqual(run_async(outer()), ([], [0, 2, 4, 6, 8]))

    def test_origin_tracking(self):
        orig_depth = sys.get_coroutine_origin_tracking_depth()

        async def inner():
            await AsyncYield(1)
            return sys._getframe().f_generator.cr_origin

        async def outer():
            coro = inner()
            eager = (await coro)[0]
            lazy = (await inner())[0]
            return eager, lazy

        sys.set_coroutine_origin_tracking_depth(2)
        try:
            _, (eager, lazy) = run_async(outer())
        finally:
            sys.set_coroutine_origin_tracking_depth(orig_depth)
        # Origin tracking disables the lazy path: the origin is the
        # call, not the await, and is captured at creation.
        self.assertEqual(eager[0], __file__)
        self.assertEqual(eager[2], "outer")
        self.assertEqual(lazy[0], __file__)
        self.assertEqual(lazy[2], "outer")
        self.assertEqual(lazy[1], eager[1] + 2)

    def test_origin_tracking_multiline_await(self):
        orig_depth = sys.get_coroutine_origin_tracking_depth()

        async def inner():
            await AsyncYield(1)
            return sys._getframe().f_generator.cr_origin

        async def outer():
            origin = await (
                inner()
            )
            return origin[0][1] - outer.__code__.co_firstlineno

        sys.set_coroutine_origin_tracking_depth(1)
        try:
            _, line = run_async(outer())
        finally:
            sys.set_coroutine_origin_tracking_depth(orig_depth)
        # The line of the call, not of the await.
        self.assertEqual(line, 2)

    def test_settrace_events(self):
        async def inner():
            await AsyncYield(1)
            return 1

        async def eager():
            coro = inner()
            return await coro

        async def lazy():
            return await inner()

        def trace(coro_fn):
            events = []

            def tracer(frame, event, arg):
                if frame.f_code is inner.__code__:
                    events.append((event, frame.f_lineno - frame.f_code.co_firstlineno))
                return tracer

            sys.settrace(tracer)
            try:
                result = run_async(coro_fn())
            finally:
                sys.settrace(None)
            return events, result

        self.assertEqual(trace(lazy), trace(eager))

    def test_monitoring_events(self):
        async def inner():
            await AsyncYield(1)
            return 1

        async def eager():
            coro = inner()
            return await coro

        async def lazy():
            return await inner()

        E = sys.monitoring.events
        # No LINE events: eager() has one more line.
        names = {E.PY_START: "start", E.PY_RESUME: "resume",
                 E.PY_RETURN: "return", E.PY_YIELD: "yield",
                 E.STOP_ITERATION: "stop_iteration"}
        codes = {inner.__code__: "inner", eager.__code__: "outer",
                 lazy.__code__: "outer"}

        def monitor(coro_fn):
            events = []
            tool = sys.monitoring.DEBUGGER_ID
            sys.monitoring.use_tool_id(tool, "test")
            try:
                for event, name in names.items():
                    def callback(code, offset, *args, name=name):
                        if code in codes:
                            events.append(
                                (codes[code], name, type(args[-1]) if args else None))
                    sys.monitoring.register_callback(tool, event, callback)
                sys.monitoring.set_events(tool, sum(names))
                result = run_async(coro_fn())
            finally:
                sys.monitoring.set_events(tool, 0)
                sys.monitoring.free_tool_id(tool)
            return events, result

        self.assertEqual(monitor(lazy), monitor(eager))
        self.assertIn(("outer", "stop_iteration", StopIteration),
                      monitor(lazy)[0])

    def test_stop_iteration_event_without_suspending(self):
        async def inner():
            return 42

        async def outer():
            return await inner()

        E = sys.monitoring.events
        events = []
        tool = sys.monitoring.DEBUGGER_ID
        sys.monitoring.use_tool_id(tool, "test")
        try:
            def callback(code, offset, exc):
                if code is outer.__code__:
                    events.append(exc.value)
            sys.monitoring.register_callback(tool, E.STOP_ITERATION, callback)
            sys.monitoring.set_events(tool, E.STOP_ITERATION)
            self.assertEqual(run_async(outer()), ([], 42))
        finally:
            sys.monitoring.set_events(tool, 0)
            sys.monitoring.free_tool_id(tool)
        self.assertEqual(events, [42])

    def test_exc_state_after_suspending_inside_handler(self):
        # The coroutine's own exception state must not keep the
        # caller's exception once its handler is done.
        async def inner():
            try:
                raise KeyError
            except KeyError:
                await AsyncYield(1)
            await AsyncYield(2)
            return sys.exc_info()[1]

        async def outer():
            try:
                raise TypeError
            except TypeError:
                return await inner()

        coro = outer()
        coro.send(None)
        inner_coro = coro.cr_await
        inner_coro.send(None)
        self.assertNotIn(TypeError,
                         map(type, gc.get_referents(inner_coro)))
        # Resumed directly, not through outer: no handled exception.
        with self.assertRaises(StopIteration) as cm:
            inner_coro.send(None)
        self.assertIsNone(cm.exception.value)
        coro.close()

    def test_throw_into_coroutine_suspended_inside_handler(self):
        async def inner():
            try:
                raise KeyError
            except KeyError:
                try:
                    await AsyncYield(1)
                except ZeroDivisionError:
                    pass
            return sys.exception()

        async def outer():
            try:
                raise TypeError
            except TypeError:
                result = await inner()
                self.assertIsInstance(sys.exception(), TypeError)
                return result

        coro = outer()
        self.assertEqual(coro.send(None), 1)
        with self.assertRaises(StopIteration) as cm:
            coro.throw(ZeroDivisionError)
        self.assertIsNone(cm.exception.value)

    def test_import_hook_locals(self):
        seen = []
        orig_import = builtins.__import__

        def hook(name, globals=None, locals=None, fromlist=(), level=0):
            seen.append(locals)
            return orig_import(name, globals, locals, fromlist, level)

        async def inner():
            import sys as _sys
            return _sys

        async def outer():
            try:
                raise TypeError
            except TypeError:
                return await inner()

        with support.swap_attr(builtins, "__import__", hook):
            self.assertEqual(run_async(outer()), ([], sys))
        self.assertEqual(seen, [None])

    def test_traceback_frame_keeps_no_caller_exception(self):
        async def inner():
            raise ValueError

        async def outer():
            try:
                raise TypeError
            except TypeError:
                try:
                    await inner()
                except ValueError as exc:
                    return exc.__traceback__.tb_next.tb_frame

        _, frame = run_async(outer())
        self.assertIs(frame.f_code, inner.__code__)
        self.assertNotIn(TypeError, map(type, gc.get_referents(frame)))

    def test_code_replaced_before_first_suspension(self):
        # The coroutine object is created at the first suspension, so it
        # must be sized for the code the frame runs, not func.__code__.
        async def small():
            big, small  # same free variables as big()
            await AsyncYield(1)

        async def big():
            a = b = c = d = e = f = g = h = i = j = k = m = n = o = p = 1
            big.__code__ = small.__code__
            await AsyncYield(1)
            return (a, b, c, d, e, f, g, h, i, j, k, m, n, o, p)

        async def outer():
            return await big()

        self.assertEqual(run_async(outer()), ([1], (1,) * 15))

    def test_finished_frame_has_no_f_back(self):
        async def inner():
            return sys._getframe()

        async def outer():
            return await inner()

        _, frame = run_async(outer())
        self.assertIs(frame.f_code, inner.__code__)
        self.assertIsNone(frame.f_back)

    @unittest.skipIf(_testcapi is None, "requires _testcapi")
    def test_memory_error_at_first_suspension(self):
        @types.coroutine
        def suspend():
            yield 1

        class CM:
            async def __aenter__(self):
                log.append("enter")
            async def __aexit__(self, typ, *args):
                log.append(("exit", typ))

        log = []
        awaitable = suspend()

        async def inner():
            async with CM():
                _testcapi.set_nomemory(0, 1)
                try:
                    await awaitable
                finally:
                    _testcapi.remove_mem_hooks()
                    log.append("finally")
            return "resumed"

        async def outer():
            return await inner()

        coro = outer()
        try:
            with self.assertRaises(MemoryError):
                coro.send(None)
        finally:
            _testcapi.remove_mem_hooks()
            awaitable.close()
        self.assertEqual(log, ["enter", "finally", ("exit", MemoryError)])

    def test_recursion_error_before_body_runs(self):
        async def f():
            return await f()

        coro = f()
        with support.infinite_recursion(50):
            with self.assertRaises(RecursionError):
                coro.send(None)

    def test_memory_error_before_body_runs(self):
        # MAKE_CELL runs before RETURN_GENERATOR.
        async def inner(x):
            def g():
                return x
            return g

        async def outer():
            _testcapi.set_nomemory(0, 1)
            try:
                await inner(1)
            except MemoryError:
                return "MemoryError"
            finally:
                _testcapi.remove_mem_hooks()

        self.assertEqual(run_async(outer()), ([], "MemoryError"))

    def test_caller_location_while_body_runs(self):
        positions = []

        async def inner():
            caller = sys._getframe(1)
            positions.append(
                list(caller.f_code.co_positions())[caller.f_lasti // 2])

        async def outer():
            x = (await
                 inner())

        run_async(outer())
        line = outer.__code__.co_firstlineno + 1
        self.assertEqual(positions, [(line, line + 1, 17, 24)])

    def test_origin_tracking_enabled_in_body(self):
        @types.coroutine
        def suspend():
            yield

        async def inner():
            sys.set_coroutine_origin_tracking_depth(5)
            await suspend()

        async def outer():
            await inner()

        coro = outer()
        try:
            coro.send(None)
            self.assertIsNone(coro.cr_await.cr_origin)
        finally:
            sys.set_coroutine_origin_tracking_depth(0)
            coro.close()

    def test_coroutine_flag_without_coroutine_body(self):
        def f():
            return 42
        code = f.__code__.replace(co_flags=f.__code__.co_flags | inspect.CO_COROUTINE)
        g = types.FunctionType(code, globals())

        async def outer():
            return await g()

        with self.assertRaisesRegex(TypeError, "can't be awaited"):
            run_async(outer())

    def test_line_events_at_awaited_call_jump_target(self):
        # The list(genexpr) fast path jumps to END_AWAITED_CALL.
        async def list(arg):
            await AsyncYield("suspended")
            return "done"

        async def outer():
            x = (await
                 list(i for i in range(3)))
            return x

        def tracer(frame, event, arg):
            if event == "line" and frame.f_code is outer.__code__:
                gc.collect()
            return tracer

        for _ in range(100):  # Specialize the call.
            run_async(outer())
        old_trace = sys.gettrace()
        sys.settrace(tracer)
        try:
            self.assertEqual(run_async(outer()), (["suspended"], "done"))
        finally:
            sys.settrace(old_trace)

    def test_line_events_on_await_line(self):
        async def returns():
            return "done"

        async def suspends():
            await AsyncYield("suspended")
            return "done"

        async def outer(f):
            x = (await
                 f())
            return x

        mon = sys.monitoring
        tool = mon.DEBUGGER_ID
        for f in returns, suspends:
            with self.subTest(f=f.__name__):
                lines = []
                def line(code, lineno):
                    if code is outer.__code__:
                        lines.append(lineno - code.co_firstlineno)
                mon.use_tool_id(tool, "test")
                try:
                    mon.register_callback(tool, mon.events.LINE, line)
                    mon.set_events(tool, mon.events.LINE)
                    run_async(outer(f))
                finally:
                    mon.set_events(tool, 0)
                    mon.register_callback(tool, mon.events.LINE, None)
                    mon.free_tool_id(tool)
                self.assertEqual(lines, [2, 1, 3])

    def test_call_forms(self):
        class C:
            async def meth(self, x, *, y=0):
                if y:
                    await AsyncYield(y)
                return ("meth", x, y)

        async def func(*args, **kwargs):
            if kwargs.get("suspend"):
                await AsyncYield("f")
            return (args, kwargs)

        obj = C()
        bound = obj.meth

        async def outer():
            return [
                await obj.meth(1),
                await obj.meth(2, y=3),
                await bound(4),
                await bound(5, y=6),
                await func(7),
                await func(*[8, 9]),
                await func(**{"a": 10}),
                await func(11, suspend=True),
            ]

        for _ in range(3):  # also with specialized calls
            self.assertEqual(run_async(outer()), (
                [3, 6, "f"],
                [("meth", 1, 0), ("meth", 2, 3), ("meth", 4, 0),
                 ("meth", 5, 6), ((7,), {}), ((8, 9), {}),
                 ((), {"a": 10}), ((11,), {"suspend": True})],
            ))

    def test_send_specialized_for_other_awaitables(self):
        # The SEND of an await site may have been specialized for another
        # kind of awaitable before a coroutine function is awaited there.
        async def agen():
            yield 1

        class Iterator:
            async def __anext__(self):
                return 2

        class SuspendingIterator:
            async def __anext__(self):
                await AsyncYield("s")
                return 3

        async def outer(it):
            return await type(it).__anext__(it)

        def check(it, expected):
            self.assertEqual(run_async(outer(it)), expected)

        for _ in range(3):
            for _ in range(20):
                check(agen(), ([], 1))
            check(Iterator(), ([], 2))
            check(SuspendingIterator(), (["s"], 3))

    def test_traceback_like_eager_coroutine(self):
        async def inner():
            raise ValueError

        async def outer():
            await inner()

        def format_tb():
            try:
                run_async(outer())
            except ValueError:
                return traceback.format_exc()

        lazy = format_tb()
        # Origin tracking makes awaited calls create the coroutine eagerly.
        old_depth = sys.get_coroutine_origin_tracking_depth()
        sys.set_coroutine_origin_tracking_depth(1)
        try:
            eager = format_tb()
        finally:
            sys.set_coroutine_origin_tracking_depth(old_depth)
        # The whole `await inner()` line, not just `inner()` with carets.
        self.assertIn("    await inner()\n  File", lazy)
        self.assertEqual(lazy, eager)

    def test_traceback_multiline_await(self):
        async def inner():
            raise ValueError

        async def outer():
            x = (await
                 inner())
            return x

        def format_tb():
            try:
                run_async(outer())
            except ValueError:
                return traceback.format_exc()

        lazy = format_tb()
        old_depth = sys.get_coroutine_origin_tracking_depth()
        sys.set_coroutine_origin_tracking_depth(1)
        try:
            eager = format_tb()
        finally:
            sys.set_coroutine_origin_tracking_depth(old_depth)
        self.assertEqual(lazy, eager)

    def test_instrumentation_enabled_before_first_suspension(self):
        # END_AWAITED_CALL must run right after the body suspends, before
        # anything else, even if LINE or INSTRUCTION events get enabled
        # while the body runs.
        E = sys.monitoring.events
        tool = sys.monitoring.DEBUGGER_ID

        async def inner(events):
            sys.monitoring.set_events(tool, events)
            await AsyncYield("suspended")
            return "done"

        async def outer(events):
            x = (await
                 inner(events))
            return x

        for events in (E.LINE, E.INSTRUCTION, E.LINE | E.INSTRUCTION):
            with self.subTest(events=events):
                sys.monitoring.use_tool_id(tool, "test")
                try:
                    for event in (E.LINE, E.INSTRUCTION):
                        sys.monitoring.register_callback(
                            tool, event, lambda *args: gc.collect())
                    self.assertEqual(run_async(outer(events)),
                                     (["suspended"], "done"))
                finally:
                    sys.monitoring.set_events(tool, 0)
                    sys.monitoring.free_tool_id(tool)


@support.cpython_only
@unittest.skipIf(_testcapi is None, "requires _testcapi")
class CAPITest(unittest.TestCase):

    def test_tp_await_1(self):
        from _testcapi import awaitType as at

        async def foo():
            future = at(iter([1]))
            return (await future)

        self.assertEqual(foo().send(None), 1)

    def test_tp_await_2(self):
        # Test tp_await to __await__ mapping
        from _testcapi import awaitType as at
        future = at(iter([1]))
        self.assertEqual(next(future.__await__()), 1)

    def test_tp_await_3(self):
        from _testcapi import awaitType as at

        async def foo():
            future = at(1)
            return (await future)

        with self.assertRaisesRegex(
                TypeError, "__await__.*must return an iterator, not int"):
            self.assertEqual(foo().send(None), 1)


if __name__=="__main__":
    unittest.main()
