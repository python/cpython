#!/usr/local/bin/python -O

""" A Python Benchmark Suite

"""
#
# Note: Please keep this module compatible to Python 1.5.2.
#
# Tests may include features in later Python versions, but these
# should then be embedded in try-except clauses in the configuration
# module Setup.py.
#

# pybench Copyright
__copyright__ = """\
Copyright (c), 1997-2006, Marc-Andre Lemburg (mal@lemburg.com)
Copyright (c), 2000-2006, eGenix.com Software GmbH (info@egenix.com)

                   All Rights Reserved.

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee or royalty is hereby
granted, provided that the above copyright notice appear in all copies
and that both that copyright notice and this permission notice appear
in supporting documentation or portions thereof, including
modifications, that you make.

THE AUTHOR MARC-ANDRE LEMBURG DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE !
"""

import sys, time, operator, string, platform
from CommandLine import *

try:
    import cPickle
    pickle = cPickle
except ImportError:
    import pickle

# Version number; version history: see README file !
__version__ = '2.0'

### Constants

# Second fractions
MILLI_SECONDS = 1e3
MICRO_SECONDS = 1e6

# Percent unit
PERCENT = 100

# Horizontal line length
LINE = 79

# Minimum test run-time
MIN_TEST_RUNTIME = 1e-3

# Number of calibration runs to use for calibrating the tests
CALIBRATION_RUNS = 20

# Number of calibration loops to run for each calibration run
CALIBRATION_LOOPS = 20

# Allow skipping calibration ?
ALLOW_SKIPPING_CALIBRATION = 1

# Timer types
TIMER_TIME_TIME = 'time.time'
TIMER_TIME_CLOCK = 'time.clock'
TIMER_SYSTIMES_PROCESSTIME = 'systimes.processtime'

# Choose platform default timer
if sys.platform[:3] == 'win':
    # On WinXP this has 2.5ms resolution
    TIMER_PLATFORM_DEFAULT = TIMER_TIME_CLOCK
else:
    # On Linux this has 1ms resolution
    TIMER_PLATFORM_DEFAULT = TIMER_TIME_TIME

# Print debug information ?
_debug = 0

### Helpers

def get_timer(timertype):

    if timertype == TIMER_TIME_TIME:
        return time.time
    elif timertype == TIMER_TIME_CLOCK:
        return time.clock
    elif timertype == TIMER_SYSTIMES_PROCESSTIME:
        import systimes
        return systimes.processtime
    else:
        raise TypeError('unknown timer type: %s' % timertype)

def get_machine_details():

    if _debug:
        print 'Getting machine details...'
    buildno, builddate = platform.python_build()
    python = platform.python_version()
    try:
        unichr(100000)
    except ValueError:
        # UCS2 build (standard)
        unicode = 'UCS2'
    except NameError:
        unicode = None
    else:
        # UCS4 build (most recent Linux distros)
        unicode = 'UCS4'
    bits, linkage = platform.architecture()
    return {
        'platform': platform.platform(),
        'processor': platform.processor(),
        'executable': sys.executable,
        'implementation': getattr(platform, 'python_implementation', 'n/a'),
        'python': platform.python_version(),
        'compiler': platform.python_compiler(),
        'buildno': buildno,
        'builddate': builddate,
        'unicode': unicode,
        'bits': bits,
        }

def print_machine_details(d, indent=''):

    l = ['Machine Details:',
         '   Platform ID:    %s' % d.get('platform', 'n/a'),
         '   Processor:      %s' % d.get('processor', 'n/a'),
         '',
         'Python:',
         '   Implementation: %s' % d.get('implementation', 'n/a'),
         '   Executable:     %s' % d.get('executable', 'n/a'),
         '   Version:        %s' % d.get('python', 'n/a'),
         '   Compiler:       %s' % d.get('compiler', 'n/a'),
         '   Bits:           %s' % d.get('bits', 'n/a'),
         '   Build:          %s (#%s)' % (d.get('builddate', 'n/a'),
                                          d.get('buildno', 'n/a')),
         '   Unicode:        %s' % d.get('unicode', 'n/a'),
         ]
    print indent + string.join(l, '\n' + indent) + '\n'

### Test baseclass

