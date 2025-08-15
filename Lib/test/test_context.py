import concurrent.futures
import contextvars
import functools
import gc
import random
import time
import unittest
import weakref
from test import support
from test.support import threading_helper

try:
    from _testinternalcapi import hamt
except ImportError:
    hamt = None


def isolated_context(func):
    """Needed to make reftracking test mode work."""
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        ctx = contextvars.Context()
        return ctx.run(func, *args, **kwargs)
    return wrapper


class ContextTest(unittest.TestCase):
    def test_context_var_new_1(self):
        with self.assertRaisesRegex(TypeError, 'takes exactly 1'):
            contextvars.ContextVar()

        with self.assertRaisesRegex(TypeError, 'must be a str'):
            contextvars.ContextVar(1)

        c = contextvars.ContextVar('aaa')
        self.assertEqual(c.name, 'aaa')

        with self.assertRaises(AttributeError):
            c.name = 'bbb'

        self.assertNotEqual(hash(c), hash('aaa'))

    @isolated_context
    def test_context_var_repr_1(self):
        c = contextvars.ContextVar('a')
        self.assertIn('a', repr(c))

        c = contextvars.ContextVar('a', default=123)
        self.assertIn('123', repr(c))

        lst = []
        c = contextvars.ContextVar('a', default=lst)
        lst.append(c)
        self.assertIn('...', repr(c))
        self.assertIn('...', repr(lst))

        t = c.set(1)
        self.assertIn(repr(c), repr(t))
        self.assertNotIn(' used ', repr(t))
        c.reset(t)
        self.assertIn(' used ', repr(t))

    def test_context_subclassing_1(self):
        with self.assertRaisesRegex(TypeError, 'not an acceptable base type'):
            class MyContextVar(contextvars.ContextVar):
                # Potentially we might want ContextVars to be subclassable.
                pass

        with self.assertRaisesRegex(TypeError, 'not an acceptable base type'):
            class MyContext(contextvars.Context):
                pass

        with self.assertRaisesRegex(TypeError, 'not an acceptable base type'):
            class MyToken(contextvars.Token):
                pass

    def test_context_new_1(self):
        with self.assertRaisesRegex(TypeError, 'any arguments'):
            contextvars.Context(1)
        with self.assertRaisesRegex(TypeError, 'any arguments'):
            contextvars.Context(1, a=1)
        with self.assertRaisesRegex(TypeError, 'any arguments'):
            contextvars.Context(a=1)
        contextvars.Context(**{})

    def test_context_new_unhashable_str_subclass(self):
        # gh-132002: it used to crash on unhashable str subtypes.
        class weird_str(str):
            def __eq__(self, other):
                pass

        with self.assertRaisesRegex(TypeError, 'unhashable type'):
            contextvars.ContextVar(weird_str())

    def test_context_typerrors_1(self):
        ctx = contextvars.Context()

        with self.assertRaisesRegex(TypeError, 'ContextVar key was expected'):
            ctx[1]
        with self.assertRaisesRegex(TypeError, 'ContextVar key was expected'):
            1 in ctx
        with self.assertRaisesRegex(TypeError, 'ContextVar key was expected'):
            ctx.get(1)

    def test_context_get_context_1(self):
        ctx = contextvars.copy_context()
        self.assertIsInstance(ctx, contextvars.Context)

    def test_context_run_1(self):
        ctx = contextvars.Context()

        with self.assertRaisesRegex(TypeError, 'missing 1 required'):
            ctx.run()

    def test_context_run_2(self):
        ctx = contextvars.Context()

        def func(*args, **kwargs):
            kwargs['spam'] = 'foo'
            args += ('bar',)
            return args, kwargs

        for f in (func, functools.partial(func)):
            # partial doesn't support FASTCALL

            self.assertEqual(ctx.run(f), (('bar',), {'spam': 'foo'}))
            self.assertEqual(ctx.run(f, 1), ((1, 'bar'), {'spam': 'foo'}))

            self.assertEqual(
                ctx.run(f, a=2),
                (('bar',), {'a': 2, 'spam': 'foo'}))

            self.assertEqual(
                ctx.run(f, 11, a=2),
                ((11, 'bar'), {'a': 2, 'spam': 'foo'}))

            a = {}
            self.assertEqual(
                ctx.run(f, 11, **a),
                ((11, 'bar'), {'spam': 'foo'}))
            self.assertEqual(a, {})

    def test_context_run_3(self):
        ctx = contextvars.Context()

        def func(*args, **kwargs):
            1 / 0

        with self.assertRaises(ZeroDivisionError):
            ctx.run(func)
        with self.assertRaises(ZeroDivisionError):
            ctx.run(func, 1, 2)
        with self.assertRaises(ZeroDivisionError):
            ctx.run(func, 1, 2, a=123)

    @isolated_context
    def test_context_run_4(self):
        ctx1 = contextvars.Context()
        ctx2 = contextvars.Context()
        var = contextvars.ContextVar('var')

        def func2():
            self.assertIsNone(var.get(None))

        def func1():
            self.assertIsNone(var.get(None))
            var.set('spam')
            ctx2.run(func2)
            self.assertEqual(var.get(None), 'spam')

            cur = contextvars.copy_context()
            self.assertEqual(len(cur), 1)
            self.assertEqual(cur[var], 'spam')
            return cur

        returned_ctx = ctx1.run(func1)
        self.assertEqual(ctx1, returned_ctx)
        self.assertEqual(returned_ctx[var], 'spam')
        self.assertIn(var, returned_ctx)

    def test_context_run_5(self):
        ctx = contextvars.Context()
        var = contextvars.ContextVar('var')

        def func():
            self.assertIsNone(var.get(None))
            var.set('spam')
            1 / 0

        with self.assertRaises(ZeroDivisionError):
            ctx.run(func)

        self.assertIsNone(var.get(None))

    def test_context_run_6(self):
        ctx = contextvars.Context()
        c = contextvars.ContextVar('a', default=0)

        def fun():
            self.assertEqual(c.get(), 0)
            self.assertIsNone(ctx.get(c))

            c.set(42)
            self.assertEqual(c.get(), 42)
            self.assertEqual(ctx.get(c), 42)

        ctx.run(fun)

    def test_context_run_7(self):
        ctx = contextvars.Context()

        def fun():
            with self.assertRaisesRegex(RuntimeError, 'is already entered'):
                ctx.run(fun)

        ctx.run(fun)

    @isolated_context
    def test_context_getset_1(self):
        c = contextvars.ContextVar('c')
        with self.assertRaises(LookupError):
            c.get()

        self.assertIsNone(c.get(None))

        t0 = c.set(42)
        self.assertEqual(c.get(), 42)
        self.assertEqual(c.get(None), 42)
        self.assertIs(t0.old_value, t0.MISSING)
        self.assertIs(t0.old_value, contextvars.Token.MISSING)
        self.assertIs(t0.var, c)

        t = c.set('spam')
        self.assertEqual(c.get(), 'spam')
        self.assertEqual(c.get(None), 'spam')
        self.assertEqual(t.old_value, 42)
        c.reset(t)

        self.assertEqual(c.get(), 42)
        self.assertEqual(c.get(None), 42)

        c.set('spam2')
        with self.assertRaisesRegex(RuntimeError, 'has already been used'):
            c.reset(t)
        self.assertEqual(c.get(), 'spam2')

        ctx1 = contextvars.copy_context()
        self.assertIn(c, ctx1)

        c.reset(t0)
        with self.assertRaisesRegex(RuntimeError, 'has already been used'):
            c.reset(t0)
        self.assertIsNone(c.get(None))

        self.assertIn(c, ctx1)
        self.assertEqual(ctx1[c], 'spam2')
        self.assertEqual(ctx1.get(c, 'aa'), 'spam2')
        self.assertEqual(len(ctx1), 1)
        self.assertEqual(list(ctx1.items()), [(c, 'spam2')])
        self.assertEqual(list(ctx1.values()), ['spam2'])
        self.assertEqual(list(ctx1.keys()), [c])
        self.assertEqual(list(ctx1), [c])

        ctx2 = contextvars.copy_context()
        self.assertNotIn(c, ctx2)
        with self.assertRaises(KeyError):
            ctx2[c]
        self.assertEqual(ctx2.get(c, 'aa'), 'aa')
        self.assertEqual(len(ctx2), 0)
        self.assertEqual(list(ctx2), [])

    @isolated_context
    def test_context_getset_2(self):
        v1 = contextvars.ContextVar('v1')
        v2 = contextvars.ContextVar('v2')

        t1 = v1.set(42)
        with self.assertRaisesRegex(ValueError, 'by a different'):
            v2.reset(t1)

    @isolated_context
    def test_context_getset_3(self):
        c = contextvars.ContextVar('c', default=42)
        ctx = contextvars.Context()

        def fun():
            self.assertEqual(c.get(), 42)
            with self.assertRaises(KeyError):
                ctx[c]
            self.assertIsNone(ctx.get(c))
            self.assertEqual(ctx.get(c, 'spam'), 'spam')
            self.assertNotIn(c, ctx)
            self.assertEqual(list(ctx.keys()), [])

            t = c.set(1)
            self.assertEqual(list(ctx.keys()), [c])
            self.assertEqual(ctx[c], 1)

            c.reset(t)
            self.assertEqual(list(ctx.keys()), [])
            with self.assertRaises(KeyError):
                ctx[c]

        ctx.run(fun)

    @isolated_context
    def test_context_getset_4(self):
        c = contextvars.ContextVar('c', default=42)
        ctx = contextvars.Context()

        tok = ctx.run(c.set, 1)

        with self.assertRaisesRegex(ValueError, 'different Context'):
            c.reset(tok)

    @isolated_context
    def test_context_getset_5(self):
        c = contextvars.ContextVar('c', default=42)
        c.set([])

        def fun():
            c.set([])
            c.get().append(42)
            self.assertEqual(c.get(), [42])

        contextvars.copy_context().run(fun)
        self.assertEqual(c.get(), [])

    def test_context_copy_1(self):
        ctx1 = contextvars.Context()
        c = contextvars.ContextVar('c', default=42)

        def ctx1_fun():
            c.set(10)

            ctx2 = ctx1.copy()
            self.assertEqual(ctx2[c], 10)

            c.set(20)
            self.assertEqual(ctx1[c], 20)
            self.assertEqual(ctx2[c], 10)

            ctx2.run(ctx2_fun)
            self.assertEqual(ctx1[c], 20)
            self.assertEqual(ctx2[c], 30)

        def ctx2_fun():
            self.assertEqual(c.get(), 10)
            c.set(30)
            self.assertEqual(c.get(), 30)

        ctx1.run(ctx1_fun)

    @isolated_context
    @threading_helper.requires_working_threading()
    def test_context_threads_1(self):
        cvar = contextvars.ContextVar('cvar')

        def sub(num):
            for i in range(10):
                cvar.set(num + i)
                time.sleep(random.uniform(0.001, 0.05))
                self.assertEqual(cvar.get(), num + i)
            return num

        tp = concurrent.futures.ThreadPoolExecutor(max_workers=10)
        try:
            results = list(tp.map(sub, range(10)))
        finally:
            tp.shutdown()
        self.assertEqual(results, list(range(10)))


