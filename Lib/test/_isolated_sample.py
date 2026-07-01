"""Sample tests driven by test.test_support.TestIsolated.

This module is imported, never run as a test file, so that
:func:`test.support.isolation.runInSubprocess` has a real, importable target to run in
a subprocess.  Several of these tests fail, error or are skipped on purpose.
"""

import time
import unittest
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
class FixtureBindingSample(unittest.TestCase):
    # setUpClass must run bound to the runtime class, so a subclass sees its own
    # name here rather than the base class's.

    @classmethod
    def setUpClass(cls):
        cls.setup_class_name = cls.__name__

    def test_runtime_class(self):
        self.assertEqual(self.setup_class_name, type(self).__name__)


class FixtureBindingSubclassSample(FixtureBindingSample):
    pass
