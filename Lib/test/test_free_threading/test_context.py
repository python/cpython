import contextvars
import unittest
from threading import Event, Thread

from test.support import threading_helper


@threading_helper.requires_working_threading()
class TestContext(unittest.TestCase):
    def test_racing_read_write(self):
        # gh-154535: reading a Context object from one thread while another
        # thread sets variables in it used to crash.  The readers looked at
        # Context.ctx_vars without owning a reference to it, so the writer
        # could deallocate the mapping while a reader was walking it.
        ctx = contextvars.Context()
        cvars = [contextvars.ContextVar(f"cvar{i}") for i in range(64)]
        done = Event()
        errors = []

        def writer():
            def body():
                i = 0
                while not done.is_set():
                    cvars[i % len(cvars)].set(i)
                    i += 1
            try:
                ctx.run(body)
            except BaseException as e:
                errors.append(e)

        def reader():
            try:
                for _ in range(200):
                    ctx.copy()
                    len(ctx)
                    list(ctx)
                    list(ctx.items())
                    list(ctx.keys())
                    list(ctx.values())
                    cvars[0] in ctx
                    ctx.get(cvars[0])
                    ctx == ctx
            except BaseException as e:
                errors.append(e)
            finally:
                done.set()

        threads = [Thread(target=writer)]
        threads += [Thread(target=reader) for _ in range(4)]
        with threading_helper.start_threads(threads, done.set):
            pass

        self.assertEqual(errors, [], msg=f"unexpected errors: {errors}")


if __name__ == "__main__":
    unittest.main()