class Test:

    """ All test must have this class as baseclass. It provides
        the necessary interface to the benchmark machinery.

        The tests must set .rounds to a value high enough to let the
        test run between 20-50 seconds. This is needed because
        clock()-timing only gives rather inaccurate values (on Linux,
        for example, it is accurate to a few hundreths of a
        second). If you don't want to wait that long, use a warp
        factor larger than 1.

        It is also important to set the .operations variable to a
        value representing the number of "virtual operations" done per
        call of .run().

        If you change a test in some way, don't forget to increase
        its version number.

    """

    ### Instance variables that each test should override

    # Version number of the test as float (x.yy); this is important
    # for comparisons of benchmark runs - tests with unequal version
    # number will not get compared.
    version = 2.0

    # The number of abstract operations done in each round of the
    # test. An operation is the basic unit of what you want to
    # measure. The benchmark will output the amount of run-time per
    # operation. Note that in order to raise the measured timings
    # significantly above noise level, it is often required to repeat
    # sets of operations more than once per test round. The measured
    # overhead per test round should be less than 1 second.
    operations = 1

    # Number of rounds to execute per test run. This should be
    # adjusted to a figure that results in a test run-time of between
    # 1-2 seconds.
    rounds = 100000

    ### Internal variables

    # Mark this class as implementing a test
    is_a_test = 1

    # Last timing: (real, run, overhead)
    last_timing = (0.0, 0.0, 0.0)

    # Warp factor to use for this test
    warp = 1

    # Number of calibration runs to use
    calibration_runs = CALIBRATION_RUNS

    # List of calibration timings
    overhead_times = None

    # List of test run timings
    times = []

    # Timer used for the benchmark
    timer = TIMER_PLATFORM_DEFAULT

    def __init__(self, warp=None, calibration_runs=None, timer=None):

        # Set parameters
        if warp is not None:
            self.rounds = int(self.rounds / warp)
            if self.rounds == 0:
                raise ValueError('warp factor set too high')
            self.warp = warp
        if calibration_runs is not None:
            if (not ALLOW_SKIPPING_CALIBRATION and
                calibration_runs < 1):
                raise ValueError('at least one calibration run is required')
            self.calibration_runs = calibration_runs
        if timer is not None:
            timer = timer

        # Init variables
        self.times = []
        self.overhead_times = []

        # We want these to be in the instance dict, so that pickle
        # saves them
        self.version = self.version
        self.operations = self.operations
        self.rounds = self.rounds

    def get_timer(self):

        """ Return the timer function to use for the test.

        """
        return get_timer(self.timer)

    def compatible(self, other):

        """ Return 1/0 depending on whether the test is compatible
            with the other Test instance or not.

        """
        if self.version != other.version:
            return 0
        if self.rounds != other.rounds:
            return 0
        return 1

    def calibrate_test(self):

        if self.calibration_runs == 0:
            self.overhead_times = [0.0]
            return

        calibrate = self.calibrate
        timer = self.get_timer()
        calibration_loops = range(CALIBRATION_LOOPS)

        # Time the calibration loop overhead
        prep_times = []
        for i in range(self.calibration_runs):
            t = timer()
            for i in calibration_loops:
                pass
            t = timer() - t
            prep_times.append(t)
        min_prep_time = min(prep_times)
        if _debug:
            print
            print 'Calib. prep time     = %.6fms' % (
                min_prep_time * MILLI_SECONDS)

        # Time the calibration runs (doing CALIBRATION_LOOPS loops of
        # .calibrate() method calls each)
        for i in range(self.calibration_runs):
            t = timer()
            for i in calibration_loops:
                calibrate()
            t = timer() - t
            self.overhead_times.append(t / CALIBRATION_LOOPS
                                       - min_prep_time)

        # Check the measured times
        min_overhead = min(self.overhead_times)
        max_overhead = max(self.overhead_times)
        if _debug:
            print 'Calib. overhead time = %.6fms' % (
                min_overhead * MILLI_SECONDS)
        if min_overhead < 0.0:
            raise ValueError('calibration setup did not work')
        if max_overhead - min_overhead > 0.1:
            raise ValueError(
                'overhead calibration timing range too inaccurate: '
                '%r - %r' % (min_overhead, max_overhead))

    def run(self):

        """ Run the test in two phases: first calibrate, then
            do the actual test. Be careful to keep the calibration
            timing low w/r to the test timing.

        """
        test = self.test
        timer = self.get_timer()

        # Get calibration
        min_overhead = min(self.overhead_times)

        # Test run
        t = timer()
        test()
        t = timer() - t
        if t < MIN_TEST_RUNTIME:
            raise ValueError('warp factor too high: '
                             'test times are < 10ms')
        eff_time = t - min_overhead
        if eff_time < 0:
            raise ValueError('wrong calibration')
        self.last_timing = (eff_time, t, min_overhead)
        self.times.append(eff_time)

    def calibrate(self):

        """ Calibrate the test.

            This method should execute everything that is needed to
            setup and run the test - except for the actual operations
            that you intend to measure. pybench uses this method to
            measure the test implementation overhead.

        """
        return

    def test(self):

        """ Run the test.

            The test needs to run self.rounds executing
            self.operations number of operations each.

        """
        return

    def stat(self):

        """ Return test run statistics as tuple:

            (minimum run time,
             average run time,
             total run time,
             average time per operation,
             minimum overhead time)

        """
        runs = len(self.times)
        if runs == 0:
            return 0.0, 0.0, 0.0, 0.0
        min_time = min(self.times)
        total_time = reduce(operator.add, self.times, 0.0)
        avg_time = total_time / float(runs)
        operation_avg = total_time / float(runs
                                           * self.rounds
                                           * self.operations)
        if self.overhead_times:
            min_overhead = min(self.overhead_times)
        else:
            min_overhead = self.last_timing[2]
        return min_time, avg_time, total_time, operation_avg, min_overhead

