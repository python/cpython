import io
import sys
import textwrap

from test.support import warnings_helper, captured_stdout, captured_stderr

import traceback
import unittest
from unittest.util import strclass


class MockTraceback(object):
    class TracebackException:
        def __init__(self, *args, **kwargs):
            self.capture_locals = kwargs.get('capture_locals', False)
        def format(self):
            result = ['A traceback']
            if self.capture_locals:
                result.append('locals')
            return result

def restore_traceback():
    unittest.result.traceback = traceback


def bad_cleanup1():
    print('do cleanup1')
    raise TypeError('bad cleanup1')


def bad_cleanup2():
    print('do cleanup2')
    raise ValueError('bad cleanup2')


class Test_TestResult(unittest.TestCase):
    # Note: there are not separate tests for TestResult.wasSuccessful(),
    # TestResult.errors, TestResult.failures, TestResult.testsRun or
    # TestResult.shouldStop because these only have meaning in terms of
    # other TestResult methods.
    #
    # Accordingly, tests for the aforenamed attributes are incorporated
    # in with the tests for the defining methods.
    ################################################################

    def test_init(self):
        result = unittest.TestResult()

        self.assertTrue(result.wasSuccessful())
        self.assertEqual(len(result.errors), 0)
        self.assertEqual(len(result.failures), 0)
        self.assertEqual(result.testsRun, 0)
        self.assertEqual(result.shouldStop, False)
        self.assertIsNone(result._stdout_buffer)
        self.assertIsNone(result._stderr_buffer)

    # "This method can be called to signal that the set of tests being
    # run should be aborted by setting the TestResult's shouldStop
    # attribute to True."
    def test_stop(self):
        result = unittest.TestResult()

        result.stop()

        self.assertEqual(result.shouldStop, True)

    # "Called when the test case test is about to be run. The default
    # implementation simply increments the instance's testsRun counter."
    def test_startTest(self):
        class Foo(unittest.TestCase):
            def test_1(self):
                pass

        test = Foo('test_1')

        result = unittest.TestResult()

        result.startTest(test)

        self.assertTrue(result.wasSuccessful())
        self.assertEqual(len(result.errors), 0)
        self.assertEqual(len(result.failures), 0)
        self.assertEqual(result.testsRun, 1)
        self.assertEqual(result.shouldStop, False)

        result.stopTest(test)

    # "Called after the test case test has been executed, regardless of
    # the outcome. The default implementation does nothing."
    def test_stopTest(self):
        class Foo(unittest.TestCase):
            def test_1(self):
                pass

        test = Foo('test_1')

        result = unittest.TestResult()

        result.startTest(test)

        self.assertTrue(result.wasSuccessful())
        self.assertEqual(len(result.errors), 0)
        self.assertEqual(len(result.failures), 0)
        self.assertEqual(result.testsRun, 1)
        self.assertEqual(result.shouldStop, False)

        result.stopTest(test)

        # Same tests as above; make sure nothing has changed
        self.assertTrue(result.wasSuccessful())
        self.assertEqual(len(result.errors), 0)
        self.assertEqual(len(result.failures), 0)
        self.assertEqual(result.testsRun, 1)
        self.assertEqual(result.shouldStop, False)

    # "Called before and after tests are run. The default implementation does nothing."
    def test_startTestRun_stopTestRun(self):
        result = unittest.TestResult()
        result.startTestRun()
        result.stopTestRun()

    # "addSuccess(test)"
    # ...
    # "Called when the test case test succeeds"
    # ...
    # "wasSuccessful() - Returns True if all tests run so far have passed,
    # otherwise returns False"
    # ...
    # "testsRun - The total number of tests run so far."
    # ...
    # "errors - A list containing 2-tuples of TestCase instances and
    # formatted tracebacks. Each tuple represents a test which raised an
    # unexpected exception. Contains formatted
    # tracebacks instead of sys.exc_info() results."
    # ...
    # "failures - A list containing 2-tuples of TestCase instances and
    # formatted tracebacks. Each tuple represents a test where a failure was
    # explicitly signalled using the TestCase.fail*() or TestCase.assert*()
    # methods. Contains formatted tracebacks instead
    # of sys.exc_info() results."
    def test_addSuccess(self):
        class Foo(unittest.TestCase):
            def test_1(self):
                pass

        test = Foo('test_1')

        result = unittest.TestResult()

        result.startTest(test)
        result.addSuccess(test)
        result.stopTest(test)

        self.assertTrue(result.wasSuccessful())
        self.assertEqual(len(result.errors), 0)
        self.assertEqual(len(result.failures), 0)
        self.assertEqual(result.testsRun, 1)
        self.assertEqual(result.shouldStop, False)

    # "addFailure(test, err)"
    # ...
    # "Called when the test case test signals a failure. err is a tuple of
    # the form returned by sys.exc_info(): (type, value, traceback)"
    # ...
    # "wasSuccessful() - Returns True if all tests run so far have passed,
    # otherwise returns False"
    # ...
    # "testsRun - The total number of tests run so far."
    # ...
    # "errors - A list containing 2-tuples of TestCase instances and
    # formatted tracebacks. Each tuple represents a test which raised an
    # unexpected exception. Contains formatted
    # tracebacks instead of sys.exc_info() results."
    # ...
    # "failures - A list containing 2-tuples of TestCase instances and
    # formatted tracebacks. Each tuple represents a test where a failure was
    # explicitly signalled using the TestCase.fail*() or TestCase.assert*()
    # methods. Contains formatted tracebacks instead
    # of sys.exc_info() results."
    def test_addFailure(self):
        class Foo(unittest.TestCase):
            def test_1(self):
                pass

        test = Foo('test_1')
        try:
            test.fail("foo")
        except:
            exc_info_tuple = sys.exc_info()

        result = unittest.TestResult()

        result.startTest(test)
        result.addFailure(test, exc_info_tuple)
        result.stopTest(test)

        self.assertFalse(result.wasSuccessful())
        self.assertEqual(len(result.errors), 0)
        self.assertEqual(len(result.failures), 1)
        self.assertEqual(result.testsRun, 1)
        self.assertEqual(result.shouldStop, False)

        test_case, formatted_exc = result.failures[0]
        self.assertIs(test_case, test)
        self.assertIsInstance(formatted_exc, str)

    # "addError(test, err)"
    # ...
    # "Called when the test case test raises an unexpected exception err
    # is a tuple of the form returned by sys.exc_info():
    # (type, value, traceback)"
    # ...
    # "wasSuccessful() - Returns True if all tests run so far have passed,
    # otherwise returns False"
    # ...
    # "testsRun - The total number of tests run so far."
    # ...
    # "errors - A list containing 2-tuples of TestCase instances and
    # formatted tracebacks. Each tuple represents a test which raised an
    # unexpected exception. Contains formatted
    # tracebacks instead of sys.exc_info() results."
    # ...
    # "failures - A list containing 2-tuples of TestCase instances and
    # formatted tracebacks. Each tuple represents a test where a failure was
    # explicitly signalled using the TestCase.fail*() or TestCase.assert*()
    # methods. Contains formatted tracebacks instead
    # of sys.exc_info() results."
    def test_addError(self):
        class Foo(unittest.TestCase):
            def test_1(self):
                pass

        test = Foo('test_1')
        try:
            raise TypeError()
        except:
            exc_info_tuple = sys.exc_info()

        result = unittest.TestResult()

        result.startTest(test)
        result.addError(test, exc_info_tuple)
        result.stopTest(test)

        self.assertFalse(result.wasSuccessful())
        self.assertEqual(len(result.errors), 1)
        self.assertEqual(len(result.failures), 0)
        self.assertEqual(result.testsRun, 1)
        self.assertEqual(result.shouldStop, False)

        test_case, formatted_exc = result.errors[0]
        self.assertIs(test_case, test)
        self.assertIsInstance(formatted_exc, str)

    def test_addError_locals(self):
        class Foo(unittest.TestCase):
            def test_1(self):
                1/0

        test = Foo('test_1')
        result = unittest.TestResult()
        result.tb_locals = True

        unittest.result.traceback = MockTraceback
        self.addCleanup(restore_traceback)
        result.startTestRun()
        test.run(result)
        result.stopTestRun()

        self.assertEqual(len(result.errors), 1)
        test_case, formatted_exc = result.errors[0]
        self.assertEqual('A tracebacklocals', formatted_exc)

    def test_addSubTest(self):
        class Foo(unittest.TestCase):
            def test_1(self):
                nonlocal subtest
                with self.subTest(foo=1):
                    subtest = self._subtest
                    try:
                        1/0
                    except ZeroDivisionError:
                        exc_info_tuple = sys.exc_info()
                    # Register an error by hand (to check the API)
                    result.addSubTest(test, subtest, exc_info_tuple)
                    # Now trigger a failure
                    self.fail("some recognizable failure")

        subtest = None
        test = Foo('test_1')
        result = unittest.TestResult()

        test.run(result)

        self.assertFalse(result.wasSuccessful())
        self.assertEqual(len(result.errors), 1)
        self.assertEqual(len(result.failures), 1)
        self.assertEqual(result.testsRun, 1)
        self.assertEqual(result.shouldStop, False)

        test_case, formatted_exc = result.errors[0]
        self.assertIs(test_case, subtest)
        self.assertIn("ZeroDivisionError", formatted_exc)
        test_case, formatted_exc = result.failures[0]
        self.assertIs(test_case, subtest)
        self.assertIn("some recognizable failure", formatted_exc)

    def testStackFrameTrimming(self):
        class Frame(object):
            class tb_frame(object):
                f_globals = {}
        result = unittest.TestResult()
        self.assertFalse(result._is_relevant_tb_level(Frame))

        Frame.tb_frame.f_globals['__unittest'] = True
        self.assertTrue(result._is_relevant_tb_level(Frame))

    def testFailFast(self):
        result = unittest.TestResult()
        result._exc_info_to_string = lambda *_: ''
        result.failfast = True
        result.addError(None, None)
        self.assertTrue(result.shouldStop)

        result = unittest.TestResult()
        result._exc_info_to_string = lambda *_: ''
        result.failfast = True
        result.addFailure(None, None)
        self.assertTrue(result.shouldStop)

        result = unittest.TestResult()
        result._exc_info_to_string = lambda *_: ''
        result.failfast = True
        result.addUnexpectedSuccess(None)
        self.assertTrue(result.shouldStop)

    def testFailFastSetByRunner(self):
        runner = unittest.TextTestRunner(stream=io.StringIO(), failfast=True)
        def test(result):
            self.assertTrue(result.failfast)
        result = runner.run(test)


