import collections
import email
import os
import re
import unicodedata
import unittest
from curses.ascii import controlnames
from email.message import Message
from email._policybase import compat32
from test.support import load_package_tests
from test.test_email import __file__ as landmark
from test.test_email.params import C, params_map, ParamsMixin

# Load all tests in package
def load_tests(*args):
    return load_package_tests(os.path.dirname(__file__), *args)


# helper code used by a number of test modules.

def openfile(filename, *args, **kws):
    path = os.path.join(os.path.dirname(landmark), 'data', filename)
    return open(path, *args, **kws)

def charname(c):
    try:
        n = unicodedata.name(c).lower().replace(' ', '_').replace('-', '_')
    except ValueError:
        try:
            n = controlnames[ord(c)]
        except IndexError:
            assert c == '\x7F'
            return 'DEL'
    return n

def for_each_character(chars, skip=''):
    """Create a filter that expands each input into a test per character.

    chars should be an iterable of characters (eg a string), as should skip.

    For each character in chars that is not in skip, the filter should process
    all arguments and keywords, creating a new call spec.  For any objects and
    (recursively} sub-objects found that have a 'format' attribute, replace the
    object in the new call spec with the results of calling the object's format
    method, passing the method three keyword arguments: 'char', set to the
    character, 'echar', set to the character passed through re.escape, and
    'erchar', set to the repr of the character (without the quotes) passed
    through re.escape.

    Process any dictionary object's values, but not its keys.  Assume that any
    other object that is an iterator can be recreated by passing its type a
    list of objects.

    Return the character name as derived from unicodedata or the curses ascii
    module as as the name string to be added to the test name.

    """
    chars = {charname(v): v for v in chars if v not in skip}
    @params_map
    def for_each_character_in(*args, **kw):
        for name, c in chars.items():
            subs = dict(
                char=c,
                echar=re.escape(c),
                erchar=re.escape(repr(c)[1:-1]),
                )
            yield name, C(*args, **kw).fmtall(**subs)
    return for_each_character_in


# Base test class
class TestEmailBase(ParamsMixin, unittest.TestCase):

    # XXX XXX temporary usability hack, edit this out before publishing PR.
    def __str__(self):
        from unittest.util import strclass
        return "%s.%s" % (strclass(self.__class__), self._testMethodName)

    maxDiff = None
    # Currently the default policy is compat32.  By setting that as the default
    # here we make minimal changes in the test_email tests compared to their
    # pre-3.3 state.
    policy = compat32
    # Likewise, the default message object is Message.
    message = Message

    def __init__(self, *args, **kw):
        super().__init__(*args, **kw)
        self.addTypeEqualityFunc(bytes, self.assertBytesEqual)

    # Backward compatibility to minimize test_email test changes.
    ndiffAssertEqual = unittest.TestCase.assertEqual

    def _msgobj(self, filename):
        with openfile(filename, encoding="utf-8") as fp:
            return email.message_from_file(fp, policy=self.policy)

    def _str_msg(self, string, message=None, policy=None):
        if policy is None:
            policy = self.policy
        if message is None:
            message = self.message
        return email.message_from_string(string, message, policy=policy)

    def _bytes_msg(self, bytestring, message=None, policy=None):
        if policy is None:
            policy = self.policy
        if message is None:
            message = self.message
        return email.message_from_bytes(bytestring, message, policy=policy)

    def _make_message(self):
        return self.message(policy=self.policy)

    def _bytes_repr(self, b):
        return [repr(x) for x in b.splitlines(keepends=True)]

    def assertBytesEqual(self, first, second, msg):
        """Our byte strings are really encoded strings; improve diff output"""
        self.assertEqual(self._bytes_repr(first), self._bytes_repr(second))

    def assertDefectsMatch(self, actual, expected):
        """Assert list of defects matches a list of expected defect patterns

        actual should be a list of actual defect instances.  expected should
        a list of patterns.  Match the patterns against the actual list,
        and report any defects that do not match a pattern or any patterns
        that do not match a defect.  Matching must be one to one: if there
        are two identical defects in the actual list, it should be an error
        if there are not two patterns that match those defects in the
        expected list.

        A pattern can be one of three things:
            1) a defect class (eg: InvalidHeaderDefect)
            2) a tuple of (defect_class, regex), where the regex must
                match the message produced by calling str on the actual defect
            3) a tuple of (callable, *args) where calling the callable
                with the args must produce a tuple as in (2).

        """
        aleft = list(actual)
        eleft = []
        for x in expected:
            p = None
            while not p:
                if type(x) is type:
                    p = (x, '.*')
                elif not hasattr(x, '__getitem__'):
                    raise ValueError(f'invalid defect pattern: {x!r}')
                elif type(x[0]) is type:
                    p = x
                elif callable(x[0]):
                    x = x[0](*x[1:])
                else:
                    raise ValueError(f'invalid defect pattern: {x!r}')
            eleft.append(p)
        for t, s in list(eleft):
            for a in aleft:
                if type(a) == t and re.search(s, str(a), flags=re.I):
                    eleft.remove((t, s))
                    aleft.remove(a)
                    break
        if eleft or aleft:
            areprs = [repr((type(a), str(a))) for a in aleft]
            ereprs = [repr(e) for e in eleft]
            matched = f"{len(actual) - len(aleft)} defects matched"
            if len(eleft) == len(aleft):
                raise self.failureException(
                    f"{matched}, {len(aleft)} defects did not match:"
                    f"\n  unmatched expected:\n    {'\n    '.join(ereprs)}"
                    f"\n  unmatched actual:\n    {'\n    '.join(areprs)}"
                    )
            if len(eleft) == 0:
                raise self.failureException(
                    f"{matched}, {len(aleft)} extra defects:"
                    f"\n  {'\n  '.join(areprs)}"
                    )
            if len(aleft) == 0:
                raise self.failureException(
                    f"{matched}, {len(eleft)} missing defects:"
                    f"\n  {'\n  '.join(ereprs)}"
                    )
            else:
                raise self.failureException(
                    f"Expected {len(expected)} defects but got {len(actual)};"
                    f" {matched}, {len(eleft)} missing, {len(aleft)} extra:"
                    f"\n  unmatched actual:\n    {'\n    '.join(areprs)}"
                    f"\n  unmatched expected:\n    {'\n    '.join(ereprs)}"
                    )

    # XXX assertDefectsEqual can go away when it is no longer used.
    assertDefectsEqual = assertDefectsMatch


