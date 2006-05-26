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

# Version number
__version__ = '1.3'

#
# NOTE: Use xrange for all test loops unless you want to face
#       a 20MB process !
#
# All tests should have rounds set to values so that a run()
# takes between 20-50 seconds. This is to get fairly good
# clock() values. You can use option -w to speedup the tests
# by a fixed integer factor (the "warp factor").
#

import sys,time,operator
from CommandLine import *

try:
    import cPickle
    pickle = cPickle
except ImportError:
    import pickle

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
        it's version number.

    """

    ### Instance variables that each test should override

    # Version number of the test as float (x.yy); this is important
    # for comparisons of benchmark runs - tests with unequal version
    # number will not get compared.
    version = 1.0

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
    # 20-50 seconds.
    rounds = 10000

    ### Internal variables

    # Mark this class as implementing a test
    is_a_test = 1

    # Misc. internal variables
    last_timing = (0,0,0) # last timing (real,run,calibration)
    warp = 1            # warp factor this test uses
    cruns = 20          # number of calibration runs
    overhead = None     # list of calibration timings

    def __init__(self,warp=1):

        if warp > 1:
            self.rounds = self.rounds / warp
            if self.rounds == 0:
                self.rounds = 1
            self.warp = warp
        self.times = []
        self.overhead = []
        # We want these to be in the instance dict, so that pickle
        # saves them
        self.version = self.version
        self.operations = self.operations
        self.rounds = self.rounds

    def run(self, cruns):

        """ Run the test in two phases: first calibrate, then
            do the actual test. Be careful to keep the calibration
            timing low w/r to the test timing.

        """
        test = self.test
        calibrate = self.calibrate
        clock = time.clock
        # first calibrate
        t = clock()
        calibrate()
        offset = clock() - t
        if cruns:
            for i in range(cruns-1):
                t = clock()
                calibrate()
                t = clock() - t
                if t < offset:
                    offset = t
        # now the real thing
        t = clock()
        test()
        t = clock() - t
        if t < 0.01:
            sys.exit("Lower warp required: test times < 10 ms are unreliable")
        self.last_timing = (t-offset,t,offset)
        self.times.append(t-offset)

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
        # do some tests
        return

    def stat(self):

        """ Returns four values:
        minimum round time
        average time per round
        average time per operation
        average overhead time

        XXX Should this take warp factors into account?
        """
        runs = len(self.times)
        if runs == 0:
            return 0,0
        mintime = min(self.times)
        totaltime = reduce(operator.add,self.times,0.0)
        avg = totaltime / float(runs)
        op_avg = totaltime / float(runs * self.rounds * self.operations)
        if self.overhead:
            totaloverhead = reduce(operator.add,self.overhead,0.0)
            ov_avg = totaloverhead / float(runs)
        else:
            # use self.last_timing - not too accurate
            ov_avg = self.last_timing[2]
        return mintime, avg, op_avg, ov_avg

### Load Setup

# This has to be done after the definition of the Test class, since
# the Setup module will import subclasses using this class.

import Setup

### Benchmark base class

class Benchmark:

    name = '?'                  # Name of the benchmark
    rounds = 1                  # Number of rounds to run
    warp = 1                    # Warp factor
    roundtime = 0               # Average round time
    version = None              # Benchmark version number (see __init__)
                                # as float x.yy

    def __init__(self):

        self.tests = {}
        self.version = 0.31

    def load_tests(self, setupmod, warp=1, limitnames="", verbose=0):

        self.warp = warp
        if limitnames:
            limitnames = re.compile(limitnames, re.I)
        else:
            limitnames = None
        tests = self.tests
        if verbose:
            print 'Searching for tests ...',
        setupmod.__dict__.values()
        for c in setupmod.__dict__.values():
            if not hasattr(c,'is_a_test'):
                continue
            name = c.__name__
            if  name == 'Test':
                continue
            if limitnames is not None and limitnames.search(name) is None:
                continue
            tests[name] = c(warp)
        l = tests.keys()
        l.sort()
        if verbose:
            print
            for t in l:
                print '  ', t
            print len(l), "tests found"
        print

    def run(self, verbose, cruns):

        tests = self.tests.items()
        tests.sort()
        clock = time.clock
        print 'Running %i round(s) of the suite at warp factor %i:' % (self.rounds, self.warp)
        print
        roundtime = clock()
        for i in range(self.rounds):
            roundstarttime = clock()
            if verbose:
                print ' Round %-25i  real   abs    overhead' % (i+1)
            for j in range(len(tests)):
                name, t = tests[j]
                if verbose:
                    print '%30s:' % name,
                t.run(cruns)
                if verbose:
                    print '  %.3fr %.3fa %.3fo' % t.last_timing
            if verbose:
                print '                                 ----------------------'
                print '            Average round time:      %.3f seconds' % \
                    ((clock() - roundtime)/(i+1))
                print
            else:
                print '%d done in %.3f seconds' % (i+1, (clock() - roundstarttime))
        self.roundtime = (clock() - roundtime) / self.rounds
        print

    def print_stat(self, compare_to=None, hidenoise=0):

        if not compare_to:
            print '%-30s       min run    avg run    per oprn  overhead' % 'Tests:'
            print '-'*77
            tests = self.tests.items()
            tests.sort()
            totalmintime = 0
            for name,t in tests:
                mintime,avg,op_avg,ov_avg = t.stat()
                totalmintime += mintime
                print '%30s: %9.2f ms %9.2f ms %6.2f us  %6.2f' % \
                      (name,mintime*1000.0,avg*1000.0,op_avg*1000000.0,ov_avg*1000.0)
            print '-'*77
            print '%30s: %9.2f ms' % \
                  ('Notional minimum round time', totalmintime * 1000.0)

        else:
            print 'Comparing with: %s (rounds=%i, warp=%i)' % \
                  (compare_to.name,compare_to.rounds,compare_to.warp)
            print '%-30s      min run     cmp run     avg run      diff' % \
                  'Tests:'
            print '-'*77
            tests = self.tests.items()
            tests.sort()
            compatible = 1
            totalmintime = other_totalmintime = 0
            for name, t in tests:
                mintime, avg, op_avg, ov_avg = t.stat()
                totalmintime += mintime
                try:
                    other = compare_to.tests[name]
                except KeyError:
                    other = None
                if other and other.version == t.version and \
                   other.operations == t.operations:
                    mintime1, avg1, op_avg1, ov_avg1 = other.stat()
                    other_totalmintime += mintime1
                    diff = ((mintime*self.warp)/(mintime1*other.warp) - 1.0)*100.0
                    if hidenoise and abs(qop_avg) < 10:
                        diff = ''
                    else:
                        diff = '%+7.2f%%' % diff
                else:
                    qavg, diff = 'n/a', 'n/a'
                    compatible = 0
                print '%30s: %8.2f ms %8.2f ms %8.2f ms  %8s' % \
                      (name,mintime*1000.0,mintime1*1000.0 * compare_to.warp/self.warp, avg*1000.0,diff)
            print '-'*77
            #
            # Summarise test results
            #
            if compatible and compare_to.roundtime > 0 and \
               compare_to.version == self.version:
                print '%30s: %8.2f ms %8.2f ms              %+7.2f%%' % \
                      ('Notional minimum round time', totalmintime * 1000.0,
                      other_totalmintime * 1000.0 * compare_to.warp/self.warp,
                       ((totalmintime*self.warp)/
                        (other_totalmintime*compare_to.warp)-1.0)*100.0)
            else:
                print '%30s: %9.2f ms                    n/a' % \
                      ('Notional minimum round time', totalmintime * 1000.0)
        print

def print_machine():

    import platform
    print 'Machine Details:'
    print '   Platform ID:  %s' % platform.platform()
    print '   Executable:   %s' % sys.executable
    # There's a bug in Python 2.2b1+...
    if sys.version[:6] == '2.2b1+':
        return
    print '   Python:       %s' % platform.python_version()
    print '   Compiler:     %s' % platform.python_compiler()
    buildno, builddate = platform.python_build()
    print '   Build:        %s (#%s)' % (builddate, buildno)

class PyBenchCmdline(Application):

    header = ("PYBENCH - a benchmark test suite for Python "
              "interpreters/compilers.")

    version = __version__

    options = [ArgumentOption('-n','number of rounds',Setup.Number_of_rounds),
               ArgumentOption('-f','save benchmark to file arg',''),
               ArgumentOption('-c','compare benchmark with the one in file arg',''),
               ArgumentOption('-s','show benchmark in file arg, then exit',''),
               SwitchOption('-S','show statistics of benchmarks',0),
               ArgumentOption('-w','set warp factor to arg',Setup.Warp_factor),
               SwitchOption('-d','hide noise in compares', 0),
               SwitchOption('-v','verbose output (not recommended)', 0),
               SwitchOption('--no-gc','disable garbage collection', 0),
               SwitchOption('--no-syscheck',
                    '"disable" sys check interval (set to sys.maxint)', 0),
               ArgumentOption('-t', 'tests containing substring', ''),
               ArgumentOption('-C', 'number of calibration runs (default 20)', 20)
               ]

    about = """\