class Test_TextTestResult(unittest.TestCase):
    maxDiff = None

    def testGetDescriptionWithoutDocstring(self):
        result = unittest.TextTestResult(None, True, 1)
        self.assertEqual(
                result.getDescription(self),
                'testGetDescriptionWithoutDocstring (' + __name__ +
                '.Test_TextTestResult)')

    def testGetSubTestDescriptionWithoutDocstring(self):
        with self.subTest(foo=1, bar=2):
            result = unittest.TextTestResult(None, True, 1)
            self.assertEqual(
                    result.getDescription(self._subtest),
                    'testGetSubTestDescriptionWithoutDocstring (' + __name__ +
                    '.Test_TextTestResult) (foo=1, bar=2)')
        with self.subTest('some message'):
            result = unittest.TextTestResult(None, True, 1)
            self.assertEqual(
                    result.getDescription(self._subtest),
                    'testGetSubTestDescriptionWithoutDocstring (' + __name__ +
                    '.Test_TextTestResult) [some message]')

    def testGetSubTestDescriptionWithoutDocstringAndParams(self):
        with self.subTest():
            result = unittest.TextTestResult(None, True, 1)
            self.assertEqual(
                    result.getDescription(self._subtest),
                    'testGetSubTestDescriptionWithoutDocstringAndParams '
                    '(' + __name__ + '.Test_TextTestResult) (<subtest>)')

    def testGetSubTestDescriptionForFalsyValues(self):
        expected = 'testGetSubTestDescriptionForFalsyValues (%s.Test_TextTestResult) [%s]'
        result = unittest.TextTestResult(None, True, 1)
        for arg in [0, None, []]:
            with self.subTest(arg):
                self.assertEqual(
                    result.getDescription(self._subtest),
                    expected % (__name__, arg)
                )

    def testGetNestedSubTestDescriptionWithoutDocstring(self):
        with self.subTest(foo=1):
            with self.subTest(baz=2, bar=3):
                result = unittest.TextTestResult(None, True, 1)
                self.assertEqual(
                        result.getDescription(self._subtest),
                        'testGetNestedSubTestDescriptionWithoutDocstring '
                        '(' + __name__ + '.Test_TextTestResult) (baz=2, bar=3, foo=1)')

    def testGetDuplicatedNestedSubTestDescriptionWithoutDocstring(self):
        with self.subTest(foo=1, bar=2):
            with self.subTest(baz=3, bar=4):
                result = unittest.TextTestResult(None, True, 1)
                self.assertEqual(
                        result.getDescription(self._subtest),
                        'testGetDuplicatedNestedSubTestDescriptionWithoutDocstring '
                        '(' + __name__ + '.Test_TextTestResult) (baz=3, bar=4, foo=1)')

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def testGetDescriptionWithOneLineDocstring(self):
        """Tests getDescription() for a method with a docstring."""
        result = unittest.TextTestResult(None, True, 1)
        self.assertEqual(
                result.getDescription(self),
               ('testGetDescriptionWithOneLineDocstring '
                '(' + __name__ + '.Test_TextTestResult)\n'
                'Tests getDescription() for a method with a docstring.'))

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def testGetSubTestDescriptionWithOneLineDocstring(self):
        """Tests getDescription() for a method with a docstring."""
        result = unittest.TextTestResult(None, True, 1)
        with self.subTest(foo=1, bar=2):
            self.assertEqual(
                result.getDescription(self._subtest),
               ('testGetSubTestDescriptionWithOneLineDocstring '
                '(' + __name__ + '.Test_TextTestResult) (foo=1, bar=2)\n'
                'Tests getDescription() for a method with a docstring.'))

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def testGetDescriptionWithMultiLineDocstring(self):
        """Tests getDescription() for a method with a longer docstring.
        The second line of the docstring.
        """
        result = unittest.TextTestResult(None, True, 1)
        self.assertEqual(
                result.getDescription(self),
               ('testGetDescriptionWithMultiLineDocstring '
                '(' + __name__ + '.Test_TextTestResult)\n'
                'Tests getDescription() for a method with a longer '
                'docstring.'))

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def testGetSubTestDescriptionWithMultiLineDocstring(self):
        """Tests getDescription() for a method with a longer docstring.
        The second line of the docstring.
        """
        result = unittest.TextTestResult(None, True, 1)
        with self.subTest(foo=1, bar=2):
            self.assertEqual(
                result.getDescription(self._subtest),
               ('testGetSubTestDescriptionWithMultiLineDocstring '
                '(' + __name__ + '.Test_TextTestResult) (foo=1, bar=2)\n'
                'Tests getDescription() for a method with a longer '
                'docstring.'))

    class Test(unittest.TestCase):
        def testSuccess(self):
            pass
        def testSkip(self):
            self.skipTest('skip')
        def testFail(self):
            self.fail('fail')
        def testError(self):
            raise Exception('error')
        def testSubTestSuccess(self):
            with self.subTest('one', a=1):
                pass
            with self.subTest('two', b=2):
                pass
        def testSubTestMixed(self):
            with self.subTest('success', a=1):
                pass
            with self.subTest('skip', b=2):
                self.skipTest('skip')
            with self.subTest('fail', c=3):
                self.fail('fail')
            with self.subTest('error', d=4):
                raise Exception('error')

        tearDownError = None
        def tearDown(self):
            if self.tearDownError is not None:
                raise self.tearDownError

    def _run_test(self, test_name, verbosity, tearDownError=None):
        stream = io.StringIO()
        stream = unittest.runner._WritelnDecorator(stream)
        result = unittest.TextTestResult(stream, True, verbosity)
        test = self.Test(test_name)
        test.tearDownError = tearDownError
        test.run(result)
        return stream.getvalue()

    def testDotsOutput(self):
        self.assertEqual(self._run_test('testSuccess', 1), '.')
        self.assertEqual(self._run_test('testSkip', 1), 's')
        self.assertEqual(self._run_test('testFail', 1), 'F')
        self.assertEqual(self._run_test('testError', 1), 'E')

    def testLongOutput(self):
        classname = f'{__name__}.{self.Test.__qualname__}'
        self.assertEqual(self._run_test('testSuccess', 2),
                         f'testSuccess ({classname}) ... ok\n')
        self.assertEqual(self._run_test('testSkip', 2),
                         f"testSkip ({classname}) ... skipped 'skip'\n")
        self.assertEqual(self._run_test('testFail', 2),
                         f'testFail ({classname}) ... FAIL\n')
        self.assertEqual(self._run_test('testError', 2),
                         f'testError ({classname}) ... ERROR\n')

    def testDotsOutputSubTestSuccess(self):
        self.assertEqual(self._run_test('testSubTestSuccess', 1), '.')

    def testLongOutputSubTestSuccess(self):
        classname = f'{__name__}.{self.Test.__qualname__}'
        self.assertEqual(self._run_test('testSubTestSuccess', 2),
                         f'testSubTestSuccess ({classname}) ... ok\n')

    def testDotsOutputSubTestMixed(self):
        self.assertEqual(self._run_test('testSubTestMixed', 1), 'sFE')

    def testLongOutputSubTestMixed(self):
        classname = f'{__name__}.{self.Test.__qualname__}'
        self.assertEqual(self._run_test('testSubTestMixed', 2),
                f'testSubTestMixed ({classname}) ... \n'
                f"  testSubTestMixed ({classname}) [skip] (b=2) ... skipped 'skip'\n"
                f'  testSubTestMixed ({classname}) [fail] (c=3) ... FAIL\n'
                f'  testSubTestMixed ({classname}) [error] (d=4) ... ERROR\n')

    def testDotsOutputTearDownFail(self):
        out = self._run_test('testSuccess', 1, AssertionError('fail'))
        self.assertEqual(out, 'F')
        out = self._run_test('testError', 1, AssertionError('fail'))
        self.assertEqual(out, 'EF')
        out = self._run_test('testFail', 1, Exception('error'))
        self.assertEqual(out, 'FE')
        out = self._run_test('testSkip', 1, AssertionError('fail'))
        self.assertEqual(out, 'sF')

    def testLongOutputTearDownFail(self):
        classname = f'{__name__}.{self.Test.__qualname__}'
        out = self._run_test('testSuccess', 2, AssertionError('fail'))
        self.assertEqual(out,
                         f'testSuccess ({classname}) ... FAIL\n')
        out = self._run_test('testError', 2, AssertionError('fail'))
        self.assertEqual(out,
                         f'testError ({classname}) ... ERROR\n'
                         f'testError ({classname}) ... FAIL\n')
        out = self._run_test('testFail', 2, Exception('error'))
        self.assertEqual(out,
                         f'testFail ({classname}) ... FAIL\n'
                         f'testFail ({classname}) ... ERROR\n')
        out = self._run_test('testSkip', 2, AssertionError('fail'))
        self.assertEqual(out,
                         f"testSkip ({classname}) ... skipped 'skip'\n"
                         f'testSkip ({classname}) ... FAIL\n')


