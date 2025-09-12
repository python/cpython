import os
import sys
import io
import traceback
import gc
import subprocess
from test import support
import unittest
import unittest.mock
import test.test_unittest
from test.test_unittest.test_result import BufferedWriter


@support.force_not_colorized_test_class
class Test_TestProgram(unittest.TestCase):

    def test_discovery_from_dotted_path(self):
        loader = unittest.TestLoader()

        tests = [self]
        expectedPath = os.path.abspath(os.path.dirname(test.test_unittest.__file__))

        self.wasRun = False
        def _find_tests(start_dir, pattern):
            self.wasRun = True
            self.assertEqual(start_dir, expectedPath)
            return tests
        loader._find_tests = _find_tests
        suite = loader.discover('test.test_unittest')
        self.assertTrue(self.wasRun)
        self.assertEqual(suite._tests, tests)

    # Horrible white box test
    def testNoExit(self):
        result = object()
        test = object()

        class FakeRunner(object):
            def run(self, test, debug=False):
                self.test = test
                return result

        runner = FakeRunner()

        oldParseArgs = unittest.TestProgram.parseArgs
        def restoreParseArgs():
            unittest.TestProgram.parseArgs = oldParseArgs
        unittest.TestProgram.parseArgs = lambda *args: None
        self.addCleanup(restoreParseArgs)

        def removeTest():
            del unittest.TestProgram.test
        unittest.TestProgram.test = test
        self.addCleanup(removeTest)

        program = unittest.TestProgram(testRunner=runner, exit=False, verbosity=2)

        self.assertEqual(program.result, result)
        self.assertEqual(runner.test, test)
        self.assertEqual(program.verbosity, 2)

    class FooBar(unittest.TestCase):
        def testPass(self):
            pass
        def testFail(self):
            raise AssertionError
        def testError(self):
            1/0
        @unittest.skip('skipping')
        def testSkipped(self):
            raise AssertionError
        @unittest.expectedFailure
        def testExpectedFailure(self):
            raise AssertionError
        @unittest.expectedFailure
        def testUnexpectedSuccess(self):
            pass

    class Empty(unittest.TestCase):
        pass

    class TestLoader(unittest.TestLoader):
        """Test loader that returns a suite containing the supplied testcase."""

        def __init__(self, testcase):
            self.testcase = testcase

        def loadTestsFromModule(self, module):
            return self.suiteClass(
                [self.loadTestsFromTestCase(self.testcase)])

        def loadTestsFromNames(self, names, module):
            return self.suiteClass(
                [self.loadTestsFromTestCase(self.testcase)])

    def test_defaultTest_with_string(self):
        class FakeRunner(object):
            def run(self, test, debug=False):
                self.test = test
                return True

        old_argv = sys.argv
        sys.argv = ['faketest']
        runner = FakeRunner()
        program = unittest.TestProgram(testRunner=runner, exit=False,
                                       defaultTest='test.test_unittest',
                                       testLoader=self.TestLoader(self.FooBar))
        sys.argv = old_argv
        self.assertEqual(('test.test_unittest',), program.testNames)

    def test_defaultTest_with_iterable(self):
        class FakeRunner(object):
            def run(self, test, debug=False):
                self.test = test
                return True

        old_argv = sys.argv
        sys.argv = ['faketest']
        runner = FakeRunner()
        program = unittest.TestProgram(
            testRunner=runner, exit=False,
            defaultTest=['test.test_unittest', 'test.test_unittest2'],
            testLoader=self.TestLoader(self.FooBar))
        sys.argv = old_argv
        self.assertEqual(['test.test_unittest', 'test.test_unittest2'],
                          program.testNames)

    def test_NonExit(self):
        stream = BufferedWriter()
        program = unittest.main(exit=False,
                                argv=["foobar"],
                                testRunner=unittest.TextTestRunner(stream=stream),
                                testLoader=self.TestLoader(self.FooBar))
        self.assertHasAttr(program, 'result')
        out = stream.getvalue()
        self.assertIn('\nFAIL: testFail ', out)
        self.assertIn('\nERROR: testError ', out)
        self.assertIn('\nUNEXPECTED SUCCESS: testUnexpectedSuccess ', out)
        expected = ('\n\nFAILED (failures=1, errors=1, skipped=1, '
                    'expected failures=1, unexpected successes=1)\n')
        self.assertEndsWith(out, expected)

    def test_Exit(self):
        stream = BufferedWriter()
        with self.assertRaises(SystemExit) as cm:
            unittest.main(
                argv=["foobar"],
                testRunner=unittest.TextTestRunner(stream=stream),
                exit=True,
                testLoader=self.TestLoader(self.FooBar))
        self.assertEqual(cm.exception.code, 1)
        out = stream.getvalue()
        self.assertIn('\nFAIL: testFail ', out)
        self.assertIn('\nERROR: testError ', out)
        self.assertIn('\nUNEXPECTED SUCCESS: testUnexpectedSuccess ', out)
        expected = ('\n\nFAILED (failures=1, errors=1, skipped=1, '
                    'expected failures=1, unexpected successes=1)\n')
        self.assertEndsWith(out, expected)

    def test_ExitAsDefault(self):
        stream = BufferedWriter()
        with self.assertRaises(SystemExit):
            unittest.main(
                argv=["foobar"],
                testRunner=unittest.TextTestRunner(stream=stream),
                testLoader=self.TestLoader(self.FooBar))
        out = stream.getvalue()
        self.assertIn('\nFAIL: testFail ', out)
        self.assertIn('\nERROR: testError ', out)
        self.assertIn('\nUNEXPECTED SUCCESS: testUnexpectedSuccess ', out)
        expected = ('\n\nFAILED (failures=1, errors=1, skipped=1, '
                    'expected failures=1, unexpected successes=1)\n')
        self.assertEndsWith(out, expected)

    def test_ExitSkippedSuite(self):
        stream = BufferedWriter()
        with self.assertRaises(SystemExit) as cm:
            unittest.main(
                argv=["foobar", "-k", "testSkipped"],
                testRunner=unittest.TextTestRunner(stream=stream),
                testLoader=self.TestLoader(self.FooBar))
        self.assertEqual(cm.exception.code, 0)
        out = stream.getvalue()
        expected = '\n\nOK (skipped=1)\n'
        self.assertEndsWith(out, expected)

    def test_ExitEmptySuite(self):
        stream = BufferedWriter()
        with self.assertRaises(SystemExit) as cm:
            unittest.main(
                argv=["empty"],
                testRunner=unittest.TextTestRunner(stream=stream),
                testLoader=self.TestLoader(self.Empty))
        self.assertEqual(cm.exception.code, 5)
        out = stream.getvalue()
        self.assertIn('\nNO TESTS RAN\n', out)

    class TestRaise(unittest.TestCase):
        td_log = ''
        class Error(Exception):
            pass
        def test_raise(self):
            self = self
            raise self.Error
        def setUp(self):
            self.addCleanup(self.clInstance)
            self.addClassCleanup(self.clClass)
            unittest.case.addModuleCleanup(self.clModule)
        def tearDown(self):
            __class__.td_log += 't'
            1 / 0  # should not block further cleanups
        def clInstance(self):
            __class__.td_log += 'c'
        @classmethod
        def tearDownClass(cls):
            __class__.td_log += 'T'
        @classmethod
        def clClass(cls):
            __class__.td_log += 'C'
            2 / 0
        @staticmethod
        def clModule():
            __class__.td_log += 'M'

    class TestRaiseLoader(unittest.TestLoader):
        def loadTestsFromModule(self, module):
            return self.suiteClass(
                [self.loadTestsFromTestCase(Test_TestProgram.TestRaise)])

        def loadTestsFromNames(self, names, module):
            return self.suiteClass(
                [self.loadTestsFromTestCase(Test_TestProgram.TestRaise)])

    def test_debug(self):
        self.TestRaise.td_log = ''
        try:
            unittest.main(
                argv=["TestRaise", "--debug"],
                testRunner=unittest.TextTestRunner(stream=io.StringIO()),
                testLoader=self.TestRaiseLoader())
        except self.TestRaise.Error as e:
            # outer pm handling of original exception
            assert self.TestRaise.td_log == ''  # still set up!
        else:
            self.fail("TestRaise not raised")
        # test delayed automatic teardown after leaving outer exception
        # handling. Note, the explicit e.pm_teardown() variant is tested below
        # in test_inline_debugging().
        if not hasattr(sys, 'getrefcount'):
            # PyPy etc.
            gc.collect()
        assert self.TestRaise.td_log == 'tcTCM', self.TestRaise.td_log

    def test_no_debug(self):
        self.assertRaises(
            SystemExit,
            unittest.main,
            argv=["TestRaise"],
            testRunner=unittest.TextTestRunner(stream=io.StringIO()),
            testLoader=self.TestRaiseLoader())

    def test_inline_debugging(self):
        from test.test_pdb import PdbTestInput
        # post-mortem --pdb
        out, err = io.StringIO(), io.StringIO()
        try:
            with unittest.mock.patch('sys.stdout', out),\
                    unittest.mock.patch('sys.stderr', err),\
                    PdbTestInput(['c'] * 3):
                unittest.main(
                    argv=["TestRaise", "--pdb"],
                    testRunner=unittest.TextTestRunner(stream=err),
                    testLoader=self.TestRaiseLoader())
        except SystemExit:
            assert '-> raise self.Error\n(Pdb)' in out.getvalue(), 'out:' + out.getvalue()
            assert '-> 2 / 0\n(Pdb)' in out.getvalue(), 'out:' + out.getvalue()
            assert 'FAILED (errors=3)' in err.getvalue(), 'err:' + err.getvalue()
        else:
            self.fail("SystemExit not raised")

        # post-mortem --pm=<DebuggerClass>, early user debugger quit
        out, err = io.StringIO(), io.StringIO()
        self.TestRaise.td_log = ''
        try:
            with unittest.mock.patch('sys.stdout', out),\
                    unittest.mock.patch('sys.stderr', err),\
                    PdbTestInput(['q']):
                unittest.main(
                    argv=["TestRaise", "--pm=pdb.Pdb"],
                    testRunner=unittest.TextTestRunner(stream=err),
                    testLoader=self.TestRaiseLoader())
        except unittest.case.DebuggerQuit as e:
            assert e.__context__.__class__ == self.TestRaise.Error
            assert self.TestRaise.td_log == ''  # still set up!
            assert out.getvalue().endswith('-> raise self.Error\n(Pdb) q\n'), 'out:' + out.getvalue()
            # test explicit pm teardown variant.
            e.pm_teardown()
            assert self.TestRaise.td_log == 'tcTCM', self.TestRaise.td_log
            assert e.pm_teardown.result.testsRun == 1
            e_hold = e  # noqa
        else:
            self.fail("DebuggerQuit not raised")
        # delayed teardowns must not be repeated
        e_hold.pm_teardown()
        del e_hold
        gc.collect()
        assert self.TestRaise.td_log == 'tcTCM', self.TestRaise.td_log

        # --trace
        out, err = io.StringIO(), io.StringIO()
        try:
            with unittest.mock.patch('sys.stdout', out), PdbTestInput(['c']):
                unittest.main(
                    argv=["TestRaise", "--trace"],
                    testRunner=unittest.TextTestRunner(stream=err),
                    testLoader=self.TestRaiseLoader())
        except SystemExit:
            assert '-> self = self\n(Pdb)' in out.getvalue(), 'out:' + out.getvalue()
            assert 'FAILED (errors=3)' in err.getvalue(), 'err:' + err.getvalue()
        else:
            self.fail("SystemExit not raised")


