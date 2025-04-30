"""Run a test case multiple times in parallel threads."""

import copy
import threading
import unittest

from unittest import TestCase


class ParallelTestCase(TestCase):
    def __init__(self, test_case: TestCase, num_threads: int):
        self.test_case = test_case
        self.num_threads = num_threads
        self._testMethodName = test_case._testMethodName
        self._testMethodDoc = test_case._testMethodDoc

    def __str__(self):
        return f"{str(self.test_case)} [threads={self.num_threads}]"

    def run_worker(self, test_case: TestCase, result: unittest.TestResult,
                   barrier: threading.Barrier):
        barrier.wait()
        test_case.run(result)

    def run(self, result=None):
        if result is None:
            result = test_case.defaultTestResult()
            startTestRun = getattr(result, 'startTestRun', None)
            stopTestRun = getattr(result, 'stopTestRun', None)
            if startTestRun is not None:
                startTestRun()
        else:
            stopTestRun = None

        # Called at the beginning of each test. See TestCase.run.
        result.startTest(self)

        cases = [copy.copy(self.test_case) for _ in range(self.num_threads)]
        results = [unittest.TestResult() for _ in range(self.num_threads)]

        barrier = threading.Barrier(self.num_threads)
        threads = []
        for i, (case, r) in enumerate(zip(cases, results)):
            thread = threading.Thread(target=self.run_worker,
                                      args=(case, r, barrier),
                                      name=f"{str(self.test_case)}-{i}",
                                      daemon=True)
            threads.append(thread)

        for thread in threads:
            thread.start()

        for threads in threads:
            threads.join()

        # Aggregate test results
        if all(r.wasSuccessful() for r in results):
            result.addSuccess(self)

        # Note: We can't call result.addError, result.addFailure, etc. because
        # we no longer have the original exception, just the string format.
        for r in results:
            if len(r.errors) > 0 or len(r.failures) > 0:
                result._mirrorOutput = True
            result.errors.extend(r.errors)
            result.failures.extend(r.failures)
            result.skipped.extend(r.skipped)
            result.expectedFailures.extend(r.expectedFailures)
            result.unexpectedSuccesses.extend(r.unexpectedSuccesses)
            result.collectedDurations.extend(r.collectedDurations)

        if any(r.shouldStop for r in results):
            result.stop()

        # Test has finished running
        result.stopTest(self)
        if stopTestRun is not None:
            stopTestRun()