The normal operation is to run the suite and display the
results. Use -f to save them for later reuse or comparisms.

Examples:

python1.5 pybench.py -w 100 -f p15
python1.4 pybench.py -w 100 -f p14
python pybench.py -s p15 -c p14
"""
    copyright = __copyright__

    def handle_S(self, value):

        """ Display one line stats for each benchmark file given on the
            command line.

        """
        for benchmark in self.files:
            try:
                f = open(benchmark, 'rb')
                bench = pickle.load(f)
                f.close()
            except IOError:
                print '* Error opening/reading file %s' % repr(benchmark)
            else:
                print '%s,%-.2f,ms' % (benchmark, bench.roundtime*1000.0)
        return 0

    def main(self):

        rounds = self.values['-n']
        reportfile = self.values['-f']
        show_bench = self.values['-s']
        compare_to = self.values['-c']
        hidenoise = self.values['-d']
        warp = self.values['-w']
        nogc = self.values['--no-gc']
        limitnames = self.values['-t']
        verbose = self.verbose
        nosyscheck = self.values['--no-syscheck']
        cruns = self.values['-C']
        print "CRUNS:", cruns
        
        print 'PYBENCH',__version__

        # Switch off GC
        if nogc:
            try:
                import gc
            except ImportError:
                nogc = 0
            else:
                if self.values['--no-gc']:
                    gc.disable()
                    print 'NO GC'

        # maximise sys check interval
        if nosyscheck:
            sys.setcheckinterval(sys.maxint)
            print 'CHECKINTERVAL =', sys.maxint

        print

        if not compare_to:
            print_machine()
            print

        if compare_to:
            try:
                f = open(compare_to,'rb')
                bench = pickle.load(f)
                bench.name = compare_to
                f.close()
                compare_to = bench
            except IOError:
                print '* Error opening/reading file',compare_to
                compare_to = None

        if show_bench:
            try:
                f = open(show_bench,'rb')
                bench = pickle.load(f)
                bench.name = show_bench
                f.close()
                print 'Benchmark: %s (rounds=%i, warp=%i)' % \
                      (bench.name,bench.rounds,bench.warp)
                print
                bench.print_stat(compare_to, hidenoise)
            except IOError:
                print '* Error opening/reading file',show_bench
                print
            return

        if reportfile:
            if nogc:
                print 'Benchmark: %s (rounds=%i, warp=%i, no GC)' % \
                      (reportfile,rounds,warp)
            else:
                print 'Benchmark: %s (rounds=%i, warp=%i)' % \
                      (reportfile,rounds,warp)
            print

        # Create benchmark object
        bench = Benchmark()
        bench.rounds = rounds
        bench.load_tests(Setup, warp, limitnames, verbose)
        try:
            bench.run(verbose, cruns)
        except KeyboardInterrupt:
            print
            print '*** KeyboardInterrupt -- Aborting'
            print
            return
        bench.print_stat(compare_to)
        # ring bell
        sys.stderr.write('\007')

        if reportfile:
            try:
                f = open(reportfile,'wb')
                bench.name = reportfile
                pickle.dump(bench,f)
                f.close()
            except IOError:
                print '* Error opening/writing reportfile'

if __name__ == '__main__':
    PyBenchCmdline()
