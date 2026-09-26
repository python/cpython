import sys
import textwrap
import unittest
from test import support
from test.support import script_helper

# This function is available for the --enable-pystats config.
HAVE_PYSTATS = hasattr(sys, '_stats_on')

TEST_TEMPLATE = """
    import sys
    import threading
    import time

    THREADS = 2

    class A:
        pass

    class B:
        pass

    def modify_class():
        # This is used as a rare event we can assume doesn't happen unless we do it.
        # It increments the "Rare event (set_class)" count.
        a = A()
        a.__class__ = B

    TURNED_ON = False
    def stats_on():
        global TURNED_ON
        sys._stats_on()
        TURNED_ON = True

    TURNED_OFF = False
    def stats_off():
        global TURNED_OFF
        sys._stats_off()
        TURNED_OFF = True

    CLEARED = False
    def stats_clear():
        global CLEARED
        sys._stats_clear()
        CLEARED = True

    def func_start():
        pass

    def func_end():
        pass

    def func_test(thread_id):
        pass

    _TEST_CODE_

    func_start()
    threads = []
    for i in range(THREADS):
        t = threading.Thread(target=func_test, args=(i,))
        threads.append(t)
        t.start()
    for t in threads:
        t.join()
    func_end()
    """


def run_test_code(
    test_code,
    args=[],
    env_vars=None,
    stat_names=None,
):
    """Run test code and return stats counters printed when it exits.

    Returns the value of the "set_class" counter, or, when `stat_names` is
    given, a dict of those counters collected from the same run.
    """
    code = textwrap.dedent(TEST_TEMPLATE)
    code = code.replace('_TEST_CODE_', textwrap.dedent(test_code))
    script_args = args + ['-c', code]
    env_vars = env_vars or {}
    res, _ = script_helper.run_python_until_end(*script_args, **env_vars)
    stderr = res.err.decode("ascii", "backslashreplace")

    if stat_names is None:
        stat_names = ('Rare event (set_class)',)
        wanted_one = True
    else:
        wanted_one = False

    # A run may print more than one block of stats, e.g. when it calls
    # sys._stats_dump() itself and then prints again on the way out. Keep the
    # first value seen for each counter.
    found = {}
    for line in stderr.split('\n'):
        label, sep, value = line.partition(':')
        label = label.strip()
        if sep and label in stat_names and label not in found:
            found[label] = value.strip()

    if wanted_one:
        return found.get('Rare event (set_class)', '')
    return found


