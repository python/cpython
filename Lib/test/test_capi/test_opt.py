import contextlib
import itertools
import sys
import textwrap
import unittest
import gc
import os
import types

import _opcode

from test.support import (script_helper, requires_specialization,
                          import_helper, Py_GIL_DISABLED, requires_jit_enabled,
                          reset_code)

_testinternalcapi = import_helper.import_module("_testinternalcapi")

from _testinternalcapi import _PY_NSMALLPOSINTS, TIER2_THRESHOLD, TIER2_RESUME_THRESHOLD

#For test of issue 136154
GLOBAL_136154 = 42

# For frozendict JIT tests
FROZEN_DICT_CONST = frozendict(x=1, y=2)

# For frozenset JIT tests
FROZEN_SET_CONST = frozenset({1, 2, 3})

class _GenericKey:
    pass

_GENERIC_KEY = _GenericKey()


@contextlib.contextmanager
def clear_executors(func):
    # Clear executors in func before and after running a block
    reset_code(func)
    try:
        yield
    finally:
        reset_code(func)


def get_first_executor(func):
    code = func.__code__
    co_code = code.co_code
    for i in range(0, len(co_code), 2):
        try:
            return _opcode.get_executor(code, i)
        except ValueError:
            pass
    return None

def get_all_executors(func):
    code = func.__code__
    co_code = code.co_code
    executors = []
    for i in range(0, len(co_code), 2):
        try:
            executors.append(_opcode.get_executor(code, i))
        except ValueError:
            pass
    return executors


def iter_opnames(ex):
    for item in ex:
        yield item[0]


def get_opnames(ex):
    return list(iter_opnames(ex))

def iter_ops(ex):
    for item in ex:
        yield item

def get_ops(ex):
    return list(iter_ops(ex))

def count_ops(ex, name):
    return len([opname for opname in iter_opnames(ex) if opname == name])


@requires_specialization
@unittest.skipIf(Py_GIL_DISABLED, "optimizer not yet supported in free-threaded builds")
@requires_jit_enabled
class TestExecutorInvalidation(unittest.TestCase):

    def test_invalidate_object(self):
        # Generate a new set of functions at each call
        ns = {}
        func_src = "\n".join(
            f"""
            def f{n}():
                for _ in range({TIER2_THRESHOLD}):
                    pass
            """ for n in range(5)
        )
        exec(textwrap.dedent(func_src), ns, ns)
        funcs = [ ns[f'f{n}'] for n in range(5)]
        objects = [object() for _ in range(5)]

        for f in funcs:
            f()
        executors = [get_first_executor(f) for f in funcs]
        # Set things up so each executor depends on the objects
        # with an equal or lower index.
        for i, exe in enumerate(executors):
            self.assertTrue(exe.is_valid())
            for obj in objects[:i+1]:
                _testinternalcapi.add_executor_dependency(exe, obj)
            self.assertTrue(exe.is_valid())
        # Assert that the correct executors are invalidated
        # and check that nothing crashes when we invalidate
        # an executor multiple times.
        for i in (4,3,2,1,0):
            _testinternalcapi.invalidate_executors(objects[i])
            for exe in executors[i:]:
                self.assertFalse(exe.is_valid())
            for exe in executors[:i]:
                self.assertTrue(exe.is_valid())

    @unittest.skipIf(os.getenv("PYTHON_UOPS_OPTIMIZE") == "0", "Needs uop optimizer to run.")
    def test_uop_optimizer_invalidation(self):
        # Generate a new function at each call
        ns = {}
        exec(textwrap.dedent(f"""
            def f():
                for i in range({TIER2_THRESHOLD}):
                    pass
        """), ns, ns)
        f = ns['f']
        f()
        exe = get_first_executor(f)
        self.assertIsNotNone(exe)
        self.assertTrue(exe.is_valid())
        _testinternalcapi.invalidate_executors(f.__code__)
        self.assertFalse(exe.is_valid())

    def test_sys__clear_internal_caches(self):
        def f():
            for _ in range(TIER2_THRESHOLD):
                pass
        f()
        exe = get_first_executor(f)
        self.assertIsNotNone(exe)
        self.assertTrue(exe.is_valid())
        sys._clear_internal_caches()
        self.assertFalse(exe.is_valid())
        exe = get_first_executor(f)
        self.assertIsNone(exe)

    def test_prev_executor_freed_while_tracing(self):
        def f(start, end, way):
            for x in range(start, end):
                # For the first trace, create a bad branch on purpose to trace into.
                # A side exit will form from here on the second trace.
                y = way + way
                if x >= TIER2_THRESHOLD:
                    # Invalidate the first trace while tracing the second.
                    _testinternalcapi.invalidate_executors(f.__code__)
                    _testinternalcapi.clear_executor_deletion_list()
        f(0, TIER2_THRESHOLD, 1)
        f(1, TIER2_THRESHOLD + 1, 1.0)


def get_bool_guard_ops():
    delta = id(True) ^ id(False)
    for bit in range(4, 8):
        if delta & (1 << bit):
            if id(True) & (1 << bit):
                return f"_GUARD_BIT_IS_UNSET_POP_{bit}", f"_GUARD_BIT_IS_SET_POP_{bit}"
            else:
                return f"_GUARD_BIT_IS_SET_POP_{bit}", f"_GUARD_BIT_IS_UNSET_POP_{bit}"
    return "_GUARD_IS_FALSE_POP", "_GUARD_IS_TRUE_POP"


