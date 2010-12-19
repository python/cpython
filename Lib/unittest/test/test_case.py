import difflib
import pprint
import pickle
import re
import sys
import warnings
import inspect

from copy import deepcopy
from test import support

import unittest

from .support import (
    TestEquality, TestHashing, LoggingResult,
    ResultWithNoStartTestRunStopTestRun
)


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

        self.assertEqual(Test().id()[-13:], '.Test.runTest')

    # "class TestCase([methodName])"
    # ...
    # "Each instance of TestCase will run a single test method: the
    # method named methodName."
    def test_init__test_name__valid(self):
        class Test(unittest.TestCase):
            def runTest(self): raise MyException()
            def test(self): pass

        self.assertEqual(Test('test').id()[-10:], '.Test.test')

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

        expected = ['startTest', 'setUp', 'test', 'tearDown',
                    'addError', 'stopTest']
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
                    'tearDown', 'addError', 'stopTest', 'stopTestRun']
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

        expected = ['startTest', 'setUp', 'test', 'tearDown',
                    'addFailure', 'stopTest']
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
                    'tearDown', 'addFailure', 'stopTest', 'stopTestRun']
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

        Foo('test').run()

    # "This class attribute gives the exception raised by the test() method.
    # If a test framework needs to use a specialized exception, possibly to
    # carry additional information, it must subclass this exception in
    # order to ``play fair'' with the framework.  The initial value of this
    # attribute is AssertionError"
    def test_failureException__default(self):
        class Foo(unittest.TestCase):
            def test(self):
                pass

        self.assertTrue(Foo('test').failureException is AssertionError)

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

        self.assertTrue(Foo('test').failureException is RuntimeError)


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

        self.assertTrue(Foo('test').failureException is RuntimeError)


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


    # "If result is omitted or None, a temporary result object is created
    # and used, but is not made available to the caller. As TestCase owns the
    # temporary result startTestRun and stopTestRun are called.

    def test_run__uses_defaultTestResult(self):
        events = []

        class Foo(unittest.TestCase):
            def test(self):
                events.append('test')

            def defaultTestResult(self):
                return LoggingResult(events)

        # Make run() find a result object on its own
        Foo('test').run()

        expected = ['startTestRun', 'startTest', 'test', 'addSuccess',
            'stopTest', 'stopTestRun']
        self.assertEqual(events, expected)

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
        # from this TestCase instance but since its a local nothing else
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
        thing = []
        self.assertIsInstance(thing, list)
        self.assertRaises(self.failureException, self.assertIsInstance,
                          thing, dict)

    def testAssertNotIsInstance(self):
        thing = []
        self.assertNotIsInstance(thing, dict)
        self.assertRaises(self.failureException, self.assertNotIsInstance,
                          thing, list)

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

    def testAssertDictContainsSubset(self):
        self.assertDictContainsSubset({}, {})
        self.assertDictContainsSubset({}, {'a': 1})
        self.assertDictContainsSubset({'a': 1}, {'a': 1})
        self.assertDictContainsSubset({'a': 1}, {'a': 1, 'b': 2})
        self.assertDictContainsSubset({'a': 1, 'b': 2}, {'a': 1, 'b': 2})

        with self.assertRaises(self.failureException):
            self.assertDictContainsSubset({1: "one"}, {})

        with self.assertRaises(self.failureException):
            self.assertDictContainsSubset({'a': 2}, {'a': 1})

        with self.assertRaises(self.failureException):
            self.assertDictContainsSubset({'c': 1}, {'a': 1})

        with self.assertRaises(self.failureException):
            self.assertDictContainsSubset({'a': 1, 'c': 1}, {'a': 1})

        with self.assertRaises(self.failureException):
            self.assertDictContainsSubset({'a': 1, 'c': 1}, {'a': 1})

        one = ''.join(chr(i) for i in range(255))
        # this used to cause a UnicodeDecodeError constructing the failure msg
        with self.assertRaises(self.failureException):
            self.assertDictContainsSubset({'foo': one}, {'foo': '\uFFFD'})

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
        self.assertTrue(len(msg) < len(diff))
        self.assertIn(omitted, msg)

        self.maxDiff = len(diff) * 2
        try:
            self.assertSequenceEqual(seq1, seq2)
        except self.failureException as e:
            msg = e.args[0]
        else:
            self.fail('assertSequenceEqual did not fail.')
        self.assertTrue(len(msg) > len(diff))
        self.assertNotIn(omitted, msg)

        self.maxDiff = None
        try:
            self.assertSequenceEqual(seq1, seq2)
        except self.failureException as e:
            msg = e.args[0]
        else:
            self.fail('assertSequenceEqual did not fail.')
        self.assertTrue(len(msg) > len(diff))
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

    def testassertCountEqual(self):
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

        # hashable types, but not orderable
        self.assertRaises(self.failureException, self.assertCountEqual,
                          [], [divmod, 'x', 1, 5j, 2j, frozenset()])
        # comparing dicts
        self.assertCountEqual([{'a': 1}, {'b': 2}], [{'b': 2}, {'a': 1}])
        # comparing heterogenous non-hashable sequences
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

            # no fair testing ourself with ourself, and assertEqual is used for strings
            # so can't use assertEqual either. Just use assertTrue.
            self.assertTrue(sample_text_error == error)

    def testAsertEqualSingleLine(self):
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
            error = str(e).split('\n', 1)[1]
            self.assertTrue(sample_text_error == error)

    def testAssertIsNone(self):
        self.assertIsNone(None)
        self.assertRaises(self.failureException, self.assertIsNone, False)
        self.assertIsNotNone('DjZoPloGears on Rails')
        self.assertRaises(self.failureException, self.assertIsNotNone, None)

    def testAssertRegex(self):
        self.assertRegex('asdfabasdf', r'ab+')
        self.assertRaises(self.failureException, self.assertRegex,
                          'saaas', r'aaaa')

    def testAssertRaisesRegex(self):
        class ExceptionMock(Exception):
            pass

        def Stub():
            raise ExceptionMock('We expect')

        self.assertRaisesRegex(ExceptionMock, re.compile('expect$'), Stub)
        self.assertRaisesRegex(ExceptionMock, 'expect$', Stub)

    def testAssertNotRaisesRegex(self):
        self.assertRaisesRegex(
                self.failureException, '^Exception not raised by <lambda>$',
                self.assertRaisesRegex, Exception, re.compile('x'),
                lambda: None)
        self.assertRaisesRegex(
                self.failureException, '^Exception not raised by <lambda>$',
                self.assertRaisesRegex, Exception, 'x',
                lambda: None)

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
        # Believe it or not, it is preferrable to duplicate all tests above,
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

    def testAssertWarnsRegexCallable(self):
        def _runtime_warn(msg):
            warnings.warn(msg, RuntimeWarning)
        self.assertWarnsRegex(RuntimeWarning, "o+",
                              _runtime_warn, "foox")
        # Failure when no warning is triggered
        with self.assertRaises(self.failureException):
            self.assertWarnsRegex(RuntimeWarning, "o+",
                                  lambda: 0)
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

    def testDeprecatedMethodNames(self):
        """Test that the deprecated methods raise a DeprecationWarning.

        The fail* methods will be removed in 3.3. The assert* methods will
        have to stay around for a few more versions.  See #9424.
        """
        old = (
            (self.failIfEqual, (3, 5)),
            (self.assertNotEquals, (3, 5)),
            (self.failUnlessEqual, (3, 3)),
            (self.assertEquals, (3, 3)),
            (self.failUnlessAlmostEqual, (2.0, 2.0)),
            (self.assertAlmostEquals, (2.0, 2.0)),
            (self.failIfAlmostEqual, (3.0, 5.0)),
            (self.assertNotAlmostEquals, (3.0, 5.0)),
            (self.failUnless, (True,)),
            (self.assert_, (True,)),
            (self.failUnlessRaises, (TypeError, lambda _: 3.14 + 'spam')),
            (self.failIf, (False,)),
            (self.assertSameElements, ([1, 1, 2, 3], [1, 2, 3])),
            (self.assertRaisesRegexp, (KeyError, 'foo', lambda: {}['foo'])),
            (self.assertRegexpMatches, ('bar', 'bar')),
        )
        for meth, args in old:
            with self.assertWarns(DeprecationWarning):
                meth(*args)

    def testDeprecatedFailMethods(self):
        """Test that the deprecated fail* methods get removed in 3.3"""
        if sys.version_info[:2] < (3, 3):
            return
        deprecated_names = [
            'failIfEqual', 'failUnlessEqual', 'failUnlessAlmostEqual',
            'failIfAlmostEqual', 'failUnless', 'failUnlessRaises', 'failIf',
            'assertSameElements'
        ]
        for deprecated_name in deprecated_names:
            with self.assertRaises(AttributeError):
                getattr(self, deprecated_name)  # remove these in 3.3

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
