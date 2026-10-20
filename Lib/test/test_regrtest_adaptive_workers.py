"""Deterministic tests for percentage-job admission and load averages."""

import contextlib
import io
import queue
import threading
import unittest
from unittest.mock import Mock, patch

from test.libregrtest.adaptive_workers import AdaptiveWorkerGate
from test.libregrtest.logger import Logger
from test.libregrtest.results import TestResults
from test.libregrtest.run_workers import RunWorkers


class AdaptiveWorkerGateTests(unittest.TestCase):
    def make_gate(self):
        logger = Mock()
        logger.get_load_avg.return_value = 0
        with patch('os.process_cpu_count', return_value=4):
            gate = AdaptiveWorkerGate(0.25, 3, logger)
        logger.get_load_avg.reset_mock()
        return gate, logger.get_load_avg

    def update(self, gate, get_load_avg, value):
        get_load_avg.return_value = value * 4
        gate.update()

    def test_adjustment(self):
        gate, get_load_avg = self.make_gate()
        for expected in (2, 3, 3):
            self.update(gate, get_load_avg, 0.2)
            self.assertEqual(gate.limit, expected)
        for value in (0.24, 0.26):
            self.update(gate, get_load_avg, value)
            self.assertEqual(gate.limit, 3)
        for expected in (2, 1, 1):
            self.update(gate, get_load_avg, 1.0)
            self.assertEqual(gate.limit, expected)

    def test_each_update_samples_load(self):
        gate, get_load_avg = self.make_gate()
        gate.update()
        self.assertEqual(gate.limit, 2)
        gate.update()
        self.assertEqual(gate.limit, 3)
        self.assertEqual(get_load_avg.call_count, 2)

    def test_custom_target_tolerance(self):
        gate, get_load_avg = self.make_gate()
        gate.target_tolerance = 0.125
        gate.limit = 2
        for value in (gate.target - gate.target_tolerance,
                      gate.target, gate.target + gate.target_tolerance):
            self.update(gate, get_load_avg, value)
            self.assertEqual(gate.limit, 2)
        self.update(gate, get_load_avg, 0.0625)
        self.assertEqual(gate.limit, 3)
        self.update(gate, get_load_avg, 0.4375)
        self.assertEqual(gate.limit, 2)

    def test_active_workers(self):
        gate, _ = self.make_gate()
        self.assertEqual(gate.get_active_workers(), 0)
        self.assertTrue(gate.acquire())
        self.assertEqual(gate.get_active_workers(), 1)
        gate.release()
        self.assertEqual(gate.get_active_workers(), 0)

    def test_waiter_shutdown(self):
        gate, _ = self.make_gate()
        self.assertTrue(gate.acquire())
        acquired = []
        thread = threading.Thread(target=lambda: acquired.append(gate.acquire()))
        thread.start()
        try:
            gate.stop()
            thread.join(5)
            self.assertFalse(thread.is_alive())
            self.assertEqual(acquired, [False])
        finally:
            gate.stop()
            thread.join(5)
            gate.release()

    def test_release_wakes_waiter(self):
        gate, _ = self.make_gate()
        self.assertTrue(gate.acquire())
        acquired = threading.Event()

        def worker():
            if gate.acquire():
                acquired.set()
                gate.release()

        thread = threading.Thread(target=worker)
        thread.start()
        try:
            self.assertFalse(acquired.is_set())
            gate.release()
            self.assertTrue(acquired.wait(5))
        finally:
            gate.stop()
            thread.join(5)
        self.assertEqual(gate.get_active_workers(), 0)

    def test_lower_limit_does_not_interrupt_active_workers(self):
        gate, get_load_avg = self.make_gate()
        gate.limit = 2
        self.assertTrue(gate.acquire())
        self.assertTrue(gate.acquire())
        self.update(gate, get_load_avg, 1.0)
        self.assertEqual(gate.limit, 1)
        self.assertEqual(gate.get_active_workers(), 2)
        gate.release()
        gate.release()

    def test_increase_wakes_waiter(self):
        gate, get_load_avg = self.make_gate()
        self.assertTrue(gate.acquire())
        acquired = threading.Event()

        def worker():
            if gate.acquire():
                acquired.set()
                gate.release()

        thread = threading.Thread(target=worker)
        thread.start()
        try:
            self.update(gate, get_load_avg, 0.2)
            self.assertTrue(acquired.wait(5))
        finally:
            gate.stop()
            thread.join(5)
            gate.release()



