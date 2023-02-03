"""Test default behavior of multiprocessing."""

from inspect import currentframe, getframeinfo
import multiprocessing
from multiprocessing.context import DefaultForkDeprecationWarning
import sys
from test.support import import_helper, threading_helper
import unittest
import warnings

# Skip tests if _multiprocessing wasn't built.
import_helper.import_module('_multiprocessing')


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
            warnings.simplefilter('ignore')
            warnings.filterwarnings('always', category=DefaultForkDeprecationWarning)
            process = multiprocessing.Process(target=do_nothing)
            process.start()  # warning should point here.
        join_process(process)
        self.assertIsInstance(ws[0].message, DefaultForkDeprecationWarning)
        self.assertIn(__file__, ws[0].filename)
        self.assertEqual(getframeinfo(currentframe()).lineno-4, ws[0].lineno)
        self.assertIn("'fork'", str(ws[0].message))
        self.assertIn("get_context", str(ws[0].message))
        self.assertEqual(len(ws), 1, msg=[str(x) for x in ws])

    def test_default_fork_start_method_warning_pool(self):
        with warnings.catch_warnings(record=True) as ws:
            warnings.simplefilter('ignore')
            warnings.filterwarnings('always', category=DefaultForkDeprecationWarning)
            pool = multiprocessing.Pool(1)  # warning should point here.
        pool.terminate()
        pool.join()
        self.assertIsInstance(ws[0].message, DefaultForkDeprecationWarning)
        self.assertIn(__file__, ws[0].filename)
        self.assertEqual(getframeinfo(currentframe()).lineno-5, ws[0].lineno)
        self.assertIn("'fork'", str(ws[0].message))
        self.assertIn("get_context", str(ws[0].message))
        self.assertEqual(len(ws), 1, msg=[str(x) for x in ws])

    def test_default_fork_start_method_warning_manager(self):
        with warnings.catch_warnings(record=True) as ws:
            warnings.simplefilter('ignore')
            warnings.filterwarnings('always', category=DefaultForkDeprecationWarning)
            manager = multiprocessing.Manager()  # warning should point here.
        manager.shutdown()
        self.assertIsInstance(ws[0].message, DefaultForkDeprecationWarning)
        self.assertIn(__file__, ws[0].filename)
        self.assertEqual(getframeinfo(currentframe()).lineno-4, ws[0].lineno)
        self.assertIn("'fork'", str(ws[0].message))
        self.assertIn("get_context", str(ws[0].message))
        self.assertEqual(len(ws), 1, msg=[str(x) for x in ws])

    def test_no_mp_warning_when_using_explicit_fork_context(self):
        with warnings.catch_warnings(record=True) as ws:
            warnings.simplefilter('ignore')
            warnings.filterwarnings('always', category=DefaultForkDeprecationWarning)
            fork_mp = multiprocessing.get_context('fork')
            pool = fork_mp.Pool(1)
            pool.terminate()
            pool.join()
        self.assertEqual(len(ws), 0, msg=[str(x) for x in ws])


if __name__ == '__main__':
    unittest.main()
