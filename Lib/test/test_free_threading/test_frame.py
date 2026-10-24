import functools
import sys
import threading
import unittest

from test.support import import_helper, threading_helper

threading_helper.requires_working_threading(module=True)


def run_with_frame(funcs, runner=None, iters=10):
    """Run funcs with a frame from another thread that is currently executing.

    Args:
        funcs: A function or list of functions that take a frame argument
        runner: Optional function to run in the executor thread. If provided,
                it will be called and should return eventually. The frame
                passed to funcs will be the runner's frame.
        iters: Number of iterations each func should run
    """
    if not isinstance(funcs, list):
        funcs = [funcs]

    frame_var = None
    e = threading.Event()
    b = threading.Barrier(len(funcs) + 1)

    if runner is None:
        def runner():
            j = 0
            for i in range(100):
                j += i

    def executor():
        nonlocal frame_var
        frame_var = sys._getframe()
        e.set()
        b.wait()
        runner()

    def func_wrapper(func):
        e.wait()
        frame = frame_var
        b.wait()
        for _ in range(iters):
            func(frame)

    test_funcs = [functools.partial(func_wrapper, f) for f in funcs]
    threading_helper.run_concurrently([executor] + test_funcs)


class TestFrameRaces(unittest.TestCase):
    def test_concurrent_f_lasti(self):
        run_with_frame(lambda frame: frame.f_lasti)

    def test_concurrent_f_lineno(self):
        run_with_frame(lambda frame: frame.f_lineno)

    def test_concurrent_f_code(self):
        run_with_frame(lambda frame: frame.f_code)

    def test_concurrent_f_back(self):
        run_with_frame(lambda frame: frame.f_back)

    def test_concurrent_f_globals(self):
        run_with_frame(lambda frame: frame.f_globals)

    def test_concurrent_f_builtins(self):
        run_with_frame(lambda frame: frame.f_builtins)

    def test_concurrent_f_locals(self):
        run_with_frame(lambda frame: frame.f_locals)

    def test_concurrent_f_trace_read(self):
        run_with_frame(lambda frame: frame.f_trace)

    def test_concurrent_f_trace_opcodes_read(self):
        run_with_frame(lambda frame: frame.f_trace_opcodes)

    def test_concurrent_repr(self):
        run_with_frame(lambda frame: repr(frame))

    def test_concurrent_f_trace_write(self):
        def trace_func(frame, event, arg):
            return trace_func

        def writer(frame):
            frame.f_trace = trace_func
            frame.f_trace = None

        run_with_frame(writer)

    def test_concurrent_f_trace_read_write(self):
        # Test concurrent reads and writes of f_trace on a live frame.
        def trace_func(frame, event, arg):
            return trace_func

        def reader(frame):
            _ = frame.f_trace

        def writer(frame):
            frame.f_trace = trace_func
            frame.f_trace = None

        run_with_frame([reader, writer, reader, writer])

    def test_concurrent_f_trace_opcodes_write(self):
        def writer(frame):
            frame.f_trace_opcodes = True
            frame.f_trace_opcodes = False

        run_with_frame(writer)

    def test_concurrent_f_trace_opcodes_read_write(self):
        # Test concurrent reads and writes of f_trace_opcodes on a live frame.
        def reader(frame):
            _ = frame.f_trace_opcodes

        def writer(frame):
            frame.f_trace_opcodes = True
            frame.f_trace_opcodes = False

        run_with_frame([reader, writer, reader, writer])

    def test_concurrent_f_locals_read_values(self):
        def runner():
            a = 1
            b = "hello"
            c = [1, 2, 3]
            for i in range(100):
                a += i

        def reader(frame):
            locals_dict = frame.f_locals
            list(locals_dict.keys())
            list(locals_dict.values())

        run_with_frame(reader, runner=runner)

    def test_concurrent_f_locals_write(self):
        def runner():
            x = 0
            for i in range(100):
                x += i

        def writer(frame):
            frame.f_locals["new_var"] = 42

        run_with_frame(writer, runner=runner)

    def test_concurrent_f_locals_read_write(self):
        def runner():
            a = 1
            b = 2
            for i in range(100):
                a += i

        def reader(frame):
            _ = frame.f_locals.get("a")
            _ = frame.f_locals.get("b")

        def writer(frame):
            frame.f_locals["a"] = 42

        run_with_frame([reader, writer, reader, writer], runner=runner)

    def test_concurrent_f_locals_iteration(self):
        def runner():
            a = 1
            b = "hello"
            c = [1, 2, 3]
            for i in range(100):
                a += i

        def iterator(frame):
            for key, value in frame.f_locals.items():
                pass

        run_with_frame(iterator, runner=runner)

    def test_gen_f_locals_read_while_running(self):
        # gh-144446: reading f_locals of a generator frame while the
        # generator is executing on another thread.
        for _ in range(5):
            def gen_fn():
                x = 0
                obj = None
                s = None
                yield
                for i in range(2000):
                    obj = [i] * 4
                    s = str(i) * 8
                    x += i
                yield x

            g = gen_fn()
            next(g)
            frame = g.gi_frame
            barrier = threading.Barrier(3)

            def runner():
                barrier.wait()
                next(g)

            def reader():
                barrier.wait()
                for _ in range(100):
                    fl = frame.f_locals
                    list(fl.values())
                    fl.get("obj")
                    fl.get("s")
                    len(fl)

            threading_helper.run_concurrently([runner, reader, reader])
            g.close()

    def test_gen_f_locals_vs_resume_cycle(self):
        # Concurrent f_locals access must not make a concurrent send()
        # spuriously fail with "already executing".
        for _ in range(5):
            def gen_fn():
                x = 0
                while True:
                    x += 1
                    yield x

            g = gen_fn()
            next(g)
            frame = g.gi_frame
            barrier = threading.Barrier(3)

            def runner():
                barrier.wait()
                for _ in range(1000):
                    next(g)

            def reader():
                barrier.wait()
                for _ in range(200):
                    fl = frame.f_locals
                    fl.get("x")
                    list(fl.items())

            threading_helper.run_concurrently([runner, reader, reader])
            g.close()

    def test_gen_f_locals_write_suspended(self):
        # Writes through f_locals must be synchronized with resuming.
        for _ in range(5):
            def gen_fn():
                x = 0
                extra = None
                while True:
                    x += 1
                    yield x

            g = gen_fn()
            next(g)
            frame = g.gi_frame
            barrier = threading.Barrier(3)

            def runner():
                barrier.wait()
                for _ in range(500):
                    next(g)

            def writer():
                barrier.wait()
                for i in range(200):
                    frame.f_locals["extra"] = [i]
                    frame.f_locals["new_var"] = i

            threading_helper.run_concurrently([runner, writer, writer])
            g.close()

    def test_gen_f_locals_inside_running_gen(self):
        # f_locals access from inside a running generator happens on the
        # executing thread itself and must work without synchronization
        # with other threads accessing the same frame.
        for _ in range(5):
            def gen_fn():
                x = 0
                yield
                frame = sys._getframe()
                for i in range(500):
                    x += i
                    assert frame.f_locals["x"] == x
                yield x

            g = gen_fn()
            next(g)
            frame = g.gi_frame
            barrier = threading.Barrier(3)

            def runner():
                barrier.wait()
                next(g)

            def reader():
                barrier.wait()
                for _ in range(100):
                    frame.f_locals.get("x")

            threading_helper.run_concurrently([runner, reader, reader])
            g.close()

    def test_gen_f_locals_dying_generator(self):
        # Access f_locals while the last reference to the generator is
        # dropped and the frame ownership moves to the frame object.
        for _ in range(20):
            def gen_fn():
                x = 42
                yield x

            g = gen_fn()
            next(g)
            frame = g.gi_frame
            barrier = threading.Barrier(3)
            ref = [g]
            del g

            def dropper():
                barrier.wait()
                ref.clear()

            def reader():
                barrier.wait()
                for _ in range(100):
                    frame.f_locals.get("x")
                    list(frame.f_locals.values())

            threading_helper.run_concurrently([dropper, reader, reader])

    def test_setitem_old_value_destructor_reenters_proxy(self):
        # gh-144446: the value displaced by a f_locals store must be
        # released outside the synchronized region: its destructor may
        # access the proxy again (this would deadlock on the frame's
        # critical section, or try to stop the world twice).
        deleted = []
        frame = sys._getframe()

        class Old:
            def __del__(self):
                deleted.append(frame.f_locals.get("marker"))

        marker = 42
        # Not a real local: goes to the frame's extra locals dict.
        frame.f_locals["extra_key"] = Old()
        frame.f_locals["extra_key"] = None    # replace: destructor runs
        self.assertEqual(deleted, [42])
        del frame.f_locals["extra_key"]

    def test_gen_setitem_old_value_destructor_stw(self):
        # Same as above, but on a suspended generator frame, where the
        # store happens under stop-the-world.
        deleted = []

        def gen_fn():
            yield

        g = gen_fn()
        next(g)
        frame = g.gi_frame

        class Old:
            def __del__(self):
                # Accessing the suspended generator frame's proxy stops
                # the world again; it must run after the world restarts.
                deleted.append(len(frame.f_locals))

        frame.f_locals["extra_key"] = Old()
        frame.f_locals["extra_key"] = None
        self.assertEqual(len(deleted), 1)
        del frame.f_locals["extra_key"]
        g.close()

    def test_gen_setitem_cell_old_value_destructor_stw(self):
        # The old value displaced from a cell variable must also be
        # released after the world restarts.
        deleted = []

        def make_gen():
            x = None
            def gen_fn():
                nonlocal x
                yield x
            return gen_fn()

        g = make_gen()
        next(g)
        frame = g.gi_frame

        class Old:
            def __del__(self):
                deleted.append(frame.f_locals.get("x"))

        frame.f_locals["x"] = Old()
        frame.f_locals["x"] = "new"    # replace cell value: destructor runs
        self.assertEqual(deleted, ["new"])
        g.close()

    def test_gen_pop_extra_locals_concurrent(self):
        # pop() must be synchronized with the frame's owner like the
        # other accessors.
        for _ in range(5):
            def gen_fn():
                x = 0
                while True:
                    x += 1
                    yield x

            g = gen_fn()
            next(g)
            frame = g.gi_frame
            barrier = threading.Barrier(3)

            def runner():
                barrier.wait()
                for _ in range(500):
                    next(g)

            def writer():
                barrier.wait()
                for i in range(200):
                    frame.f_locals["extra_key"] = [i]
                    frame.f_locals.pop("extra_key", None)

            threading_helper.run_concurrently([runner, writer, writer])
            g.close()

    def test_gen_getvar_while_running(self):
        # PyFrame_GetVar() reads fast locals and must synchronize with
        # the frame's owner as well.
        _testcapi = import_helper.import_module("_testcapi")
        for _ in range(5):
            def gen_fn():
                obj = None
                yield
                for i in range(2000):
                    obj = [i] * 4
                yield obj

            g = gen_fn()
            next(g)
            frame = g.gi_frame
            barrier = threading.Barrier(3)

            def runner():
                barrier.wait()
                next(g)

            def reader():
                barrier.wait()
                for _ in range(100):
                    try:
                        _testcapi.frame_getvar(frame, "obj")
                    except NameError:
                        pass

            threading_helper.run_concurrently([runner, reader, reader])
            g.close()

    def test_concurrent_frame_clear(self):
        # Test race between frame.clear() and attribute reads.
        def create_frame():
            x = 1
            y = 2
            return sys._getframe()

        frame = create_frame()

        def reader():
            for _ in range(10):
                try:
                    _ = frame.f_locals
                    _ = frame.f_code
                    _ = frame.f_lineno
                except ValueError:
                    # Frame may be cleared
                    pass

        def clearer():
            frame.clear()

        threading_helper.run_concurrently([reader, reader, clearer])


if __name__ == "__main__":
    unittest.main()
