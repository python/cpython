import contextlib
import difflib
import pprint
import pickle
import re
import sys
import logging
import warnings
import weakref
import inspect
import types

from collections import UserString
from copy import deepcopy
from test import support

import unittest

from test.test_unittest.support import (
    TestEquality, TestHashing, LoggingResult, LegacyLoggingResult,
    ResultWithNoStartTestRunStopTestRun
)
from test.support import captured_stderr, gc_collect


log_foo = logging.getLogger('foo')
log_foobar = logging.getLogger('foo.bar')
log_quux = logging.getLogger('quux')


class Test(object):
    "Keep these TestCase classes out of the main namespace"

    class Foo(unittest.TestCase):
        def runTest(self): pass
        def test1(self): pass

    class Bar(Foo):
        def test2(self): pass

    class LoggingTestCase(unittest.TestCase):
        """A test case which logs its calls."""

        def __init__(self, events):
            super(Test.LoggingTestCase, self).__init__('test')
            self.events = events

        def setUp(self):
            self.events.append('setUp')

        def test(self):
            self.events.append('test')

        def tearDown(self):
            self.events.append('tearDown')


class List(list):
    pass


class Test_TestCase(unittest.TestCase, TestEquality, TestHashing):

    ### Set up attributes used by inherited tests
    ################################################################

    # Used by TestHashing.test_hash and TestEquality.test_eq
    eq_pairs = [(Test.Foo('test1'), Test.Foo('test1'))]

    # Used by TestEquality.test_ne
    ne_pairs = [(Test.Foo('test1'), Test.Foo('runTest')),
                (Test.Foo('test1'), Test.Bar('test1')),
                (Test.Foo('test1'), Test.Bar('test2'))]

    ################################################################
    ### /Set up attributes used by inherited tests


    # "class TestCase([methodName])"
    # ...
    # "Each instance of TestCase will run a single test method: the
    # method named methodName."
    # ...
    # "methodName defaults to "runTest"."
    #
    # Make sure it really is optional, and that it defaults to the proper
    # thing.
    def test_init__no_test_name(self):
        class Test(unittest.TestCase):
            def runTest(self): raise MyException()
            def test(self): pass

        self.assertEndsWith(Test().id(), '.Test.runTest')

        # test that TestCase can be instantiated with no args
        # primarily for use at the interactive interpreter
        test = unittest.TestCase()
        test.assertEqual(3, 3)
        with test.assertRaises(test.failureException):
            test.assertEqual(3, 2)

        with self.assertRaises(AttributeError):
            test.run()

    # "class TestCase([methodName])"
    # ...
    # "Each instance of TestCase will run a single test method: the
    # method named methodName."
    def test_init__test_name__valid(self):
        class Test(unittest.TestCase):
            def runTest(self): raise MyException()
            def test(self): pass

        self.assertEndsWith(Test('test').id(), '.Test.test')

    # "class TestCase([methodName])"
    # ...
    # "Each instance of TestCase will run a single test method: the
    # method named methodName."
    def test_init__test_name__invalid(self):
        class Test(unittest.TestCase):
            def runTest(self): raise MyException()
            def test(self): pass

        try:
            Test('testfoo')
        except ValueError:
            pass
        else:
            self.fail("Failed to raise ValueError")

    # "Return the number of tests represented by the this test object. For
    # TestCase instances, this will always be 1"
    def test_countTestCases(self):
        class Foo(unittest.TestCase):
            def test(self): pass

        self.assertEqual(Foo('test').countTestCases(), 1)

    # "Return the default type of test result object to be used to run this
    # test. For TestCase instances, this will always be
    # unittest.TestResult;  subclasses of TestCase should
    # override this as necessary."
    def test_defaultTestResult(self):
        class Foo(unittest.TestCase):
            def runTest(self):
                pass

        result = Foo().defaultTestResult()
        self.assertEqual(type(result), unittest.TestResult)

    # "When a setUp() method is defined, the test runner will run that method
    # prior to each test. Likewise, if a tearDown() method is defined, the
    # test runner will invoke that method after each test. In the example,
    # setUp() was used to create a fresh sequence for each test."
    #
    # Make sure the proper call order is maintained, even if setUp() raises
    # an exception.
    def test_run_call_order__error_in_setUp(self):
        events = []
        result = LoggingResult(events)

        class Foo(Test.LoggingTestCase):
            def setUp(self):
                super(Foo, self).setUp()
                raise RuntimeError('raised by Foo.setUp')

        Foo(events).run(result)
        expected = ['startTest', 'setUp', 'addError', 'stopTest']
        self.assertEqual(events, expected)

    # "With a temporary result stopTestRun is called when setUp errors.
    def test_run_call_order__error_in_setUp_default_result(self):
        events = []

        class Foo(Test.LoggingTestCase):
            def defaultTestResult(self):
                return LoggingResult(self.events)

            def setUp(self):
                super(Foo, self).setUp()
                raise RuntimeError('raised by Foo.setUp')

        Foo(events).run()
        expected = ['startTestRun', 'startTest', 'setUp', 'addError',
                    'stopTest', 'stopTestRun']
        self.assertEqual(events, expected)

    # "When a setUp() method is defined, the test runner will run that method
    # prior to each test. Likewise, if a tearDown() method is defined, the
    # test runner will invoke that method after each test. In the example,
    # setUp() was used to create a fresh sequence for each test."
    #
    # Make sure the proper call order is maintained, even if the test raises
    # an error (as opposed to a failure).
    def test_run_call_order__error_in_test(self):
        events = []
        result = LoggingResult(events)

        class Foo(Test.LoggingTestCase):
            def test(self):
                super(Foo, self).test()
                raise RuntimeError('raised by Foo.test')

        expected = ['startTest', 'setUp', 'test',
                    'addError', 'tearDown', 'stopTest']
        Foo(events).run(result)
        self.assertEqual(events, expected)

    # "With a default result, an error in the test still results in stopTestRun
    # being called."
    def test_run_call_order__error_in_test_default_result(self):
        events = []

        class Foo(Test.LoggingTestCase):
            def defaultTestResult(self):
                return LoggingResult(self.events)

            def test(self):
                super(Foo, self).test()
                raise RuntimeError('raised by Foo.test')

        expected = ['startTestRun', 'startTest', 'setUp', 'test',
                    'addError', 'tearDown', 'stopTest', 'stopTestRun']
        Foo(events).run()
        self.assertEqual(events, expected)

    # "When a setUp() method is defined, the test runner will run that method
    # prior to each test. Likewise, if a tearDown() method is defined, the
    # test runner will invoke that method after each test. In the example,
    # setUp() was used to create a fresh sequence for each test."
    #
    # Make sure the proper call order is maintained, even if the test signals
    # a failure (as opposed to an error).
    def test_run_call_order__failure_in_test(self):
        events = []
        result = LoggingResult(events)

        class Foo(Test.LoggingTestCase):
            def test(self):
                super(Foo, self).test()
                self.fail('raised by Foo.test')

        expected = ['startTest', 'setUp', 'test',
                    'addFailure', 'tearDown', 'stopTest']
        Foo(events).run(result)
        self.assertEqual(events, expected)

    # "When a test fails with a default result stopTestRun is still called."
    def test_run_call_order__failure_in_test_default_result(self):

        class Foo(Test.LoggingTestCase):
            def defaultTestResult(self):
                return LoggingResult(self.events)
            def test(self):
                super(Foo, self).test()
                self.fail('raised by Foo.test')

        expected = ['startTestRun', 'startTest', 'setUp', 'test',
                    'addFailure', 'tearDown', 'stopTest', 'stopTestRun']
        events = []
        Foo(events).run()
        self.assertEqual(events, expected)

    # "When a setUp() method is defined, the test runner will run that method
    # prior to each test. Likewise, if a tearDown() method is defined, the
    # test runner will invoke that method after each test. In the example,
    # setUp() was used to create a fresh sequence for each test."
    #
    # Make sure the proper call order is maintained, even if tearDown() raises
    # an exception.
    def test_run_call_order__error_in_tearDown(self):
        events = []
        result = LoggingResult(events)

        class Foo(Test.LoggingTestCase):
            def tearDown(self):
                super(Foo, self).tearDown()
                raise RuntimeError('raised by Foo.tearDown')

        Foo(events).run(result)
        expected = ['startTest', 'setUp', 'test', 'tearDown', 'addError',
                    'stopTest']
        self.assertEqual(events, expected)

    # "When tearDown errors with a default result stopTestRun is still called."
    def test_run_call_order__error_in_tearDown_default_result(self):

        class Foo(Test.LoggingTestCase):
            def defaultTestResult(self):
                return LoggingResult(self.events)
            def tearDown(self):
                super(Foo, self).tearDown()
                raise RuntimeError('raised by Foo.tearDown')

        events = []
        Foo(events).run()
        expected = ['startTestRun', 'startTest', 'setUp', 'test', 'tearDown',
                    'addError', 'stopTest', 'stopTestRun']
        self.assertEqual(events, expected)

    # "TestCase.run() still works when the defaultTestResult is a TestResult
    # that does not support startTestRun and stopTestRun.
    def test_run_call_order_default_result(self):

        class Foo(unittest.TestCase):
            def defaultTestResult(self):
                return ResultWithNoStartTestRunStopTestRun()
            def test(self):
                pass

        with self.assertWarns(RuntimeWarning):
            Foo('test').run()

    def test_deprecation_of_return_val_from_test(self):
        # Issue 41322 - deprecate return of value that is not None from a test
        class Nothing:
            def __eq__(self, o):
                return o is None
        class Foo(unittest.TestCase):
            def test1(self):
                return 1
            def test2(self):
                yield 1
            def test3(self):
                return Nothing()

        with self.assertWarns(DeprecationWarning) as w:
            Foo('test1').run()
        self.assertIn('It is deprecated to return a value that is not None', str(w.warning))
        self.assertIn('test1', str(w.warning))
        self.assertEqual(w.filename, __file__)
        self.assertIn("returned 'int'", str(w.warning))

        with self.assertWarns(DeprecationWarning) as w:
            Foo('test2').run()
        self.assertIn('It is deprecated to return a value that is not None', str(w.warning))
        self.assertIn('test2', str(w.warning))
        self.assertEqual(w.filename, __file__)
        self.assertIn("returned 'generator'", str(w.warning))

        with self.assertWarns(DeprecationWarning) as w:
            Foo('test3').run()
        self.assertIn('It is deprecated to return a value that is not None', str(w.warning))
        self.assertIn('test3', str(w.warning))
        self.assertEqual(w.filename, __file__)
        self.assertIn(f'returned {Nothing.__name__!r}', str(w.warning))

    def test_deprecation_of_return_val_from_test_async_method(self):
        class Foo(unittest.TestCase):
            async def test1(self):
                return 1

        with self.assertWarns(DeprecationWarning) as w:
            warnings.filterwarnings('ignore',
                    'coroutine .* was never awaited', RuntimeWarning)
            Foo('test1').run()
            support.gc_collect()
        self.assertIn('It is deprecated to return a value that is not None', str(w.warning))
        self.assertIn('test1', str(w.warning))
        self.assertEqual(w.filename, __file__)
        self.assertIn("returned 'coroutine'", str(w.warning))
        self.assertIn(
            'Maybe you forgot to use IsolatedAsyncioTestCase as the base class?',
            str(w.warning),
        )

    def _check_call_order__subtests(self, result, events, expected_events):
        class Foo(Test.LoggingTestCase):
            def test(self):
                super(Foo, self).test()
                for i in [1, 2, 3]:
                    with self.subTest(i=i):
                        if i == 1:
                            self.fail('failure')
                        for j in [2, 3]:
                            with self.subTest(j=j):
                                if i * j == 6:
                                    raise RuntimeError('raised by Foo.test')
                1 / 0

        # Order is the following:
        # i=1 => subtest failure
        # i=2, j=2 => subtest success
        # i=2, j=3 => subtest error
        # i=3, j=2 => subtest error
        # i=3, j=3 => subtest success
        # toplevel => error
        Foo(events).run(result)
        self.assertEqual(events, expected_events)

    def test_run_call_order__subtests(self):
        events = []
        result = LoggingResult(events)
        expected = ['startTest', 'setUp', 'test',
                    'addSubTestFailure', 'addSubTestSuccess',
                    'addSubTestFailure', 'addSubTestFailure',
                    'addSubTestSuccess', 'addError', 'tearDown', 'stopTest']
        self._check_call_order__subtests(result, events, expected)

    def test_run_call_order__subtests_legacy(self):
        # With a legacy result object (without an addSubTest method),
        # text execution stops after the first subtest failure.
        events = []
        result = LegacyLoggingResult(events)
        expected = ['startTest', 'setUp', 'test',
                    'addFailure', 'tearDown', 'stopTest']
        self._check_call_order__subtests(result, events, expected)

    def _check_call_order__subtests_success(self, result, events, expected_events):
        class Foo(Test.LoggingTestCase):
            def test(self):
                super(Foo, self).test()
                for i in [1, 2]:
                    with self.subTest(i=i):
                        for j in [2, 3]:
                            with self.subTest(j=j):
                                pass

        Foo(events).run(result)
        self.assertEqual(events, expected_events)

    def test_run_call_order__subtests_success(self):
        events = []
        result = LoggingResult(events)
        # The 6 subtest successes are individually recorded, in addition
        # to the whole test success.
        expected = (['startTest', 'setUp', 'test']
                    + 6 * ['addSubTestSuccess']
                    + ['tearDown', 'addSuccess', 'stopTest'])
        self._check_call_order__subtests_success(result, events, expected)

    def test_run_call_order__subtests_success_legacy(self):
        # With a legacy result, only the whole test success is recorded.
        events = []
        result = LegacyLoggingResult(events)
        expected = ['startTest', 'setUp', 'test', 'tearDown',
                    'addSuccess', 'stopTest']
        self._check_call_order__subtests_success(result, events, expected)

    def test_run_call_order__subtests_failfast(self):
        events = []
        result = LoggingResult(events)
        result.failfast = True

        class Foo(Test.LoggingTestCase):
            def test(self):
                super(Foo, self).test()
                with self.subTest(i=1):
                    self.fail('failure')
                with self.subTest(i=2):
                    self.fail('failure')
                self.fail('failure')

        expected = ['startTest', 'setUp', 'test',
                    'addSubTestFailure', 'tearDown', 'stopTest']
        Foo(events).run(result)
        self.assertEqual(events, expected)

    def test_subtests_failfast(self):
        # Ensure proper test flow with subtests and failfast (issue #22894)
        events = []

        class Foo(unittest.TestCase):
            def test_a(self):
                with self.subTest():
                    events.append('a1')
                events.append('a2')

            def test_b(self):
                with self.subTest():
                    events.append('b1')
                with self.subTest():
                    self.fail('failure')
                events.append('b2')

            def test_c(self):
                events.append('c')

        result = unittest.TestResult()
        result.failfast = True
        suite = unittest.TestLoader().loadTestsFromTestCase(Foo)
        suite.run(result)

        expected = ['a1', 'a2', 'b1']
        self.assertEqual(events, expected)

    def test_subtests_debug(self):
        # Test debug() with a test that uses subTest() (bpo-34900)
        events = []

        class Foo(unittest.TestCase):
            def test_a(self):
                events.append('test case')
                with self.subTest():
                    events.append('subtest 1')

        Foo('test_a').debug()

        self.assertEqual(events, ['test case', 'subtest 1'])

    # "This class attribute gives the exception raised by the test() method.
    # If a test framework needs to use a specialized exception, possibly to
    # carry additional information, it must subclass this exception in
    # order to ``play fair'' with the framework.  The initial value of this
    # attribute is AssertionError"
    def test_failureException__default(self):
        class Foo(unittest.TestCase):
            def test(self):
                pass

        self.assertIs(Foo('test').failureException, AssertionError)

    # "This class attribute gives the exception raised by the test() method.
    # If a test framework needs to use a specialized exception, possibly to
    # carry additional information, it must subclass this exception in
    # order to ``play fair'' with the framework."
    #
    # Make sure TestCase.run() respects the designated failureException
    def test_failureException__subclassing__explicit_raise(self):
        events = []
        result = LoggingResult(events)

        class Foo(unittest.TestCase):
            def test(self):
                raise RuntimeError()

            failureException = RuntimeError

        self.assertIs(Foo('test').failureException, RuntimeError)


        Foo('test').run(result)
        expected = ['startTest', 'addFailure', 'stopTest']
        self.assertEqual(events, expected)

    # "This class attribute gives the exception raised by the test() method.
    # If a test framework needs to use a specialized exception, possibly to
    # carry additional information, it must subclass this exception in
    # order to ``play fair'' with the framework."
    #
    # Make sure TestCase.run() respects the designated failureException
    def test_failureException__subclassing__implicit_raise(self):
        events = []
        result = LoggingResult(events)

        class Foo(unittest.TestCase):
            def test(self):
                self.fail("foo")

            failureException = RuntimeError

        self.assertIs(Foo('test').failureException, RuntimeError)


        Foo('test').run(result)
        expected = ['startTest', 'addFailure', 'stopTest']
        self.assertEqual(events, expected)

    # "The default implementation does nothing."
    def test_setUp(self):
        class Foo(unittest.TestCase):
            def runTest(self):
                pass

        # ... and nothing should happen
        Foo().setUp()

    # "The default implementation does nothing."
    def test_tearDown(self):
        class Foo(unittest.TestCase):
            def runTest(self):
                pass

        # ... and nothing should happen
        Foo().tearDown()

    # "Return a string identifying the specific test case."
    #
    # Because of the vague nature of the docs, I'm not going to lock this
    # test down too much. Really all that can be asserted is that the id()
    # will be a string (either 8-byte or unicode -- again, because the docs
    # just say "string")
    def test_id(self):
        class Foo(unittest.TestCase):
            def runTest(self):
                pass

        self.assertIsInstance(Foo().id(), str)


    # "If result is omitted or None, a temporary result object is created,
    # used, and is made available to the caller. As TestCase owns the
    # temporary result startTestRun and stopTestRun are called.

    def test_run__uses_defaultTestResult(self):
        events = []
        defaultResult = LoggingResult(events)

        class Foo(unittest.TestCase):
            def test(self):
                events.append('test')

            def defaultTestResult(self):
                return defaultResult

        # Make run() find a result object on its own
        result = Foo('test').run()

        self.assertIs(result, defaultResult)
        expected = ['startTestRun', 'startTest', 'test', 'addSuccess',
            'stopTest', 'stopTestRun']
        self.assertEqual(events, expected)


    # "The result object is returned to run's caller"
    def test_run__returns_given_result(self):

        class Foo(unittest.TestCase):
            def test(self):
                pass

        result = unittest.TestResult()

        retval = Foo('test').run(result)
        self.assertIs(retval, result)


    # "The same effect [as method run] may be had by simply calling the
    # TestCase instance."
    def test_call__invoking_an_instance_delegates_to_run(self):
        resultIn = unittest.TestResult()
        resultOut = unittest.TestResult()

        class Foo(unittest.TestCase):
            def test(self):
                pass

            def run(self, result):
                self.assertIs(result, resultIn)
                return resultOut

        retval = Foo('test')(resultIn)

        self.assertIs(retval, resultOut)


    def testShortDescriptionWithoutDocstring(self):
        self.assertIsNone(self.shortDescription())

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def testShortDescriptionWithOneLineDocstring(self):
        """Tests shortDescription() for a method with a docstring."""
        self.assertEqual(
                self.shortDescription(),
                'Tests shortDescription() for a method with a docstring.')

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def testShortDescriptionWithMultiLineDocstring(self):
        """Tests shortDescription() for a method with a longer docstring.

        This method ensures that only the first line of a docstring is
        returned used in the short description, no matter how long the
        whole thing is.
        """
        self.assertEqual(
                self.shortDescription(),
                 'Tests shortDescription() for a method with a longer '
                 'docstring.')

    @unittest.skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def testShortDescriptionWhitespaceTrimming(self):
        """
            Tests shortDescription() whitespace is trimmed, so that the first
            line of nonwhite-space text becomes the docstring.
        """
        self.assertEqual(
            self.shortDescription(),
            'Tests shortDescription() whitespace is trimmed, so that the first')

    def testAddTypeEqualityFunc(self):
        class SadSnake(object):
            """Dummy class for test_addTypeEqualityFunc."""
        s1, s2 = SadSnake(), SadSnake()
        self.assertFalse(s1 == s2)
        def AllSnakesCreatedEqual(a, b, msg=None):
            return type(a) == type(b) == SadSnake
        self.addTypeEqualityFunc(SadSnake, AllSnakesCreatedEqual)
        self.assertEqual(s1, s2)
        # No this doesn't clean up and remove the SadSnake equality func
        # from this TestCase instance but since it's local nothing else
        # will ever notice that.

    def testAssertIs(self):
        thing = object()
        self.assertIs(thing, thing)
        self.assertRaises(self.failureException, self.assertIs, thing, object())

    def testAssertIsNot(self):
        thing = object()
        self.assertIsNot(thing, object())
        self.assertRaises(self.failureException, self.assertIsNot, thing, thing)

    def testAssertIsInstance(self):
        thing = List()
        self.assertIsInstance(thing, list)
        self.assertIsInstance(thing, (int, list))
        with self.assertRaises(self.failureException) as cm:
            self.assertIsInstance(thing, int)
        self.assertEqual(str(cm.exception),
                "[] is not an instance of <class 'int'>")
        with self.assertRaises(self.failureException) as cm:
            self.assertIsInstance(thing, (int, float))
        self.assertEqual(str(cm.exception),
                "[] is not an instance of any of (<class 'int'>, <class 'float'>)")

        with self.assertRaises(self.failureException) as cm:
            self.assertIsInstance(thing, int, 'ababahalamaha')
        self.assertIn('ababahalamaha', str(cm.exception))
        with self.assertRaises(self.failureException) as cm:
            self.assertIsInstance(thing, int, msg='ababahalamaha')
        self.assertIn('ababahalamaha', str(cm.exception))

    def testAssertNotIsInstance(self):
        thing = List()
        self.assertNotIsInstance(thing, int)
        self.assertNotIsInstance(thing, (int, float))
        with self.assertRaises(self.failureException) as cm:
            self.assertNotIsInstance(thing, list)
        self.assertEqual(str(cm.exception),
                "[] is an instance of <class 'list'>")
        with self.assertRaises(self.failureException) as cm:
            self.assertNotIsInstance(thing, (int, list))
        self.assertEqual(str(cm.exception),
                "[] is an instance of <class 'list'>")

        with self.assertRaises(self.failureException) as cm:
            self.assertNotIsInstance(thing, list, 'ababahalamaha')
        self.assertIn('ababahalamaha', str(cm.exception))
        with self.assertRaises(self.failureException) as cm:
            self.assertNotIsInstance(thing, list, msg='ababahalamaha')
        self.assertIn('ababahalamaha', str(cm.exception))

    def testAssertIsSubclass(self):
        self.assertIsSubclass(List, list)
        self.assertIsSubclass(List, (int, list))
        with self.assertRaises(self.failureException) as cm:
            self.assertIsSubclass(List, int)
        self.assertEqual(str(cm.exception),
                f"{List!r} is not a subclass of <class 'int'>")
        with self.assertRaises(self.failureException) as cm:
            self.assertIsSubclass(List, (int, float))
        self.assertEqual(str(cm.exception),
                f"{List!r} is not a subclass of any of (<class 'int'>, <class 'float'>)")
        with self.assertRaises(self.failureException) as cm:
            self.assertIsSubclass(1, int)
        self.assertEqual(str(cm.exception), "1 is not a class")

        with self.assertRaises(self.failureException) as cm:
            self.assertIsSubclass(List, int, 'ababahalamaha')
        self.assertIn('ababahalamaha', str(cm.exception))
        with self.assertRaises(self.failureException) as cm:
            self.assertIsSubclass(List, int, msg='ababahalamaha')
        self.assertIn('ababahalamaha', str(cm.exception))

    def testAssertNotIsSubclass(self):
        self.assertNotIsSubclass(List, int)
        self.assertNotIsSubclass(List, (int, float))
        with self.assertRaises(self.failureException) as cm:
            self.assertNotIsSubclass(List, list)
        self.assertEqual(str(cm.exception),
                f"{List!r} is a subclass of <class 'list'>")
        with self.assertRaises(self.failureException) as cm:
            self.assertNotIsSubclass(List, (int, list))
        self.assertEqual(str(cm.exception),
                f"{List!r} is a subclass of <class 'list'>")
        with self.assertRaises(self.failureException) as cm:
            self.assertNotIsSubclass(1, int)
        self.assertEqual(str(cm.exception), "1 is not a class")

        with self.assertRaises(self.failureException) as cm:
            self.assertNotIsSubclass(List, list, 'ababahalamaha')
        self.assertIn('ababahalamaha', str(cm.exception))
        with self.assertRaises(self.failureException) as cm:
            self.assertNotIsSubclass(List, list, msg='ababahalamaha')
        self.assertIn('ababahalamaha', str(cm.exception))

    def testAssertHasAttr(self):
        a = List()
        a.x = 1
        self.assertHasAttr(a, 'x')
        with self.assertRaises(self.failureException) as cm:
            self.assertHasAttr(a, 'y')
        self.assertEqual(str(cm.exception),
                "'List' object has no attribute 'y'")
        with self.assertRaises(self.failureException) as cm:
            self.assertHasAttr(List, 'spam')
        self.assertEqual(str(cm.exception),
                "type object 'List' has no attribute 'spam'")
        with self.assertRaises(self.failureException) as cm:
            self.assertHasAttr(sys, 'nonexistent')
        self.assertEqual(str(cm.exception),
                "module 'sys' has no attribute 'nonexistent'")

        with self.assertRaises(self.failureException) as cm:
            self.assertHasAttr(a, 'y', 'ababahalamaha')
        self.assertIn('ababahalamaha', str(cm.exception))
        with self.assertRaises(self.failureException) as cm:
            self.assertHasAttr(a, 'y', msg='ababahalamaha')
        self.assertIn('ababahalamaha', str(cm.exception))

    def testAssertNotHasAttr(self):
        a = List()
        a.x = 1
        self.assertNotHasAttr(a, 'y')
        with self.assertRaises(self.failureException) as cm:
            self.assertNotHasAttr(a, 'x')
        self.assertEqual(str(cm.exception),
                "'List' object has unexpected attribute 'x'")
        with self.assertRaises(self.failureException) as cm:
            self.assertNotHasAttr(List, 'append')
        self.assertEqual(str(cm.exception),
                "type object 'List' has unexpected attribute 'append'")
        with self.assertRaises(self.failureException) as cm:
            self.assertNotHasAttr(sys, 'modules')
        self.assertEqual(str(cm.exception),
                "module 'sys' has unexpected attribute 'modules'")

        with self.assertRaises(self.failureException) as cm:
            self.assertNotHasAttr(a, 'x', 'ababahalamaha')
        self.assertIn('ababahalamaha', str(cm.exception))
        with self.assertRaises(self.failureException) as cm:
            self.assertNotHasAttr(a, 'x', msg='ababahalamaha')
        self.assertIn('ababahalamaha', str(cm.exception))

    def testAssertIn(self):
        animals = {'monkey': 'banana', 'cow': 'grass', 'seal': 'fish'}

        self.assertIn('a', 'abc')
        self.assertIn(2, [1, 2, 3])
        self.assertIn('monkey', animals)

        self.assertNotIn('d', 'abc')
        self.assertNotIn(0, [1, 2, 3])
        self.assertNotIn('otter', animals)

        self.assertRaises(self.failureException, self.assertIn, 'x', 'abc')
        self.assertRaises(self.failureException, self.assertIn, 4, [1, 2, 3])
        self.assertRaises(self.failureException, self.assertIn, 'elephant',
                          animals)

        self.assertRaises(self.failureException, self.assertNotIn, 'c', 'abc')
        self.assertRaises(self.failureException, self.assertNotIn, 1, [1, 2, 3])
        self.assertRaises(self.failureException, self.assertNotIn, 'cow',
                          animals)

    def testAssertEqual(self):
        equal_pairs = [
                ((), ()),
                ({}, {}),
                ([], []),
                (set(), set()),
                (frozenset(), frozenset())]
        for a, b in equal_pairs:
            # This mess of try excepts is to test the assertEqual behavior
            # itself.
            try:
                self.assertEqual(a, b)
            except self.failureException:
                self.fail('assertEqual(%r, %r) failed' % (a, b))
            try:
                self.assertEqual(a, b, msg='foo')
            except self.failureException:
                self.fail('assertEqual(%r, %r) with msg= failed' % (a, b))
            try:
                self.assertEqual(a, b, 'foo')
            except self.failureException:
                self.fail('assertEqual(%r, %r) with third parameter failed' %
                          (a, b))

        unequal_pairs = [
               ((), []),
               ({}, set()),
               (set([4,1]), frozenset([4,2])),
               (frozenset([4,5]), set([2,3])),
               (set([3,4]), set([5,4]))]
        for a, b in unequal_pairs:
            self.assertRaises(self.failureException, self.assertEqual, a, b)
            self.assertRaises(self.failureException, self.assertEqual, a, b,
                              'foo')
            self.assertRaises(self.failureException, self.assertEqual, a, b,
                              msg='foo')

    def testEquality(self):
        self.assertListEqual([], [])
        self.assertTupleEqual((), ())
        self.assertSequenceEqual([], ())

        a = [0, 'a', []]
        b = []
        self.assertRaises(unittest.TestCase.failureException,
                          self.assertListEqual, a, b)
        self.assertRaises(unittest.TestCase.failureException,
                          self.assertListEqual, tuple(a), tuple(b))
        self.assertRaises(unittest.TestCase.failureException,
                          self.assertSequenceEqual, a, tuple(b))

        b.extend(a)
        self.assertListEqual(a, b)
        self.assertTupleEqual(tuple(a), tuple(b))
        self.assertSequenceEqual(a, tuple(b))
        self.assertSequenceEqual(tuple(a), b)

        self.assertRaises(self.failureException, self.assertListEqual,
                          a, tuple(b))
        self.assertRaises(self.failureException, self.assertTupleEqual,
                          tuple(a), b)
        self.assertRaises(self.failureException, self.assertListEqual, None, b)
        self.assertRaises(self.failureException, self.assertTupleEqual, None,
                          tuple(b))
        self.assertRaises(self.failureException, self.assertSequenceEqual,
                          None, tuple(b))
        self.assertRaises(self.failureException, self.assertListEqual, 1, 1)
        self.assertRaises(self.failureException, self.assertTupleEqual, 1, 1)
        self.assertRaises(self.failureException, self.assertSequenceEqual,
                          1, 1)

        self.assertDictEqual({}, {})

        c = { 'x': 1 }
        d = {}
        self.assertRaises(unittest.TestCase.failureException,
                          self.assertDictEqual, c, d)

        d.update(c)
        self.assertDictEqual(c, d)

        d['x'] = 0
        self.assertRaises(unittest.TestCase.failureException,
                          self.assertDictEqual, c, d, 'These are unequal')

        self.assertRaises(self.failureException, self.assertDictEqual, None, d)
        self.assertRaises(self.failureException, self.assertDictEqual, [], d)
        self.assertRaises(self.failureException, self.assertDictEqual, 1, 1)

    def testAssertSequenceEqualMaxDiff(self):
        self.assertEqual(self.maxDiff, 80*8)
        seq1 = 'a' + 'x' * 80**2
        seq2 = 'b' + 'x' * 80**2
        diff = '\n'.join(difflib.ndiff(pprint.pformat(seq1).splitlines(),
                                       pprint.pformat(seq2).splitlines()))
        # the +1 is the leading \n added by assertSequenceEqual
        omitted = unittest.case.DIFF_OMITTED % (len(diff) + 1,)

        self.maxDiff = len(diff)//2
        try:

            self.assertSequenceEqual(seq1, seq2)
        except self.failureException as e:
            msg = e.args[0]
        else:
            self.fail('assertSequenceEqual did not fail.')
        self.assertLess(len(msg), len(diff))
        self.assertIn(omitted, msg)

        self.maxDiff = len(diff) * 2
        try:
            self.assertSequenceEqual(seq1, seq2)
        except self.failureException as e:
            msg = e.args[0]
        else:
            self.fail('assertSequenceEqual did not fail.')
        self.assertGreater(len(msg), len(diff))
        self.assertNotIn(omitted, msg)

        self.maxDiff = None
        try:
            self.assertSequenceEqual(seq1, seq2)
        except self.failureException as e:
            msg = e.args[0]
        else:
            self.fail('assertSequenceEqual did not fail.')
        self.assertGreater(len(msg), len(diff))
        self.assertNotIn(omitted, msg)

    def testTruncateMessage(self):
        self.maxDiff = 1
        message = self._truncateMessage('foo', 'bar')
        omitted = unittest.case.DIFF_OMITTED % len('bar')
        self.assertEqual(message, 'foo' + omitted)

        self.maxDiff = None
        message = self._truncateMessage('foo', 'bar')
        self.assertEqual(message, 'foobar')

        self.maxDiff = 4
        message = self._truncateMessage('foo', 'bar')
        self.assertEqual(message, 'foobar')

    def testAssertDictEqualTruncates(self):
        test = unittest.TestCase('assertEqual')
        def truncate(msg, diff):
            return 'foo'
        test._truncateMessage = truncate
        try:
            test.assertDictEqual({}, {1: 0})
        except self.failureException as e:
            self.assertEqual(str(e), 'foo')
        else:
            self.fail('assertDictEqual did not fail')

    def testAssertMultiLineEqualTruncates(self):
        test = unittest.TestCase('assertEqual')
        def truncate(msg, diff):
            return 'foo'
        test._truncateMessage = truncate
        try:
            test.assertMultiLineEqual('foo', 'bar')
        except self.failureException as e:
            self.assertEqual(str(e), 'foo')
        else:
            self.fail('assertMultiLineEqual did not fail')

    def testAssertEqual_diffThreshold(self):
        # check threshold value
        self.assertEqual(self._diffThreshold, 2**16)
        # disable madDiff to get diff markers
        self.maxDiff = None

        # set a lower threshold value and add a cleanup to restore it
        old_threshold = self._diffThreshold
        self._diffThreshold = 2**5
        self.addCleanup(lambda: setattr(self, '_diffThreshold', old_threshold))

        # under the threshold: diff marker (^) in error message
        s = 'x' * (2**4)
        with self.assertRaises(self.failureException) as cm:
            self.assertEqual(s + 'a', s + 'b')
        self.assertIn('^', str(cm.exception))
        self.assertEqual(s + 'a', s + 'a')

        # over the threshold: diff not used and marker (^) not in error message
        s = 'x' * (2**6)
        # if the path that uses difflib is taken, _truncateMessage will be
        # called -- replace it with explodingTruncation to verify that this
        # doesn't happen
        def explodingTruncation(message, diff):
            raise SystemError('this should not be raised')
        old_truncate = self._truncateMessage
        self._truncateMessage = explodingTruncation
        self.addCleanup(lambda: setattr(self, '_truncateMessage', old_truncate))

        s1, s2 = s + 'a', s + 'b'
        with self.assertRaises(self.failureException) as cm:
            self.assertEqual(s1, s2)
        self.assertNotIn('^', str(cm.exception))
        self.assertEqual(str(cm.exception), '%r != %r' % (s1, s2))
        self.assertEqual(s + 'a', s + 'a')

    def testAssertEqual_shorten(self):
        # set a lower threshold value and add a cleanup to restore it
        old_threshold = self._diffThreshold
        self._diffThreshold = 0
        self.addCleanup(lambda: setattr(self, '_diffThreshold', old_threshold))

        s = 'x' * 100
        s1, s2 = s + 'a', s + 'b'
        with self.assertRaises(self.failureException) as cm:
            self.assertEqual(s1, s2)
        c = 'xxxx[35 chars]' + 'x' * 61
        self.assertEqual(str(cm.exception), "'%sa' != '%sb'" % (c, c))
        self.assertEqual(s + 'a', s + 'a')

        p = 'y' * 50
        s1, s2 = s + 'a' + p, s + 'b' + p
        with self.assertRaises(self.failureException) as cm:
            self.assertEqual(s1, s2)
        c = 'xxxx[85 chars]xxxxxxxxxxx'
        self.assertEqual(str(cm.exception), "'%sa%s' != '%sb%s'" % (c, p, c, p))

        p = 'y' * 100
        s1, s2 = s + 'a' + p, s + 'b' + p
        with self.assertRaises(self.failureException) as cm:
            self.assertEqual(s1, s2)
        c = 'xxxx[91 chars]xxxxx'
        d = 'y' * 40 + '[56 chars]yyyy'
        self.assertEqual(str(cm.exception), "'%sa%s' != '%sb%s'" % (c, d, c, d))

    def testAssertCountEqual(self):
        a = object()
        self.assertCountEqual([1, 2, 3], [3, 2, 1])
        self.assertCountEqual(['foo', 'bar', 'baz'], ['bar', 'baz', 'foo'])
        self.assertCountEqual([a, a, 2, 2, 3], (a, 2, 3, a, 2))
        self.assertCountEqual([1, "2", "a", "a"], ["a", "2", True, "a"])
        self.assertRaises(self.failureException, self.assertCountEqual,
                          [1, 2] + [3] * 100, [1] * 100 + [2, 3])
        self.assertRaises(self.failureException, self.assertCountEqual,
                          [1, "2", "a", "a"], ["a", "2", True, 1])
        self.assertRaises(self.failureException, self.assertCountEqual,
                          [10], [10, 11])
        self.assertRaises(self.failureException, self.assertCountEqual,
                          [10, 11], [10])
        self.assertRaises(self.failureException, self.assertCountEqual,
                          [10, 11, 10], [10, 11])

        # Test that sequences of unhashable objects can be tested for sameness:
        self.assertCountEqual([[1, 2], [3, 4], 0], [False, [3, 4], [1, 2]])
        # Test that iterator of unhashable objects can be tested for sameness:
        self.assertCountEqual(iter([1, 2, [], 3, 4]),
                              iter([1, 2, [], 3, 4]))

        # hashable types, but not orderable
        self.assertRaises(self.failureException, self.assertCountEqual,
                          [], [divmod, 'x', 1, 5j, 2j, frozenset()])
        # comparing dicts
        self.assertCountEqual([{'a': 1}, {'b': 2}], [{'b': 2}, {'a': 1}])
        # comparing heterogeneous non-hashable sequences
        self.assertCountEqual([1, 'x', divmod, []], [divmod, [], 'x', 1])
        self.assertRaises(self.failureException, self.assertCountEqual,
                          [], [divmod, [], 'x', 1, 5j, 2j, set()])
        self.assertRaises(self.failureException, self.assertCountEqual,
                          [[1]], [[2]])

        # Same elements, but not same sequence length
        self.assertRaises(self.failureException, self.assertCountEqual,
                          [1, 1, 2], [2, 1])
        self.assertRaises(self.failureException, self.assertCountEqual,
                          [1, 1, "2", "a", "a"], ["2", "2", True, "a"])
        self.assertRaises(self.failureException, self.assertCountEqual,
                          [1, {'b': 2}, None, True], [{'b': 2}, True, None])

        # Same elements which don't reliably compare, in
        # different order, see issue 10242
        a = [{2,4}, {1,2}]
        b = a[::-1]
        self.assertCountEqual(a, b)

        # test utility functions supporting assertCountEqual()

        diffs = set(unittest.util._count_diff_all_purpose('aaabccd', 'abbbcce'))
        expected = {(3,1,'a'), (1,3,'b'), (1,0,'d'), (0,1,'e')}
        self.assertEqual(diffs, expected)

        diffs = unittest.util._count_diff_all_purpose([[]], [])
        self.assertEqual(diffs, [(1, 0, [])])

        diffs = set(unittest.util._count_diff_hashable('aaabccd', 'abbbcce'))
        expected = {(3,1,'a'), (1,3,'b'), (1,0,'d'), (0,1,'e')}
        self.assertEqual(diffs, expected)

    def testAssertSetEqual(self):
        set1 = set()
        set2 = set()
        self.assertSetEqual(set1, set2)

        self.assertRaises(self.failureException, self.assertSetEqual, None, set2)
        self.assertRaises(self.failureException, self.assertSetEqual, [], set2)
        self.assertRaises(self.failureException, self.assertSetEqual, set1, None)
        self.assertRaises(self.failureException, self.assertSetEqual, set1, [])

        set1 = set(['a'])
        set2 = set()
        self.assertRaises(self.failureException, self.assertSetEqual, set1, set2)

        set1 = set(['a'])
        set2 = set(['a'])
        self.assertSetEqual(set1, set2)

        set1 = set(['a'])
        set2 = set(['a', 'b'])
        self.assertRaises(self.failureException, self.assertSetEqual, set1, set2)

        set1 = set(['a'])
        set2 = frozenset(['a', 'b'])
        self.assertRaises(self.failureException, self.assertSetEqual, set1, set2)

        set1 = set(['a', 'b'])
        set2 = frozenset(['a', 'b'])
        self.assertSetEqual(set1, set2)

        set1 = set()
        set2 = "foo"
        self.assertRaises(self.failureException, self.assertSetEqual, set1, set2)
        self.assertRaises(self.failureException, self.assertSetEqual, set2, set1)

        # make sure any string formatting is tuple-safe
        set1 = set([(0, 1), (2, 3)])
        set2 = set([(4, 5)])
        self.assertRaises(self.failureException, self.assertSetEqual, set1, set2)

    def testInequality(self):
        # Try ints
        self.assertGreater(2, 1)
        self.assertGreaterEqual(2, 1)
        self.assertGreaterEqual(1, 1)
        self.assertLess(1, 2)
        self.assertLessEqual(1, 2)
        self.assertLessEqual(1, 1)
        self.assertRaises(self.failureException, self.assertGreater, 1, 2)
        self.assertRaises(self.failureException, self.assertGreater, 1, 1)
        self.assertRaises(self.failureException, self.assertGreaterEqual, 1, 2)
        self.assertRaises(self.failureException, self.assertLess, 2, 1)
        self.assertRaises(self.failureException, self.assertLess, 1, 1)
        self.assertRaises(self.failureException, self.assertLessEqual, 2, 1)

        # Try Floats
        self.assertGreater(1.1, 1.0)
        self.assertGreaterEqual(1.1, 1.0)
        self.assertGreaterEqual(1.0, 1.0)
        self.assertLess(1.0, 1.1)
        self.assertLessEqual(1.0, 1.1)
        self.assertLessEqual(1.0, 1.0)
        self.assertRaises(self.failureException, self.assertGreater, 1.0, 1.1)
        self.assertRaises(self.failureException, self.assertGreater, 1.0, 1.0)
        self.assertRaises(self.failureException, self.assertGreaterEqual, 1.0, 1.1)
        self.assertRaises(self.failureException, self.assertLess, 1.1, 1.0)
        self.assertRaises(self.failureException, self.assertLess, 1.0, 1.0)
        self.assertRaises(self.failureException, self.assertLessEqual, 1.1, 1.0)

        # Try Strings
        self.assertGreater('bug', 'ant')
        self.assertGreaterEqual('bug', 'ant')
        self.assertGreaterEqual('ant', 'ant')
        self.assertLess('ant', 'bug')
        self.assertLessEqual('ant', 'bug')
        self.assertLessEqual('ant', 'ant')
        self.assertRaises(self.failureException, self.assertGreater, 'ant', 'bug')
        self.assertRaises(self.failureException, self.assertGreater, 'ant', 'ant')
        self.assertRaises(self.failureException, self.assertGreaterEqual, 'ant', 'bug')
        self.assertRaises(self.failureException, self.assertLess, 'bug', 'ant')
        self.assertRaises(self.failureException, self.assertLess, 'ant', 'ant')
        self.assertRaises(self.failureException, self.assertLessEqual, 'bug', 'ant')

        # Try bytes
        self.assertGreater(b'bug', b'ant')
        self.assertGreaterEqual(b'bug', b'ant')
        self.assertGreaterEqual(b'ant', b'ant')
        self.assertLess(b'ant', b'bug')
        self.assertLessEqual(b'ant', b'bug')
        self.assertLessEqual(b'ant', b'ant')
        self.assertRaises(self.failureException, self.assertGreater, b'ant', b'bug')
        self.assertRaises(self.failureException, self.assertGreater, b'ant', b'ant')
        self.assertRaises(self.failureException, self.assertGreaterEqual, b'ant',
                          b'bug')
        self.assertRaises(self.failureException, self.assertLess, b'bug', b'ant')
        self.assertRaises(self.failureException, self.assertLess, b'ant', b'ant')
        self.assertRaises(self.failureException, self.assertLessEqual, b'bug', b'ant')

    def testAssertMultiLineEqual(self):
        sample_text = """\
http://www.python.org/doc/2.3/lib/module-unittest.html
test case
    A test case is the smallest unit of testing. [...]
"""
        revised_sample_text = """\
http://www.python.org/doc/2.4.1/lib/module-unittest.html
test case
    A test case is the smallest unit of testing. [...] You may provide your
    own implementation that does not subclass from TestCase, of course.
"""
        sample_text_error = """\
- http://www.python.org/doc/2.3/lib/module-unittest.html
?                             ^
+ http://www.python.org/doc/2.4.1/lib/module-unittest.html
?                             ^^^
  test case
-     A test case is the smallest unit of testing. [...]
+     A test case is the smallest unit of testing. [...] You may provide your
?                                                       +++++++++++++++++++++
+     own implementation that does not subclass from TestCase, of course.
"""
        self.maxDiff = None
        try:
            self.assertMultiLineEqual(sample_text, revised_sample_text)
        except self.failureException as e:
            # need to remove the first line of the error message
            error = str(e).split('\n', 1)[1]
            self.assertEqual(sample_text_error, error)
        else:
            self.fail(f'{self.failureException} not raised')

    def testAssertEqualSingleLine(self):
        sample_text = "laden swallows fly slowly"
        revised_sample_text = "unladen swallows fly quickly"
        sample_text_error = """\
- laden swallows fly slowly
?                    ^^^^
+ unladen swallows fly quickly
? ++                   ^^^^^
"""
        try:
            self.assertEqual(sample_text, revised_sample_text)
        except self.failureException as e:
            # need to remove the first line of the error message
            error = str(e).split('\n', 1)[1]
            self.assertEqual(sample_text_error, error)
        else:
            self.fail(f'{self.failureException} not raised')

    def testAssertEqualwithEmptyString(self):
        '''Verify when there is an empty string involved, the diff output
         does not treat the empty string as a single empty line. It should
         instead be handled as a non-line.
        '''
        sample_text = ''
        revised_sample_text = 'unladen swallows fly quickly'
        sample_text_error = '''\
+ unladen swallows fly quickly
'''
        try:
            self.assertEqual(sample_text, revised_sample_text)
        except self.failureException as e:
            # need to remove the first line of the error message
            error = str(e).split('\n', 1)[1]
            self.assertEqual(sample_text_error, error)
        else:
            self.fail(f'{self.failureException} not raised')

    def testAssertEqualMultipleLinesMissingNewlineTerminator(self):
        '''Verifying format of diff output from assertEqual involving strings
         with multiple lines, but missing the terminating newline on both.
        '''
        sample_text = 'laden swallows\nfly sloely'
        revised_sample_text = 'laden swallows\nfly slowly'
        sample_text_error = '''\
  laden swallows
- fly sloely
?        ^
+ fly slowly
?        ^
'''
        try:
            self.assertEqual(sample_text, revised_sample_text)
        except self.failureException as e:
            # need to remove the first line of the error message
            error = str(e).split('\n', 1)[1]
            self.assertEqual(sample_text_error, error)
        else:
            self.fail(f'{self.failureException} not raised')

    def testAssertEqualMultipleLinesMismatchedNewlinesTerminators(self):
        '''Verifying format of diff output from assertEqual involving strings
         with multiple lines and mismatched newlines. The output should
         include a - on it's own line to indicate the newline difference
         between the two strings
        '''
        sample_text = 'laden swallows\nfly sloely\n'
        revised_sample_text = 'laden swallows\nfly slowly'
        sample_text_error = '''\
  laden swallows
- fly sloely
?        ^
+ fly slowly
?        ^
-\x20
'''
        try:
            self.assertEqual(sample_text, revised_sample_text)
        except self.failureException as e:
            # need to remove the first line of the error message
            error = str(e).split('\n', 1)[1]
            self.assertEqual(sample_text_error, error)
        else:
            self.fail(f'{self.failureException} not raised')

    def testEqualityBytesWarning(self):
        if sys.flags.bytes_warning:
            def bytes_warning():
                return self.assertWarnsRegex(BytesWarning,
                            'Comparison between bytes and string')
        else:
            def bytes_warning():
                return contextlib.ExitStack()

        with bytes_warning(), self.assertRaises(self.failureException):
            self.assertEqual('a', b'a')
        with bytes_warning():
            self.assertNotEqual('a', b'a')

        a = [0, 'a']
        b = [0, b'a']
        with bytes_warning(), self.assertRaises(self.failureException):
            self.assertListEqual(a, b)
        with bytes_warning(), self.assertRaises(self.failureException):
            self.assertTupleEqual(tuple(a), tuple(b))
        with bytes_warning(), self.assertRaises(self.failureException):
            self.assertSequenceEqual(a, tuple(b))
        with bytes_warning(), self.assertRaises(self.failureException):
            self.assertSequenceEqual(tuple(a), b)
        with bytes_warning(), self.assertRaises(self.failureException):
            self.assertSequenceEqual('a', b'a')
        with bytes_warning(), self.assertRaises(self.failureException):
            self.assertSetEqual(set(a), set(b))

        with self.assertRaises(self.failureException):
            self.assertListEqual(a, tuple(b))
        with self.assertRaises(self.failureException):
            self.assertTupleEqual(tuple(a), b)

        a = [0, b'a']
        b = [0]
        with self.assertRaises(self.failureException):
            self.assertListEqual(a, b)
        with self.assertRaises(self.failureException):
            self.assertTupleEqual(tuple(a), tuple(b))
        with self.assertRaises(self.failureException):
            self.assertSequenceEqual(a, tuple(b))
        with self.assertRaises(self.failureException):
            self.assertSequenceEqual(tuple(a), b)
        with self.assertRaises(self.failureException):
            self.assertSetEqual(set(a), set(b))

        a = [0]
        b = [0, b'a']
        with self.assertRaises(self.failureException):
            self.assertListEqual(a, b)
        with self.assertRaises(self.failureException):
            self.assertTupleEqual(tuple(a), tuple(b))
        with self.assertRaises(self.failureException):
            self.assertSequenceEqual(a, tuple(b))
        with self.assertRaises(self.failureException):
            self.assertSequenceEqual(tuple(a), b)
        with self.assertRaises(self.failureException):
            self.assertSetEqual(set(a), set(b))

        with bytes_warning(), self.assertRaises(self.failureException):
            self.assertDictEqual({'a': 0}, {b'a': 0})
        with self.assertRaises(self.failureException):
            self.assertDictEqual({}, {b'a': 0})
        with self.assertRaises(self.failureException):
            self.assertDictEqual({b'a': 0}, {})

        with self.assertRaises(self.failureException):
            self.assertCountEqual([b'a', b'a'], [b'a', b'a', b'a'])
        with bytes_warning():
            self.assertCountEqual(['a', b'a'], ['a', b'a'])
        with bytes_warning(), self.assertRaises(self.failureException):
            self.assertCountEqual(['a', 'a'], [b'a', b'a'])
        with bytes_warning(), self.assertRaises(self.failureException):
            self.assertCountEqual(['a', 'a', []], [b'a', b'a', []])

    def testAssertIsNone(self):
        self.assertIsNone(None)
        self.assertRaises(self.failureException, self.assertIsNone, False)
        self.assertIsNotNone('DjZoPloGears on Rails')
        self.assertRaises(self.failureException, self.assertIsNotNone, None)

    def testAssertRegex(self):
        self.assertRegex('asdfabasdf', r'ab+')
        self.assertRaises(self.failureException, self.assertRegex,
                          'saaas', r'aaaa')

    def testAssertRaisesCallable(self):
        class ExceptionMock(Exception):
            pass
        def Stub():
            raise ExceptionMock('We expect')
        self.assertRaises(ExceptionMock, Stub)
        # A tuple of exception classes is accepted
        self.assertRaises((ValueError, ExceptionMock), Stub)
        # *args and **kwargs also work
        self.assertRaises(ValueError, int, '19', base=8)
        # Failure when no exception is raised
        with self.assertRaises(self.failureException):
            self.assertRaises(ExceptionMock, lambda: 0)
        # Failure when the function is None
        with self.assertRaises(TypeError):
            self.assertRaises(ExceptionMock, None)
        # Failure when another exception is raised
        with self.assertRaises(ExceptionMock):
            self.assertRaises(ValueError, Stub)

    def testAssertRaisesContext(self):
        class ExceptionMock(Exception):
            pass
        def Stub():
            raise ExceptionMock('We expect')
        with self.assertRaises(ExceptionMock):
            Stub()
        # A tuple of exception classes is accepted
        with self.assertRaises((ValueError, ExceptionMock)) as cm:
            Stub()
        # The context manager exposes caught exception
        self.assertIsInstance(cm.exception, ExceptionMock)
        self.assertEqual(cm.exception.args[0], 'We expect')
        # *args and **kwargs also work
        with self.assertRaises(ValueError):
            int('19', base=8)
        # Failure when no exception is raised
        with self.assertRaises(self.failureException):
            with self.assertRaises(ExceptionMock):
                pass
        # Custom message
        with self.assertRaisesRegex(self.failureException, 'foobar'):
            with self.assertRaises(ExceptionMock, msg='foobar'):
                pass
        # Invalid keyword argument
        with self.assertRaisesRegex(TypeError, 'foobar'):
            with self.assertRaises(ExceptionMock, foobar=42):
                pass
        # Failure when another exception is raised
        with self.assertRaises(ExceptionMock):
            self.assertRaises(ValueError, Stub)

    def testAssertRaisesNoExceptionType(self):
        with self.assertRaises(TypeError):
            self.assertRaises()
        with self.assertRaises(TypeError):
            self.assertRaises(1)
        with self.assertRaises(TypeError):
            self.assertRaises(object)
        with self.assertRaises(TypeError):
            self.assertRaises((ValueError, 1))
        with self.assertRaises(TypeError):
            self.assertRaises((ValueError, object))

    def testAssertRaisesRefcount(self):
        # bpo-23890: assertRaises() must not keep objects alive longer
        # than expected
        def func() :
            try:
                raise ValueError
            except ValueError:
                raise ValueError

        refcount = sys.getrefcount(func)
        self.assertRaises(ValueError, func)
        self.assertEqual(refcount, sys.getrefcount(func))

    def testAssertRaisesRegex(self):
        class ExceptionMock(Exception):
            pass

        def Stub():
            raise ExceptionMock('We expect')

        self.assertRaisesRegex(ExceptionMock, re.compile('expect$'), Stub)
        self.assertRaisesRegex(ExceptionMock, 'expect$', Stub)
        with self.assertRaises(TypeError):
            self.assertRaisesRegex(ExceptionMock, 'expect$', None)

    def testAssertNotRaisesRegex(self):
        self.assertRaisesRegex(
                self.failureException, '^Exception not raised by <lambda>$',
                self.assertRaisesRegex, Exception, re.compile('x'),
                lambda: None)
        self.assertRaisesRegex(
                self.failureException, '^Exception not raised by <lambda>$',
                self.assertRaisesRegex, Exception, 'x',
                lambda: None)
        # Custom message
        with self.assertRaisesRegex(self.failureException, 'foobar'):
            with self.assertRaisesRegex(Exception, 'expect', msg='foobar'):
                pass
        # Invalid keyword argument
        with self.assertRaisesRegex(TypeError, 'foobar'):
            with self.assertRaisesRegex(Exception, 'expect', foobar=42):
                pass

    def testAssertRaisesRegexInvalidRegex(self):
        # Issue 20145.
        class MyExc(Exception):
            pass
        self.assertRaises(TypeError, self.assertRaisesRegex, MyExc, lambda: True)

    def testAssertWarnsRegexInvalidRegex(self):
        # Issue 20145.
        class MyWarn(Warning):
            pass
        self.assertRaises(TypeError, self.assertWarnsRegex, MyWarn, lambda: True)

    def testAssertWarnsModifySysModules(self):
        # bpo-29620: handle modified sys.modules during iteration
        class Foo(types.ModuleType):
            @property
            def __warningregistry__(self):
                sys.modules['@bar@'] = 'bar'

        sys.modules['@foo@'] = Foo('foo')
        try:
            self.assertWarns(UserWarning, warnings.warn, 'expected')
        finally:
            del sys.modules['@foo@']
            del sys.modules['@bar@']

    def testAssertRaisesRegexMismatch(self):
        def Stub():
            raise Exception('Unexpected')

        self.assertRaisesRegex(
                self.failureException,
                r'"\^Expected\$" does not match "Unexpected"',
                self.assertRaisesRegex, Exception, '^Expected$',
                Stub)
        self.assertRaisesRegex(
                self.failureException,
                r'"\^Expected\$" does not match "Unexpected"',
                self.assertRaisesRegex, Exception,
                re.compile('^Expected$'), Stub)

    def testAssertRaisesExcValue(self):
        class ExceptionMock(Exception):
            pass

        def Stub(foo):
            raise ExceptionMock(foo)
        v = "particular value"

        ctx = self.assertRaises(ExceptionMock)
        with ctx:
            Stub(v)
        e = ctx.exception
        self.assertIsInstance(e, ExceptionMock)
        self.assertEqual(e.args[0], v)

    def testAssertRaisesRegexNoExceptionType(self):
        with self.assertRaises(TypeError):
            self.assertRaisesRegex()
        with self.assertRaises(TypeError):
            self.assertRaisesRegex(ValueError)
        with self.assertRaises(TypeError):
            self.assertRaisesRegex(1, 'expect')
        with self.assertRaises(TypeError):
            self.assertRaisesRegex(object, 'expect')
        with self.assertRaises(TypeError):
            self.assertRaisesRegex((ValueError, 1), 'expect')
        with self.assertRaises(TypeError):
            self.assertRaisesRegex((ValueError, object), 'expect')

    def testAssertWarnsCallable(self):
        def _runtime_warn():
            warnings.warn("foo", RuntimeWarning)
        # Success when the right warning is triggered, even several times
        self.assertWarns(RuntimeWarning, _runtime_warn)
        self.assertWarns(RuntimeWarning, _runtime_warn)
        # A tuple of warning classes is accepted
        self.assertWarns((DeprecationWarning, RuntimeWarning), _runtime_warn)
        # *args and **kwargs also work
        self.assertWarns(RuntimeWarning,
                         warnings.warn, "foo", category=RuntimeWarning)
        # Failure when no warning is triggered
        with self.assertRaises(self.failureException):
            self.assertWarns(RuntimeWarning, lambda: 0)
        # Failure when the function is None
        with self.assertRaises(TypeError):
            self.assertWarns(RuntimeWarning, None)
        # Failure when another warning is triggered
        with warnings.catch_warnings():
            # Force default filter (in case tests are run with -We)
            warnings.simplefilter("default", RuntimeWarning)
            with self.assertRaises(self.failureException):
                self.assertWarns(DeprecationWarning, _runtime_warn)
        # Filters for other warnings are not modified
        with warnings.catch_warnings():
            warnings.simplefilter("error", RuntimeWarning)
            with self.assertRaises(RuntimeWarning):
                self.assertWarns(DeprecationWarning, _runtime_warn)

    def testAssertWarnsContext(self):
        # Believe it or not, it is preferable to duplicate all tests above,
        # to make sure the __warningregistry__ $@ is circumvented correctly.
        def _runtime_warn():
            warnings.warn("foo", RuntimeWarning)
        _runtime_warn_lineno = inspect.getsourcelines(_runtime_warn)[1]
        with self.assertWarns(RuntimeWarning) as cm:
            _runtime_warn()
        # A tuple of warning classes is accepted
        with self.assertWarns((DeprecationWarning, RuntimeWarning)) as cm:
            _runtime_warn()
        # The context manager exposes various useful attributes
        self.assertIsInstance(cm.warning, RuntimeWarning)
        self.assertEqual(cm.warning.args[0], "foo")
        self.assertIn("test_case.py", cm.filename)
        self.assertEqual(cm.lineno, _runtime_warn_lineno + 1)
        # Same with several warnings
        with self.assertWarns(RuntimeWarning):
            _runtime_warn()
            _runtime_warn()
        with self.assertWarns(RuntimeWarning):
            warnings.warn("foo", category=RuntimeWarning)
        # Failure when no warning is triggered
        with self.assertRaises(self.failureException):
            with self.assertWarns(RuntimeWarning):
                pass
        # Custom message
        with self.assertRaisesRegex(self.failureException, 'foobar'):
            with self.assertWarns(RuntimeWarning, msg='foobar'):
                pass
        # Invalid keyword argument
        with self.assertRaisesRegex(TypeError, 'foobar'):
            with self.assertWarns(RuntimeWarning, foobar=42):
                pass
        # Failure when another warning is triggered
        with warnings.catch_warnings():
            # Force default filter (in case tests are run with -We)
            warnings.simplefilter("default", RuntimeWarning)
            with self.assertRaises(self.failureException):
                with self.assertWarns(DeprecationWarning):
                    _runtime_warn()
        # Filters for other warnings are not modified
        with warnings.catch_warnings():
            warnings.simplefilter("error", RuntimeWarning)
            with self.assertRaises(RuntimeWarning):
                with self.assertWarns(DeprecationWarning):
                    _runtime_warn()

    def testAssertWarnsNoExceptionType(self):
        with self.assertRaises(TypeError):
            self.assertWarns()
        with self.assertRaises(TypeError):
            self.assertWarns(1)
        with self.assertRaises(TypeError):
            self.assertWarns(object)
        with self.assertRaises(TypeError):
            self.assertWarns((UserWarning, 1))
        with self.assertRaises(TypeError):
            self.assertWarns((UserWarning, object))
        with self.assertRaises(TypeError):
            self.assertWarns((UserWarning, Exception))

    def testAssertWarnsRegexCallable(self):
        def _runtime_warn(msg):
            warnings.warn(msg, RuntimeWarning)
        self.assertWarnsRegex(RuntimeWarning, "o+",
                              _runtime_warn, "foox")
        # Failure when no warning is triggered
        with self.assertRaises(self.failureException):
            self.assertWarnsRegex(RuntimeWarning, "o+",
                                  lambda: 0)
        # Failure when the function is None
        with self.assertRaises(TypeError):
            self.assertWarnsRegex(RuntimeWarning, "o+", None)
        # Failure when another warning is triggered
        with warnings.catch_warnings():
            # Force default filter (in case tests are run with -We)
            warnings.simplefilter("default", RuntimeWarning)
            with self.assertRaises(self.failureException):
                self.assertWarnsRegex(DeprecationWarning, "o+",
                                      _runtime_warn, "foox")
        # Failure when message doesn't match
        with self.assertRaises(self.failureException):
            self.assertWarnsRegex(RuntimeWarning, "o+",
                                  _runtime_warn, "barz")
        # A little trickier: we ask RuntimeWarnings to be raised, and then
        # check for some of them.  It is implementation-defined whether
        # non-matching RuntimeWarnings are simply re-raised, or produce a
        # failureException.
        with warnings.catch_warnings():
            warnings.simplefilter("error", RuntimeWarning)
            with self.assertRaises((RuntimeWarning, self.failureException)):
                self.assertWarnsRegex(RuntimeWarning, "o+",
                                      _runtime_warn, "barz")

    def testAssertWarnsRegexContext(self):
        # Same as above, but with assertWarnsRegex as a context manager
        def _runtime_warn(msg):
            warnings.warn(msg, RuntimeWarning)
        _runtime_warn_lineno = inspect.getsourcelines(_runtime_warn)[1]
        with self.assertWarnsRegex(RuntimeWarning, "o+") as cm:
            _runtime_warn("foox")
        self.assertIsInstance(cm.warning, RuntimeWarning)
        self.assertEqual(cm.warning.args[0], "foox")
        self.assertIn("test_case.py", cm.filename)
        self.assertEqual(cm.lineno, _runtime_warn_lineno + 1)
        # Failure when no warning is triggered
        with self.assertRaises(self.failureException):
            with self.assertWarnsRegex(RuntimeWarning, "o+"):
                pass
        # Custom message
        with self.assertRaisesRegex(self.failureException, 'foobar'):
            with self.assertWarnsRegex(RuntimeWarning, 'o+', msg='foobar'):
                pass
        # Invalid keyword argument
        with self.assertRaisesRegex(TypeError, 'foobar'):
            with self.assertWarnsRegex(RuntimeWarning, 'o+', foobar=42):
                pass
        # Failure when another warning is triggered
        with warnings.catch_warnings():
            # Force default filter (in case tests are run with -We)
            warnings.simplefilter("default", RuntimeWarning)
            with self.assertRaises(self.failureException):
                with self.assertWarnsRegex(DeprecationWarning, "o+"):
                    _runtime_warn("foox")
        # Failure when message doesn't match
        with self.assertRaises(self.failureException):
            with self.assertWarnsRegex(RuntimeWarning, "o+"):
                _runtime_warn("barz")
        # A little trickier: we ask RuntimeWarnings to be raised, and then
        # check for some of them.  It is implementation-defined whether
        # non-matching RuntimeWarnings are simply re-raised, or produce a
        # failureException.
        with warnings.catch_warnings():
            warnings.simplefilter("error", RuntimeWarning)
            with self.assertRaises((RuntimeWarning, self.failureException)):
                with self.assertWarnsRegex(RuntimeWarning, "o+"):
                    _runtime_warn("barz")

    def testAssertWarnsRegexNoExceptionType(self):
        with self.assertRaises(TypeError):
            self.assertWarnsRegex()
        with self.assertRaises(TypeError):
            self.assertWarnsRegex(UserWarning)
        with self.assertRaises(TypeError):
            self.assertWarnsRegex(1, 'expect')
        with self.assertRaises(TypeError):
            self.assertWarnsRegex(object, 'expect')
        with self.assertRaises(TypeError):
            self.assertWarnsRegex((UserWarning, 1), 'expect')
        with self.assertRaises(TypeError):
            self.assertWarnsRegex((UserWarning, object), 'expect')
        with self.assertRaises(TypeError):
            self.assertWarnsRegex((UserWarning, Exception), 'expect')

    @contextlib.contextmanager
    def assertNoStderr(self):
        with captured_stderr() as buf:
            yield
        self.assertEqual(buf.getvalue(), "")

    def assertLogRecords(self, records, matches):
        self.assertEqual(len(records), len(matches))
        for rec, match in zip(records, matches):
            self.assertIsInstance(rec, logging.LogRecord)
            for k, v in match.items():
                self.assertEqual(getattr(rec, k), v)

    def testAssertLogsDefaults(self):
        # defaults: root logger, level INFO
        with self.assertNoStderr():
            with self.assertLogs() as cm:
                log_foo.info("1")
                log_foobar.debug("2")
            self.assertEqual(cm.output, ["INFO:foo:1"])
            self.assertLogRecords(cm.records, [{'name': 'foo'}])

    def testAssertLogsTwoMatchingMessages(self):
        # Same, but with two matching log messages
        with self.assertNoStderr():
            with self.assertLogs() as cm:
                log_foo.info("1")
                log_foobar.debug("2")
                log_quux.warning("3")
            self.assertEqual(cm.output, ["INFO:foo:1", "WARNING:quux:3"])
            self.assertLogRecords(cm.records,
                                   [{'name': 'foo'}, {'name': 'quux'}])

    def checkAssertLogsPerLevel(self, level):
        # Check level filtering
        with self.assertNoStderr():
            with self.assertLogs(level=level) as cm:
                log_foo.warning("1")
                log_foobar.error("2")
                log_quux.critical("3")
            self.assertEqual(cm.output, ["ERROR:foo.bar:2", "CRITICAL:quux:3"])
            self.assertLogRecords(cm.records,
                                   [{'name': 'foo.bar'}, {'name': 'quux'}])

    def testAssertLogsPerLevel(self):
        self.checkAssertLogsPerLevel(logging.ERROR)
        self.checkAssertLogsPerLevel('ERROR')

    def checkAssertLogsPerLogger(self, logger):
        # Check per-logger filtering
        with self.assertNoStderr():
            with self.assertLogs(level='DEBUG') as outer_cm:
                with self.assertLogs(logger, level='DEBUG') as cm:
                    log_foo.info("1")
                    log_foobar.debug("2")
                    log_quux.warning("3")
                self.assertEqual(cm.output, ["INFO:foo:1", "DEBUG:foo.bar:2"])
                self.assertLogRecords(cm.records,
                                       [{'name': 'foo'}, {'name': 'foo.bar'}])
            # The outer catchall caught the quux log
            self.assertEqual(outer_cm.output, ["WARNING:quux:3"])

    def testAssertLogsPerLogger(self):
        self.checkAssertLogsPerLogger(logging.getLogger('foo'))
        self.checkAssertLogsPerLogger('foo')

    def testAssertLogsFailureNoLogs(self):
        # Failure due to no logs
        with self.assertNoStderr():
            with self.assertRaises(self.failureException):
                with self.assertLogs():
                    pass

    def testAssertLogsFailureLevelTooHigh(self):
        # Failure due to level too high
        with self.assertNoStderr():
            with self.assertRaises(self.failureException):
                with self.assertLogs(level='WARNING'):
                    log_foo.info("1")

    def testAssertLogsFailureLevelTooHigh_FilterInRootLogger(self):
        # Failure due to level too high - message propagated to root
        with self.assertNoStderr():
            oldLevel = log_foo.level
            log_foo.setLevel(logging.INFO)
            try:
                with self.assertRaises(self.failureException):
                    with self.assertLogs(level='WARNING'):
                        log_foo.info("1")
            finally:
                log_foo.setLevel(oldLevel)

    def testAssertLogsFailureMismatchingLogger(self):
        # Failure due to mismatching logger (and the logged message is
        # passed through)
        with self.assertLogs('quux', level='ERROR'):
            with self.assertRaises(self.failureException):
                with self.assertLogs('foo'):
                    log_quux.error("1")

    def testAssertLogsUnexpectedException(self):
        # Check unexpected exception will go through.
        with self.assertRaises(ZeroDivisionError):
            with self.assertLogs():
                raise ZeroDivisionError("Unexpected")

    def testAssertNoLogsDefault(self):
        with self.assertRaises(self.failureException) as cm:
            with self.assertNoLogs():
                log_foo.info("1")
                log_foobar.debug("2")
        self.assertEqual(
            str(cm.exception),
            "Unexpected logs found: ['INFO:foo:1']",
        )

    def testAssertNoLogsFailureFoundLogs(self):
        with self.assertRaises(self.failureException) as cm:
            with self.assertNoLogs():
                log_quux.error("1")
                log_foo.error("foo")

        self.assertEqual(
            str(cm.exception),
            "Unexpected logs found: ['ERROR:quux:1', 'ERROR:foo:foo']",
        )

    def testAssertNoLogsPerLogger(self):
        with self.assertNoStderr():
            with self.assertLogs(log_quux):
                with self.assertNoLogs(logger=log_foo):
                    log_quux.error("1")

    def testAssertNoLogsFailurePerLogger(self):
        # Failure due to unexpected logs for the given logger or its
        # children.
        with self.assertRaises(self.failureException) as cm:
            with self.assertLogs(log_quux):
                with self.assertNoLogs(logger=log_foo):
                    log_quux.error("1")
                    log_foobar.info("2")
        self.assertEqual(
            str(cm.exception),
            "Unexpected logs found: ['INFO:foo.bar:2']",
        )

    def testAssertNoLogsPerLevel(self):
        # Check per-level filtering
        with self.assertNoStderr():
            with self.assertNoLogs(level="ERROR"):
                log_foo.info("foo")
                log_quux.debug("1")

    def testAssertNoLogsFailurePerLevel(self):
        # Failure due to unexpected logs at the specified level.
        with self.assertRaises(self.failureException) as cm:
            with self.assertNoLogs(level="DEBUG"):
                log_foo.debug("foo")
                log_quux.debug("1")
        self.assertEqual(
            str(cm.exception),
            "Unexpected logs found: ['DEBUG:foo:foo', 'DEBUG:quux:1']",
        )

    def testAssertNoLogsUnexpectedException(self):
        # Check unexpected exception will go through.
        with self.assertRaises(ZeroDivisionError):
            with self.assertNoLogs():
                raise ZeroDivisionError("Unexpected")

    def testAssertNoLogsYieldsNone(self):
        with self.assertNoLogs() as value:
            pass
        self.assertIsNone(value)

    def testAssertStartswith(self):
        self.assertStartsWith('ababahalamaha', 'ababa')
        self.assertStartsWith('ababahalamaha', ('x', 'ababa', 'y'))
        self.assertStartsWith(UserString('ababahalamaha'), 'ababa')
        self.assertStartsWith(UserString('ababahalamaha'), ('x', 'ababa', 'y'))
        self.assertStartsWith(bytearray(b'ababahalamaha'), b'ababa')
        self.assertStartsWith(bytearray(b'ababahalamaha'), (b'x', b'ababa', b'y'))
        self.assertStartsWith(b'ababahalamaha', bytearray(b'ababa'))
        self.assertStartsWith(b'ababahalamaha',
                (bytearray(b'x'), bytearray(b'ababa'), bytearray(b'y')))

        with self.assertRaises(self.failureException) as cm:
            self.assertStartsWith('ababahalamaha', 'amaha')
        self.assertEqual(str(cm.exception),
                "'ababahalamaha' doesn't start with 'amaha'")
        with self.assertRaises(self.failureException) as cm:
            self.assertStartsWith('ababahalamaha', ('x', 'y'))
        self.assertEqual(str(cm.exception),
                "'ababahalamaha' doesn't start with any of ('x', 'y')")

        with self.assertRaises(self.failureException) as cm:
            self.assertStartsWith(b'ababahalamaha', 'ababa')
        self.assertEqual(str(cm.exception), 'Expected str, not bytes')
        with self.assertRaises(self.failureException) as cm:
            self.assertStartsWith(b'ababahalamaha', ('amaha', 'ababa'))
        self.assertEqual(str(cm.exception), 'Expected str, not bytes')
        with self.assertRaises(self.failureException) as cm:
            self.assertStartsWith([], 'ababa')
        self.assertEqual(str(cm.exception), 'Expected str, not list')
        with self.assertRaises(self.failureException) as cm:
            self.assertStartsWith('ababahalamaha', b'ababa')
        self.assertEqual(str(cm.exception), 'Expected bytes, not str')
        with self.assertRaises(self.failureException) as cm:
            self.assertStartsWith('ababahalamaha', (b'amaha', b'ababa'))
        self.assertEqual(str(cm.exception), 'Expected bytes, not str')
        with self.assertRaises(TypeError):
            self.assertStartsWith('ababahalamaha', ord('a'))

        with self.assertRaises(self.failureException) as cm:
            self.assertStartsWith('ababahalamaha', 'amaha', 'abracadabra')
        self.assertIn('ababahalamaha', str(cm.exception))
        with self.assertRaises(self.failureException) as cm:
            self.assertStartsWith('ababahalamaha', 'amaha', msg='abracadabra')
        self.assertIn('ababahalamaha', str(cm.exception))

    def testAssertNotStartswith(self):
        self.assertNotStartsWith('ababahalamaha', 'amaha')
        self.assertNotStartsWith('ababahalamaha', ('x', 'amaha', 'y'))
        self.assertNotStartsWith(UserString('ababahalamaha'), 'amaha')
        self.assertNotStartsWith(UserString('ababahalamaha'), ('x', 'amaha', 'y'))
        self.assertNotStartsWith(bytearray(b'ababahalamaha'), b'amaha')
        self.assertNotStartsWith(bytearray(b'ababahalamaha'), (b'x', b'amaha', b'y'))
        self.assertNotStartsWith(b'ababahalamaha', bytearray(b'amaha'))
        self.assertNotStartsWith(b'ababahalamaha',
                (bytearray(b'x'), bytearray(b'amaha'), bytearray(b'y')))

        with self.assertRaises(self.failureException) as cm:
            self.assertNotStartsWith('ababahalamaha', 'ababa')
        self.assertEqual(str(cm.exception),
                "'ababahalamaha' starts with 'ababa'")
        with self.assertRaises(self.failureException) as cm:
            self.assertNotStartsWith('ababahalamaha', ('x', 'ababa', 'y'))
        self.assertEqual(str(cm.exception),
                "'ababahalamaha' starts with 'ababa'")

        with self.assertRaises(self.failureException) as cm:
            self.assertNotStartsWith(b'ababahalamaha', 'ababa')
        self.assertEqual(str(cm.exception), 'Expected str, not bytes')
        with self.assertRaises(self.failureException) as cm:
            self.assertNotStartsWith(b'ababahalamaha', ('amaha', 'ababa'))
        self.assertEqual(str(cm.exception), 'Expected str, not bytes')
        with self.assertRaises(self.failureException) as cm:
            self.assertNotStartsWith([], 'ababa')
        self.assertEqual(str(cm.exception), 'Expected str, not list')
        with self.assertRaises(self.failureException) as cm:
            self.assertNotStartsWith('ababahalamaha', b'ababa')
        self.assertEqual(str(cm.exception), 'Expected bytes, not str')
        with self.assertRaises(self.failureException) as cm:
            self.assertNotStartsWith('ababahalamaha', (b'amaha', b'ababa'))
        self.assertEqual(str(cm.exception), 'Expected bytes, not str')
        with self.assertRaises(TypeError):
            self.assertNotStartsWith('ababahalamaha', ord('a'))

        with self.assertRaises(self.failureException) as cm:
            self.assertNotStartsWith('ababahalamaha', 'ababa', 'abracadabra')
        self.assertIn('ababahalamaha', str(cm.exception))
        with self.assertRaises(self.failureException) as cm:
            self.assertNotStartsWith('ababahalamaha', 'ababa', msg='abracadabra')
        self.assertIn('ababahalamaha', str(cm.exception))

    def testAssertEndswith(self):
        self.assertEndsWith('ababahalamaha', 'amaha')
        self.assertEndsWith('ababahalamaha', ('x', 'amaha', 'y'))
        self.assertEndsWith(UserString('ababahalamaha'), 'amaha')
        self.assertEndsWith(UserString('ababahalamaha'), ('x', 'amaha', 'y'))
        self.assertEndsWith(bytearray(b'ababahalamaha'), b'amaha')
        self.assertEndsWith(bytearray(b'ababahalamaha'), (b'x', b'amaha', b'y'))
        self.assertEndsWith(b'ababahalamaha', bytearray(b'amaha'))
        self.assertEndsWith(b'ababahalamaha',
                (bytearray(b'x'), bytearray(b'amaha'), bytearray(b'y')))

        with self.assertRaises(self.failureException) as cm:
            self.assertEndsWith('ababahalamaha', 'ababa')
        self.assertEqual(str(cm.exception),
                "'ababahalamaha' doesn't end with 'ababa'")
        with self.assertRaises(self.failureException) as cm:
            self.assertEndsWith('ababahalamaha', ('x', 'y'))
        self.assertEqual(str(cm.exception),
                "'ababahalamaha' doesn't end with any of ('x', 'y')")

        with self.assertRaises(self.failureException) as cm:
            self.assertEndsWith(b'ababahalamaha', 'amaha')
        self.assertEqual(str(cm.exception), 'Expected str, not bytes')
        with self.assertRaises(self.failureException) as cm:
            self.assertEndsWith(b'ababahalamaha', ('ababa', 'amaha'))
        self.assertEqual(str(cm.exception), 'Expected str, not bytes')
        with self.assertRaises(self.failureException) as cm:
            self.assertEndsWith([], 'amaha')
        self.assertEqual(str(cm.exception), 'Expected str, not list')
        with self.assertRaises(self.failureException) as cm:
            self.assertEndsWith('ababahalamaha', b'amaha')
        self.assertEqual(str(cm.exception), 'Expected bytes, not str')
        with self.assertRaises(self.failureException) as cm:
            self.assertEndsWith('ababahalamaha', (b'ababa', b'amaha'))
        self.assertEqual(str(cm.exception), 'Expected bytes, not str')
        with self.assertRaises(TypeError):
            self.assertEndsWith('ababahalamaha', ord('a'))

        with self.assertRaises(self.failureException) as cm:
            self.assertEndsWith('ababahalamaha', 'ababa', 'abracadabra')
        self.assertIn('ababahalamaha', str(cm.exception))
        with self.assertRaises(self.failureException) as cm:
            self.assertEndsWith('ababahalamaha', 'ababa', msg='abracadabra')
        self.assertIn('ababahalamaha', str(cm.exception))

    def testAssertNotEndswith(self):
        self.assertNotEndsWith('ababahalamaha', 'ababa')
        self.assertNotEndsWith('ababahalamaha', ('x', 'ababa', 'y'))
        self.assertNotEndsWith(UserString('ababahalamaha'), 'ababa')
        self.assertNotEndsWith(UserString('ababahalamaha'), ('x', 'ababa', 'y'))
        self.assertNotEndsWith(bytearray(b'ababahalamaha'), b'ababa')
        self.assertNotEndsWith(bytearray(b'ababahalamaha'), (b'x', b'ababa', b'y'))
        self.assertNotEndsWith(b'ababahalamaha', bytearray(b'ababa'))
        self.assertNotEndsWith(b'ababahalamaha',
                (bytearray(b'x'), bytearray(b'ababa'), bytearray(b'y')))

        with self.assertRaises(self.failureException) as cm:
            self.assertNotEndsWith('ababahalamaha', 'amaha')
        self.assertEqual(str(cm.exception),
                "'ababahalamaha' ends with 'amaha'")
        with self.assertRaises(self.failureException) as cm:
            self.assertNotEndsWith('ababahalamaha', ('x', 'amaha', 'y'))
        self.assertEqual(str(cm.exception),
                "'ababahalamaha' ends with 'amaha'")

        with self.assertRaises(self.failureException) as cm:
            self.assertNotEndsWith(b'ababahalamaha', 'amaha')
        self.assertEqual(str(cm.exception), 'Expected str, not bytes')
        with self.assertRaises(self.failureException) as cm:
            self.assertNotEndsWith(b'ababahalamaha', ('ababa', 'amaha'))
        self.assertEqual(str(cm.exception), 'Expected str, not bytes')
        with self.assertRaises(self.failureException) as cm:
            self.assertNotEndsWith([], 'amaha')
        self.assertEqual(str(cm.exception), 'Expected str, not list')
        with self.assertRaises(self.failureException) as cm:
            self.assertNotEndsWith('ababahalamaha', b'amaha')
        self.assertEqual(str(cm.exception), 'Expected bytes, not str')
        with self.assertRaises(self.failureException) as cm:
            self.assertNotEndsWith('ababahalamaha', (b'ababa', b'amaha'))
        self.assertEqual(str(cm.exception), 'Expected bytes, not str')
        with self.assertRaises(TypeError):
            self.assertNotEndsWith('ababahalamaha', ord('a'))

        with self.assertRaises(self.failureException) as cm:
            self.assertNotEndsWith('ababahalamaha', 'amaha', 'abracadabra')
        self.assertIn('ababahalamaha', str(cm.exception))
        with self.assertRaises(self.failureException) as cm:
            self.assertNotEndsWith('ababahalamaha', 'amaha', msg='abracadabra')
        self.assertIn('ababahalamaha', str(cm.exception))

    def testDeprecatedFailMethods(self):
        """Test that the deprecated fail* methods get removed in 3.12"""
        deprecated_names = [
            'failIfEqual', 'failUnlessEqual', 'failUnlessAlmostEqual',
            'failIfAlmostEqual', 'failUnless', 'failUnlessRaises', 'failIf',
            'assertNotEquals', 'assertEquals', 'assertAlmostEquals',
            'assertNotAlmostEquals', 'assert_', 'assertDictContainsSubset',
            'assertRaisesRegexp', 'assertRegexpMatches'
        ]
        for deprecated_name in deprecated_names:
            with self.assertRaises(AttributeError):
                getattr(self, deprecated_name)

    def testDeepcopy(self):
        # Issue: 5660
        class TestableTest(unittest.TestCase):
            def testNothing(self):
                pass

        test = TestableTest('testNothing')

        # This shouldn't blow up
        deepcopy(test)

    def testPickle(self):
        # Issue 10326

        # Can't use TestCase classes defined in Test class as
        # pickle does not work with inner classes
        test = unittest.TestCase('run')
        for protocol in range(pickle.HIGHEST_PROTOCOL + 1):

            # blew up prior to fix
            pickled_test = pickle.dumps(test, protocol=protocol)
            unpickled_test = pickle.loads(pickled_test)
            self.assertEqual(test, unpickled_test)

            # exercise the TestCase instance in a way that will invoke
            # the type equality lookup mechanism
            unpickled_test.assertEqual(set(), set())

    def testKeyboardInterrupt(self):
        def _raise(self=None):
            raise KeyboardInterrupt
        def nothing(self):
            pass

        class Test1(unittest.TestCase):
            test_something = _raise

        class Test2(unittest.TestCase):
            setUp = _raise
            test_something = nothing

        class Test3(unittest.TestCase):
            test_something = nothing
            tearDown = _raise

        class Test4(unittest.TestCase):
            def test_something(self):
                self.addCleanup(_raise)

        for klass in (Test1, Test2, Test3, Test4):
            with self.assertRaises(KeyboardInterrupt):
                klass('test_something').run()

    def testSkippingEverywhere(self):
        def _skip(self=None):
            raise unittest.SkipTest('some reason')
        def nothing(self):
            pass

        class Test1(unittest.TestCase):
            test_something = _skip

        class Test2(unittest.TestCase):
            setUp = _skip
            test_something = nothing

        class Test3(unittest.TestCase):
            test_something = nothing
            tearDown = _skip

        class Test4(unittest.TestCase):
            def test_something(self):
                self.addCleanup(_skip)

        for klass in (Test1, Test2, Test3, Test4):
            result = unittest.TestResult()
            klass('test_something').run(result)
            self.assertEqual(len(result.skipped), 1)
            self.assertEqual(result.testsRun, 1)

    def testSystemExit(self):
        def _raise(self=None):
            raise SystemExit
        def nothing(self):
            pass

        class Test1(unittest.TestCase):
            test_something = _raise

        class Test2(unittest.TestCase):
            setUp = _raise
            test_something = nothing

        class Test3(unittest.TestCase):
            test_something = nothing
            tearDown = _raise

        class Test4(unittest.TestCase):
            def test_something(self):
                self.addCleanup(_raise)

        for klass in (Test1, Test2, Test3, Test4):
            result = unittest.TestResult()
            klass('test_something').run(result)
            self.assertEqual(len(result.errors), 1)
            self.assertEqual(result.testsRun, 1)

    @support.cpython_only
    def testNoCycles(self):
        case = unittest.TestCase()
        wr = weakref.ref(case)
        with support.disable_gc():
            del case
            self.assertFalse(wr())

    def test_no_exception_leak(self):
        # Issue #19880: TestCase.run() should not keep a reference
        # to the exception
        class MyException(Exception):
            ninstance = 0

            def __init__(self):
                MyException.ninstance += 1
                Exception.__init__(self)

            def __del__(self):
                MyException.ninstance -= 1

        class TestCase(unittest.TestCase):
            def test1(self):
                raise MyException()

            @unittest.expectedFailure
            def test2(self):
                raise MyException()

        for method_name in ('test1', 'test2'):
            testcase = TestCase(method_name)
            testcase.run()
            gc_collect()  # For PyPy or other GCs.
            self.assertEqual(MyException.ninstance, 0)


if __name__ == "__main__":
    unittest.main()
