#!/usr/bin/env python
'''
Python unit testing framework, based on Erich Gamma's JUnit and Kent Beck's
Smalltalk testing framework.

This module contains the core framework classes that form the basis of
specific test cases and suites (TestCase, TestSuite etc.), and also a
text-based utility class for running the tests and reporting the results
 (TextTestRunner).

Simple usage:

    import unittest

    class IntegerArithmenticTestCase(unittest.TestCase):
        def testAdd(self):  ## test method names begin 'test*'
            self.assertEqual((1 + 2), 3)
            self.assertEqual(0 + 1, 1)
        def testMultiply(self):
            self.assertEqual((0 * 10), 0)
            self.assertEqual((5 * 8), 40)

    if __name__ == '__main__':
        unittest.main()

Further information is available in the bundled documentation, and from

  http://docs.python.org/library/unittest.html

Copyright (c) 1999-2003 Steve Purcell
Copyright (c) 2003-2009 Python Software Foundation
This module is free software, and you may redistribute it and/or modify
it under the same terms as Python itself, so long as this copyright message
and disclaimer are retained in their original form.

IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF
THIS CODE, EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE.  THE CODE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS,
AND THERE IS NO OBLIGATION WHATSOEVER TO PROVIDE MAINTENANCE,
SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
'''

import difflib
import functools
import os
import pprint
import re
import sys
import time
import traceback
import types
import warnings

##############################################################################
# Exported classes and functions
##############################################################################
__all__ = ['TestResult', 'TestCase', 'TestSuite',
           'TextTestRunner', 'TestLoader', 'FunctionTestCase', 'main',
           'defaultTestLoader', 'SkipTest', 'skip', 'skipIf', 'skipUnless',
           'expectedFailure']

# Expose obsolete functions for backwards compatibility
__all__.extend(['getTestCaseNames', 'makeSuite', 'findTestCases'])


##############################################################################
# Test framework core
##############################################################################

def _strclass(cls):
    return "%s.%s" % (cls.__module__, cls.__name__)


class SkipTest(Exception):
    """
    Raise this exception in a test to skip it.

    Usually you can use TestResult.skip() or one of the skipping decorators
    instead of raising this directly.
    """
    pass

class _ExpectedFailure(Exception):
    """
    Raise this when a test is expected to fail.

    This is an implementation detail.
    """

    def __init__(self, exc_info):
        super(_ExpectedFailure, self).__init__()
        self.exc_info = exc_info

class _UnexpectedSuccess(Exception):
    """
    The test was supposed to fail, but it didn't!
    """
    pass

def _id(obj):
    return obj

def skip(reason):
    """
    Unconditionally skip a test.
    """
    def decorator(test_item):
        if isinstance(test_item, type) and issubclass(test_item, TestCase):
            test_item.__unittest_skip__ = True
            test_item.__unittest_skip_why__ = reason
            return test_item
        @functools.wraps(test_item)
        def skip_wrapper(*args, **kwargs):
            raise SkipTest(reason)
        return skip_wrapper
    return decorator

def skipIf(condition, reason):
    """
    Skip a test if the condition is true.
    """
    if condition:
        return skip(reason)
    return _id

def skipUnless(condition, reason):
    """
    Skip a test unless the condition is true.
    """
    if not condition:
        return skip(reason)
    return _id


def expectedFailure(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        try:
            func(*args, **kwargs)
        except Exception:
            raise _ExpectedFailure(sys.exc_info())
        raise _UnexpectedSuccess
    return wrapper

__unittest = 1

class TestResult(object):
    """Holder for test result information.

    Test results are automatically managed by the TestCase and TestSuite
    classes, and do not need to be explicitly manipulated by writers of tests.

    Each instance holds the total number of tests run, and collections of
    failures and errors that occurred among those test runs. The collections
    contain tuples of (testcase, exceptioninfo), where exceptioninfo is the
    formatted traceback of the error that occurred.
    """
    def __init__(self):
        self.failures = []
        self.errors = []
        self.testsRun = 0
        self.skipped = []
        self.expectedFailures = []
        self.unexpectedSuccesses = []
        self.shouldStop = False

    def startTest(self, test):
        "Called when the given test is about to be run"
        self.testsRun = self.testsRun + 1

    def startTestRun(self):
        """Called once before any tests are executed.

        See startTest for a method called before each test.
        """

    def stopTest(self, test):
        "Called when the given test has been run"
        pass

    def stopTestRun(self):
        """Called once after all tests are executed.

        See stopTest for a method called after each test.
        """

    def addError(self, test, err):
        """Called when an error has occurred. 'err' is a tuple of values as
        returned by sys.exc_info().
        """
        self.errors.append((test, self._exc_info_to_string(err, test)))

    def addFailure(self, test, err):
        """Called when an error has occurred. 'err' is a tuple of values as
        returned by sys.exc_info()."""
        self.failures.append((test, self._exc_info_to_string(err, test)))

    def addSuccess(self, test):
        "Called when a test has completed successfully"
        pass

    def addSkip(self, test, reason):
        """Called when a test is skipped."""
        self.skipped.append((test, reason))

    def addExpectedFailure(self, test, err):
        """Called when an expected failure/error occured."""
        self.expectedFailures.append(
            (test, self._exc_info_to_string(err, test)))

    def addUnexpectedSuccess(self, test):
        """Called when a test was expected to fail, but succeed."""
        self.unexpectedSuccesses.append(test)

    def wasSuccessful(self):
        "Tells whether or not this result was a success"
        return len(self.failures) == len(self.errors) == 0

    def stop(self):
        "Indicates that the tests should be aborted"
        self.shouldStop = True

    def _exc_info_to_string(self, err, test):
        """Converts a sys.exc_info()-style tuple of values into a string."""
        exctype, value, tb = err
        # Skip test runner traceback levels
        while tb and self._is_relevant_tb_level(tb):
            tb = tb.tb_next
        if exctype is test.failureException:
            # Skip assert*() traceback levels
            length = self._count_relevant_tb_levels(tb)
            return ''.join(traceback.format_exception(exctype, value,
                                                      tb, length))
        return ''.join(traceback.format_exception(exctype, value, tb))

    def _is_relevant_tb_level(self, tb):
        return '__unittest' in tb.tb_frame.f_globals

    def _count_relevant_tb_levels(self, tb):
        length = 0
        while tb and not self._is_relevant_tb_level(tb):
            length += 1
            tb = tb.tb_next
        return length

    def __repr__(self):
        return "<%s run=%i errors=%i failures=%i>" % \
               (_strclass(self.__class__), self.testsRun, len(self.errors),
                len(self.failures))


class _AssertRaisesContext(object):
    """A context manager used to implement TestCase.assertRaises* methods."""


    def __init__(self, expected, test_case, callable_obj=None,
                 expected_regexp=None):
        self.expected = expected
        self.failureException = test_case.failureException
        if callable_obj is not None:
            try:
                self.obj_name = callable_obj.__name__
            except AttributeError:
                self.obj_name = str(callable_obj)
        else:
            self.obj_name = None
        self.expected_regex = expected_regexp

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, tb):
        if exc_type is None:
            try:
                exc_name = self.expected.__name__
            except AttributeError:
                exc_name = str(self.expected)
            if self.obj_name:
                raise self.failureException("{0} not raised by {1}"
                    .format(exc_name, self.obj_name))
            else:
                raise self.failureException("{0} not raised"
                    .format(exc_name))
        if not issubclass(exc_type, self.expected):
            # let unexpected exceptions pass through
            return False
        if self.expected_regex is None:
            return True

        expected_regexp = self.expected_regex
        if isinstance(expected_regexp, (bytes, str)):
            expected_regexp = re.compile(expected_regexp)
        if not expected_regexp.search(str(exc_value)):
            raise self.failureException('"%s" does not match "%s"' %
                     (expected_regexp.pattern, str(exc_value)))
        return True