@requires_specialization
@unittest.skipIf(Py_GIL_DISABLED, "optimizer not yet supported in free-threaded builds")
@requires_jit_enabled
@unittest.skipIf(os.getenv("PYTHON_UOPS_OPTIMIZE") == "0", "Needs uop optimizer to run.")
class TestUops(unittest.TestCase):

    def setUp(self):
        self.guard_is_false, self.guard_is_true = get_bool_guard_ops()

    def test_basic_loop(self):
        def testfunc(x):
            i = 0
            while i < x:
                i += 1

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_JUMP_TO_TOP", uops)
        self.assertIn("_LOAD_FAST_BORROW_0", uops)

    def test_extended_arg(self):
        "Check EXTENDED_ARG handling in superblock creation"
        ns = {}
        exec(textwrap.dedent(f"""
            def many_vars():
                # 260 vars, so z9 should have index 259
                a0 = a1 = a2 = a3 = a4 = a5 = a6 = a7 = a8 = a9 = 42
                b0 = b1 = b2 = b3 = b4 = b5 = b6 = b7 = b8 = b9 = 42
                c0 = c1 = c2 = c3 = c4 = c5 = c6 = c7 = c8 = c9 = 42
                d0 = d1 = d2 = d3 = d4 = d5 = d6 = d7 = d8 = d9 = 42
                e0 = e1 = e2 = e3 = e4 = e5 = e6 = e7 = e8 = e9 = 42
                f0 = f1 = f2 = f3 = f4 = f5 = f6 = f7 = f8 = f9 = 42
                g0 = g1 = g2 = g3 = g4 = g5 = g6 = g7 = g8 = g9 = 42
                h0 = h1 = h2 = h3 = h4 = h5 = h6 = h7 = h8 = h9 = 42
                i0 = i1 = i2 = i3 = i4 = i5 = i6 = i7 = i8 = i9 = 42
                j0 = j1 = j2 = j3 = j4 = j5 = j6 = j7 = j8 = j9 = 42
                k0 = k1 = k2 = k3 = k4 = k5 = k6 = k7 = k8 = k9 = 42
                l0 = l1 = l2 = l3 = l4 = l5 = l6 = l7 = l8 = l9 = 42
                m0 = m1 = m2 = m3 = m4 = m5 = m6 = m7 = m8 = m9 = 42
                n0 = n1 = n2 = n3 = n4 = n5 = n6 = n7 = n8 = n9 = 42
                o0 = o1 = o2 = o3 = o4 = o5 = o6 = o7 = o8 = o9 = 42
                p0 = p1 = p2 = p3 = p4 = p5 = p6 = p7 = p8 = p9 = 42
                q0 = q1 = q2 = q3 = q4 = q5 = q6 = q7 = q8 = q9 = 42
                r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = 42
                s0 = s1 = s2 = s3 = s4 = s5 = s6 = s7 = s8 = s9 = 42
                t0 = t1 = t2 = t3 = t4 = t5 = t6 = t7 = t8 = t9 = 42
                u0 = u1 = u2 = u3 = u4 = u5 = u6 = u7 = u8 = u9 = 42
                v0 = v1 = v2 = v3 = v4 = v5 = v6 = v7 = v8 = v9 = 42
                w0 = w1 = w2 = w3 = w4 = w5 = w6 = w7 = w8 = w9 = 42
                x0 = x1 = x2 = x3 = x4 = x5 = x6 = x7 = x8 = x9 = 42
                y0 = y1 = y2 = y3 = y4 = y5 = y6 = y7 = y8 = y9 = 42
                z0 = z1 = z2 = z3 = z4 = z5 = z6 = z7 = z8 = z9 = {TIER2_THRESHOLD}
                while z9 > 0:
                    z9 = z9 - 1
                    +z9
        """), ns, ns)
        many_vars = ns["many_vars"]

        ex = get_first_executor(many_vars)
        self.assertIsNone(ex)
        many_vars()

        ex = get_first_executor(many_vars)
        self.assertIsNotNone(ex)
        self.assertTrue(any((opcode, oparg, operand) == ("_LOAD_FAST_BORROW", 259, 0)
                            for opcode, oparg, _, operand in list(ex)))

    def test_unspecialized_unpack(self):
        # An example of an unspecialized opcode
        def testfunc(x):
            i = 0
            while i < x:
                i += 1
                a, b = {1: 2, 3: 3}
            assert a == 1 and b == 3
            i = 0
            while i < x:
                i += 1

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_UNPACK_SEQUENCE", uops)

    def test_pop_jump_if_false(self):
        def testfunc(n):
            i = 0
            while i < n:
                i += 1

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn(self.guard_is_true, uops)

    def test_pop_jump_if_none(self):
        def testfunc(a):
            for x in a:
                if x is None:
                    x = 0

        testfunc(range(TIER2_THRESHOLD))

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_GUARD_IS_NONE_POP", uops)
        self.assertNotIn("_GUARD_IS_NOT_NONE_POP", uops)

    def test_pop_jump_if_not_none(self):
        def testfunc(a):
            for x in a:
                x = None
                if x is not None:
                    x = 0

        testfunc(range(TIER2_THRESHOLD))

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_GUARD_IS_NONE_POP", uops)
        self.assertNotIn("_GUARD_IS_NOT_NONE_POP", uops)

    def test_pop_jump_if_true(self):
        def testfunc(n):
            i = 0
            while not i >= n:
                i += 1

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn(self.guard_is_false, uops)

    def test_jump_backward(self):
        def testfunc(n):
            i = 0
            while i < n:
                i += 1

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_JUMP_TO_TOP", uops)

    def test_resume(self):
        def testfunc(x):
            if x <= 1:
                return 1
            return testfunc(x-1)

        for _ in range((TIER2_RESUME_THRESHOLD + 99)//100):
            testfunc(101)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # 0. _START_EXECUTOR
        # 1. _MAKE_WARM
        # 2. _TIER2_RESUME_CHECK
        self.assertEqual(uops[2], "_TIER2_RESUME_CHECK")

    def test_jump_forward(self):
        def testfunc(n):
            a = 0
            while a < n:
                if a < 0:
                    a = -a
                else:
                    a = +a
                a += 1
            return a

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # Since there is no JUMP_FORWARD instruction,
        # look for indirect evidence: the += operator
        self.assertIn("_BINARY_OP_ADD_INT", uops)

    def test_get_iter_list(self):
        l = list(range(10))
        def testfunc(n):
            total = 0
            while n:
                n -= 1
                total += n
                for i in l:
                    break
            return total

        total = testfunc(TIER2_THRESHOLD)
        self.assertEqual(total, sum(range(TIER2_THRESHOLD)))
        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_PUSH_TAGGED_ZERO", uops)
        self.assertNotIn("_GET_ITER", uops)
        self.assertNotIn("_GET_ITER_TRAD", uops)
        self.assertNotIn("_GET_ITER_VIRTUAL", uops)
        self.assertNotIn("_GET_ITER_SELF", uops)

    def test_get_iter_gen(self):
        def gen():
            while True:
                yield 1

        def testfunc(n):
            total = 0
            while n:
                n -= 1
                total += n
                for i in gen():
                    break
            return total

        total = testfunc(TIER2_THRESHOLD)
        self.assertEqual(total, sum(range(TIER2_THRESHOLD)))
        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_PUSH_NULL", uops)
        self.assertNotIn("_GET_ITER", uops)
        self.assertNotIn("_GET_ITER_TRAD", uops)
        self.assertNotIn("_GET_ITER_VIRTUAL", uops)
        self.assertNotIn("_GET_ITER_SELF", uops)

    def test_get_iter_trad(self):
        d = {v:v for v in range(10)}
        def testfunc(n):
            total = 0
            while n:
                n -= 1
                total += n
                for i in d:
                    break
            return total

        total = testfunc(TIER2_THRESHOLD)
        self.assertEqual(total, sum(range(TIER2_THRESHOLD)))
        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_GET_ITER_TRAD", uops)
        self.assertNotIn("_GET_ITER", uops)
        self.assertNotIn("_GET_ITER_VIRTUAL", uops)
        self.assertNotIn("_GET_ITER_SELF", uops)


    def test_for_iter_range(self):
        def testfunc(n):
            total = 0
            for i in range(n):
                total += i
            return total

        total = testfunc(TIER2_THRESHOLD)
        self.assertEqual(total, sum(range(TIER2_THRESHOLD)))

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        # for i, (opname, oparg) in enumerate(ex):
        #     print(f"{i:4d}: {opname:<20s} {oparg:3d}")
        uops = get_opnames(ex)
        self.assertIn("_GUARD_NOT_EXHAUSTED_RANGE", uops)
        # Verification that the jump goes past END_FOR
        # is done by manual inspection of the output

    def test_for_iter_list(self):
        def testfunc(a):
            total = 0
            for i in a:
                total += i
            return total

        a = list(range(TIER2_THRESHOLD))
        total = testfunc(a)
        self.assertEqual(total, sum(a))

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        # for i, (opname, oparg) in enumerate(ex):
        #     print(f"{i:4d}: {opname:<20s} {oparg:3d}")
        uops = get_opnames(ex)
        self.assertIn("_GUARD_NOT_EXHAUSTED_LIST", uops)
        # Verification that the jump goes past END_FOR
        # is done by manual inspection of the output

    def test_for_iter_tuple(self):
        def testfunc(a):
            total = 0
            for i in a:
                total += i
            return total

        a = tuple(range(TIER2_THRESHOLD))
        total = testfunc(a)
        self.assertEqual(total, sum(a))

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        # for i, (opname, oparg) in enumerate(ex):
        #     print(f"{i:4d}: {opname:<20s} {oparg:3d}")
        uops = get_opnames(ex)
        self.assertIn("_GUARD_NOT_EXHAUSTED_TUPLE", uops)
        # Verification that the jump goes past END_FOR
        # is done by manual inspection of the output

    def test_list_edge_case(self):
        def testfunc(it):
            for x in it:
                pass

        a = [1, 2, 3]
        it = iter(a)
        testfunc(it)
        a.append(4)
        with self.assertRaises(StopIteration):
            next(it)

    def test_call_py_exact_args(self):
        def testfunc(n):
            def dummy(x):
                return x+1
            for i in range(n):
                dummy(i)

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_PUSH_FRAME", uops)
        self.assertIn("_BINARY_OP_ADD_INT", uops)

    def test_call_py_ex(self):
        def testfunc(n):
            def ex_py(*args, **kwargs):
                return 1

            for _ in range(n):
                args = (1, 2, 3)
                kwargs = {}
                ex_py(*args, **kwargs)

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_PUSH_FRAME", uops)
        self.assertIn("_PY_FRAME_EX", uops)

    def test_branch_taken(self):
        def testfunc(n):
            for i in range(n):
                if i < 0:
                    i = 0
                else:
                    i = 1

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn(self.guard_is_false, uops)

    def test_branch_coincident_targets(self):
        # test for gh-144681: https://github.com/python/cpython/issues/144681
        def testfunc(n):
            for _ in range(n):
                r = [x for x in range(10) if [].append(x) or True]
            return r

        res = testfunc(TIER2_THRESHOLD)
        ex = get_first_executor(testfunc)

        self.assertEqual(res, list(range(10)))
        self.assertIsNotNone(ex)

    def test_for_iter_tier_two(self):
        class MyIter:
            def __init__(self, n):
                self.n = n
            def __iter__(self):
                return self
            def __next__(self):
                self.n -= 1
                if self.n < 0:
                    raise StopIteration
                return self.n

        def testfunc(n, m):
            x = 0
            for i in range(m):
                for j in MyIter(n):
                    x += j
            return x

        x = testfunc(TIER2_THRESHOLD, 2)

        self.assertEqual(x, sum(range(TIER2_THRESHOLD)) * 2)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_FOR_ITER_TIER_TWO", uops)


@requires_specialization
@unittest.skipIf(Py_GIL_DISABLED, "optimizer not yet supported in free-threaded builds")
@requires_jit_enabled
@unittest.skipIf(os.getenv("PYTHON_UOPS_OPTIMIZE") == "0", "Needs uop optimizer to run.")
class TestUopsOptimization(unittest.TestCase):

    def setUp(self):
        self.guard_is_false, self.guard_is_true = get_bool_guard_ops()

    def _run_with_optimizer(self, testfunc, arg):
        res = testfunc(arg)

        ex = get_first_executor(testfunc)
        return res, ex


    def test_int_type_propagation(self):
        def testfunc(loops):
            num = 0
            for i in range(loops):
                x = num + num
                a = x + 1
                num += 1
            return a

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        self.assertEqual(res, (TIER2_THRESHOLD - 1) * 2 + 1)
        binop_count = [opname for opname in iter_opnames(ex) if opname == "_BINARY_OP_ADD_INT"]
        guard_tos_int_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_TOS_INT"]
        guard_nos_int_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_NOS_INT"]
        self.assertGreaterEqual(len(binop_count), 3)
        self.assertLessEqual(len(guard_tos_int_count), 1)
        self.assertLessEqual(len(guard_nos_int_count), 1)

    def test_int_type_propagation_through_frame(self):
        def double(x):
            return x + x
        def testfunc(loops):
            num = 0
            for i in range(loops):
                x = num + num
                a = double(x)
                num += 1
            return a

        res = testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        self.assertEqual(res, (TIER2_THRESHOLD - 1) * 4)
        binop_count = [opname for opname in iter_opnames(ex) if opname == "_BINARY_OP_ADD_INT"]
        guard_tos_int_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_TOS_INT"]
        guard_nos_int_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_NOS_INT"]
        self.assertGreaterEqual(len(binop_count), 3)
        self.assertLessEqual(len(guard_tos_int_count), 1)
        self.assertLessEqual(len(guard_nos_int_count), 1)

    def test_int_type_propagation_from_frame(self):
        def double(x):
            return x + x
        def testfunc(loops):
            num = 0
            for i in range(loops):
                a = double(num)
                x = a + a
                num += 1
            return x

        res = testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        self.assertEqual(res, (TIER2_THRESHOLD - 1) * 4)
        binop_count = [opname for opname in iter_opnames(ex) if opname == "_BINARY_OP_ADD_INT"]
        guard_tos_int_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_TOS_INT"]
        guard_nos_int_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_NOS_INT"]
        self.assertGreaterEqual(len(binop_count), 3)
        self.assertLessEqual(len(guard_tos_int_count), 1)
        self.assertLessEqual(len(guard_nos_int_count), 1)

    def test_int_impure_region(self):
        def testfunc(loops):
            num = 0
            while num < loops:
                x = num + num
                y = 1
                x // 2
                a = x + y
                num += 1
            return a

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        binop_count = [opname for opname in iter_opnames(ex) if opname == "_BINARY_OP_ADD_INT"]
        self.assertGreaterEqual(len(binop_count), 3)

    def test_call_py_exact_args(self):
        def testfunc(n):
            def dummy(x):
                return x+1
            for i in range(n):
                dummy(i)

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_PUSH_FRAME", uops)
        self.assertIn("_BINARY_OP_ADD_INT", uops)
        self.assertNotIn("_CHECK_PEP_523", uops)
        self.assertNotIn("_GUARD_CODE_VERSION__PUSH_FRAME", uops)
        self.assertNotIn("_GUARD_IP__PUSH_FRAME", uops)

    def test_int_type_propagate_through_range(self):
        def testfunc(n):

            for i in range(n):
                x = i + i
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, (TIER2_THRESHOLD - 1) * 2)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_GUARD_TOS_INT", uops)
        self.assertNotIn("_GUARD_NOS_INT", uops)

    def test_int_value_numbering(self):
        def testfunc(n):

            y = 1
            for i in range(n):
                x = y
                z = x
                a = z
                b = a
                res = x + z + a + b
            return res

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 4)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_GUARD_TOS_INT", uops)
        self.assertNotIn("_GUARD_NOS_INT", uops)
        guard_tos_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_TOS_INT"]
        self.assertEqual(len(guard_tos_count), 1)

    def test_comprehension(self):
        def testfunc(n):
            for _ in range(n):
                return [i for i in range(n)]

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, list(range(TIER2_THRESHOLD)))
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_BINARY_OP_ADD_INT", uops)

    def test_call_py_exact_args_disappearing(self):
        def dummy(x):
            return x+1

        def testfunc(n):
            for i in range(n):
                dummy(i)

        # Trigger specialization
        testfunc(8)
        del dummy
        gc.collect()

        def dummy(x):
            return x + 2
        testfunc(32)

        ex = get_first_executor(testfunc)
        # Honestly as long as it doesn't crash it's fine.
        # Whether we get an executor or not is non-deterministic,
        # because it's decided by when the function is freed.
        # This test is a little implementation specific.

    def test_promote_globals_to_constants(self):

        result = script_helper.run_python_until_end('-c', textwrap.dedent("""
        import _testinternalcapi
        import opcode
        import _opcode

        def get_first_executor(func):
            code = func.__code__
            co_code = code.co_code
            for i in range(0, len(co_code), 2):
                try:
                    return _opcode.get_executor(code, i)
                except ValueError:
                    pass
            return None

        def get_opnames(ex):
            return {item[0] for item in ex}

        def testfunc(n):
            for i in range(n):
                x = range(i)
            return x

        testfunc(_testinternalcapi.TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        assert ex is not None
        uops = get_opnames(ex)
        assert "_LOAD_GLOBAL_BUILTINS" not in uops
        assert "_LOAD_CONST_INLINE_BORROW" in uops
        """), PYTHON_JIT="1")
        self.assertEqual(result[0].rc, 0, result)

    def test_float_add_constant_propagation(self):
        def testfunc(n):
            a = 1.0
            for _ in range(n):
                a = a + 0.25
                a = a + 0.25
                a = a + 0.25
                a = a + 0.25
            return a

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertAlmostEqual(res, TIER2_THRESHOLD + 1)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        guard_tos_float_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_TOS_FLOAT"]
        guard_nos_float_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_NOS_FLOAT"]
        self.assertLessEqual(len(guard_tos_float_count), 1)
        self.assertLessEqual(len(guard_nos_float_count), 1)
        # TODO gh-115506: this assertion may change after propagating constants.
        # We'll also need to verify that propagation actually occurs.
        self.assertIn("_POP_TOP_NOP", uops)

    def test_float_subtract_constant_propagation(self):
        def testfunc(n):
            a = 1.0
            for _ in range(n):
                a = a - 0.25
                a = a - 0.25
                a = a - 0.25
                a = a - 0.25
            return a

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertAlmostEqual(res, -TIER2_THRESHOLD + 1)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        guard_tos_float_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_TOS_FLOAT"]
        guard_nos_float_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_NOS_FLOAT"]
        self.assertLessEqual(len(guard_tos_float_count), 1)
        self.assertLessEqual(len(guard_nos_float_count), 1)
        # TODO gh-115506: this assertion may change after propagating constants.
        # We'll also need to verify that propagation actually occurs.
        self.assertIn("_POP_TOP_NOP", uops)

    def test_float_multiply_constant_propagation(self):
        def testfunc(n):
            a = 1.0
            for _ in range(n):
                a = a * 1.0
                a = a * 1.0
                a = a * 1.0
                a = a * 1.0
            return a

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertAlmostEqual(res, 1.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        guard_tos_float_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_TOS_FLOAT"]
        guard_nos_float_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_NOS_FLOAT"]
        self.assertLessEqual(len(guard_tos_float_count), 1)
        self.assertLessEqual(len(guard_nos_float_count), 1)
        # TODO gh-115506: this assertion may change after propagating constants.
        # We'll also need to verify that propagation actually occurs.
        self.assertIn("_POP_TOP_NOP", uops)

    def test_add_unicode_propagation(self):
        def testfunc(n):
            a = ""
            for _ in range(n):
                a + a
                a + a
                a + a
                a + a
            return a

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, "")
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        guard_tos_unicode_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_TOS_UNICODE"]
        guard_nos_unicode_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_NOS_UNICODE"]
        self.assertLessEqual(len(guard_tos_unicode_count), 1)
        self.assertLessEqual(len(guard_nos_unicode_count), 1)
        self.assertIn("_BINARY_OP_ADD_UNICODE", uops)

    def test_compare_op_type_propagation_float(self):
        def testfunc(n):
            a = 1.0
            for _ in range(n):
                x = a == a
                x = a == a
                x = a == a
                x = a == a
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertTrue(res)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        guard_tos_float_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_TOS_FLOAT"]
        guard_nos_float_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_NOS_FLOAT"]
        self.assertLessEqual(len(guard_tos_float_count), 1)
        self.assertLessEqual(len(guard_nos_float_count), 1)
        self.assertIn("_COMPARE_OP_FLOAT", uops)

    def test_compare_op_type_propagation_int(self):
        def testfunc(n):
            a = 1
            for _ in range(n):
                x = a == a
                x = a == a
                x = a == a
                x = a == a
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertTrue(res)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        guard_tos_int_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_TOS_INT"]
        guard_nos_int_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_NOS_INT"]
        self.assertLessEqual(len(guard_tos_int_count), 1)
        self.assertLessEqual(len(guard_nos_int_count), 1)
        self.assertIn("_COMPARE_OP_INT", uops)

    def test_compare_op_type_propagation_int_partial(self):
        def testfunc(n):
            a = 1
            for _ in range(n):
                if a > 2:
                    x = 0
                if a < 2:
                    x = 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 1)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        guard_nos_int_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_NOS_INT"]
        guard_tos_int_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_TOS_INT"]
        self.assertLessEqual(len(guard_nos_int_count), 1)
        self.assertEqual(len(guard_tos_int_count), 0)
        self.assertIn("_COMPARE_OP_INT", uops)

    def test_compare_op_type_propagation_float_partial(self):
        def testfunc(n):
            a = 1.0
            for _ in range(n):
                if a > 2.0:
                    x = 0
                if a < 2.0:
                    x = 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 1)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        guard_nos_float_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_NOS_FLOAT"]
        guard_tos_float_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_TOS_FLOAT"]
        self.assertLessEqual(len(guard_nos_float_count), 1)
        self.assertEqual(len(guard_tos_float_count), 0)
        self.assertIn("_COMPARE_OP_FLOAT", uops)

    def test_compare_op_type_propagation_unicode(self):
        def testfunc(n):
            a = ""
            for _ in range(n):
                x = a == a
                x = a == a
                x = a == a
                x = a == a
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertTrue(res)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        guard_tos_unicode_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_TOS_UNICODE"]
        guard_nos_unicode_count = [opname for opname in iter_opnames(ex) if opname == "_GUARD_NOS_UNICODE"]
        self.assertLessEqual(len(guard_tos_unicode_count), 1)
        self.assertLessEqual(len(guard_nos_unicode_count), 1)
        self.assertIn("_COMPARE_OP_STR", uops)

    def test_compare_int_eq_narrows_to_constant(self):
        def f(n):
            def return_1():
                return 1

            hits = 0
            v = return_1()
            for _ in range(n):
                if v == 1:
                    if v == 1:
                        hits += 1
            return hits

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        # Constant narrowing allows constant folding for second comparison
        self.assertLessEqual(count_ops(ex, "_COMPARE_OP_INT"), 1)

    def test_compare_int_ne_narrows_to_constant(self):
        def f(n):
            def return_1():
                return 1

            hits = 0
            v = return_1()
            for _ in range(n):
                if v != 1:
                    hits += 1000
                else:
                    if v == 1:
                        hits += v + 1
            return hits

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 2)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        # Constant narrowing allows constant folding for second comparison
        self.assertLessEqual(count_ops(ex, "_COMPARE_OP_INT"), 1)

    def test_compare_float_eq_narrows_to_constant(self):
        def f(n):
            def return_tenth():
                return 0.1

            hits = 0
            v = return_tenth()
            for _ in range(n):
                if v == 0.1:
                    if v == 0.1:
                        hits += 1
            return hits

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        # Constant narrowing allows constant folding for second comparison
        self.assertLessEqual(count_ops(ex, "_COMPARE_OP_FLOAT"), 1)

    def test_compare_float_ne_narrows_to_constant(self):
        def f(n):
            def return_tenth():
                return 0.1

            hits = 0
            v = return_tenth()
            for _ in range(n):
                if v != 0.1:
                    hits += 1000
                else:
                    if v == 0.1:
                        hits += 1
            return hits

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        # Constant narrowing allows constant folding for second comparison
        self.assertLessEqual(count_ops(ex, "_COMPARE_OP_FLOAT"), 1)

    def test_combine_stack_space_checks_sequential(self):
        def dummy12(x):
            return x - 1
        def dummy13(y):
            z = y + 2
            return y, z
        def testfunc(n):
            a = 0
            for _ in range(n):
                b = dummy12(7)
                c, d = dummy13(9)
                a += b + c + d
            return a

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 26)
        self.assertIsNotNone(ex)

        uops_and_operands = [(opcode, operand) for opcode, _, _, operand in ex]
        uop_names = [uop[0] for uop in uops_and_operands]
        self.assertEqual(uop_names.count("_PUSH_FRAME"), 2)
        self.assertEqual(uop_names.count("_RETURN_VALUE"), 2)
        self.assertEqual(uop_names.count("_CHECK_STACK_SPACE"), 0)
        # Each call gets its own _CHECK_STACK_SPACE_OPERAND
        self.assertEqual(uop_names.count("_CHECK_STACK_SPACE_OPERAND"), 2)
        # Each _CHECK_STACK_SPACE_OPERAND has the framesize of its function
        self.assertIn(("_CHECK_STACK_SPACE_OPERAND",
                       _testinternalcapi.get_co_framesize(dummy12.__code__)), uops_and_operands)
        self.assertIn(("_CHECK_STACK_SPACE_OPERAND",
                       _testinternalcapi.get_co_framesize(dummy13.__code__)), uops_and_operands)

    def test_combine_stack_space_checks_nested(self):
        def dummy12(x):
            return x + 3
        def dummy15(y):
            z = dummy12(y)
            return y, z
        def testfunc(n):
            a = 0
            for _ in range(n):
                b, c = dummy15(2)
                a += b + c
            return a

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 7)
        self.assertIsNotNone(ex)

        uops_and_operands = [(opcode, operand) for opcode, _, _, operand in ex]
        uop_names = [uop[0] for uop in uops_and_operands]
        self.assertEqual(uop_names.count("_PUSH_FRAME"), 2)
        self.assertEqual(uop_names.count("_RETURN_VALUE"), 2)
        self.assertEqual(uop_names.count("_CHECK_STACK_SPACE"), 0)
        self.assertEqual(uop_names.count("_CHECK_STACK_SPACE_OPERAND"), 2)
        self.assertIn(("_CHECK_STACK_SPACE_OPERAND",
                       _testinternalcapi.get_co_framesize(dummy15.__code__)), uops_and_operands)
        self.assertIn(("_CHECK_STACK_SPACE_OPERAND",
                       _testinternalcapi.get_co_framesize(dummy12.__code__)), uops_and_operands)

    def test_combine_stack_space_checks_several_calls(self):
        def dummy12(x):
            return x + 3
        def dummy13(y):
            z = y + 2
            return y, z
        def dummy18(y):
            z = dummy12(y)
            x, w = dummy13(z)
            return z, x, w
        def testfunc(n):
            a = 0
            for _ in range(n):
                b = dummy12(5)
                c, d, e = dummy18(2)
                a += b + c + d + e
            return a

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 25)
        self.assertIsNotNone(ex)

        uops_and_operands = [(opcode, operand) for opcode, _, _, operand in ex]
        uop_names = [uop[0] for uop in uops_and_operands]
        self.assertEqual(uop_names.count("_PUSH_FRAME"), 4)
        self.assertEqual(uop_names.count("_RETURN_VALUE"), 4)
        self.assertEqual(uop_names.count("_CHECK_STACK_SPACE"), 0)
        self.assertEqual(uop_names.count("_CHECK_STACK_SPACE_OPERAND"), 4)
        self.assertIn(("_CHECK_STACK_SPACE_OPERAND",
                       _testinternalcapi.get_co_framesize(dummy12.__code__)), uops_and_operands)
        self.assertIn(("_CHECK_STACK_SPACE_OPERAND",
                       _testinternalcapi.get_co_framesize(dummy13.__code__)), uops_and_operands)
        self.assertIn(("_CHECK_STACK_SPACE_OPERAND",
                       _testinternalcapi.get_co_framesize(dummy18.__code__)), uops_and_operands)

    def test_combine_stack_space_checks_several_calls_different_order(self):
        # same as `several_calls` but with top-level calls reversed
        def dummy12(x):
            return x + 3
        def dummy13(y):
            z = y + 2
            return y, z
        def dummy18(y):
            z = dummy12(y)
            x, w = dummy13(z)
            return z, x, w
        def testfunc(n):
            a = 0
            for _ in range(n):
                c, d, e = dummy18(2)
                b = dummy12(5)
                a += b + c + d + e
            return a

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 25)
        self.assertIsNotNone(ex)

        uops_and_operands = [(opcode, operand) for opcode, _, _, operand in ex]
        uop_names = [uop[0] for uop in uops_and_operands]
        self.assertEqual(uop_names.count("_PUSH_FRAME"), 4)
        self.assertEqual(uop_names.count("_RETURN_VALUE"), 4)
        self.assertEqual(uop_names.count("_CHECK_STACK_SPACE"), 0)
        self.assertEqual(uop_names.count("_CHECK_STACK_SPACE_OPERAND"), 4)
        self.assertIn(("_CHECK_STACK_SPACE_OPERAND",
                       _testinternalcapi.get_co_framesize(dummy12.__code__)), uops_and_operands)
        self.assertIn(("_CHECK_STACK_SPACE_OPERAND",
                       _testinternalcapi.get_co_framesize(dummy13.__code__)), uops_and_operands)
        self.assertIn(("_CHECK_STACK_SPACE_OPERAND",
                       _testinternalcapi.get_co_framesize(dummy18.__code__)), uops_and_operands)

    @unittest.skip("reopen when we combine multiple stack space checks into one")
    def test_combine_stack_space_complex(self):
        def dummy0(x):
            return x
        def dummy1(x):
            return dummy0(x)
        def dummy2(x):
            return dummy1(x)
        def dummy3(x):
            return dummy0(x)
        def dummy4(x):
            y = dummy0(x)
            return dummy3(y)
        def dummy5(x):
            return dummy2(x)
        def dummy6(x):
            y = dummy5(x)
            z = dummy0(y)
            return dummy4(z)
        def testfunc(n):
            a = 0
            for _ in range(n):
                b = dummy5(1)
                c = dummy0(1)
                d = dummy6(1)
                a += b + c + d
            return a

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 3)
        self.assertIsNotNone(ex)

        uops_and_operands = [(opcode, operand) for opcode, _, _, operand in ex]
        uop_names = [uop[0] for uop in uops_and_operands]
        self.assertEqual(uop_names.count("_PUSH_FRAME"), 15)
        self.assertEqual(uop_names.count("_RETURN_VALUE"), 15)

        self.assertEqual(uop_names.count("_CHECK_STACK_SPACE"), 0)
        self.assertEqual(uop_names.count("_CHECK_STACK_SPACE_OPERAND"), 1)
        largest_stack = (
            _testinternalcapi.get_co_framesize(dummy6.__code__) +
            _testinternalcapi.get_co_framesize(dummy5.__code__) +
            _testinternalcapi.get_co_framesize(dummy2.__code__) +
            _testinternalcapi.get_co_framesize(dummy1.__code__) +
            _testinternalcapi.get_co_framesize(dummy0.__code__)
        )
        self.assertIn(
            ("_CHECK_STACK_SPACE_OPERAND", largest_stack), uops_and_operands
        )

    @unittest.skip("reopen when we combine multiple stack space checks into one")
    def test_combine_stack_space_checks_large_framesize(self):
        # Create a function with a large framesize. This ensures _CHECK_STACK_SPACE is
        # actually doing its job. Note that the resulting trace hits
        # UOP_MAX_TRACE_LENGTH, but since all _CHECK_STACK_SPACEs happen early, this
        # test is still meaningful.
        repetitions = 10000
        ns = {}
        header = """
            def dummy_large(a0):
        """
        body = "".join([f"""
                a{n+1} = a{n} + 1
        """ for n in range(repetitions)])
        return_ = f"""
                return a{repetitions-1}
        """
        exec(textwrap.dedent(header + body + return_), ns, ns)
        dummy_large = ns['dummy_large']

        # this is something like:
        #
        # def dummy_large(a0):
        #     a1 = a0 + 1
        #     a2 = a1 + 1
        #     ....
        #     a9999 = a9998 + 1
        #     return a9999

        def dummy15(z):
            y = dummy_large(z)
            return y + 3

        def testfunc(n):
            b = 0
            for _ in range(n):
                b += dummy15(7)
            return b

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * (repetitions + 9))
        self.assertIsNotNone(ex)

        uops_and_operands = [(opcode, operand) for opcode, _, _, operand in ex]
        uop_names = [uop[0] for uop in uops_and_operands]
        self.assertEqual(uop_names.count("_PUSH_FRAME"), 2)
        self.assertEqual(uop_names.count("_CHECK_STACK_SPACE_OPERAND"), 1)

        # this hits a different case during trace projection in refcount test runs only,
        # so we need to account for both possibilities
        self.assertIn(uop_names.count("_CHECK_STACK_SPACE"), [0, 1])
        if uop_names.count("_CHECK_STACK_SPACE") == 0:
            largest_stack = (
                _testinternalcapi.get_co_framesize(dummy15.__code__) +
                _testinternalcapi.get_co_framesize(dummy_large.__code__)
            )
        else:
            largest_stack = _testinternalcapi.get_co_framesize(dummy15.__code__)
        self.assertIn(
            ("_CHECK_STACK_SPACE_OPERAND", largest_stack), uops_and_operands
        )

    @unittest.skip("reopen when we combine multiple stack space checks into one")
    def test_combine_stack_space_checks_recursion(self):
        def dummy15(x):
            while x > 0:
                return dummy15(x - 1)
            return 42
        def testfunc(n):
            a = 0
            for _ in range(n):
                a += dummy15(n)
            return a

        recursion_limit = sys.getrecursionlimit()
        try:
            sys.setrecursionlimit(TIER2_THRESHOLD + recursion_limit)
            res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        finally:
            sys.setrecursionlimit(recursion_limit)
        self.assertEqual(res, TIER2_THRESHOLD * 42)
        self.assertIsNotNone(ex)

        uops_and_operands = [(opcode, operand) for opcode, _, _, operand in ex]
        uop_names = [uop[0] for uop in uops_and_operands]
        self.assertEqual(uop_names.count("_PUSH_FRAME"), 2)
        self.assertEqual(uop_names.count("_RETURN_VALUE"), 0)
        self.assertEqual(uop_names.count("_CHECK_STACK_SPACE"), 1)
        self.assertEqual(uop_names.count("_CHECK_STACK_SPACE_OPERAND"), 1)
        largest_stack = _testinternalcapi.get_co_framesize(dummy15.__code__)
        self.assertIn(("_CHECK_STACK_SPACE_OPERAND", largest_stack), uops_and_operands)

    def test_many_nested(self):
        # overflow the trace_stack
        def dummy_a(x):
            return x
        def dummy_b(x):
            return dummy_a(x)
        def dummy_c(x):
            return dummy_b(x)
        def dummy_d(x):
            return dummy_c(x)
        def dummy_e(x):
            return dummy_d(x)
        def dummy_f(x):
            return dummy_e(x)
        def dummy_g(x):
            return dummy_f(x)
        def dummy_h(x):
            return dummy_g(x)
        def testfunc(n):
            a = 0
            for _ in range(n):
                a += dummy_h(n)
            return a

        res, ex = self._run_with_optimizer(testfunc, 32)
        self.assertEqual(res, 32 * 32)
        self.assertIsNone(ex)

    def test_return_generator(self):
        def gen():
            yield None
        def testfunc(n):
            for i in range(n):
                gen()
            return i
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD - 1)
        self.assertIsNotNone(ex)
        self.assertIn("_RETURN_GENERATOR", get_opnames(ex))

    def test_make_heap_safe_optimized_immortal(self):
        def returns_immortal():
            return None
        def testfunc(n):
            a = 0
            for _ in range(n):
                a = returns_immortal()
            return a
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNone(res)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_MAKE_HEAP_SAFE", uops)
        self.assertIn("_RETURN_VALUE", uops)

    def test_make_heap_safe_optimized_yield(self):
        def gen(n):
            for _ in range(n):
                yield 1
        def testfunc(n):
            for _ in gen(n):
                pass
        testfunc(TIER2_THRESHOLD * 2)
        # The generator may be inlined into testfunc's trace,
        # so check whichever executor contains _YIELD_VALUE.
        gen_ex = get_first_executor(gen)
        testfunc_ex = get_first_executor(testfunc)
        ex = gen_ex or testfunc_ex
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_MAKE_HEAP_SAFE", uops)
        self.assertIn("_YIELD_VALUE", uops)

    def test_make_heap_safe_not_optimized_for_owned(self):
        def returns_owned(x):
            return x + 1
        def testfunc(n):
            a = 0
            for _ in range(n):
                a = returns_owned(a)
            return a
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_MAKE_HEAP_SAFE", uops)
        self.assertIn("_RETURN_VALUE", uops)

    def test_for_iter(self):
        def testfunc(n):
            t = 0
            for i in set(range(n)):
                t += i
            return t
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * (TIER2_THRESHOLD - 1) // 2)
        self.assertIsNotNone(ex)
        self.assertIn("_FOR_ITER_TIER_TWO", get_opnames(ex))

    def test_modified_local_is_seen_by_optimized_code(self):
        l = sys._getframe().f_locals
        a = 1
        s = 0
        for j in range(1 << 10):
            a + a
            l["xa"[j >> 9]] = 1.0
            s += a
        self.assertIs(type(a), float)
        self.assertIs(type(s), float)
        self.assertEqual(s, 1024.0)

    def test_guard_type_version_removed(self):
        def thing(a):
            x = 0
            for _ in range(TIER2_THRESHOLD):
                x += a.attr
                x += a.attr
            return x

        class Foo:
            attr = 1

        res, ex = self._run_with_optimizer(thing, Foo())
        opnames = list(iter_opnames(ex))
        self.assertIsNotNone(ex)
        self.assertEqual(res, TIER2_THRESHOLD * 2)
        guard_type_version_count = opnames.count("_GUARD_TYPE_VERSION")
        self.assertEqual(guard_type_version_count, 1)

    def test_guard_type_version_removed_inlined(self):
        """
        Verify that the guard type version if we have an inlined function
        """

        def fn():
            pass

        def thing(a):
            x = 0
            for _ in range(TIER2_THRESHOLD):
                x += a.attr
                fn()
                x += a.attr
            return x

        class Foo:
            attr = 1

        res, ex = self._run_with_optimizer(thing, Foo())
        opnames = list(iter_opnames(ex))
        self.assertIsNotNone(ex)
        self.assertEqual(res, TIER2_THRESHOLD * 2)
        guard_type_version_count = opnames.count("_GUARD_TYPE_VERSION")
        self.assertEqual(guard_type_version_count, 1)

    def test_guard_type_version_removed_invalidation(self):

        def thing(a):
            x = 0
            for i in range(TIER2_THRESHOLD + 1):
                x += a.attr
                # The first TIER2_THRESHOLD iterations we set the attribute on
                # this dummy class, which shouldn't trigger the type watcher.
                # Note that the code needs to be in this weird form so it's
                # optimized inline without any control flow:
                setattr((Bar, Foo)[i == TIER2_THRESHOLD + 1], "attr", 2)
                x += a.attr
            return x

        class Foo:
            attr = 1

        class Bar:
            pass

        res, ex = self._run_with_optimizer(thing, Foo())
        opnames = list(iter_opnames(ex))
        self.assertEqual(res, TIER2_THRESHOLD * 2 + 2)
        call = opnames.index("_CALL_BUILTIN_FAST")
        load_attr_top = opnames.index("_LOAD_CONST_INLINE_BORROW", 0, call)
        load_attr_bottom = opnames.index("_LOAD_CONST_INLINE_BORROW", call)
        self.assertEqual(opnames[:load_attr_top].count("_GUARD_TYPE_VERSION"), 1)
        self.assertEqual(opnames[call:load_attr_bottom].count("_CHECK_VALIDITY"), 2)

    def test_guard_type_version_removed_escaping(self):

        def thing(a):
            x = 0
            for i in range(TIER2_THRESHOLD):
                x += a.attr
                # eval should be escaping
                eval("None")
                x += a.attr
            return x

        class Foo:
            attr = 1
        res, ex = self._run_with_optimizer(thing, Foo())
        opnames = list(iter_opnames(ex))
        self.assertIsNotNone(ex)
        self.assertEqual(res, TIER2_THRESHOLD * 2)
        call = opnames.index("_CALL_BUILTIN_FAST_WITH_KEYWORDS")
        load_attr_top = opnames.index("_LOAD_CONST_INLINE_BORROW", 0, call)
        load_attr_bottom = opnames.index("_LOAD_CONST_INLINE_BORROW", call)
        self.assertEqual(opnames[:load_attr_top].count("_GUARD_TYPE_VERSION"), 1)
        self.assertEqual(opnames[call:load_attr_bottom].count("_CHECK_VALIDITY"), 2)

    def test_guard_type_version_executor_invalidated(self):
        """
        Verify that the executor is invalided on a type change.
        """

        def thing(a):
            x = 0
            for i in range(TIER2_THRESHOLD):
                x += a.attr
                x += a.attr
            return x

        class Foo:
            attr = 1

        res, ex = self._run_with_optimizer(thing, Foo())
        self.assertEqual(res, TIER2_THRESHOLD * 2)
        self.assertIsNotNone(ex)
        self.assertEqual(list(iter_opnames(ex)).count("_GUARD_TYPE_VERSION"), 1)
        self.assertTrue(ex.is_valid())
        Foo.attr = 0
        self.assertFalse(ex.is_valid())

    def test_guard_type_version_locked_removed(self):
        """
        Verify that redundant _GUARD_TYPE_VERSION_LOCKED guards are
        eliminated for sequential STORE_ATTR_INSTANCE_VALUE in __init__.
        """

        class Foo:
            def __init__(self):
                self.a = 1
                self.b = 2
                self.c = 3

        def thing(n):
            for _ in range(n):
                Foo()

        res, ex = self._run_with_optimizer(thing, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        opnames = list(iter_opnames(ex))
        guard_locked_count = opnames.count("_GUARD_TYPE_VERSION_LOCKED")
        # Only the first store needs the guard; the rest should be NOPed.
        self.assertEqual(guard_locked_count, 1)

    def test_type_version_doesnt_segfault(self):
        """
        Tests that setting a type version doesn't cause a segfault when later looking at the stack.
        """

        # Minimized from mdp.py benchmark

        class A:
            def __init__(self):
                self.attr = {}

            def method(self, arg):
                self.attr[arg] = None

        def fn(a):
            for _ in range(100):
                (_ for _ in [])
                (_ for _ in [a.method(None)])

        fn(A())

    def test_init_resolves_callable(self):
        """
        _CHECK_AND_ALLOCATE_OBJECT should resolve __init__ to a constant,
        enabling the optimizer to propagate type information through the frame
        and eliminate redundant function version and arg count checks.
        """
        class MyPoint:
            def __init__(self, x, y):
                # If __init__ callable is propagated through, then
                # These will get promoted from globals to constants.
                self.x = range(1)
                self.y = range(1)

        def testfunc(n):
            for _ in range(n):
                p = MyPoint(1.0, 2.0)

        _, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # The __init__ call should be traced through via _PUSH_FRAME
        self.assertIn("_PUSH_FRAME", uops)
        # __init__ resolution allows promotion of range to constant
        self.assertNotIn("_LOAD_GLOBAL_BUILTINS", uops)

    def test_init_guards_removed(self):
        class MyPoint:
            def __init__(self, x, y):
                return None

        def testfunc(n):
            point_local = MyPoint
            for _ in range(n):
                p = point_local(1.0, 2.0)
                p = point_local(1.0, 2.0)

        _, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        # The __init__ call should be traced through via _PUSH_FRAME
        count = count_ops(ex, "_CREATE_INIT_FRAME")
        self.assertEqual(count, 2)
        # __init__ resolution allows promotion of range to constant
        count = count_ops(ex, "_CHECK_OBJECT")
        self.assertEqual(count, 1)

    def test_init_guards_removed_global(self):

        def testfunc(n):
            for _ in range(n):
                p = MyGlobalPoint(1.0, 2.0)
                p = MyGlobalPoint(1.0, 2.0)

        _, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        # The __init__ call should be traced through via _PUSH_FRAME
        count = count_ops(ex, "_CREATE_INIT_FRAME")
        self.assertEqual(count, 2)
        # __init__ resolution allows promotion of range to constant
        count = count_ops(ex, "_CHECK_OBJECT")
        self.assertEqual(count, 0)

    def test_guard_type_version_locked_propagates(self):
        """
        _GUARD_TYPE_VERSION_LOCKED should set the type version on the
        symbol so repeated accesses to the same type can benefit.
        """
        class Item:
            def __init__(self, val):
                self.val = val

            def get(self):
                return self.val

            def get2(self):
                return self.val + 1

        def testfunc(n):
            item = Item(42)
            total = 0
            for _ in range(n):
                # Two method calls on the same object — the second
                # should benefit from type info set by the first.
                total += item.get() + item.get2()
            return total

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * (42 + 43))
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # Both methods should be traced through
        self.assertEqual(uops.count("_PUSH_FRAME"), 2)
        # Type version propagation: one guard covers both method lookups
        self.assertEqual(uops.count("_GUARD_TYPE_VERSION"), 1)
        # Function checks cannot be eliminated for safety reasons.
        self.assertIn("_CHECK_FUNCTION_VERSION", uops)

    def test_method_chain_guard_elimination(self):
        """
        Calling two methods on the same object should share the outer
        type guard — only one _GUARD_TYPE_VERSION for the two lookups.
        """
        class Calc:
            def __init__(self, val):
                self.val = val

            def add(self, x):
                self.val += x
                return self

        def testfunc(n):
            c = Calc(0)
            for _ in range(n):
                c.add(1).add(2)
            return c.val

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 3)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # Both add() calls should be inlined
        push_count = uops.count("_PUSH_FRAME")
        self.assertEqual(push_count, 2)
        # Only one outer type version guard for the two method lookups
        # on the same object c (the second lookup reuses type info)
        guard_version_count = uops.count("_GUARD_TYPE_VERSION")
        self.assertEqual(guard_version_count, 1)

    def test_func_guards_removed_or_reduced(self):
        def testfunc(n):
            for i in range(n):
                # Only works on functions promoted to constants
                global_identity(i)

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_PUSH_FRAME", uops)
        self.assertIn("_CHECK_FUNCTION_VERSION", uops)
        # Removed guard
        self.assertNotIn("_CHECK_FUNCTION_EXACT_ARGS", uops)

    def test_method_guards_removed_or_reduced(self):
        def testfunc(n):
            result = 0
            for i in range(n):
                result += test_bound_method(i)
            return result
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, sum(range(TIER2_THRESHOLD)))
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_PUSH_FRAME", uops)
        # Strength reduced version
        self.assertIn("_CHECK_FUNCTION_VERSION_INLINE", uops)
        self.assertNotIn("_CHECK_METHOD_VERSION", uops)

    def test_record_bound_method_general(self):
        class MyClass:
            def method(self, *args):
                return args[0] + 1

        def testfunc(n):
            obj = MyClass()
            bound = obj.method
            result = 0
            for i in range(n):
                result += bound(i)
            return result

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(
            res, sum(i + 1 for i in range(TIER2_THRESHOLD))
        )
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_PUSH_FRAME", uops)

    def test_record_bound_method_exact_args(self):
        class MyClass:
            def method(self, x):
                return x + 1

        def testfunc(n):
            obj = MyClass()
            bound = obj.method
            result = 0
            for i in range(n):
                result += bound(i)
            return result

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(
            res, sum(i + 1 for i in range(TIER2_THRESHOLD))
        )
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_PUSH_FRAME", uops)
        self.assertNotIn("_CHECK_FUNCTION_EXACT_ARGS", uops)

    def test_jit_error_pops(self):
        """
        Tests that the correct number of pops are inserted into the
        exit stub
        """
        items = 17 * [None] + [[]]
        with self.assertRaises(TypeError):
            {item for item in items}

    def test_power_type_depends_on_input_values(self):
        template = textwrap.dedent("""
            import _testinternalcapi

            L, R, X, Y = {l}, {r}, {x}, {y}

            def check(actual: complex, expected: complex) -> None:
                assert actual == expected, (actual, expected)
                assert type(actual) is type(expected), (actual, expected)

            def f(l: complex, r: complex) -> None:
                expected_local_local = pow(l, r) + pow(l, r)
                expected_const_local = pow(L, r) + pow(L, r)
                expected_local_const = pow(l, R) + pow(l, R)
                expected_const_const = pow(L, R) + pow(L, R)
                for _ in range(_testinternalcapi.TIER2_THRESHOLD):
                    # Narrow types:
                    l + l, r + r
                    # The powers produce results, and the addition is unguarded:
                    check(l ** r + l ** r, expected_local_local)
                    check(L ** r + L ** r, expected_const_local)
                    check(l ** R + l ** R, expected_local_const)
                    check(L ** R + L ** R, expected_const_const)

            # JIT for one pair of values...
            f(L, R)
            # ...then run with another:
            f(X, Y)
        """)
        interesting = [
            (1, 1),  # int ** int -> int
            (1, -1),  # int ** int -> float
            (1.0, 1),  # float ** int -> float
            (1, 1.0),  # int ** float -> float
            (-1, 0.5),  # int ** float -> complex
            (1.0, 1.0),  # float ** float -> float
            (-1.0, 0.5),  # float ** float -> complex
        ]
        for (l, r), (x, y) in itertools.product(interesting, repeat=2):
            s = template.format(l=l, r=r, x=x, y=y)
            with self.subTest(l=l, r=r, x=x, y=y):
                script_helper.assert_python_ok("-c", s)

    def test_symbols_flow_through_tuples(self):
        def testfunc(n):
            for _ in range(n):
                a = 1
                b = 2
                t = a, b
                x, y = t
                r = x + y
            return r

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 3)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_BINARY_OP_ADD_INT", uops)
        self.assertNotIn("_GUARD_NOS_INT", uops)
        self.assertNotIn("_GUARD_TOS_INT", uops)

    def test_decref_escapes(self):
        class Convert9999ToNone:
            def __del__(self):
                ns = sys._getframe(1).f_locals
                if ns["i"] == _testinternalcapi.TIER2_THRESHOLD:
                    ns["i"] = None

        def crash_addition():
            try:
                for i in range(_testinternalcapi.TIER2_THRESHOLD + 1):
                    n = Convert9999ToNone()
                    i + i  # Remove guards for i.
                    n = None  # Change i.
                    i + i  # This crashed when we didn't treat DECREF as escaping (gh-124483)
            except TypeError:
                pass

        crash_addition()

    def test_narrow_type_to_constant_bool_false(self):
        def f(n):
            trace = []
            for i in range(n):
                # false is always False, but we can only prove that it's a bool:
                false = i == TIER2_THRESHOLD
                trace.append("A")
                if not false:  # Kept.
                    trace.append("B")
                    if not false:  # Removed!
                        trace.append("C")
                    trace.append("D")
                    if false:  # Removed!
                        trace.append("X")
                    trace.append("E")
                trace.append("F")
                if false:  # Removed!
                    trace.append("X")
                trace.append("G")
            return trace

        trace, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(trace, list("ABCDEFG") * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # Only one guard remains:
        self.assertEqual(uops.count(self.guard_is_false), 1)
        self.assertEqual(uops.count(self.guard_is_true), 0)
        # But all of the appends we care about are still there:
        self.assertEqual(uops.count("_CALL_LIST_APPEND"), len("ABCDEFG"))

    def test_narrow_type_to_constant_bool_true(self):
        def f(n):
            trace = []
            for i in range(n):
                # true always True, but we can only prove that it's a bool:
                true = i != TIER2_THRESHOLD
                trace.append("A")
                if true:  # Kept.
                    trace.append("B")
                    if not true:  # Removed!
                        trace.append("X")
                    trace.append("C")
                    if true:  # Removed!
                        trace.append("D")
                    trace.append("E")
                trace.append("F")
                if not true:  # Removed!
                    trace.append("X")
                trace.append("G")
            return trace

        trace, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(trace, list("ABCDEFG") * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # Only one guard remains:
        self.assertEqual(uops.count(self.guard_is_false), 0)
        self.assertEqual(uops.count(self.guard_is_true), 1)
        # But all of the appends we care about are still there:
        self.assertEqual(uops.count("_CALL_LIST_APPEND"), len("ABCDEFG"))

    def test_narrow_type_to_constant_int_zero(self):
        def f(n):
            trace = []
            for i in range(n):
                # zero is always (int) 0, but we can only prove that it's a integer:
                false = i == TIER2_THRESHOLD # this will always be false, while hopefully still fooling optimizer improvements
                zero = false + 0 # this should always set the variable zero equal to 0
                trace.append("A")
                if not zero:  # Kept.
                    trace.append("B")
                    if not zero:  # Removed!
                        trace.append("C")
                    trace.append("D")
                    if zero:  # Removed!
                        trace.append("X")
                    trace.append("E")
                trace.append("F")
                if zero:  # Removed!
                    trace.append("X")
                trace.append("G")
            return trace

        trace, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(trace, list("ABCDEFG") * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # Only one guard remains:
        self.assertEqual(uops.count(self.guard_is_false), 1)
        self.assertEqual(uops.count(self.guard_is_true), 0)
        # But all of the appends we care about are still there:
        self.assertEqual(uops.count("_CALL_LIST_APPEND"), len("ABCDEFG"))

    def test_narrow_type_to_constant_str_empty(self):
        def f(n):
            trace = []
            for i in range(n):
                # Hopefully the optimizer can't guess what the value is.
                # empty is always "", but we can only prove that it's a string:
                false = i == TIER2_THRESHOLD
                empty = "X"[:false]
                trace.append("A")
                if not empty:  # Kept.
                    trace.append("B")
                    if not empty:  # Removed!
                        trace.append("C")
                    trace.append("D")
                    if empty:  # Removed!
                        trace.append("X")
                    trace.append("E")
                trace.append("F")
                if empty:  # Removed!
                    trace.append("X")
                trace.append("G")
            return trace

        trace, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(trace, list("ABCDEFG") * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # Only one guard remains:
        self.assertEqual(uops.count(self.guard_is_false), 1)
        self.assertEqual(uops.count(self.guard_is_true), 0)
        # But all of the appends we care about are still there:
        self.assertEqual(uops.count("_CALL_LIST_APPEND"), len("ABCDEFG"))

    def test_unary_negative_pop_top_load_const_inline_borrow(self):
        def testfunc(n):
            x = 0
            for i in range(n):
                a = 1
                result = -a
                if result < 0:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_UNARY_NEGATIVE", uops)

    def test_unary_not_pop_top_load_const_inline_borrow(self):
        def testfunc(n):
                x = 0
                for i in range(n):
                    a = 42
                    result = not a
                    if result:
                        x += 1
                return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_UNARY_NOT", uops)
        # TODO (gh-143723): After refactoring TO_BOOL_INT to eliminate redundant
        # refcounts, 'not a' is now constant-folded and currently lowered to
        # _POP_TOP + _LOAD_CONST_INLINE_BORROW. Re-enable once constant folding
        # avoids emitting these.
        # self.assertNotIn("_LOAD_CONST_INLINE_BORROW", uops)

    def test_unary_invert_insert_1_load_const_inline_borrow(self):
        def testfunc(n):
            x = 0
            for i in range(n):
                a = 0
                result = ~a
                if result < 0:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_UNARY_INVERT", uops)
        self.assertIn("_LOAD_CONST_INLINE_BORROW", uops)

    def test_compare_op_pop_two_load_const_inline_borrow(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                a = 10
                b = 10.0
                if a == b:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_COMPARE_OP", uops)

    def test_compare_op_int_insert_two_load_const_inline_borrow(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                a = 10
                b = 10
                if a == b:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_COMPARE_OP_INT", uops)
        self.assertIn("_LOAD_CONST_INLINE_BORROW", uops)

    def test_compare_op_str_insert_two_load_const_inline_borrow(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                a = "foo"
                b = "foo"
                if a == b:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_COMPARE_OP_STR", uops)
        self.assertIn("_LOAD_CONST_INLINE_BORROW", uops)

    def test_compare_op_float_insert_two_load_const_inline_borrow(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                a = 1.0
                b = 1.0
                if a == b:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_COMPARE_OP_FLOAT", uops)
        self.assertIn("_LOAD_CONST_INLINE_BORROW", uops)

    def test_contains_op_pop_two_load_const_inline_borrow(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                a = "foo"
                s = "foo bar baz"
                if a in s:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_CONTAINS_OP", uops)

    def test_to_bool_bool_contains_op_set(self):
        """
        Test that _TO_BOOL_BOOL is removed from code like:

        res = foo in some_set
        if res:
            ....

        """
        def testfunc(n):
            x = 0
            s = {1, 2, 3}
            for _ in range(n):
                a = 2
                in_set = a in s
                if in_set:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CONTAINS_OP_SET", uops)
        self.assertNotIn("_TO_BOOL_BOOL", uops)

    def test_to_bool_bool_contains_op_dict(self):
        """
        Test that _TO_BOOL_BOOL is removed from code like:

        res = foo in some_dict
        if res:
            ....

        """
        def testfunc(n):
            x = 0
            s = {1: 1, 2: 2, 3: 3}
            for _ in range(n):
                a = 2
                in_dict = a in s
                if in_dict:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CONTAINS_OP_DICT", uops)
        self.assertNotIn("_TO_BOOL_BOOL", uops)

    def test_remove_guard_for_known_type_str(self):
        def f(n):
            for i in range(n):
                false = i == TIER2_THRESHOLD
                empty = "X"[:false]
                if empty:
                    return 1
            return 0

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, 0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_TO_BOOL_STR", uops)
        self.assertNotIn("_GUARD_TOS_UNICODE", uops)

    def test_remove_guard_for_known_type_dict(self):
        def f(n):
            x = 0
            for _ in range(n):
                d = {}
                d["Spam"] = 1  # unguarded!
                x += d["Spam"]  # ...unguarded!
            return x

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertEqual(uops.count("_GUARD_NOS_DICT_SUBSCRIPT"), 0)
        self.assertEqual(uops.count("_GUARD_NOS_DICT_STORE_SUBSCRIPT"), 0)
        self.assertEqual(uops.count("_BINARY_OP_SUBSCR_DICT_KNOWN_HASH"), 1)

    def test_dict_subclass_subscr(self):
        import collections

        def f(n):
            x = 0
            d = collections.defaultdict(int)
            for _ in range(n):
                d["key"] = 1
                x += d["key"]
            return x

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertEqual(uops.count("_BINARY_OP_SUBSCR_DICT_KNOWN_HASH"), 1)
        self.assertEqual(uops.count("_STORE_SUBSCR_DICT_KNOWN_HASH"), 1)
        self.assertEqual(uops.count("_GUARD_NOS_DICT_SUBSCRIPT"), 0)
        self.assertEqual(uops.count("_GUARD_NOS_DICT_STORE_SUBSCRIPT"), 0)
        self.assertEqual(uops.count("_GUARD_TYPE"), 1)

    def test_dict_subclass_subscr_with_override(self):
        class MyDict(dict):
            def __getitem__(self, key):
                return 42

        def f(n):
            d = MyDict()
            x = 0
            for _ in range(n):
                x += d["anything"]
            return x

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, 42 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertEqual(uops.count("_BINARY_OP_SUBSCR_INIT_CALL"), 1)

    def test_remove_guard_for_known_type_list(self):
        def f(n):
            x = 0
            for _ in range(n):
                l = [0]
                l[0] = 1  # unguarded!
                [a] = l  # ...unguarded!
                b = l[0]  # ...unguarded!
                if l:  # ...unguarded!
                    x += a + b
            return x

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, 2 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertEqual(uops.count("_GUARD_NOS_LIST"), 0)
        self.assertEqual(uops.count("_STORE_SUBSCR_LIST_INT"), 1)
        self.assertEqual(uops.count("_GUARD_TOS_LIST"), 0)
        self.assertEqual(uops.count("_UNPACK_SEQUENCE_LIST"), 1)
        self.assertEqual(uops.count("_BINARY_OP_SUBSCR_LIST_INT"), 1)
        self.assertEqual(uops.count("_TO_BOOL_LIST"), 1)

    def test_unique_tuple_unpack(self):
        def f(n):
            def four_tuple(x):
                return (x, x, x, x)
            hits = 0
            for i in range(n):
                w, x, y, z = four_tuple(1)
                hits += w + x + y + z
            return hits

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 4)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_BUILD_TUPLE", uops)
        self.assertIn("_UNPACK_SEQUENCE_UNIQUE_TUPLE", uops)
        self.assertNotIn("_UNPACK_SEQUENCE_TUPLE", uops)

    def test_non_unique_tuple_unpack(self):
        def f(n):
            def four_tuple(x):
                return (x, x, x, x)
            hits = 0
            for i in range(n):
                t = four_tuple(1)
                w, x, y, z = t
                hits += w + x + y + z
            return hits

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 4)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_BUILD_TUPLE", uops)
        self.assertIn("_UNPACK_SEQUENCE_TUPLE", uops)
        self.assertNotIn("_UNPACK_SEQUENCE_UNIQUE_TUPLE", uops)

    def test_unique_three_tuple_unpack(self):
        def f(n):
            def three_tuple(x):
                return (x, x, x)
            hits = 0
            for i in range(n):
                x, y, z = three_tuple(1)
                hits += x + y + z
            return hits

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 3)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_BUILD_TUPLE", uops)
        self.assertIn("_UNPACK_SEQUENCE_UNIQUE_THREE_TUPLE", uops)
        self.assertNotIn("_UNPACK_SEQUENCE_TUPLE", uops)

    def test_non_unique_three_tuple_unpack(self):
        def f(n):
            def three_tuple(x):
                return (x, x, x)
            hits = 0
            for i in range(n):
                t = three_tuple(1)
                x, y, z = t
                hits += x + y + z
            return hits

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 3)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_BUILD_TUPLE", uops)
        self.assertIn("_UNPACK_SEQUENCE_TUPLE", uops)
        self.assertNotIn("_UNPACK_SEQUENCE_UNIQUE_THREE_TUPLE", uops)

    def test_unique_two_tuple_unpack(self):
        def f(n):
            def two_tuple(x):
                return (x, x)
            hits = 0
            for i in range(n):
                x, y = two_tuple(1)
                hits += x + y
            return hits

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 2)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_BUILD_TUPLE", uops)
        self.assertIn("_UNPACK_SEQUENCE_UNIQUE_TWO_TUPLE", uops)
        self.assertNotIn("_UNPACK_SEQUENCE_TWO_TUPLE", uops)
        self.assertNotIn("_UNPACK_SEQUENCE_TUPLE", uops)

    def test_non_unique_two_tuple_unpack(self):
        def f(n):
            def two_tuple(x):
                return (x, x)
            hits = 0
            for i in range(n):
                tt = two_tuple(1)
                x, y = tt
                hits += x + y
            return hits

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 2)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_BUILD_TUPLE", uops)
        self.assertIn("_UNPACK_SEQUENCE_TWO_TUPLE", uops)
        self.assertNotIn("_UNPACK_SEQUENCE_TUPLE", uops)
        self.assertNotIn("_UNPACK_SEQUENCE_UNIQUE_TWO_TUPLE", uops)

    def test_remove_guard_for_known_type_set(self):
        def f(n):
            x = 0
            for _ in range(n):
                x += "Spam" in {"Spam"}  # Unguarded!
            return x

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_GUARD_TOS_ANY_SET", uops)
        # _CONTAINS_OP_SET is constant-folded away for frozenset literals
        self.assertIn("_LOAD_CONST_INLINE_BORROW", uops)

    def test_remove_guard_for_known_type_tuple(self):
        def f(n):
            x = 0
            for _ in range(n):
                t = (1, 2, (3, (4,)))
                t_0, t_1, (t_2_0, t_2_1) = t  # Unguarded!
                t_2_1_0 = t_2_1[0]  # Unguarded!
                x += t_0 + t_1 + t_2_0 + t_2_1_0
            return x

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, 10 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_GUARD_TOS_TUPLE", uops)
        self.assertIn("_UNPACK_SEQUENCE_TUPLE", uops)
        self.assertIn("_UNPACK_SEQUENCE_TWO_TUPLE", uops)
        self.assertNotIn("_GUARD_NOS_TUPLE", uops)
        self.assertIn("_BINARY_OP_SUBSCR_TUPLE_INT", uops)

    def test_remove_guard_for_known_type_slice(self):
        def f(n):
            x = 0
            for _ in range(n):
                l = [1, 2, 3]
                slice_obj = slice(0, 1)
                x += l[slice_obj][0] # guarded
                x += l[slice_obj][0] # unguarded
            return x
        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 2)
        uops = get_opnames(ex)

        count = count_ops(ex, "_GUARD_TOS_SLICE")
        self.assertEqual(count, 1)
        self.assertIn("_BINARY_OP_SUBSCR_LIST_INT", uops)

    def test_remove_guard_for_tuple_bounds_check(self):
        def f(n):
            x = 0
            for _ in range(n):
                t = (1, 2, 3)
                x += t[0]
            return x

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_GUARD_BINARY_OP_SUBSCR_TUPLE_INT_BOUNDS", uops)
        self.assertIn("_BINARY_OP_SUBSCR_TUPLE_INT", uops)

    def test_binary_subcsr_str_int_narrows_to_str(self):
        def testfunc(n):
            x = []
            s = "foo"
            for _ in range(n):
                y = s[0]       # _BINARY_OP_SUBSCR_STR_INT
                z = "bar" + y  # (_GUARD_TOS_UNICODE) + _BINARY_OP_ADD_UNICODE
                x.append(z)
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, ["barf"] * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_SUBSCR_STR_INT", uops)
        # _BINARY_OP_SUBSCR_STR_INT narrows the result to 'str' so
        # the unicode guard before _BINARY_OP_ADD_UNICODE is removed.
        self.assertNotIn("_GUARD_TOS_UNICODE", uops)
        self.assertIn("_BINARY_OP_ADD_UNICODE", uops)

    def test_binary_subcsr_ustr_int_narrows_to_str(self):
        def testfunc(n):
            x = []
            s = "바이트코f드_특수화"
            for _ in range(n):
                y = s[4]       # _BINARY_OP_SUBSCR_USTR_INT
                z = "bar" + y  # (_GUARD_TOS_UNICODE) + _BINARY_OP_ADD_UNICODE
                x.append(z)
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, ["barf"] * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_SUBSCR_USTR_INT", uops)
        # _BINARY_OP_SUBSCR_USTR_INT narrows the result to 'str' so
        # the unicode guard before _BINARY_OP_ADD_UNICODE is removed.
        self.assertNotIn("_GUARD_TOS_UNICODE", uops)
        self.assertIn("_BINARY_OP_ADD_UNICODE", uops)

    def test_binary_op_subscr_str_int(self):
        def testfunc(n):
            x = 0
            s = "hello"
            for _ in range(n):
                c = s[1]  # _BINARY_OP_SUBSCR_STR_INT
                if c == 'e':
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_SUBSCR_STR_INT", uops)
        self.assertIn("_COMPARE_OP_STR", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)
        self.assertLessEqual(count_ops(ex, "_POP_TOP_INT"), 1)

    def test_binary_op_subscr_ustr_int(self):
        def testfunc(n):
            x = 0
            s = "hello바"
            for _ in range(n):
                c = s[1]  # _BINARY_OP_SUBSCR_USTR_INT
                if c == 'e':
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_SUBSCR_USTR_INT", uops)
        self.assertIn("_COMPARE_OP_STR", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)
        self.assertLessEqual(count_ops(ex, "_POP_TOP_INT"), 1)

    def test_binary_op_subscr_dict(self):
        def testfunc(n):
            x = 0
            d = {'a': 1, 'b': 2}
            for _ in range(n):
                v = d['a']  # _BINARY_OP_SUBSCR_DICT
                if v == 1:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_SUBSCR_DICT_KNOWN_HASH", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_binary_op_subscr_dict_known_hash(self):
        # str, int, bytes, float, complex, tuple and any python object which has generic hash
        def testfunc(n):
            x = 0
            d = {'a': 1, 1: 2, b'b': 3, (1, 2): 4, _GENERIC_KEY: 5, 1.5: 6, 1+2j: 7}
            for _ in range(n):
                x += d['a'] + d[1] + d[b'b'] + d[(1, 2)] + d[_GENERIC_KEY] + d[1.5] + d[1+2j]
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 28 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_SUBSCR_DICT_KNOWN_HASH", uops)
        self.assertNotIn("_BINARY_OP_SUBSCR_DICT", uops)

    def test_binary_op_subscr_defaultdict_known_hash(self):
        # str, int, bytes, float, complex, tuple and any python object which has generic hash
        import collections

        def testfunc(n):
            x = 0
            d = collections.defaultdict(lambda: 1)
            for _ in range(n):
                x += d['a'] + d[1] + d[b'b'] + d[(1, 2)] + d[_GENERIC_KEY] + d[1.5] + d[1+2j]
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 7 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_SUBSCR_DICT_KNOWN_HASH", uops)
        self.assertNotIn("_BINARY_OP_SUBSCR_DICT", uops)

    def test_binary_op_subscr_constant_frozendict_known_hash(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                x += FROZEN_DICT_CONST['x']
            return x

        res, ex = self._run_with_optimizer(testfunc, 2 * TIER2_THRESHOLD)
        self.assertEqual(res, 2 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_BINARY_OP_SUBSCR_DICT_KNOWN_HASH", uops)
        self.assertNotIn("_BINARY_OP_SUBSCR_DICT", uops)

    def test_store_subscr_dict_known_hash(self):
        # str, int, bytes, float, complex, tuple and any python object which has generic hash
        def testfunc(n):
            d = {'a': 0, 1: 0, b'b': 0, (1, 2): 0, _GENERIC_KEY: 0, 1.5: 0, 1+2j: 0}
            for _ in range(n):
                d['a'] += 1
                d[1] += 2
                d[b'b'] += 3
                d[(1, 2)] += 4
                d[_GENERIC_KEY] += 5
                d[1.5] += 6
                d[1+2j] += 7
            return d['a'] + d[1] + d[b'b'] + d[(1, 2)] + d[_GENERIC_KEY] + d[1.5] + d[1+2j]

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 28 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_STORE_SUBSCR_DICT_KNOWN_HASH", uops)
        self.assertNotIn("_STORE_SUBSCR_DICT", uops)

    def test_store_subscr_defaultdict_known_hash(self):
        import collections

        def testfunc(n):
            d = collections.defaultdict(lambda: 0)
            for _ in range(n):
                d['a'] += 1
                d[1] += 2
                d[b'b'] += 3
                d[(1, 2)] += 4
                d[_GENERIC_KEY] += 5
                d[1.5] += 6
                d[1+2j] += 7
            return d['a'] + d[1] + d[b'b'] + d[(1, 2)] + d[_GENERIC_KEY] + d[1.5] + d[1+2j]

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 28 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_STORE_SUBSCR_DICT_KNOWN_HASH", uops)
        self.assertNotIn("_STORE_SUBSCR_DICT", uops)

    def test_contains_op(self):
        def testfunc(n):
            x = 0
            items = [1, 2, 3]
            for _ in range(n):
                if 2 in items:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CONTAINS_OP", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_contains_op_set(self):
        def testfunc(n):
            x = 0
            s = {1, 2, 3}
            for _ in range(n):
                if 2 in s:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CONTAINS_OP_SET", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_contains_op_dict(self):
        def testfunc(n):
            x = 0
            d = {'a': 1, 'b': 2}
            for _ in range(n):
                if 'a' in d:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CONTAINS_OP_DICT", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_call_type_1_guards_removed(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                foo = eval('42')
                x += type(foo) is int
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_TYPE_1", uops)
        self.assertNotIn("_GUARD_NOS_NULL", uops)
        self.assertNotIn("_GUARD_CALLABLE_TYPE_1", uops)

    def test_call_type_1_known_type(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                x += type(42) is int
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # When the result of type(...) is known, _CALL_TYPE_1 is decomposed.
        self.assertNotIn("_CALL_TYPE_1", uops)
        # _CALL_TYPE_1 produces 2 _POP_TOP_NOP (callable and null)
        # type(42) is int produces 4 _POP_TOP_NOP
        self.assertGreaterEqual(count_ops(ex, "_POP_TOP_NOP"), 6)

    def test_call_type_1_result_is_const(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                t = type(42)
                if t is not None:  # guard is removed
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_GUARD_IS_NOT_NONE_POP", uops)

    def test_call_type_1_pop_top(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                foo = eval('42')
                x += type(foo) is int
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_TYPE_1", uops)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_call_tuple_1_pop_top(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                t = tuple(())
                x += len(t) == 0
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_TUPLE_1", uops)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_call_str_1(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = str(42)
                if y == '42':
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_STR_1", uops)
        self.assertNotIn("_GUARD_NOS_NULL", uops)
        self.assertNotIn("_GUARD_CALLABLE_STR_1", uops)

    def test_call_str_1_pop_top(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                t = str("")
                x += 1 if len(t) == 0 else 0
            return x
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_STR_1", uops)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_call_str_1_result_is_str(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = str(42) + 'foo'
                if y == '42foo':
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_STR_1", uops)
        self.assertIn("_BINARY_OP_ADD_UNICODE", uops)
        self.assertNotIn("_GUARD_NOS_UNICODE", uops)
        self.assertNotIn("_GUARD_TOS_UNICODE", uops)

    def test_call_str_1_result_is_const_for_str_input(self):
        # Test a special case where the argument of str(arg)
        # is known to be a string. The information about the
        # argument being a string should be propagated to the
        # result of str(arg).
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = str('foo')  # string argument
                if y:           # _TO_BOOL_STR + _GUARD_IS_TRUE_POP are removed
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_STR_1", uops)
        self.assertNotIn("_TO_BOOL_STR", uops)
        self.assertNotIn(self.guard_is_true, uops)

    def test_call_tuple_1(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = tuple([1, 2])  # _CALL_TUPLE_1
                if y == (1, 2):
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_TUPLE_1", uops)
        self.assertNotIn("_GUARD_NOS_NULL", uops)
        self.assertNotIn("_GUARD_CALLABLE_TUPLE_1", uops)

    def test_call_tuple_1_result_is_tuple(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = tuple([1, 2])  # _CALL_TUPLE_1
                if y[0] == 1:      # _BINARY_OP_SUBSCR_TUPLE_INT
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_TUPLE_1", uops)
        self.assertIn("_BINARY_OP_SUBSCR_TUPLE_INT", uops)
        self.assertNotIn("_GUARD_NOS_TUPLE", uops)

    def test_call_tuple_1_result_propagates_for_tuple_input(self):
        # Test a special case where the argument of tuple(arg)
        # is known to be a tuple. The information about the
        # argument being a tuple should be propagated to the
        # result of tuple(arg).
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = tuple((1, 2))  # tuple argument
                a, _ = y           # _UNPACK_SEQUENCE_TWO_TUPLE
                if a == 1:         # _COMPARE_OP_INT + _GUARD_IS_TRUE_POP are removed
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_TUPLE_1", uops)
        self.assertIn("_UNPACK_SEQUENCE_TWO_TUPLE", uops)
        self.assertNotIn("_COMPARE_OP_INT", uops)
        self.assertNotIn(self.guard_is_true, uops)

    def test_call_len(self):
        def testfunc(n):
            a = [1, 2, 3, 4]
            for _ in range(n):
                _ = len(a) - 1

        _, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        uops = get_opnames(ex)
        self.assertNotIn("_GUARD_NOS_NULL", uops)
        self.assertNotIn("_GUARD_CALLABLE_LEN", uops)
        self.assertIn("_CALL_LEN", uops)
        self.assertNotIn("_GUARD_NOS_INT", uops)
        self.assertNotIn("_GUARD_TOS_INT", uops)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_check_is_not_py_callable(self):
        def testfunc(n):
            total = 0
            f = len
            xs = (1, 2, 3)
            for _ in range(n):
                total += f(xs)
            return total

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 3 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_CHECK_IS_NOT_PY_CALLABLE", uops)

    def test_check_is_not_py_callable_ex(self):
        def testfunc(n):
            total = 0
            xs = (1, 2, 3)
            args = (xs,)
            for _ in range(n):
                total += len(*args)
            return total

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 3 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_CHECK_IS_NOT_PY_CALLABLE_EX", uops)

    def test_check_is_not_py_callable_kw(self):
        def testfunc(n):
            total = 0
            xs = (3, 1, 2)
            for _ in range(n):
                total += sorted(xs, reverse=False)[0]
            return total

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_CHECK_IS_NOT_PY_CALLABLE_KW", uops)

    def test_call_len_string(self):
        def testfunc(n):
            for _ in range(n):
                _ = len("abc")
                d = ''
                _ = len(d)
                _ = len(b"def")
                _ = len(b"")

        _, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_CALL_LEN", uops)
        self.assertGreaterEqual(count_ops(ex, "_LOAD_CONST_INLINE_BORROW"), 8)

    def test_call_len_known_length_small_int(self):
        # Make sure that len(t) is optimized for a tuple of length 5.
        # See https://github.com/python/cpython/issues/139393.
        self.assertGreater(_PY_NSMALLPOSINTS, 5)

        def testfunc(n):
            x = 0
            for _ in range(n):
                t = (1, 2, 3, 4, 5)
                if len(t) == 5:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # When the length is < _PY_NSMALLPOSINTS, the len() call is replaced
        # with just an inline load.
        self.assertNotIn("_CALL_LEN", uops)

    def test_call_len_known_length(self):
        # Make sure that len(t) is not optimized for a tuple of length 2048.
        # See https://github.com/python/cpython/issues/139393.
        self.assertLess(_PY_NSMALLPOSINTS, 2048)

        def testfunc(n):
            class C:
                t = tuple(range(2048))

            x = 0
            for _ in range(n):
                if len(C.t) == 2048:  # comparison + guard removed
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # When the length is >= _PY_NSMALLPOSINTS, we cannot replace
        # the len() call with an inline load, but knowing the exact
        # length allows us to optimize more code, such as conditionals
        # in this case
        self.assertIn("_CALL_LEN", uops)
        self.assertNotIn("_COMPARE_OP_INT", uops)
        self.assertNotIn(self.guard_is_true, uops)

    def test_call_builtin_class(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = int("42")
                x += y
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 42)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_BUILTIN_CLASS", uops)
        self.assertNotIn("_GUARD_CALLABLE_BUILTIN_CLASS", uops)

    def test_call_builtin_o(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = abs(1)
                x += y
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_BUILTIN_O", uops)
        self.assertNotIn("_GUARD_CALLABLE_BUILTIN_O", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 4)

    def test_call_builtin_fast(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = divmod(10, 3)
                x += y[0]
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 3)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_BUILTIN_FAST", uops)
        self.assertNotIn("_GUARD_CALLABLE_BUILTIN_FAST", uops)
        # divmod(10, 3) should have at least 3 _POP_TOP_NOP
        # x += y[0] produces at least 3 _POP_TOP_NOP
        self.assertGreaterEqual(count_ops(ex, "_POP_TOP_NOP"), 6)

    def test_call_builtin_fast_with_keywords(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = sorted([3, 1, 2])
                x += y[0]
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_BUILTIN_FAST_WITH_KEYWORDS", uops)
        self.assertNotIn("_GUARD_CALLABLE_BUILTIN_FAST_WITH_KEYWORDS", uops)

    def test_call_method_descriptor_o(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = (1, 2, 3)
                z = y.count(2)
                x += z
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_METHOD_DESCRIPTOR_O_INLINE", uops)
        self.assertNotIn("_CALL_METHOD_DESCRIPTOR_O", uops)
        self.assertNotIn("_GUARD_CALLABLE_METHOD_DESCRIPTOR_O", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 4)

    def test_call_method_descriptor_noargs(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = "hello"
                z = y.upper()
                x += len(z)
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 5)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_CALL_METHOD_DESCRIPTOR_NOARGS_INLINE", uops)
        self.assertNotIn("_CALL_METHOD_DESCRIPTOR_NOARGS", uops)
        self.assertNotIn("_GUARD_CALLABLE_METHOD_DESCRIPTOR_NOARGS", uops)
        self.assertGreaterEqual(count_ops(ex, "_POP_TOP"), 5)
        self.assertGreaterEqual(count_ops(ex, "_POP_TOP"), 3)

    def test_call_method_descriptor_fast(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = (1, 2, 3)
                z = y.index(2)
                x += z
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_METHOD_DESCRIPTOR_FAST_INLINE", uops)
        self.assertNotIn("_CALL_METHOD_DESCRIPTOR_FAST", uops)
        self.assertNotIn("_GUARD_CALLABLE_METHOD_DESCRIPTOR_FAST", uops)

    def test_call_method_descriptor_fast_with_keywords(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = "hello world"
                a, b = y.split()
                x += len(a)
            return x
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 5)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS_INLINE", uops)
        self.assertNotIn("_CALL_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS", uops)
        self.assertNotIn("_GUARD_CALLABLE_METHOD_DESCRIPTOR_FAST_WITH_KEYWORDS", uops)

    def test_check_recursion_limit_deduplicated(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = "hello"
                a = y.upper()
                b = y.lower()
                x += len(a)
                x += len(b)
            return x
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 10)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_METHOD_DESCRIPTOR_NOARGS_INLINE", uops)
        self.assertEqual(count_ops(ex, "_CHECK_RECURSION_LIMIT"), 1)

    def test_call_intrinsic_1(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                +x
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 0)
        uops = get_opnames(ex)

        self.assertIn("_CALL_INTRINSIC_1", uops)
        self.assertEqual(count_ops(ex, "_POP_TOP_NOP"), 1)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_call_intrinsic_2(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                def test_testfunc[T](n):
                    pass
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 0)
        uops = get_opnames(ex)

        self.assertIn("_CALL_INTRINSIC_2", uops)
        self.assertGreaterEqual(count_ops(ex, "_POP_TOP_NOP"), 2)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 4)

    def test_get_len_with_const_tuple(self):
        def testfunc(n):
            x = 0.0
            for _ in range(n):
                match (1, 2, 3, 4):
                    case [_, _, _, _]:
                        x += 1.0
            return x
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(int(res), TIER2_THRESHOLD)
        uops = get_opnames(ex)
        self.assertNotIn("_GUARD_NOS_INT", uops)
        self.assertNotIn("_GET_LEN", uops)
        self.assertIn("_LOAD_CONST_INLINE_BORROW", uops)

    def test_get_len_with_non_const_tuple(self):
        def testfunc(n):
            x = 0.0
            for _ in range(n):
                match object(), object():
                    case [_, _]:
                        x += 1.0
            return x
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(int(res), TIER2_THRESHOLD)
        uops = get_opnames(ex)
        self.assertNotIn("_GUARD_NOS_INT", uops)
        self.assertNotIn("_GET_LEN", uops)
        self.assertIn("_LOAD_CONST_INLINE_BORROW", uops)

    def test_get_len_with_non_tuple(self):
        def testfunc(n):
            x = 0.0
            for _ in range(n):
                match [1, 2, 3, 4]:
                    case [_, _, _, _]:
                        x += 1.0
            return x
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(int(res), TIER2_THRESHOLD)
        uops = get_opnames(ex)
        self.assertNotIn("_GUARD_NOS_INT", uops)
        self.assertIn("_GET_LEN", uops)

    def test_binary_op_subscr_tuple_int(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = (1, 2)
                if y[0] == 1:  # _COMPARE_OP_INT + _GUARD_IS_TRUE_POP are removed
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_SUBSCR_TUPLE_INT", uops)
        self.assertNotIn("_COMPARE_OP_INT", uops)
        self.assertNotIn(self.guard_is_true, uops)

    def test_call_isinstance_guards_removed(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = isinstance(42, int)
                if y:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_CALL_ISINSTANCE", uops)
        self.assertNotIn("_GUARD_THIRD_NULL", uops)
        self.assertNotIn("_GUARD_CALLABLE_ISINSTANCE", uops)

    def test_call_list_append(self):
        def testfunc(n):
            a = []
            for i in range(n):
                a.append(i)
            return sum(a)

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, sum(range(TIER2_THRESHOLD)))
        uops = get_opnames(ex)
        self.assertIn("_CALL_LIST_APPEND", uops)

    def test_call_list_append_pop_top(self):
        def testfunc(n):
            a = []
            for i in range(n):
                a.append(1)
            return sum(a)
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        uops = get_opnames(ex)
        self.assertIn("_CALL_LIST_APPEND", uops)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_call_isinstance_is_true(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = isinstance(42, int)
                if y:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_CALL_ISINSTANCE", uops)
        self.assertNotIn("_TO_BOOL_BOOL", uops)
        self.assertNotIn(self.guard_is_true, uops)

    def test_call_isinstance_is_false(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = isinstance(42, str)
                if not y:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_CALL_ISINSTANCE", uops)
        self.assertNotIn("_TO_BOOL_BOOL", uops)
        self.assertNotIn(self.guard_is_false, uops)

    def test_call_isinstance_subclass(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                y = isinstance(True, int)
                if y:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_CALL_ISINSTANCE", uops)
        self.assertNotIn("_TO_BOOL_BOOL", uops)
        self.assertNotIn(self.guard_is_true, uops)

    def test_call_isinstance_unknown_object(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                # The optimizer doesn't know the return type here:
                bar = eval("42")
                # This will only narrow to bool:
                y = isinstance(bar, int)
                if y:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_ISINSTANCE", uops)
        self.assertNotIn("_TO_BOOL_BOOL", uops)
        self.assertIn(self.guard_is_true, uops)

    def test_call_isinstance_tuple_of_classes(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                # A tuple of classes is currently not optimized,
                # so this is only narrowed to bool:
                y = isinstance(42, (int, str))
                if y:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_ISINSTANCE", uops)
        self.assertNotIn("_TO_BOOL_BOOL", uops)
        self.assertIn(self.guard_is_true, uops)

    def test_call_isinstance_metaclass(self):
        class EvenNumberMeta(type):
            def __instancecheck__(self, number):
                return number % 2 == 0

        class EvenNumber(metaclass=EvenNumberMeta):
            pass

        def testfunc(n):
            x = 0
            for _ in range(n):
                # Only narrowed to bool
                y = isinstance(42, EvenNumber)
                if y:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_CALL_ISINSTANCE", uops)
        self.assertNotIn("_TO_BOOL_BOOL", uops)
        self.assertIn(self.guard_is_true, uops)

    def test_set_type_version_sets_type(self):
        class C:
            A = 1

        def testfunc(n):
            x = 0
            c = C()
            for _ in range(n):
                x += c.A  # Guarded.
                x += type(c).A  # Unguarded!
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 2 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_GUARD_TYPE_VERSION", uops)
        self.assertNotIn("_CHECK_ATTR_CLASS", uops)

    def test_load_common_constant(self):
        def testfunc(n):
            for _ in range(n):
                x = list(i for i in ())
            return x
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, list(()))
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BUILD_LIST", uops)
        self.assertNotIn("_LOAD_COMMON_CONSTANT", uops)

    def test_load_small_int(self):
        def testfunc(n):
            x = 0
            for i in range(n):
                x += 1
            return x
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_LOAD_SMALL_INT", uops)
        self.assertIn("_LOAD_CONST_INLINE_BORROW", uops)

    def test_cached_attributes(self):
        class C:
            A = 1
            def m(self):
                return 1
        class D:
            __slots__ = ()
            A = 1
            def m(self):
                return 1
        class E(Exception):
            def m(self):
                return 1
        def f(n):
            x = 0
            c = C()
            d = D()
            e = E()
            for _ in range(n):
                x += C.A  # _LOAD_ATTR_CLASS
                x += c.A  # _LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES
                x += d.A  # _LOAD_ATTR_NONDESCRIPTOR_NO_DICT
                x += c.m()  # _LOAD_ATTR_METHOD_WITH_VALUES
                x += d.m()  # _LOAD_ATTR_METHOD_NO_DICT
                x += e.m()  # _LOAD_ATTR_METHOD_LAZY_DICT
            return x

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, 6 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_LOAD_ATTR_CLASS", uops)
        self.assertNotIn("_LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES", uops)
        self.assertNotIn("_LOAD_ATTR_NONDESCRIPTOR_NO_DICT", uops)
        self.assertNotIn("_LOAD_ATTR_METHOD_WITH_VALUES", uops)
        self.assertNotIn("_LOAD_ATTR_METHOD_NO_DICT", uops)
        self.assertNotIn("_LOAD_ATTR_METHOD_LAZY_DICT", uops)

    def test_cached_attributes_fixed_version_tag(self):
        def f(n):
            c = 1
            x = 0
            for _ in range(n):
                x += c.bit_length()
            return x
        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        self.assertEqual(res, TIER2_THRESHOLD)
        uops = get_opnames(ex)
        self.assertNotIn("_LOAD_ATTR_METHOD_NO_DICT", uops)
        self.assertIn("_LOAD_CONST_INLINE_BORROW", uops)

    def test_cached_load_special(self):
        class CM:
            def __enter__(self):
                return self
            def __exit__(self, *args):
                pass
        def f(n):
            cm = CM()
            x = 0
            for _ in range(n):
                with cm:
                    x += 1
            return x
        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        self.assertEqual(res, TIER2_THRESHOLD)
        uops = get_opnames(ex)
        self.assertNotIn("_LOAD_SPECIAL", uops)
        # __enter__/__exit__ produce 2 _POP_TOP_NOP
        # x += 1 produces 2 _POP_TOP_NOP
        # __exit__()'s None return produces 1 _POP_TOP_NOP
        self.assertGreaterEqual(count_ops(ex, "_POP_TOP_NOP"), 5)

    def test_store_fast_refcount_elimination(self):
        def foo(x):
            # Since x is known to be
            # a constant value (1) here,
            # The refcount is eliminated in the STORE_FAST.
            x = 2
            return x
        def testfunc(n):
            # The STORE_FAST for the range here needs a POP_TOP
            # (for now, until we do loop peeling).
            for _ in range(n):
                foo(1)

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertIn("_PUSH_FRAME", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 1)

    def test_store_fast_refcount_elimination_when_uninitialized(self):
        def foo():
            # Since y is known to be
            # uninitialized (NULL) here,
            # The refcount is eliminated in the STORE_FAST.
            y = 2
            return y
        def testfunc(n):
            # The STORE_FAST for the range here needs a POP_TOP
            # (for now, until we do loop peeling).
            for _ in range(n):
                foo()

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertIn("_PUSH_FRAME", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 1)


    def test_float_op_refcount_elimination(self):
        def testfunc(args):
            a, b, n = args
            c = 0.0
            for _ in range(n):
                c += a + b
            return c

        res, ex = self._run_with_optimizer(testfunc, (0.1, 0.1, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * (0.1 + 0.1))
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_float_add_inplace_unique_lhs(self):
        # a * b produces a unique float; adding c reuses it in place
        def testfunc(args):
            a, b, c, n = args
            total = 0.0
            for _ in range(n):
                total += a * b + c
            return total

        res, ex = self._run_with_optimizer(testfunc, (2.0, 3.0, 4.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * 10.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_ADD_FLOAT_INPLACE", uops)

    def test_float_add_inplace_unique_rhs(self):
        # a * b produces a unique float on the right side of +
        def testfunc(args):
            a, b, c, n = args
            total = 0.0
            for _ in range(n):
                total += c + a * b
            return total

        res, ex = self._run_with_optimizer(testfunc, (2.0, 3.0, 4.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * 10.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_ADD_FLOAT_INPLACE_RIGHT", uops)

    def test_float_add_no_inplace_non_unique(self):
        # Both operands of a + b are locals — neither is unique,
        # so the first add is regular. But total += (a+b) has a
        # unique RHS, so it uses _INPLACE_RIGHT.
        def testfunc(args):
            a, b, n = args
            total = 0.0
            for _ in range(n):
                total += a + b
            return total

        res, ex = self._run_with_optimizer(testfunc, (2.0, 3.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * 5.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # a + b: both are locals, no inplace
        self.assertIn("_BINARY_OP_ADD_FLOAT", uops)
        # total += result: result is unique RHS
        self.assertIn("_BINARY_OP_ADD_FLOAT_INPLACE_RIGHT", uops)
        # No LHS inplace variant for the first add
        self.assertNotIn("_BINARY_OP_ADD_FLOAT_INPLACE", uops)

    def test_float_subtract_inplace_unique_lhs(self):
        # a * b produces a unique float; subtracting c reuses it
        def testfunc(args):
            a, b, c, n = args
            total = 0.0
            for _ in range(n):
                total += a * b - c
            return total

        res, ex = self._run_with_optimizer(testfunc, (2.0, 3.0, 1.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * 5.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_SUBTRACT_FLOAT_INPLACE", uops)

    def test_float_subtract_inplace_unique_rhs(self):
        # a * b produces a unique float on the right of -;
        # result is c - (a * b), must get the sign correct
        def testfunc(args):
            a, b, c, n = args
            total = 0.0
            for _ in range(n):
                total += c - a * b
            return total

        res, ex = self._run_with_optimizer(testfunc, (2.0, 3.0, 1.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * -5.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_SUBTRACT_FLOAT_INPLACE_RIGHT", uops)

    def test_float_multiply_inplace_unique_lhs(self):
        # (a + b) produces a unique float; multiplying by c reuses it
        def testfunc(args):
            a, b, c, n = args
            total = 0.0
            for _ in range(n):
                total += (a + b) * c
            return total

        res, ex = self._run_with_optimizer(testfunc, (2.0, 3.0, 4.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * 20.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_MULTIPLY_FLOAT_INPLACE", uops)

    def test_float_multiply_inplace_unique_rhs(self):
        # (a + b) produces a unique float on the right side of *
        def testfunc(args):
            a, b, c, n = args
            total = 0.0
            for _ in range(n):
                total += c * (a + b)
            return total

        res, ex = self._run_with_optimizer(testfunc, (2.0, 3.0, 4.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * 20.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_MULTIPLY_FLOAT_INPLACE_RIGHT", uops)

    def test_float_inplace_chain_propagation(self):
        # a * b + c * d: both products are unique, the + reuses one;
        # result of + is also unique for the subsequent +=
        def testfunc(args):
            a, b, c, d, n = args
            total = 0.0
            for _ in range(n):
                total += a * b + c * d
            return total

        res, ex = self._run_with_optimizer(testfunc, (2.0, 3.0, 4.0, 5.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * 26.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # The + between the two products should use an inplace variant
        inplace_add = (
            "_BINARY_OP_ADD_FLOAT_INPLACE" in uops
            or "_BINARY_OP_ADD_FLOAT_INPLACE_RIGHT" in uops
        )
        self.assertTrue(inplace_add,
            "Expected an inplace add for unique intermediate results")

    def test_float_negate_inplace_unique(self):
        # -(a * b): the product is unique, negate it in place
        def testfunc(args):
            a, b, n = args
            total = 0.0
            for _ in range(n):
                total += -(a * b)
            return total

        res, ex = self._run_with_optimizer(testfunc, (2.0, 3.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * -6.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_UNARY_NEGATIVE_FLOAT_INPLACE", uops)

    def test_float_negate_no_inplace_non_unique(self):
        # -a where a is a local — not unique, no inplace
        def testfunc(args):
            a, n = args
            total = 0.0
            for _ in range(n):
                total += -a
            return total

        res, ex = self._run_with_optimizer(testfunc, (2.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * -2.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_UNARY_NEGATIVE_FLOAT_INPLACE", uops)

    def test_float_truediv_inplace_unique_lhs(self):
        # (a + b) / (c + d): LHS is unique float from add, RHS is unique
        # float from add. The division reuses the LHS in place.
        def testfunc(args):
            a, b, c, d, n = args
            total = 0.0
            for _ in range(n):
                total += (a + b) / (c + d)
            return total

        res, ex = self._run_with_optimizer(testfunc, (2.0, 3.0, 1.0, 3.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * 1.25)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_TRUEDIV_FLOAT_INPLACE", uops)

    def test_float_truediv_inplace_unique_rhs(self):
        # x = c + d stores to a local (not unique when reloaded).
        # (a + b) is unique. The division should use inplace on the RHS.
        def testfunc(args):
            a, b, c, d, n = args
            total = 0.0
            for _ in range(n):
                x = c + d
                total += x / (a + b)
            return total

        res, ex = self._run_with_optimizer(testfunc, (2.0, 3.0, 4.0, 5.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * (9.0 / 5.0))
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_TRUEDIV_FLOAT_INPLACE_RIGHT", uops)

    def test_float_truediv_speculative_guards_from_tracing(self):
        # a, b are locals with no statically known type. _RECORD_TOS_TYPE /
        # _RECORD_NOS_TYPE (added to the BINARY_OP macro) capture the observed
        # operand types during tracing, and the optimizer then speculatively
        # emits _GUARD_{TOS,NOS}_FLOAT and specializes the division.
        def testfunc(args):
            a, b, n = args
            total = 0.0
            for _ in range(n):
                total += a / b
            return total

        res, ex = self._run_with_optimizer(testfunc, (10.0, 3.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * (10.0 / 3.0))
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_GUARD_TOS_FLOAT", uops)
        self.assertIn("_GUARD_NOS_FLOAT", uops)
        self.assertIn("_BINARY_OP_TRUEDIV_FLOAT", uops)

    def test_float_remainder_speculative_guards_from_tracing(self):
        # a, b are locals with no statically known type. Tracing records
        # them as floats; the optimizer then speculatively emits
        # _GUARD_{TOS,NOS}_FLOAT for NB_REMAINDER. That narrows both
        # operands to float, and the _BINARY_OP handler marks the result
        # as a unique float. Downstream, `* 2.0` therefore specializes
        # to _BINARY_OP_MULTIPLY_FLOAT_INPLACE.
        def testfunc(args):
            a, b, n = args
            total = 0.0
            for _ in range(n):
                total += (a % b) * 2.0
            return total

        res, ex = self._run_with_optimizer(testfunc, (10.0, 3.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * (10.0 % 3.0) * 2.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_GUARD_TOS_FLOAT", uops)
        self.assertIn("_GUARD_NOS_FLOAT", uops)
        self.assertIn("_BINARY_OP_MULTIPLY_FLOAT_INPLACE", uops)

    def test_float_truediv_type_propagation(self):
        # Test the _BINARY_OP_TRUEDIV_FLOAT propagates type information
        def testfunc(args):
            a, b, n = args
            total = 0.0
            for _ in range(n):
                x = (a + b) # type of x will specialize to float
                total += x / x - x / x
            return total

        res, ex = self._run_with_optimizer(testfunc,
            (2.0, 3.0, TIER2_THRESHOLD))
        expected = TIER2_THRESHOLD * ((2.0 + 3.0) / (2.0 + 3.0) - (2.0 + 3.0) / (2.0 + 3.0))
        self.assertAlmostEqual(res, expected)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_TRUEDIV_FLOAT", uops)
        self.assertIn("_BINARY_OP_SUBTRACT_FLOAT_INPLACE", uops)

    def test_float_truediv_unique_result_enables_inplace(self):
        # (a+b) / (c+d) / (e+f): chained divisions where each result
        # is unique, enabling inplace for subsequent divisions.
        def testfunc(args):
            a, b, c, d, e, f, n = args
            total = 0.0
            for _ in range(n):
                total += (a + b) / (c + d) / (e + f)
            return total

        res, ex = self._run_with_optimizer(testfunc,
            (2.0, 3.0, 1.0, 1.0, 1.0, 1.0, TIER2_THRESHOLD))
        expected = TIER2_THRESHOLD * ((2.0 + 3.0) / (1.0 + 1.0) / (1.0 + 1.0))
        self.assertAlmostEqual(res, expected)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_TRUEDIV_FLOAT_INPLACE", uops)

    def test_float_add_chain_both_unique(self):
        # (a+b) + (c+d): both sub-additions produce unique floats.
        # The outer + should use inplace on one of them.
        def testfunc(args):
            a, b, c, d, n = args
            total = 0.0
            for _ in range(n):
                total += (a + b) + (c + d)
            return total

        res, ex = self._run_with_optimizer(testfunc, (1.0, 2.0, 3.0, 4.0, TIER2_THRESHOLD))
        self.assertAlmostEqual(res, TIER2_THRESHOLD * 10.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # The outer + should use inplace (at least one operand is unique)
        inplace = (
            "_BINARY_OP_ADD_FLOAT_INPLACE" in uops
            or "_BINARY_OP_ADD_FLOAT_INPLACE_RIGHT" in uops
        )
        self.assertTrue(inplace, "Expected inplace add for unique sub-results")

    def test_float_truediv_non_float_type_no_crash(self):
        # Fraction / Fraction goes through _BINARY_OP with NB_TRUE_DIVIDE
        # but returns Fraction, not float. The optimizer must not assume
        # the result is float for non-int/float operands. See gh-146306.
        from fractions import Fraction
        def testfunc(args):
            a, b, n = args
            total = Fraction(0)
            for _ in range(n):
                total += a / b
            return float(total)

        res, ex = self._run_with_optimizer(testfunc, (Fraction(10), Fraction(3), TIER2_THRESHOLD))
        expected = float(TIER2_THRESHOLD * Fraction(10, 3))
        self.assertAlmostEqual(res, expected)

    def test_float_truediv_mixed_float_fraction_no_crash(self):
        # float / Fraction: lhs is known float from a prior guard,
        # but rhs is Fraction. The guard insertion for rhs should
        # deopt cleanly at runtime, not crash.
        from fractions import Fraction
        def testfunc(args):
            a, b, c, n = args
            total = 0.0
            for _ in range(n):
                total += (a + b) / c  # (a+b) is float, c is Fraction
            return total

        res, ex = self._run_with_optimizer(testfunc, (2.0, 3.0, Fraction(4), TIER2_THRESHOLD))
        expected = TIER2_THRESHOLD * (5.0 / Fraction(4))
        self.assertAlmostEqual(res, float(expected))

    def test_float_truediv_partial_float_no_stack_underflow(self):
        # gh-149049: a speculative _GUARD_*_FLOAT for a partially-float
        # truediv/remainder must not drop the original _BINARY_OP.
        def truediv(args):
            n, = args
            nan = float("nan")
            def victim(a=0, b=nan, c=2):
                return (a + b) / c
            for _ in range(n):
                victim()

        def remainder(args):
            n, = args
            nan = float("nan")
            def victim(a=0, b=nan, c=2):
                return (a + b) % c
            for _ in range(n):
                victim()

        for testfunc in (truediv, remainder):
            with self.subTest(op=testfunc.__name__):
                # Iterations must be high enough that the buggy trace
                # is not only built but executed (where it underflows).
                _, ex = self._run_with_optimizer(
                    testfunc, (TIER2_THRESHOLD * 10,))
                self.assertIsNotNone(ex)
                uops = get_opnames(ex)
                self.assertTrue(
                    "_GUARD_TOS_FLOAT" in uops or "_GUARD_NOS_FLOAT" in uops,
                    uops,
                )

    def test_int_add_inplace_unique_lhs(self):
        # a * b produces a unique compact int; adding c reuses it in place
        def testfunc(args):
            a, b, c, n = args
            total = 0
            for _ in range(n):
                total += a * b + c
            return total

        res, ex = self._run_with_optimizer(testfunc, (2000, 3, 4000, TIER2_THRESHOLD))
        self.assertEqual(res, TIER2_THRESHOLD * 10000)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_ADD_INT_INPLACE", uops)

    def test_int_add_inplace_unique_rhs(self):
        # a * b produces a unique compact int on the right side of +
        def testfunc(args):
            a, b, c, n = args
            total = 0
            for _ in range(n):
                total += c + a * b
            return total

        res, ex = self._run_with_optimizer(testfunc, (2000, 3, 4000, TIER2_THRESHOLD))
        self.assertEqual(res, TIER2_THRESHOLD * 10000)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_ADD_INT_INPLACE_RIGHT", uops)

    def test_int_add_no_inplace_non_unique(self):
        # Both operands of a + b are locals — neither is unique,
        # so the first add uses the regular op. But total += (a+b)
        # has a unique RHS (result of a+b), so it uses _INPLACE_RIGHT.
        def testfunc(args):
            a, b, n = args
            total = 0
            for _ in range(n):
                total += a + b
            return total

        res, ex = self._run_with_optimizer(testfunc, (2000, 3000, TIER2_THRESHOLD))
        self.assertEqual(res, TIER2_THRESHOLD * 5000)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # a + b: both are locals, no inplace
        self.assertIn("_BINARY_OP_ADD_INT", uops)
        # total += result: result is unique RHS
        self.assertIn("_BINARY_OP_ADD_INT_INPLACE_RIGHT", uops)
        # No LHS inplace variant for the first add
        self.assertNotIn("_BINARY_OP_ADD_INT_INPLACE", uops)

    def test_int_add_inplace_small_int_result(self):
        # When the result is a small int, the inplace path falls back
        # to _PyCompactLong_Add. Verify correctness (no singleton corruption).
        def testfunc(args):
            a, b, n = args
            total = 0
            for _ in range(n):
                total += a * b + 1  # a*b=6, +1=7, small int
            return total

        res, ex = self._run_with_optimizer(testfunc, (2, 3, TIER2_THRESHOLD))
        self.assertEqual(res, TIER2_THRESHOLD * 7)
        # Verify small int singletons are not corrupted
        self.assertEqual(7, 3 + 4)

    def test_int_subtract_inplace_unique_lhs(self):
        # a * b produces a unique compact int; subtracting c reuses it
        def testfunc(args):
            a, b, c, n = args
            total = 0
            for _ in range(n):
                total += a * b - c
            return total

        res, ex = self._run_with_optimizer(testfunc, (2000, 3, 1000, TIER2_THRESHOLD))
        self.assertEqual(res, TIER2_THRESHOLD * 5000)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_SUBTRACT_INT_INPLACE", uops)

    def test_int_subtract_inplace_unique_rhs(self):
        # a * b produces a unique compact int on the right of -
        def testfunc(args):
            a, b, c, n = args
            total = 0
            for _ in range(n):
                total += c - a * b
            return total

        res, ex = self._run_with_optimizer(testfunc, (2000, 3, 10000, TIER2_THRESHOLD))
        self.assertEqual(res, TIER2_THRESHOLD * 4000)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_SUBTRACT_INT_INPLACE_RIGHT", uops)

    def test_int_multiply_inplace_unique_lhs(self):
        # (a + b) produces a unique compact int; multiplying by c reuses it
        def testfunc(args):
            a, b, c, n = args
            total = 0
            for _ in range(n):
                total += (a + b) * c
            return total

        res, ex = self._run_with_optimizer(testfunc, (2000, 3000, 4, TIER2_THRESHOLD))
        self.assertEqual(res, TIER2_THRESHOLD * 20000)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_MULTIPLY_INT_INPLACE", uops)

    def test_int_multiply_inplace_unique_rhs(self):
        # (a + b) produces a unique compact int on the right side of *
        def testfunc(args):
            a, b, c, n = args
            total = 0
            for _ in range(n):
                total += c * (a + b)
            return total

        res, ex = self._run_with_optimizer(testfunc, (2000, 3000, 4, TIER2_THRESHOLD))
        self.assertEqual(res, TIER2_THRESHOLD * 20000)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_MULTIPLY_INT_INPLACE_RIGHT", uops)

    def test_int_inplace_chain_propagation(self):
        # a * b + c * d: both products are unique, the + reuses one;
        # result of + is also unique for the subsequent +=
        def testfunc(args):
            a, b, c, d, n = args
            total = 0
            for _ in range(n):
                total += a * b + c * d
            return total

        res, ex = self._run_with_optimizer(testfunc, (2000, 3, 4000, 5, TIER2_THRESHOLD))
        self.assertEqual(res, TIER2_THRESHOLD * 26000)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        inplace_add = (
            "_BINARY_OP_ADD_INT_INPLACE" in uops
            or "_BINARY_OP_ADD_INT_INPLACE_RIGHT" in uops
        )
        self.assertTrue(inplace_add,
            "Expected an inplace add for unique intermediate results")

    def test_load_attr_instance_value(self):
        def testfunc(n):
            class C():
                pass
            c = C()
            c.x = n
            x = 0
            for _ in range(n):
                x = c.x
            return x
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_LOAD_ATTR_INSTANCE_VALUE", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_load_attr_module(self):
        def testfunc(n):
            import math
            x = 0
            for _ in range(n):
                y = math.pi
                if y:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn(("_LOAD_ATTR_MODULE", "_POP_TOP_NOP"), itertools.pairwise(uops))
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_load_attr_with_hint(self):
        def testfunc(n):
            class C:
                pass
            c = C()
            c.x = 42
            for i in range(_testinternalcapi.SHARED_KEYS_MAX_SIZE - 1):
                setattr(c, f"_{i}", None)
            x = 0
            for i in range(n):
                x += c.x
            return x
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 42 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_LOAD_ATTR_WITH_HINT", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_load_addr_slot(self):
        def testfunc(n):
            class C:
                __slots__ = ('x',)
            c = C()
            c.x = 42
            x = 0
            for _ in range(n):
                x += c.x
            return x
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 42 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_LOAD_ATTR_SLOT", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_int_add_op_refcount_elimination(self):
        def testfunc(n):
            c = 1
            res = 0
            for _ in range(n):
                res = c + c
            return res

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_ADD_INT", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_int_sub_op_refcount_elimination(self):
        def testfunc(n):
            c = 1
            res = 0
            for _ in range(n):
                res = c - c
            return res

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_SUBTRACT_INT", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_int_mul_op_refcount_elimination(self):
        def testfunc(n):
            c = 1
            res = 0
            for _ in range(n):
                res = c * c
            return res

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_MULTIPLY_INT", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_int_cmp_op_refcount_elimination(self):
        def testfunc(n):
            c = 1
            res = 0
            for _ in range(n):
                res = c == c
            return res

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_COMPARE_OP_INT", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_float_cmp_op_refcount_elimination(self):
        def testfunc(n):
            c = 1.0
            res = False
            for _ in range(n):
                res = c == c
            return res

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_COMPARE_OP_FLOAT", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_str_cmp_op_refcount_elimination(self):
        def testfunc(n):
            c = "a"
            res = False
            for _ in range(n):
                res = c == c
            return res

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_COMPARE_OP_STR", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_unicode_add_op_refcount_elimination(self):
        def testfunc(n):
            c = "a"
            res = ""
            for _ in range(n):
                res = c + c
            return res

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, "aa")
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_ADD_UNICODE", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_binary_op_refcount_elimination(self):
        class CustomAdder:
            def __init__(self, val):
                self.val = val
            def __add__(self, other):
                return CustomAdder(self.val + other.val)

        def testfunc(n):
            a = CustomAdder(1)
            b = CustomAdder(2)
            res = None
            for _ in range(n):
                res = a + b
            return res.val if res else 0

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 3)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_binary_op_extend_float_long_add_refcount_elimination(self):
        def testfunc(n):
            a = 1.5
            b = 2
            res = 0.0
            for _ in range(n):
                res = a + b
            return res

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 3.5)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_EXTEND", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_remove_guard_for_slice_list(self):
        def f(n):
            for i in range(n):
                false = i == TIER2_THRESHOLD
                sliced = [1, 2, 3][:false]
                if sliced:
                    return 1
            return 0

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, 0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_TO_BOOL_LIST", uops)
        self.assertNotIn("_GUARD_TOS_LIST", uops)

    def test_remove_guard_for_slice_tuple(self):
        def f(n):
            for i in range(n):
                false = i == TIER2_THRESHOLD
                a, b = (1, 2, 3)[: false + 2]

        _, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_UNPACK_SEQUENCE_TWO_TUPLE", uops)
        self.assertNotIn("_GUARD_TOS_TUPLE", uops)

    def test_binary_op_extend_float_result_enables_inplace_multiply(self):
        # (2 + x) * y with x, y floats: `2 + x` goes through _BINARY_OP_EXTEND
        # (int + float). The result_type/result_unique info should let the
        # subsequent float multiply use the inplace variant.
        def testfunc(n):
            x = 3.5
            y = 2.0
            res = 0.0
            for _ in range(n):
                res = (2 + x) * y
            return res

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 11.0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_EXTEND", uops)
        self.assertIn("_BINARY_OP_MULTIPLY_FLOAT_INPLACE", uops)
        self.assertNotIn("_BINARY_OP_MULTIPLY_FLOAT", uops)
        # NOS guard on the multiply is eliminated because _BINARY_OP_EXTEND
        # propagates PyFloat_Type.
        self.assertNotIn("_GUARD_NOS_FLOAT", uops)

    def test_binary_op_extend_list_concat_type_propagation(self):
        # list + list is specialized via BINARY_OP_EXTEND. The tier 2 optimizer
        # should learn that the result is a list and eliminate subsequent
        # list-type guards.
        def testfunc(n):
            a = [1, 2]
            b = [3, 4]
            x = True
            for _ in range(n):
                c = a + b
                if c[0]:
                    x = False
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, False)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_EXTEND", uops)
        # The c[0] subscript emits _GUARD_NOS_LIST before _BINARY_OP_SUBSCR_LIST_INT;
        # since _BINARY_OP_EXTEND now propagates PyList_Type, that guard is gone.
        self.assertIn("_BINARY_OP_SUBSCR_LIST_INT", uops)
        self.assertNotIn("_GUARD_NOS_LIST", uops)

    def test_binary_op_extend_tuple_concat_type_propagation(self):
        # tuple + tuple is specialized via BINARY_OP_EXTEND. The tier 2 optimizer
        # should learn the result is a tuple and eliminate subsequent tuple guards.
        def testfunc(n):
            t1 = (1, 2)
            t2 = (3, 4)
            for _ in range(n):
                a, b, c, d = t1 + t2
            return a + b + c + d

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 10)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_EXTEND", uops)
        self.assertIn("_UNPACK_SEQUENCE_TUPLE", uops)
        self.assertNotIn("_GUARD_TOS_TUPLE", uops)

    def test_binary_op_extend_guard_elimination(self):
        # When both operands have known types (e.g., from a prior
        # _BINARY_OP_EXTEND result), the _GUARD_BINARY_OP_EXTEND
        # should be eliminated.
        def testfunc(n):
            a = [1, 2]
            b = [3, 4]
            total = 0
            for _ in range(n):
                c = a + b    # first: guard stays, result type = list
                d = c + c    # second: both operands are list -> guard eliminated
                total += d[0]
            return total

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        # Both list additions use _BINARY_OP_EXTEND
        self.assertEqual(uops.count("_BINARY_OP_EXTEND"), 2)
        # But the second guard is eliminated because both operands
        # are known to be lists from the first _BINARY_OP_EXTEND.
        self.assertEqual(uops.count("_GUARD_BINARY_OP_EXTEND"), 1)

    def test_binary_op_extend_partial_guard_lhs_known(self):
        # When the lhs type is already known (from a prior _BINARY_OP_EXTEND
        # result) but the rhs type is not, the optimizer should emit
        # _GUARD_BINARY_OP_EXTEND_RHS (checking only the rhs) instead of
        # the full _GUARD_BINARY_OP_EXTEND.
        def testfunc(n):
            a = [1, 2]
            b = [3, 4]
            total = 0
            for _ in range(n):
                c = a + b    # result type is list (known)
                d = c + b    # lhs (c) is known list, rhs (b) is not -> _GUARD_BINARY_OP_EXTEND_RHS
                total += d[0]
            return total

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_EXTEND", uops)
        self.assertIn("_GUARD_BINARY_OP_EXTEND_RHS", uops)
        self.assertNotIn("_GUARD_BINARY_OP_EXTEND_LHS", uops)

    def test_binary_op_extend_partial_guard_rhs_known(self):
        # When the rhs type is already known (from a prior _BINARY_OP_EXTEND
        # result) but the lhs type is not, the optimizer should emit
        # _GUARD_BINARY_OP_EXTEND_LHS (checking only the lhs) instead of
        # the full _GUARD_BINARY_OP_EXTEND.
        def testfunc(n):
            a = [1, 2]
            b = [3, 4]
            total = 0
            for _ in range(n):
                c = a + b    # result type is list (known)
                d = b + c    # rhs (c) is known list, lhs (b) is not -> _GUARD_BINARY_OP_EXTEND_LHS
                total += d[2]
            return total

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_BINARY_OP_EXTEND", uops)
        self.assertIn("_GUARD_BINARY_OP_EXTEND_LHS", uops)
        self.assertNotIn("_GUARD_BINARY_OP_EXTEND_RHS", uops)

    def test_unary_invert_long_type(self):
        def testfunc(n):
            for _ in range(n):
                a = 9397
                x = ~a + ~a

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertNotIn("_GUARD_TOS_INT", uops)
        self.assertNotIn("_GUARD_NOS_INT", uops)

    def test_store_attr_instance_value(self):
        def testfunc(n):
            class C:
                pass
            c = C()
            for i in range(n):
                c.a = i
            return c.a
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD - 1)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_STORE_ATTR_INSTANCE_VALUE", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 1)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_store_attr_with_hint(self):
        def testfunc(n):
            class C:
                pass
            c = C()
            for i in range(_testinternalcapi.SHARED_KEYS_MAX_SIZE - 1):
                setattr(c, f"_{i}", None)

            for i in range(n):
                c.x = i
            return c.x
        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD - 1)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_STORE_ATTR_WITH_HINT", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 1)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_store_subscr_int(self):
        def testfunc(n):
            l = [0, 0, 0, 0]
            for _ in range(n):
                l[0] = 1
                l[1] = 2
                l[2] = 3
                l[3] = 4
            return sum(l)

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 10)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_STORE_SUBSCR_LIST_INT", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 1)
        self.assertNotIn("_POP_TOP_INT", uops)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_store_attr_slot(self):
        class C:
            __slots__ = ('x',)

        def testfunc(n):
            c = C()
            for _ in range(n):
                c.x = 42
                y = c.x
            return y

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 42)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_STORE_ATTR_SLOT", uops)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_store_subscr_dict(self):
        def testfunc(n):
            d = {}
            for _ in range(n):
                d['a'] = 1
                d['b'] = 2
                d['c'] = 3
                d['d'] = 4
            return sum(d.values())

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 10)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_STORE_SUBSCR_DICT_KNOWN_HASH", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 1)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_to_bool_str(self):
        def f(n):
            for i in range(n):
                false = i == TIER2_THRESHOLD
                empty = "X"[:false]
                if empty:
                    return 1
            return 0

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, 0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_TO_BOOL_STR", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 3)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_to_bool_int(self):
        def f(n):
            for i in range(n):
                truthy = (i == TIER2_THRESHOLD)
                x = 0 + truthy
                if x:
                    return 1
            return 0

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, 0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_TO_BOOL_INT", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 3)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_to_bool_list(self):
        def f(n):
            for i in range(n):
                lst = [] if i != TIER2_THRESHOLD else [1]
                if lst:
                    return 1
            return 0

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, 0)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_TO_BOOL_LIST", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 3)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_to_bool_always_true(self):
        def testfunc(n):
            class A:
                pass

            a = A()
            for _ in range(n):
                if not a:
                    return 0
            return 1

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 1)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_REPLACE_WITH_TRUE", uops)

    def test_attr_promotion_failure(self):
        # We're not testing for any specific uops here, just
        # testing it doesn't crash.
        script_helper.assert_python_ok('-c', textwrap.dedent("""
        import _testinternalcapi
        import _opcode
        import email

        def get_first_executor(func):
            code = func.__code__
            co_code = code.co_code
            for i in range(0, len(co_code), 2):
                try:
                    return _opcode.get_executor(code, i)
                except ValueError:
                    pass
            return None

        def testfunc(n):
            for _ in range(n):
                email.jit_testing = None
                prompt = email.jit_testing
                del email.jit_testing


        testfunc(_testinternalcapi.TIER2_THRESHOLD)
        """))

    def test_pop_top_specialize_none(self):
        def testfunc(n):
            for _ in range(n):
                global_identity(None)

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_POP_TOP_NOP", uops)

    def test_pop_top_specialize_int(self):
        def testfunc(n):
            for _ in range(n):
                global_identity(100000)

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_POP_TOP_INT", uops)

    def test_pop_top_specialize_float(self):
        def testfunc(n):
            for _ in range(n):
                global_identity(1e6)

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_POP_TOP_FLOAT", uops)


    def test_unary_negative_long_float_type(self):
        def testfunc(n):
            for _ in range(n):
                a = 9397
                f = 9397.0
                x = -a + -a
                y = -f + -f

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertNotIn("_GUARD_TOS_INT", uops)
        self.assertNotIn("_GUARD_NOS_INT", uops)
        self.assertNotIn("_GUARD_TOS_FLOAT", uops)
        self.assertNotIn("_GUARD_NOS_FLOAT", uops)

    def test_binary_op_constant_evaluate(self):
        def testfunc(n):
            for _ in range(n):
                2 ** 65

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        # For now... until we constant propagate it away.
        self.assertIn("_BINARY_OP", uops)

    def test_jitted_code_sees_changed_globals(self):
        "Issue 136154: Check that jitted code spots the change in the globals"

        def make_f():
            def f():
                return GLOBAL_136154
            return f

        make_f_with_bad_globals = types.FunctionType(make_f.__code__, {})

        def jitted(funcs):
            for func in funcs:
                func()

        # Make a "good" f:
        f = make_f()
        # Compile jitted for the "good" f:
        jitted([f] * TIER2_THRESHOLD)
        # This "bad" f has different globals, but the *same* code/function versions:
        f_with_bad_globals = make_f_with_bad_globals()
        # A "good" f to enter the JIT code, and a "bad" f to trigger the bug:
        with self.assertRaises(NameError):
            jitted([f, f_with_bad_globals])

    def test_reference_tracking_across_call_doesnt_crash(self):

        def f1():
            for _ in range(TIER2_THRESHOLD + 1):
                # Choose a value that won't occur elsewhere to avoid sharing
                str("value that won't occur elsewhere to avoid sharing")

        f1()

        def f2():
            for _ in range(TIER2_THRESHOLD + 1):
                # Choose a value that won't occur elsewhere to avoid sharing
                tuple((31, -17, 25, "won't occur elsewhere"))

        f2()

    def test_next_instr_for_exception_handler_set(self):
        # gh-140104: We just want the exception to be caught properly.
        def f():
            for i in range(TIER2_THRESHOLD + 3):
                try:
                    undefined_variable(i)
                except Exception:
                    pass

        f()

    def test_next_instr_for_exception_handler_set_lasts_instr(self):
        # gh-140104: We just want the exception to be caught properly.
        def f():
            a_list = []
            for _ in range(TIER2_THRESHOLD + 3):
                try:
                    a_list[""] = 0
                except Exception:
                    pass

        f()

    def test_interpreter_finalization_with_generator_alive(self):
        script_helper.assert_python_ok("-c", textwrap.dedent("""
            import sys
            t = tuple(range(%d))
            def simple_for():
                for x in t:
                    x

            def gen():
                try:
                    yield
                except:
                    simple_for()

            sys.settrace(lambda *args: None)
            simple_for()
            g = gen()
            next(g)
        """ % _testinternalcapi.SPECIALIZATION_THRESHOLD))

    def test_executor_side_exits_create_another_executor(self):
        def f():
            for x in range(TIER2_THRESHOLD + 3):
                for y in range(TIER2_THRESHOLD + 3):
                    z = x + y

        f()
        all_executors = get_all_executors(f)
        # Inner loop warms up first.
        # Outer loop warms up later, linking to the inner one.
        # Therefore, we have at least two executors.
        self.assertGreaterEqual(len(all_executors), 2)
        executor_ids = [id(e) for e in all_executors]
        for executor in all_executors:
            ops = get_ops(executor)
            # Assert all executors first terminator ends in
            # _EXIT_TRACE or _JUMP_TO_TOP, not _DEOPT
            for idx, op in enumerate(ops):
                opname = op[0]
                if opname == "_EXIT_TRACE":
                    # As this is a link outer executor to inner
                    # executor problem, all executors exits should point to
                    # another valid executor. In this case, none of them
                    # should be the cold executor.
                    exit = op[3]
                    link_to = _testinternalcapi.get_exit_executor(exit)
                    self.assertIn(id(link_to), executor_ids)
                    break
                elif opname == "_JUMP_TO_TOP":
                    break
                elif opname == "_DEOPT":
                    self.fail(f"_DEOPT encountered first at executor"
                              f" {executor} at offset {idx} rather"
                              f" than expected _EXIT_TRACE")

    def test_enter_executor_valid_op_arg(self):
        script_helper.assert_python_ok("-c", textwrap.dedent("""
            import sys
            sys.setrecursionlimit(30) # reduce time of the run

            str_v1 = ''
            tuple_v2 = (None, None, None, None, None)
            small_int_v3 = 4

            def f1():

                for _ in range(10):
                    abs(0)

                tuple_v2[small_int_v3]
                tuple_v2[small_int_v3]
                tuple_v2[small_int_v3]

                def recursive_wrapper_4569():
                    str_v1 > str_v1
                    str_v1 > str_v1
                    str_v1 > str_v1
                    recursive_wrapper_4569()

                recursive_wrapper_4569()

            for i_f1 in range(19000):
                try:
                    f1()
                except RecursionError:
                    pass
        """))

    def test_attribute_changes_are_watched(self):
        # Just running to make sure it doesn't crash.
        script_helper.assert_python_ok("-c", textwrap.dedent("""
            from concurrent.futures import ThreadPoolExecutor
            from unittest import TestCase
            NTHREADS = 6
            BOTTOM = 0
            TOP = 1250000
            class A:
                attr = 10**1000
            class TestType(TestCase):
                def read(id0):
                    for _ in range(BOTTOM, TOP):
                        A.attr
                def write(id0):
                    x = A.attr
                    x += 1
                    A.attr = x
                    with ThreadPoolExecutor(NTHREADS) as pool:
                        pool.submit(read, (1,))
                        pool.submit(write, (1,))
        """))

    def test_handling_of_tos_cache_with_side_exits(self):
        # https://github.com/python/cpython/issues/142718
        class EvilAttr:
            def __init__(self, d):
                self.d = d

            def __del__(self):
                try:
                    del self.d['attr']
                except Exception:
                    pass

        class Obj:
            pass

        obj = Obj()
        obj.__dict__ = {}

        for _ in range(TIER2_THRESHOLD+1):
            obj.attr = EvilAttr(obj.__dict__)

    def test_promoted_global_refcount_eliminated(self):
        result = script_helper.run_python_until_end('-c', textwrap.dedent("""
        import _testinternalcapi
        import opcode
        import _opcode

        def get_first_executor(func):
            code = func.__code__
            co_code = code.co_code
            for i in range(0, len(co_code), 2):
                try:
                    return _opcode.get_executor(code, i)
                except ValueError:
                    pass
            return None

        def get_opnames(ex):
            return {item[0] for item in ex}


        def testfunc(n):
            y = []
            for i in range(n):
                x = tuple(y)
            return x

        testfunc(_testinternalcapi.TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        assert ex is not None
        uops = get_opnames(ex)
        assert "_LOAD_GLOBAL_BUILTIN" not in uops
        assert "_LOAD_CONST_INLINE_BORROW" in uops
        assert "_POP_TOP_NOP" in uops
        pop_top_count = len([opname for opname in ex if opname == "_POP_TOP" ])
        assert pop_top_count <= 2
        """), PYTHON_JIT="1")
        self.assertEqual(result[0].rc, 0, result)

    def test_constant_fold_tuple(self):
        def testfunc(n):
            for _ in range(n):
                t = (1,)
                p = len(t)

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertNotIn("_CALL_LEN", uops)

    def test_binary_subscr_list_int(self):
        def testfunc(n):
            l = [1]
            x = 0
            for _ in range(n):
                y = l[0]
                x += y
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_BINARY_OP_SUBSCR_LIST_INT", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)
        self.assertLessEqual(count_ops(ex, "_POP_TOP_INT"), 1)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_binary_subscr_tuple_int(self):
        def testfunc(n):
            t = (1,)
            x = 0
            for _ in range(n):
                y = t[0]
                x += y
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_BINARY_OP_SUBSCR_TUPLE_INT", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 3)
        self.assertLessEqual(count_ops(ex, "_POP_TOP_INT"), 1)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_binary_subscr_frozendict_lowering(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                x += FROZEN_DICT_CONST['x']
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertGreaterEqual(count_ops(ex, "_LOAD_CONST_INLINE_BORROW"), 2)
        self.assertNotIn("_BINARY_OP_SUBSCR_DICT", uops)

    def test_binary_subscr_frozendict_const_fold(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                if FROZEN_DICT_CONST['x'] == 1:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertGreaterEqual(count_ops(ex, "_LOAD_CONST_INLINE_BORROW"), 3)
        # lookup result is folded to constant 1, so comparison is optimized away
        self.assertNotIn("_COMPARE_OP_INT", uops)

    def test_contains_op_frozenset_const_fold(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                if 1 in FROZEN_SET_CONST:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertGreaterEqual(count_ops(ex, "_LOAD_CONST_INLINE_BORROW"), 3)
        self.assertNotIn("_CONTAINS_OP_SET", uops)

    def test_contains_op_frozendict_const_fold(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                if 'x' in FROZEN_DICT_CONST:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertGreaterEqual(count_ops(ex, "_LOAD_CONST_INLINE_BORROW"), 3)
        self.assertNotIn("_CONTAINS_OP_DICT", uops)

    def test_not_contains_op_frozendict_const_fold(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                if 'z' not in FROZEN_DICT_CONST:
                    x += 1
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertGreaterEqual(count_ops(ex, "_LOAD_CONST_INLINE_BORROW"), 3)
        self.assertNotIn("_CONTAINS_OP_DICT", uops)

    def test_binary_subscr_list_slice(self):
        def testfunc(n):
            x = 0
            l = [1, 2, 3]
            for _ in range(n):
                x += l[0:1][0]
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        uops = get_opnames(ex)

        self.assertIn("_BINARY_OP_SUBSCR_LIST_SLICE", uops)
        self.assertNotIn("_GUARD_TOS_LIST", uops)
        self.assertEqual(count_ops(ex, "_POP_TOP"), 2)
        self.assertEqual(count_ops(ex, "_POP_TOP_NOP"), 4)

    def test_is_op(self):
        def test_is_false(n):
            a = object()
            b = object()
            for _ in range(n):
                res = a is b
            return res

        res, ex = self._run_with_optimizer(test_is_false, TIER2_THRESHOLD)
        self.assertEqual(res, False)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_IS_OP", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)


        def test_is_true(n):
            a = object()
            for _ in range(n):
                res = a is a
            return res

        res, ex = self._run_with_optimizer(test_is_true, TIER2_THRESHOLD)
        self.assertEqual(res, True)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_IS_OP", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)


        def test_is_not(n):
            a = object()
            b = object()
            for _ in range(n):
                res = a is not b
            return res

        res, ex = self._run_with_optimizer(test_is_not, TIER2_THRESHOLD)
        self.assertEqual(res, True)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_IS_OP", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)


        def test_is_none(n):
            a = None
            for _ in range(n):
                res = a is None
            return res

        res, ex = self._run_with_optimizer(test_is_none, TIER2_THRESHOLD)
        self.assertEqual(res, True)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_IS_OP", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_is_true_narrows_to_constant(self):
        def f(n):
            def return_true():
                return True

            hits = 0
            v = return_true()
            for i in range(n):
                if v is True:
                    hits += v + 1
            return hits

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD * 2)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        # v + 1 should be constant folded
        self.assertNotIn("_BINARY_OP", uops)

    def test_is_none_narrows_to_constant(self):
        def testfunc(n):
            value = None
            hits = 0
            for _ in range(n):
                if value is None:
                    hits += 1
            return hits

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertNotIn("_IS_NONE", uops)
        self.assertIn("_GUARD_IS_NONE_POP", uops)
        self.assertIn("_POP_TOP_NOP", uops)

    def test_is_false_narrows_to_constant(self):
        def f(n):
            def return_false():
                return False

            hits = 0
            v = return_false()
            for i in range(n):
                if v is False:
                    hits += v + 1
            return hits

        res, ex = self._run_with_optimizer(f, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        # v + 1 should be constant folded
        self.assertNotIn("_BINARY_OP", uops)

    def test_for_iter_gen_frame(self):
        def f(n):
            for i in range(n):
                # Should be optimized to POP_TOP_NOP
                yield i + i
        def testfunc(n):
            for _ in f(n):
                pass

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD*2)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_FOR_ITER_GEN_FRAME", uops)
        # _POP_TOP_NOP is a sign the optimizer ran and didn't hit bottom.
        self.assertGreaterEqual(count_ops(ex, "_POP_TOP_NOP"), 1)

    def test_send_gen_frame(self):

        def gen(n):
            for i in range(n):
                yield i + i
        def send_gen(n):
            yield from gen(n)
        def testfunc(n):
            for _ in send_gen(n):
                pass

        for _ in range(_testinternalcapi.SPECIALIZATION_THRESHOLD):
            # Ensure SEND is specialized to SEND_GEN
            send_gen(10)

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD*2)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_FOR_ITER_GEN_FRAME", uops)
        self.assertIn("_SEND_GEN_FRAME", uops)
        # _POP_TOP_NOP is a sign the optimizer ran and didn't hit bottom.
        self.assertGreaterEqual(count_ops(ex, "_POP_TOP_NOP"), 1)

    def test_binary_op_subscr_init_frame(self):
        class B:
            def __getitem__(self, other):
                return other + 1
        def testfunc(*args):
            n, b = args[0]
            for _ in range(n):
                y = b[2]

        res, ex = self._run_with_optimizer(testfunc, (TIER2_THRESHOLD, B()))
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_BINARY_OP_SUBSCR_INIT_CALL", uops)
        # _POP_TOP_NOP is a sign the optimizer ran and didn't hit contradiction.
        self.assertGreaterEqual(count_ops(ex, "_POP_TOP_NOP"), 1)

    def test_load_attr_property_frame(self):
        class B:
            @property
            def prop(self):
                return 3
        def testfunc(*args):
            n, b = args[0]
            for _ in range(n):
                y = b.prop + b.prop

        testfunc((3, B()))
        res, ex = self._run_with_optimizer(testfunc, (TIER2_THRESHOLD, B()))
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_LOAD_ATTR_PROPERTY_FRAME", uops)
        # This is a sign the optimizer ran and didn't hit contradiction.
        self.assertIn("_LOAD_CONST_INLINE_BORROW", uops)

    def test_load_attr_getattribute_frame(self):
        class B:
            def __getattribute__(self, name):
                return len(name)

        def testfunc(n):
            b = B()
            y = 0
            for _ in range(n):
                y += b.x + b.y
            return y

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        self.assertEqual(res, 2 * TIER2_THRESHOLD)
        uops = get_opnames(ex)
        self.assertIn("_LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN_FRAME", uops)
        self.assertNotIn("_LOAD_GLOBAL_BUILTINS", uops)

    def test_load_attr_property_frame_invalidates_on_code_change(self):
        class C:
            @property
            def val(self):
                return int(1)

        fget = C.val.fget

        def testfunc(*args):
            n, c = args[0]
            total = 0
            for _ in range(n):
                total += c.val
            return total

        testfunc((3, C()))
        res, ex = self._run_with_optimizer(testfunc, (TIER2_THRESHOLD, C()))
        self.assertEqual(res, TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_LOAD_ATTR_PROPERTY_FRAME", uops)
        # Check the optimizer traced through the property call.
        self.assertNotIn("_LOAD_GLOBAL_BUILTINS", uops)
        self.assertIn("_CALL_BUILTIN_CLASS", uops)

        fget.__code__ = (lambda self: 2).__code__
        _testinternalcapi.clear_executor_deletion_list()
        ex = get_first_executor(testfunc)
        self.assertIsNone(ex)
        res = testfunc((TIER2_THRESHOLD, C()))
        self.assertEqual(res, TIER2_THRESHOLD * 2)

    def test_unary_negative(self):
        def testfunc(n):
            a = 3
            for _ in range(n):
                res = -a
            return res

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, -3)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_UNARY_NEGATIVE", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_unary_invert(self):
        def testfunc(n):
            a = 3
            for _ in range(n):
                res = ~a
            return res

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, -4)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)

        self.assertIn("_UNARY_INVERT", uops)
        self.assertIn("_POP_TOP_NOP", uops)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_make_function(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                func = lambda: 1
                x += func()
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        uops = get_opnames(ex)

        self.assertIn("_MAKE_FUNCTION", uops)
        self.assertEqual(uops.count("_POP_TOP_NOP"), 2)

    def test_iter_check_list(self):
        def testfunc(n):
            x = 0
            for _ in range(n):
                l = [1]
                for num in l: # unguarded
                    x += num
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        uops = get_opnames(ex)

        self.assertIn("_BUILD_LIST", uops)
        self.assertNotIn("_ITER_CHECK_LIST", uops)

    def test_match_class(self):
        def testfunc(n):
            class A:
                val = 1
            x = A()
            ret = 0
            for _ in range(n):
                match x:
                    case A():
                        ret += x.val
            return ret

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, TIER2_THRESHOLD)
        uops = get_opnames(ex)

        self.assertIn("_MATCH_CLASS", uops)
        self.assertEqual(count_ops(ex, "_POP_TOP_NOP"), 4)

    def test_dict_update(self):
        def testfunc(n):
            d = {1: 2, 3: 4}
            for _ in range(n):
                x = {**d}
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, {1: 2, 3: 4})
        uops = get_opnames(ex)

        self.assertIn("_DICT_UPDATE", uops)
        self.assertEqual(count_ops(ex, "_POP_TOP_NOP"), 1)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_set_update(self):
        def testfunc(n):
            s = {1, 2, 3}
            for _ in range(n):
                x = {*s}
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, {1, 2, 3})
        uops = get_opnames(ex)

        self.assertIn("_SET_UPDATE", uops)
        self.assertEqual(count_ops(ex, "_POP_TOP_NOP"), 1)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_dict_merge(self):
        def testfunc(n):
            d = {"a": 1, "b": 2}
            def f(**kwargs):
                return kwargs
            for _ in range(n):
                x = f(**d)
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, {"a": 1, "b": 2})
        uops = get_opnames(ex)

        self.assertIn("_DICT_MERGE", uops)
        self.assertGreaterEqual(count_ops(ex, "_POP_TOP_NOP"), 1)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_list_extend(self):
        def testfunc(n):
            a = [1, 2, 3]
            for _ in range(n):
                x = [*a]
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, [1, 2, 3])
        uops = get_opnames(ex)

        self.assertIn("_LIST_EXTEND", uops)
        self.assertGreaterEqual(count_ops(ex, "_POP_TOP_NOP"), 1)
        self.assertLessEqual(count_ops(ex, "_POP_TOP"), 2)

    def test_143026(self):
        # https://github.com/python/cpython/issues/143026

        result = script_helper.run_python_until_end('-c', textwrap.dedent("""
        import gc
        thresholds = gc.get_threshold()
        try:
            gc.set_threshold(1)

            def f1():
                for i in range(5000):
                    globals()[''] = i

            f1()
        finally:
            gc.set_threshold(*thresholds)
        """), PYTHON_JIT="1")
        self.assertEqual(result[0].rc, 0, result)

    def test_143092(self):
        def f1():
            a = "a"
            for i in range(50):
                x = a[i % len(a)]

            s = ""
            for _ in range(10):
                s += ""

            class A: ...
            class B: ...

            match s:
                case int(): ...
                case str(): ...
                case dict(): ...

            (
                u0,
                *u1,
                u2,
                u4,
                u5,
                u6,
                u7,
                u8,
                u9, u10, u11,
                u12, u13, u14, u15, u16, u17, u18, u19, u20, u21, u22, u23, u24, u25, u26, u27, u28, u29,
            ) = [None, None, None, None, None, None, None, None, None, None, None, None, None, None, None,
                 None, None, None, None, None, None, None, None, None, None, None, None, None, None, None,
                 None, None, None, None, None, None, None, None, None, None, None, None, None, None, None,
                 None, None, None, None, None, None, None, None,]

            s = ""
            for _ in range(10):
                s += ""
                s += ""

        for i in range(TIER2_THRESHOLD * 10):
            f1()

    def test_143183(self):
        # https://github.com/python/cpython/issues/143183

        result = script_helper.run_python_until_end('-c', textwrap.dedent(f"""
        def f1():
            class AsyncIter:
                def __init__(self):
                    self.limit = 0
                    self.count = 0

                def __aiter__(self):
                    return self

                async def __anext__(self):
                    if self.count >= self.limit:
                        ...
                    self.count += 1j

            class AsyncCtx:
                async def async_for_driver():
                    try:
                        for _ in range({TIER2_THRESHOLD}):
                            try:
                                async for _ in AsyncIter():
                                    ...
                            except TypeError:
                                ...
                    except Exception:
                        ...

                c = async_for_driver()
                while True:
                    try:
                        c.send(None)
                    except StopIteration:
                        break

        for _ in range({TIER2_THRESHOLD // 40}):
            f1()
        """), PYTHON_JIT="1")
        self.assertEqual(result[0].rc, 0, result)

    def test_143358(self):
        # https://github.com/python/cpython/issues/143358

        result = script_helper.run_python_until_end('-c', textwrap.dedent(f"""
        def f1():

            class EvilIterator:

                def __init__(self):
                    self._items = [1, 2]
                    self._index = 1

                def __iter__(self):
                    return self

                def __next__(self):
                    if not len(self._items) % 13:
                        self._items.clear()

                    for i_loop_9279 in range(10):
                        self._items.extend([1, "", None])

                    if not len(self._items) % 11:
                        return 'unexpected_type_from_iterator'

                    if self._index >= len(self._items):
                        raise StopIteration

                    item = self._items[self._index]
                    self._index += 1
                    return item

            evil_iter = EvilIterator()

            large_num = 2**31
            for _ in range(400):
                try:
                    _ = [x + y for x in evil_iter for y in evil_iter if evil_iter._items.append(x) or large_num]
                except TypeError:
                    pass

        f1()
        """), PYTHON_JIT="1", PYTHON_JIT_STRESS="1")
        self.assertEqual(result[0].rc, 0, result)

    def test_144068_daemon_thread_jit_cleanup(self):
        result = script_helper.run_python_until_end('-c', textwrap.dedent("""
        import threading
        import time

        def hot_loop():
            end = time.time() + 5.0
            while time.time() < end:
                pass

        # Create a daemon thread that will be abandoned at shutdown
        t = threading.Thread(target=hot_loop, daemon=True)
        t.start()

        time.sleep(0.1)
        """), PYTHON_JIT="1", ASAN_OPTIONS="detect_leaks=1")
        self.assertEqual(result[0].rc, 0, result)
        stderr = result[0].err.decode('utf-8', errors='replace')
        self.assertNotIn('LeakSanitizer', stderr,
                         f"Memory leak detected by ASan:\n{stderr}")
        self.assertNotIn('_PyJit_TryInitializeTracing', stderr,
                         f"JIT tracer memory leak detected:\n{stderr}")

    def test_cold_exit_on_init_cleanup_frame(self):

        result = script_helper.run_python_until_end('-c', textwrap.dedent("""
        class A:
            __slots__ = ('x', 'y', 'z', 'w')
            def __init__(self):
                self.x = self.y = -1
                self.z = self.w = None

        class B(A):
            __slots__ = ('a', 'b', 'c', 'd', 'e')
            def __init__(self):
                super().__init__()
                self.a = self.b = None
                self.c = ""
                self.d = self.e = False

        class C(B):
            __slots__ = ('name', 'flag')
            def __init__(self, name):
                super().__init__()
                self.name = name
                self.flag = False

        funcs = []
        for n in range(20, 80):
            lines = [f"def f{n}(names, info):"]
            for j in range(n):
                lines.append(f"    v{j} = names[{j % 3}]")
                if j % 3 == 0:
                    lines.append(f"    if v{j} in info:")
                    lines.append(f"        v{j} = info[v{j}]")
                elif j % 5 == 0:
                    lines.append(f"    v{j} = len(v{j}) if isinstance(v{j}, str) else 0")
            lines.append("    return C(names[0])")
            ns = {'C': C}
            exec("\\n".join(lines), ns)
            funcs.append(ns[f"f{n}"])

        names = ['alpha', 'beta', 'gamma']
        info = {'alpha': 'x', 'beta': 'y', 'gamma': 'z'}

        for f in funcs:
            for _ in range(10):
                f(names, info)
        """), PYTHON_JIT="1", PYTHON_JIT_STRESS="1",
             PYTHON_JIT_SIDE_EXIT_INITIAL_VALUE="1")
        self.assertEqual(result[0].rc, 0, result)

    def test_for_iter_gen_cleared_frame_does_not_crash(self):
        # See: https://github.com/python/cpython/issues/145197
        result = script_helper.run_python_until_end('-c', textwrap.dedent("""
        def g():
            yield 1
            yield 2

        for _ in range(4002):
            for _ in g():
                pass

        for i in range(4002):
            it = g()
            if (i & 7) == 0:
                next(it)
                it.close()
            for _ in it:
                pass
        """),
        PYTHON_JIT="1", PYTHON_JIT_STRESS="1")
        self.assertEqual(result[0].rc, 0, result)

    def test_call_kw(self):
        def func(a):
            return int(a) * 42

        def testfunc(n):
            x = 0
            for _ in range(n):
                x += func(a=1)
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 42 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_PUSH_FRAME", uops)
        self.assertIn("_CHECK_FUNCTION_VERSION_KW", uops)
        # Check the optimizer has optmized through the function call
        # by promoting global `int` to a constant.
        self.assertNotIn("_LOAD_GLOBAL_BUILTINS", uops)
        self.assertIn("_CALL_BUILTIN_CLASS", uops)

    def test_call_kw_bound_method(self):
        class C:
            def method(self, a, b):
                return int(a) + int(b)

        def testfunc(n):
            obj = C()
            x = 0
            meth = obj.method
            for _ in range(n):
                x += meth(a=1, b=2)
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 3 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_PUSH_FRAME", uops)
        self.assertIn("_CHECK_METHOD_VERSION_KW", uops)
        # Check the optimizer has optmized through the function call
        # by promoting global `int` to a constant.
        self.assertNotIn("_LOAD_GLOBAL_BUILTINS", uops)
        self.assertIn("_CALL_BUILTIN_CLASS", uops)

    def test_func_version_guarded_on_change(self):
        def testfunc(n):
            for i in range(n):
                # Only works on functions promoted to constants
                global_identity_code_will_be_modified(i)

        testfunc(TIER2_THRESHOLD)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertIn("_PUSH_FRAME", uops)
        self.assertIn("_CHECK_FUNCTION_VERSION", uops)

        global_identity_code_will_be_modified.__code__ = (lambda a: 0xdeadead).__code__
        _testinternalcapi.clear_executor_deletion_list()
        ex = get_first_executor(testfunc)
        self.assertIsNone(ex)
        # JItted code should've deopted.
        self.assertEqual(global_identity_code_will_be_modified(None), 0xdeadead)

    def test_call_super(self):
        class A:
            def method1(self):
                return 42

            def method2(self):
                return 21

        class B(A):
            def method1(self):
                return super().method1()

            def method2(self):
                return super(B, self).method2()

        b = B()

        def testfunc(n):
            x = 0
            for _ in range(n):
                x += b.method1()
                x += b.method2()
            return x

        res, ex = self._run_with_optimizer(testfunc, TIER2_THRESHOLD)
        self.assertEqual(res, 63 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_LOAD_SUPER_ATTR_METHOD", uops)
        self.assertEqual(uops.count("_GUARD_NOS_TYPE_VERSION"), 2)
        self.assertTrue(ex.is_valid())
        # this should change the type version of A, which should invalidate the executor
        A.method1 = lambda self: 1
        self.assertFalse(ex.is_valid())
        # re-running should create a new executor
        res, ex = self._run_with_optimizer(testfunc, 4 * TIER2_THRESHOLD)
        self.assertEqual(res, 4 * 22 * TIER2_THRESHOLD)
        self.assertIsNotNone(ex)
        uops = get_opnames(ex)
        self.assertNotIn("_LOAD_SUPER_ATTR_METHOD", uops)
        self.assertEqual(uops.count("_GUARD_NOS_TYPE_VERSION"), 2)

    def test_settrace_then_polymorphic_call_does_not_crash(self):
        script_helper.assert_python_ok("-c", textwrap.dedent("""
            import sys
            sys.settrace(lambda *_: None)
            sys.settrace(None)

            class C:
                def __init__(self, x):
                    pass

            for i in 0, 1, 0, 1:
                C(0) if i else str(0)
        """))

def global_identity(x):
    return x

def global_identity_code_will_be_modified(x):
    return x

class TestObject:
    def test(self, *args, **kwargs):
        return args[0]

test_object = TestObject()
test_bound_method = TestObject.test.__get__(test_object)

class MyGlobalPoint:
    def __init__(self, x, y):
        return None

if __name__ == "__main__":
    unittest.main()