@unittest.skipUnless(HAVE_PYSTATS, "requires pystats build option")
class TestPyStats(unittest.TestCase):
    """Tests for pystats functionality (requires --enable-pystats build
    option).
    """

    def test_stats_toggle_on(self):
        """Check the toggle on functionality.
        """
        code = """
        def func_start():
            modify_class()
        """

        # If turned on with command line flag, should get one count.
        stat_count = run_test_code(code, args=['-X', 'pystats'])
        self.assertEqual(stat_count, '1')

        # If turned on with env var, should get one count.
        stat_count = run_test_code(code, env_vars={'PYTHONSTATS': '1'})
        self.assertEqual(stat_count, '1')

        # If not turned on, should be no counts.
        stat_count = run_test_code(code)
        self.assertEqual(stat_count, '')

        code = """
        def func_start():
            modify_class()
            sys._stats_on()
            modify_class()
        """
        # Not initially turned on but enabled by sys._stats_on(), should get
        # one count.
        stat_count = run_test_code(code)
        self.assertEqual(stat_count, '1')

    def test_stats_toggle_on_thread(self):
        """Check the toggle on functionality when threads are used.
        """
        code = """
        def func_test(thread_id):
            if thread_id == 0:
                modify_class()
                stats_on()
                modify_class()
            else:
                while not TURNED_ON:
                    pass
                modify_class()
        """
        # Turning on in one thread will count in other thread.
        stat_count = run_test_code(code)
        self.assertEqual(stat_count, '2')

        code = """
        def func_test(thread_id):
            if thread_id == 0:
                modify_class()
                stats_off()
                modify_class()
            else:
                while not TURNED_OFF:
                    pass
                modify_class()
        """
        # Turning off in one thread will not count in other threads.
        stat_count = run_test_code(code, args=['-X', 'pystats'])
        self.assertEqual(stat_count, '1')

    def test_thread_exit_merge(self):
        """Check that per-thread stats (when free-threading enabled) are merged.
        """
        code = """
        def func_test(thread_id):
            modify_class()
            if thread_id == 0:
                raise SystemExit
        """
        # Stats from a thread exiting early should still be counted.
        stat_count = run_test_code(code, args=['-X', 'pystats'])
        self.assertEqual(stat_count, '2')

    def test_stats_dump(self):
        """Check that sys._stats_dump() works.
        """
        code = """
        def func_test(thread_id):
            if thread_id == 0:
                stats_on()
            else:
                while not TURNED_ON:
                    pass
                modify_class()
                sys._stats_dump()
                stats_off()
        """
        # Stats from a thread exiting early should still be counted.
        stat_count = run_test_code(code)
        self.assertEqual(stat_count, '1')

    def test_stats_clear(self):
        """Check that sys._stats_clear() works.
        """
        code = """
        ready = False
        def func_test(thread_id):
            global ready
            if thread_id == 0:
                stats_on()
                modify_class()
                while not ready:
                    pass  # wait until other thread has called modify_class()
                stats_clear()  # clears stats for all threads
            else:
                while not TURNED_ON:
                    pass
                modify_class()
                ready = True
        """
        # Clearing stats will clear for all threads
        stat_count = run_test_code(code)
        self.assertEqual(stat_count, '0')


@unittest.skipUnless(HAVE_PYSTATS, "requires pystats build option")
@unittest.skipUnless(support.Py_GIL_DISABLED, "requires free-threaded build")
class TestFreeThreadingStats(unittest.TestCase):
    """Tests for the counters that only the free-threaded build keeps.
    """

    # How many times each worker thread stops the world.
    COLLECTS = 10
    WORKERS = 4

    WORLD_STOPS = 'World stops (world_stops)'
    TOTAL_NS = 'World stop total ns (world_stop_total_ns)'
    MAX_NS = 'World stop max ns (world_stop_max_ns)'

    def collect_in_threads(self):
        """Stop the world from several threads, then report the counters.
        """
        code = f"""
        import gc

        THREADS = {self.WORKERS}

        def func_start():
            # Discard whatever start-up accumulated, so the counts below come
            # from the collections the workers do and nothing else.
            sys._stats_clear()

        def func_test(thread_id):
            for _ in range({self.COLLECTS}):
                gc.collect()
        """
        return run_test_code(
            code,
            args=['-X', 'pystats'],
            stat_names=(self.WORLD_STOPS, self.TOTAL_NS, self.MAX_NS),
        )

    def test_counters_are_summed_over_threads(self):
        """Each thread's counts must be added, not overwrite the previous one.
        """
        stats = self.collect_in_threads()
        stops = int(stats[self.WORLD_STOPS])
        # Every worker stops the world COLLECTS times. Merging a thread's stats
        # into the interpreter's by assignment would leave just one worker's
        # share, so anything close to a single worker's count means the counts
        # of the others were thrown away.
        self.assertGreater(stops, 2 * self.COLLECTS)

    def test_world_stop_durations_are_recorded(self):
        stats = self.collect_in_threads()
        stops = int(stats[self.WORLD_STOPS])
        total = int(stats[self.TOTAL_NS])
        longest = int(stats[self.MAX_NS])

        self.assertGreater(stops, 0)
        # Pauses that were counted must also have been timed.
        self.assertGreater(total, 0)
        self.assertGreater(longest, 0)
        # A single pause cannot last longer than all of them put together.
        self.assertLessEqual(longest, total)


if __name__ == "__main__":
    unittest.main()