class _AssertWrapper(object):
    """Wrap entries in the _type_equality_funcs registry to make them deep
    copyable."""

    def __init__(self, function):
        self.function = function

    def __deepcopy__(self, memo):
        memo[id(self)] = self


class TestCase(object):
    """A class whose instances are single test cases.

    By default, the test code itself should be placed in a method named
    'runTest'.

    If the fixture may be used for many test cases, create as
    many test methods as are needed. When instantiating such a TestCase
    subclass, specify in the constructor arguments the name of the test method
    that the instance is to execute.

    Test authors should subclass TestCase for their own tests. Construction
    and deconstruction of the test's environment ('fixture') can be
    implemented by overriding the 'setUp' and 'tearDown' methods respectively.

    If it is necessary to override the __init__ method, the base class
    __init__ method must always be called. It is important that subclasses
    should not change the signature of their __init__ method, since instances
    of the classes are instantiated automatically by parts of the framework
    in order to be run.
    """

    # This attribute determines which exception will be raised when
    # the instance's assertion methods fail; test methods raising this
    # exception will be deemed to have 'failed' rather than 'errored'

    failureException = AssertionError

    # This attribute determines whether long messages (including repr of
    # objects used in assert methods) will be printed on failure in *addition*
    # to any explicit message passed.

    longMessage = False


    def __init__(self, methodName='runTest'):
        """Create an instance of the class that will use the named test
           method when executed. Raises a ValueError if the instance does
           not have a method with the specified name.
        """
        self._testMethodName = methodName
        self._resultForDoCleanups = None
        try:
            testMethod = getattr(self, methodName)
        except AttributeError:
            raise ValueError("no such test method in %s: %s" % \
                  (self.__class__, methodName))
        self._testMethodDoc = testMethod.__doc__
        self._cleanups = []

        # Map types to custom assertEqual functions that will compare
        # instances of said type in more detail to generate a more useful
        # error message.
        self._type_equality_funcs = {}
        self.addTypeEqualityFunc(dict, self.assertDictEqual)
        self.addTypeEqualityFunc(list, self.assertListEqual)
        self.addTypeEqualityFunc(tuple, self.assertTupleEqual)
        self.addTypeEqualityFunc(set, self.assertSetEqual)
        self.addTypeEqualityFunc(frozenset, self.assertSetEqual)

    def addTypeEqualityFunc(self, typeobj, function):
        """Add a type specific assertEqual style function to compare a type.

        This method is for use by TestCase subclasses that need to register
        their own type equality functions to provide nicer error messages.

        Args:
            typeobj: The data type to call this function on when both values
                    are of the same type in assertEqual().
            function: The callable taking two arguments and an optional
                    msg= argument that raises self.failureException with a
                    useful error message when the two arguments are not equal.
        """
        self._type_equality_funcs[typeobj] = _AssertWrapper(function)

    def addCleanup(self, function, *args, **kwargs):
        """Add a function, with arguments, to be called when the test is
        completed. Functions added are called on a LIFO basis and are
        called after tearDown on test failure or success.

        Cleanup items are called even if setUp fails (unlike tearDown)."""
        self._cleanups.append((function, args, kwargs))

    def setUp(self):
        "Hook method for setting up the test fixture before exercising it."
        pass

    def tearDown(self):
        "Hook method for deconstructing the test fixture after testing it."
        pass

    def countTestCases(self):
        return 1

    def defaultTestResult(self):
        return TestResult()

    def shortDescription(self):
        """Returns both the test method name and first line of its docstring.

        If no docstring is given, only returns the method name.

        This method overrides unittest.TestCase.shortDescription(), which
        only returns the first line of the docstring, obscuring the name
        of the test upon failure.
        """
        desc = str(self)
        doc_first_line = None

        if self._testMethodDoc:
            doc_first_line = self._testMethodDoc.split("\n")[0].strip()
        if doc_first_line:
            desc = '\n'.join((desc, doc_first_line))
        return desc

    def id(self):
        return "%s.%s" % (_strclass(self.__class__), self._testMethodName)

    def __eq__(self, other):
        if type(self) is not type(other):
            return NotImplemented

        return self._testMethodName == other._testMethodName

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash((type(self), self._testMethodName))

    def __str__(self):
        return "%s (%s)" % (self._testMethodName, _strclass(self.__class__))

    def __repr__(self):
        return "<%s testMethod=%s>" % \
               (_strclass(self.__class__), self._testMethodName)

    def run(self, result=None):
        orig_result = result
        if result is None:
            result = self.defaultTestResult()
            startTestRun = getattr(result, 'startTestRun', None)
            if startTestRun is not None:
                startTestRun()

        self._resultForDoCleanups = result
        result.startTest(self)
        if getattr(self.__class__, "__unittest_skip__", False):
            # If the whole class was skipped.
            try:
                result.addSkip(self, self.__class__.__unittest_skip_why__)
            finally:
                result.stopTest(self)
            return
        testMethod = getattr(self, self._testMethodName)
        try:
            success = False
            try:
                self.setUp()
            except SkipTest as e:
                result.addSkip(self, str(e))
            except Exception:
                result.addError(self, sys.exc_info())
            else:
                try:
                    testMethod()
                except self.failureException:
                    result.addFailure(self, sys.exc_info())
                except _ExpectedFailure as e:
                    result.addExpectedFailure(self, e.exc_info)
                except _UnexpectedSuccess:
                    result.addUnexpectedSuccess(self)
                except SkipTest as e:
                    result.addSkip(self, str(e))
                except Exception:
                    result.addError(self, sys.exc_info())
                else:
                    success = True

                try:
                    self.tearDown()
                except Exception:
                    result.addError(self, sys.exc_info())
                    success = False

            cleanUpSuccess = self.doCleanups()
            success = success and cleanUpSuccess
            if success:
                result.addSuccess(self)
        finally:
            result.stopTest(self)
            if orig_result is None:
                stopTestRun = getattr(result, 'stopTestRun', None)
                if stopTestRun is not None:
                    stopTestRun()

    def doCleanups(self):
        """Execute all cleanup functions. Normally called for you after
        tearDown."""
        result = self._resultForDoCleanups
        ok = True
        while self._cleanups:
            function, args, kwargs = self._cleanups.pop(-1)
            try:
                function(*args, **kwargs)
            except Exception:
                ok = False
                result.addError(self, sys.exc_info())
        return ok

    def __call__(self, *args, **kwds):
        return self.run(*args, **kwds)

    def debug(self):
        """Run the test without collecting errors in a TestResult"""
        self.setUp()
        getattr(self, self._testMethodName)()
        self.tearDown()

    def skipTest(self, reason):
        """Skip this test."""
        raise SkipTest(reason)

    def fail(self, msg=None):
        """Fail immediately, with the given message."""
        raise self.failureException(msg)

    def assertFalse(self, expr, msg=None):
        "Fail the test if the expression is true."
        if expr:
            msg = self._formatMessage(msg, "%r is not False" % expr)
            raise self.failureException(msg)

    def assertTrue(self, expr, msg=None):
        """Fail the test unless the expression is true."""
        if not expr:
            msg = self._formatMessage(msg, "%r is not True" % expr)
            raise self.failureException(msg)

    def _formatMessage(self, msg, standardMsg):
        """Honour the longMessage attribute when generating failure messages.
        If longMessage is False this means:
        * Use only an explicit message if it is provided
        * Otherwise use the standard message for the assert

        If longMessage is True:
        * Use the standard message
        * If an explicit message is provided, plus ' : ' and the explicit message
        """
        if not self.longMessage:
            return msg or standardMsg
        if msg is None:
            return standardMsg
        return standardMsg + ' : ' + msg


    def assertRaises(self, excClass, callableObj=None, *args, **kwargs):
        """Fail unless an exception of class excClass is thrown
           by callableObj when invoked with arguments args and keyword
           arguments kwargs. If a different type of exception is
           thrown, it will not be caught, and the test case will be
           deemed to have suffered an error, exactly as for an
           unexpected exception.

           If called with callableObj omitted or None, will return a
           context object used like this::

                with self.assertRaises(some_error_class):
                    do_something()
        """
        context = _AssertRaisesContext(excClass, self, callableObj)
        if callableObj is None:
            return context
        with context:
            callableObj(*args, **kwargs)

    def _getAssertEqualityFunc(self, first, second):
        """Get a detailed comparison function for the types of the two args.

        Returns: A callable accepting (first, second, msg=None) that will
        raise a failure exception if first != second with a useful human
        readable error message for those types.
        """
        #
        # NOTE(gregory.p.smith): I considered isinstance(first, type(second))
        # and vice versa.  I opted for the conservative approach in case
        # subclasses are not intended to be compared in detail to their super
        # class instances using a type equality func.  This means testing
        # subtypes won't automagically use the detailed comparison.  Callers
        # should use their type specific assertSpamEqual method to compare
        # subclasses if the detailed comparison is desired and appropriate.
        # See the discussion in http://bugs.python.org/issue2578.
        #
        if type(first) is type(second):
            asserter = self._type_equality_funcs.get(type(first))
            if asserter is not None:
                return asserter.function

        return self._baseAssertEqual

    def _baseAssertEqual(self, first, second, msg=None):
        """The default assertEqual implementation, not type specific."""
        if not first == second:
            standardMsg = '%r != %r' % (first, second)
            msg = self._formatMessage(msg, standardMsg)
            raise self.failureException(msg)

    def assertEqual(self, first, second, msg=None):
        """Fail if the two objects are unequal as determined by the '=='
           operator.
        """
        assertion_func = self._getAssertEqualityFunc(first, second)
        assertion_func(first, second, msg=msg)

    def assertNotEqual(self, first, second, msg=None):
        """Fail if the two objects are equal as determined by the '=='
           operator.
        """
        if not first != second:
            msg = self._formatMessage(msg, '%r == %r' % (first, second))
            raise self.failureException(msg)

    def assertAlmostEqual(self, first, second, places=7, msg=None):
        """Fail if the two objects are unequal as determined by their
           difference rounded to the given number of decimal places
           (default 7) and comparing to zero.

           Note that decimal places (from zero) are usually not the same
           as significant digits (measured from the most signficant digit).
        """
        if round(abs(second-first), places) != 0:
            standardMsg = '%r != %r within %r places' % (first, second, places)
            msg = self._formatMessage(msg, standardMsg)
            raise self.failureException(msg)

    def assertNotAlmostEqual(self, first, second, places=7, msg=None):
        """Fail if the two objects are equal as determined by their
           difference rounded to the given number of decimal places
           (default 7) and comparing to zero.

           Note that decimal places (from zero) are usually not the same
           as significant digits (measured from the most signficant digit).
        """
        if round(abs(second-first), places) == 0:
            standardMsg = '%r == %r within %r places' % (first, second, places)
            msg = self._formatMessage(msg, standardMsg)
            raise self.failureException(msg)

    # Synonyms for assertion methods

    # The plurals are undocumented.  Keep them that way to discourage use.
    # Do not add more.  Do not remove.
    # Going through a deprecation cycle on these would annoy many people.
    assertEquals = assertEqual
    assertNotEquals = assertNotEqual
    assertAlmostEquals = assertAlmostEqual
    assertNotAlmostEquals = assertNotAlmostEqual
    assert_ = assertTrue

    # These fail* assertion method names are pending deprecation and will
    # be a DeprecationWarning in 3.2; http://bugs.python.org/issue2578
    def _deprecate(original_func):
        def deprecated_func(*args, **kwargs):
            warnings.warn(
                'Please use {0} instead.'.format(original_func.__name__),
                PendingDeprecationWarning, 2)
            return original_func(*args, **kwargs)
        return deprecated_func

    failUnlessEqual = _deprecate(assertEqual)
    failIfEqual = _deprecate(assertNotEqual)
    failUnlessAlmostEqual = _deprecate(assertAlmostEqual)
    failIfAlmostEqual = _deprecate(assertNotAlmostEqual)
    failUnless = _deprecate(assertTrue)
    failUnlessRaises = _deprecate(assertRaises)
    failIf = _deprecate(assertFalse)

    def assertSequenceEqual(self, seq1, seq2, msg=None, seq_type=None):
        """An equality assertion for ordered sequences (like lists and tuples).

        For the purposes of this function, a valid ordered sequence type is one
        which can be indexed, has a length, and has an equality operator.

        Args:
            seq1: The first sequence to compare.
            seq2: The second sequence to compare.
            seq_type: The expected datatype of the sequences, or None if no
                    datatype should be enforced.
            msg: Optional message to use on failure instead of a list of
                    differences.
        """
        if seq_type != None:
            seq_type_name = seq_type.__name__
            if not isinstance(seq1, seq_type):
                raise self.failureException('First sequence is not a %s: %r'
                                            % (seq_type_name, seq1))
            if not isinstance(seq2, seq_type):
                raise self.failureException('Second sequence is not a %s: %r'
                                            % (seq_type_name, seq2))
        else:
            seq_type_name = "sequence"

        differing = None
        try:
            len1 = len(seq1)
        except (TypeError, NotImplementedError):
            differing = 'First %s has no length.    Non-sequence?' % (
                    seq_type_name)

        if differing is None:
            try:
                len2 = len(seq2)
            except (TypeError, NotImplementedError):
                differing = 'Second %s has no length.    Non-sequence?' % (
                        seq_type_name)

        if differing is None:
            if seq1 == seq2:
                return

            seq1_repr = repr(seq1)
            seq2_repr = repr(seq2)
            if len(seq1_repr) > 30:
                seq1_repr = seq1_repr[:30] + '...'
            if len(seq2_repr) > 30:
                seq2_repr = seq2_repr[:30] + '...'
            elements = (seq_type_name.capitalize(), seq1_repr, seq2_repr)
            differing = '%ss differ: %s != %s\n' % elements

            for i in range(min(len1, len2)):
                try:
                    item1 = seq1[i]
                except (TypeError, IndexError, NotImplementedError):
                    differing += ('\nUnable to index element %d of first %s\n' %
                                 (i, seq_type_name))
                    break

                try:
                    item2 = seq2[i]
                except (TypeError, IndexError, NotImplementedError):
                    differing += ('\nUnable to index element %d of second %s\n' %
                                 (i, seq_type_name))
                    break

                if item1 != item2:
                    differing += ('\nFirst differing element %d:\n%s\n%s\n' %
                                 (i, item1, item2))
                    break
            else:
                if (len1 == len2 and seq_type is None and
                    type(seq1) != type(seq2)):
                    # The sequences are the same, but have differing types.
                    return

            if len1 > len2:
                differing += ('\nFirst %s contains %d additional '
                             'elements.\n' % (seq_type_name, len1 - len2))
                try:
                    differing += ('First extra element %d:\n%s\n' %
                                  (len2, seq1[len2]))
                except (TypeError, IndexError, NotImplementedError):
                    differing += ('Unable to index element %d '
                                  'of first %s\n' % (len2, seq_type_name))
            elif len1 < len2:
                differing += ('\nSecond %s contains %d additional '
                             'elements.\n' % (seq_type_name, len2 - len1))
                try:
                    differing += ('First extra element %d:\n%s\n' %
                                  (len1, seq2[len1]))
                except (TypeError, IndexError, NotImplementedError):
                    differing += ('Unable to index element %d '
                                  'of second %s\n' % (len1, seq_type_name))
        standardMsg = differing + '\n' + '\n'.join(difflib.ndiff(pprint.pformat(seq1).splitlines(),
                                            pprint.pformat(seq2).splitlines()))
        msg = self._formatMessage(msg, standardMsg)
        self.fail(msg)

    def assertListEqual(self, list1, list2, msg=None):
        """A list-specific equality assertion.

        Args:
            list1: The first list to compare.
            list2: The second list to compare.
            msg: Optional message to use on failure instead of a list of
                    differences.

        """
        self.assertSequenceEqual(list1, list2, msg, seq_type=list)

    def assertTupleEqual(self, tuple1, tuple2, msg=None):
        """A tuple-specific equality assertion.

        Args:
            tuple1: The first tuple to compare.
            tuple2: The second tuple to compare.
            msg: Optional message to use on failure instead of a list of
                    differences.
        """
        self.assertSequenceEqual(tuple1, tuple2, msg, seq_type=tuple)

    def assertSetEqual(self, set1, set2, msg=None):
        """A set-specific equality assertion.

        Args:
            set1: The first set to compare.
            set2: The second set to compare.
            msg: Optional message to use on failure instead of a list of
                    differences.

        For more general containership equality, assertSameElements will work
        with things other than sets.    This uses ducktyping to support
        different types of sets, and is optimized for sets specifically
        (parameters must support a difference method).
        """
        try:
            difference1 = set1.difference(set2)
        except TypeError as e:
            self.fail('invalid type when attempting set difference: %s' % e)
        except AttributeError as e:
            self.fail('first argument does not support set difference: %s' % e)

        try:
            difference2 = set2.difference(set1)
        except TypeError as e:
            self.fail('invalid type when attempting set difference: %s' % e)
        except AttributeError as e:
            self.fail('second argument does not support set difference: %s' % e)

        if not (difference1 or difference2):
            return

        lines = []
        if difference1:
            lines.append('Items in the first set but not the second:')
            for item in difference1:
                lines.append(repr(item))
        if difference2:
            lines.append('Items in the second set but not the first:')
            for item in difference2:
                lines.append(repr(item))

        standardMsg = '\n'.join(lines)
        self.fail(self._formatMessage(msg, standardMsg))

    def assertIn(self, member, container, msg=None):
        """Just like self.assertTrue(a in b), but with a nicer default message."""
        if member not in container:
            standardMsg = '%r not found in %r' % (member, container)
            self.fail(self._formatMessage(msg, standardMsg))

    def assertNotIn(self, member, container, msg=None):
        """Just like self.assertTrue(a not in b), but with a nicer default message."""
        if member in container:
            standardMsg = '%r unexpectedly found in %r' % (member, container)
            self.fail(self._formatMessage(msg, standardMsg))

    def assertIs(self, expr1, expr2, msg=None):
        """Just like self.assertTrue(a is b), but with a nicer default message."""
        if expr1 is not expr2:
            standardMsg = '%r is not %r' % (expr1, expr2)
            self.fail(self._formatMessage(msg, standardMsg))

    def assertIsNot(self, expr1, expr2, msg=None):
        """Just like self.assertTrue(a is not b), but with a nicer default message."""
        if expr1 is expr2:
            standardMsg = 'unexpectedly identical: %r' % (expr1,)
            self.fail(self._formatMessage(msg, standardMsg))

    def assertDictEqual(self, d1, d2, msg=None):
        self.assert_(isinstance(d1, dict), 'First argument is not a dictionary')
        self.assert_(isinstance(d2, dict), 'Second argument is not a dictionary')

        if d1 != d2:
            standardMsg = ('\n' + '\n'.join(difflib.ndiff(
                           pprint.pformat(d1).splitlines(),
                           pprint.pformat(d2).splitlines())))
            self.fail(self._formatMessage(msg, standardMsg))

    def assertDictContainsSubset(self, expected, actual, msg=None):
        """Checks whether actual is a superset of expected."""
        missing = []
        mismatched = []
        for key, value in expected.items():
            if key not in actual:
                missing.append(key)
            elif value != actual[key]:
                mismatched.append('%s, expected: %s, actual: %s' % (key, value,                                                                                                       actual[key]))

        if not (missing or mismatched):
            return

        standardMsg = ''
        if missing:
            standardMsg = 'Missing: %r' % ','.join(missing)
        if mismatched:
            if standardMsg:
                standardMsg += '; '
            standardMsg += 'Mismatched values: %s' % ','.join(mismatched)

        self.fail(self._formatMessage(msg, standardMsg))

    def assertSameElements(self, expected_seq, actual_seq, msg=None):
        """An unordered sequence specific comparison.

        Raises with an error message listing which elements of expected_seq
        are missing from actual_seq and vice versa if any.
        """
        try:
            expected = set(expected_seq)
            actual = set(actual_seq)
            missing = list(expected.difference(actual))
            unexpected = list(actual.difference(expected))
            missing.sort()
            unexpected.sort()
        except TypeError:
            # Fall back to slower list-compare if any of the objects are
            # not hashable.
            expected = list(expected_seq)
            actual = list(actual_seq)
            try:
                expected.sort()
                actual.sort()
            except TypeError:
                missing, unexpected = _UnorderableListDifference(expected, actual)
            else:
                missing, unexpected = _SortedListDifference(expected, actual)
        errors = []
        if missing:
            errors.append('Expected, but missing:\n    %r' % missing)
        if unexpected:
            errors.append('Unexpected, but present:\n    %r' % unexpected)
        if errors:
            standardMsg = '\n'.join(errors)
            self.fail(self._formatMessage(msg, standardMsg))

    def assertMultiLineEqual(self, first, second, msg=None):
        """Assert that two multi-line strings are equal."""
        self.assert_(isinstance(first, str), (
                'First argument is not a string'))
        self.assert_(isinstance(second, str), (
                'Second argument is not a string'))

        if first != second:
            standardMsg = '\n' + ''.join(difflib.ndiff(first.splitlines(True), second.splitlines(True)))
            self.fail(self._formatMessage(msg, standardMsg))

    def assertLess(self, a, b, msg=None):
        """Just like self.assertTrue(a < b), but with a nicer default message."""
        if not a < b:
            standardMsg = '%r not less than %r' % (a, b)
            self.fail(self._formatMessage(msg, standardMsg))

    def assertLessEqual(self, a, b, msg=None):
        """Just like self.assertTrue(a <= b), but with a nicer default message."""
        if not a <= b:
            standardMsg = '%r not less than or equal to %r' % (a, b)
            self.fail(self._formatMessage(msg, standardMsg))

    def assertGreater(self, a, b, msg=None):
        """Just like self.assertTrue(a > b), but with a nicer default message."""
        if not a > b:
            standardMsg = '%r not greater than %r' % (a, b)
            self.fail(self._formatMessage(msg, standardMsg))

    def assertGreaterEqual(self, a, b, msg=None):
        """Just like self.assertTrue(a >= b), but with a nicer default message."""
        if not a >= b:
            standardMsg = '%r not greater than or equal to %r' % (a, b)
            self.fail(self._formatMessage(msg, standardMsg))

    def assertIsNone(self, obj, msg=None):
        """Same as self.assertTrue(obj is None), with a nicer default message."""
        if obj is not None:
            standardMsg = '%r is not None' % obj
            self.fail(self._formatMessage(msg, standardMsg))

    def assertIsNotNone(self, obj, msg=None):
        """Included for symmetry with assertIsNone."""
        if obj is None:
            standardMsg = 'unexpectedly None'
            self.fail(self._formatMessage(msg, standardMsg))

    def assertRaisesRegexp(self, expected_exception, expected_regexp,
                           callable_obj=None, *args, **kwargs):
        """Asserts that the message in a raised exception matches a regexp.

        Args:
            expected_exception: Exception class expected to be raised.
            expected_regexp: Regexp (re pattern object or string) expected
                    to be found in error message.
            callable_obj: Function to be called.
            args: Extra args.
            kwargs: Extra kwargs.
        """
        context = _AssertRaisesContext(expected_exception, self, callable_obj,
                                       expected_regexp)
        if callable_obj is None:
            return context
        with context:
            callable_obj(*args, **kwargs)

    def assertRegexpMatches(self, text, expected_regex, msg=None):
        if isinstance(expected_regex, (str, bytes)):
            expected_regex = re.compile(expected_regex)
        if not expected_regex.search(text):
            msg = msg or "Regexp didn't match"
            msg = '%s: %r not found in %r' % (msg, expected_regex.pattern, text)
            raise self.failureException(msg)


