"""Sample tests driven by test.test_support.TestIsolated.

This module is imported, never run as a test file, so that
:func:`test.support.isolation.runInSubprocess` has a real, importable target to run in
a subprocess.  Several of these tests fail, error or are skipped on purpose.
"""

import atexit
import os
import sys
import time
import unittest
from test import support
from test.support import isolation

# DurationSample sleeps this long in the subprocess; a parent-reported duration
# close to it proves the subprocess timing was forwarded, not the replay time.
DURATION_SLEEP = 0.2


class MethodSample(unittest.TestCase):

    @isolation.runInSubprocess()
    def test_pass(self):
        self.assertTrue(isolation.runningInSubprocess)

    @isolation.runInSubprocess()
    def test_fail(self):
        self.assertEqual(1, 2)

    @isolation.runInSubprocess()
    def test_error(self):
        raise RuntimeError('boom')

    @isolation.runInSubprocess()
    def test_skip(self):
        self.skipTest('nope')

    @isolation.runInSubprocess()
    @unittest.expectedFailure
    def test_expected_failure(self):
        self.assertEqual(1, 2)

    @isolation.runInSubprocess()
    @unittest.expectedFailure
    def test_unexpected_success(self):
        pass


@isolation.runInSubprocess()
class ClassSample(unittest.TestCase):

    def test_pass(self):
        self.assertTrue(isolation.runningInSubprocess)

    def test_fail(self):
        self.assertEqual(1, 2)

    @unittest.expectedFailure
    def test_expected_failure(self):
        self.assertEqual(1, 2)


class SubtestSample(unittest.TestCase):

    @isolation.runInSubprocess()
    def test_subtests(self):
        for i in range(3):
            with self.subTest(i=i):
                self.assertNotEqual(i, 1)


@isolation.runInSubprocess()
class DurationSample(unittest.TestCase):

    def test_slow(self):
        time.sleep(DURATION_SLEEP)


@isolation.runInSubprocess()
class SubclassingSample(unittest.TestCase):
    # setUpClass must run bound to the runtime class, so a subclass sees its own
    # name here rather than the base class's.

    @classmethod
    def setUpClass(cls):
        cls.setup_class_name = cls.__name__

    def setUp(self):
        self.set_up = True

    def test_runtime_class(self):
        self.assertEqual(self.setup_class_name, type(self).__name__)


class SubclassSample(SubclassingSample):
    # What a subclass adds or overrides must run in the subprocess too.

    def setUp(self):
        super().setUp()
        self.set_up_in_subclass = True

    def test_added_in_subclass(self):
        self.assertTrue(isolation.runningInSubprocess)
        self.assertTrue(self.set_up)
        self.assertTrue(self.set_up_in_subclass)


class BrokenSubclassSample(SubclassingSample):
    # An overriding setUpClass() that does not call super() bypasses the
    # subprocess entirely.

    @classmethod
    def setUpClass(cls):
        pass


# The exit code the samples below die with, after their tests have run.
EXIT_CODE = 3


def _die_at_exit():
    atexit.register(os._exit, EXIT_CODE)


class MethodExitSample(unittest.TestCase):

    @isolation.runInSubprocess()
    def test_passes_then_dies(self):
        _die_at_exit()

    @isolation.runInSubprocess()
    def test_fails_and_dies(self):
        _die_at_exit()
        self.fail('the test itself failed')


@isolation.runInSubprocess()
class ClassExitSample(unittest.TestCase):

    def test_pass(self):
        pass

    def test_dies(self):
        _die_at_exit()


@isolation.runInSubprocess(options=['-X', 'dev', '-W', 'error::BytesWarning'])
class OptionsSample(unittest.TestCase):

    def test_options_applied(self):
        self.assertTrue(sys.flags.dev_mode)
        self.assertIn('error::BytesWarning', sys.warnoptions)


class EnvSample(unittest.TestCase):

    @isolation.runInSubprocess(env={'_PYTHON_ISOLATION_PROBE': 'set-by-test'})
    def test_env_set(self):
        self.assertEqual(os.environ.get('_PYTHON_ISOLATION_PROBE'), 'set-by-test')

    @isolation.runInSubprocess(env={'_PYTHON_ISOLATION_PROBE': None})
    def test_env_unset(self):
        self.assertNotIn('_PYTHON_ISOLATION_PROBE', os.environ)

    @isolation.runInSubprocess()
    def test_env_inherited(self):
        # Without env= the subprocess inherits the parent environment as it is.
        self.assertEqual(os.environ.get('_PYTHON_ISOLATION_PROBE'), 'set-by-parent')


# TimeoutSample hangs this long, so that the timeout always fires first.
TIMEOUT_HANG = 60.0
TIMEOUT = 0.5


class TimeoutSample(unittest.TestCase):

    @isolation.runInSubprocess(timeout=TIMEOUT)
    def test_hang(self):
        time.sleep(TIMEOUT_HANG)


class BigmemSample(unittest.TestCase):

    @support.bigmemtest(size=1024, memuse=1)
    def test_where_it_runs(self, size):
        # A real run is isolated by bigmemtest() itself, a dummy run is not.
        self.assertEqual(isolation.runningInSubprocess,
                         bool(support.real_max_memuse))
