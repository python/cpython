"""Sample tests driven by test.test_support.TestIsolated.

This module is imported, never run as a test file, so that
:func:`test.support.isolation.isolated` has a real, importable target to run in
a subprocess.  Several of these tests fail, error or are skipped on purpose.
"""

import time
import unittest
from test.support import isolation

# A test in DurationSample sleeps this long in the subprocess; the parent
# replays it instantly, so a parent-reported duration close to this proves the
# subprocess timing was forwarded rather than the replay time measured.
DURATION_SLEEP = 0.2


class MethodSample(unittest.TestCase):

    @isolation.isolated()
    def test_pass(self):
        self.assertTrue(isolation.running_isolated)

    @isolation.isolated()
    def test_fail(self):
        self.assertEqual(1, 2)

    @isolation.isolated()
    def test_error(self):
        raise RuntimeError('boom')

    @isolation.isolated()
    def test_skip(self):
        self.skipTest('nope')

    @isolation.isolated()
    @unittest.expectedFailure
    def test_expected_failure(self):
        self.assertEqual(1, 2)

    @isolation.isolated()
    @unittest.expectedFailure
    def test_unexpected_success(self):
        pass


@isolation.isolated()
class ClassSample(unittest.TestCase):

    def test_pass(self):
        self.assertTrue(isolation.running_isolated)

    def test_fail(self):
        self.assertEqual(1, 2)

    @unittest.expectedFailure
    def test_expected_failure(self):
        self.assertEqual(1, 2)


class SubtestSample(unittest.TestCase):

    @isolation.isolated()
    def test_subtests(self):
        for i in range(3):
            with self.subTest(i=i):
                self.assertNotEqual(i, 1)


@isolation.isolated()
class DurationSample(unittest.TestCase):

    def test_slow(self):
        time.sleep(DURATION_SLEEP)