def _SortedListDifference(expected, actual):
    """Finds elements in only one or the other of two, sorted input lists.

    Returns a two-element tuple of lists.    The first list contains those
    elements in the "expected" list but not in the "actual" list, and the
    second contains those elements in the "actual" list but not in the
    "expected" list.    Duplicate elements in either input list are ignored.
    """
    i = j = 0
    missing = []
    unexpected = []
    while True:
        try:
            e = expected[i]
            a = actual[j]
            if e < a:
                missing.append(e)
                i += 1
                while expected[i] == e:
                    i += 1
            elif e > a:
                unexpected.append(a)
                j += 1
                while actual[j] == a:
                    j += 1
            else:
                i += 1
                try:
                    while expected[i] == e:
                        i += 1
                finally:
                    j += 1
                    while actual[j] == a:
                        j += 1
        except IndexError:
            missing.extend(expected[i:])
            unexpected.extend(actual[j:])
            break
    return missing, unexpected

def _UnorderableListDifference(expected, actual):
    """Same behavior as _SortedListDifference but
    for lists of unorderable items (like dicts).

    As it does a linear search per item (remove) it
    has O(n*n) performance."""
    missing = []
    while expected:
        item = expected.pop()
        try:
            actual.remove(item)
        except ValueError:
            missing.append(item)

    # anything left in actual is unexpected
    return missing, actual

