import unittest
import __builtin__
import exceptions
import warnings
from test.test_support import run_unittest, check_warnings
import os
import sys
from platform import system as platform_system

DEPRECATION_WARNINGS = ["BaseException.message has been deprecated"]

if sys.py3kwarning:
    DEPRECATION_WARNINGS.extend(
        ["exceptions must derive from BaseException",
         "catching classes that don't inherit from BaseException is not allowed",
         "__get(item|slice)__ not supported for exception classes"])

_deprecations = [(msg, DeprecationWarning) for msg in DEPRECATION_WARNINGS]

# Silence Py3k and other deprecation warnings
def ignore_deprecation_warnings(func):
    """Ignore the known DeprecationWarnings."""
    def wrapper(*args, **kw):
        with check_warnings(*_deprecations, quiet=True):
            return func(*args, **kw)
    return wrapper

class ExceptionClassTests(unittest.TestCase):

    """Tests for anything relating to exception objects themselves (e.g.,
    inheritance hierarchy)"""

    def test_builtins_new_style(self):
        self.failUnless(issubclass(Exception, object))

    @ignore_deprecation_warnings
    def verify_instance_interface(self, ins):
        for attr in ("args", "message", "__str__", "__repr__", "__getitem__"):
            self.assertTrue(hasattr(ins, attr),
                            "%s missing %s attribute" %
                            (ins.__class__.__name__, attr))

    def test_inheritance(self):
        # Make sure the inheritance hierarchy matches the documentation
        exc_set = set(x for x in dir(exceptions) if not x.startswith('_'))
        inheritance_tree = open(os.path.join(os.path.split(__file__)[0],
                                                'exception_hierarchy.txt'))
        try:
            superclass_name = inheritance_tree.readline().rstrip()
            try:
                last_exc = getattr(__builtin__, superclass_name)
            except AttributeError:
                self.fail("base class %s not a built-in" % superclass_name)
            self.failUnless(superclass_name in exc_set)
            exc_set.discard(superclass_name)
            superclasses = []  # Loop will insert base exception
            last_depth = 0
            for exc_line in inheritance_tree:
                exc_line = exc_line.rstrip()
                depth = exc_line.rindex('-')
                exc_name = exc_line[depth+2:]  # Slice past space
                if '(' in exc_name:
                    paren_index = exc_name.index('(')
                    platform_name = exc_name[paren_index+1:-1]
                    exc_name = exc_name[:paren_index-1]  # Slice off space
                    if platform_system() != platform_name:
                        exc_set.discard(exc_name)
                        continue
                if '[' in exc_name:
                    left_bracket = exc_name.index('[')
                    exc_name = exc_name[:left_bracket-1]  # cover space
                try:
                    exc = getattr(__builtin__, exc_name)
                except AttributeError:
                    self.fail("%s not a built-in exception" % exc_name)
                if last_depth < depth:
                    superclasses.append((last_depth, last_exc))
                elif last_depth > depth:
                    while superclasses[-1][0] >= depth:
                        superclasses.pop()
                self.failUnless(issubclass(exc, superclasses[-1][1]),
                "%s is not a subclass of %s" % (exc.__name__,
                    superclasses[-1][1].__name__))
                try:  # Some exceptions require arguments; just skip them
                    self.verify_instance_interface(exc())
                except TypeError:
                    pass
                self.failUnless(exc_name in exc_set)
                exc_set.discard(exc_name)
                last_exc = exc
                last_depth = depth
        finally:
            inheritance_tree.close()
        self.failUnlessEqual(len(exc_set), 0, "%s not accounted for" % exc_set)

    interface_tests = ("length", "args", "message", "str", "unicode", "repr",
            "indexing")

    def interface_test_driver(self, results):
        for test_name, (given, expected) in zip(self.interface_tests, results):
            self.failUnlessEqual(given, expected, "%s: %s != %s" % (test_name,
                given, expected))

    @ignore_deprecation_warnings
    def test_interface_single_arg(self):
        # Make sure interface works properly when given a single argument
        arg = "spam"
        exc = Exception(arg)
        results = ([len(exc.args), 1], [exc.args[0], arg], [exc.message, arg],
                   [str(exc), str(arg)], [unicode(exc), unicode(arg)],
                   [repr(exc), exc.__class__.__name__ + repr(exc.args)],
                   [exc[0], arg])
        self.interface_test_driver(results)

    @ignore_deprecation_warnings
    def test_interface_multi_arg(self):
        # Make sure interface correct when multiple arguments given
        arg_count = 3
        args = tuple(range(arg_count))
        exc = Exception(*args)
        results = ([len(exc.args), arg_count], [exc.args, args],
                   [exc.message, ''], [str(exc), str(args)],
                   [unicode(exc), unicode(args)],
                   [repr(exc), exc.__class__.__name__ + repr(exc.args)],
                   [exc[-1], args[-1]])
        self.interface_test_driver(results)

    @ignore_deprecation_warnings
    def test_interface_no_arg(self):
        # Make sure that with no args that interface is correct
        exc = Exception()
        results = ([len(exc.args), 0], [exc.args, tuple()],
                   [exc.message, ''],
                   [str(exc), ''], [unicode(exc), u''],
                   [repr(exc), exc.__class__.__name__ + '()'], [True, True])
        self.interface_test_driver(results)


    def test_message_deprecation(self):
        # As of Python 2.6, BaseException.message is deprecated.
        with check_warnings(("", DeprecationWarning)):
            BaseException().message


