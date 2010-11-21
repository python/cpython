"""Test script for unittest.

By Collin Winter <collinw at gmail.com>

Still need testing:
    TestCase.{assert,fail}* methods (some are tested implicitly)
"""

import re
from test import support
import unittest
from unittest import TestCase, TestProgram
import types
from copy import deepcopy
import io

### Support code
################################################################

class LoggingResult(unittest.TestResult):
    def __init__(self, log):
        self._events = log
        super().__init__()

    def startTest(self, test):
        self._events.append('startTest')
        super().startTest(test)

    def startTestRun(self):
        self._events.append('startTestRun')
        super(LoggingResult, self).startTestRun()

    def stopTest(self, test):
        self._events.append('stopTest')
        super().stopTest(test)

    def stopTestRun(self):
        self._events.append('stopTestRun')
        super(LoggingResult, self).stopTestRun()

    def addFailure(self, *args):
        self._events.append('addFailure')
        super().addFailure(*args)

    def addSuccess(self, *args):
        self._events.append('addSuccess')
        super(LoggingResult, self).addSuccess(*args)

    def addError(self, *args):
        self._events.append('addError')
        super().addError(*args)

    def addSkip(self, *args):
        self._events.append('addSkip')
        super(LoggingResult, self).addSkip(*args)

    def addExpectedFailure(self, *args):
        self._events.append('addExpectedFailure')
        super(LoggingResult, self).addExpectedFailure(*args)

    def addUnexpectedSuccess(self, *args):
        self._events.append('addUnexpectedSuccess')
        super(LoggingResult, self).addUnexpectedSuccess(*args)


class TestEquality(object):
    """Used as a mixin for TestCase"""

    # Check for a valid __eq__ implementation
    def test_eq(self):
        for obj_1, obj_2 in self.eq_pairs:
            self.assertEqual(obj_1, obj_2)
            self.assertEqual(obj_2, obj_1)

    # Check for a valid __ne__ implementation
    def test_ne(self):
        for obj_1, obj_2 in self.ne_pairs:
            self.assertNotEqual(obj_1, obj_2)
            self.assertNotEqual(obj_2, obj_1)

class TestHashing(object):
    """Used as a mixin for TestCase"""

    # Check for a valid __hash__ implementation
    def test_hash(self):
        for obj_1, obj_2 in self.eq_pairs:
            try:
                if not hash(obj_1) == hash(obj_2):
                    self.fail("%r and %r do not hash equal" % (obj_1, obj_2))
            except KeyboardInterrupt:
                raise
            except Exception as e:
                self.fail("Problem hashing %r and %r: %s" % (obj_1, obj_2, e))

        for obj_1, obj_2 in self.ne_pairs:
            try:
                if hash(obj_1) == hash(obj_2):
                    self.fail("%s and %s hash equal, but shouldn't" %
                              (obj_1, obj_2))
            except KeyboardInterrupt:
                raise
            except Exception as e:
                self.fail("Problem hashing %s and %s: %s" % (obj_1, obj_2, e))


# List subclass we can add attributes to.
class MyClassSuite(list):

    def __init__(self, tests):
        super(MyClassSuite, self).__init__(tests)


################################################################
### /Support code