# HAMT Tests


class HashKey:
    _crasher = None

    def __init__(self, hash, name, *, error_on_eq_to=None):
        assert hash != -1
        self.name = name
        self.hash = hash
        self.error_on_eq_to = error_on_eq_to

    def __repr__(self):
        return f'<Key name:{self.name} hash:{self.hash}>'

    def __hash__(self):
        if self._crasher is not None and self._crasher.error_on_hash:
            raise HashingError

        return self.hash

    def __eq__(self, other):
        if not isinstance(other, HashKey):
            return NotImplemented

        if self._crasher is not None and self._crasher.error_on_eq:
            raise EqError

        if self.error_on_eq_to is not None and self.error_on_eq_to is other:
            raise ValueError(f'cannot compare {self!r} to {other!r}')
        if other.error_on_eq_to is not None and other.error_on_eq_to is self:
            raise ValueError(f'cannot compare {other!r} to {self!r}')

        return (self.name, self.hash) == (other.name, other.hash)


class KeyStr(str):
    def __hash__(self):
        if HashKey._crasher is not None and HashKey._crasher.error_on_hash:
            raise HashingError
        return super().__hash__()

    def __eq__(self, other):
        if HashKey._crasher is not None and HashKey._crasher.error_on_eq:
            raise EqError
        return super().__eq__(other)