class TestSuite(object):
    """A test suite is a composite test consisting of a number of TestCases.

    For use, create an instance of TestSuite, then add test case instances.
    When all tests have been added, the suite can be passed to a test
    runner, such as TextTestRunner. It will run the individual test cases
    in the order in which they were added, aggregating the results. When
    subclassing, do not forget to call the base class constructor.
    """
    def __init__(self, tests=()):
        self._tests = []
        self.addTests(tests)

    def __repr__(self):
        return "<%s tests=%s>" % (_strclass(self.__class__), list(self))

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            return NotImplemented
        return list(self) == list(other)

    def __ne__(self, other):
        return not self == other

    def __iter__(self):
        return iter(self._tests)

    def countTestCases(self):
        cases = 0
        for test in self:
            cases += test.countTestCases()
        return cases

    def addTest(self, test):
        # sanity checks
        if not hasattr(test, '__call__'):
            raise TypeError("the test to add must be callable")
        if isinstance(test, type) and issubclass(test, (TestCase, TestSuite)):
            raise TypeError("TestCases and TestSuites must be instantiated "
                            "before passing them to addTest()")
        self._tests.append(test)

    def addTests(self, tests):
        if isinstance(tests, str):
            raise TypeError("tests must be an iterable of tests, not a string")
        for test in tests:
            self.addTest(test)

    def run(self, result):
        for test in self:
            if result.shouldStop:
                break
            test(result)
        return result

    def __call__(self, *args, **kwds):
        return self.run(*args, **kwds)

    def debug(self):
        """Run the tests without collecting errors in a TestResult"""
        for test in self:
            test.debug()


