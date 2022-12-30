"""Test default behavior of multiprocessing."""

from inspect import currentframe, getframeinfo
import multiprocessing
import sys
from test.support import threading_helper
import unittest
import warnings


def do_nothing():
    pass


# Process has the same API as Thread so this helper works.
join_process = threading_helper.join_thread


class DefaultWarningsTest(unittest.TestCase):

    @unittest.skipIf(sys.platform in ('win32', 'darwin'),
                     'The default is not "fork" on Windows or macOS.')
    def setUp(self):
        self.assertEqual(multiprocessing.get_start_method(), 'fork')
        self.assertIsInstance(multiprocessing.get_context(),
                              multiprocessing.context._DefaultForkContext)

    def test_default_fork_start_method_warning_process(self):
        with warnings.catch_warnings(record=True) as ws:
            warnings.simplefilter('always')
            process = multiprocessing.Process(target=do_nothing)
            process.start()  # warning should point here.
        join_process(process)
        self.assertEqual(len(ws), 1, msg=[str(x) for x in ws])
        self.assertIn(__file__, ws[0].filename)
        self.assertEqual(getframeinfo(currentframe()).lineno-4, ws[0].lineno)
        self.assertIn("'fork'", str(ws[0].message))
        self.assertIn("start_method API", str(ws[0].message))

    def test_default_fork_start_method_warning_pool(self):
        with warnings.catch_warnings(record=True) as ws:
            warnings.simplefilter('always')
            pool = multiprocessing.Pool(1)  # warning should point here.
        pool.terminate()
        pool.join()
        self.assertEqual(len(ws), 1, msg=[str(x) for x in ws])
        self.assertIn(__file__, ws[0].filename)
        self.assertEqual(getframeinfo(currentframe()).lineno-5, ws[0].lineno)
        self.assertIn("'fork'", str(ws[0].message))
        self.assertIn("start_method API", str(ws[0].message))

    def test_no_warning_when_using_explicit_fork_mp_context(self):
        with warnings.catch_warnings(record=True) as ws:
            warnings.simplefilter('always')  # Enable all warnings.
            fork_mp = multiprocessing.get_context('fork')
            pool = fork_mp.Pool(1)
            pool.terminate()
            pool.join()
        self.assertEqual(len(ws), 0, msg=[str(x) for x in ws])


if __name__ == '__main__':
    unittest.main()