class InitialisableProgram(unittest.TestProgram):
    exit = False
    result = None
    verbosity = 1
    defaultTest = None
    tb_locals = False
    debug = False
    testRunner = None
    testLoader = unittest.defaultTestLoader
    module = '__main__'
    progName = 'test'
    test = 'test'
    def __init__(self, *args):
        pass

RESULT = object()

class FakeRunner(object):
    initArgs = None
    test = None
    raiseError = 0

    def __init__(self, **kwargs):
        FakeRunner.initArgs = kwargs
        if FakeRunner.raiseError:
            FakeRunner.raiseError -= 1
            raise TypeError

    def run(self, test, debug=False):
        FakeRunner.test = test
        return RESULT


@support.requires_subprocess()
class TestCommandLineArgs(unittest.TestCase):

    def setUp(self):
        self.program = InitialisableProgram()
        self.program.createTests = lambda: None
        FakeRunner.initArgs = None
        FakeRunner.test = None
        FakeRunner.raiseError = 0

    def testVerbosity(self):
        program = self.program

        for opt in '-q', '--quiet':
            program.verbosity = 1
            program.parseArgs([None, opt])
            self.assertEqual(program.verbosity, 0)

        for opt in '-v', '--verbose':
            program.verbosity = 1
            program.parseArgs([None, opt])
            self.assertEqual(program.verbosity, 2)

    def testBufferCatchFailfast(self):
        program = self.program
        for arg, attr in (('buffer', 'buffer'), ('failfast', 'failfast'),
                      ('catch', 'catchbreak')):

            setattr(program, attr, None)
            program.parseArgs([None])
            self.assertIs(getattr(program, attr), False)

            false = []
            setattr(program, attr, false)
            program.parseArgs([None])
            self.assertIs(getattr(program, attr), false)

            true = [42]
            setattr(program, attr, true)
            program.parseArgs([None])
            self.assertIs(getattr(program, attr), true)

            short_opt = '-%s' % arg[0]
            long_opt = '--%s' % arg
            for opt in short_opt, long_opt:
                setattr(program, attr, None)
                program.parseArgs([None, opt])
                self.assertIs(getattr(program, attr), True)

                setattr(program, attr, False)
                with support.captured_stderr() as stderr, \
                    self.assertRaises(SystemExit) as cm:
                    program.parseArgs([None, opt])
                self.assertEqual(cm.exception.args, (2,))

                setattr(program, attr, True)
                with support.captured_stderr() as stderr, \
                    self.assertRaises(SystemExit) as cm:
                    program.parseArgs([None, opt])
                self.assertEqual(cm.exception.args, (2,))

    def testWarning(self):
        """Test the warnings argument"""
        # see #10535
        class FakeTP(unittest.TestProgram):
            def parseArgs(self, *args, **kw): pass
            def runTests(self, *args, **kw): pass
        warnoptions = sys.warnoptions[:]
        try:
            sys.warnoptions[:] = []
            # no warn options, no arg -> default
            self.assertEqual(FakeTP().warnings, 'default')
            # no warn options, w/ arg -> arg value
            self.assertEqual(FakeTP(warnings='ignore').warnings, 'ignore')
            sys.warnoptions[:] = ['somevalue']
            # warn options, no arg -> None
            # warn options, w/ arg -> arg value
            self.assertEqual(FakeTP().warnings, None)
            self.assertEqual(FakeTP(warnings='ignore').warnings, 'ignore')
        finally:
            sys.warnoptions[:] = warnoptions

    def testRunTestsRunnerClass(self):
        program = self.program

        program.testRunner = FakeRunner
        program.verbosity = 'verbosity'
        program.failfast = 'failfast'
        program.buffer = 'buffer'
        program.warnings = 'warnings'
        program.durations = '5'

        program.runTests()

        self.assertEqual(FakeRunner.initArgs, {'verbosity': 'verbosity',
                                                'failfast': 'failfast',
                                                'buffer': 'buffer',
                                                'tb_locals': False,
                                                'warnings': 'warnings',
                                                'durations': '5'})
        self.assertEqual(FakeRunner.test, 'test')
        self.assertIs(program.result, RESULT)

    def testRunTestsRunnerInstance(self):
        program = self.program

        program.testRunner = FakeRunner()
        FakeRunner.initArgs = None

        program.runTests()

        # A new FakeRunner should not have been instantiated
        self.assertIsNone(FakeRunner.initArgs)

        self.assertEqual(FakeRunner.test, 'test')
        self.assertIs(program.result, RESULT)

    def test_locals(self):
        program = self.program

        program.testRunner = FakeRunner
        program.parseArgs([None, '--locals'])
        self.assertEqual(True, program.tb_locals)
        program.runTests()
        self.assertEqual(FakeRunner.initArgs, {'buffer': False,
                                               'failfast': False,
                                               'tb_locals': True,
                                               'verbosity': 1,
                                               'warnings': None,
                                               'durations': None})

    def test_debug(self):
        program = self.program
        program.testRunner = FakeRunner
        program.parseArgs([None, '--debug'])
        self.assertTrue(program.debug)
        program.parseArgs([None, '--pdb'])
        self.assertTrue(program.pdb)
        program.parseArgs([None, '--pm=pdb'])
        self.assertEqual(program.pm, 'pdb')
        program.parseArgs([None, '--trace'])
        self.assertTrue(program.trace)


    def testRunTestsOldRunnerClass(self):
        program = self.program

        # Two TypeErrors are needed to fall all the way back to old-style
        # runners - one to fail tb_locals, one to fail buffer etc.
        FakeRunner.raiseError = 2
        program.testRunner = FakeRunner
        program.verbosity = 'verbosity'
        program.failfast = 'failfast'
        program.buffer = 'buffer'
        program.test = 'test'
        program.durations = '0'

        program.runTests()

        # If initialising raises a type error it should be retried
        # without the new keyword arguments
        self.assertEqual(FakeRunner.initArgs, {})
        self.assertEqual(FakeRunner.test, 'test')
        self.assertIs(program.result, RESULT)

    def testCatchBreakInstallsHandler(self):
        module = sys.modules['unittest.main']
        original = module.installHandler
        def restore():
            module.installHandler = original
        self.addCleanup(restore)

        self.installed = False
        def fakeInstallHandler():
            self.installed = True
        module.installHandler = fakeInstallHandler

        program = self.program
        program.catchbreak = True
        program.durations = None

        program.testRunner = FakeRunner

        program.runTests()
        self.assertTrue(self.installed)

    def _patch_isfile(self, names, exists=True):
        def isfile(path):
            return path in names
        original = os.path.isfile
        os.path.isfile = isfile
        def restore():
            os.path.isfile = original
        self.addCleanup(restore)


    def testParseArgsFileNames(self):
        # running tests with filenames instead of module names
        program = self.program
        argv = ['progname', 'foo.py', 'bar.Py', 'baz.PY', 'wing.txt']
        self._patch_isfile(argv)

        program.createTests = lambda: None
        program.parseArgs(argv)

        # note that 'wing.txt' is not a Python file so the name should
        # *not* be converted to a module name
        expected = ['foo', 'bar', 'baz', 'wing.txt']
        self.assertEqual(program.testNames, expected)


    def testParseArgsFilePaths(self):
        program = self.program
        argv = ['progname', 'foo/bar/baz.py', 'green\\red.py']
        self._patch_isfile(argv)

        program.createTests = lambda: None
        program.parseArgs(argv)

        expected = ['foo.bar.baz', 'green.red']
        self.assertEqual(program.testNames, expected)


    def testParseArgsNonExistentFiles(self):
        program = self.program
        argv = ['progname', 'foo/bar/baz.py', 'green\\red.py']
        self._patch_isfile([])

        program.createTests = lambda: None
        program.parseArgs(argv)

        self.assertEqual(program.testNames, argv[1:])

    def testParseArgsAbsolutePathsThatCanBeConverted(self):
        cur_dir = os.getcwd()
        program = self.program
        def _join(name):
            return os.path.join(cur_dir, name)
        argv = ['progname', _join('foo/bar/baz.py'), _join('green\\red.py')]
        self._patch_isfile(argv)

        program.createTests = lambda: None
        program.parseArgs(argv)

        expected = ['foo.bar.baz', 'green.red']
        self.assertEqual(program.testNames, expected)

    def testParseArgsAbsolutePathsThatCannotBeConverted(self):
        program = self.program
        drive = os.path.splitdrive(os.getcwd())[0]
        argv = ['progname', f'{drive}/foo/bar/baz.py', f'{drive}/green/red.py']
        self._patch_isfile(argv)

        program.createTests = lambda: None
        program.parseArgs(argv)

        self.assertEqual(program.testNames, argv[1:])

        # it may be better to use platform specific functions to normalise paths
        # rather than accepting '.PY' and '\' as file separator on Linux / Mac
        # it would also be better to check that a filename is a valid module
        # identifier (we have a regex for this in loader.py)
        # for invalid filenames should we raise a useful error rather than
        # leaving the current error message (import of filename fails) in place?

    def testParseArgsSelectedTestNames(self):
        program = self.program
        argv = ['progname', '-k', 'foo', '-k', 'bar', '-k', '*pat*']

        program.createTests = lambda: None
        program.parseArgs(argv)

        self.assertEqual(program.testNamePatterns, ['*foo*', '*bar*', '*pat*'])

    def testSelectedTestNamesFunctionalTest(self):
        def run_unittest(args):
            # Use -E to ignore PYTHONSAFEPATH env var
            cmd = [sys.executable, '-E', '-m', 'unittest'] + args
            p = subprocess.Popen(cmd,
                stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, cwd=os.path.dirname(__file__))
            with p:
                _, stderr = p.communicate()
            return stderr.decode()

        t = '_test_warnings'
        self.assertIn('Ran 5 tests', run_unittest([t]))
        self.assertIn('Ran 5 tests', run_unittest(['-k', 'TestWarnings', t]))
        self.assertIn('Ran 5 tests', run_unittest(['discover', '-p', '*_test*', '-k', 'TestWarnings']))
        self.assertIn('Ran 1 test ', run_unittest(['-k', 'f', t]))
        self.assertIn('Ran 5 tests', run_unittest(['-k', 't', t]))
        self.assertIn('Ran 2 tests', run_unittest(['-k', '*t', t]))
        self.assertIn('Ran 5 tests', run_unittest(['-k', '*test_warnings.*Warning*', t]))
        self.assertIn('Ran 1 test ', run_unittest(['-k', '*test_warnings.*warning*', t]))


if __name__ == '__main__':
    unittest.main()