class FunctionTestCase(TestCase):
    """A test case that wraps a test function.

    This is useful for slipping pre-existing test functions into the
    unittest framework. Optionally, set-up and tidy-up functions can be
    supplied. As with TestCase, the tidy-up ('tearDown') function will
    always be called if the set-up ('setUp') function ran successfully.
    """

    def __init__(self, testFunc, setUp=None, tearDown=None, description=None):
        super(FunctionTestCase, self).__init__()
        self._setUpFunc = setUp
        self._tearDownFunc = tearDown
        self._testFunc = testFunc
        self._description = description

    def setUp(self):
        if self._setUpFunc is not None:
            self._setUpFunc()

    def tearDown(self):
        if self._tearDownFunc is not None:
            self._tearDownFunc()

    def runTest(self):
        self._testFunc()

    def id(self):
        return self._testFunc.__name__

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            return NotImplemented

        return self._setUpFunc == other._setUpFunc and \
               self._tearDownFunc == other._tearDownFunc and \
               self._testFunc == other._testFunc and \
               self._description == other._description

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash((type(self), self._setUpFunc, self._tearDownFunc,
                     self._testFunc, self._description))

    def __str__(self):
        return "%s (%s)" % (_strclass(self.__class__), self._testFunc.__name__)

    def __repr__(self):
        return "<%s testFunc=%s>" % (_strclass(self.__class__), self._testFunc)

    def shortDescription(self):
        if self._description is not None:
            return self._description
        doc = self._testFunc.__doc__
        return doc and doc.split("\n")[0].strip() or None



