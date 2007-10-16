"""Test script for unittest.

By Collin Winter <collinw at gmail.com>

Still need testing:
    TestCase.{assert,fail}* methods (some are tested implicitly)
"""

from test import test_support
import unittest
from unittest import TestCase

### Support code
################################################################

class LoggingResult(unittest.TestResult):
    def __init__(self, log):
        self._events = log
        super().__init__()

    def startTest(self, test):
        self._events.append('startTest')
        super().startTest(test)

    def stopTest(self, test):
        self._events.append('stopTest')
        super().stopTest(test)

    def addFailure(self, *args):
        self._events.append('addFailure')
        super().addFailure(*args)

    def addError(self, *args):
        self._events.append('addError')
        super().addError(*args)

class TestEquality(object):
    # Check for a valid __eq__ implementation
    def test_eq(self):
        for obj_1, obj_2 in self.eq_pairs:
            self.assertEqual(obj_1, obj_2)
            self.assertEqual(obj_2, obj_1)

    # Check for a valid __ne__ implementation
    def test_ne(self):
        for obj_1, obj_2 in self.ne_pairs:
            self.failIfEqual(obj_1, obj_2)
            self.failIfEqual(obj_2, obj_1)

class TestHashing(object):
    # Check for a valid __hash__ implementation
    def test_hash(self):
        for obj_1, obj_2 in self.eq_pairs:
            try:
                assert hash(obj_1) == hash(obj_2)
            except KeyboardInterrupt:
                raise
            except AssertionError:
                self.fail("%s and %s do not hash equal" % (obj_1, obj_2))
            except Exception as e:
                self.fail("Problem hashing %s and %s: %s" % (obj_1, obj_2, e))

        for obj_1, obj_2 in self.ne_pairs:
            try:
                assert hash(obj_1) != hash(obj_2)
            except KeyboardInterrupt:
                raise
            except AssertionError:
                self.fail("%s and %s hash equal, but shouldn't" % (obj_1, obj_2))
            except Exception as e:
                self.fail("Problem hashing %s and %s: %s" % (obj_1, obj_2, e))


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
        self.failIf('runTest'.startswith(loader.testMethodPrefix))

        suite = loader.loadTestsFromTestCase(Foo)
        self.failUnless(isinstance(suite, loader.suiteClass))
        self.assertEqual(list(suite), [Foo('runTest')])

    ################################################################
    ### /Tests for TestLoader.loadTestsFromTestCase

    ### Tests for TestLoader.loadTestsFromModule
    ################################################################

    # "This method searches `module` for classes derived from TestCase"
    def test_loadTestsFromModule__TestCase_subclass(self):
        import new
        m = new.module('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromModule(m)
        self.failUnless(isinstance(suite, loader.suiteClass))

        expected = [loader.suiteClass([MyTestCase('test')])]
        self.assertEqual(list(suite), expected)

    # "This method searches `module` for classes derived from TestCase"
    #
    # What happens if no tests are found (no TestCase instances)?
    def test_loadTestsFromModule__no_TestCase_instances(self):
        import new
        m = new.module('m')

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromModule(m)
        self.failUnless(isinstance(suite, loader.suiteClass))
        self.assertEqual(list(suite), [])

    # "This method searches `module` for classes derived from TestCase"
    #
    # What happens if no tests are found (TestCases instances, but no tests)?
    def test_loadTestsFromModule__no_TestCase_tests(self):
        import new
        m = new.module('m')
        class MyTestCase(unittest.TestCase):
            pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromModule(m)
        self.failUnless(isinstance(suite, loader.suiteClass))

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
        import new
        m = new.module('m')
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
        import new
        m = new.module('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromName('testcase_1', m)
        self.failUnless(isinstance(suite, loader.suiteClass))
        self.assertEqual(list(suite), [MyTestCase('test')])

    # "The specifier name is a ``dotted name'' that may resolve either to
    # a module, a test case class, a TestSuite instance, a test method
    # within a test case class, or a callable object which returns a
    # TestCase or TestSuite instance."
    def test_loadTestsFromName__relative_TestSuite(self):
        import new
        m = new.module('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testsuite = unittest.TestSuite([MyTestCase('test')])

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromName('testsuite', m)
        self.failUnless(isinstance(suite, loader.suiteClass))

        self.assertEqual(list(suite), [MyTestCase('test')])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a test method within a test case class"
    def test_loadTestsFromName__relative_testmethod(self):
        import new
        m = new.module('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromName('testcase_1.test', m)
        self.failUnless(isinstance(suite, loader.suiteClass))

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
        import new
        m = new.module('m')
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
        import new
        m = new.module('m')
        testcase_1 = unittest.FunctionTestCase(lambda: None)
        testcase_2 = unittest.FunctionTestCase(lambda: None)
        def return_TestSuite():
            return unittest.TestSuite([testcase_1, testcase_2])
        m.return_TestSuite = return_TestSuite

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromName('return_TestSuite', m)
        self.failUnless(isinstance(suite, loader.suiteClass))
        self.assertEqual(list(suite), [testcase_1, testcase_2])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a callable object which returns a TestCase ... instance"
    def test_loadTestsFromName__callable__TestCase_instance(self):
        import new
        m = new.module('m')
        testcase_1 = unittest.FunctionTestCase(lambda: None)
        def return_TestCase():
            return testcase_1
        m.return_TestCase = return_TestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromName('return_TestCase', m)
        self.failUnless(isinstance(suite, loader.suiteClass))
        self.assertEqual(list(suite), [testcase_1])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a callable object which returns a TestCase or TestSuite instance"
    #
    # What happens if the callable returns something else?
    def test_loadTestsFromName__callable__wrong_type(self):
        import new
        m = new.module('m')
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

            self.failUnless(isinstance(suite, loader.suiteClass))
            self.assertEqual(list(suite), [])

            # audioop should now be loaded, thanks to loadTestsFromName()
            self.failUnless(module_name in sys.modules)
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
        self.failUnless(isinstance(suite, loader.suiteClass))
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
        self.failUnless(isinstance(suite, loader.suiteClass))
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
        import new
        m = new.module('m')
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
        import new
        m = new.module('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromNames(['testcase_1'], m)
        self.failUnless(isinstance(suite, loader.suiteClass))

        expected = loader.suiteClass([MyTestCase('test')])
        self.assertEqual(list(suite), [expected])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a TestSuite instance"
    def test_loadTestsFromNames__relative_TestSuite(self):
        import new
        m = new.module('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testsuite = unittest.TestSuite([MyTestCase('test')])

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromNames(['testsuite'], m)
        self.failUnless(isinstance(suite, loader.suiteClass))

        self.assertEqual(list(suite), [m.testsuite])

    # "The specifier name is a ``dotted name'' that may resolve ... to ... a
    # test method within a test case class"
    def test_loadTestsFromNames__relative_testmethod(self):
        import new
        m = new.module('m')
        class MyTestCase(unittest.TestCase):
            def test(self):
                pass
        m.testcase_1 = MyTestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromNames(['testcase_1.test'], m)
        self.failUnless(isinstance(suite, loader.suiteClass))

        ref_suite = unittest.TestSuite([MyTestCase('test')])
        self.assertEqual(list(suite), [ref_suite])

    # "The specifier name is a ``dotted name'' that may resolve ... to ... a
    # test method within a test case class"
    #
    # Does the method gracefully handle names that initially look like they
    # resolve to "a test method within a test case class" but don't?
    def test_loadTestsFromNames__relative_invalid_testmethod(self):
        import new
        m = new.module('m')
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
        import new
        m = new.module('m')
        testcase_1 = unittest.FunctionTestCase(lambda: None)
        testcase_2 = unittest.FunctionTestCase(lambda: None)
        def return_TestSuite():
            return unittest.TestSuite([testcase_1, testcase_2])
        m.return_TestSuite = return_TestSuite

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromNames(['return_TestSuite'], m)
        self.failUnless(isinstance(suite, loader.suiteClass))

        expected = unittest.TestSuite([testcase_1, testcase_2])
        self.assertEqual(list(suite), [expected])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a callable object which returns a TestCase ... instance"
    def test_loadTestsFromNames__callable__TestCase_instance(self):
        import new
        m = new.module('m')
        testcase_1 = unittest.FunctionTestCase(lambda: None)
        def return_TestCase():
            return testcase_1
        m.return_TestCase = return_TestCase

        loader = unittest.TestLoader()
        suite = loader.loadTestsFromNames(['return_TestCase'], m)
        self.failUnless(isinstance(suite, loader.suiteClass))

        ref_suite = unittest.TestSuite([testcase_1])
        self.assertEqual(list(suite), [ref_suite])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a callable object which returns a TestCase or TestSuite instance"
    #
    # Are staticmethods handled correctly?
    def test_loadTestsFromNames__callable__call_staticmethod(self):
        import new
        m = new.module('m')
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
        self.failUnless(isinstance(suite, loader.suiteClass))

        ref_suite = unittest.TestSuite([testcase_1])
        self.assertEqual(list(suite), [ref_suite])

    # "The specifier name is a ``dotted name'' that may resolve ... to
    # ... a callable object which returns a TestCase or TestSuite instance"
    #
    # What happens when the callable returns something else?
    def test_loadTestsFromNames__callable__wrong_type(self):
        import new
        m = new.module('m')
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

            self.failUnless(isinstance(suite, loader.suiteClass))
            self.assertEqual(list(suite), [unittest.TestSuite()])

            # audioop should now be loaded, thanks to loadTestsFromName()
            self.failUnless(module_name in sys.modules)
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
        import new
        m = new.module('m')
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
        import new
        m = new.module('m')
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
        import new
        m = new.module('m')
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
            return -cmp(x, y)

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
            return -cmp(x, y)

        import new
        m = new.module('m')
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
            return -cmp(x, y)

        import new
        m = new.module('m')
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
            return -cmp(x, y)

        import new
        m = new.module('m')
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
            return -cmp(x, y)

        class Foo(unittest.TestCase):
            def test_1(self): pass
            def test_2(self): pass

        loader = unittest.TestLoader()
        loader.sortTestMethodsUsing = reversed_cmp

        test_names = ['test_2', 'test_1']
        self.assertEqual(loader.getTestCaseNames(Foo), test_names)

    # "The default value is the built-in cmp() function"
    def test_sortTestMethodsUsing__default_value(self):
        loader = unittest.TestLoader()
        self.failUnless(loader.sortTestMethodsUsing is cmp)

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
        import new
        m = new.module('m')
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
        import new
        m = new.module('m')
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
        import new
        m = new.module('m')
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
        self.failUnless(loader.suiteClass is unittest.TestSuite)

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

        self.failUnless(isinstance(test.id(), str))

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

        self.failUnless(result.wasSuccessful())
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

        self.failUnless(result.wasSuccessful())
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

        self.failUnless(result.wasSuccessful())
        self.assertEqual(len(result.errors), 0)
        self.assertEqual(len(result.failures), 0)
        self.assertEqual(result.testsRun, 1)
        self.assertEqual(result.shouldStop, False)

        result.stopTest(test)

        # Same tests as above; make sure nothing has changed
        self.failUnless(result.wasSuccessful())
        self.assertEqual(len(result.errors), 0)
        self.assertEqual(len(result.failures), 0)
        self.assertEqual(result.testsRun, 1)
        self.assertEqual(result.shouldStop, False)

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

        self.failUnless(result.wasSuccessful())
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

        self.failIf(result.wasSuccessful())
        self.assertEqual(len(result.errors), 0)
        self.assertEqual(len(result.failures), 1)
        self.assertEqual(result.testsRun, 1)
        self.assertEqual(result.shouldStop, False)

        test_case, formatted_exc = result.failures[0]
        self.failUnless(test_case is test)
        self.failUnless(isinstance(formatted_exc, str))

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

        self.failIf(result.wasSuccessful())
        self.assertEqual(len(result.errors), 1)
        self.assertEqual(len(result.failures), 0)
        self.assertEqual(result.testsRun, 1)
        self.assertEqual(result.shouldStop, False)

        test_case, formatted_exc = result.errors[0]
        self.failUnless(test_case is test)
        self.failUnless(isinstance(formatted_exc, str))

### Support code for Test_TestCase
################################################################

class Foo(unittest.TestCase):
    def runTest(self): pass
    def test1(self): pass

class Bar(Foo):
    def test2(self): pass

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

        class Foo(unittest.TestCase):
            def setUp(self):
                events.append('setUp')
                raise RuntimeError('raised by Foo.setUp')

            def test(self):
                events.append('test')

            def tearDown(self):
                events.append('tearDown')

        Foo('test').run(result)
        expected = ['startTest', 'setUp', 'addError', 'stopTest']
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

        class Foo(unittest.TestCase):
            def setUp(self):
                events.append('setUp')

            def test(self):
                events.append('test')
                raise RuntimeError('raised by Foo.test')

            def tearDown(self):
                events.append('tearDown')

        expected = ['startTest', 'setUp', 'test', 'addError', 'tearDown',
                    'stopTest']
        Foo('test').run(result)
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

        class Foo(unittest.TestCase):
            def setUp(self):
                events.append('setUp')

            def test(self):
                events.append('test')
                self.fail('raised by Foo.test')

            def tearDown(self):
                events.append('tearDown')

        expected = ['startTest', 'setUp', 'test', 'addFailure', 'tearDown',
                    'stopTest']
        Foo('test').run(result)
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

        class Foo(unittest.TestCase):
            def setUp(self):
                events.append('setUp')

            def test(self):
                events.append('test')

            def tearDown(self):
                events.append('tearDown')
                raise RuntimeError('raised by Foo.tearDown')

        Foo('test').run(result)
        expected = ['startTest', 'setUp', 'test', 'tearDown', 'addError',
                    'stopTest']
        self.assertEqual(events, expected)

    # "This class attribute gives the exception raised by the test() method.
    # If a test framework needs to use a specialized exception, possibly to
    # carry additional information, it must subclass this exception in
    # order to ``play fair'' with the framework.  The initial value of this
    # attribute is AssertionError"
    def test_failureException__default(self):
        class Foo(unittest.TestCase):
            def test(self):
                pass

        self.failUnless(Foo('test').failureException is AssertionError)

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

        self.failUnless(Foo('test').failureException is RuntimeError)


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

        self.failUnless(Foo('test').failureException is RuntimeError)


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

        self.failUnless(isinstance(Foo().id(), str))

    # "Returns a one-line description of the test, or None if no description
    # has been provided. The default implementation of this method returns
    # the first line of the test method's docstring, if available, or None."
    def test_shortDescription__no_docstring(self):
        class Foo(unittest.TestCase):
            def runTest(self):
                pass

        self.assertEqual(Foo().shortDescription(), None)

    # "Returns a one-line description of the test, or None if no description
    # has been provided. The default implementation of this method returns
    # the first line of the test method's docstring, if available, or None."
    def test_shortDescription__singleline_docstring(self):
        class Foo(unittest.TestCase):
            def runTest(self):
                "this tests foo"
                pass

        self.assertEqual(Foo().shortDescription(), "this tests foo")

    # "Returns a one-line description of the test, or None if no description
    # has been provided. The default implementation of this method returns
    # the first line of the test method's docstring, if available, or None."
    def test_shortDescription__multiline_docstring(self):
        class Foo(unittest.TestCase):
            def runTest(self):
                """this tests foo
                blah, bar and baz are also tested"""
                pass

        self.assertEqual(Foo().shortDescription(), "this tests foo")

    # "If result is omitted or None, a temporary result object is created
    # and used, but is not made available to the caller"
    def test_run__uses_defaultTestResult(self):
        events = []

        class Foo(unittest.TestCase):
            def test(self):
                events.append('test')

            def defaultTestResult(self):
                return LoggingResult(events)

        # Make run() find a result object on its own
        Foo('test').run()

        expected = ['startTest', 'test', 'stopTest']
        self.assertEqual(events, expected)

class Test_Assertions(TestCase):
    def test_AlmostEqual(self):
        self.failUnlessAlmostEqual(1.00000001, 1.0)
        self.failIfAlmostEqual(1.0000001, 1.0)
        self.assertRaises(AssertionError,
                          self.failUnlessAlmostEqual, 1.0000001, 1.0)
        self.assertRaises(AssertionError,
                          self.failIfAlmostEqual, 1.00000001, 1.0)

        self.failUnlessAlmostEqual(1.1, 1.0, places=0)
        self.assertRaises(AssertionError,
                          self.failUnlessAlmostEqual, 1.1, 1.0, places=1)

        self.failUnlessAlmostEqual(0, .1+.1j, places=0)
        self.failIfAlmostEqual(0, .1+.1j, places=1)
        self.assertRaises(AssertionError,
                          self.failUnlessAlmostEqual, 0, .1+.1j, places=1)
        self.assertRaises(AssertionError,
                          self.failIfAlmostEqual, 0, .1+.1j, places=0)

######################################################################
## Main
######################################################################

def test_main():
    test_support.run_unittest(Test_TestCase, Test_TestLoader,
        Test_TestSuite, Test_TestResult, Test_FunctionTestCase,
        Test_Assertions)

if __name__ == "__main__":
    test_main()
