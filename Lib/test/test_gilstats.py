import os
import sys
import threading
import time
import unittest
import warnings

from test import support
from test.support import import_helper, script_helper, threading_helper

_gilstats = import_helper.import_module('_gilstats')

if not getattr(sys, '_is_gil_enabled', lambda: True)():
    raise unittest.SkipTest('requires the GIL')

N_THREADS = 4
FAST = dict(drain_interval_min=0.005, drain_interval_max=0.02)


def wait_for(predicate, timeout=support.SHORT_TIMEOUT):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        value = predicate()
        if value:
            return value
        time.sleep(0.01)
    raise AssertionError('timed out')


class Contention:
    """N CPU-bound threads fighting for the GIL."""

    def __init__(self, n=N_THREADS):
        self.n = n

    def __enter__(self):
        self.old_interval = sys.getswitchinterval()
        sys.setswitchinterval(1e-4)
        self.stop = False

        def spin():
            while not self.stop:
                pass

        self.threads = [threading.Thread(target=spin) for _ in range(self.n)]
        for t in self.threads:
            t.start()
        return self

    def __exit__(self, *exc):
        self.stop = True
        for t in self.threads:
            t.join()
        sys.setswitchinterval(self.old_interval)


class SummarizeTests(unittest.TestCase):
    """Histogram, quantile and top-K math, on synthetic input."""

    def test_empty(self):
        s = _gilstats._summarize([])
        self.assertEqual(s['count'], 0)
        self.assertEqual(s['buckets'], [])
        self.assertEqual(s['top_waits'], [])
        for key in ('p50_us', 'p90_us', 'p99_us', 'p999_us'):
            self.assertIsNone(s[key])

    def test_log2_buckets(self):
        # Bucket with upper bound U holds [U/2, U); the first holds 0.
        cases = {0: 1, 1: 2, 2: 4, 3: 4, 4: 8, 1023: 1024, 1024: 2048,
                 2**31: 2**32, 2**32 - 1: 2**32}
        for us, upper in cases.items():
            with self.subTest(us=us):
                s = _gilstats._summarize([us])
                self.assertEqual(s['buckets'], [(upper, 1)])
                self.assertEqual(s['max_wait_us'], us)

    def test_clamped_to_uint32(self):
        s = _gilstats._summarize([2**40])
        self.assertEqual(s['max_wait_us'], 2**32 - 1)

    def test_quantiles(self):
        # 900 waits of ~10us, 90 of ~1000us, 10 of ~100000us.
        waits = [10] * 900 + [1000] * 90 + [100_000] * 10
        s = _gilstats._summarize(waits)
        self.assertEqual(s['count'], 1000)
        self.assertEqual(s['sum_us'], sum(waits))
        self.assertEqual(s['p50_us'], 15)         # [8, 16)
        self.assertEqual(s['p90_us'], 15)         # rank 900 is still 10us
        self.assertEqual(s['p99_us'], 1023)       # [512, 1024)
        # Capped by the exact max rather than the bucket bound (131071).
        self.assertEqual(s['p999_us'], 100_000)

    def test_quantile_within_2x(self):
        waits = list(range(1, 10_001))
        s = _gilstats._summarize(waits)
        for key, q in (('p50_us', 0.5), ('p90_us', 0.9), ('p99_us', 0.99)):
            exact = waits[int(q * len(waits)) - 1]
            with self.subTest(key=key):
                self.assertGreaterEqual(s[key], exact)
                self.assertLess(s[key], 2 * exact)

    def test_below_threshold_is_bottom_of_distribution(self):
        s = _gilstats._summarize([5000] * 10, below_threshold=90,
                                 threshold_us=100)
        self.assertEqual(s['count'], 10)
        self.assertEqual(s['below_threshold'], 90)
        self.assertEqual(s['p50_us'], 100)        # reported as threshold
        self.assertEqual(s['p90_us'], 100)
        self.assertEqual(s['p99_us'], 5000)

    def test_top_waits(self):
        waits = [(us, us % 7) for us in range(1, 101)]
        s = _gilstats._summarize(reversed(waits))
        self.assertEqual(s['top_waits'], waits[::-1][:16])
        self.assertEqual(s['max_waiters'], 6)

    def test_top_waits_fewer_than_k(self):
        s = _gilstats._summarize([(3, 1), (1, 1), (2, 1)])
        self.assertEqual(s['top_waits'], [(3, 1), (2, 1), (1, 1)])

    def test_bad_input(self):
        with self.assertRaises(TypeError):
            _gilstats._summarize(['x'])
        with self.assertRaises(OverflowError):
            _gilstats._summarize([-1])