##############################################################################
# Locating and loading tests
##############################################################################

def CmpToKey(mycmp):
    'Convert a cmp= function into a key= function'
    class K(object):
        def __init__(self, obj, *args):
            self.obj = obj
        def __lt__(self, other):
            return mycmp(self.obj, other.obj) == -1
    return K

def three_way_cmp(x, y):
    """Return -1 if x < y, 0 if x == y and 1 if x > y"""
    return (x > y) - (x < y)

class TestLoader(object):
    """
    This class is responsible for loading tests according to various criteria
    and returning them wrapped in a TestSuite
    """
    testMethodPrefix = 'test'
    sortTestMethodsUsing = staticmethod(three_way_cmp)
    suiteClass = TestSuite

    def loadTestsFromTestCase(self, testCaseClass):
        """Return a suite of all tests cases contained in testCaseClass"""
        if issubclass(testCaseClass, TestSuite):
            raise TypeError("Test cases should not be derived from TestSuite." \
                                " Maybe you meant to derive from TestCase?")
        testCaseNames = self.getTestCaseNames(testCaseClass)
        if not testCaseNames and hasattr(testCaseClass, 'runTest'):
            testCaseNames = ['runTest']
        suite = self.suiteClass(map(testCaseClass, testCaseNames))
        return suite

    def loadTestsFromModule(self, module):
        """Return a suite of all tests cases contained in the given module"""
        tests = []
        for name in dir(module):
            obj = getattr(module, name)
            if isinstance(obj, type) and issubclass(obj, TestCase):
                tests.append(self.loadTestsFromTestCase(obj))
        return self.suiteClass(tests)

    def loadTestsFromName(self, name, module=None):
        """Return a suite of all tests cases given a string specifier.

        The name may resolve either to a module, a test case class, a
        test method within a test case class, or a callable object which
        returns a TestCase or TestSuite instance.

        The method optionally resolves the names relative to a given module.
        """
        parts = name.split('.')
        if module is None:
            parts_copy = parts[:]
            while parts_copy:
                try:
                    module = __import__('.'.join(parts_copy))
                    break
                except ImportError:
                    del parts_copy[-1]
                    if not parts_copy:
                        raise
            parts = parts[1:]
        obj = module
        for part in parts:
            parent, obj = obj, getattr(obj, part)

        if isinstance(obj, types.ModuleType):
            return self.loadTestsFromModule(obj)
        elif isinstance(obj, type) and issubclass(obj, TestCase):
            return self.loadTestsFromTestCase(obj)
        elif (isinstance(obj, types.FunctionType) and
              isinstance(parent, type) and
              issubclass(parent, TestCase)):
            name = obj.__name__
            inst = parent(name)
            # static methods follow a different path
            if not isinstance(getattr(inst, name), types.FunctionType):
                return TestSuite([inst])
        elif isinstance(obj, TestSuite):
            return obj

        if hasattr(obj, '__call__'):
            test = obj()
            if isinstance(test, TestSuite):
                return test
            elif isinstance(test, TestCase):
                return TestSuite([test])
            else:
                raise TypeError("calling %s returned %s, not a test" %
                                (obj, test))
        else:
            raise TypeError("don't know how to make test from: %s" % obj)

    def loadTestsFromNames(self, names, module=None):
        """Return a suite of all tests cases found using the given sequence
        of string specifiers. See 'loadTestsFromName()'.
        """
        suites = [self.loadTestsFromName(name, module) for name in names]
        return self.suiteClass(suites)

    def getTestCaseNames(self, testCaseClass):
        """Return a sorted sequence of method names found within testCaseClass
        """
        def isTestMethod(attrname, testCaseClass=testCaseClass,
                         prefix=self.testMethodPrefix):
            return attrname.startswith(prefix) and \
                hasattr(getattr(testCaseClass, attrname), '__call__')
        testFnNames = list(filter(isTestMethod, dir(testCaseClass)))
        if self.sortTestMethodsUsing:
            testFnNames.sort(key=CmpToKey(self.sortTestMethodsUsing))
        return testFnNames