classDict = dict(unittest.TestResult.__dict__)
for m in ('addSkip', 'addExpectedFailure', 'addUnexpectedSuccess',
           '__init__'):
    del classDict[m]

def __init__(self, stream=None, descriptions=None, verbosity=None):
    self.failures = []
    self.errors = []
    self.testsRun = 0
    self.shouldStop = False
    self.buffer = False
    self.tb_locals = False

classDict['__init__'] = __init__
OldResult = type('OldResult', (object,), classDict)

class Test_OldTestResult(unittest.TestCase):

    def assertOldResultWarning(self, test, failures):
        with warnings_helper.check_warnings(
                ("TestResult has no add.+ method,", RuntimeWarning)):
            result = OldResult()
            test.run(result)
            self.assertEqual(len(result.failures), failures)

    def testOldTestResult(self):
        class Test(unittest.TestCase):
            def testSkip(self):
                self.skipTest('foobar')
            @unittest.expectedFailure
            def testExpectedFail(self):
                raise TypeError
            @unittest.expectedFailure
            def testUnexpectedSuccess(self):
                pass

        for test_name, should_pass in (('testSkip', True),
                                       ('testExpectedFail', True),
                                       ('testUnexpectedSuccess', False)):
            test = Test(test_name)
            self.assertOldResultWarning(test, int(not should_pass))

    def testOldTestTesultSetup(self):
        class Test(unittest.TestCase):
            def setUp(self):
                self.skipTest('no reason')
            def testFoo(self):
                pass
        self.assertOldResultWarning(Test('testFoo'), 0)

    def testOldTestResultClass(self):
        @unittest.skip('no reason')
        class Test(unittest.TestCase):
            def testFoo(self):
                pass
        self.assertOldResultWarning(Test('testFoo'), 0)

    def testOldResultWithRunner(self):
        class Test(unittest.TestCase):
            def testFoo(self):
                pass
        runner = unittest.TextTestRunner(resultclass=OldResult,
                                          stream=io.StringIO())
        # This will raise an exception if TextTestRunner can't handle old
        # test result objects
        runner.run(Test('testFoo'))


