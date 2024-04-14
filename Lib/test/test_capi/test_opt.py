import contextlib
import opcode
import textwrap
import unittest

import _testinternalcapi


@contextlib.contextmanager
def temporary_optimizer(opt):
    old_opt = _testinternalcapi.get_optimizer()
    _testinternalcapi.set_optimizer(opt)
    try:
        yield
    finally:
        _testinternalcapi.set_optimizer(old_opt)


@contextlib.contextmanager
def clear_executors(func):
    # Clear executors in func before and after running a block
    func.__code__ = func.__code__.replace()
    try:
        yield
    finally:
        func.__code__ = func.__code__.replace()


class TestOptimizerAPI(unittest.TestCase):

    def test_get_counter_optimizer_dealloc(self):
        # See gh-108727
        def f():
            _testinternalcapi.get_counter_optimizer()

        f()

    def test_get_set_optimizer(self):
        old = _testinternalcapi.get_optimizer()
        opt = _testinternalcapi.get_counter_optimizer()
        try:
            _testinternalcapi.set_optimizer(opt)
            self.assertEqual(_testinternalcapi.get_optimizer(), opt)
            _testinternalcapi.set_optimizer(None)
            self.assertEqual(_testinternalcapi.get_optimizer(), None)
        finally:
            _testinternalcapi.set_optimizer(old)


    def test_counter_optimizer(self):
        # Generate a new function at each call
        ns = {}
        exec(textwrap.dedent("""
            def loop():
                for _ in range(1000):
                    pass
        """), ns, ns)
        loop = ns['loop']

        for repeat in range(5):
            opt = _testinternalcapi.get_counter_optimizer()
            with temporary_optimizer(opt):
                self.assertEqual(opt.get_count(), 0)
                with clear_executors(loop):
                    loop()
                self.assertEqual(opt.get_count(), 1000)

    def test_long_loop(self):
        "Check that we aren't confused by EXTENDED_ARG"

        # Generate a new function at each call
        ns = {}
        exec(textwrap.dedent("""
            def nop():
                pass

            def long_loop():
                for _ in range(10):
                    nop(); nop(); nop(); nop(); nop(); nop(); nop(); nop();
                    nop(); nop(); nop(); nop(); nop(); nop(); nop(); nop();
                    nop(); nop(); nop(); nop(); nop(); nop(); nop(); nop();
                    nop(); nop(); nop(); nop(); nop(); nop(); nop(); nop();
                    nop(); nop(); nop(); nop(); nop(); nop(); nop(); nop();
                    nop(); nop(); nop(); nop(); nop(); nop(); nop(); nop();
                    nop(); nop(); nop(); nop(); nop(); nop(); nop(); nop();
        """), ns, ns)
        long_loop = ns['long_loop']

        opt = _testinternalcapi.get_counter_optimizer()
        with temporary_optimizer(opt):
            self.assertEqual(opt.get_count(), 0)
            long_loop()
            self.assertEqual(opt.get_count(), 10)

    def test_code_restore_for_ENTER_EXECUTOR(self):
        def testfunc(x):
            i = 0
            while i < x:
                i += 1

        opt = _testinternalcapi.get_counter_optimizer()
        with temporary_optimizer(opt):
            testfunc(1000)
            code, replace_code  = testfunc.__code__, testfunc.__code__.replace()
            self.assertEqual(code, replace_code)
            self.assertEqual(hash(code), hash(replace_code))


def get_first_executor(func):
    code = func.__code__
    co_code = code.co_code
    JUMP_BACKWARD = opcode.opmap["JUMP_BACKWARD"]
    for i in range(0, len(co_code), 2):
        if co_code[i] == JUMP_BACKWARD:
            try:
                return _testinternalcapi.get_executor(code, i)
            except ValueError:
                pass
    return None