defaultTestLoader = TestLoader()


##############################################################################
# Patches for old functions: these functions should be considered obsolete
##############################################################################

def _makeLoader(prefix, sortUsing, suiteClass=None):
    loader = TestLoader()
    loader.sortTestMethodsUsing = sortUsing
    loader.testMethodPrefix = prefix
    if suiteClass: loader.suiteClass = suiteClass
    return loader

def getTestCaseNames(testCaseClass, prefix, sortUsing=three_way_cmp):
    return _makeLoader(prefix, sortUsing).getTestCaseNames(testCaseClass)

def makeSuite(testCaseClass, prefix='test', sortUsing=three_way_cmp,
              suiteClass=TestSuite):
    return _makeLoader(prefix, sortUsing, suiteClass).loadTestsFromTestCase(
        testCaseClass)

def findTestCases(module, prefix='test', sortUsing=three_way_cmp,
                  suiteClass=TestSuite):
    return _makeLoader(prefix, sortUsing, suiteClass).loadTestsFromModule(
        module)


##############################################################################
# Text UI
##############################################################################

class _WritelnDecorator(object):
    """Used to decorate file-like objects with a handy 'writeln' method"""
    def __init__(self,stream):
        self.stream = stream

    def __getattr__(self, attr):
        return getattr(self.stream,attr)

    def writeln(self, arg=None):
        if arg:
            self.write(arg)
        self.write('\n') # text-mode streams translate to \r\n if needed


class _TextTestResult(TestResult):
    """A test result class that can print formatted text results to a stream.

    Used by TextTestRunner.
    """
    separator1 = '=' * 70
    separator2 = '-' * 70

    def __init__(self, stream, descriptions, verbosity):
        super(_TextTestResult, self).__init__()
        self.stream = stream
        self.showAll = verbosity > 1
        self.dots = verbosity == 1
        self.descriptions = descriptions

    def getDescription(self, test):
        if self.descriptions:
            return test.shortDescription() or str(test)
        else:
            return str(test)

    def startTest(self, test):
        super(_TextTestResult, self).startTest(test)
        if self.showAll:
            self.stream.write(self.getDescription(test))
            self.stream.write(" ... ")
            self.stream.flush()

    def addSuccess(self, test):
        super(_TextTestResult, self).addSuccess(test)
        if self.showAll:
            self.stream.writeln("ok")
        elif self.dots:
            self.stream.write('.')
            self.stream.flush()

    def addError(self, test, err):
        super(_TextTestResult, self).addError(test, err)
        if self.showAll:
            self.stream.writeln("ERROR")
        elif self.dots:
            self.stream.write('E')
            self.stream.flush()

    def addFailure(self, test, err):
        super(_TextTestResult, self).addFailure(test, err)
        if self.showAll:
            self.stream.writeln("FAIL")
        elif self.dots:
            self.stream.write('F')
            self.stream.flush()

    def addSkip(self, test, reason):
        super(_TextTestResult, self).addSkip(test, reason)
        if self.showAll:
            self.stream.writeln("skipped {0!r}".format(reason))
        elif self.dots:
            self.stream.write("s")
            self.stream.flush()

    def addExpectedFailure(self, test, err):
        super(_TextTestResult, self).addExpectedFailure(test, err)
        if self.showAll:
            self.stream.writeln("expected failure")
        elif self.dots:
            self.stream.write("x")
            self.stream.flush()

    def addUnexpectedSuccess(self, test):
        super(_TextTestResult, self).addUnexpectedSuccess(test)
        if self.showAll:
            self.stream.writeln("unexpected success")
        elif self.dots:
            self.stream.write("u")
            self.stream.flush()

    def printErrors(self):
        if self.dots or self.showAll:
            self.stream.writeln()
        self.printErrorList('ERROR', self.errors)
        self.printErrorList('FAIL', self.failures)

    def printErrorList(self, flavour, errors):
        for test, err in errors:
            self.stream.writeln(self.separator1)
            self.stream.writeln("%s: %s" % (flavour,self.getDescription(test)))
            self.stream.writeln(self.separator2)
            self.stream.writeln("%s" % err)