@threading_helper.requires_working_threading()
class CollectionTests(unittest.TestCase):

    def setUp(self):
        self.addCleanup(_gilstats.set_callback, None)
        self.addCleanup(_gilstats.disable)

    def enable(self, **kwargs):
        _gilstats.enable(**{**FAST, **kwargs})

    def collect(self, duration=0.5, n=N_THREADS, **window):
        """Run contention for `duration` and return one published window."""
        got = []
        _gilstats.set_callback(got.append, **{
            'period': 60.0, 'min_events': 0, 'max_period': 60.0, **window})
        with Contention(n):
            time.sleep(duration)
        # Publish immediately: shrink the period now that load is over.
        _gilstats.set_callback(got.append, period=0.01, min_events=0,
                               max_period=0.01)
        wait_for(lambda: got)
        return got[0]

    def test_enable_disable(self):
        self.assertFalse(_gilstats.is_enabled())
        self.enable()
        self.assertTrue(_gilstats.is_enabled())
        self.enable()   # restart with new settings
        self.assertTrue(_gilstats.is_enabled())
        _gilstats.disable()
        self.assertFalse(_gilstats.is_enabled())
        _gilstats.disable()   # no-op

    def test_aggregator_thread_is_stopped(self):
        before = threading.active_count()
        self.enable()
        _gilstats.disable()
        self.assertEqual(threading.active_count(), before)

    def test_config_validation(self):
        for kwargs in ({'ring_size': 0}, {'ring_size': 3},
                       {'drain_interval_min': 0},
                       {'drain_interval_min': 1, 'drain_interval_max': 0.5}):
            with self.subTest(**kwargs), self.assertRaises(ValueError):
                _gilstats.enable(**kwargs)
        self.assertFalse(_gilstats.is_enabled())
        with self.assertRaises(TypeError):
            _gilstats.set_callback(42)
        with self.assertRaises(ValueError):
            _gilstats.set_callback(None, period=0)
        with self.assertRaises(ValueError):
            _gilstats.set_callback(None, period=10, max_period=5)

    def test_contention(self):
        self.enable(record_threshold_us=0)
        s = self.collect()
        self.assertGreater(s['count'], 0)
        self.assertGreaterEqual(s['max_waiters'], N_THREADS - 1)
        self.assertEqual(s['dropped'], 0)
        self.assertGreater(s['max_wait_us'], 0)
        self.assertEqual(s['max_wait_us'], s['top_waits'][0][0])
        self.assertEqual(sum(c for _, c in s['buckets']), s['count'])
        self.assertLessEqual(s['p50_us'], s['p90_us'])
        self.assertLessEqual(s['p90_us'], s['p99_us'])
        self.assertLessEqual(s['p99_us'], s['p999_us'])
        self.assertLessEqual(s['p999_us'], s['max_wait_us'])
        self.assertLess(s['window_start_ns'], s['window_end_ns'])

    def test_max_waiters_counts_filtered_waits(self):
        # Every wait is below the threshold, so nothing reaches the ring,
        # but the waiters high-water mark must still see all of them.
        self.enable(record_threshold_us=2**32 - 1)
        s = self.collect()
        self.assertEqual(s['count'], 0)
        self.assertGreater(s['below_threshold'], 0)
        self.assertGreaterEqual(s['max_waiters'], N_THREADS - 1)
        self.assertEqual(s['p50_us'], 2**32 - 1)

    def test_uncontended(self):
        self.enable()
        _gilstats.set_callback(None, period=0.05, min_events=0,
                               max_period=0.05)
        for _ in range(50):
            time.sleep(0.001)   # releases and re-takes the GIL
        s = wait_for(lambda: (snap := _gilstats.snapshot()) and
                     snap['uncontended'] > 0 and snap)
        self.assertEqual(s['count'], 0)
        self.assertEqual(s['max_waiters'], 0)

    def test_snapshot_none_before_first_window(self):
        self.enable()
        self.assertIsNone(_gilstats.snapshot())

    def test_dropped_when_ring_is_full(self):
        # Tiny ring, slow drain.
        self.enable(ring_size=2, record_threshold_us=0,
                    drain_interval_min=1.0, drain_interval_max=1.0)
        s = self.collect(duration=0.3)
        self.assertGreater(s['dropped'], 0)

    def test_adaptive_drain_interval(self):
        # Under load a small ring fills up: the interval drops to the min.
        # Look at windows published during the load: once it stops, the
        # interval starts growing again.
        self.enable(ring_size=16, record_threshold_us=0,
                    drain_interval_min=0.001, drain_interval_max=0.1)
        during = []
        _gilstats.set_callback(during.append, period=0.05, min_events=0,
                               max_period=0.05)
        with Contention():
            wait_for(lambda: len(during) >= 5)
        self.assertEqual(min(s['drain_interval'] for s in during), 0.001)

        # Idle: the interval grows to the max.
        self.enable(drain_interval_min=0.001, drain_interval_max=0.016)
        got = []
        _gilstats.set_callback(got.append, period=0.3, min_events=0,
                               max_period=0.3)
        wait_for(lambda: got)
        self.assertEqual(got[0]['drain_interval'], 0.016)

    def test_min_events_delays_publication(self):
        self.enable()
        got = []
        _gilstats.set_callback(got.append, period=0.01,
                               min_events=10**9, max_period=0.5)
        start = time.monotonic()
        wait_for(lambda: got)
        self.assertGreaterEqual(time.monotonic() - start, 0.4)
        s = got[0]
        self.assertGreaterEqual(s['window_end_ns'] - s['window_start_ns'],
                                0.5e9)

    def test_callback_runs_on_other_thread(self):
        self.enable()
        idents = []
        _gilstats.set_callback(
            lambda s: idents.append(threading.get_ident()),
            period=0.01, min_events=0, max_period=0.01)
        wait_for(lambda: len(idents) >= 3)
        self.assertNotIn(threading.get_ident(), idents)
        self.assertEqual(len(set(idents)), 1)

    def test_callback_exception(self):
        self.enable()

        def callback(snapshot):
            raise ZeroDivisionError('boom')

        with support.catch_unraisable_exception() as cm:
            _gilstats.set_callback(callback, period=0.01, min_events=0,
                                   max_period=0.01)
            wait_for(lambda: cm.unraisable is not None)
            _gilstats.set_callback(None)
            self.assertIsInstance(cm.unraisable.exc_value, ZeroDivisionError)
            self.assertIn('GIL statistics callback', cm.unraisable.err_msg)

    def test_disable_from_callback(self):
        self.enable()
        errors = []

        def callback(snapshot):
            try:
                _gilstats.disable()
            except RuntimeError as e:
                errors.append(e)

        _gilstats.set_callback(callback, period=0.01, min_events=0,
                               max_period=0.01)
        wait_for(lambda: errors)
        self.assertTrue(_gilstats.is_enabled())

    def test_aggregator_excluded_from_stats(self):
        # The main thread spins while the aggregator repeatedly takes the
        # GIL to run the callback.  The aggregator has to wait a full switch
        # interval (100 ms) each time; if it were counted, those waits would
        # show up.  The main thread only waits while the (trivial) callback
        # runs.
        old_interval = sys.getswitchinterval()
        sys.setswitchinterval(0.1)
        self.addCleanup(sys.setswitchinterval, old_interval)
        self.enable(record_threshold_us=0)
        got = []
        _gilstats.set_callback(got.append, period=0.005, min_events=0,
                               max_period=0.005)
        deadline = time.monotonic() + 1.0
        while time.monotonic() < deadline:
            pass
        _gilstats.set_callback(None)
        self.assertGreaterEqual(len(got), 3)
        self.assertLess(max(s['max_wait_us'] for s in got), 50_000)
        self.assertLessEqual(max(s['max_waiters'] for s in got), 1)

    @support.requires_fork()
    def test_fork(self):
        self.enable(record_threshold_us=0)
        pid = os.fork()
        if pid == 0:
            code = 1
            try:
                if _gilstats.is_enabled():
                    s = self.collect(duration=0.3)
                    if s['count'] > 0 and s['max_waiters'] >= N_THREADS - 1:
                        code = 0
            finally:
                os._exit(code)
        support.wait_process(pid, exitcode=0)
        self.assertTrue(_gilstats.is_enabled())

    @support.requires_fork()
    def test_fork_does_not_warn_about_aggregator(self):
        # The aggregator is fork-safe and must not trigger the
        # "process is multi-threaded" DeprecationWarning on its own.
        self.enable()
        with warnings.catch_warnings():
            warnings.simplefilter('error', DeprecationWarning)
            pid = os.fork()
            if pid == 0:
                os._exit(0)
        support.wait_process(pid, exitcode=0)