class TestOutputBuffering(unittest.TestCase):

    def setUp(self):
        self._real_out = sys.stdout
        self._real_err = sys.stderr

    def tearDown(self):
        sys.stdout = self._real_out
        sys.stderr = self._real_err

    def testBufferOutputOff(self):
        real_out = self._real_out
        real_err = self._real_err

        result = unittest.TestResult()
        self.assertFalse(result.buffer)

        self.assertIs(real_out, sys.stdout)
        self.assertIs(real_err, sys.stderr)

        result.startTest(self)

        self.assertIs(real_out, sys.stdout)
        self.assertIs(real_err, sys.stderr)

    def testBufferOutputStartTestAddSuccess(self):
        real_out = self._real_out
        real_err = self._real_err

        result = unittest.TestResult()
        self.assertFalse(result.buffer)

        result.buffer = True

        self.assertIs(real_out, sys.stdout)
        self.assertIs(real_err, sys.stderr)

        result.startTest(self)

        self.assertIsNot(real_out, sys.stdout)
        self.assertIsNot(real_err, sys.stderr)
        self.assertIsInstance(sys.stdout, io.StringIO)
        self.assertIsInstance(sys.stderr, io.StringIO)
        self.assertIsNot(sys.stdout, sys.stderr)

        out_stream = sys.stdout
        err_stream = sys.stderr

        result._original_stdout = io.StringIO()
        result._original_stderr = io.StringIO()

        print('foo')
        print('bar', file=sys.stderr)

        self.assertEqual(out_stream.getvalue(), 'foo\n')
        self.assertEqual(err_stream.getvalue(), 'bar\n')

        self.assertEqual(result._original_stdout.getvalue(), '')
        self.assertEqual(result._original_stderr.getvalue(), '')

        result.addSuccess(self)
        result.stopTest(self)

        self.assertIs(sys.stdout, result._original_stdout)
        self.assertIs(sys.stderr, result._original_stderr)

        self.assertEqual(result._original_stdout.getvalue(), '')
        self.assertEqual(result._original_stderr.getvalue(), '')

        self.assertEqual(out_stream.getvalue(), '')
        self.assertEqual(err_stream.getvalue(), '')


    def getStartedResult(self):
        result = unittest.TestResult()
        result.buffer = True
        result.startTest(self)
        return result

    def testBufferOutputAddErrorOrFailure(self):
        unittest.result.traceback = MockTraceback
        self.addCleanup(restore_traceback)

        for message_attr, add_attr, include_error in [
            ('errors', 'addError', True),
            ('failures', 'addFailure', False),
            ('errors', 'addError', True),
            ('failures', 'addFailure', False)
        ]:
            result = self.getStartedResult()
            buffered_out = sys.stdout
            buffered_err = sys.stderr
            result._original_stdout = io.StringIO()
            result._original_stderr = io.StringIO()

            print('foo', file=sys.stdout)
            if include_error:
                print('bar', file=sys.stderr)


            addFunction = getattr(result, add_attr)
            addFunction(self, (None, None, None))
            result.stopTest(self)

            result_list = getattr(result, message_attr)
            self.assertEqual(len(result_list), 1)

            test, message = result_list[0]
            expectedOutMessage = textwrap.dedent("""
                Stdout:
                foo
            """)
            expectedErrMessage = ''
            if include_error:
                expectedErrMessage = textwrap.dedent("""
                Stderr:
                bar
            """)

            expectedFullMessage = 'A traceback%s%s' % (expectedOutMessage, expectedErrMessage)

            self.assertIs(test, self)
            self.assertEqual(result._original_stdout.getvalue(), expectedOutMessage)
            self.assertEqual(result._original_stderr.getvalue(), expectedErrMessage)
            self.assertMultiLineEqual(message, expectedFullMessage)

    def testBufferSetUp(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            def setUp(self):
                print('set up')
                1/0
            def test_foo(self):
                pass
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\nset up\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 1)
        description = f'test_foo ({strclass(Foo)})'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(str(test_case), description)
        self.assertIn('ZeroDivisionError: division by zero', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferTearDown(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            def tearDown(self):
                print('tear down')
                1/0
            def test_foo(self):
                pass
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\ntear down\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 1)
        description = f'test_foo ({strclass(Foo)})'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(str(test_case), description)
        self.assertIn('ZeroDivisionError: division by zero', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferDoCleanups(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            def setUp(self):
                print('set up')
                self.addCleanup(bad_cleanup1)
                self.addCleanup(bad_cleanup2)
            def test_foo(self):
                pass
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\nset up\ndo cleanup2\ndo cleanup1\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 2)
        description = f'test_foo ({strclass(Foo)})'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(str(test_case), description)
        self.assertIn('ValueError: bad cleanup2', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn('\nStdout:\nset up\ndo cleanup2\n', formatted_exc)
        self.assertNotIn('\ndo cleanup1\n', formatted_exc)
        test_case, formatted_exc = result.errors[1]
        self.assertEqual(str(test_case), description)
        self.assertIn('TypeError: bad cleanup1', formatted_exc)
        self.assertNotIn('ValueError', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferSetUp_DoCleanups(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            def setUp(self):
                print('set up')
                self.addCleanup(bad_cleanup1)
                self.addCleanup(bad_cleanup2)
                1/0
            def test_foo(self):
                pass
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\nset up\ndo cleanup2\ndo cleanup1\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 3)
        description = f'test_foo ({strclass(Foo)})'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(str(test_case), description)
        self.assertIn('ZeroDivisionError: division by zero', formatted_exc)
        self.assertNotIn('ValueError', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn('\nStdout:\nset up\n', formatted_exc)
        self.assertNotIn('\ndo cleanup2\n', formatted_exc)
        self.assertNotIn('\ndo cleanup1\n', formatted_exc)
        test_case, formatted_exc = result.errors[1]
        self.assertEqual(str(test_case), description)
        self.assertIn('ValueError: bad cleanup2', formatted_exc)
        self.assertNotIn('ZeroDivisionError', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn('\nStdout:\nset up\ndo cleanup2\n', formatted_exc)
        self.assertNotIn('\ndo cleanup1\n', formatted_exc)
        test_case, formatted_exc = result.errors[2]
        self.assertEqual(str(test_case), description)
        self.assertIn('TypeError: bad cleanup1', formatted_exc)
        self.assertNotIn('ZeroDivisionError', formatted_exc)
        self.assertNotIn('ValueError', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferTearDown_DoCleanups(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            def setUp(self):
                print('set up')
                self.addCleanup(bad_cleanup1)
                self.addCleanup(bad_cleanup2)
            def tearDown(self):
                print('tear down')
                1/0
            def test_foo(self):
                pass
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\nset up\ntear down\ndo cleanup2\ndo cleanup1\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 3)
        description = f'test_foo ({strclass(Foo)})'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(str(test_case), description)
        self.assertIn('ZeroDivisionError: division by zero', formatted_exc)
        self.assertNotIn('ValueError', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn('\nStdout:\nset up\ntear down\n', formatted_exc)
        self.assertNotIn('\ndo cleanup2\n', formatted_exc)
        self.assertNotIn('\ndo cleanup1\n', formatted_exc)
        test_case, formatted_exc = result.errors[1]
        self.assertEqual(str(test_case), description)
        self.assertIn('ValueError: bad cleanup2', formatted_exc)
        self.assertNotIn('ZeroDivisionError', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn('\nStdout:\nset up\ntear down\ndo cleanup2\n', formatted_exc)
        self.assertNotIn('\ndo cleanup1\n', formatted_exc)
        test_case, formatted_exc = result.errors[2]
        self.assertEqual(str(test_case), description)
        self.assertIn('TypeError: bad cleanup1', formatted_exc)
        self.assertNotIn('ZeroDivisionError', formatted_exc)
        self.assertNotIn('ValueError', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferSetupClass(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                print('set up class')
                1/0
            def test_foo(self):
                pass
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\nset up class\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 1)
        description = f'setUpClass ({strclass(Foo)})'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(test_case.description, description)
        self.assertIn('ZeroDivisionError: division by zero', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferTearDownClass(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            @classmethod
            def tearDownClass(cls):
                print('tear down class')
                1/0
            def test_foo(self):
                pass
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\ntear down class\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 1)
        description = f'tearDownClass ({strclass(Foo)})'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(test_case.description, description)
        self.assertIn('ZeroDivisionError: division by zero', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferDoClassCleanups(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                print('set up class')
                cls.addClassCleanup(bad_cleanup1)
                cls.addClassCleanup(bad_cleanup2)
            @classmethod
            def tearDownClass(cls):
                print('tear down class')
            def test_foo(self):
                pass
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\ntear down class\ndo cleanup2\ndo cleanup1\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 2)
        description = f'tearDownClass ({strclass(Foo)})'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(test_case.description, description)
        self.assertIn('ValueError: bad cleanup2', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn(expected_out, formatted_exc)
        test_case, formatted_exc = result.errors[1]
        self.assertEqual(test_case.description, description)
        self.assertIn('TypeError: bad cleanup1', formatted_exc)
        self.assertNotIn('ValueError', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferSetupClass_DoClassCleanups(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                print('set up class')
                cls.addClassCleanup(bad_cleanup1)
                cls.addClassCleanup(bad_cleanup2)
                1/0
            def test_foo(self):
                pass
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\nset up class\ndo cleanup2\ndo cleanup1\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 3)
        description = f'setUpClass ({strclass(Foo)})'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(test_case.description, description)
        self.assertIn('ZeroDivisionError: division by zero', formatted_exc)
        self.assertNotIn('ValueError', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn('\nStdout:\nset up class\n', formatted_exc)
        test_case, formatted_exc = result.errors[1]
        self.assertEqual(test_case.description, description)
        self.assertIn('ValueError: bad cleanup2', formatted_exc)
        self.assertNotIn('ZeroDivisionError', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn(expected_out, formatted_exc)
        test_case, formatted_exc = result.errors[2]
        self.assertEqual(test_case.description, description)
        self.assertIn('TypeError: bad cleanup1', formatted_exc)
        self.assertNotIn('ZeroDivisionError', formatted_exc)
        self.assertNotIn('ValueError', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferTearDownClass_DoClassCleanups(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            @classmethod
            def setUpClass(cls):
                print('set up class')
                cls.addClassCleanup(bad_cleanup1)
                cls.addClassCleanup(bad_cleanup2)
            @classmethod
            def tearDownClass(cls):
                print('tear down class')
                1/0
            def test_foo(self):
                pass
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\ntear down class\ndo cleanup2\ndo cleanup1\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 3)
        description = f'tearDownClass ({strclass(Foo)})'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(test_case.description, description)
        self.assertIn('ZeroDivisionError: division by zero', formatted_exc)
        self.assertNotIn('ValueError', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn('\nStdout:\ntear down class\n', formatted_exc)
        test_case, formatted_exc = result.errors[1]
        self.assertEqual(test_case.description, description)
        self.assertIn('ValueError: bad cleanup2', formatted_exc)
        self.assertNotIn('ZeroDivisionError', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn(expected_out, formatted_exc)
        test_case, formatted_exc = result.errors[2]
        self.assertEqual(test_case.description, description)
        self.assertIn('TypeError: bad cleanup1', formatted_exc)
        self.assertNotIn('ZeroDivisionError', formatted_exc)
        self.assertNotIn('ValueError', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferSetUpModule(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            def test_foo(self):
                pass
        class Module(object):
            @staticmethod
            def setUpModule():
                print('set up module')
                1/0

        Foo.__module__ = 'Module'
        sys.modules['Module'] = Module
        self.addCleanup(sys.modules.pop, 'Module')
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\nset up module\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 1)
        description = 'setUpModule (Module)'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(test_case.description, description)
        self.assertIn('ZeroDivisionError: division by zero', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferTearDownModule(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            def test_foo(self):
                pass
        class Module(object):
            @staticmethod
            def tearDownModule():
                print('tear down module')
                1/0

        Foo.__module__ = 'Module'
        sys.modules['Module'] = Module
        self.addCleanup(sys.modules.pop, 'Module')
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\ntear down module\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 1)
        description = 'tearDownModule (Module)'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(test_case.description, description)
        self.assertIn('ZeroDivisionError: division by zero', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferDoModuleCleanups(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            def test_foo(self):
                pass
        class Module(object):
            @staticmethod
            def setUpModule():
                print('set up module')
                unittest.addModuleCleanup(bad_cleanup1)
                unittest.addModuleCleanup(bad_cleanup2)

        Foo.__module__ = 'Module'
        sys.modules['Module'] = Module
        self.addCleanup(sys.modules.pop, 'Module')
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\ndo cleanup2\ndo cleanup1\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 1)
        description = 'tearDownModule (Module)'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(test_case.description, description)
        self.assertIn('ValueError: bad cleanup2', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferSetUpModule_DoModuleCleanups(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            def test_foo(self):
                pass
        class Module(object):
            @staticmethod
            def setUpModule():
                print('set up module')
                unittest.addModuleCleanup(bad_cleanup1)
                unittest.addModuleCleanup(bad_cleanup2)
                1/0

        Foo.__module__ = 'Module'
        sys.modules['Module'] = Module
        self.addCleanup(sys.modules.pop, 'Module')
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\nset up module\ndo cleanup2\ndo cleanup1\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 2)
        description = 'setUpModule (Module)'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(test_case.description, description)
        self.assertIn('ZeroDivisionError: division by zero', formatted_exc)
        self.assertNotIn('ValueError', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn('\nStdout:\nset up module\n', formatted_exc)
        test_case, formatted_exc = result.errors[1]
        self.assertIn(expected_out, formatted_exc)
        self.assertEqual(test_case.description, description)
        self.assertIn('ValueError: bad cleanup2', formatted_exc)
        self.assertNotIn('ZeroDivisionError', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn(expected_out, formatted_exc)

    def testBufferTearDownModule_DoModuleCleanups(self):
        with captured_stdout() as stdout:
            result = unittest.TestResult()
        result.buffer = True

        class Foo(unittest.TestCase):
            def test_foo(self):
                pass
        class Module(object):
            @staticmethod
            def setUpModule():
                print('set up module')
                unittest.addModuleCleanup(bad_cleanup1)
                unittest.addModuleCleanup(bad_cleanup2)
            @staticmethod
            def tearDownModule():
                print('tear down module')
                1/0

        Foo.__module__ = 'Module'
        sys.modules['Module'] = Module
        self.addCleanup(sys.modules.pop, 'Module')
        suite = unittest.TestSuite([Foo('test_foo')])
        suite(result)
        expected_out = '\nStdout:\ntear down module\ndo cleanup2\ndo cleanup1\n'
        self.assertEqual(stdout.getvalue(), expected_out)
        self.assertEqual(len(result.errors), 2)
        description = 'tearDownModule (Module)'
        test_case, formatted_exc = result.errors[0]
        self.assertEqual(test_case.description, description)
        self.assertIn('ZeroDivisionError: division by zero', formatted_exc)
        self.assertNotIn('ValueError', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn('\nStdout:\ntear down module\n', formatted_exc)
        test_case, formatted_exc = result.errors[1]
        self.assertEqual(test_case.description, description)
        self.assertIn('ValueError: bad cleanup2', formatted_exc)
        self.assertNotIn('ZeroDivisionError', formatted_exc)
        self.assertNotIn('TypeError', formatted_exc)
        self.assertIn(expected_out, formatted_exc)


if __name__ == '__main__':
    unittest.main()