class WorkerUpdateIntervalTests(unittest.TestCase):
    def make_runner(self):
        gate = Mock(update_interval=5.0)
        runtests = Mock(timeout=None, pgo=True)
        runtests.get_jobs.return_value = 2
        runtests.iter_case_groups.return_value = iter(())
        with (patch('test.libregrtest.adaptive_workers.AdaptiveWorkerGate', return_value=gate),
              patch('test.libregrtest.run_workers.time.monotonic', return_value=0)):
            runner = RunWorkers(2, runtests, Mock(), Mock(), Mock(), adaptive_target=0.8)
        runner.live_worker_count = 1
        runner.output = Mock()
        return runner, gate

    def test_frequent_results_preserve_deadline(self):
        runner, gate = self.make_runner()
        result = (False, Mock())
        runner.output.get.return_value = result
        for now, expected_updates in ((0, 0), (1, 0), (4.9, 0),
                                      (5, 1), (5.1, 1), (9.9, 1), (10, 2)):
            with self.subTest(now=now):
                with patch('test.libregrtest.run_workers.time.monotonic', return_value=now):
                    self.assertIs(runner._get_result(), result)
                self.assertEqual(gate.update.call_count, expected_updates)

    def test_idle_wait_ends_at_update_deadline(self):
        runner, gate = self.make_runner()
        now = 4.0
        result = (False, Mock())

        def get_result(*, timeout):
            nonlocal now
            if now == 4.0:
                self.assertEqual(timeout, 1.0)
                now += timeout
                raise queue.Empty
            self.assertEqual(timeout, 5.0)
            return result

        runner.output.get.side_effect = get_result
        with patch('test.libregrtest.run_workers.time.monotonic', side_effect=lambda: now):
            self.assertIs(runner._get_result(), result)
        gate.update.assert_called_once_with()

    def test_late_update_does_not_catch_up_in_a_burst(self):
        runner, gate = self.make_runner()
        runner.output.get.return_value = (False, Mock())
        with patch('test.libregrtest.run_workers.time.monotonic', return_value=30):
            runner._get_result()
            runner._get_result()
        gate.update.assert_called_once_with()
        self.assertEqual(runner.next_worker_update, 35)


class AdaptiveLoggerTests(unittest.TestCase):
    def test_active_workers_output(self):
        logger = Logger(TestResults(), quiet=False, pgo=False)
        with patch.object(logger, 'get_load_avg', return_value=None):
            for active in (0, 2):
                with self.subTest(active=active):
                    logger.get_active_workers = lambda: active
                    with contextlib.redirect_stdout(io.StringIO()) as output:
                        logger.log('test_example passed')
                    self.assertIn(f'running: {active} test_example passed',
                                  output.getvalue())
            logger.get_active_workers = None
            with contextlib.redirect_stdout(io.StringIO()) as output:
                logger.log('test_example passed')
            self.assertNotIn('running:', output.getvalue())

    def test_memory_output(self):
        logger = Logger(TestResults(), quiet=False, pgo=False)
        with patch.object(logger, 'get_load_avg', return_value=None):
            for usage, expected in ((16 * 1024**2, 'mem:   16.0 MiB'),
                                    (2 * 1024**3, 'mem:    2.0 GiB')):
                with self.subTest(usage=usage):
                    logger.get_mem_usage = lambda: usage
                    with contextlib.redirect_stdout(io.StringIO()) as output:
                        logger.log('test_example passed')
                    self.assertIn(expected, output.getvalue())


class LoadAverageTests(unittest.TestCase):
    def logger(self, values):
        logger = Mock()
        logger.get_load_avg.side_effect = values
        return logger

    def test_process_cpu_count_sets_target(self):
        with (patch('os.cpu_count', return_value=16),
              patch('os.process_cpu_count', return_value=4)):
            gate = AdaptiveWorkerGate(0.8, 36, self.logger([0]))
        self.assertEqual(gate.load_target, 3.2)
        self.assertEqual(gate.limit, 4)

    def test_initial_concurrency(self):
        for cpus, target, maximum, expected in (
                (16, 0.8, 36, 13), (4, 0.5, 12, 2),
                (1, 0.01, 6, 1), (16, 1.0, 3, 3), (None, 0.8, 6, 1)):
            with self.subTest(cpus=cpus, target=target, maximum=maximum):
                with patch('os.process_cpu_count', return_value=cpus):
                    gate = AdaptiveWorkerGate(target, maximum, self.logger([0]))
                self.assertEqual(gate.limit, expected)

    def test_tracking_failure_reverts_to_initial_count(self):
        for failure in (None, -1, float('nan'), float('inf'),
                        OSError('no load'), ValueError('bad load')):
            for initial_load in (True, False):
                with self.subTest(failure=failure, initial_load=initial_load):
                    values = [failure, 0] if initial_load else [0, failure, 0]
                    logger = self.logger(values)
                    with patch('os.process_cpu_count', return_value=4):
                        gate = AdaptiveWorkerGate(0.5, 8, logger)
                    if not initial_load:
                        gate.update()
                        self.assertEqual(gate.limit, 3)
                    gate.update()
                    self.assertEqual(gate.limit, 2)
                    calls = logger.get_load_avg.call_count
                    gate.update()
                    self.assertEqual(logger.get_load_avg.call_count, calls + 1)
                    self.assertEqual(gate.limit, 3)

    def test_unknown_cpu_count(self):
        with patch('os.process_cpu_count', return_value=None):
            gate = AdaptiveWorkerGate(0.8, 8, self.logger([0]))
        self.assertEqual(gate.load_target, 0.8)
        self.assertEqual(gate.limit, 1)



if __name__ == '__main__':
    unittest.main()