class StartupTests(unittest.TestCase):

    def test_env_var(self):
        code = 'import _gilstats; print(_gilstats.is_enabled())'
        for value, expected in (('1', b'True'), ('0', b'False'),
                                ('', b'False')):
            with self.subTest(value=value):
                res = script_helper.assert_python_ok(
                    '-c', code, PYTHONGILSTATS=value)
                self.assertEqual(res.out.strip(), expected)
        res = script_helper.assert_python_ok('-E', '-c', code,
                                             PYTHONGILSTATS='1')
        self.assertEqual(res.out.strip(), b'False')

    @threading_helper.requires_working_threading()
    def test_exit_while_enabled(self):
        # Finalization must stop the aggregator even with a callback set
        # and non-daemon threads around.
        code = '''if 1:
            import _gilstats, threading, sys
            sys.setswitchinterval(1e-4)
            _gilstats.enable(record_threshold_us=0, drain_interval_min=0.001)
            _gilstats.set_callback(lambda s: None, period=0.001,
                                   min_events=0, max_period=0.001)
            def spin():
                for _ in range(200_000):
                    pass
            for _ in range(4):
                threading.Thread(target=spin).start()
        '''
        script_helper.assert_python_ok('-c', code)


if __name__ == '__main__':
    unittest.main()