# XXX Don't use this for new tests, use params instead.  @parameterized will be
# deprecated and removed eventually.
def parameterize(cls):
    """A test method parameterization class decorator.

    Parameters are specified as the value of a class attribute that ends with
    the string '_params'.  Call the portion before '_params' the prefix.  Then
    a method to be parameterized must have the same prefix, the string
    '_as_', and an arbitrary suffix.

    The value of the _params attribute may be either a dictionary or a list.
    The values in the dictionary and the elements of the list may either be
    single values, or a list.  If single values, they are turned into single
    element tuples.  However derived, the resulting sequence is passed via
    *args to the parameterized test function.

    In a _params dictionary, the keys become part of the name of the generated
    tests.  In a _params list, the values in the list are converted into a
    string by joining the string values of the elements of the tuple by '_' and
    converting any blanks into '_'s, and this become part of the name.
    The  full name of a generated test is a 'test_' prefix, the portion of the
    test function name after the  '_as_' separator, plus an '_', plus the name
    derived as explained above.

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

    The values in the params dict can be a single value, a tuple, or a
    dict.  If a single value of a tuple, it is passed to the test function
    as positional arguments.  If a dict, it is a passed via **kw.

    """
    paramdicts = {}
    testers = collections.defaultdict(list)
    for name, attr in cls.__dict__.items():
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
        if '_as_' in name:
            testers[name.split('_as_')[0] + '_as_'].append(name)
    testfuncs = {}
    for name in paramdicts:
        if name not in testers:
            raise ValueError("No tester found for {}".format(name))
    for name in testers:
        if name not in paramdicts:
            raise ValueError("No params found for {}".format(name))
    for name, attr in cls.__dict__.items():
        for paramsname, paramsdict in paramdicts.items():
            if name.startswith(paramsname):
                testnameroot = 'test_' + name[len(paramsname):]
                for paramname, params in paramsdict.items():
                    if hasattr(params, 'keys'):
                        test = (lambda self, name=name, params=params:
                                    getattr(self, name)(**params))
                    else:
                        test = (lambda self, name=name, params=params:
                                        getattr(self, name)(*params))
                    testname = testnameroot + '_' + paramname
                    test.__name__ = testname
                    testfuncs[testname] = test
    for key, value in testfuncs.items():
        setattr(cls, key, value)
    return cls