### Load Setup

# This has to be done after the definition of the Test class, since
# the Setup module will import subclasses using this class.

import Setup

### Benchmark base class

class Benchmark:

    # Name of the benchmark
    name = ''

    # Number of benchmark rounds to run
    rounds = 1

    # Warp factor use to run the tests
    warp = 1                    # Warp factor

    # Average benchmark round time
    roundtime = 0

    # Benchmark version number as float x.yy
    version = 2.0

    # Produce verbose output ?
    verbose = 0

    # Dictionary with the machine details
    machine_details = None

    # Timer used for the benchmark
    timer = TIMER_PLATFORM_DEFAULT

    def __init__(self, name, verbose=None, timer=None, warp=None,
                 calibration_runs=None):

        if name:
            self.name = name
        else:
            self.name = '%04i-%02i-%02i %02i:%02i:%02i' % \
                        (time.localtime(time.time())[:6])
        if verbose is not None:
            self.verbose = verbose
        if timer is not None:
            self.timer = timer
        if warp is not None:
            self.warp = warp
        if calibration_runs is not None:
            self.calibration_runs = calibration_runs

        # Init vars
        self.tests = {}
        if _debug:
            print 'Getting machine details...'
        self.machine_details = get_machine_details()

        # Make .version an instance attribute to have it saved in the
        # Benchmark pickle
        self.version = self.version

    def get_timer(self):

        """ Return the timer function to use for the test.

        """
        return get_timer(self.timer)

    def compatible(self, other):

        """ Return 1/0 depending on whether the benchmark is
            compatible with the other Benchmark instance or not.

        """
        if self.version != other.version:
            return 0
        if (self.machine_details == other.machine_details and
            self.timer != other.timer):
            return 0
        if (self.calibration_runs == 0 and
            other.calibration_runs != 0):
            return 0
        if (self.calibration_runs != 0 and
            other.calibration_runs == 0):
            return 0
        return 1

    def load_tests(self, setupmod, limitnames=None):

        # Add tests
        if self.verbose:
            print 'Searching for tests ...'
            print '--------------------------------------'
        for testclass in setupmod.__dict__.values():
            if not hasattr(testclass, 'is_a_test'):
                continue
            name = testclass.__name__
            if  name == 'Test':
                continue
            if (limitnames is not None and
                limitnames.search(name) is None):
                continue
            self.tests[name] = testclass(
                warp=self.warp,
                calibration_runs=self.calibration_runs,
                timer=self.timer)
        l = self.tests.keys()
        l.sort()
        if self.verbose:
            for name in l:
                print '  %s' % name
            print '--------------------------------------'
            print '  %i tests found' % len(l)
            print

    def calibrate(self):

        print 'Calibrating tests. Please wait...',
        if self.verbose:
            print
            print
            print 'Test                              min      max'
            print '-' * LINE
        tests = self.tests.items()
        tests.sort()
        for i in range(len(tests)):
            name, test = tests[i]
            test.calibrate_test()
            if self.verbose:
                print '%30s:  %6.3fms  %6.3fms' % \
                      (name,
                       min(test.overhead_times) * MILLI_SECONDS,
                       max(test.overhead_times) * MILLI_SECONDS)
        if self.verbose:
            print
            print 'Done with the calibration.'
        else:
            print 'done.'
        print

    def run(self):

        tests = self.tests.items()
        tests.sort()
        timer = self.get_timer()
        print 'Running %i round(s) of the suite at warp factor %i:' % \
              (self.rounds, self.warp)
        print
        self.roundtimes = []
        for i in range(self.rounds):
            if self.verbose:
                print ' Round %-25i  effective   absolute  overhead' % (i+1)
            total_eff_time = 0.0
            for j in range(len(tests)):
                name, test = tests[j]
                if self.verbose:
                    print '%30s:' % name,
                test.run()
                (eff_time, abs_time, min_overhead) = test.last_timing
                total_eff_time = total_eff_time + eff_time
                if self.verbose:
                    print '    %5.0fms    %5.0fms %7.3fms' % \
                          (eff_time * MILLI_SECONDS,
                           abs_time * MILLI_SECONDS,
                           min_overhead * MILLI_SECONDS)
            self.roundtimes.append(total_eff_time)
            if self.verbose:
                print ('                   '
                       '               ------------------------------')
                print ('                   '
                       '     Totals:    %6.0fms' %
                       (total_eff_time * MILLI_SECONDS))
                print
            else:
                print '* Round %i done in %.3f seconds.' % (i+1,
                                                            total_eff_time)
        print

    def stat(self):

        """ Return benchmark run statistics as tuple:

            (minimum round time,
             average round time,
             maximum round time)

            XXX Currently not used, since the benchmark does test
                statistics across all rounds.

        """
        runs = len(self.roundtimes)
        if runs == 0:
            return 0.0, 0.0
        min_time = min(self.roundtimes)
        total_time = reduce(operator.add, self.roundtimes, 0.0)
        avg_time = total_time / float(runs)
        max_time = max(self.roundtimes)
        return (min_time, avg_time, max_time)

    def print_header(self, title='Benchmark'):

        print '-' * LINE
        print '%s: %s' % (title, self.name)
        print '-' * LINE
        print
        print '    Rounds: %s' % self.rounds
        print '    Warp:   %s' % self.warp
        print '    Timer:  %s' % self.timer
        print
        if self.machine_details:
            print_machine_details(self.machine_details, indent='    ')
            print

    def print_benchmark(self, hidenoise=0, limitnames=None):

        print ('Test                          '
               '   minimum  average  operation  overhead')
        print '-' * LINE
        tests = self.tests.items()
        tests.sort()
        total_min_time = 0.0
        total_avg_time = 0.0
        for name, test in tests:
            if (limitnames is not None and
                limitnames.search(name) is None):
                continue
            (min_time,
             avg_time,
             total_time,
             op_avg,
             min_overhead) = test.stat()
            total_min_time = total_min_time + min_time
            total_avg_time = total_avg_time + avg_time
            print '%30s:  %5.0fms  %5.0fms  %6.2fus  %7.3fms' % \
                  (name,
                   min_time * MILLI_SECONDS,
                   avg_time * MILLI_SECONDS,
                   op_avg * MICRO_SECONDS,
                   min_overhead *MILLI_SECONDS)
        print '-' * LINE
        print ('Totals:                        '
               ' %6.0fms %6.0fms' %
               (total_min_time * MILLI_SECONDS,
                total_avg_time * MILLI_SECONDS,
                ))
        print

    def print_comparison(self, compare_to, hidenoise=0, limitnames=None):

        # Check benchmark versions
        if compare_to.version != self.version:
            print ('* Benchmark versions differ: '
                   'cannot compare this benchmark to "%s" !' %
                   compare_to.name)
            print
            self.print_benchmark(hidenoise=hidenoise,
                                 limitnames=limitnames)
            return

        # Print header
        compare_to.print_header('Comparing with')
        print ('Test                          '
               '   minimum run-time        average  run-time')
        print ('                              '
               '   this    other   diff    this    other   diff')
        print '-' * LINE

        # Print test comparisons
        tests = self.tests.items()
        tests.sort()
        total_min_time = other_total_min_time = 0.0
        total_avg_time = other_total_avg_time = 0.0
        benchmarks_compatible = self.compatible(compare_to)
        tests_compatible = 1
        for name, test in tests:
            if (limitnames is not None and
                limitnames.search(name) is None):
                continue
            (min_time,
             avg_time,
             total_time,
             op_avg,
             min_overhead) = test.stat()
            total_min_time = total_min_time + min_time
            total_avg_time = total_avg_time + avg_time
            try:
                other = compare_to.tests[name]
            except KeyError:
                other = None
            if other is None:
                # Other benchmark doesn't include the given test
                min_diff, avg_diff = 'n/a', 'n/a'
                other_min_time = 0.0
                other_avg_time = 0.0
                tests_compatible = 0
            else:
                (other_min_time,
                 other_avg_time,
                 other_total_time,
                 other_op_avg,
                 other_min_overhead) = other.stat()
                other_total_min_time = other_total_min_time + other_min_time
                other_total_avg_time = other_total_avg_time + other_avg_time
                if (benchmarks_compatible and
                    test.compatible(other)):
                    # Both benchmark and tests are comparible
                    min_diff = ((min_time * self.warp) /
                                (other_min_time * other.warp) - 1.0)
                    avg_diff = ((avg_time * self.warp) /
                                (other_avg_time * other.warp) - 1.0)
                    if hidenoise and abs(min_diff) < 10.0:
                        min_diff = ''
                    else:
                        min_diff = '%+5.1f%%' % (min_diff * PERCENT)
                    if hidenoise and abs(avg_diff) < 10.0:
                        avg_diff = ''
                    else:
                        avg_diff = '%+5.1f%%' % (avg_diff * PERCENT)
                else:
                    # Benchmark or tests are not comparible
                    min_diff, avg_diff = 'n/a', 'n/a'
                    tests_compatible = 0
            print '%30s: %5.0fms %5.0fms %7s %5.0fms %5.0fms %7s' % \
                  (name,
                   min_time * MILLI_SECONDS,
                   other_min_time * MILLI_SECONDS * compare_to.warp / self.warp,
                   min_diff,
                   avg_time * MILLI_SECONDS,
                   other_avg_time * MILLI_SECONDS * compare_to.warp / self.warp,
                   avg_diff)
        print '-' * LINE

        # Summarise test results
        if not benchmarks_compatible or not tests_compatible:
            min_diff, avg_diff = 'n/a', 'n/a'
        else:
            if other_total_min_time != 0.0:
                min_diff = '%+5.1f%%' % (
                    ((total_min_time * self.warp) /
                     (other_total_min_time * compare_to.warp) - 1.0) * PERCENT)
            else:
                min_diff = 'n/a'
            if other_total_avg_time != 0.0:
                avg_diff = '%+5.1f%%' % (
                    ((total_avg_time * self.warp) /
                     (other_total_avg_time * compare_to.warp) - 1.0) * PERCENT)
            else:
                avg_diff = 'n/a'
        print ('Totals:                       '
               '  %5.0fms %5.0fms %7s %5.0fms %5.0fms %7s' %
               (total_min_time * MILLI_SECONDS,
                (other_total_min_time * compare_to.warp/self.warp
                 * MILLI_SECONDS),
                min_diff,
                total_avg_time * MILLI_SECONDS,
                (other_total_avg_time * compare_to.warp/self.warp
                 * MILLI_SECONDS),
                avg_diff
               ))
        print
        print '(this=%s, other=%s)' % (self.name,
                                       compare_to.name)
        print

