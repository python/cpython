import timeit
import unittest
import sys
import io
import time
from textwrap import dedent

from test.support import captured_stdout
from test.support import captured_stderr

# timeit's default number of iterations.
DEFAULT_NUMBER = 1000000

# timeit's default number of repetitions.
DEFAULT_REPEAT = 3

# XXX: some tests are commented out that would improve the coverage but take a
# long time to run because they test the default number of loops, which is
# large.  The tests could be enabled if there was a way to override the default
# number of loops during testing, but this would require changing the signature
# of some functions that use the default as a default argument.

class FakeTimer:
    BASE_TIME = 42.0
    def __init__(self, seconds_per_increment=1.0):
        self.count = 0
        self.setup_calls = 0
        self.seconds_per_increment=seconds_per_increment
        timeit._fake_timer = self

    def __call__(self):
        return self.BASE_TIME + self.count * self.seconds_per_increment

    def inc(self):
        self.count += 1

    def setup(self):
        self.setup_calls += 1

    def wrap_timer(self, timer):
        """Records 'timer' and returns self as callable timer."""
        self.saved_timer = timer
        return self

class TestTimeit(unittest.TestCase):

    def tearDown(self):
        try:
            del timeit._fake_timer
        except AttributeError:
            pass

    def test_reindent_empty(self):
        self.assertEqual(timeit.reindent("", 0), "")
        self.assertEqual(timeit.reindent("", 4), "")

    def test_reindent_single(self):
        self.assertEqual(timeit.reindent("pass", 0), "pass")
        self.assertEqual(timeit.reindent("pass", 4), "pass")

    def test_reindent_multi_empty(self):
        self.assertEqual(timeit.reindent("\n\n", 0), "\n\n")
        self.assertEqual(timeit.reindent("\n\n", 4), "\n    \n    ")

    def test_reindent_multi(self):
        self.assertEqual(timeit.reindent(
            "print()\npass\nbreak", 0),
            "print()\npass\nbreak")
        self.assertEqual(timeit.reindent(
            "print()\npass\nbreak", 4),
            "print()\n    pass\n    break")

    def test_timer_invalid_stmt(self):
        self.assertRaises(ValueError, timeit.Timer, stmt=None)
        self.assertRaises(SyntaxError, timeit.Timer, stmt='return')
        self.assertRaises(SyntaxError, timeit.Timer, stmt='yield')
        self.assertRaises(SyntaxError, timeit.Timer, stmt='yield from ()')
        self.assertRaises(SyntaxError, timeit.Timer, stmt='break')
        self.assertRaises(SyntaxError, timeit.Timer, stmt='continue')
        self.assertRaises(SyntaxError, timeit.Timer, stmt='from timeit import *')

    def test_timer_invalid_setup(self):
        self.assertRaises(ValueError, timeit.Timer, setup=None)
        self.assertRaises(SyntaxError, timeit.Timer, setup='return')
        self.assertRaises(SyntaxError, timeit.Timer, setup='yield')
        self.assertRaises(SyntaxError, timeit.Timer, setup='yield from ()')
        self.assertRaises(SyntaxError, timeit.Timer, setup='break')
        self.assertRaises(SyntaxError, timeit.Timer, setup='continue')
        self.assertRaises(SyntaxError, timeit.Timer, setup='from timeit import *')

    fake_setup = "import timeit\ntimeit._fake_timer.setup()"
    fake_stmt = "import timeit\ntimeit._fake_timer.inc()"

    def fake_callable_setup(self):
        self.fake_timer.setup()

    def fake_callable_stmt(self):
        self.fake_timer.inc()

    def timeit(self, stmt, setup, number=None, globals=None):
        self.fake_timer = FakeTimer()
        t = timeit.Timer(stmt=stmt, setup=setup, timer=self.fake_timer,
                globals=globals)
        kwargs = {}
        if number is None:
            number = DEFAULT_NUMBER
        else:
            kwargs['number'] = number
        delta_time = t.timeit(**kwargs)
        self.assertEqual(self.fake_timer.setup_calls, 1)
        self.assertEqual(self.fake_timer.count, number)
        self.assertEqual(delta_time, number)

    # Takes too long to run in debug build.
    #def test_timeit_default_iters(self):
    #    self.timeit(self.fake_stmt, self.fake_setup)

    def test_timeit_zero_iters(self):
        self.timeit(self.fake_stmt, self.fake_setup, number=0)

    def test_timeit_few_iters(self):
        self.timeit(self.fake_stmt, self.fake_setup, number=3)

    def test_timeit_callable_stmt(self):
        self.timeit(self.fake_callable_stmt, self.fake_setup, number=3)

    def test_timeit_callable_setup(self):
        self.timeit(self.fake_stmt, self.fake_callable_setup, number=3)

    def test_timeit_callable_stmt_and_setup(self):
        self.timeit(self.fake_callable_stmt,
                self.fake_callable_setup, number=3)

    # Takes too long to run in debug build.
    #def test_timeit_function(self):
    #    delta_time = timeit.timeit(self.fake_stmt, self.fake_setup,
    #            timer=FakeTimer())
    #    self.assertEqual(delta_time, DEFAULT_NUMBER)

    def test_timeit_function_zero_iters(self):
        delta_time = timeit.timeit(self.fake_stmt, self.fake_setup, number=0,
                timer=FakeTimer())
        self.assertEqual(delta_time, 0)

    def test_timeit_globals_args(self):
        global _global_timer
        _global_timer = FakeTimer()
        t = timeit.Timer(stmt='_global_timer.inc()', timer=_global_timer)
        self.assertRaises(NameError, t.timeit, number=3)
        timeit.timeit(stmt='_global_timer.inc()', timer=_global_timer,
                      globals=globals(), number=3)
        local_timer = FakeTimer()
        timeit.timeit(stmt='local_timer.inc()', timer=local_timer,
                      globals=locals(), number=3)

    def repeat(self, stmt, setup, repeat=None, number=None):
        self.fake_timer = FakeTimer()
        t = timeit.Timer(stmt=stmt, setup=setup, timer=self.fake_timer)
        kwargs = {}
        if repeat is None:
            repeat = DEFAULT_REPEAT
        else:
            kwargs['repeat'] = repeat
        if number is None:
            number = DEFAULT_NUMBER
        else:
            kwargs['number'] = number
        delta_times = t.repeat(**kwargs)
        self.assertEqual(self.fake_timer.setup_calls, repeat)
        self.assertEqual(self.fake_timer.count, repeat * number)
        self.assertEqual(delta_times, repeat * [float(number)])

    # Takes too long to run in debug build.
    #def test_repeat_default(self):
    #    self.repeat(self.fake_stmt, self.fake_setup)

    def test_repeat_zero_reps(self):
        self.repeat(self.fake_stmt, self.fake_setup, repeat=0)

    def test_repeat_zero_iters(self):
        self.repeat(self.fake_stmt, self.fake_setup, number=0)

    def test_repeat_few_reps_and_iters(self):
        self.repeat(self.fake_stmt, self.fake_setup, repeat=3, number=5)

    def test_repeat_callable_stmt(self):
        self.repeat(self.fake_callable_stmt, self.fake_setup,
                repeat=3, number=5)

    def test_repeat_callable_setup(self):
        self.repeat(self.fake_stmt, self.fake_callable_setup,
                repeat=3, number=5)

    def test_repeat_callable_stmt_and_setup(self):
        self.repeat(self.fake_callable_stmt, self.fake_callable_setup,
                repeat=3, number=5)

    # Takes too long to run in debug build.
    #def test_repeat_function(self):
    #    delta_times = timeit.repeat(self.fake_stmt, self.fake_setup,
    #            timer=FakeTimer())
    #    self.assertEqual(delta_times, DEFAULT_REPEAT * [float(DEFAULT_NUMBER)])

    def test_repeat_function_zero_reps(self):
        delta_times = timeit.repeat(self.fake_stmt, self.fake_setup, repeat=0,
                timer=FakeTimer())
        self.assertEqual(delta_times, [])

    def test_repeat_function_zero_iters(self):
        delta_times = timeit.repeat(self.fake_stmt, self.fake_setup, number=0,
                timer=FakeTimer())
        self.assertEqual(delta_times, DEFAULT_REPEAT * [0.0])

    def assert_exc_string(self, exc_string, expected_exc_name):
        exc_lines = exc_string.splitlines()
        self.assertGreater(len(exc_lines), 2)
        self.assertTrue(exc_lines[0].startswith('Traceback'))
        self.assertTrue(exc_lines[-1].startswith(expected_exc_name))

    def test_print_exc(self):
        s = io.StringIO()
        t = timeit.Timer("1/0")
        try:
            t.timeit()
        except:
            t.print_exc(s)
        self.assert_exc_string(s.getvalue(), 'ZeroDivisionError')

    MAIN_DEFAULT_OUTPUT = "10 loops, best of 3: 1 sec per loop\n"

    def run_main(self, seconds_per_increment=1.0, switches=None, timer=None):
        if timer is None:
            timer = FakeTimer(seconds_per_increment=seconds_per_increment)
        if switches is None:
            args = []
        else:
            args = switches[:]
        args.append(self.fake_stmt)
        # timeit.main() modifies sys.path, so save and restore it.
        orig_sys_path = sys.path[:]
        with captured_stdout() as s:
            timeit.main(args=args, _wrap_timer=timer.wrap_timer)
        sys.path[:] = orig_sys_path[:]
        return s.getvalue()

    def test_main_bad_switch(self):
        s = self.run_main(switches=['--bad-switch'])
        self.assertEqual(s, dedent("""\
            option --bad-switch not recognized
            use -h/--help for command line help
            """))

    def test_main_seconds(self):
        s = self.run_main(seconds_per_increment=5.5)
        self.assertEqual(s, "10 loops, best of 3: 5.5 sec per loop\n")

    def test_main_milliseconds(self):
        s = self.run_main(seconds_per_increment=0.0055)
        self.assertEqual(s, "100 loops, best of 3: 5.5 msec per loop\n")

    def test_main_microseconds(self):
        s = self.run_main(seconds_per_increment=0.0000025, switches=['-n100'])
        self.assertEqual(s, "100 loops, best of 3: 2.5 usec per loop\n")

    def test_main_fixed_iters(self):
        s = self.run_main(seconds_per_increment=2.0, switches=['-n35'])
        self.assertEqual(s, "35 loops, best of 3: 2 sec per loop\n")

    def test_main_setup(self):
        s = self.run_main(seconds_per_increment=2.0,
                switches=['-n35', '-s', 'print("CustomSetup")'])
        self.assertEqual(s, "CustomSetup\n" * 3 +
                "35 loops, best of 3: 2 sec per loop\n")

    def test_main_multiple_setups(self):
        s = self.run_main(seconds_per_increment=2.0,
                switches=['-n35', '-s', 'a = "CustomSetup"', '-s', 'print(a)'])
        self.assertEqual(s, "CustomSetup\n" * 3 +
                "35 loops, best of 3: 2 sec per loop\n")

    def test_main_fixed_reps(self):
        s = self.run_main(seconds_per_increment=60.0, switches=['-r9'])
        self.assertEqual(s, "10 loops, best of 9: 60 sec per loop\n")

    def test_main_negative_reps(self):
        s = self.run_main(seconds_per_increment=60.0, switches=['-r-5'])
        self.assertEqual(s, "10 loops, best of 1: 60 sec per loop\n")

    @unittest.skipIf(sys.flags.optimize >= 2, "need __doc__")
    def test_main_help(self):
        s = self.run_main(switches=['-h'])
        # Note: It's not clear that the trailing space was intended as part of
        # the help text, but since it's there, check for it.
        self.assertEqual(s, timeit.__doc__ + ' ')

    def test_main_using_time(self):
        fake_timer = FakeTimer()
        s = self.run_main(switches=['-t'], timer=fake_timer)
        self.assertEqual(s, self.MAIN_DEFAULT_OUTPUT)
        self.assertIs(fake_timer.saved_timer, time.time)

    def test_main_using_clock(self):
        fake_timer = FakeTimer()
        s = self.run_main(switches=['-c'], timer=fake_timer)
        self.assertEqual(s, self.MAIN_DEFAULT_OUTPUT)
        self.assertIs(fake_timer.saved_timer, time.clock)

    def test_main_verbose(self):
        s = self.run_main(switches=['-v'])
        self.assertEqual(s, dedent("""\
                10 loops -> 10 secs
                raw times: 10 10 10
                10 loops, best of 3: 1 sec per loop
            """))

    def test_main_very_verbose(self):
        s = self.run_main(seconds_per_increment=0.000050, switches=['-vv'])
        self.assertEqual(s, dedent("""\
                10 loops -> 0.0005 secs
                100 loops -> 0.005 secs
                1000 loops -> 0.05 secs
                10000 loops -> 0.5 secs
                raw times: 0.5 0.5 0.5
                10000 loops, best of 3: 50 usec per loop
            """))

    def test_main_with_time_unit(self):
        unit_sec = self.run_main(seconds_per_increment=0.002,
                switches=['-u', 'sec'])
        self.assertEqual(unit_sec,
                "1000 loops, best of 3: 0.002 sec per loop\n")
        unit_msec = self.run_main(seconds_per_increment=0.002,
                switches=['-u', 'msec'])
        self.assertEqual(unit_msec,
                "1000 loops, best of 3: 2 msec per loop\n")
        unit_usec = self.run_main(seconds_per_increment=0.002,
                switches=['-u', 'usec'])
        self.assertEqual(unit_usec,
                "1000 loops, best of 3: 2e+03 usec per loop\n")
        # Test invalid unit input
        with captured_stderr() as error_stringio:
            invalid = self.run_main(seconds_per_increment=0.002,
                    switches=['-u', 'parsec'])
        self.assertEqual(error_stringio.getvalue(),
                    "Unrecognized unit. Please select usec, msec, or sec.\n")

    def test_main_exception(self):
        with captured_stderr() as error_stringio:
            s = self.run_main(switches=['1/0'])
        self.assert_exc_string(error_stringio.getvalue(), 'ZeroDivisionError')

    def test_main_exception_fixed_reps(self):
        with captured_stderr() as error_stringio:
            s = self.run_main(switches=['-n1', '1/0'])
        self.assert_exc_string(error_stringio.getvalue(), 'ZeroDivisionError')

    def autorange(self, callback=None):
        timer = FakeTimer(seconds_per_increment=0.001)
        t = timeit.Timer(stmt=self.fake_stmt, setup=self.fake_setup, timer=timer)
        return t.autorange(callback)

    def test_autorange(self):
        num_loops, time_taken = self.autorange()
        self.assertEqual(num_loops, 1000)
        self.assertEqual(time_taken, 1.0)

    def test_autorange_with_callback(self):
        def callback(a, b):
            print("{} {:.3f}".format(a, b))
        with captured_stdout() as s:
            num_loops, time_taken = self.autorange(callback)
        self.assertEqual(num_loops, 1000)
        self.assertEqual(time_taken, 1.0)
        expected = ('10 0.010\n'
                    '100 0.100\n'
                    '1000 1.000\n')
        self.assertEqual(s.getvalue(), expected)


if __name__ == '__main__':
    unittest.main()
