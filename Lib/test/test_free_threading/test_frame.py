import functools
import sys
import threading
import unittest

from test.support import threading_helper

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