class TextTestRunner(object):
    """A test runner class that displays results in textual form.

    It prints out the names of tests as they are run, errors as they
    occur, and a summary of the results at the end of the test run.
    """
    def __init__(self, stream=sys.stderr, descriptions=1, verbosity=1):
        self.stream = _WritelnDecorator(stream)
        self.descriptions = descriptions
        self.verbosity = verbosity

    def _makeResult(self):
        return _TextTestResult(self.stream, self.descriptions, self.verbosity)

    def run(self, test):
        "Run the given test case or test suite."
        result = self._makeResult()
        startTime = time.time()
        startTestRun = getattr(result, 'startTestRun', None)
        if startTestRun is not None:
            startTestRun()
        try:
            test(result)
        finally:
            stopTestRun = getattr(result, 'stopTestRun', None)
            if stopTestRun is not None:
                stopTestRun()
        stopTime = time.time()
        timeTaken = stopTime - startTime
        result.printErrors()
        self.stream.writeln(result.separator2)
        run = result.testsRun
        self.stream.writeln("Ran %d test%s in %.3fs" %
                            (run, run != 1 and "s" or "", timeTaken))
        self.stream.writeln()
        results = map(len, (result.expectedFailures,
                            result.unexpectedSuccesses,
                            result.skipped))
        expectedFails, unexpectedSuccesses, skipped = results
        infos = []
        if not result.wasSuccessful():
            self.stream.write("FAILED")
            failed, errored = len(result.failures), len(result.errors)
            if failed:
                infos.append("failures=%d" % failed)
            if errored:
                infos.append("errors=%d" % errored)
        else:
            self.stream.write("OK")
        if skipped:
            infos.append("skipped=%d" % skipped)
        if expectedFails:
            infos.append("expected failures=%d" % expectedFails)
        if unexpectedSuccesses:
            infos.append("unexpected successes=%d" % unexpectedSuccesses)
        if infos:
            self.stream.writeln(" (%s)" % (", ".join(infos),))
        else:
            self.stream.write("\n")
        return result



##############################################################################
# Facilities for running tests from the command line
##############################################################################

class TestProgram(object):
    """A command-line program that runs a set of tests; this is primarily
       for making test modules conveniently executable.
    """
    USAGE = """\
Usage: %(progName)s [options] [test] [...]

Options:
  -h, --help       Show this message
  -v, --verbose    Verbose output
  -q, --quiet      Minimal output

Examples:
  %(progName)s                               - run default set of tests
  %(progName)s MyTestSuite                   - run suite 'MyTestSuite'
  %(progName)s MyTestCase.testSomething      - run MyTestCase.testSomething
  %(progName)s MyTestCase                    - run all 'test*' test methods
                                               in MyTestCase
"""
    def __init__(self, module='__main__', defaultTest=None,
                 argv=None, testRunner=TextTestRunner,
                 testLoader=defaultTestLoader, exit=True):
        if isinstance(module, str):
            self.module = __import__(module)
            for part in module.split('.')[1:]:
                self.module = getattr(self.module, part)
        else:
            self.module = module
        if argv is None:
            argv = sys.argv

        self.exit = exit
        self.verbosity = 1
        self.defaultTest = defaultTest
        self.testRunner = testRunner
        self.testLoader = testLoader
        self.progName = os.path.basename(argv[0])
        self.parseArgs(argv)
        self.runTests()

    def usageExit(self, msg=None):
        if msg:
            print(msg)
        print(self.USAGE % self.__dict__)
        sys.exit(2)

    def parseArgs(self, argv):
        import getopt
        long_opts = ['help','verbose','quiet']
        try:
            options, args = getopt.getopt(argv[1:], 'hHvq', long_opts)
            for opt, value in options:
                if opt in ('-h','-H','--help'):
                    self.usageExit()
                if opt in ('-q','--quiet'):
                    self.verbosity = 0
                if opt in ('-v','--verbose'):
                    self.verbosity = 2
            if len(args) == 0 and self.defaultTest is None:
                self.test = self.testLoader.loadTestsFromModule(self.module)
                return
            if len(args) > 0:
                self.testNames = args
            else:
                self.testNames = (self.defaultTest,)
            self.createTests()
        except getopt.error as msg:
            self.usageExit(msg)

    def createTests(self):
        self.test = self.testLoader.loadTestsFromNames(self.testNames,
                                                       self.module)

    def runTests(self):
        if isinstance(self.testRunner, type):
            try:
                testRunner = self.testRunner(verbosity=self.verbosity)
            except TypeError:
                # didn't accept the verbosity argument
                testRunner = self.testRunner()
        else:
            # it is assumed to be a TestRunner instance
            testRunner = self.testRunner
        self.result = testRunner.run(self.test)
        if self.exit:
            sys.exit(not self.result.wasSuccessful())

main = TestProgram


##############################################################################
# Executing this module from the command line
##############################################################################

if __name__ == "__main__":
    main(module=None)
