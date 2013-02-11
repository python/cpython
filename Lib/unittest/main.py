"""Unittest main program"""

import sys
import optparse
import os

from . import loader, runner
from .signals import installHandler

__unittest = True

FAILFAST     = "  -f, --failfast   Stop on first failure\n"
CATCHBREAK   = "  -c, --catch      Catch control-C and display results\n"
BUFFEROUTPUT = "  -b, --buffer     Buffer stdout and stderr during test runs\n"

USAGE_AS_MAIN = """\
Usage: %(progName)s [options] [tests]

Options:
  -h, --help       Show this message
  -v, --verbose    Verbose output
  -q, --quiet      Minimal output
%(failfast)s%(catchbreak)s%(buffer)s
Examples:
  %(progName)s test_module               - run tests from test_module
  %(progName)s module.TestClass          - run tests from module.TestClass
  %(progName)s module.Class.test_method  - run specified test method

[tests] can be a list of any number of test modules, classes and test
methods.

Alternative Usage: %(progName)s discover [options]

Options:
  -v, --verbose    Verbose output
%(failfast)s%(catchbreak)s%(buffer)s  -s directory     Directory to start discovery ('.' default)
  -p pattern       Pattern to match test files ('test*.py' default)
  -t directory     Top level directory of project (default to
                   start directory)

For test discovery all test modules must be importable from the top
level directory of the project.
"""

USAGE_FROM_MODULE = """\
Usage: %(progName)s [options] [test] [...]

Options:
  -h, --help       Show this message
  -v, --verbose    Verbose output
  -q, --quiet      Minimal output
%(failfast)s%(catchbreak)s%(buffer)s
Examples:
  %(progName)s                               - run default set of tests
  %(progName)s MyTestSuite                   - run suite 'MyTestSuite'
  %(progName)s MyTestCase.testSomething      - run MyTestCase.testSomething
  %(progName)s MyTestCase                    - run all 'test*' test methods
                                               in MyTestCase
"""

def _convert_name(name):
    # on Linux / Mac OS X 'foo.PY' is not importable, but on
    # Windows it is. Simpler to do a case insensitive match
    # a better check would be to check that the name is a
    # valid Python module name.
    if os.path.isfile(name) and name.lower().endswith('.py'):
        if os.path.isabs(name):
            rel_path = os.path.relpath(name, os.getcwd())
            if os.path.isabs(rel_path) or rel_path.startswith(os.pardir):
                return name
            name = rel_path
        # on Windows both '\' and '/' are used as path
        # separators. Better to replace both than rely on os.path.sep
        return name[:-3].replace('\\', '.').replace('/', '.')
    return name

def _convert_names(names):
    return [_convert_name(name) for name in names]