class PyBenchCmdline(Application):

    header = ("PYBENCH - a benchmark test suite for Python "
              "interpreters/compilers.")

    version = __version__

    debug = _debug

    options = [ArgumentOption('-n',
                              'number of rounds',
                              Setup.Number_of_rounds),
               ArgumentOption('-f',
                              'save benchmark to file arg',
                              ''),
               ArgumentOption('-c',
                              'compare benchmark with the one in file arg',
                              ''),
               ArgumentOption('-s',
                              'show benchmark in file arg, then exit',
                              ''),
               ArgumentOption('-w',
                              'set warp factor to arg',
                              Setup.Warp_factor),
               ArgumentOption('-t',
                              'run only tests with names matching arg',
                              ''),
               ArgumentOption('-C',
                              'set the number of calibration runs to arg',
                              CALIBRATION_RUNS),
               SwitchOption('-d',
                            'hide noise in comparisons',
                            0),
               SwitchOption('-v',
                            'verbose output (not recommended)',
                            0),
               SwitchOption('--with-gc',
                            'enable garbage collection',
                            0),
               SwitchOption('--with-syscheck',
                            'use default sys check interval',
                            0),
               ArgumentOption('--timer',
                            'use given timer',
                            TIMER_PLATFORM_DEFAULT),
               ]

    about = """\
The normal operation is to run the suite and display the
results. Use -f to save them for later reuse or comparisons.

Available timers:

   time.time
   time.clock
   systimes.processtime

Examples:

python2.1 pybench.py -f p21.pybench
python2.5 pybench.py -f p25.pybench
python pybench.py -s p25.pybench -c p21.pybench
"""
    copyright = __copyright__

    def main(self):

        rounds = self.values['-n']
        reportfile = self.values['-f']
        show_bench = self.values['-s']
        compare_to = self.values['-c']
        hidenoise = self.values['-d']
        warp = int(self.values['-w'])
        withgc = self.values['--with-gc']
        limitnames = self.values['-t']
        if limitnames:
            if _debug:
                print '* limiting test names to one with substring "%s"' % \
                      limitnames
            limitnames = re.compile(limitnames, re.I)
        else:
            limitnames = None
        verbose = self.verbose
        withsyscheck = self.values['--with-syscheck']
        calibration_runs = self.values['-C']
        timer = self.values['--timer']

        print '-' * LINE
        print 'PYBENCH %s' % __version__
        print '-' * LINE
        print '* using %s %s' % (
            getattr(platform, 'python_implementation', 'Python'),
            string.join(string.split(sys.version), ' '))

        # Switch off garbage collection
        if not withgc:
            try:
                import gc
            except ImportError:
                print '* Python version doesn\'t support garbage collection'
            else:
                try:
                    gc.disable()
                except NotImplementedError:
                    print '* Python version doesn\'t support gc.disable'
                else:
                    print '* disabled garbage collection'

        # "Disable" sys check interval
        if not withsyscheck:
            # Too bad the check interval uses an int instead of a long...
            value = 2147483647
            try:
                sys.setcheckinterval(value)
            except (AttributeError, NotImplementedError):
                print '* Python version doesn\'t support sys.setcheckinterval'
            else:
                print '* system check interval set to maximum: %s' % value

        if timer == TIMER_SYSTIMES_PROCESSTIME:
            import systimes
            print '* using timer: systimes.processtime (%s)' % \
                  systimes.SYSTIMES_IMPLEMENTATION
        else:
            print '* using timer: %s' % timer

        print

        if compare_to:
            try:
                f = open(compare_to,'rb')
                bench = pickle.load(f)
                bench.name = compare_to
                f.close()
                compare_to = bench
            except IOError, reason:
                print '* Error opening/reading file %s: %s' % (
                    repr(compare_to),
                    reason)
                compare_to = None

        if show_bench:
            try:
                f = open(show_bench,'rb')
                bench = pickle.load(f)
                bench.name = show_bench
                f.close()
                bench.print_header()
                if compare_to:
                    bench.print_comparison(compare_to,
                                           hidenoise=hidenoise,
                                           limitnames=limitnames)
                else:
                    bench.print_benchmark(hidenoise=hidenoise,
                                          limitnames=limitnames)
            except IOError, reason:
                print '* Error opening/reading file %s: %s' % (
                    repr(show_bench),
                    reason)
                print
            return

        if reportfile:
            print 'Creating benchmark: %s (rounds=%i, warp=%i)' % \
                  (reportfile, rounds, warp)
            print

        # Create benchmark object
        bench = Benchmark(reportfile,
                          verbose=verbose,
                          timer=timer,
                          warp=warp,
                          calibration_runs=calibration_runs)
        bench.rounds = rounds
        bench.load_tests(Setup, limitnames=limitnames)
        try:
            bench.calibrate()
            bench.run()
        except KeyboardInterrupt:
            print
            print '*** KeyboardInterrupt -- Aborting'
            print
            return
        bench.print_header()
        if compare_to:
            bench.print_comparison(compare_to,
                                   hidenoise=hidenoise,
                                   limitnames=limitnames)
        else:
            bench.print_benchmark(hidenoise=hidenoise,
                                  limitnames=limitnames)

        # Ring bell
        sys.stderr.write('\007')

        if reportfile:
            try:
                f = open(reportfile,'wb')
                bench.name = reportfile
                pickle.dump(bench,f)
                f.close()
            except IOError, reason:
                print '* Error opening/writing reportfile'
            except IOError, reason:
                print '* Error opening/writing reportfile %s: %s' % (
                    reportfile,
                    reason)
                print

if __name__ == '__main__':
    PyBenchCmdline()