class HaskKeyCrasher:
    def __init__(self, *, error_on_hash=False, error_on_eq=False):
        self.error_on_hash = error_on_hash
        self.error_on_eq = error_on_eq

    def __enter__(self):
        if HashKey._crasher is not None:
            raise RuntimeError('cannot nest crashers')
        HashKey._crasher = self

    def __exit__(self, *exc):
        HashKey._crasher = None


class HashingError(Exception):
    pass


class EqError(Exception):
    pass


@unittest.skipIf(hamt is None, '_testinternalcapi.hamt() not available')
class HamtTest(unittest.TestCase):

    def test_hashkey_helper_1(self):
        k1 = HashKey(10, 'aaa')
        k2 = HashKey(10, 'bbb')

        self.assertNotEqual(k1, k2)
        self.assertEqual(hash(k1), hash(k2))

        d = dict()
        d[k1] = 'a'
        d[k2] = 'b'

        self.assertEqual(d[k1], 'a')
        self.assertEqual(d[k2], 'b')

    def test_hamt_basics_1(self):
        h = hamt()
        h = None  # NoQA

    def test_hamt_basics_2(self):
        h = hamt()
        self.assertEqual(len(h), 0)

        h2 = h.set('a', 'b')
        self.assertIsNot(h, h2)
        self.assertEqual(len(h), 0)
        self.assertEqual(len(h2), 1)

        self.assertIsNone(h.get('a'))
        self.assertEqual(h.get('a', 42), 42)

        self.assertEqual(h2.get('a'), 'b')

        h3 = h2.set('b', 10)
        self.assertIsNot(h2, h3)
        self.assertEqual(len(h), 0)
        self.assertEqual(len(h2), 1)
        self.assertEqual(len(h3), 2)
        self.assertEqual(h3.get('a'), 'b')
        self.assertEqual(h3.get('b'), 10)

        self.assertIsNone(h.get('b'))
        self.assertIsNone(h2.get('b'))

        self.assertIsNone(h.get('a'))
        self.assertEqual(h2.get('a'), 'b')

        h = h2 = h3 = None

    def test_hamt_basics_3(self):
        h = hamt()
        o = object()
        h1 = h.set('1', o)
        h2 = h1.set('1', o)
        self.assertIs(h1, h2)

    def test_hamt_basics_4(self):
        h = hamt()
        h1 = h.set('key', [])
        h2 = h1.set('key', [])
        self.assertIsNot(h1, h2)
        self.assertEqual(len(h1), 1)
        self.assertEqual(len(h2), 1)
        self.assertIsNot(h1.get('key'), h2.get('key'))

    def test_hamt_collision_1(self):
        k1 = HashKey(10, 'aaa')
        k2 = HashKey(10, 'bbb')
        k3 = HashKey(10, 'ccc')

        h = hamt()
        h2 = h.set(k1, 'a')
        h3 = h2.set(k2, 'b')

        self.assertEqual(h.get(k1), None)
        self.assertEqual(h.get(k2), None)

        self.assertEqual(h2.get(k1), 'a')
        self.assertEqual(h2.get(k2), None)

        self.assertEqual(h3.get(k1), 'a')
        self.assertEqual(h3.get(k2), 'b')

        h4 = h3.set(k2, 'cc')
        h5 = h4.set(k3, 'aa')

        self.assertEqual(h3.get(k1), 'a')
        self.assertEqual(h3.get(k2), 'b')
        self.assertEqual(h4.get(k1), 'a')
        self.assertEqual(h4.get(k2), 'cc')
        self.assertEqual(h4.get(k3), None)
        self.assertEqual(h5.get(k1), 'a')
        self.assertEqual(h5.get(k2), 'cc')
        self.assertEqual(h5.get(k2), 'cc')
        self.assertEqual(h5.get(k3), 'aa')

        self.assertEqual(len(h), 0)
        self.assertEqual(len(h2), 1)
        self.assertEqual(len(h3), 2)
        self.assertEqual(len(h4), 2)
        self.assertEqual(len(h5), 3)

    def test_hamt_collision_3(self):
        # Test that iteration works with the deepest tree possible.
        # https://github.com/python/cpython/issues/93065

        C = HashKey(0b10000000_00000000_00000000_00000000, 'C')
        D = HashKey(0b10000000_00000000_00000000_00000000, 'D')

        E = HashKey(0b00000000_00000000_00000000_00000000, 'E')

        h = hamt()
        h = h.set(C, 'C')
        h = h.set(D, 'D')
        h = h.set(E, 'E')

        # BitmapNode(size=2 count=1 bitmap=0b1):
        #   NULL:
        #     BitmapNode(size=2 count=1 bitmap=0b1):
        #       NULL:
        #         BitmapNode(size=2 count=1 bitmap=0b1):
        #           NULL:
        #             BitmapNode(size=2 count=1 bitmap=0b1):
        #               NULL:
        #                 BitmapNode(size=2 count=1 bitmap=0b1):
        #                   NULL:
        #                     BitmapNode(size=2 count=1 bitmap=0b1):
        #                       NULL:
        #                         BitmapNode(size=4 count=2 bitmap=0b101):
        #                           <Key name:E hash:0>: 'E'
        #                           NULL:
        #                             CollisionNode(size=4 id=0x107a24520):
        #                               <Key name:C hash:2147483648>: 'C'
        #                               <Key name:D hash:2147483648>: 'D'

        self.assertEqual({k.name for k in h.keys()}, {'C', 'D', 'E'})

    @support.requires_resource('cpu')
    def test_hamt_stress(self):
        COLLECTION_SIZE = 7000
        TEST_ITERS_EVERY = 647
        CRASH_HASH_EVERY = 97
        CRASH_EQ_EVERY = 11
        RUN_XTIMES = 3

        for _ in range(RUN_XTIMES):
            h = hamt()
            d = dict()

            for i in range(COLLECTION_SIZE):
                key = KeyStr(i)

                if not (i % CRASH_HASH_EVERY):
                    with HaskKeyCrasher(error_on_hash=True):
                        with self.assertRaises(HashingError):
                            h.set(key, i)

                h = h.set(key, i)

                if not (i % CRASH_EQ_EVERY):
                    with HaskKeyCrasher(error_on_eq=True):
                        with self.assertRaises(EqError):
                            h.get(KeyStr(i))  # really trigger __eq__

                d[key] = i
                self.assertEqual(len(d), len(h))

                if not (i % TEST_ITERS_EVERY):
                    self.assertEqual(set(h.items()), set(d.items()))
                    self.assertEqual(len(h.items()), len(d.items()))

            self.assertEqual(len(h), COLLECTION_SIZE)

            for key in range(COLLECTION_SIZE):
                self.assertEqual(h.get(KeyStr(key), 'not found'), key)

            keys_to_delete = list(range(COLLECTION_SIZE))
            random.shuffle(keys_to_delete)
            for iter_i, i in enumerate(keys_to_delete):
                key = KeyStr(i)

                if not (iter_i % CRASH_HASH_EVERY):
                    with HaskKeyCrasher(error_on_hash=True):
                        with self.assertRaises(HashingError):
                            h.delete(key)

                if not (iter_i % CRASH_EQ_EVERY):
                    with HaskKeyCrasher(error_on_eq=True):
                        with self.assertRaises(EqError):
                            h.delete(KeyStr(i))

                h = h.delete(key)
                self.assertEqual(h.get(key, 'not found'), 'not found')
                del d[key]
                self.assertEqual(len(d), len(h))

                if iter_i == COLLECTION_SIZE // 2:
                    hm = h
                    dm = d.copy()

                if not (iter_i % TEST_ITERS_EVERY):
                    self.assertEqual(set(h.keys()), set(d.keys()))
                    self.assertEqual(len(h.keys()), len(d.keys()))

            self.assertEqual(len(d), 0)
            self.assertEqual(len(h), 0)

            # ============

            for key in dm:
                self.assertEqual(hm.get(str(key)), dm[key])
            self.assertEqual(len(dm), len(hm))

            for i, key in enumerate(keys_to_delete):
                hm = hm.delete(str(key))
                self.assertEqual(hm.get(str(key), 'not found'), 'not found')
                dm.pop(str(key), None)
                self.assertEqual(len(d), len(h))

                if not (i % TEST_ITERS_EVERY):
                    self.assertEqual(set(h.values()), set(d.values()))
                    self.assertEqual(len(h.values()), len(d.values()))

            self.assertEqual(len(d), 0)
            self.assertEqual(len(h), 0)
            self.assertEqual(list(h.items()), [])

    def test_hamt_delete_1(self):
        A = HashKey(100, 'A')
        B = HashKey(101, 'B')
        C = HashKey(102, 'C')
        D = HashKey(103, 'D')
        E = HashKey(104, 'E')
        Z = HashKey(-100, 'Z')

        Er = HashKey(103, 'Er', error_on_eq_to=D)

        h = hamt()
        h = h.set(A, 'a')
        h = h.set(B, 'b')
        h = h.set(C, 'c')
        h = h.set(D, 'd')
        h = h.set(E, 'e')

        orig_len = len(h)

        # BitmapNode(size=10 bitmap=0b111110000 id=0x10eadc618):
        #     <Key name:A hash:100>: 'a'
        #     <Key name:B hash:101>: 'b'
        #     <Key name:C hash:102>: 'c'
        #     <Key name:D hash:103>: 'd'
        #     <Key name:E hash:104>: 'e'

        h = h.delete(C)
        self.assertEqual(len(h), orig_len - 1)

        with self.assertRaisesRegex(ValueError, 'cannot compare'):
            h.delete(Er)

        h = h.delete(D)
        self.assertEqual(len(h), orig_len - 2)

        h2 = h.delete(Z)
        self.assertIs(h2, h)

        h = h.delete(A)
        self.assertEqual(len(h), orig_len - 3)

        self.assertEqual(h.get(A, 42), 42)
        self.assertEqual(h.get(B), 'b')
        self.assertEqual(h.get(E), 'e')

    def test_hamt_delete_2(self):
        A = HashKey(100, 'A')
        B = HashKey(201001, 'B')
        C = HashKey(101001, 'C')
        D = HashKey(103, 'D')
        E = HashKey(104, 'E')
        Z = HashKey(-100, 'Z')

        Er = HashKey(201001, 'Er', error_on_eq_to=B)

        h = hamt()
        h = h.set(A, 'a')
        h = h.set(B, 'b')
        h = h.set(C, 'c')
        h = h.set(D, 'd')
        h = h.set(E, 'e')

        orig_len = len(h)

        # BitmapNode(size=8 bitmap=0b1110010000):
        #     <Key name:A hash:100>: 'a'
        #     <Key name:D hash:103>: 'd'
        #     <Key name:E hash:104>: 'e'
        #     NULL:
        #         BitmapNode(size=4 bitmap=0b100000000001000000000):
        #             <Key name:B hash:201001>: 'b'
        #             <Key name:C hash:101001>: 'c'

        with self.assertRaisesRegex(ValueError, 'cannot compare'):
            h.delete(Er)

        h = h.delete(Z)
        self.assertEqual(len(h), orig_len)

        h = h.delete(C)
        self.assertEqual(len(h), orig_len - 1)

        h = h.delete(B)
        self.assertEqual(len(h), orig_len - 2)

        h = h.delete(A)
        self.assertEqual(len(h), orig_len - 3)

        self.assertEqual(h.get(D), 'd')
        self.assertEqual(h.get(E), 'e')

        h = h.delete(A)
        h = h.delete(B)
        h = h.delete(D)
        h = h.delete(E)
        self.assertEqual(len(h), 0)

    def test_hamt_delete_3(self):
        A = HashKey(100, 'A')
        B = HashKey(101, 'B')
        C = HashKey(100100, 'C')
        D = HashKey(100100, 'D')
        E = HashKey(104, 'E')

        h = hamt()
        h = h.set(A, 'a')
        h = h.set(B, 'b')
        h = h.set(C, 'c')
        h = h.set(D, 'd')
        h = h.set(E, 'e')

        orig_len = len(h)

        # BitmapNode(size=6 bitmap=0b100110000):
        #     NULL:
        #         BitmapNode(size=4 bitmap=0b1000000000000000000001000):
        #             <Key name:A hash:100>: 'a'
        #             NULL:
        #                 CollisionNode(size=4 id=0x108572410):
        #                     <Key name:C hash:100100>: 'c'
        #                     <Key name:D hash:100100>: 'd'
        #     <Key name:B hash:101>: 'b'
        #     <Key name:E hash:104>: 'e'

        h = h.delete(A)
        self.assertEqual(len(h), orig_len - 1)

        h = h.delete(E)
        self.assertEqual(len(h), orig_len - 2)

        self.assertEqual(h.get(C), 'c')
        self.assertEqual(h.get(B), 'b')

    def test_hamt_delete_4(self):
        A = HashKey(100, 'A')
        B = HashKey(101, 'B')
        C = HashKey(100100, 'C')
        D = HashKey(100100, 'D')
        E = HashKey(100100, 'E')

        h = hamt()
        h = h.set(A, 'a')
        h = h.set(B, 'b')
        h = h.set(C, 'c')
        h = h.set(D, 'd')
        h = h.set(E, 'e')

        orig_len = len(h)

        # BitmapNode(size=4 bitmap=0b110000):
        #     NULL:
        #         BitmapNode(size=4 bitmap=0b1000000000000000000001000):
        #             <Key name:A hash:100>: 'a'
        #             NULL:
        #                 CollisionNode(size=6 id=0x10515ef30):
        #                     <Key name:C hash:100100>: 'c'
        #                     <Key name:D hash:100100>: 'd'
        #                     <Key name:E hash:100100>: 'e'
        #     <Key name:B hash:101>: 'b'

        h = h.delete(D)
        self.assertEqual(len(h), orig_len - 1)

        h = h.delete(E)
        self.assertEqual(len(h), orig_len - 2)

        h = h.delete(C)
        self.assertEqual(len(h), orig_len - 3)

        h = h.delete(A)
        self.assertEqual(len(h), orig_len - 4)

        h = h.delete(B)
        self.assertEqual(len(h), 0)

    def test_hamt_delete_5(self):
        h = hamt()

        keys = []
        for i in range(17):
            key = HashKey(i, str(i))
            keys.append(key)
            h = h.set(key, f'val-{i}')

        collision_key16 = HashKey(16, '18')
        h = h.set(collision_key16, 'collision')

        # ArrayNode(id=0x10f8b9318):
        #     0::
        #     BitmapNode(size=2 count=1 bitmap=0b1):
        #         <Key name:0 hash:0>: 'val-0'
        #
        # ... 14 more BitmapNodes ...
        #
        #     15::
        #     BitmapNode(size=2 count=1 bitmap=0b1):
        #         <Key name:15 hash:15>: 'val-15'
        #
        #     16::
        #     BitmapNode(size=2 count=1 bitmap=0b1):
        #         NULL:
        #             CollisionNode(size=4 id=0x10f2f5af8):
        #                 <Key name:16 hash:16>: 'val-16'
        #                 <Key name:18 hash:16>: 'collision'

        self.assertEqual(len(h), 18)

        h = h.delete(keys[2])
        self.assertEqual(len(h), 17)

        h = h.delete(collision_key16)
        self.assertEqual(len(h), 16)
        h = h.delete(keys[16])
        self.assertEqual(len(h), 15)

        h = h.delete(keys[1])
        self.assertEqual(len(h), 14)
        h = h.delete(keys[1])
        self.assertEqual(len(h), 14)

        for key in keys:
            h = h.delete(key)
        self.assertEqual(len(h), 0)

    def test_hamt_items_1(self):
        A = HashKey(100, 'A')
        B = HashKey(201001, 'B')
        C = HashKey(101001, 'C')
        D = HashKey(103, 'D')
        E = HashKey(104, 'E')
        F = HashKey(110, 'F')

        h = hamt()
        h = h.set(A, 'a')
        h = h.set(B, 'b')
        h = h.set(C, 'c')
        h = h.set(D, 'd')
        h = h.set(E, 'e')
        h = h.set(F, 'f')

        it = h.items()
        self.assertEqual(
            set(list(it)),
            {(A, 'a'), (B, 'b'), (C, 'c'), (D, 'd'), (E, 'e'), (F, 'f')})

    def test_hamt_items_2(self):
        A = HashKey(100, 'A')
        B = HashKey(101, 'B')
        C = HashKey(100100, 'C')
        D = HashKey(100100, 'D')
        E = HashKey(100100, 'E')
        F = HashKey(110, 'F')

        h = hamt()
        h = h.set(A, 'a')
        h = h.set(B, 'b')
        h = h.set(C, 'c')
        h = h.set(D, 'd')
        h = h.set(E, 'e')
        h = h.set(F, 'f')

        it = h.items()
        self.assertEqual(
            set(list(it)),
            {(A, 'a'), (B, 'b'), (C, 'c'), (D, 'd'), (E, 'e'), (F, 'f')})

    def test_hamt_keys_1(self):
        A = HashKey(100, 'A')
        B = HashKey(101, 'B')
        C = HashKey(100100, 'C')
        D = HashKey(100100, 'D')
        E = HashKey(100100, 'E')
        F = HashKey(110, 'F')

        h = hamt()
        h = h.set(A, 'a')
        h = h.set(B, 'b')
        h = h.set(C, 'c')
        h = h.set(D, 'd')
        h = h.set(E, 'e')
        h = h.set(F, 'f')

        self.assertEqual(set(list(h.keys())), {A, B, C, D, E, F})
        self.assertEqual(set(list(h)), {A, B, C, D, E, F})

    def test_hamt_items_3(self):
        h = hamt()
        self.assertEqual(len(h.items()), 0)
        self.assertEqual(list(h.items()), [])

    def test_hamt_eq_1(self):
        A = HashKey(100, 'A')
        B = HashKey(101, 'B')
        C = HashKey(100100, 'C')
        D = HashKey(100100, 'D')
        E = HashKey(120, 'E')

        h1 = hamt()
        h1 = h1.set(A, 'a')
        h1 = h1.set(B, 'b')
        h1 = h1.set(C, 'c')
        h1 = h1.set(D, 'd')

        h2 = hamt()
        h2 = h2.set(A, 'a')

        self.assertFalse(h1 == h2)
        self.assertTrue(h1 != h2)

        h2 = h2.set(B, 'b')
        self.assertFalse(h1 == h2)
        self.assertTrue(h1 != h2)

        h2 = h2.set(C, 'c')
        self.assertFalse(h1 == h2)
        self.assertTrue(h1 != h2)

        h2 = h2.set(D, 'd2')
        self.assertFalse(h1 == h2)
        self.assertTrue(h1 != h2)

        h2 = h2.set(D, 'd')
        self.assertTrue(h1 == h2)
        self.assertFalse(h1 != h2)

        h2 = h2.set(E, 'e')
        self.assertFalse(h1 == h2)
        self.assertTrue(h1 != h2)

        h2 = h2.delete(D)
        self.assertFalse(h1 == h2)
        self.assertTrue(h1 != h2)

        h2 = h2.set(E, 'd')
        self.assertFalse(h1 == h2)
        self.assertTrue(h1 != h2)

    def test_hamt_eq_2(self):
        A = HashKey(100, 'A')
        Er = HashKey(100, 'Er', error_on_eq_to=A)

        h1 = hamt()
        h1 = h1.set(A, 'a')

        h2 = hamt()
        h2 = h2.set(Er, 'a')

        with self.assertRaisesRegex(ValueError, 'cannot compare'):
            h1 == h2

        with self.assertRaisesRegex(ValueError, 'cannot compare'):
            h1 != h2

    def test_hamt_gc_1(self):
        A = HashKey(100, 'A')

        h = hamt()
        h = h.set(0, 0)  # empty HAMT node is memoized in hamt.c
        ref = weakref.ref(h)

        a = []
        a.append(a)
        a.append(h)
        b = []
        a.append(b)
        b.append(a)
        h = h.set(A, b)

        del h, a, b

        gc.collect()
        gc.collect()
        gc.collect()

        self.assertIsNone(ref())

    def test_hamt_gc_2(self):
        A = HashKey(100, 'A')
        B = HashKey(101, 'B')

        h = hamt()
        h = h.set(A, 'a')
        h = h.set(A, h)

        ref = weakref.ref(h)
        hi = h.items()
        next(hi)

        del h, hi

        gc.collect()
        gc.collect()
        gc.collect()

        self.assertIsNone(ref())

    def test_hamt_in_1(self):
        A = HashKey(100, 'A')
        AA = HashKey(100, 'A')

        B = HashKey(101, 'B')

        h = hamt()
        h = h.set(A, 1)

        self.assertTrue(A in h)
        self.assertFalse(B in h)

        with self.assertRaises(EqError):
            with HaskKeyCrasher(error_on_eq=True):
                AA in h

        with self.assertRaises(HashingError):
            with HaskKeyCrasher(error_on_hash=True):
                AA in h

    def test_hamt_getitem_1(self):
        A = HashKey(100, 'A')
        AA = HashKey(100, 'A')

        B = HashKey(101, 'B')

        h = hamt()
        h = h.set(A, 1)

        self.assertEqual(h[A], 1)
        self.assertEqual(h[AA], 1)

        with self.assertRaises(KeyError):
            h[B]

        with self.assertRaises(EqError):
            with HaskKeyCrasher(error_on_eq=True):
                h[AA]

        with self.assertRaises(HashingError):
            with HaskKeyCrasher(error_on_hash=True):
                h[AA]


if __name__ == "__main__":
    unittest.main()