class TestProgram(object):
    """A command-line program that runs a set of tests; this is primarily
       for making test modules conveniently executable.
    """
    USAGE = USAGE_FROM_MODULE

    # defaults for testing
    failfast = catchbreak = buffer = progName = warnings = None

    def __init__(self, module='__main__', defaultTest=None, argv=None,
                    testRunner=None, testLoader=loader.defaultTestLoader,
                    exit=True, verbosity=1, failfast=None, catchbreak=None,
                    buffer=None, warnings=None):
        if isinstance(module, str):
            self.module = __import__(module)
            for part in module.split('.')[1:]:
                self.module = getattr(self.module, part)
        else:
            self.module = module
        if argv is None:
            argv = sys.argv

        self.exit = exit
        self.failfast = failfast
        self.catchbreak = catchbreak
        self.verbosity = verbosity
        self.buffer = buffer
        if warnings is None and not sys.warnoptions:
            # even if DreprecationWarnings are ignored by default
            # print them anyway unless other warnings settings are
            # specified by the warnings arg or the -W python flag
            self.warnings = 'default'
        else:
            # here self.warnings is set either to the value passed
            # to the warnings args or to None.
            # If the user didn't pass a value self.warnings will
            # be None. This means that the behavior is unchanged
            # and depends on the values passed to -W.
            self.warnings = warnings
        self.defaultTest = defaultTest
        self.testRunner = testRunner
        self.testLoader = testLoader
        self.progName = os.path.basename(argv[0])
        self.parseArgs(argv)
        self.runTests()

    def usageExit(self, msg=None):
        if msg:
            print(msg)
        usage = {'progName': self.progName, 'catchbreak': '', 'failfast': '',
                 'buffer': ''}
        if self.failfast != False:
            usage['failfast'] = FAILFAST
        if self.catchbreak != False:
            usage['catchbreak'] = CATCHBREAK
        if self.buffer != False:
            usage['buffer'] = BUFFEROUTPUT
        print(self.USAGE % usage)
        sys.exit(2)

    def parseArgs(self, argv):
        if ((len(argv) > 1 and argv[1].lower() == 'discover') or
            (len(argv) == 1 and self.module is None)):
            self._do_discovery(argv[2:])
            return

        parser = self._getOptParser()
        options, args = parser.parse_args(argv[1:])
        self._setAttributesFromOptions(options)

        if len(args) == 0 and self.module is None:
            # this allows "python -m unittest -v" to still work for
            # test discovery. This means -c / -b / -v / -f options will
            # be handled twice, which is harmless but not ideal.
            self._do_discovery(argv[1:])
            return

        if len(args) == 0 and self.defaultTest is None:
            # createTests will load tests from self.module
            self.testNames = None
        elif len(args) > 0:
            self.testNames = _convert_names(args)
            if __name__ == '__main__':
                # to support python -m unittest ...
                self.module = None
        else:
            self.testNames = (self.defaultTest,)
        self.createTests()

    def createTests(self):
        if self.testNames is None:
            self.test = self.testLoader.loadTestsFromModule(self.module)
        else:
            self.test = self.testLoader.loadTestsFromNames(self.testNames,
                                                           self.module)

    def _getOptParser(self):
        import optparse
        parser = optparse.OptionParser()
        parser.prog = self.progName
        parser.add_option('-v', '--verbose', dest='verbose', default=False,
                          help='Verbose output', action='store_true')
        parser.add_option('-q', '--quiet', dest='quiet', default=False,
                          help='Quiet output', action='store_true')

        if self.failfast != False:
            parser.add_option('-f', '--failfast', dest='failfast', default=False,
                              help='Stop on first fail or error',
                              action='store_true')
        if self.catchbreak != False:
            parser.add_option('-c', '--catch', dest='catchbreak', default=False,
                              help='Catch ctrl-C and display results so far',
                              action='store_true')
        if self.buffer != False:
            parser.add_option('-b', '--buffer', dest='buffer', default=False,
                              help='Buffer stdout and stderr during tests',
                              action='store_true')
        return parser

    def _setAttributesFromOptions(self, options):
        # only set options from the parsing here
        # if they weren't set explicitly in the constructor
        if self.failfast is None:
            self.failfast = options.failfast
        if self.catchbreak is None:
            self.catchbreak = options.catchbreak
        if self.buffer is None:
            self.buffer = options.buffer

        if options.verbose:
            self.verbosity = 2
        elif options.quiet:
            self.verbosity = 0

    def _addDiscoveryOptions(self, parser):
        parser.add_option('-s', '--start-directory', dest='start', default='.',
                          help="Directory to start discovery ('.' default)")
        parser.add_option('-p', '--pattern', dest='pattern', default='test*.py',
                          help="Pattern to match tests ('test*.py' default)")
        parser.add_option('-t', '--top-level-directory', dest='top', default=None,
                          help='Top level directory of project (defaults to start directory)')

    def _do_discovery(self, argv, Loader=None):
        if Loader is None:
            Loader = lambda: self.testLoader

        # handle command line args for test discovery
        self.progName = '%s discover' % self.progName
        parser = self._getOptParser()
        self._addDiscoveryOptions(parser)

        options, args = parser.parse_args(argv)
        if len(args) > 3:
            self.usageExit()

        for name, value in zip(('start', 'pattern', 'top'), args):
            setattr(options, name, value)

        self._setAttributesFromOptions(options)

        start_dir = options.start
        pattern = options.pattern
        top_level_dir = options.top

        loader = Loader()
        self.test = loader.discover(start_dir, pattern, top_level_dir)

    def runTests(self):
        if self.catchbreak:
            installHandler()
        if self.testRunner is None:
            self.testRunner = runner.TextTestRunner
        if isinstance(self.testRunner, type):
            try:
                testRunner = self.testRunner(verbosity=self.verbosity,
                                             failfast=self.failfast,
                                             buffer=self.buffer,
                                             warnings=self.warnings)
            except TypeError:
                # didn't accept the verbosity, buffer or failfast arguments
                testRunner = self.testRunner()
        else:
            # it is assumed to be a TestRunner instance
            testRunner = self.testRunner
        self.result = testRunner.run(self.test)
        if self.exit:
            sys.exit(not self.result.wasSuccessful())

main = TestProgram
