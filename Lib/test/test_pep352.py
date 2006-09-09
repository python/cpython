import unittest
import __builtin__
import exceptions
import warnings
from test.test_support import run_unittest
import os
from platform import system as platform_system

class ExceptionClassTests(unittest.TestCase):

    """Tests for anything relating to exception objects themselves (e.g.,
    inheritance hierarchy)"""

    def test_builtins_new_style(self):
        self.failUnless(issubclass(Exception, object))

    def verify_instance_interface(self, ins):
        for attr in ("args", "message", "__str__", "__repr__", "__getitem__"):
            self.failUnless(hasattr(ins, attr), "%s missing %s attribute" %
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

    def test_interface_single_arg(self):
        # Make sure interface works properly when given a single argument
        arg = "spam"
        exc = Exception(arg)
        results = ([len(exc.args), 1], [exc.args[0], arg], [exc.message, arg],
                [str(exc), str(arg)], [unicode(exc), unicode(arg)],
            [repr(exc), exc.__class__.__name__ + repr(exc.args)], [exc[0], arg])
        self.interface_test_driver(results)

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

    def test_interface_no_arg(self):
        # Make sure that with no args that interface is correct
        exc = Exception()
        results = ([len(exc.args), 0], [exc.args, tuple()], [exc.message, ''],
                [str(exc), ''], [unicode(exc), u''],
                [repr(exc), exc.__class__.__name__ + '()'], [True, True])
        self.interface_test_driver(results)

class UsageTests(unittest.TestCase):

    """Test usage of exceptions"""

    def setUp(self):
        self._filters = warnings.filters[:]

    def tearDown(self):
        warnings.filters = self._filters[:]

    def test_raise_classic(self):
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
            self.fail("unable to raise class class instance")

    def test_raise_new_style_non_exception(self):
        class NewStyleClass(object):
            pass
        try:
            raise NewStyleClass
        except TypeError:
            pass
        except:
            self.fail("unable to raise new-style class")
        try:
            raise NewStyleClass()
        except TypeError:
            pass
        except:
            self.fail("unable to raise new-style class instance")

    def test_raise_string(self):
        warnings.resetwarnings()
        warnings.filterwarnings("error")
        try:
            raise "spam"
        except DeprecationWarning:
            pass
        except:
            self.fail("raising a string did not cause a DeprecationWarning")

    def test_catch_string(self):
        # Test will be pertinent when catching exceptions raises a
        #   DeprecationWarning
        warnings.filterwarnings("ignore", "raising")
        str_exc = "spam"
        try:
            raise str_exc
        except str_exc:
            pass
        except:
            self.fail("catching a string exception failed")

def test_main():
    run_unittest(ExceptionClassTests, UsageTests)



if __name__ == '__main__':
    test_main()