class TestExecutorInvalidation(unittest.TestCase):

    def setUp(self):
        self.old = _testinternalcapi.get_optimizer()
        self.opt = _testinternalcapi.get_counter_optimizer()
        _testinternalcapi.set_optimizer(self.opt)

    def tearDown(self):
        _testinternalcapi.set_optimizer(self.old)

    def test_invalidate_object(self):
        # Generate a new set of functions at each call
        ns = {}
        func_src = "\n".join(
            f"""
            def f{n}():
                for _ in range(1000):
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
        # an executor mutliple times.
        for i in (4,3,2,1,0):
            _testinternalcapi.invalidate_executors(objects[i])
            for exe in executors[i:]:
                self.assertFalse(exe.is_valid())
            for exe in executors[:i]:
                self.assertTrue(exe.is_valid())

    def test_uop_optimizer_invalidation(self):
        # Generate a new function at each call
        ns = {}
        exec(textwrap.dedent("""
            def f():
                for i in range(1000):
                    pass
        """), ns, ns)
        f = ns['f']
        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            f()
        exe = get_first_executor(f)
        self.assertIsNotNone(exe)
        self.assertTrue(exe.is_valid())
        _testinternalcapi.invalidate_executors(f.__code__)
        self.assertFalse(exe.is_valid())

class TestUops(unittest.TestCase):

    def test_basic_loop(self):
        def testfunc(x):
            i = 0
            while i < x:
                i += 1

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            testfunc(1000)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = {opname for opname, _, _ in ex}
        self.assertIn("_SET_IP", uops)
        self.assertIn("_LOAD_FAST", uops)

    def test_extended_arg(self):
        "Check EXTENDED_ARG handling in superblock creation"
        ns = {}
        exec(textwrap.dedent("""
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
                z0 = z1 = z2 = z3 = z4 = z5 = z6 = z7 = z8 = z9 = 42
                while z9 > 0:
                    z9 = z9 - 1
        """), ns, ns)
        many_vars = ns["many_vars"]

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            ex = get_first_executor(many_vars)
            self.assertIsNone(ex)
            many_vars()

        ex = get_first_executor(many_vars)
        self.assertIsNotNone(ex)
        self.assertIn(("_LOAD_FAST", 259, 0), list(ex))

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

        opt = _testinternalcapi.get_uop_optimizer()

        with temporary_optimizer(opt):
            testfunc(20)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = {opname for opname, _, _ in ex}
        self.assertIn("_UNPACK_SEQUENCE", uops)

    def test_pop_jump_if_false(self):
        def testfunc(n):
            i = 0
            while i < n:
                i += 1

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            testfunc(20)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = {opname for opname, _, _ in ex}
        self.assertIn("_GUARD_IS_TRUE_POP", uops)

    def test_pop_jump_if_none(self):
        def testfunc(a):
            for x in a:
                if x is None:
                    x = 0

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            testfunc(range(20))

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = {opname for opname, _, _ in ex}
        self.assertIn("_GUARD_IS_NOT_NONE_POP", uops)

    def test_pop_jump_if_not_none(self):
        def testfunc(a):
            for x in a:
                x = None
                if x is not None:
                    x = 0

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            testfunc(range(20))

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = {opname for opname, _, _ in ex}
        self.assertIn("_GUARD_IS_NONE_POP", uops)

    def test_pop_jump_if_true(self):
        def testfunc(n):
            i = 0
            while not i >= n:
                i += 1

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            testfunc(20)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = {opname for opname, _, _ in ex}
        self.assertIn("_GUARD_IS_FALSE_POP", uops)

    def test_jump_backward(self):
        def testfunc(n):
            i = 0
            while i < n:
                i += 1

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            testfunc(20)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = {opname for opname, _, _ in ex}
        self.assertIn("_JUMP_TO_TOP", uops)

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

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            testfunc(20)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = {opname for opname, _, _ in ex}
        # Since there is no JUMP_FORWARD instruction,
        # look for indirect evidence: the += operator
        self.assertIn("_BINARY_OP_ADD_INT", uops)

    def test_for_iter_range(self):
        def testfunc(n):
            total = 0
            for i in range(n):
                total += i
            return total

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            total = testfunc(20)
            self.assertEqual(total, 190)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        # for i, (opname, oparg) in enumerate(ex):
        #     print(f"{i:4d}: {opname:<20s} {oparg:3d}")
        uops = {opname for opname, _, _ in ex}
        self.assertIn("_GUARD_NOT_EXHAUSTED_RANGE", uops)
        # Verification that the jump goes past END_FOR
        # is done by manual inspection of the output

    def test_for_iter_list(self):
        def testfunc(a):
            total = 0
            for i in a:
                total += i
            return total

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            a = list(range(20))
            total = testfunc(a)
            self.assertEqual(total, 190)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        # for i, (opname, oparg) in enumerate(ex):
        #     print(f"{i:4d}: {opname:<20s} {oparg:3d}")
        uops = {opname for opname, _, _ in ex}
        self.assertIn("_GUARD_NOT_EXHAUSTED_LIST", uops)
        # Verification that the jump goes past END_FOR
        # is done by manual inspection of the output

    def test_for_iter_tuple(self):
        def testfunc(a):
            total = 0
            for i in a:
                total += i
            return total

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            a = tuple(range(20))
            total = testfunc(a)
            self.assertEqual(total, 190)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        # for i, (opname, oparg) in enumerate(ex):
        #     print(f"{i:4d}: {opname:<20s} {oparg:3d}")
        uops = {opname for opname, _, _ in ex}
        self.assertIn("_GUARD_NOT_EXHAUSTED_TUPLE", uops)
        # Verification that the jump goes past END_FOR
        # is done by manual inspection of the output

    def test_list_edge_case(self):
        def testfunc(it):
            for x in it:
                pass

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
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

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            testfunc(20)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = {opname for opname, _, _ in ex}
        self.assertIn("_PUSH_FRAME", uops)
        self.assertIn("_BINARY_OP_ADD_INT", uops)

    def test_branch_taken(self):
        def testfunc(n):
            for i in range(n):
                if i < 0:
                    i = 0
                else:
                    i = 1

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            testfunc(20)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = {opname for opname, _, _ in ex}
        self.assertIn("_GUARD_IS_FALSE_POP", uops)

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
                    x += 1000*i + j
            return x

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            x = testfunc(10, 10)

        self.assertEqual(x, sum(range(10)) * 10010)

        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        uops = {opname for opname, _, _ in ex}
        self.assertIn("_FOR_ITER_TIER_TWO", uops)

    def test_confidence_score(self):
        def testfunc(n):
            bits = 0
            for i in range(n):
                if i & 0x01:
                    bits += 1
                if i & 0x02:
                    bits += 1
                if i&0x04:
                    bits += 1
                if i&0x08:
                    bits += 1
                if i&0x10:
                    bits += 1
                if i&0x20:
                    bits += 1
            return bits

        opt = _testinternalcapi.get_uop_optimizer()
        with temporary_optimizer(opt):
            x = testfunc(20)

        self.assertEqual(x, 40)
        ex = get_first_executor(testfunc)
        self.assertIsNotNone(ex)
        ops = [opname for opname, _, _ in ex]
        count = ops.count("_GUARD_IS_TRUE_POP")
        # Because Each 'if' halves the score, the second branch is
        # too much already.
        self.assertEqual(count, 1)


if __name__ == "__main__":
    unittest.main()