class UsageTests(unittest.TestCase):

    """Test usage of exceptions"""

    def raise_fails(self, object_):
        """Make sure that raising 'object_' triggers a TypeError."""
        try:
            raise object_
        except TypeError:
            return  # What is expected.
        self.fail("TypeError expected for raising %s" % type(object_))

    def catch_fails(self, object_):
        """Catching 'object_' should raise a TypeError."""
        try:
            try:
                raise StandardError
            except object_:
                pass
        except TypeError:
            pass
        except StandardError:
            self.fail("TypeError expected when catching %s" % type(object_))

        try:
            try:
                raise StandardError
            except (object_,):
                pass
        except TypeError:
            return
        except StandardError:
            self.fail("TypeError expected when catching %s as specified in a "
                        "tuple" % type(object_))

    @ignore_deprecation_warnings
    def test_raise_classic(self):
        # Raising a classic class is okay (for now).
        class ClassicClass:
            pass
        try:
            raise ClassicClass
        except ClassicClass:
            pass
        except:
            self.fail("unable to raise classic class")
        try:
            raise ClassicClass()
        except ClassicClass:
            pass
        except:
            self.fail("unable to raise classic class instance")

    def test_raise_new_style_non_exception(self):
        # You cannot raise a new-style class that does not inherit from
        # BaseException; the ability was not possible until BaseException's
        # introduction so no need to support new-style objects that do not
        # inherit from it.
        class NewStyleClass(object):
            pass
        self.raise_fails(NewStyleClass)
        self.raise_fails(NewStyleClass())

    def test_raise_string(self):
        # Raising a string raises TypeError.
        self.raise_fails("spam")

    def test_catch_string(self):
        # Catching a string should trigger a DeprecationWarning.
        with warnings.catch_warnings():
            warnings.resetwarnings()
            warnings.filterwarnings("error")
            str_exc = "spam"
            try:
                try:
                    raise StandardError
                except str_exc:
                    pass
            except DeprecationWarning:
                pass
            except StandardError:
                self.fail("catching a string exception did not raise "
                            "DeprecationWarning")
            # Make sure that even if the string exception is listed in a tuple
            # that a warning is raised.
            try:
                try:
                    raise StandardError
                except (AssertionError, str_exc):
                    pass
            except DeprecationWarning:
                pass
            except StandardError:
                self.fail("catching a string exception specified in a tuple did "
                            "not raise DeprecationWarning")


def test_main():
    run_unittest(ExceptionClassTests, UsageTests)



if __name__ == '__main__':
    test_main()