class Test_TestLoader(TestCase):

    ### Tests for TestLoader.loadTestsFromTestCase
    ################################################################

    # "Return a suite of all tests cases contained in the TestCase-derived
    # class testCaseClass"
    def test_loadTestsFromTestCase(self):
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
            def foo_bar(self): pass

        tests = unittest.TestSuite([Foo('test_1'), Foo('test_2')])

        loader = unittest.TestLoader()
        self.assertEqual(loader.loadTestsFromTestCase(Foo), tests)

    # "Return a suite of all tests cases contained in the TestCase-derived
    # class testCaseClass"
    #
    # Make sure it does the right thing even if no tests were found
    def test_loadTestsFromTestCase__no_matches(self):
        class Foo(unittest.TestCase):
            def foo_bar(self): pass

        empty_suite = unittest.TestSuite()

        loader = unittest.TestLoader()
        self.assertEqual(loader.loadTestsFromTestCase(Foo), empty_suite)

    # "Return a suite of all tests cases contained in the TestCase-derived
    # class testCaseClass"
    #
    # What happens if loadTestsFromTestCase() is given an object
    # that isn't a subclass of TestCase? Specifically, what happens
    # if testCaseClass is a subclass of TestSuite?
    #
    # This is checked for specifically in the code, so we better add a
    # test for it.
    def test_loadTestsFromTestCase__TestSuite_subclass(self):
        class NotATestCase(unittest.TestSuite):
            pass

        loader = unittest.TestLoader()
        try:
            loader.loadTestsFromTestCase(NotATestCase)
        except TypeError:
            pass
        else:
            self.fail('Should raise TypeError')

    # "Return a suite of all tests cases contained in the TestCase-derived
    # class testCaseClass"
    #
    # Make sure loadTestsFromTestCase() picks up the default test method
    # name (as specified by TestCase), even though the method name does
    # not match the default TestLoader.testMethodPrefix string
    def test_loadTestsFromTestCase__default_method_name(self):
        class Foo(unittest.TestCase):
            def runTest(self):
                pass

        loader = unittest.TestLoader()
        # This has to be false for the test to succeed
        self.assertFalse('runTest'.startswith(loader.testMethodPrefix))

        suite = loader.loadTestsFromTestCase(Foo)
        self.assertTrue(isinstance(suite, loader.suiteClass))
        self.assertEqual(list(suite), [Foo('runTest')])

    ################################################################
    ### /Tests for TestLoader.loadTestsFromTestCase

    ### Tests for TestLoader.loadTestsFromModule
    ################################################################

    # "This method searches `module` for classes derived from TestCase"
    def test_loadTestsFromModule__TestCase_subclass(self):
        m = types.ModuleType('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromModule(m)
        self.assertTrue(isinstance(suite, loader.suiteClass))

        expected = [loader.suiteClass([MyTestCase('test')])]
        self.assertEqual(list(suite), expected)

    # "This method searches `module` for classes derived from TestCase"
    #
    # What happens if no tests are found (no TestCase instances)?
    def test_loadTestsFromModule__no_TestCase_instances(self):
        m = types.ModuleType('m')

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromModule(m)
        self.assertTrue(isinstance(suite, loader.suiteClass))
        self.assertEqual(list(suite), [])

    # "This method searches `module` for classes derived from TestCase"
    #
    # What happens if no tests are found (TestCases instances, but no tests)?
    def test_loadTestsFromModule__no_TestCase_tests(self):
        m = types.ModuleType('m')
        class MyTestCase(unittest.TestCase):
            pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromModule(m)
        self.assertTrue(isinstance(suite, loader.suiteClass))

        self.assertEqual(list(suite), [loader.suiteClass()])

    # "This method searches `module` for classes derived from TestCase"s
    #
    # What happens if loadTestsFromModule() is given something other
    # than a module?
    #
    # XXX Currently, it succeeds anyway. This flexibility
    # should either be documented or loadTestsFromModule() should
    # raise a TypeError
    #
    # XXX Certain people are using this behaviour. We'll add a test for it
    def test_loadTestsFromModule__not_a_module(self):
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass

        class NotAModule(object):
            test_2 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromModule(NotAModule)

        reference = [unittest.TestSuite([MyTestCase('test')])]
        self.assertEqual(list(suite), reference)

    ################################################################
    ### /Tests for TestLoader.loadTestsFromModule()

    ### Tests for TestLoader.loadTestsFromName()
    ################################################################

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    #
    # Is ValueError raised in response to an empty name?
    def test_loadTestsFromName__empty_name(self):
        loader = unittest.TestLoader()

        try:
            loader.loadTestsFromName('')
        except ValueError as e:
            self.assertEqual(str(e), "Empty module name")
        else:
            self.fail("TestLoader.loadTestsFromName failed to raise ValueError")

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    #
    # What happens when the name contains invalid characters?
    def test_loadTestsFromName__malformed_name(self):
        loader = unittest.TestLoader()

        # XXX Should this raise ValueError or ImportError?
        try:
            loader.loadTestsFromName('abc () //')
        except ValueError:
            pass
        except ImportError:
            pass
        else:
            self.fail("TestLoader.loadTestsFromName failed to raise ValueError")

    # "The specifier name is a ``dotted name'' that may resolve ... to a
    # module"
    #
    # What happens when a module by that name can't be found?
    def test_loadTestsFromName__unknown_module_name(self):
        loader = unittest.TestLoader()

        try:
            loader.loadTestsFromName('sdasfasfasdf')
        except ImportError as e:
            self.assertEqual(str(e), "No module named sdasfasfasdf")
        else:
            self.fail("TestLoader.loadTestsFromName failed to raise ImportError")

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    #
    # What happens when the module is found, but the attribute can't?
    def test_loadTestsFromName__unknown_attr_name(self):
        loader = unittest.TestLoader()

        try:
            loader.loadTestsFromName('unittest.sdasfasfasdf')
        except AttributeError as e:
            self.assertEqual(str(e), "'module' object has no attribute 'sdasfasfasdf'")
        else:
            self.fail("TestLoader.loadTestsFromName failed to raise AttributeError")

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    #
    # What happens when we provide the module, but the attribute can't be
    # found?
    def test_loadTestsFromName__relative_unknown_name(self):
        loader = unittest.TestLoader()

        try:
            loader.loadTestsFromName('sdasfasfasdf', unittest)
        except AttributeError as e:
            self.assertEqual(str(e), "'module' object has no attribute 'sdasfasfasdf'")
        else:
            self.fail("TestLoader.loadTestsFromName failed to raise AttributeError")

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    # ...
    # "The method optionally resolves name relative to the given module"
    #
    # Does loadTestsFromName raise ValueError when passed an empty
    # name relative to a provided module?
    #
    # XXX Should probably raise a ValueError instead of an AttributeError
    def test_loadTestsFromName__relative_empty_name(self):
        loader = unittest.TestLoader()

        try:
            loader.loadTestsFromName('', unittest)
        except AttributeError as e:
            pass
        else:
            self.fail("Failed to raise AttributeError")

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    # ...
    # "The method optionally resolves name relative to the given module"
    #
    # What happens when an impossible name is given, relative to the provided
    # `module`?
    def test_loadTestsFromName__relative_malformed_name(self):
        loader = unittest.TestLoader()

        # XXX Should this raise AttributeError or ValueError?
        try:
            loader.loadTestsFromName('abc () //', unittest)
        except ValueError:
            pass
        except AttributeError:
            pass
        else:
            self.fail("TestLoader.loadTestsFromName failed to raise ValueError")

    # "The method optionally resolves name relative to the given module"
    #
    # Does loadTestsFromName raise TypeError when the `module` argument
    # isn't a module object?
    #
    # XXX Accepts the not-a-module object, ignorning the object's type
    # This should raise an exception or the method name should be changed
    #
    # XXX Some people are relying on this, so keep it for now
    def test_loadTestsFromName__relative_not_a_module(self):
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass

        class NotAModule(object):
            test_2 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromName('test_2', NotAModule)

        reference = [MyTestCase('test')]
        self.assertEqual(list(suite), reference)

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    #
    # Does it raise an exception if the name resolves to an invalid
    # object?
    def test_loadTestsFromName__relative_bad_object(self):
        m = types.ModuleType('m')
        m.testcase_1 = object()

        loader = unittest.TestLoader()
        try:
            loader.loadTestsFromName('testcase_1', m)
        except TypeError:
            pass
        else:
            self.fail("Should have raised TypeError")

    # "The specifier name is a ``dotted name'' that may
    # resolve either to ... a test case class"
    def test_loadTestsFromName__relative_TestCase_subclass(self):
        m = types.ModuleType('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromName('testcase_1', m)
        self.assertTrue(isinstance(suite, loader.suiteClass))
        self.assertEqual(list(suite), [MyTestCase('test')])

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    def test_loadTestsFromName__relative_TestSuite(self):
        m = types.ModuleType('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testsuite = unittest.TestSuite([MyTestCase('test')])

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromName('testsuite', m)
        self.assertTrue(isinstance(suite, loader.suiteClass))

        self.assertEqual(list(suite), [MyTestCase('test')])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a test method within a test case class"
    def test_loadTestsFromName__relative_testmethod(self):
        m = types.ModuleType('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromName('testcase_1.test', m)
        self.assertTrue(isinstance(suite, loader.suiteClass))

        self.assertEqual(list(suite), [MyTestCase('test')])

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    #
    # Does loadTestsFromName() raise the proper exception when trying to
    # resolve "a test method within a test case class" that doesn't exist
    # for the given name (relative to a provided module)?
    def test_loadTestsFromName__relative_invalid_testmethod(self):
        m = types.ModuleType('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        try:
            loader.loadTestsFromName('testcase_1.testfoo', m)
        except AttributeError as e:
            self.assertEqual(str(e), "type object 'MyTestCase' has no attribute 'testfoo'")
        else:
            self.fail("Failed to raise AttributeError")

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a callable object which returns a ... TestSuite instance"
    def test_loadTestsFromName__callable__TestSuite(self):
        m = types.ModuleType('m')
        testcase_1 = unittest.FunctionTestCase(lambda: None)
        testcase_2 = unittest.FunctionTestCase(lambda: None)
        def return_TestSuite():
            return unittest.TestSuite([testcase_1, testcase_2])
        m.return_TestSuite = return_TestSuite

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromName('return_TestSuite', m)
        self.assertTrue(isinstance(suite, loader.suiteClass))
        self.assertEqual(list(suite), [testcase_1, testcase_2])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a callable object which returns a TestCase ... instance"
    def test_loadTestsFromName__callable__TestCase_instance(self):
        m = types.ModuleType('m')
        testcase_1 = unittest.FunctionTestCase(lambda: None)
        def return_TestCase():
            return testcase_1
        m.return_TestCase = return_TestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromName('return_TestCase', m)
        self.assertTrue(isinstance(suite, loader.suiteClass))
        self.assertEqual(list(suite), [testcase_1])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a callable object which returns a TestCase or TestSuite instance"
    #
    # What happens if the callable returns something else?
    def test_loadTestsFromName__callable__wrong_type(self):
        m = types.ModuleType('m')
        def return_wrong():
            return 6
        m.return_wrong = return_wrong

        loader = unittest.TestLoader()
        try:
            suite = loader.loadTestsFromName('return_wrong', m)
        except TypeError:
            pass
        else:
            self.fail("TestLoader.loadTestsFromName failed to raise TypeError")

    # "The specifier can refer to modules and packages which have not been
    # imported; they will be imported as a side-effect"
    def test_loadTestsFromName__module_not_loaded(self):
        # We're going to try to load this module as a side-effect, so it
        # better not be loaded before we try.
        #
        # Why pick audioop? Google shows it isn't used very often, so there's
        # a good chance that it won't be imported when this test is run
        module_name = 'audioop'

        import sys
        if module_name in sys.modules:
            del sys.modules[module_name]

        loader = unittest.TestLoader()
        try:
            suite = loader.loadTestsFromName(module_name)

            self.assertTrue(isinstance(suite, loader.suiteClass))
            self.assertEqual(list(suite), [])

            # audioop should now be loaded, thanks to loadTestsFromName()
            self.assertTrue(module_name in sys.modules)
        finally:
            if module_name in sys.modules:
                del sys.modules[module_name]

    ################################################################
    ### Tests for TestLoader.loadTestsFromName()

    ### Tests for TestLoader.loadTestsFromNames()
    ################################################################

    # "Similar to loadTestsFromName(), but takes a sequence of names rather
    # than a single name."
    #
    # What happens if that sequence of names is empty?
    def test_loadTestsFromNames__empty_name_list(self):
        loader = unittest.TestLoader()

        suite = loader.loadTestsFromNames([])
        self.assertTrue(isinstance(suite, loader.suiteClass))
        self.assertEqual(list(suite), [])

    # "Similar to loadTestsFromName(), but takes a sequence of names rather
    # than a single name."
    # ...
    # "The method optionally resolves name relative to the given module"
    #
    # What happens if that sequence of names is empty?
    #
    # XXX Should this raise a ValueError or just return an empty TestSuite?
    def test_loadTestsFromNames__relative_empty_name_list(self):
        loader = unittest.TestLoader()

        suite = loader.loadTestsFromNames([], unittest)
        self.assertTrue(isinstance(suite, loader.suiteClass))
        self.assertEqual(list(suite), [])

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    #
    # Is ValueError raised in response to an empty name?
    def test_loadTestsFromNames__empty_name(self):
        loader = unittest.TestLoader()

        try:
            loader.loadTestsFromNames([''])
        except ValueError as e:
            self.assertEqual(str(e), "Empty module name")
        else:
            self.fail("TestLoader.loadTestsFromNames failed to raise ValueError")

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    #
    # What happens when presented with an impossible module name?
    def test_loadTestsFromNames__malformed_name(self):
        loader = unittest.TestLoader()

        # XXX Should this raise ValueError or ImportError?
        try:
            loader.loadTestsFromNames(['abc () //'])
        except ValueError:
            pass
        except ImportError:
            pass
        else:
            self.fail("TestLoader.loadTestsFromNames failed to raise ValueError")

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    #
    # What happens when no module can be found for the given name?
    def test_loadTestsFromNames__unknown_module_name(self):
        loader = unittest.TestLoader()

        try:
            loader.loadTestsFromNames(['sdasfasfasdf'])
        except ImportError as e:
            self.assertEqual(str(e), "No module named sdasfasfasdf")
        else:
            self.fail("TestLoader.loadTestsFromNames failed to raise ImportError")

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    #
    # What happens when the module can be found, but not the attribute?
    def test_loadTestsFromNames__unknown_attr_name(self):
        loader = unittest.TestLoader()

        try:
            loader.loadTestsFromNames(['unittest.sdasfasfasdf', 'unittest'])
        except AttributeError as e:
            self.assertEqual(str(e), "'module' object has no attribute 'sdasfasfasdf'")
        else:
            self.fail("TestLoader.loadTestsFromNames failed to raise AttributeError")

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    # ...
    # "The method optionally resolves name relative to the given module"
    #
    # What happens when given an unknown attribute on a specified `module`
    # argument?
    def test_loadTestsFromNames__unknown_name_relative_1(self):
        loader = unittest.TestLoader()

        try:
            loader.loadTestsFromNames(['sdasfasfasdf'], unittest)
        except AttributeError as e:
            self.assertEqual(str(e), "'module' object has no attribute 'sdasfasfasdf'")
        else:
            self.fail("TestLoader.loadTestsFromName failed to raise AttributeError")

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    # ...
    # "The method optionally resolves name relative to the given module"
    #
    # Do unknown attributes (relative to a provided module) still raise an
    # exception even in the presence of valid attribute names?
    def test_loadTestsFromNames__unknown_name_relative_2(self):
        loader = unittest.TestLoader()

        try:
            loader.loadTestsFromNames(['TestCase', 'sdasfasfasdf'], unittest)
        except AttributeError as e:
            self.assertEqual(str(e), "'module' object has no attribute 'sdasfasfasdf'")
        else:
            self.fail("TestLoader.loadTestsFromName failed to raise AttributeError")

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    # ...
    # "The method optionally resolves name relative to the given module"
    #
    # What happens when faced with the empty string?
    #
    # XXX This currently raises AttributeError, though ValueError is probably
    # more appropriate
    def test_loadTestsFromNames__relative_empty_name(self):
        loader = unittest.TestLoader()

        try:
            loader.loadTestsFromNames([''], unittest)
        except AttributeError:
            pass
        else:
            self.fail("Failed to raise ValueError")

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    # ...
    # "The method optionally resolves name relative to the given module"
    #
    # What happens when presented with an impossible attribute name?
    def test_loadTestsFromNames__relative_malformed_name(self):
        loader = unittest.TestLoader()

        # XXX Should this raise AttributeError or ValueError?
        try:
            loader.loadTestsFromNames(['abc () //'], unittest)
        except AttributeError:
            pass
        except ValueError:
            pass
        else:
            self.fail("TestLoader.loadTestsFromNames failed to raise ValueError")

    # "The method optionally resolves name relative to the given module"
    #
    # Does loadTestsFromNames() make sure the provided `module` is in fact
    # a module?
    #
    # XXX This validation is currently not done. This flexibility should
    # either be documented or a TypeError should be raised.
    def test_loadTestsFromNames__relative_not_a_module(self):
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass

        class NotAModule(object):
            test_2 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromNames(['test_2'], NotAModule)

        reference = [unittest.TestSuite([MyTestCase('test')])]
        self.assertEqual(list(suite), reference)

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    #
    # Does it raise an exception if the name resolves to an invalid
    # object?
    def test_loadTestsFromNames__relative_bad_object(self):
        m = types.ModuleType('m')
        m.testcase_1 = object()

        loader = unittest.TestLoader()
        try:
            loader.loadTestsFromNames(['testcase_1'], m)
        except TypeError:
            pass
        else:
            self.fail("Should have raised TypeError")

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a test case class"
    def test_loadTestsFromNames__relative_TestCase_subclass(self):
        m = types.ModuleType('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromNames(['testcase_1'], m)
        self.assertTrue(isinstance(suite, loader.suiteClass))

        expected = loader.suiteClass([MyTestCase('test')])
        self.assertEqual(list(suite), [expected])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a TestSuite instance"
    def test_loadTestsFromNames__relative_TestSuite(self):
        m = types.ModuleType('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testsuite = unittest.TestSuite([MyTestCase('test')])

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromNames(['testsuite'], m)
        self.assertTrue(isinstance(suite, loader.suiteClass))

        self.assertEqual(list(suite), [m.testsuite])

    # "The specifier name is a ``dotted name'' that may resolve ... to ... a
    # test method within a test case class"
    def test_loadTestsFromNames__relative_testmethod(self):
        m = types.ModuleType('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromNames(['testcase_1.test'], m)
        self.assertTrue(isinstance(suite, loader.suiteClass))

        ref_suite = unittest.TestSuite([MyTestCase('test')])
        self.assertEqual(list(suite), [ref_suite])

    # "The specifier name is a ``dotted name'' that may resolve ... to ... a
    # test method within a test case class"
    #
    # Does the method gracefully handle names that initially look like they
    # resolve to "a test method within a test case class" but don't?
    def test_loadTestsFromNames__relative_invalid_testmethod(self):
        m = types.ModuleType('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        try:
            loader.loadTestsFromNames(['testcase_1.testfoo'], m)
        except AttributeError as e:
            self.assertEqual(str(e), "type object 'MyTestCase' has no attribute 'testfoo'")
        else:
            self.fail("Failed to raise AttributeError")

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a callable object which returns a ... TestSuite instance"
    def test_loadTestsFromNames__callable__TestSuite(self):
        m = types.ModuleType('m')
        testcase_1 = unittest.FunctionTestCase(lambda: None)
        testcase_2 = unittest.FunctionTestCase(lambda: None)
        def return_TestSuite():
            return unittest.TestSuite([testcase_1, testcase_2])
        m.return_TestSuite = return_TestSuite

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromNames(['return_TestSuite'], m)
        self.assertTrue(isinstance(suite, loader.suiteClass))

        expected = unittest.TestSuite([testcase_1, testcase_2])
        self.assertEqual(list(suite), [expected])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a callable object which returns a TestCase ... instance"
    def test_loadTestsFromNames__callable__TestCase_instance(self):
        m = types.ModuleType('m')
        testcase_1 = unittest.FunctionTestCase(lambda: None)
        def return_TestCase():
            return testcase_1
        m.return_TestCase = return_TestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromNames(['return_TestCase'], m)
        self.assertTrue(isinstance(suite, loader.suiteClass))

        ref_suite = unittest.TestSuite([testcase_1])
        self.assertEqual(list(suite), [ref_suite])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a callable object which returns a TestCase or TestSuite instance"
    #
    # Are staticmethods handled correctly?
    def test_loadTestsFromNames__callable__call_staticmethod(self):
        m = types.ModuleType('m')
        class Test1(unittest.TestCase):
            def test(self):
                pass

        testcase_1 = Test1('test')
        class Foo(unittest.TestCase):
            @staticmethod
            def foo():
                return testcase_1
        m.Foo = Foo

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromNames(['Foo.foo'], m)
        self.assertTrue(isinstance(suite, loader.suiteClass))

        ref_suite = unittest.TestSuite([testcase_1])
        self.assertEqual(list(suite), [ref_suite])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a callable object which returns a TestCase or TestSuite instance"
    #
    # What happens when the callable returns something else?
    def test_loadTestsFromNames__callable__wrong_type(self):
        m = types.ModuleType('m')
        def return_wrong():
            return 6
        m.return_wrong = return_wrong

        loader = unittest.TestLoader()
        try:
            suite = loader.loadTestsFromNames(['return_wrong'], m)
        except TypeError:
            pass
        else:
            self.fail("TestLoader.loadTestsFromNames failed to raise TypeError")

    # "The specifier can refer to modules and packages which have not been
    # imported; they will be imported as a side-effect"
    def test_loadTestsFromNames__module_not_loaded(self):
        # We're going to try to load this module as a side-effect, so it
        # better not be loaded before we try.
        #
        # Why pick audioop? Google shows it isn't used very often, so there's
        # a good chance that it won't be imported when this test is run
        module_name = 'audioop'

        import sys
        if module_name in sys.modules:
            del sys.modules[module_name]

        loader = unittest.TestLoader()
        try:
            suite = loader.loadTestsFromNames([module_name])

            self.assertTrue(isinstance(suite, loader.suiteClass))
            self.assertEqual(list(suite), [unittest.TestSuite()])

            # audioop should now be loaded, thanks to loadTestsFromName()
            self.assertTrue(module_name in sys.modules)
        finally:
            if module_name in sys.modules:
                del sys.modules[module_name]

    ################################################################
    ### /Tests for TestLoader.loadTestsFromNames()

    ### Tests for TestLoader.getTestCaseNames()
    ################################################################

    # "Return a sorted sequence of method names found within testCaseClass"
    #
    # Test.foobar is defined to make sure getTestCaseNames() respects
    # loader.testMethodPrefix
    def test_getTestCaseNames(self):
        class Test(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
            def foobar(self): pass

        loader = unittest.TestLoader()

        self.assertEqual(loader.getTestCaseNames(Test), ['test_1', 'test_2'])

    # "Return a sorted sequence of method names found within testCaseClass"
    #
    # Does getTestCaseNames() behave appropriately if no tests are found?
    def test_getTestCaseNames__no_tests(self):
        class Test(unittest.TestCase):
            def foobar(self): pass

        loader = unittest.TestLoader()

        self.assertEqual(loader.getTestCaseNames(Test), [])

    # "Return a sorted sequence of method names found within testCaseClass"
    #
    # Are not-TestCases handled gracefully?
    #
    # XXX This should raise a TypeError, not return a list
    #
    # XXX It's too late in the 2.5 release cycle to fix this, but it should
    # probably be revisited for 2.6
    def test_getTestCaseNames__not_a_TestCase(self):
        class BadCase(int):
            def test_foo(self):
                pass

        loader = unittest.TestLoader()
        names = loader.getTestCaseNames(BadCase)

        self.assertEqual(names, ['test_foo'])

    # "Return a sorted sequence of method names found within testCaseClass"
    #
    # Make sure inherited names are handled.
    #
    # TestP.foobar is defined to make sure getTestCaseNames() respects
    # loader.testMethodPrefix
    def test_getTestCaseNames__inheritance(self):
        class TestP(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
            def foobar(self): pass

        class TestC(TestP):
            def test_1(self): pass
            def test_3(self): pass

        loader = unittest.TestLoader()

        names = ['test_1', 'test_2', 'test_3']
        self.assertEqual(loader.getTestCaseNames(TestC), names)

    ################################################################
    ### /Tests for TestLoader.getTestCaseNames()

    ### Tests for TestLoader.testMethodPrefix
    ################################################################

    # "String giving the prefix of method names which will be interpreted as
    # test methods"
    #
    # Implicit in the documentation is that testMethodPrefix is respected by
    # all loadTestsFrom* methods.
    def test_testMethodPrefix__loadTestsFromTestCase(self):
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
            def foo_bar(self): pass

        tests_1 = unittest.TestSuite([Foo('foo_bar')])
        tests_2 = unittest.TestSuite([Foo('test_1'), Foo('test_2')])

        loader = unittest.TestLoader()
        loader.testMethodPrefix = 'foo'
        self.assertEqual(loader.loadTestsFromTestCase(Foo), tests_1)

        loader.testMethodPrefix = 'test'
        self.assertEqual(loader.loadTestsFromTestCase(Foo), tests_2)

    # "String giving the prefix of method names which will be interpreted as
    # test methods"
    #
    # Implicit in the documentation is that testMethodPrefix is respected by
    # all loadTestsFrom* methods.
    def test_testMethodPrefix__loadTestsFromModule(self):
        m = types.ModuleType('m')
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
            def foo_bar(self): pass
        m.Foo = Foo

        tests_1 = [unittest.TestSuite([Foo('foo_bar')])]
        tests_2 = [unittest.TestSuite([Foo('test_1'), Foo('test_2')])]

        loader = unittest.TestLoader()
        loader.testMethodPrefix = 'foo'
        self.assertEqual(list(loader.loadTestsFromModule(m)), tests_1)

        loader.testMethodPrefix = 'test'
        self.assertEqual(list(loader.loadTestsFromModule(m)), tests_2)

    # "String giving the prefix of method names which will be interpreted as
    # test methods"
    #
    # Implicit in the documentation is that testMethodPrefix is respected by
    # all loadTestsFrom* methods.
    def test_testMethodPrefix__loadTestsFromName(self):
        m = types.ModuleType('m')
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
            def foo_bar(self): pass
        m.Foo = Foo

        tests_1 = unittest.TestSuite([Foo('foo_bar')])
        tests_2 = unittest.TestSuite([Foo('test_1'), Foo('test_2')])

        loader = unittest.TestLoader()
        loader.testMethodPrefix = 'foo'
        self.assertEqual(loader.loadTestsFromName('Foo', m), tests_1)

        loader.testMethodPrefix = 'test'
        self.assertEqual(loader.loadTestsFromName('Foo', m), tests_2)

    # "String giving the prefix of method names which will be interpreted as
    # test methods"
    #
    # Implicit in the documentation is that testMethodPrefix is respected by
    # all loadTestsFrom* methods.
    def test_testMethodPrefix__loadTestsFromNames(self):
        m = types.ModuleType('m')
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
            def foo_bar(self): pass
        m.Foo = Foo

        tests_1 = unittest.TestSuite([unittest.TestSuite([Foo('foo_bar')])])
        tests_2 = unittest.TestSuite([Foo('test_1'), Foo('test_2')])
        tests_2 = unittest.TestSuite([tests_2])

        loader = unittest.TestLoader()
        loader.testMethodPrefix = 'foo'
        self.assertEqual(loader.loadTestsFromNames(['Foo'], m), tests_1)

        loader.testMethodPrefix = 'test'
        self.assertEqual(loader.loadTestsFromNames(['Foo'], m), tests_2)

    # "The default value is 'test'"
    def test_testMethodPrefix__default_value(self):
        loader = unittest.TestLoader()
        self.assertEqual(loader.testMethodPrefix, 'test')

    ################################################################
    ### /Tests for TestLoader.testMethodPrefix

    ### Tests for TestLoader.sortTestMethodsUsing
    ################################################################

    # "Function to be used to compare method names when sorting them in
    # getTestCaseNames() and all the loadTestsFromX() methods"
    def test_sortTestMethodsUsing__loadTestsFromTestCase(self):
        def reversed_cmp(x, y):
            return -((x > y) - (x < y))

        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass

        loader = unittest.TestLoader()
        loader.sortTestMethodsUsing = reversed_cmp

        tests = loader.suiteClass([Foo('test_2'), Foo('test_1')])
        self.assertEqual(loader.loadTestsFromTestCase(Foo), tests)

    # "Function to be used to compare method names when sorting them in
    # getTestCaseNames() and all the loadTestsFromX() methods"
    def test_sortTestMethodsUsing__loadTestsFromModule(self):
        def reversed_cmp(x, y):
            return -((x > y) - (x < y))

        m = types.ModuleType('m')
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
        m.Foo = Foo

        loader = unittest.TestLoader()
        loader.sortTestMethodsUsing = reversed_cmp

        tests = [loader.suiteClass([Foo('test_2'), Foo('test_1')])]
        self.assertEqual(list(loader.loadTestsFromModule(m)), tests)

    # "Function to be used to compare method names when sorting them in
    # getTestCaseNames() and all the loadTestsFromX() methods"
    def test_sortTestMethodsUsing__loadTestsFromName(self):
        def reversed_cmp(x, y):
            return -((x > y) - (x < y))

        m = types.ModuleType('m')
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
        m.Foo = Foo

        loader = unittest.TestLoader()
        loader.sortTestMethodsUsing = reversed_cmp

        tests = loader.suiteClass([Foo('test_2'), Foo('test_1')])
        self.assertEqual(loader.loadTestsFromName('Foo', m), tests)

    # "Function to be used to compare method names when sorting them in
    # getTestCaseNames() and all the loadTestsFromX() methods"
    def test_sortTestMethodsUsing__loadTestsFromNames(self):
        def reversed_cmp(x, y):
            return -((x > y) - (x < y))

        m = types.ModuleType('m')
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
        m.Foo = Foo

        loader = unittest.TestLoader()
        loader.sortTestMethodsUsing = reversed_cmp

        tests = [loader.suiteClass([Foo('test_2'), Foo('test_1')])]
        self.assertEqual(list(loader.loadTestsFromNames(['Foo'], m)), tests)

    # "Function to be used to compare method names when sorting them in
    # getTestCaseNames()"
    #
    # Does it actually affect getTestCaseNames()?
    def test_sortTestMethodsUsing__getTestCaseNames(self):
        def reversed_cmp(x, y):
            return -((x > y) - (x < y))

        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass

        loader = unittest.TestLoader()
        loader.sortTestMethodsUsing = reversed_cmp

        test_names = ['test_2', 'test_1']
        self.assertEqual(loader.getTestCaseNames(Foo), test_names)

    # "The default value is the built-in cmp() function"
    # Since cmp is now defunct, we simply verify that the results
    # occur in the same order as they would with the default sort.
    def test_sortTestMethodsUsing__default_value(self):
        loader = unittest.TestLoader()

        class Foo(unittest.TestCase):
            def test_2(self): pass
            def test_3(self): pass
            def test_1(self): pass

        test_names = ['test_2', 'test_3', 'test_1']
        self.assertEqual(loader.getTestCaseNames(Foo), sorted(test_names))


    # "it can be set to None to disable the sort."
    #
    # XXX How is this different from reassigning cmp? Are the tests returned
    # in a random order or something? This behaviour should die
    def test_sortTestMethodsUsing__None(self):
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass

        loader = unittest.TestLoader()
        loader.sortTestMethodsUsing = None

        test_names = ['test_2', 'test_1']
        self.assertEqual(set(loader.getTestCaseNames(Foo)), set(test_names))

    ################################################################
    ### /Tests for TestLoader.sortTestMethodsUsing

    ### Tests for TestLoader.suiteClass
    ################################################################

    # "Callable object that constructs a test suite from a list of tests."
    def test_suiteClass__loadTestsFromTestCase(self):
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
            def foo_bar(self): pass

        tests = [Foo('test_1'), Foo('test_2')]

        loader = unittest.TestLoader()
        loader.suiteClass = list
        self.assertEqual(loader.loadTestsFromTestCase(Foo), tests)

    # It is implicit in the documentation for TestLoader.suiteClass that
    # all TestLoader.loadTestsFrom* methods respect it. Let's make sure
    def test_suiteClass__loadTestsFromModule(self):
        m = types.ModuleType('m')
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
            def foo_bar(self): pass
        m.Foo = Foo

        tests = [[Foo('test_1'), Foo('test_2')]]

        loader = unittest.TestLoader()
        loader.suiteClass = list
        self.assertEqual(loader.loadTestsFromModule(m), tests)

    # It is implicit in the documentation for TestLoader.suiteClass that
    # all TestLoader.loadTestsFrom* methods respect it. Let's make sure
    def test_suiteClass__loadTestsFromName(self):
        m = types.ModuleType('m')
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
            def foo_bar(self): pass
        m.Foo = Foo

        tests = [Foo('test_1'), Foo('test_2')]

        loader = unittest.TestLoader()
        loader.suiteClass = list
        self.assertEqual(loader.loadTestsFromName('Foo', m), tests)

    # It is implicit in the documentation for TestLoader.suiteClass that
    # all TestLoader.loadTestsFrom* methods respect it. Let's make sure
    def test_suiteClass__loadTestsFromNames(self):
        m = types.ModuleType('m')
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass
            def foo_bar(self): pass
        m.Foo = Foo

        tests = [[Foo('test_1'), Foo('test_2')]]

        loader = unittest.TestLoader()
        loader.suiteClass = list
        self.assertEqual(loader.loadTestsFromNames(['Foo'], m), tests)

    # "The default value is the TestSuite class"
    def test_suiteClass__default_value(self):
        loader = unittest.TestLoader()
        self.assertTrue(loader.suiteClass is unittest.TestSuite)

    ################################################################
    ### /Tests for TestLoader.suiteClass

### Support code for Test_TestSuite
################################################################

class Foo(unittest.TestCase):
    def test_1(self): pass
    def test_2(self): pass
    def test_3(self): pass
    def runTest(self): pass

def _mk_TestSuite(*names):
    return unittest.TestSuite(Foo(n) for n in names)

################################################################
### /Support code for Test_TestSuite

class Test_TestSuite(TestCase, TestEquality):

    ### Set up attributes needed by inherited tests
    ################################################################

    # Used by TestEquality.test_eq
    eq_pairs = [(unittest.TestSuite(), unittest.TestSuite())
               ,(unittest.TestSuite(), unittest.TestSuite([]))
               ,(_mk_TestSuite('test_1'), _mk_TestSuite('test_1'))]

    # Used by TestEquality.test_ne
    ne_pairs = [(unittest.TestSuite(), _mk_TestSuite('test_1'))
               ,(unittest.TestSuite([]), _mk_TestSuite('test_1'))
               ,(_mk_TestSuite('test_1', 'test_2'), _mk_TestSuite('test_1', 'test_3'))
               ,(_mk_TestSuite('test_1'), _mk_TestSuite('test_2'))]

    ################################################################
    ### /Set up attributes needed by inherited tests

    ### Tests for TestSuite.__init__
    ################################################################

    # "class TestSuite([tests])"
    #
    # The tests iterable should be optional
    def test_init__tests_optional(self):
        suite = unittest.TestSuite()

        self.assertEqual(suite.countTestCases(), 0)

    # "class TestSuite([tests])"
    # ...
    # "If tests is given, it must be an iterable of individual test cases
    # or other test suites that will be used to build the suite initially"
    #
    # TestSuite should deal with empty tests iterables by allowing the
    # creation of an empty suite
    def test_init__empty_tests(self):
        suite = unittest.TestSuite([])

        self.assertEqual(suite.countTestCases(), 0)

    # "class TestSuite([tests])"
    # ...
    # "If tests is given, it must be an iterable of individual test cases
    # or other test suites that will be used to build the suite initially"
    #
    # TestSuite should allow any iterable to provide tests
    def test_init__tests_from_any_iterable(self):
        def tests():
            yield unittest.FunctionTestCase(lambda: None)
            yield unittest.FunctionTestCase(lambda: None)

        suite_1 = unittest.TestSuite(tests())
        self.assertEqual(suite_1.countTestCases(), 2)

        suite_2 = unittest.TestSuite(suite_1)
        self.assertEqual(suite_2.countTestCases(), 2)

        suite_3 = unittest.TestSuite(set(suite_1))
        self.assertEqual(suite_3.countTestCases(), 2)

    # "class TestSuite([tests])"
    # ...
    # "If tests is given, it must be an iterable of individual test cases
    # or other test suites that will be used to build the suite initially"
    #
    # Does TestSuite() also allow other TestSuite() instances to be present
    # in the tests iterable?
    def test_init__TestSuite_instances_in_tests(self):
        def tests():
            ftc = unittest.FunctionTestCase(lambda: None)
            yield unittest.TestSuite([ftc])
            yield unittest.FunctionTestCase(lambda: None)

        suite = unittest.TestSuite(tests())
        self.assertEqual(suite.countTestCases(), 2)

    ################################################################
    ### /Tests for TestSuite.__init__

    # Container types should support the iter protocol
    def test_iter(self):
        test1 = unittest.FunctionTestCase(lambda: None)
        test2 = unittest.FunctionTestCase(lambda: None)
        suite = unittest.TestSuite((test1, test2))

        self.assertEqual(list(suite), [test1, test2])

    # "Return the number of tests represented by the this test object.
    # ...this method is also implemented by the TestSuite class, which can
    # return larger [greater than 1] values"
    #
    # Presumably an empty TestSuite returns 0?
    def test_countTestCases_zero_simple(self):
        suite = unittest.TestSuite()

        self.assertEqual(suite.countTestCases(), 0)

    # "Return the number of tests represented by the this test object.
    # ...this method is also implemented by the TestSuite class, which can
    # return larger [greater than 1] values"
    #
    # Presumably an empty TestSuite (even if it contains other empty
    # TestSuite instances) returns 0?
    def test_countTestCases_zero_nested(self):
        class Test1(unittest.TestCase):
            def test(self):
                pass

        suite = unittest.TestSuite([unittest.TestSuite()])

        self.assertEqual(suite.countTestCases(), 0)

    # "Return the number of tests represented by the this test object.
    # ...this method is also implemented by the TestSuite class, which can
    # return larger [greater than 1] values"
    def test_countTestCases_simple(self):
        test1 = unittest.FunctionTestCase(lambda: None)
        test2 = unittest.FunctionTestCase(lambda: None)
        suite = unittest.TestSuite((test1, test2))

        self.assertEqual(suite.countTestCases(), 2)

    # "Return the number of tests represented by the this test object.
    # ...this method is also implemented by the TestSuite class, which can
    # return larger [greater than 1] values"
    #
    # Make sure this holds for nested TestSuite instances, too
    def test_countTestCases_nested(self):
        class Test1(unittest.TestCase):
            def test1(self): pass
            def test2(self): pass

        test2 = unittest.FunctionTestCase(lambda: None)
        test3 = unittest.FunctionTestCase(lambda: None)
        child = unittest.TestSuite((Test1('test2'), test2))
        parent = unittest.TestSuite((test3, child, Test1('test1')))

        self.assertEqual(parent.countTestCases(), 4)

    # "Run the tests associated with this suite, collecting the result into
    # the test result object passed as result."
    #
    # And if there are no tests? What then?
    def test_run__empty_suite(self):
        events = []
        result = LoggingResult(events)

        suite = unittest.TestSuite()

        suite.run(result)

        self.assertEqual(events, [])

    # "Note that unlike TestCase.run(), TestSuite.run() requires the
    # "result object to be passed in."
    def test_run__requires_result(self):
        suite = unittest.TestSuite()

        try:
            suite.run()
        except TypeError:
            pass
        else:
            self.fail("Failed to raise TypeError")

    # "Run the tests associated with this suite, collecting the result into
    # the test result object passed as result."
    def test_run(self):
        events = []
        result = LoggingResult(events)

        class LoggingCase(unittest.TestCase):
            def run(self, result):
                events.append('run %s' % self._testMethodName)

            def test1(self): pass
            def test2(self): pass

        tests = [LoggingCase('test1'), LoggingCase('test2')]

        unittest.TestSuite(tests).run(result)

        self.assertEqual(events, ['run test1', 'run test2'])

    # "Add a TestCase ... to the suite"
    def test_addTest__TestCase(self):
        class Foo(unittest.TestCase):
            def test(self): pass

        test = Foo('test')
        suite = unittest.TestSuite()

        suite.addTest(test)

        self.assertEqual(suite.countTestCases(), 1)
        self.assertEqual(list(suite), [test])

    # "Add a ... TestSuite to the suite"
    def test_addTest__TestSuite(self):
        class Foo(unittest.TestCase):
            def test(self): pass

        suite_2 = unittest.TestSuite([Foo('test')])

        suite = unittest.TestSuite()
        suite.addTest(suite_2)

        self.assertEqual(suite.countTestCases(), 1)
        self.assertEqual(list(suite), [suite_2])

    # "Add all the tests from an iterable of TestCase and TestSuite
    # instances to this test suite."
    #
    # "This is equivalent to iterating over tests, calling addTest() for
    # each element"
    def test_addTests(self):
        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass

        test_1 = Foo('test_1')
        test_2 = Foo('test_2')
        inner_suite = unittest.TestSuite([test_2])

        def gen():
            yield test_1
            yield test_2
            yield inner_suite

        suite_1 = unittest.TestSuite()
        suite_1.addTests(gen())

        self.assertEqual(list(suite_1), list(gen()))

        # "This is equivalent to iterating over tests, calling addTest() for
        # each element"
        suite_2 = unittest.TestSuite()
        for t in gen():
            suite_2.addTest(t)

        self.assertEqual(suite_1, suite_2)

    # "Add all the tests from an iterable of TestCase and TestSuite
    # instances to this test suite."
    #
    # What happens if it doesn't get an iterable?
    def test_addTest__noniterable(self):
        suite = unittest.TestSuite()

        try:
            suite.addTests(5)
        except TypeError:
            pass
        else:
            self.fail("Failed to raise TypeError")

    def test_addTest__noncallable(self):
        suite = unittest.TestSuite()
        self.assertRaises(TypeError, suite.addTest, 5)

    def test_addTest__casesuiteclass(self):
        suite = unittest.TestSuite()
        self.assertRaises(TypeError, suite.addTest, Test_TestSuite)
        self.assertRaises(TypeError, suite.addTest, unittest.TestSuite)

    def test_addTests__string(self):
        suite = unittest.TestSuite()
        self.assertRaises(TypeError, suite.addTests, "foo")


class Test_FunctionTestCase(TestCase):

    # "Return the number of tests represented by the this test object. For
    # TestCase instances, this will always be 1"
    def test_countTestCases(self):
        test = unittest.FunctionTestCase(lambda: None)

        self.assertEqual(test.countTestCases(), 1)

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

        def setUp():
            events.append('setUp')
            raise RuntimeError('raised by setUp')

        def test():
            events.append('test')

        def tearDown():
            events.append('tearDown')

        expected = ['startTest', 'setUp', 'addError', 'stopTest']
        unittest.FunctionTestCase(test, setUp, tearDown).run(result)
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

        def setUp():
            events.append('setUp')

        def test():
            events.append('test')
            raise RuntimeError('raised by test')

        def tearDown():
            events.append('tearDown')

        expected = ['startTest', 'setUp', 'test', 'addError', 'tearDown',
                    'stopTest']
        unittest.FunctionTestCase(test, setUp, tearDown).run(result)
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

        def setUp():
            events.append('setUp')

        def test():
            events.append('test')
            self.fail('raised by test')

        def tearDown():
            events.append('tearDown')

        expected = ['startTest', 'setUp', 'test', 'addFailure', 'tearDown',
                    'stopTest']
        unittest.FunctionTestCase(test, setUp, tearDown).run(result)
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

        def setUp():
            events.append('setUp')

        def test():
            events.append('test')

        def tearDown():
            events.append('tearDown')
            raise RuntimeError('raised by tearDown')

        expected = ['startTest', 'setUp', 'test', 'tearDown', 'addError',
                    'stopTest']
        unittest.FunctionTestCase(test, setUp, tearDown).run(result)
        self.assertEqual(events, expected)

    # "Return a string identifying the specific test case."
    #
    # Because of the vague nature of the docs, I'm not going to lock this
    # test down too much. Really all that can be asserted is that the id()
    # will be a string (either 8-byte or unicode -- again, because the docs
    # just say "string")
    def test_id(self):
        test = unittest.FunctionTestCase(lambda: None)

        self.assertTrue(isinstance(test.id(), str))

    # "Returns a one-line description of the test, or None if no description
    # has been provided. The default implementation of this method returns
    # the first line of the test method's docstring, if available, or None."
    def test_shortDescription__no_docstring(self):
        test = unittest.FunctionTestCase(lambda: None)

        self.assertEqual(test.shortDescription(), None)

    # "Returns a one-line description of the test, or None if no description
    # has been provided. The default implementation of this method returns
    # the first line of the test method's docstring, if available, or None."
    def test_shortDescription__singleline_docstring(self):
        desc = "this tests foo"
        test = unittest.FunctionTestCase(lambda: None, description=desc)

        self.assertEqual(test.shortDescription(), "this tests foo")

class Test_TestResult(TestCase):
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
        import sys

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
        self.assertTrue(test_case is test)
        self.assertTrue(isinstance(formatted_exc, str))

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
        import sys

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
        self.assertTrue(test_case is test)
        self.assertTrue(isinstance(formatted_exc, str))

### Support code for Test_TestCase
################################################################

class Foo(unittest.TestCase):
    def runTest(self): pass
    def test1(self): pass

class Bar(Foo):
    def test2(self): pass

class LoggingTestCase(unittest.TestCase):
    """A test case which logs its calls."""

    def __init__(self, events):
        super(LoggingTestCase, self).__init__('test')
        self.events = events

    def setUp(self):
        self.events.append('setUp')

    def test(self):
        self.events.append('test')

    def tearDown(self):
        self.events.append('tearDown')

class ResultWithNoStartTestRunStopTestRun(object):
    """An object honouring TestResult before startTestRun/stopTestRun."""

    def __init__(self):
        self.failures = []
        self.errors = []
        self.testsRun = 0
        self.skipped = []
        self.expectedFailures = []
        self.unexpectedSuccesses = []
        self.shouldStop = False

    def startTest(self, test):
        pass

    def stopTest(self, test):
        pass

    def addError(self, test):
        pass

    def addFailure(self, test):
        pass

    def addSuccess(self, test):
        pass

    def wasSuccessful(self):
        return True


################################################################
### /Support code for Test_TestCase

class Test_TestCase(TestCase, TestEquality, TestHashing):

    ### Set up attributes used by inherited tests
    ################################################################

    # Used by TestHashing.test_hash and TestEquality.test_eq
    eq_pairs = [(Foo('test1'), Foo('test1'))]

    # Used by TestEquality.test_ne
    ne_pairs = [(Foo('test1'), Foo('runTest'))
               ,(Foo('test1'), Bar('test1'))
               ,(Foo('test1'), Bar('test2'))]

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

        class Foo(LoggingTestCase):
            def setUp(self):
                super(Foo, self).setUp()
                raise RuntimeError('raised by Foo.setUp')

        Foo(events).run(result)
        expected = ['startTest', 'setUp', 'addError', 'stopTest']
        self.assertEqual(events, expected)

    # "With a temporary result stopTestRun is called when setUp errors.
    def test_run_call_order__error_in_setUp_default_result(self):
        events = []

        class Foo(LoggingTestCase):
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

        class Foo(LoggingTestCase):
            def test(self):
                super(Foo, self).test()
                raise RuntimeError('raised by Foo.test')

        expected = ['startTest', 'setUp', 'test', 'addError', 'tearDown',
                    'stopTest']
        Foo(events).run(result)
        self.assertEqual(events, expected)

    # "With a default result, an error in the test still results in stopTestRun
    # being called."
    def test_run_call_order__error_in_test_default_result(self):
        events = []

        class Foo(LoggingTestCase):
            def defaultTestResult(self):
                return LoggingResult(self.events)

            def test(self):
                super(Foo, self).test()
                raise RuntimeError('raised by Foo.test')

        expected = ['startTestRun', 'startTest', 'setUp', 'test', 'addError',
                    'tearDown', 'stopTest', 'stopTestRun']
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

        class Foo(LoggingTestCase):
            def test(self):
                super(Foo, self).test()
                self.fail('raised by Foo.test')

        expected = ['startTest', 'setUp', 'test', 'addFailure', 'tearDown',
                    'stopTest']
        Foo(events).run(result)
        self.assertEqual(events, expected)

    # "When a test fails with a default result stopTestRun is still called."
    def test_run_call_order__failure_in_test_default_result(self):

        class Foo(LoggingTestCase):
            def defaultTestResult(self):
                return LoggingResult(self.events)
            def test(self):
                super(Foo, self).test()
                self.fail('raised by Foo.test')

        expected = ['startTestRun', 'startTest', 'setUp', 'test', 'addFailure',
                    'tearDown', 'stopTest', 'stopTestRun']
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

        class Foo(LoggingTestCase):
            def tearDown(self):
                super(Foo, self).tearDown()
                raise RuntimeError('raised by Foo.tearDown')

        Foo(events).run(result)
        expected = ['startTest', 'setUp', 'test', 'tearDown', 'addError',
                    'stopTest']
        self.assertEqual(events, expected)

    # "When tearDown errors with a default result stopTestRun is still called."
    def test_run_call_order__error_in_tearDown_default_result(self):

        class Foo(LoggingTestCase):
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

        self.assertTrue(isinstance(Foo().id(), str))

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
        self.assertEqual(
                self.shortDescription(),
                'testShortDescriptionWithoutDocstring (' + __name__ +
                '.Test_TestCase)')

    def testShortDescriptionWithOneLineDocstring(self):
        """Tests shortDescription() for a method with a docstring."""
        self.assertEqual(
                self.shortDescription(),
                ('testShortDescriptionWithOneLineDocstring '
                 '(' + __name__ + '.Test_TestCase)\n'
                 'Tests shortDescription() for a method with a docstring.'))

    def testShortDescriptionWithMultiLineDocstring(self):
        """Tests shortDescription() for a method with a longer docstring.

        This method ensures that only the first line of a docstring is
        returned used in the short description, no matter how long the
        whole thing is.
        """
        self.assertEqual(
                self.shortDescription(),
                ('testShortDescriptionWithMultiLineDocstring '
                 '(' + __name__ + '.Test_TestCase)\n'
                 'Tests shortDescription() for a method with a longer '
                 'docstring.'))

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

        self.assertRaises(unittest.TestCase.failureException,
                          self.assertDictContainsSubset, {'a': 2}, {'a': 1},
                          '.*Mismatched values:.*')

        self.assertRaises(unittest.TestCase.failureException,
                          self.assertDictContainsSubset, {'c': 1}, {'a': 1},
                          '.*Missing:.*')

        self.assertRaises(unittest.TestCase.failureException,
                          self.assertDictContainsSubset, {'a': 1, 'c': 1},
                          {'a': 1}, '.*Missing:.*')

        self.assertRaises(unittest.TestCase.failureException,
                          self.assertDictContainsSubset, {'a': 1, 'c': 1},
                          {'a': 1}, '.*Missing:.*Mismatched values:.*')

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

        self.assertSameElements([1, 2, 3], [3, 2, 1])
        self.assertSameElements([1, 2] + [3] * 100, [1] * 100 + [2, 3])
        self.assertSameElements(['foo', 'bar', 'baz'], ['bar', 'baz', 'foo'])
        self.assertRaises(self.failureException, self.assertSameElements,
                          [10], [10, 11])
        self.assertRaises(self.failureException, self.assertSameElements,
                          [10, 11], [10])

        # Test that sequences of unhashable objects can be tested for sameness:
        self.assertSameElements([[1, 2], [3, 4]], [[3, 4], [1, 2]])

        self.assertSameElements([{'a': 1}, {'b': 2}], [{'b': 2}, {'a': 1}])
        self.assertRaises(self.failureException, self.assertSameElements,
                          [[1]], [[2]])
        self.assertRaises(self.failureException, self.assertSameElements,
                          [{'a': 1}, {'b': 2}], [{'b': 2}, {'a': 2}])

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
        sample_text_error = """
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

        try:
            self.assertMultiLineEqual(sample_text, revised_sample_text)
        except self.failureException as e:
            # no fair testing ourself with ourself, use assertEqual..
            self.assertEqual(sample_text_error, str(e))

    def testAssertIsNone(self):
        self.assertIsNone(None)
        self.assertRaises(self.failureException, self.assertIsNone, False)
        self.assertIsNotNone('DjZoPloGears on Rails')
        self.assertRaises(self.failureException, self.assertIsNotNone, None)

    def testAssertRegexpMatches(self):
        self.assertRegexpMatches('asdfabasdf', r'ab+')
        self.assertRaises(self.failureException, self.assertRegexpMatches,
                          'saaas', r'aaaa')

    def testAssertRaisesRegexp(self):
        class ExceptionMock(Exception):
            pass

        def Stub():
            raise ExceptionMock('We expect')

        self.assertRaisesRegexp(ExceptionMock, re.compile('expect$'), Stub)
        self.assertRaisesRegexp(ExceptionMock, 'expect$', Stub)

    def testAssertNotRaisesRegexp(self):
        self.assertRaisesRegexp(
                self.failureException, '^Exception not raised by <lambda>$',
                self.assertRaisesRegexp, Exception, re.compile('x'),
                lambda: None)
        self.assertRaisesRegexp(
                self.failureException, '^Exception not raised by <lambda>$',
                self.assertRaisesRegexp, Exception, 'x',
                lambda: None)

    def testAssertRaisesRegexpMismatch(self):
        def Stub():
            raise Exception('Unexpected')

        self.assertRaisesRegexp(
                self.failureException,
                r'"\^Expected\$" does not match "Unexpected"',
                self.assertRaisesRegexp, Exception, '^Expected$',
                Stub)
        self.assertRaisesRegexp(
                self.failureException,
                r'"\^Expected\$" does not match "Unexpected"',
                self.assertRaisesRegexp, Exception,
                re.compile('^Expected$'), Stub)

    def testSynonymAssertMethodNames(self):
        """Test undocumented method name synonyms.

        Please do not use these methods names in your own code.

        This test confirms their continued existence and functionality
        in order to avoid breaking existing code.
        """
        self.assertNotEquals(3, 5)
        self.assertEquals(3, 3)
        self.assertAlmostEquals(2.0, 2.0)
        self.assertNotAlmostEquals(3.0, 5.0)
        self.assert_(True)

    def testPendingDeprecationMethodNames(self):
        """Test fail* methods pending deprecation, they will warn in 3.2.

        Do not use these methods.  They will go away in 3.3.
        """
        self.failIfEqual(3, 5)
        self.failUnlessEqual(3, 3)
        self.failUnlessAlmostEqual(2.0, 2.0)
        self.failIfAlmostEqual(3.0, 5.0)
        self.failUnless(True)
        self.failUnlessRaises(TypeError, lambda _: 3.14 + 'spam')
        self.failIf(False)

    def testDeepcopy(self):
        # Issue: 5660
        class TestableTest(TestCase):
            def testNothing(self):
                pass

        test = TestableTest('testNothing')

        # This shouldn't blow up
        deepcopy(test)


class Test_TestSkipping(TestCase):

    def test_skipping(self):
        class Foo(unittest.TestCase):
            def test_skip_me(self):
                self.skipTest("skip")
        events = []
        result = LoggingResult(events)
        test = Foo("test_skip_me")
        test.run(result)
        self.assertEqual(events, ['startTest', 'addSkip', 'stopTest'])
        self.assertEqual(result.skipped, [(test, "skip")])

        # Try letting setUp skip the test now.
        class Foo(unittest.TestCase):
            def setUp(self):
                self.skipTest("testing")
            def test_nothing(self): pass
        events = []
        result = LoggingResult(events)
        test = Foo("test_nothing")
        test.run(result)
        self.assertEqual(events, ['startTest', 'addSkip', 'stopTest'])
        self.assertEqual(result.skipped, [(test, "testing")])
        self.assertEqual(result.testsRun, 1)

    def test_skipping_decorators(self):
        op_table = ((unittest.skipUnless, False, True),
                    (unittest.skipIf, True, False))
        for deco, do_skip, dont_skip in op_table:
            class Foo(unittest.TestCase):
                @deco(do_skip, "testing")
                def test_skip(self): pass

                @deco(dont_skip, "testing")
                def test_dont_skip(self): pass
            test_do_skip = Foo("test_skip")
            test_dont_skip = Foo("test_dont_skip")
            suite = unittest.TestSuite([test_do_skip, test_dont_skip])
            events = []
            result = LoggingResult(events)
            suite.run(result)
            self.assertEqual(len(result.skipped), 1)
            expected = ['startTest', 'addSkip', 'stopTest',
                        'startTest', 'addSuccess', 'stopTest']
            self.assertEqual(events, expected)
            self.assertEqual(result.testsRun, 2)
            self.assertEqual(result.skipped, [(test_do_skip, "testing")])
            self.assertTrue(result.wasSuccessful())

    def test_skip_class(self):
        @unittest.skip("testing")
        class Foo(unittest.TestCase):
            def test_1(self):
                record.append(1)
        record = []
        result = unittest.TestResult()
        test = Foo("test_1")
        suite = unittest.TestSuite([test])
        suite.run(result)
        self.assertEqual(result.skipped, [(test, "testing")])
        self.assertEqual(record, [])

    def test_expected_failure(self):
        class Foo(unittest.TestCase):
            @unittest.expectedFailure
            def test_die(self):
                self.fail("help me!")
        events = []
        result = LoggingResult(events)
        test = Foo("test_die")
        test.run(result)
        self.assertEqual(events,
                         ['startTest', 'addExpectedFailure', 'stopTest'])
        self.assertEqual(result.expectedFailures[0][0], test)
        self.assertTrue(result.wasSuccessful())

    def test_unexpected_success(self):
        class Foo(unittest.TestCase):
            @unittest.expectedFailure
            def test_die(self):
                pass
        events = []
        result = LoggingResult(events)
        test = Foo("test_die")
        test.run(result)
        self.assertEqual(events,
                         ['startTest', 'addUnexpectedSuccess', 'stopTest'])
        self.assertFalse(result.failures)
        self.assertEqual(result.unexpectedSuccesses, [test])
        self.assertTrue(result.wasSuccessful())



class Test_Assertions(TestCase):
    def test_AlmostEqual(self):
        self.assertAlmostEqual(1.00000001, 1.0)
        self.assertNotAlmostEqual(1.0000001, 1.0)
        self.assertRaises(self.failureException,
                          self.assertAlmostEqual, 1.0000001, 1.0)
        self.assertRaises(self.failureException,
                          self.assertNotAlmostEqual, 1.00000001, 1.0)

        self.assertAlmostEqual(1.1, 1.0, places=0)
        self.assertRaises(self.failureException,
                          self.assertAlmostEqual, 1.1, 1.0, places=1)

        self.assertAlmostEqual(0, .1+.1j, places=0)
        self.assertNotAlmostEqual(0, .1+.1j, places=1)
        self.assertRaises(self.failureException,
                          self.assertAlmostEqual, 0, .1+.1j, places=1)
        self.assertRaises(self.failureException,
                          self.assertNotAlmostEqual, 0, .1+.1j, places=0)

    def test_assertRaises(self):
        def _raise(e):
            raise e
        self.assertRaises(KeyError, _raise, KeyError)
        self.assertRaises(KeyError, _raise, KeyError("key"))
        try:
            self.assertRaises(KeyError, lambda: None)
        except self.failureException as e:
            self.assertIn("KeyError not raised", str(e))
        else:
            self.fail("assertRaises() didn't fail")
        try:
            self.assertRaises(KeyError, _raise, ValueError)
        except ValueError:
            pass
        else:
            self.fail("assertRaises() didn't let exception pass through")
        with self.assertRaises(KeyError):
            raise KeyError
        with self.assertRaises(KeyError):
            raise KeyError("key")
        try:
            with self.assertRaises(KeyError):
                pass
        except self.failureException as e:
            self.assertIn("KeyError not raised", str(e))
        else:
            self.fail("assertRaises() didn't fail")
        try:
            with self.assertRaises(KeyError):
                raise ValueError
        except ValueError:
            pass
        else:
            self.fail("assertRaises() didn't let exception pass through")


class TestLongMessage(TestCase):
    """Test that the individual asserts honour longMessage.
    This actually tests all the message behaviour for
    asserts that use longMessage."""

    def setUp(self):
        class TestableTestFalse(TestCase):
            longMessage = False
            failureException = self.failureException

            def testTest(self):
                pass

        class TestableTestTrue(TestCase):
            longMessage = True
            failureException = self.failureException

            def testTest(self):
                pass

        self.testableTrue = TestableTestTrue('testTest')
        self.testableFalse = TestableTestFalse('testTest')

    def testDefault(self):
        self.assertFalse(TestCase.longMessage)

    def test_formatMsg(self):
        self.assertEqual(self.testableFalse._formatMessage(None, "foo"), "foo")
        self.assertEqual(self.testableFalse._formatMessage("foo", "bar"), "foo")

        self.assertEqual(self.testableTrue._formatMessage(None, "foo"), "foo")
        self.assertEqual(self.testableTrue._formatMessage("foo", "bar"), "bar : foo")

    def assertMessages(self, methodName, args, errors):
        def getMethod(i):
            useTestableFalse  = i < 2
            if useTestableFalse:
                test = self.testableFalse
            else:
                test = self.testableTrue
            return getattr(test, methodName)

        for i, expected_regexp in enumerate(errors):
            testMethod = getMethod(i)
            kwargs = {}
            withMsg = i % 2
            if withMsg:
                kwargs = {"msg": "oops"}

            with self.assertRaisesRegexp(self.failureException,
                                         expected_regexp=expected_regexp):
                testMethod(*args, **kwargs)

    def testAssertTrue(self):
        self.assertMessages('assertTrue', (False,),
                            ["^False is not True$", "^oops$", "^False is not True$",
                             "^False is not True : oops$"])

    def testAssertFalse(self):
        self.assertMessages('assertFalse', (True,),
                            ["^True is not False$", "^oops$", "^True is not False$",
                             "^True is not False : oops$"])

    def testNotEqual(self):
        self.assertMessages('assertNotEqual', (1, 1),
                            ["^1 == 1$", "^oops$", "^1 == 1$",
                             "^1 == 1 : oops$"])

    def testAlmostEqual(self):
        self.assertMessages('assertAlmostEqual', (1, 2),
                            ["^1 != 2 within 7 places$", "^oops$",
                             "^1 != 2 within 7 places$", "^1 != 2 within 7 places : oops$"])

    def testNotAlmostEqual(self):
        self.assertMessages('assertNotAlmostEqual', (1, 1),
                            ["^1 == 1 within 7 places$", "^oops$",
                             "^1 == 1 within 7 places$", "^1 == 1 within 7 places : oops$"])

    def test_baseAssertEqual(self):
        self.assertMessages('_baseAssertEqual', (1, 2),
                            ["^1 != 2$", "^oops$", "^1 != 2$", "^1 != 2 : oops$"])

    def testAssertSequenceEqual(self):
        # Error messages are multiline so not testing on full message
        # assertTupleEqual and assertListEqual delegate to this method
        self.assertMessages('assertSequenceEqual', ([], [None]),
                            ["\+ \[None\]$", "^oops$", r"\+ \[None\]$",
                             r"\+ \[None\] : oops$"])

    def testAssertSetEqual(self):
        self.assertMessages('assertSetEqual', (set(), set([None])),
                            ["None$", "^oops$", "None$",
                             "None : oops$"])

    def testAssertIn(self):
        self.assertMessages('assertIn', (None, []),
                            ['^None not found in \[\]$', "^oops$",
                             '^None not found in \[\]$',
                             '^None not found in \[\] : oops$'])

    def testAssertNotIn(self):
        self.assertMessages('assertNotIn', (None, [None]),
                            ['^None unexpectedly found in \[None\]$', "^oops$",
                             '^None unexpectedly found in \[None\]$',
                             '^None unexpectedly found in \[None\] : oops$'])

    def testAssertDictEqual(self):
        self.assertMessages('assertDictEqual', ({}, {'key': 'value'}),
                            [r"\+ \{'key': 'value'\}$", "^oops$",
                             "\+ \{'key': 'value'\}$",
                             "\+ \{'key': 'value'\} : oops$"])

    def testAssertDictContainsSubset(self):
        self.assertMessages('assertDictContainsSubset', ({'key': 'value'}, {}),
                            ["^Missing: 'key'$", "^oops$",
                             "^Missing: 'key'$",
                             "^Missing: 'key' : oops$"])

    def testAssertSameElements(self):
        self.assertMessages('assertSameElements', ([], [None]),
                            [r"\[None\]$", "^oops$",
                             r"\[None\]$",
                             r"\[None\] : oops$"])

    def testAssertMultiLineEqual(self):
        self.assertMessages('assertMultiLineEqual', ("", "foo"),
                            [r"\+ foo$", "^oops$",
                             r"\+ foo$",
                             r"\+ foo : oops$"])

    def testAssertLess(self):
        self.assertMessages('assertLess', (2, 1),
                            ["^2 not less than 1$", "^oops$",
                             "^2 not less than 1$", "^2 not less than 1 : oops$"])

    def testAssertLessEqual(self):
        self.assertMessages('assertLessEqual', (2, 1),
                            ["^2 not less than or equal to 1$", "^oops$",
                             "^2 not less than or equal to 1$",
                             "^2 not less than or equal to 1 : oops$"])

    def testAssertGreater(self):
        self.assertMessages('assertGreater', (1, 2),
                            ["^1 not greater than 2$", "^oops$",
                             "^1 not greater than 2$",
                             "^1 not greater than 2 : oops$"])

    def testAssertGreaterEqual(self):
        self.assertMessages('assertGreaterEqual', (1, 2),
                            ["^1 not greater than or equal to 2$", "^oops$",
                             "^1 not greater than or equal to 2$",
                             "^1 not greater than or equal to 2 : oops$"])

    def testAssertIsNone(self):
        self.assertMessages('assertIsNone', ('not None',),
                            ["^'not None' is not None$", "^oops$",
                             "^'not None' is not None$",
                             "^'not None' is not None : oops$"])

    def testAssertIsNotNone(self):
        self.assertMessages('assertIsNotNone', (None,),
                            ["^unexpectedly None$", "^oops$",
                             "^unexpectedly None$",
                             "^unexpectedly None : oops$"])

    def testAssertIs(self):
        self.assertMessages('assertIs', (None, 'foo'),
                            ["^None is not 'foo'$", "^oops$",
                             "^None is not 'foo'$",
                             "^None is not 'foo' : oops$"])

    def testAssertIsNot(self):
        self.assertMessages('assertIsNot', (None, None),
                            ["^unexpectedly identical: None$", "^oops$",
                             "^unexpectedly identical: None$",
                             "^unexpectedly identical: None : oops$"])


class TestCleanUp(TestCase):

    def testCleanUp(self):
        class TestableTest(TestCase):
            def testNothing(self):
                pass

        test = TestableTest('testNothing')
        self.assertEqual(test._cleanups, [])

        cleanups = []

        def cleanup1(*args, **kwargs):
            cleanups.append((1, args, kwargs))

        def cleanup2(*args, **kwargs):
            cleanups.append((2, args, kwargs))

        test.addCleanup(cleanup1, 1, 2, 3, four='hello', five='goodbye')
        test.addCleanup(cleanup2)

        self.assertEqual(test._cleanups,
                         [(cleanup1, (1, 2, 3), dict(four='hello', five='goodbye')),
                          (cleanup2, (), {})])

        result = test.doCleanups()
        self.assertTrue(result)

        self.assertEqual(cleanups, [(2, (), {}), (1, (1, 2, 3), dict(four='hello', five='goodbye'))])

    def testCleanUpWithErrors(self):
        class TestableTest(TestCase):
            def testNothing(self):
                pass

        class MockResult(object):
            errors = []
            def addError(self, test, exc_info):
                self.errors.append((test, exc_info))

        result = MockResult()
        test = TestableTest('testNothing')
        test._resultForDoCleanups = result

        exc1 = Exception('foo')
        exc2 = Exception('bar')
        def cleanup1():
            raise exc1

        def cleanup2():
            raise exc2

        test.addCleanup(cleanup1)
        test.addCleanup(cleanup2)

        self.assertFalse(test.doCleanups())

        (test1, (Type1, instance1, _)), (test2, (Type2, instance2, _)) = reversed(MockResult.errors)
        self.assertEqual((test1, Type1, instance1), (test, Exception, exc1))
        self.assertEqual((test2, Type2, instance2), (test, Exception, exc2))

    def testCleanupInRun(self):
        blowUp = False
        ordering = []

        class TestableTest(TestCase):
            def setUp(self):
                ordering.append('setUp')
                if blowUp:
                    raise Exception('foo')

            def testNothing(self):
                ordering.append('test')

            def tearDown(self):
                ordering.append('tearDown')

        test = TestableTest('testNothing')

        def cleanup1():
            ordering.append('cleanup1')
        def cleanup2():
            ordering.append('cleanup2')
        test.addCleanup(cleanup1)
        test.addCleanup(cleanup2)

        def success(some_test):
            self.assertEqual(some_test, test)
            ordering.append('success')

        result = unittest.TestResult()
        result.addSuccess = success

        test.run(result)
        self.assertEqual(ordering, ['setUp', 'test', 'tearDown',
                                    'cleanup2', 'cleanup1', 'success'])

        blowUp = True
        ordering = []
        test = TestableTest('testNothing')
        test.addCleanup(cleanup1)
        test.run(result)
        self.assertEqual(ordering, ['setUp', 'cleanup1'])


class Test_TestProgram(TestCase):

    # Horrible white box test
    def testNoExit(self):
        result = object()
        test = object()

        class FakeRunner(object):
            def run(self, test):
                self.test = test
                return result

        runner = FakeRunner()

        try:
            oldParseArgs = TestProgram.parseArgs
            TestProgram.parseArgs = lambda *args: None
            TestProgram.test = test

            program = TestProgram(testRunner=runner, exit=False)

            self.assertEqual(program.result, result)
            self.assertEqual(runner.test, test)

        finally:
            TestProgram.parseArgs = oldParseArgs
            del TestProgram.test


    class FooBar(unittest.TestCase):
        def testPass(self):
            assert True
        def testFail(self):
            assert False

    class FooBarLoader(unittest.TestLoader):
        """Test loader that returns a suite containing FooBar."""
        def loadTestsFromModule(self, module):
            return self.suiteClass(
                [self.loadTestsFromTestCase(Test_TestProgram.FooBar)])


    def test_NonExit(self):
        program = unittest.main(exit=False,
                                argv=["foobar"],
                                testRunner=unittest.TextTestRunner(stream=io.StringIO()),
                                testLoader=self.FooBarLoader())
        self.assertTrue(hasattr(program, 'result'))


    def test_Exit(self):
        self.assertRaises(
            SystemExit,
            unittest.main,
            argv=["foobar"],
            testRunner=unittest.TextTestRunner(stream=io.StringIO()),
            exit=True,
            testLoader=self.FooBarLoader())


    def test_ExitAsDefault(self):
        self.assertRaises(
            SystemExit,
            unittest.main,
            argv=["foobar"],
            testRunner=unittest.TextTestRunner(stream=io.StringIO()),
            testLoader=self.FooBarLoader())


class Test_TextTestRunner(TestCase):
    """Tests for TextTestRunner."""

    def test_works_with_result_without_startTestRun_stopTestRun(self):
        class OldTextResult(ResultWithNoStartTestRunStopTestRun):
            separator2 = ''
            def printErrors(self):
                pass

        class Runner(unittest.TextTestRunner):
            def __init__(self):
                super(Runner, self).__init__(io.StringIO())

            def _makeResult(self):
                return OldTextResult()

        runner = Runner()
        runner.run(unittest.TestSuite())

    def test_startTestRun_stopTestRun_called(self):
        class LoggingTextResult(LoggingResult):
            separator2 = ''
            def printErrors(self):
                pass

        class LoggingRunner(unittest.TextTestRunner):
            def __init__(self, events):
                super(LoggingRunner, self).__init__(io.StringIO())
                self._events = events

            def _makeResult(self):
                return LoggingTextResult(self._events)

        events = []
        runner = LoggingRunner(events)
        runner.run(unittest.TestSuite())
        expected = ['startTestRun', 'stopTestRun']
        self.assertEqual(events, expected)


######################################################################
## Main
######################################################################

def test_main():
    support.run_unittest(Test_TestCase, Test_TestLoader,
        Test_TestSuite, Test_TestResult, Test_FunctionTestCase,
        Test_TestSkipping, Test_Assertions, TestLongMessage,
        Test_TestProgram, TestCleanUp)

if __name__ == "__main__":
    test_main()
