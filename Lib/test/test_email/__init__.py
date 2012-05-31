import os
import sys
import unittest
import test.support
import email
from email.message import Message
from email._policybase import compat32
from test.test_email import __file__ as landmark

# Run all tests in package for '-m unittest test.test_email'
def load_tests(loader, standard_tests, pattern):
    this_dir = os.path.dirname(__file__)
    if pattern is None:
        pattern = "test*"
    package_tests = loader.discover(start_dir=this_dir, pattern=pattern)
    standard_tests.addTests(package_tests)
    return standard_tests


# used by regrtest and __main__.
def test_main():
    here = os.path.dirname(__file__)
    # Unittest mucks with the path, so we have to save and restore
    # it to keep regrtest happy.
    savepath = sys.path[:]
    test.support._run_suite(unittest.defaultTestLoader.discover(here))
    sys.path[:] = savepath


# helper code used by a number of test modules.

def openfile(filename, *args, **kws):
    path = os.path.join(os.path.dirname(landmark), 'data', filename)
    return open(path, *args, **kws)


# Base test class
class TestEmailBase(unittest.TestCase):

    maxDiff = None
    # Currently the default policy is compat32.  By setting that as the default
    # here we make minimal changes in the test_email tests compared to their
    # pre-3.3 state.
    policy = compat32

    def __init__(self, *args, **kw):
        super().__init__(*args, **kw)
        self.addTypeEqualityFunc(bytes, self.assertBytesEqual)

    # Backward compatibility to minimize test_email test changes.
    ndiffAssertEqual = unittest.TestCase.assertEqual

    def _msgobj(self, filename):
        with openfile(filename) as fp:
            return email.message_from_file(fp, policy=self.policy)

    def _str_msg(self, string, message=Message, policy=None):
        if policy is None:
            policy = self.policy
        return email.message_from_string(string, message, policy=policy)

    def _bytes_repr(self, b):
        return [repr(x) for x in b.splitlines(keepends=True)]

    def assertBytesEqual(self, first, second, msg):
        """Our byte strings are really encoded strings; improve diff output"""
        self.assertEqual(self._bytes_repr(first), self._bytes_repr(second))

    def assertDefectsEqual(self, actual, expected):
        self.assertEqual(len(actual), len(expected), actual)
        for i in range(len(actual)):
            self.assertIsInstance(actual[i], expected[i],
                                    'item {}'.format(i))


# Metaclass to allow for parameterized tests
class Parameterized(type):

    """Provide a test method parameterization facility.

    Parameters are specified as the value of a class attribute that ends with
    the string '_params'.  Call the portion before '_params' the prefix.  Then
    a method to be parameterized must have the same prefix, the string
    '_as_', and an arbitrary suffix.

    The value of the _params attribute may be either a dictionary or a list.
    The values in the dictionary and the elements of the list may either be
    single values, or a list.  If single values, they are turned into single
    element tuples.  However derived, the resulting sequence is passed via
    *args to the parameterized test function.

    In a _params dictioanry, the keys become part of the name of the generated
    tests.  In a _params list, the values in the list are converted into a
    string by joining the string values of the elements of the tuple by '_' and
    converting any blanks into '_'s, and this become part of the name.  The
    full name of a generated test is the portion of the _params name before the
    '_params' portion, plus an '_', plus the name derived as explained above.

    For example, if we have:

        count_params = range(2)

        def count_as_foo_arg(self, foo):
            self.assertEqual(foo+1, myfunc(foo))

    we will get parameterized test methods named:
        test_foo_arg_0
        test_foo_arg_1
        test_foo_arg_2

    Or we could have:

        example_params = {'foo': ('bar', 1), 'bing': ('bang', 2)}

        def example_as_myfunc_input(self, name, count):
            self.assertEqual(name+str(count), myfunc(name, count))

    and get:
        test_myfunc_input_foo
        test_myfunc_input_bing

    Note: if and only if the generated test name is a valid identifier can it
    be used to select the test individually from the unittest command line.

    """

    def __new__(meta, classname, bases, classdict):
        paramdicts = {}
        for name, attr in classdict.items():
            if name.endswith('_params'):
                if not hasattr(attr, 'keys'):
                    d = {}
                    for x in attr:
                        if not hasattr(x, '__iter__'):
                            x = (x,)
                        n = '_'.join(str(v) for v in x).replace(' ', '_')
                        d[n] = x
                    attr = d
                paramdicts[name[:-7] + '_as_'] = attr
        testfuncs = {}
        for name, attr in classdict.items():
            for paramsname, paramsdict in paramdicts.items():
                if name.startswith(paramsname):
                    testnameroot = 'test_' + name[len(paramsname):]
                    for paramname, params in paramsdict.items():
                        test = (lambda self, name=name, params=params:
                                        getattr(self, name)(*params))
                        testname = testnameroot + '_' + paramname
                        test.__name__ = testname
                        testfuncs[testname] = test
        classdict.update(testfuncs)
        return super().__new__(meta, classname, bases, classdict)
