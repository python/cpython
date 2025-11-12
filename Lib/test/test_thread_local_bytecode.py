"""Tests for thread-local bytecode."""
import textwrap
import unittest

from test import support
from test.support import cpython_only, import_helper, requires_specialization_ft
from test.support.script_helper import assert_python_ok
from test.support.threading_helper import requires_working_threading

# Skip this test if the _testinternalcapi module isn't available
_testinternalcapi = import_helper.import_module("_testinternalcapi")


@cpython_only
@requires_working_threading()
@unittest.skipUnless(support.Py_GIL_DISABLED, "only in free-threaded builds")
class TLBCTests(unittest.TestCase):
    @requires_specialization_ft
    def test_new_threads_start_with_unspecialized_code(self):
        code = textwrap.dedent("""
        import dis
        import queue
        import threading

        from _testinternalcapi import get_tlbc

        def all_opnames(bc):
            return {i.opname for i in dis._get_instructions_bytes(bc)}

        def f(a, b, q=None):
            if q is not None:
                q.put(get_tlbc(f))
            return a + b

        for _ in range(100):
            # specialize
            f(1, 2)

        q = queue.Queue()
        t = threading.Thread(target=f, args=('a', 'b', q))
        t.start()
        t.join()

        assert "BINARY_OP_ADD_INT" in all_opnames(get_tlbc(f))
        assert "BINARY_OP_ADD_INT" not in all_opnames(q.get())
        """)
        assert_python_ok("-X", "tlbc=1", "-c", code)

    @requires_specialization_ft
    def test_threads_specialize_independently(self):
        code = textwrap.dedent("""
        import dis
        import queue
        import threading

        from _testinternalcapi import get_tlbc

        def all_opnames(bc):
            return {i.opname for i in dis._get_instructions_bytes(bc)}

        def f(a, b):
            return a + b

        def g(a, b, q=None):
            for _ in range(100):
                f(a, b)
            if q is not None:
                q.put(get_tlbc(f))

        # specialize in main thread
        g(1, 2)

        # specialize in other thread
        q = queue.Queue()
        t = threading.Thread(target=g, args=('a', 'b', q))
        t.start()
        t.join()

        assert "BINARY_OP_ADD_INT" in all_opnames(get_tlbc(f))
        t_opnames = all_opnames(q.get())
        assert "BINARY_OP_ADD_INT" not in t_opnames
        assert "BINARY_OP_ADD_UNICODE" in t_opnames
        """)
        assert_python_ok("-X", "tlbc=1", "-c", code)

    def test_reuse_tlbc_across_threads_different_lifetimes(self):
        code = textwrap.dedent("""
        import queue
        import threading

        from _testinternalcapi import get_tlbc_id

        def f(a, b, q=None):
            if q is not None:
                q.put(get_tlbc_id(f))
            return a + b

        q = queue.Queue()
        tlbc_ids = []
        for _ in range(3):
            t = threading.Thread(target=f, args=('a', 'b', q))
            t.start()
            t.join()
            tlbc_ids.append(q.get())

        assert tlbc_ids[0] == tlbc_ids[1]
        assert tlbc_ids[1] == tlbc_ids[2]
        """)
        assert_python_ok("-X", "tlbc=1", "-c", code)

    @support.skip_if_sanitizer("gh-129752: data race on adaptive counter", thread=True)
    def test_no_copies_if_tlbc_disabled(self):
        code = textwrap.dedent("""
        import queue
        import threading

        from _testinternalcapi import get_tlbc_id

        def f(a, b, q=None):
            if q is not None:
                q.put(get_tlbc_id(f))
            return a + b

        q = queue.Queue()
        threads = []
        for _ in range(3):
            t = threading.Thread(target=f, args=('a', 'b', q))
            t.start()
            threads.append(t)

        tlbc_ids = []
        for t in threads:
            t.join()
            tlbc_ids.append(q.get())

        main_tlbc_id = get_tlbc_id(f)
        assert main_tlbc_id is not None
        assert tlbc_ids[0] == main_tlbc_id
        assert tlbc_ids[1] == main_tlbc_id
        assert tlbc_ids[2] == main_tlbc_id
        """)
        assert_python_ok("-X", "tlbc=0", "-c", code)

    def test_no_specialization_if_tlbc_disabled(self):
        code = textwrap.dedent("""
        import dis
        import queue
        import threading

        from _testinternalcapi import get_tlbc

        def all_opnames(f):
            bc = get_tlbc(f)
            return {i.opname for i in dis._get_instructions_bytes(bc)}

        def f(a, b):
            return a + b

        for _ in range(100):
            f(1, 2)

        assert "BINARY_OP_ADD_INT" not in all_opnames(f)
        """)
        assert_python_ok("-X", "tlbc=0", "-c", code)

    def test_generator_throw(self):
        code = textwrap.dedent("""
        import queue
        import threading

        from _testinternalcapi import get_tlbc_id

        def g():
            try:
                yield
            except:
                yield get_tlbc_id(g)

        def f(q):
            gen = g()
            next(gen)
            q.put(gen.throw(ValueError))

        q = queue.Queue()
        t = threading.Thread(target=f, args=(q,))
        t.start()
        t.join()

        gen = g()
        next(gen)
        main_id = gen.throw(ValueError)
        assert main_id != q.get()
        """)
        assert_python_ok("-X", "tlbc=1", "-c", code)


if __name__ == "__main__":
    unittest.main()
