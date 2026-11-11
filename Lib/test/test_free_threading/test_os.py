import errno
import os
import sysconfig
import unittest

from test.support import threading_helper
from test.support.threading_helper import run_concurrently


NTHREADS = 10


@threading_helper.requires_working_threading()
class TestOs(unittest.TestCase):
    def test_listdir_during_gc(self):
        import gc
        from pathlib import Path
        import tempfile

        with tempfile.TemporaryDirectory() as directory:
            expected = {f"entry_{n}" for n in range(32)}
            for name in expected:
                Path(directory, name).touch()

            def worker():
                for _ in range(256):
                    gc.collect()
                    self.assertEqual(set(os.listdir(directory)), expected)

            run_concurrently(worker, nthreads=4)

    @unittest.skipUnless(sysconfig.get_config_var('_Py_HAVE_STRERROR_R'),
                         'need _Py_HAVE_STRERROR_R macro')
    def test_strerror(self):
        # gh-158893: os.strerror() is implemented with strerror_r() which is
        # thread safe. Well, check if it's actually the case.
        last_error = max([getattr(errno, name) for name in dir(errno)
                          if name.startswith('E')])
        test_errors = tuple(range(1, last_error + 1))
        loops = 20

        def worker():
            for _ in range(loops):
                for i in test_errors:
                    os.strerror(i)

        run_concurrently(
            worker_func=worker, nthreads=NTHREADS
        )


if __name__ == "__main__":
    unittest.main()
