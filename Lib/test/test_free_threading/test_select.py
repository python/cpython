import unittest

from test.support import import_helper, threading_helper
from test.support.threading_helper import run_concurrently

select = import_helper.import_module("select")

NTHREADS = 4


@threading_helper.requires_working_threading()
@unittest.skipUnless(hasattr(select, "kqueue"), "test needs select.kqueue()")
class KqueueTests(unittest.TestCase):
    def test_close_while_reading(self):
        # gh-151364: close() must not race with closed, fileno() and control()
        def reader(kq):
            for _ in range(100):
                kq.closed
                try:
                    kq.fileno()
                    kq.control(None, 0, 0)
                except (ValueError, OSError):
                    pass

        def closer(kq):
            kq.close()

        for _ in range(20):
            kq = select.kqueue()
            run_concurrently(
                worker_func=[closer] + [reader] * NTHREADS,
                args=(kq,),
            )
            self.assertTrue(kq.closed)


if __name__ == "__main__":
    unittest.main()
