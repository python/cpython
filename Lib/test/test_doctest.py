"""
Test script for doctest.
"""

from test import support
import doctest
import functools
import os
import sys
import importlib
import unittest
import tempfile

# NOTE: There are some additional tests relating to interaction with
#       zipimport in the test_zipimport_support test module.

######################################################################
## Sample Objects (used by test cases)
######################################################################

def sample_func(v):
    """
    Blah blah

    >>> print(sample_func(22))
    44

    Yee ha!
    """
    return v+v

class SampleClass:
    """
    >>> print(1)
    1

    >>> # comments get ignored.  so are empty PS1 and PS2 prompts:
    >>>
    ...

    Multiline example:
    >>> sc = SampleClass(3)
    >>> for i in range(10):
    ...     sc = sc.double()
    ...     print(' ', sc.get(), sep='', end='')
     6 12 24 48 96 192 384 768 1536 3072
    """
    def __init__(self, val):
        """
        >>> print(SampleClass(12).get())
        12
        """
        self.val = val

    def double(self):
        """
        >>> print(SampleClass(12).double().get())
        24
        """
        return SampleClass(self.val + self.val)

    def get(self):
        """
        >>> print(SampleClass(-5).get())
        -5
        """
        return self.val

    def a_staticmethod(v):
        """
        >>> print(SampleClass.a_staticmethod(10))
        11
        """
        return v+1
    a_staticmethod = staticmethod(a_staticmethod)

    def a_classmethod(cls, v):
        """
        >>> print(SampleClass.a_classmethod(10))
        12
        >>> print(SampleClass(0).a_classmethod(10))
        12
        """
        return v+2
    a_classmethod = classmethod(a_classmethod)

    a_property = property(get, doc="""
        >>> print(SampleClass(22).a_property)
        22
        """)

    class NestedClass:
        """
        >>> x = SampleClass.NestedClass(5)
        >>> y = x.square()
        >>> print(y.get())
        25
        """
        def __init__(self, val=0):
            """
            >>> print(SampleClass.NestedClass().get())
            0
            """
            self.val = val
        def square(self):
            return SampleClass.NestedClass(self.val*self.val)
        def get(self):
            return self.val

class SampleNewStyleClass(object):
    r"""
    >>> print('1\n2\n3')
    1
    2
    3
    """
    def __init__(self, val):
        """
        >>> print(SampleNewStyleClass(12).get())
        12
        """
        self.val = val

    def double(self):
        """
        >>> print(SampleNewStyleClass(12).double().get())
        24
        """
        return SampleNewStyleClass(self.val + self.val)

    def get(self):
        """
        >>> print(SampleNewStyleClass(-5).get())
        -5
        """
        return self.val

######################################################################
## Fake stdin (for testing interactive debugging)
######################################################################

class _FakeInput:
    """
    A fake input stream for pdb's interactive debugger.  Whenever a
    line is read, print it (to simulate the user typing it), and then
    return it.  The set of lines to return is specified in the
    constructor; they should not have trailing newlines.
    """
    def __init__(self, lines):
        self.lines = lines

    def readline(self):
        line = self.lines.pop(0)
        print(line)
        return line+'\n'

######################################################################
## Test Cases
######################################################################

def test_Example(): r"""
Unit tests for the `Example` class.

Example is a simple container class that holds:
  - `source`: A source string.
  - `want`: An expected output string.
  - `exc_msg`: An expected exception message string (or None if no
    exception is expected).
  - `lineno`: A line number (within the docstring).
  - `indent`: The example's indentation in the input string.
  - `options`: An option dictionary, mapping option flags to True or
    False.

These attributes are set by the constructor.  `source` and `want` are
required; the other attributes all have default values:

    >>> example = doctest.Example('print(1)', '1\n')
    >>> (example.source, example.want, example.exc_msg,
    ...  example.lineno, example.indent, example.options)
    ('print(1)\n', '1\n', None, 0, 0, {})

The first three attributes (`source`, `want`, and `exc_msg`) may be
specified positionally; the remaining arguments should be specified as
keyword arguments:

    >>> exc_msg = 'IndexError: pop from an empty list'
    >>> example = doctest.Example('[].pop()', '', exc_msg,
    ...                           lineno=5, indent=4,
    ...                           options={doctest.ELLIPSIS: True})
    >>> (example.source, example.want, example.exc_msg,
    ...  example.lineno, example.indent, example.options)
    ('[].pop()\n', '', 'IndexError: pop from an empty list\n', 5, 4, {8: True})

The constructor normalizes the `source` string to end in a newline:

    Source spans a single line: no terminating newline.
    >>> e = doctest.Example('print(1)', '1\n')
    >>> e.source, e.want
    ('print(1)\n', '1\n')

    >>> e = doctest.Example('print(1)\n', '1\n')
    >>> e.source, e.want
    ('print(1)\n', '1\n')

    Source spans multiple lines: require terminating newline.
    >>> e = doctest.Example('print(1);\nprint(2)\n', '1\n2\n')
    >>> e.source, e.want
    ('print(1);\nprint(2)\n', '1\n2\n')

    >>> e = doctest.Example('print(1);\nprint(2)', '1\n2\n')
    >>> e.source, e.want
    ('print(1);\nprint(2)\n', '1\n2\n')

    Empty source string (which should never appear in real examples)
    >>> e = doctest.Example('', '')
    >>> e.source, e.want
    ('\n', '')

The constructor normalizes the `want` string to end in a newline,
unless it's the empty string:

    >>> e = doctest.Example('print(1)', '1\n')
    >>> e.source, e.want
    ('print(1)\n', '1\n')

    >>> e = doctest.Example('print(1)', '1')
    >>> e.source, e.want
    ('print(1)\n', '1\n')

    >>> e = doctest.Example('print', '')
    >>> e.source, e.want
    ('print\n', '')

The constructor normalizes the `exc_msg` string to end in a newline,
unless it's `None`:

    Message spans one line
    >>> exc_msg = 'IndexError: pop from an empty list'
    >>> e = doctest.Example('[].pop()', '', exc_msg)
    >>> e.exc_msg
    'IndexError: pop from an empty list\n'

    >>> exc_msg = 'IndexError: pop from an empty list\n'
    >>> e = doctest.Example('[].pop()', '', exc_msg)
    >>> e.exc_msg
    'IndexError: pop from an empty list\n'

    Message spans multiple lines
    >>> exc_msg = 'ValueError: 1\n  2'
    >>> e = doctest.Example('raise ValueError("1\n  2")', '', exc_msg)
    >>> e.exc_msg
    'ValueError: 1\n  2\n'

    >>> exc_msg = 'ValueError: 1\n  2\n'
    >>> e = doctest.Example('raise ValueError("1\n  2")', '', exc_msg)
    >>> e.exc_msg
    'ValueError: 1\n  2\n'

    Empty (but non-None) exception message (which should never appear
    in real examples)
    >>> exc_msg = ''
    >>> e = doctest.Example('raise X()', '', exc_msg)
    >>> e.exc_msg
    '\n'

Compare `Example`:
    >>> example = doctest.Example('print 1', '1\n')
    >>> same_example = doctest.Example('print 1', '1\n')
    >>> other_example = doctest.Example('print 42', '42\n')
    >>> example == same_example
    True
    >>> example != same_example
    False
    >>> hash(example) == hash(same_example)
    True
    >>> example == other_example
    False
    >>> example != other_example
    True
"""

def test_DocTest(): r"""
Unit tests for the `DocTest` class.

DocTest is a collection of examples, extracted from a docstring, along
with information about where the docstring comes from (a name,
filename, and line number).  The docstring is parsed by the `DocTest`
constructor:

    >>> docstring = '''
    ...     >>> print(12)
    ...     12
    ...
    ... Non-example text.
    ...
    ...     >>> print('another\\example')
    ...     another
    ...     example
    ... '''
    >>> globs = {} # globals to run the test in.
    >>> parser = doctest.DocTestParser()
    >>> test = parser.get_doctest(docstring, globs, 'some_test',
    ...                           'some_file', 20)
    >>> print(test)
    <DocTest some_test from some_file:20 (2 examples)>
    >>> len(test.examples)
    2
    >>> e1, e2 = test.examples
    >>> (e1.source, e1.want, e1.lineno)
    ('print(12)\n', '12\n', 1)
    >>> (e2.source, e2.want, e2.lineno)
    ("print('another\\example')\n", 'another\nexample\n', 6)

Source information (name, filename, and line number) is available as
attributes on the doctest object:

    >>> (test.name, test.filename, test.lineno)
    ('some_test', 'some_file', 20)

The line number of an example within its containing file is found by
adding the line number of the example and the line number of its
containing test:

    >>> test.lineno + e1.lineno
    21
    >>> test.lineno + e2.lineno
    26

If the docstring contains inconsistent leading whitespace in the
expected output of an example, then `DocTest` will raise a ValueError:

    >>> docstring = r'''
    ...       >>> print('bad\nindentation')
    ...       bad
    ...     indentation
    ...     '''
    >>> parser.get_doctest(docstring, globs, 'some_test', 'filename', 0)
    Traceback (most recent call last):
    ValueError: line 4 of the docstring for some_test has inconsistent leading whitespace: 'indentation'

If the docstring contains inconsistent leading whitespace on
continuation lines, then `DocTest` will raise a ValueError:

    >>> docstring = r'''
    ...       >>> print(('bad indentation',
    ...     ...          2))
    ...       ('bad', 'indentation')
    ...     '''
    >>> parser.get_doctest(docstring, globs, 'some_test', 'filename', 0)
    Traceback (most recent call last):
    ValueError: line 2 of the docstring for some_test has inconsistent leading whitespace: '...          2))'

If there's no blank space after a PS1 prompt ('>>>'), then `DocTest`
will raise a ValueError:

    >>> docstring = '>>>print(1)\n1'
    >>> parser.get_doctest(docstring, globs, 'some_test', 'filename', 0)
    Traceback (most recent call last):
    ValueError: line 1 of the docstring for some_test lacks blank after >>>: '>>>print(1)'

If there's no blank space after a PS2 prompt ('...'), then `DocTest`
will raise a ValueError:

    >>> docstring = '>>> if 1:\n...print(1)\n1'
    >>> parser.get_doctest(docstring, globs, 'some_test', 'filename', 0)
    Traceback (most recent call last):
    ValueError: line 2 of the docstring for some_test lacks blank after ...: '...print(1)'

Compare `DocTest`:

    >>> docstring = '''
    ...     >>> print 12
    ...     12
    ... '''
    >>> test = parser.get_doctest(docstring, globs, 'some_test',
    ...                           'some_test', 20)
    >>> same_test = parser.get_doctest(docstring, globs, 'some_test',
    ...                                'some_test', 20)
    >>> test == same_test
    True
    >>> test != same_test
    False
    >>> hash(test) == hash(same_test)
    True
    >>> docstring = '''
    ...     >>> print 42
    ...     42
    ... '''
    >>> other_test = parser.get_doctest(docstring, globs, 'other_test',
    ...                                 'other_file', 10)
    >>> test == other_test
    False
    >>> test != other_test
    True

Compare `DocTestCase`:

    >>> DocTestCase = doctest.DocTestCase
    >>> test_case = DocTestCase(test)
    >>> same_test_case = DocTestCase(same_test)
    >>> other_test_case = DocTestCase(other_test)
    >>> test_case == same_test_case
    True
    >>> test_case != same_test_case
    False
    >>> hash(test_case) == hash(same_test_case)
    True
    >>> test == other_test_case
    False
    >>> test != other_test_case
    True

"""

class test_DocTestFinder:
    def basics(): r"""
Unit tests for the `DocTestFinder` class.

DocTestFinder is used to extract DocTests from an object's docstring
and the docstrings of its contained objects.  It can be used with
modules, functions, classes, methods, staticmethods, classmethods, and
properties.

Finding Tests in Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~
For a function whose docstring contains examples, DocTestFinder.find()
will return a single test (for that function's docstring):

    >>> finder = doctest.DocTestFinder()

We'll simulate a __file__ attr that ends in pyc:

    >>> import test.test_doctest
    >>> old = test.test_doctest.__file__
    >>> test.test_doctest.__file__ = 'test_doctest.pyc'

    >>> tests = finder.find(sample_func)

    >>> print(tests)  # doctest: +ELLIPSIS
    [<DocTest sample_func from ...:21 (1 example)>]

The exact name depends on how test_doctest was invoked, so allow for
leading path components.

    >>> tests[0].filename # doctest: +ELLIPSIS
    '...test_doctest.py'

    >>> test.test_doctest.__file__ = old


    >>> e = tests[0].examples[0]
    >>> (e.source, e.want, e.lineno)
    ('print(sample_func(22))\n', '44\n', 3)

By default, tests are created for objects with no docstring:

    >>> def no_docstring(v):
    ...     pass
    >>> finder.find(no_docstring)
    []

However, the optional argument `exclude_empty` to the DocTestFinder
constructor can be used to exclude tests for objects with empty
docstrings:

    >>> def no_docstring(v):
    ...     pass
    >>> excl_empty_finder = doctest.DocTestFinder(exclude_empty=True)
    >>> excl_empty_finder.find(no_docstring)
    []

If the function has a docstring with no examples, then a test with no
examples is returned.  (This lets `DocTestRunner` collect statistics
about which functions have no tests -- but is that useful?  And should
an empty test also be created when there's no docstring?)

    >>> def no_examples(v):
    ...     ''' no doctest examples '''
    >>> finder.find(no_examples) # doctest: +ELLIPSIS
    [<DocTest no_examples from ...:1 (no examples)>]

Finding Tests in Classes
~~~~~~~~~~~~~~~~~~~~~~~~
For a class, DocTestFinder will create a test for the class's
docstring, and will recursively explore its contents, including
methods, classmethods, staticmethods, properties, and nested classes.

    >>> finder = doctest.DocTestFinder()
    >>> tests = finder.find(SampleClass)
    >>> for t in tests:
    ...     print('%2s  %s' % (len(t.examples), t.name))
     3  SampleClass
     3  SampleClass.NestedClass
     1  SampleClass.NestedClass.__init__
     1  SampleClass.__init__
     2  SampleClass.a_classmethod
     1  SampleClass.a_property
     1  SampleClass.a_staticmethod
     1  SampleClass.double
     1  SampleClass.get

New-style classes are also supported:

    >>> tests = finder.find(SampleNewStyleClass)
    >>> for t in tests:
    ...     print('%2s  %s' % (len(t.examples), t.name))
     1  SampleNewStyleClass
     1  SampleNewStyleClass.__init__
     1  SampleNewStyleClass.double
     1  SampleNewStyleClass.get

Finding Tests in Modules
~~~~~~~~~~~~~~~~~~~~~~~~
For a module, DocTestFinder will create a test for the class's
docstring, and will recursively explore its contents, including
functions, classes, and the `__test__` dictionary, if it exists:

    >>> # A module
    >>> import types
    >>> m = types.ModuleType('some_module')
    >>> def triple(val):
    ...     '''
    ...     >>> print(triple(11))
    ...     33
    ...     '''
    ...     return val*3
    >>> m.__dict__.update({
    ...     'sample_func': sample_func,
    ...     'SampleClass': SampleClass,
    ...     '__doc__': '''
    ...         Module docstring.
    ...             >>> print('module')
    ...             module
    ...         ''',
    ...     '__test__': {
    ...         'd': '>>> print(6)\n6\n>>> print(7)\n7\n',
    ...         'c': triple}})

    >>> finder = doctest.DocTestFinder()
    >>> # Use module=test.test_doctest, to prevent doctest from
    >>> # ignoring the objects since they weren't defined in m.
    >>> import test.test_doctest
    >>> tests = finder.find(m, module=test.test_doctest)
    >>> for t in tests:
    ...     print('%2s  %s' % (len(t.examples), t.name))
     1  some_module
     3  some_module.SampleClass
     3  some_module.SampleClass.NestedClass
     1  some_module.SampleClass.NestedClass.__init__
     1  some_module.SampleClass.__init__
     2  some_module.SampleClass.a_classmethod
     1  some_module.SampleClass.a_property
     1  some_module.SampleClass.a_staticmethod
     1  some_module.SampleClass.double
     1  some_module.SampleClass.get
     1  some_module.__test__.c
     2  some_module.__test__.d
     1  some_module.sample_func

Duplicate Removal
~~~~~~~~~~~~~~~~~
If a single object is listed twice (under different names), then tests
will only be generated for it once:

    >>> from test import doctest_aliases
    >>> assert doctest_aliases.TwoNames.f
    >>> assert doctest_aliases.TwoNames.g
    >>> tests = excl_empty_finder.find(doctest_aliases)
    >>> print(len(tests))
    2
    >>> print(tests[0].name)
    test.doctest_aliases.TwoNames

    TwoNames.f and TwoNames.g are bound to the same object.
    We can't guess which will be found in doctest's traversal of
    TwoNames.__dict__ first, so we have to allow for either.

    >>> tests[1].name.split('.')[-1] in ['f', 'g']
    True

Empty Tests
~~~~~~~~~~~
By default, an object with no doctests doesn't create any tests:

    >>> tests = doctest.DocTestFinder().find(SampleClass)
    >>> for t in tests:
    ...     print('%2s  %s' % (len(t.examples), t.name))
     3  SampleClass
     3  SampleClass.NestedClass
     1  SampleClass.NestedClass.__init__
     1  SampleClass.__init__
     2  SampleClass.a_classmethod
     1  SampleClass.a_property
     1  SampleClass.a_staticmethod
     1  SampleClass.double
     1  SampleClass.get

By default, that excluded objects with no doctests.  exclude_empty=False
tells it to include (empty) tests for objects with no doctests.  This feature
is really to support backward compatibility in what doctest.master.summarize()
displays.

    >>> tests = doctest.DocTestFinder(exclude_empty=False).find(SampleClass)
    >>> for t in tests:
    ...     print('%2s  %s' % (len(t.examples), t.name))
     3  SampleClass
     3  SampleClass.NestedClass
     1  SampleClass.NestedClass.__init__
     0  SampleClass.NestedClass.get
     0  SampleClass.NestedClass.square
     1  SampleClass.__init__
     2  SampleClass.a_classmethod
     1  SampleClass.a_property
     1  SampleClass.a_staticmethod
     1  SampleClass.double
     1  SampleClass.get

Turning off Recursion
~~~~~~~~~~~~~~~~~~~~~
DocTestFinder can be told not to look for tests in contained objects
using the `recurse` flag:

    >>> tests = doctest.DocTestFinder(recurse=False).find(SampleClass)
    >>> for t in tests:
    ...     print('%2s  %s' % (len(t.examples), t.name))
     3  SampleClass

Line numbers
~~~~~~~~~~~~
DocTestFinder finds the line number of each example:

    >>> def f(x):
    ...     '''
    ...     >>> x = 12
    ...
    ...     some text
    ...
    ...     >>> # examples are not created for comments & bare prompts.
    ...     >>>
    ...     ...
    ...
    ...     >>> for x in range(10):
    ...     ...     print(x, end=' ')
    ...     0 1 2 3 4 5 6 7 8 9
    ...     >>> x//2
    ...     6
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> [e.lineno for e in test.examples]
    [1, 9, 12]
"""

    if int.__doc__: # simple check for --without-doc-strings, skip if lacking
        def non_Python_modules(): r"""

Finding Doctests in Modules Not Written in Python
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
DocTestFinder can also find doctests in most modules not written in Python.
We'll use builtins as an example, since it almost certainly isn't written in
plain ol' Python and is guaranteed to be available.

    >>> import builtins
    >>> tests = doctest.DocTestFinder().find(builtins)
    >>> 800 < len(tests) < 820 # approximate number of objects with docstrings
    True
    >>> real_tests = [t for t in tests if len(t.examples) > 0]
    >>> len(real_tests) # objects that actually have doctests
    8
    >>> for t in real_tests:
    ...     print('{}  {}'.format(len(t.examples), t.name))
    ...
    1  builtins.bin
    3  builtins.float.as_integer_ratio
    2  builtins.float.fromhex
    2  builtins.float.hex
    1  builtins.hex
    1  builtins.int
    2  builtins.int.bit_length
    1  builtins.oct

Note here that 'bin', 'oct', and 'hex' are functions; 'float.as_integer_ratio',
'float.hex', and 'int.bit_length' are methods; 'float.fromhex' is a classmethod,
and 'int' is a type.
"""


class TestDocTestFinder(unittest.TestCase):

    def test_empty_namespace_package(self):
        pkg_name = 'doctest_empty_pkg'
        with tempfile.TemporaryDirectory() as parent_dir:
            pkg_dir = os.path.join(parent_dir, pkg_name)
            os.mkdir(pkg_dir)
            sys.path.append(parent_dir)
            try:
                mod = importlib.import_module(pkg_name)
            finally:
                support.forget(pkg_name)
                sys.path.pop()
            assert doctest.DocTestFinder().find(mod) == []


def test_DocTestParser(): r"""
Unit tests for the `DocTestParser` class.

DocTestParser is used to parse docstrings containing doctest examples.

The `parse` method divides a docstring into examples and intervening
text:

    >>> s = '''
    ...     >>> x, y = 2, 3  # no output expected
    ...     >>> if 1:
    ...     ...     print(x)
    ...     ...     print(y)
    ...     2
    ...     3
    ...
    ...     Some text.
    ...     >>> x+y
    ...     5
    ...     '''
    >>> parser = doctest.DocTestParser()
    >>> for piece in parser.parse(s):
    ...     if isinstance(piece, doctest.Example):
    ...         print('Example:', (piece.source, piece.want, piece.lineno))
    ...     else:
    ...         print('   Text:', repr(piece))
       Text: '\n'
    Example: ('x, y = 2, 3  # no output expected\n', '', 1)
       Text: ''
    Example: ('if 1:\n    print(x)\n    print(y)\n', '2\n3\n', 2)
       Text: '\nSome text.\n'
    Example: ('x+y\n', '5\n', 9)
       Text: ''

The `get_examples` method returns just the examples:

    >>> for piece in parser.get_examples(s):
    ...     print((piece.source, piece.want, piece.lineno))
    ('x, y = 2, 3  # no output expected\n', '', 1)
    ('if 1:\n    print(x)\n    print(y)\n', '2\n3\n', 2)
    ('x+y\n', '5\n', 9)

The `get_doctest` method creates a Test from the examples, along with the
given arguments:

    >>> test = parser.get_doctest(s, {}, 'name', 'filename', lineno=5)
    >>> (test.name, test.filename, test.lineno)
    ('name', 'filename', 5)
    >>> for piece in test.examples:
    ...     print((piece.source, piece.want, piece.lineno))
    ('x, y = 2, 3  # no output expected\n', '', 1)
    ('if 1:\n    print(x)\n    print(y)\n', '2\n3\n', 2)
    ('x+y\n', '5\n', 9)
"""

class test_DocTestRunner:
    def basics(): r"""
Unit tests for the `DocTestRunner` class.

DocTestRunner is used to run DocTest test cases, and to accumulate
statistics.  Here's a simple DocTest case we can use:

    >>> def f(x):
    ...     '''
    ...     >>> x = 12
    ...     >>> print(x)
    ...     12
    ...     >>> x//2
    ...     6
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]

The main DocTestRunner interface is the `run` method, which runs a
given DocTest case in a given namespace (globs).  It returns a tuple
`(f,t)`, where `f` is the number of failed tests and `t` is the number
of tried tests.

    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=3)

If any example produces incorrect output, then the test runner reports
the failure and proceeds to the next example:

    >>> def f(x):
    ...     '''
    ...     >>> x = 12
    ...     >>> print(x)
    ...     14
    ...     >>> x//2
    ...     6
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=True).run(test)
    ... # doctest: +ELLIPSIS
    Trying:
        x = 12
    Expecting nothing
    ok
    Trying:
        print(x)
    Expecting:
        14
    **********************************************************************
    File ..., line 4, in f
    Failed example:
        print(x)
    Expected:
        14
    Got:
        12
    Trying:
        x//2
    Expecting:
        6
    ok
    TestResults(failed=1, attempted=3)
"""
    def verbose_flag(): r"""
The `verbose` flag makes the test runner generate more detailed
output:

    >>> def f(x):
    ...     '''
    ...     >>> x = 12
    ...     >>> print(x)
    ...     12
    ...     >>> x//2
    ...     6
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]

    >>> doctest.DocTestRunner(verbose=True).run(test)
    Trying:
        x = 12
    Expecting nothing
    ok
    Trying:
        print(x)
    Expecting:
        12
    ok
    Trying:
        x//2
    Expecting:
        6
    ok
    TestResults(failed=0, attempted=3)

If the `verbose` flag is unspecified, then the output will be verbose
iff `-v` appears in sys.argv:

    >>> # Save the real sys.argv list.
    >>> old_argv = sys.argv

    >>> # If -v does not appear in sys.argv, then output isn't verbose.
    >>> sys.argv = ['test']
    >>> doctest.DocTestRunner().run(test)
    TestResults(failed=0, attempted=3)

    >>> # If -v does appear in sys.argv, then output is verbose.
    >>> sys.argv = ['test', '-v']
    >>> doctest.DocTestRunner().run(test)
    Trying:
        x = 12
    Expecting nothing
    ok
    Trying:
        print(x)
    Expecting:
        12
    ok
    Trying:
        x//2
    Expecting:
        6
    ok
    TestResults(failed=0, attempted=3)

    >>> # Restore sys.argv
    >>> sys.argv = old_argv

In the remaining examples, the test runner's verbosity will be
explicitly set, to ensure that the test behavior is consistent.
    """
    def exceptions(): r"""
Tests of `DocTestRunner`'s exception handling.

An expected exception is specified with a traceback message.  The
lines between the first line and the type/value may be omitted or
replaced with any other string:

    >>> def f(x):
    ...     '''
    ...     >>> x = 12
    ...     >>> print(x//0)
    ...     Traceback (most recent call last):
    ...     ZeroDivisionError: integer division or modulo by zero
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=2)

An example may not generate output before it raises an exception; if
it does, then the traceback message will not be recognized as
signaling an expected exception, so the example will be reported as an
unexpected exception:

    >>> def f(x):
    ...     '''
    ...     >>> x = 12
    ...     >>> print('pre-exception output', x//0)
    ...     pre-exception output
    ...     Traceback (most recent call last):
    ...     ZeroDivisionError: integer division or modulo by zero
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 4, in f
    Failed example:
        print('pre-exception output', x//0)
    Exception raised:
        ...
        ZeroDivisionError: integer division or modulo by zero
    TestResults(failed=1, attempted=2)

Exception messages may contain newlines:

    >>> def f(x):
    ...     r'''
    ...     >>> raise ValueError('multi\nline\nmessage')
    ...     Traceback (most recent call last):
    ...     ValueError: multi
    ...     line
    ...     message
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=1)

If an exception is expected, but an exception with the wrong type or
message is raised, then it is reported as a failure:

    >>> def f(x):
    ...     r'''
    ...     >>> raise ValueError('message')
    ...     Traceback (most recent call last):
    ...     ValueError: wrong message
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 3, in f
    Failed example:
        raise ValueError('message')
    Expected:
        Traceback (most recent call last):
        ValueError: wrong message
    Got:
        Traceback (most recent call last):
        ...
        ValueError: message
    TestResults(failed=1, attempted=1)

However, IGNORE_EXCEPTION_DETAIL can be used to allow a mismatch in the
detail:

    >>> def f(x):
    ...     r'''
    ...     >>> raise ValueError('message') #doctest: +IGNORE_EXCEPTION_DETAIL
    ...     Traceback (most recent call last):
    ...     ValueError: wrong message
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=1)

IGNORE_EXCEPTION_DETAIL also ignores difference in exception formatting
between Python versions. For example, in Python 2.x, the module path of
the exception is not in the output, but this will fail under Python 3:

    >>> def f(x):
    ...     r'''
    ...     >>> from http.client import HTTPException
    ...     >>> raise HTTPException('message')
    ...     Traceback (most recent call last):
    ...     HTTPException: message
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 4, in f
    Failed example:
        raise HTTPException('message')
    Expected:
        Traceback (most recent call last):
        HTTPException: message
    Got:
        Traceback (most recent call last):
        ...
        http.client.HTTPException: message
    TestResults(failed=1, attempted=2)

But in Python 3 the module path is included, and therefore a test must look
like the following test to succeed in Python 3. But that test will fail under
Python 2.

    >>> def f(x):
    ...     r'''
    ...     >>> from http.client import HTTPException
    ...     >>> raise HTTPException('message')
    ...     Traceback (most recent call last):
    ...     http.client.HTTPException: message
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=2)

However, with IGNORE_EXCEPTION_DETAIL, the module name of the exception
(or its unexpected absence) will be ignored:

    >>> def f(x):
    ...     r'''
    ...     >>> from http.client import HTTPException
    ...     >>> raise HTTPException('message') #doctest: +IGNORE_EXCEPTION_DETAIL
    ...     Traceback (most recent call last):
    ...     HTTPException: message
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=2)

The module path will be completely ignored, so two different module paths will
still pass if IGNORE_EXCEPTION_DETAIL is given. This is intentional, so it can
be used when exceptions have changed module.

    >>> def f(x):
    ...     r'''
    ...     >>> from http.client import HTTPException
    ...     >>> raise HTTPException('message') #doctest: +IGNORE_EXCEPTION_DETAIL
    ...     Traceback (most recent call last):
    ...     foo.bar.HTTPException: message
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=2)

But IGNORE_EXCEPTION_DETAIL does not allow a mismatch in the exception type:

    >>> def f(x):
    ...     r'''
    ...     >>> raise ValueError('message') #doctest: +IGNORE_EXCEPTION_DETAIL
    ...     Traceback (most recent call last):
    ...     TypeError: wrong type
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 3, in f
    Failed example:
        raise ValueError('message') #doctest: +IGNORE_EXCEPTION_DETAIL
    Expected:
        Traceback (most recent call last):
        TypeError: wrong type
    Got:
        Traceback (most recent call last):
        ...
        ValueError: message
    TestResults(failed=1, attempted=1)

If the exception does not have a message, you can still use
IGNORE_EXCEPTION_DETAIL to normalize the modules between Python 2 and 3:

    >>> def f(x):
    ...     r'''
    ...     >>> from http.client import HTTPException
    ...     >>> raise HTTPException() #doctest: +IGNORE_EXCEPTION_DETAIL
    ...     Traceback (most recent call last):
    ...     foo.bar.HTTPException
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=2)

Note that a trailing colon doesn't matter either:

    >>> def f(x):
    ...     r'''
    ...     >>> from http.client import HTTPException
    ...     >>> raise HTTPException() #doctest: +IGNORE_EXCEPTION_DETAIL
    ...     Traceback (most recent call last):
    ...     foo.bar.HTTPException:
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=2)

If an exception is raised but not expected, then it is reported as an
unexpected exception:

    >>> def f(x):
    ...     r'''
    ...     >>> 1//0
    ...     0
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 3, in f
    Failed example:
        1//0
    Exception raised:
        Traceback (most recent call last):
        ...
        ZeroDivisionError: integer division or modulo by zero
    TestResults(failed=1, attempted=1)
"""
    def displayhook(): r"""
Test that changing sys.displayhook doesn't matter for doctest.

    >>> import sys
    >>> orig_displayhook = sys.displayhook
    >>> def my_displayhook(x):
    ...     print('hi!')
    >>> sys.displayhook = my_displayhook
    >>> def f():
    ...     '''
    ...     >>> 3
    ...     3
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> r = doctest.DocTestRunner(verbose=False).run(test)
    >>> post_displayhook = sys.displayhook

    We need to restore sys.displayhook now, so that we'll be able to test
    results.

    >>> sys.displayhook = orig_displayhook

    Ok, now we can check that everything is ok.

    >>> r
    TestResults(failed=0, attempted=1)
    >>> post_displayhook is my_displayhook
    True
"""
    def optionflags(): r"""
Tests of `DocTestRunner`'s option flag handling.

Several option flags can be used to customize the behavior of the test
runner.  These are defined as module constants in doctest, and passed
to the DocTestRunner constructor (multiple constants should be ORed
together).

The DONT_ACCEPT_TRUE_FOR_1 flag disables matches between True/False
and 1/0:

    >>> def f(x):
    ...     '>>> True\n1\n'

    >>> # Without the flag:
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=1)

    >>> # With the flag:
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> flags = doctest.DONT_ACCEPT_TRUE_FOR_1
    >>> doctest.DocTestRunner(verbose=False, optionflags=flags).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 2, in f
    Failed example:
        True
    Expected:
        1
    Got:
        True
    TestResults(failed=1, attempted=1)

The DONT_ACCEPT_BLANKLINE flag disables the match between blank lines
and the '<BLANKLINE>' marker:

    >>> def f(x):
    ...     '>>> print("a\\n\\nb")\na\n<BLANKLINE>\nb\n'

    >>> # Without the flag:
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=1)

    >>> # With the flag:
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> flags = doctest.DONT_ACCEPT_BLANKLINE
    >>> doctest.DocTestRunner(verbose=False, optionflags=flags).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 2, in f
    Failed example:
        print("a\n\nb")
    Expected:
        a
        <BLANKLINE>
        b
    Got:
        a
    <BLANKLINE>
        b
    TestResults(failed=1, attempted=1)

The NORMALIZE_WHITESPACE flag causes all sequences of whitespace to be
treated as equal:

    >>> def f(x):
    ...     '>>> print(1, 2, 3)\n  1   2\n 3'

    >>> # Without the flag:
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 2, in f
    Failed example:
        print(1, 2, 3)
    Expected:
          1   2
         3
    Got:
        1 2 3
    TestResults(failed=1, attempted=1)

    >>> # With the flag:
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> flags = doctest.NORMALIZE_WHITESPACE
    >>> doctest.DocTestRunner(verbose=False, optionflags=flags).run(test)
    TestResults(failed=0, attempted=1)

    An example from the docs:
    >>> print(list(range(20))) #doctest: +NORMALIZE_WHITESPACE
    [0,   1,  2,  3,  4,  5,  6,  7,  8,  9,
    10,  11, 12, 13, 14, 15, 16, 17, 18, 19]

The ELLIPSIS flag causes ellipsis marker ("...") in the expected
output to match any substring in the actual output:

    >>> def f(x):
    ...     '>>> print(list(range(15)))\n[0, 1, 2, ..., 14]\n'

    >>> # Without the flag:
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 2, in f
    Failed example:
        print(list(range(15)))
    Expected:
        [0, 1, 2, ..., 14]
    Got:
        [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14]
    TestResults(failed=1, attempted=1)

    >>> # With the flag:
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> flags = doctest.ELLIPSIS
    >>> doctest.DocTestRunner(verbose=False, optionflags=flags).run(test)
    TestResults(failed=0, attempted=1)

    ... also matches nothing:

    >>> if 1:
    ...     for i in range(100):
    ...         print(i**2, end=' ') #doctest: +ELLIPSIS
    ...     print('!')
    0 1...4...9 16 ... 36 49 64 ... 9801 !

    ... can be surprising; e.g., this test passes:

    >>> if 1:  #doctest: +ELLIPSIS
    ...     for i in range(20):
    ...         print(i, end=' ')
    ...     print(20)
    0 1 2 ...1...2...0

    Examples from the docs:

    >>> print(list(range(20))) # doctest:+ELLIPSIS
    [0, 1, ..., 18, 19]

    >>> print(list(range(20))) # doctest: +ELLIPSIS
    ...                 # doctest: +NORMALIZE_WHITESPACE
    [0,    1, ...,   18,    19]

The SKIP flag causes an example to be skipped entirely.  I.e., the
example is not run.  It can be useful in contexts where doctest
examples serve as both documentation and test cases, and an example
should be included for documentation purposes, but should not be
checked (e.g., because its output is random, or depends on resources
which would be unavailable.)  The SKIP flag can also be used for
'commenting out' broken examples.

    >>> import unavailable_resource           # doctest: +SKIP
    >>> unavailable_resource.do_something()   # doctest: +SKIP
    >>> unavailable_resource.blow_up()        # doctest: +SKIP
    Traceback (most recent call last):
        ...
    UncheckedBlowUpError:  Nobody checks me.

    >>> import random
    >>> print(random.random()) # doctest: +SKIP
    0.721216923889

The REPORT_UDIFF flag causes failures that involve multi-line expected
and actual outputs to be displayed using a unified diff:

    >>> def f(x):
    ...     r'''
    ...     >>> print('\n'.join('abcdefg'))
    ...     a
    ...     B
    ...     c
    ...     d
    ...     f
    ...     g
    ...     h
    ...     '''

    >>> # Without the flag:
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 3, in f
    Failed example:
        print('\n'.join('abcdefg'))
    Expected:
        a
        B
        c
        d
        f
        g
        h
    Got:
        a
        b
        c
        d
        e
        f
        g
    TestResults(failed=1, attempted=1)

    >>> # With the flag:
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> flags = doctest.REPORT_UDIFF
    >>> doctest.DocTestRunner(verbose=False, optionflags=flags).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 3, in f
    Failed example:
        print('\n'.join('abcdefg'))
    Differences (unified diff with -expected +actual):
        @@ -1,7 +1,7 @@
         a
        -B
        +b
         c
         d
        +e
         f
         g
        -h
    TestResults(failed=1, attempted=1)

The REPORT_CDIFF flag causes failures that involve multi-line expected
and actual outputs to be displayed using a context diff:

    >>> # Reuse f() from the REPORT_UDIFF example, above.
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> flags = doctest.REPORT_CDIFF
    >>> doctest.DocTestRunner(verbose=False, optionflags=flags).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 3, in f
    Failed example:
        print('\n'.join('abcdefg'))
    Differences (context diff with expected followed by actual):
        ***************
        *** 1,7 ****
          a
        ! B
          c
          d
          f
          g
        - h
        --- 1,7 ----
          a
        ! b
          c
          d
        + e
          f
          g
    TestResults(failed=1, attempted=1)


The REPORT_NDIFF flag causes failures to use the difflib.Differ algorithm
used by the popular ndiff.py utility.  This does intraline difference
marking, as well as interline differences.

    >>> def f(x):
    ...     r'''
    ...     >>> print("a b  c d e f g h i   j k l m")
    ...     a b c d e f g h i j k 1 m
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> flags = doctest.REPORT_NDIFF
    >>> doctest.DocTestRunner(verbose=False, optionflags=flags).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 3, in f
    Failed example:
        print("a b  c d e f g h i   j k l m")
    Differences (ndiff with -expected +actual):
        - a b c d e f g h i j k 1 m
        ?                       ^
        + a b  c d e f g h i   j k l m
        ?     +              ++    ^
    TestResults(failed=1, attempted=1)

The REPORT_ONLY_FIRST_FAILURE suppresses result output after the first
failing example:

    >>> def f(x):
    ...     r'''
    ...     >>> print(1) # first success
    ...     1
    ...     >>> print(2) # first failure
    ...     200
    ...     >>> print(3) # second failure
    ...     300
    ...     >>> print(4) # second success
    ...     4
    ...     >>> print(5) # third failure
    ...     500
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> flags = doctest.REPORT_ONLY_FIRST_FAILURE
    >>> doctest.DocTestRunner(verbose=False, optionflags=flags).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 5, in f
    Failed example:
        print(2) # first failure
    Expected:
        200
    Got:
        2
    TestResults(failed=3, attempted=5)

However, output from `report_start` is not suppressed:

    >>> doctest.DocTestRunner(verbose=True, optionflags=flags).run(test)
    ... # doctest: +ELLIPSIS
    Trying:
        print(1) # first success
    Expecting:
        1
    ok
    Trying:
        print(2) # first failure
    Expecting:
        200
    **********************************************************************
    File ..., line 5, in f
    Failed example:
        print(2) # first failure
    Expected:
        200
    Got:
        2
    TestResults(failed=3, attempted=5)

The FAIL_FAST flag causes the runner to exit after the first failing example,
so subsequent examples are not even attempted:

    >>> flags = doctest.FAIL_FAST
    >>> doctest.DocTestRunner(verbose=False, optionflags=flags).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 5, in f
    Failed example:
        print(2) # first failure
    Expected:
        200
    Got:
        2
    TestResults(failed=1, attempted=2)

Specifying both FAIL_FAST and REPORT_ONLY_FIRST_FAILURE is equivalent to
FAIL_FAST only:

    >>> flags = doctest.FAIL_FAST | doctest.REPORT_ONLY_FIRST_FAILURE
    >>> doctest.DocTestRunner(verbose=False, optionflags=flags).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 5, in f
    Failed example:
        print(2) # first failure
    Expected:
        200
    Got:
        2
    TestResults(failed=1, attempted=2)

For the purposes of both REPORT_ONLY_FIRST_FAILURE and FAIL_FAST, unexpected
exceptions count as failures:

    >>> def f(x):
    ...     r'''
    ...     >>> print(1) # first success
    ...     1
    ...     >>> raise ValueError(2) # first failure
    ...     200
    ...     >>> print(3) # second failure
    ...     300
    ...     >>> print(4) # second success
    ...     4
    ...     >>> print(5) # third failure
    ...     500
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> flags = doctest.REPORT_ONLY_FIRST_FAILURE
    >>> doctest.DocTestRunner(verbose=False, optionflags=flags).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 5, in f
    Failed example:
        raise ValueError(2) # first failure
    Exception raised:
        ...
        ValueError: 2
    TestResults(failed=3, attempted=5)
    >>> flags = doctest.FAIL_FAST
    >>> doctest.DocTestRunner(verbose=False, optionflags=flags).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 5, in f
    Failed example:
        raise ValueError(2) # first failure
    Exception raised:
        ...
        ValueError: 2
    TestResults(failed=1, attempted=2)

New option flags can also be registered, via register_optionflag().  Here
we reach into doctest's internals a bit.

    >>> unlikely = "UNLIKELY_OPTION_NAME"
    >>> unlikely in doctest.OPTIONFLAGS_BY_NAME
    False
    >>> new_flag_value = doctest.register_optionflag(unlikely)
    >>> unlikely in doctest.OPTIONFLAGS_BY_NAME
    True

Before 2.4.4/2.5, registering a name more than once erroneously created
more than one flag value.  Here we verify that's fixed:

    >>> redundant_flag_value = doctest.register_optionflag(unlikely)
    >>> redundant_flag_value == new_flag_value
    True

Clean up.
    >>> del doctest.OPTIONFLAGS_BY_NAME[unlikely]

    """

    def option_directives(): r"""
Tests of `DocTestRunner`'s option directive mechanism.

Option directives can be used to turn option flags on or off for a
single example.  To turn an option on for an example, follow that
example with a comment of the form ``# doctest: +OPTION``:

    >>> def f(x): r'''
    ...     >>> print(list(range(10)))      # should fail: no ellipsis
    ...     [0, 1, ..., 9]
    ...
    ...     >>> print(list(range(10)))      # doctest: +ELLIPSIS
    ...     [0, 1, ..., 9]
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 2, in f
    Failed example:
        print(list(range(10)))      # should fail: no ellipsis
    Expected:
        [0, 1, ..., 9]
    Got:
        [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    TestResults(failed=1, attempted=2)

To turn an option off for an example, follow that example with a
comment of the form ``# doctest: -OPTION``:

    >>> def f(x): r'''
    ...     >>> print(list(range(10)))
    ...     [0, 1, ..., 9]
    ...
    ...     >>> # should fail: no ellipsis
    ...     >>> print(list(range(10)))      # doctest: -ELLIPSIS
    ...     [0, 1, ..., 9]
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False,
    ...                       optionflags=doctest.ELLIPSIS).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 6, in f
    Failed example:
        print(list(range(10)))      # doctest: -ELLIPSIS
    Expected:
        [0, 1, ..., 9]
    Got:
        [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    TestResults(failed=1, attempted=2)

Option directives affect only the example that they appear with; they
do not change the options for surrounding examples:

    >>> def f(x): r'''
    ...     >>> print(list(range(10)))      # Should fail: no ellipsis
    ...     [0, 1, ..., 9]
    ...
    ...     >>> print(list(range(10)))      # doctest: +ELLIPSIS
    ...     [0, 1, ..., 9]
    ...
    ...     >>> print(list(range(10)))      # Should fail: no ellipsis
    ...     [0, 1, ..., 9]
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 2, in f
    Failed example:
        print(list(range(10)))      # Should fail: no ellipsis
    Expected:
        [0, 1, ..., 9]
    Got:
        [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    **********************************************************************
    File ..., line 8, in f
    Failed example:
        print(list(range(10)))      # Should fail: no ellipsis
    Expected:
        [0, 1, ..., 9]
    Got:
        [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    TestResults(failed=2, attempted=3)

Multiple options may be modified by a single option directive.  They
may be separated by whitespace, commas, or both:

    >>> def f(x): r'''
    ...     >>> print(list(range(10)))      # Should fail
    ...     [0, 1,  ...,   9]
    ...     >>> print(list(range(10)))      # Should succeed
    ...     ... # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
    ...     [0, 1,  ...,   9]
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 2, in f
    Failed example:
        print(list(range(10)))      # Should fail
    Expected:
        [0, 1,  ...,   9]
    Got:
        [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    TestResults(failed=1, attempted=2)

    >>> def f(x): r'''
    ...     >>> print(list(range(10)))      # Should fail
    ...     [0, 1,  ...,   9]
    ...     >>> print(list(range(10)))      # Should succeed
    ...     ... # doctest: +ELLIPSIS,+NORMALIZE_WHITESPACE
    ...     [0, 1,  ...,   9]
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 2, in f
    Failed example:
        print(list(range(10)))      # Should fail
    Expected:
        [0, 1,  ...,   9]
    Got:
        [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    TestResults(failed=1, attempted=2)

    >>> def f(x): r'''
    ...     >>> print(list(range(10)))      # Should fail
    ...     [0, 1,  ...,   9]
    ...     >>> print(list(range(10)))      # Should succeed
    ...     ... # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     [0, 1,  ...,   9]
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 2, in f
    Failed example:
        print(list(range(10)))      # Should fail
    Expected:
        [0, 1,  ...,   9]
    Got:
        [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    TestResults(failed=1, attempted=2)

The option directive may be put on the line following the source, as
long as a continuation prompt is used:

    >>> def f(x): r'''
    ...     >>> print(list(range(10)))
    ...     ... # doctest: +ELLIPSIS
    ...     [0, 1, ..., 9]
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=1)

For examples with multi-line source, the option directive may appear
at the end of any line:

    >>> def f(x): r'''
    ...     >>> for x in range(10): # doctest: +ELLIPSIS
    ...     ...     print(' ', x, end='', sep='')
    ...      0 1 2 ... 9
    ...
    ...     >>> for x in range(10):
    ...     ...     print(' ', x, end='', sep='') # doctest: +ELLIPSIS
    ...      0 1 2 ... 9
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=2)

If more than one line of an example with multi-line source has an
option directive, then they are combined:

    >>> def f(x): r'''
    ...     Should fail (option directive not on the last line):
    ...         >>> for x in range(10): # doctest: +ELLIPSIS
    ...         ...     print(x, end=' ') # doctest: +NORMALIZE_WHITESPACE
    ...         0  1    2...9
    ...     '''
    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> doctest.DocTestRunner(verbose=False).run(test)
    TestResults(failed=0, attempted=1)

It is an error to have a comment of the form ``# doctest:`` that is
*not* followed by words of the form ``+OPTION`` or ``-OPTION``, where
``OPTION`` is an option that has been registered with
`register_option`:

    >>> # Error: Option not registered
    >>> s = '>>> print(12)  #doctest: +BADOPTION'
    >>> test = doctest.DocTestParser().get_doctest(s, {}, 's', 's.py', 0)
    Traceback (most recent call last):
    ValueError: line 1 of the doctest for s has an invalid option: '+BADOPTION'

    >>> # Error: No + or - prefix
    >>> s = '>>> print(12)  #doctest: ELLIPSIS'
    >>> test = doctest.DocTestParser().get_doctest(s, {}, 's', 's.py', 0)
    Traceback (most recent call last):
    ValueError: line 1 of the doctest for s has an invalid option: 'ELLIPSIS'

It is an error to use an option directive on a line that contains no
source:

    >>> s = '>>> # doctest: +ELLIPSIS'
    >>> test = doctest.DocTestParser().get_doctest(s, {}, 's', 's.py', 0)
    Traceback (most recent call last):
    ValueError: line 0 of the doctest for s has an option directive on a line with no example: '# doctest: +ELLIPSIS'
"""

def test_testsource(): r"""
Unit tests for `testsource()`.

The testsource() function takes a module and a name, finds the (first)
test with that name in that module, and converts it to a script. The
example code is converted to regular Python code.  The surrounding
words and expected output are converted to comments:

    >>> import test.test_doctest
    >>> name = 'test.test_doctest.sample_func'
    >>> print(doctest.testsource(test.test_doctest, name))
    # Blah blah
    #
    print(sample_func(22))
    # Expected:
    ## 44
    #
    # Yee ha!
    <BLANKLINE>

    >>> name = 'test.test_doctest.SampleNewStyleClass'
    >>> print(doctest.testsource(test.test_doctest, name))
    print('1\n2\n3')
    # Expected:
    ## 1
    ## 2
    ## 3
    <BLANKLINE>

    >>> name = 'test.test_doctest.SampleClass.a_classmethod'
    >>> print(doctest.testsource(test.test_doctest, name))
    print(SampleClass.a_classmethod(10))
    # Expected:
    ## 12
    print(SampleClass(0).a_classmethod(10))
    # Expected:
    ## 12
    <BLANKLINE>
"""

def test_debug(): r"""

Create a docstring that we want to debug:

    >>> s = '''
    ...     >>> x = 12
    ...     >>> print(x)
    ...     12
    ...     '''

Create some fake stdin input, to feed to the debugger:

    >>> real_stdin = sys.stdin
    >>> sys.stdin = _FakeInput(['next', 'print(x)', 'continue'])

Run the debugger on the docstring, and then restore sys.stdin.

    >>> try: doctest.debug_src(s)
    ... finally: sys.stdin = real_stdin
    > <string>(1)<module>()
    (Pdb) next
    12
    --Return--
    > <string>(1)<module>()->None
    (Pdb) print(x)
    12
    (Pdb) continue

"""

if not hasattr(sys, 'gettrace') or not sys.gettrace():
    def test_pdb_set_trace():
        """Using pdb.set_trace from a doctest.

        You can use pdb.set_trace from a doctest.  To do so, you must
        retrieve the set_trace function from the pdb module at the time
        you use it.  The doctest module changes sys.stdout so that it can
        capture program output.  It also temporarily replaces pdb.set_trace
        with a version that restores stdout.  This is necessary for you to
        see debugger output.

          >>> doc = '''
          ... >>> x = 42
          ... >>> raise Exception('clé')
          ... Traceback (most recent call last):
          ... Exception: clé
          ... >>> import pdb; pdb.set_trace()
          ... '''
          >>> parser = doctest.DocTestParser()
          >>> test = parser.get_doctest(doc, {}, "foo-bar@baz", "foo-bar@baz.py", 0)
          >>> runner = doctest.DocTestRunner(verbose=False)

        To demonstrate this, we'll create a fake standard input that
        captures our debugger input:

          >>> real_stdin = sys.stdin
          >>> sys.stdin = _FakeInput([
          ...    'print(x)',  # print data defined by the example
          ...    'continue', # stop debugging
          ...    ''])

          >>> try: runner.run(test)
          ... finally: sys.stdin = real_stdin
          --Return--
          > <doctest foo-bar@baz[2]>(1)<module>()->None
          -> import pdb; pdb.set_trace()
          (Pdb) print(x)
          42
          (Pdb) continue
          TestResults(failed=0, attempted=3)

          You can also put pdb.set_trace in a function called from a test:

          >>> def calls_set_trace():
          ...    y=2
          ...    import pdb; pdb.set_trace()

          >>> doc = '''
          ... >>> x=1
          ... >>> calls_set_trace()
          ... '''
          >>> test = parser.get_doctest(doc, globals(), "foo-bar@baz", "foo-bar@baz.py", 0)
          >>> real_stdin = sys.stdin
          >>> sys.stdin = _FakeInput([
          ...    'print(y)',  # print data defined in the function
          ...    'up',       # out of function
          ...    'print(x)',  # print data defined by the example
          ...    'continue', # stop debugging
          ...    ''])

          >>> try:
          ...     runner.run(test)
          ... finally:
          ...     sys.stdin = real_stdin
          --Return--
          > <doctest test.test_doctest.test_pdb_set_trace[7]>(3)calls_set_trace()->None
          -> import pdb; pdb.set_trace()
          (Pdb) print(y)
          2
          (Pdb) up
          > <doctest foo-bar@baz[1]>(1)<module>()
          -> calls_set_trace()
          (Pdb) print(x)
          1
          (Pdb) continue
          TestResults(failed=0, attempted=2)

        During interactive debugging, source code is shown, even for
        doctest examples:

          >>> doc = '''
          ... >>> def f(x):
          ... ...     g(x*2)
          ... >>> def g(x):
          ... ...     print(x+3)
          ... ...     import pdb; pdb.set_trace()
          ... >>> f(3)
          ... '''
          >>> test = parser.get_doctest(doc, globals(), "foo-bar@baz", "foo-bar@baz.py", 0)
          >>> real_stdin = sys.stdin
          >>> sys.stdin = _FakeInput([
          ...    'list',     # list source from example 2
          ...    'next',     # return from g()
          ...    'list',     # list source from example 1
          ...    'next',     # return from f()
          ...    'list',     # list source from example 3
          ...    'continue', # stop debugging
          ...    ''])
          >>> try: runner.run(test)
          ... finally: sys.stdin = real_stdin
          ... # doctest: +NORMALIZE_WHITESPACE
          --Return--
          > <doctest foo-bar@baz[1]>(3)g()->None
          -> import pdb; pdb.set_trace()
          (Pdb) list
            1     def g(x):
            2         print(x+3)
            3  ->     import pdb; pdb.set_trace()
          [EOF]
          (Pdb) next
          --Return--
          > <doctest foo-bar@baz[0]>(2)f()->None
          -> g(x*2)
          (Pdb) list
            1     def f(x):
            2  ->     g(x*2)
          [EOF]
          (Pdb) next
          --Return--
          > <doctest foo-bar@baz[2]>(1)<module>()->None
          -> f(3)
          (Pdb) list
            1  -> f(3)
          [EOF]
          (Pdb) continue
          **********************************************************************
          File "foo-bar@baz.py", line 7, in foo-bar@baz
          Failed example:
              f(3)
          Expected nothing
          Got:
              9
          TestResults(failed=1, attempted=3)
          """

    def test_pdb_set_trace_nested():
        """This illustrates more-demanding use of set_trace with nested functions.

        >>> class C(object):
        ...     def calls_set_trace(self):
        ...         y = 1
        ...         import pdb; pdb.set_trace()
        ...         self.f1()
        ...         y = 2
        ...     def f1(self):
        ...         x = 1
        ...         self.f2()
        ...         x = 2
        ...     def f2(self):
        ...         z = 1
        ...         z = 2

        >>> calls_set_trace = C().calls_set_trace

        >>> doc = '''
        ... >>> a = 1
        ... >>> calls_set_trace()
        ... '''
        >>> parser = doctest.DocTestParser()
        >>> runner = doctest.DocTestRunner(verbose=False)
        >>> test = parser.get_doctest(doc, globals(), "foo-bar@baz", "foo-bar@baz.py", 0)
        >>> real_stdin = sys.stdin
        >>> sys.stdin = _FakeInput([
        ...    'print(y)',  # print data defined in the function
        ...    'step', 'step', 'step', 'step', 'step', 'step', 'print(z)',
        ...    'up', 'print(x)',
        ...    'up', 'print(y)',
        ...    'up', 'print(foo)',
        ...    'continue', # stop debugging
        ...    ''])

        >>> try:
        ...     runner.run(test)
        ... finally:
        ...     sys.stdin = real_stdin
        ... # doctest: +REPORT_NDIFF
        > <doctest test.test_doctest.test_pdb_set_trace_nested[0]>(5)calls_set_trace()
        -> self.f1()
        (Pdb) print(y)
        1
        (Pdb) step
        --Call--
        > <doctest test.test_doctest.test_pdb_set_trace_nested[0]>(7)f1()
        -> def f1(self):
        (Pdb) step
        > <doctest test.test_doctest.test_pdb_set_trace_nested[0]>(8)f1()
        -> x = 1
        (Pdb) step
        > <doctest test.test_doctest.test_pdb_set_trace_nested[0]>(9)f1()
        -> self.f2()
        (Pdb) step
        --Call--
        > <doctest test.test_doctest.test_pdb_set_trace_nested[0]>(11)f2()
        -> def f2(self):
        (Pdb) step
        > <doctest test.test_doctest.test_pdb_set_trace_nested[0]>(12)f2()
        -> z = 1
        (Pdb) step
        > <doctest test.test_doctest.test_pdb_set_trace_nested[0]>(13)f2()
        -> z = 2
        (Pdb) print(z)
        1
        (Pdb) up
        > <doctest test.test_doctest.test_pdb_set_trace_nested[0]>(9)f1()
        -> self.f2()
        (Pdb) print(x)
        1
        (Pdb) up
        > <doctest test.test_doctest.test_pdb_set_trace_nested[0]>(5)calls_set_trace()
        -> self.f1()
        (Pdb) print(y)
        1
        (Pdb) up
        > <doctest foo-bar@baz[1]>(1)<module>()
        -> calls_set_trace()
        (Pdb) print(foo)
        *** NameError: name 'foo' is not defined
        (Pdb) continue
        TestResults(failed=0, attempted=2)
    """

def test_DocTestSuite():
    """DocTestSuite creates a unittest test suite from a doctest.

       We create a Suite by providing a module.  A module can be provided
       by passing a module object:

         >>> import unittest
         >>> import test.sample_doctest
         >>> suite = doctest.DocTestSuite(test.sample_doctest)
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=9 errors=0 failures=4>

       We can also supply the module by name:

         >>> suite = doctest.DocTestSuite('test.sample_doctest')
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=9 errors=0 failures=4>

       The module need not contain any doctest examples:

         >>> suite = doctest.DocTestSuite('test.sample_doctest_no_doctests')
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=0 errors=0 failures=0>

       The module need not contain any docstrings either:

         >>> suite = doctest.DocTestSuite('test.sample_doctest_no_docstrings')
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=0 errors=0 failures=0>

       We can use the current module:

         >>> suite = test.sample_doctest.test_suite()
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=9 errors=0 failures=4>

       We can also provide a DocTestFinder:

         >>> finder = doctest.DocTestFinder()
         >>> suite = doctest.DocTestSuite('test.sample_doctest',
         ...                          test_finder=finder)
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=9 errors=0 failures=4>

       The DocTestFinder need not return any tests:

         >>> finder = doctest.DocTestFinder()
         >>> suite = doctest.DocTestSuite('test.sample_doctest_no_docstrings',
         ...                          test_finder=finder)
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=0 errors=0 failures=0>

       We can supply global variables.  If we pass globs, they will be
       used instead of the module globals.  Here we'll pass an empty
       globals, triggering an extra error:

         >>> suite = doctest.DocTestSuite('test.sample_doctest', globs={})
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=9 errors=0 failures=5>

       Alternatively, we can provide extra globals.  Here we'll make an
       error go away by providing an extra global variable:

         >>> suite = doctest.DocTestSuite('test.sample_doctest',
         ...                              extraglobs={'y': 1})
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=9 errors=0 failures=3>

       You can pass option flags.  Here we'll cause an extra error
       by disabling the blank-line feature:

         >>> suite = doctest.DocTestSuite('test.sample_doctest',
         ...                      optionflags=doctest.DONT_ACCEPT_BLANKLINE)
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=9 errors=0 failures=5>

       You can supply setUp and tearDown functions:

         >>> def setUp(t):
         ...     import test.test_doctest
         ...     test.test_doctest.sillySetup = True

         >>> def tearDown(t):
         ...     import test.test_doctest
         ...     del test.test_doctest.sillySetup

       Here, we installed a silly variable that the test expects:

         >>> suite = doctest.DocTestSuite('test.sample_doctest',
         ...      setUp=setUp, tearDown=tearDown)
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=9 errors=0 failures=3>

       But the tearDown restores sanity:

         >>> import test.test_doctest
         >>> test.test_doctest.sillySetup
         Traceback (most recent call last):
         ...
         AttributeError: module 'test.test_doctest' has no attribute 'sillySetup'

       The setUp and tearDown functions are passed test objects. Here
       we'll use the setUp function to supply the missing variable y:

         >>> def setUp(test):
         ...     test.globs['y'] = 1

         >>> suite = doctest.DocTestSuite('test.sample_doctest', setUp=setUp)
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=9 errors=0 failures=3>

       Here, we didn't need to use a tearDown function because we
       modified the test globals, which are a copy of the
       sample_doctest module dictionary.  The test globals are
       automatically cleared for us after a test.
       """

def test_DocFileSuite():
    """We can test tests found in text files using a DocFileSuite.

       We create a suite by providing the names of one or more text
       files that include examples:

         >>> import unittest
         >>> suite = doctest.DocFileSuite('test_doctest.txt',
         ...                              'test_doctest2.txt',
         ...                              'test_doctest4.txt')
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=3 errors=0 failures=2>

       The test files are looked for in the directory containing the
       calling module.  A package keyword argument can be provided to
       specify a different relative location.

         >>> import unittest
         >>> suite = doctest.DocFileSuite('test_doctest.txt',
         ...                              'test_doctest2.txt',
         ...                              'test_doctest4.txt',
         ...                              package='test')
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=3 errors=0 failures=2>

       Support for using a package's __loader__.get_data() is also
       provided.

         >>> import unittest, pkgutil, test
         >>> added_loader = False
         >>> if not hasattr(test, '__loader__'):
         ...     test.__loader__ = pkgutil.get_loader(test)
         ...     added_loader = True
         >>> try:
         ...     suite = doctest.DocFileSuite('test_doctest.txt',
         ...                                  'test_doctest2.txt',
         ...                                  'test_doctest4.txt',
         ...                                  package='test')
         ...     suite.run(unittest.TestResult())
         ... finally:
         ...     if added_loader:
         ...         del test.__loader__
         <unittest.result.TestResult run=3 errors=0 failures=2>

       '/' should be used as a path separator.  It will be converted
       to a native separator at run time:

         >>> suite = doctest.DocFileSuite('../test/test_doctest.txt')
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=1 errors=0 failures=1>

       If DocFileSuite is used from an interactive session, then files
       are resolved relative to the directory of sys.argv[0]:

         >>> import types, os.path, test.test_doctest
         >>> save_argv = sys.argv
         >>> sys.argv = [test.test_doctest.__file__]
         >>> suite = doctest.DocFileSuite('test_doctest.txt',
         ...                              package=types.ModuleType('__main__'))
         >>> sys.argv = save_argv

       By setting `module_relative=False`, os-specific paths may be
       used (including absolute paths and paths relative to the
       working directory):

         >>> # Get the absolute path of the test package.
         >>> test_doctest_path = os.path.abspath(test.test_doctest.__file__)
         >>> test_pkg_path = os.path.split(test_doctest_path)[0]

         >>> # Use it to find the absolute path of test_doctest.txt.
         >>> test_file = os.path.join(test_pkg_path, 'test_doctest.txt')

         >>> suite = doctest.DocFileSuite(test_file, module_relative=False)
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=1 errors=0 failures=1>

       It is an error to specify `package` when `module_relative=False`:

         >>> suite = doctest.DocFileSuite(test_file, module_relative=False,
         ...                              package='test')
         Traceback (most recent call last):
         ValueError: Package may only be specified for module-relative paths.

       You can specify initial global variables:

         >>> suite = doctest.DocFileSuite('test_doctest.txt',
         ...                              'test_doctest2.txt',
         ...                              'test_doctest4.txt',
         ...                              globs={'favorite_color': 'blue'})
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=3 errors=0 failures=1>

       In this case, we supplied a missing favorite color. You can
       provide doctest options:

         >>> suite = doctest.DocFileSuite('test_doctest.txt',
         ...                              'test_doctest2.txt',
         ...                              'test_doctest4.txt',
         ...                         optionflags=doctest.DONT_ACCEPT_BLANKLINE,
         ...                              globs={'favorite_color': 'blue'})
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=3 errors=0 failures=2>

       And, you can provide setUp and tearDown functions:

         >>> def setUp(t):
         ...     import test.test_doctest
         ...     test.test_doctest.sillySetup = True

         >>> def tearDown(t):
         ...     import test.test_doctest
         ...     del test.test_doctest.sillySetup

       Here, we installed a silly variable that the test expects:

         >>> suite = doctest.DocFileSuite('test_doctest.txt',
         ...                              'test_doctest2.txt',
         ...                              'test_doctest4.txt',
         ...                              setUp=setUp, tearDown=tearDown)
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=3 errors=0 failures=1>

       But the tearDown restores sanity:

         >>> import test.test_doctest
         >>> test.test_doctest.sillySetup
         Traceback (most recent call last):
         ...
         AttributeError: module 'test.test_doctest' has no attribute 'sillySetup'

       The setUp and tearDown functions are passed test objects.
       Here, we'll use a setUp function to set the favorite color in
       test_doctest.txt:

         >>> def setUp(test):
         ...     test.globs['favorite_color'] = 'blue'

         >>> suite = doctest.DocFileSuite('test_doctest.txt', setUp=setUp)
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=1 errors=0 failures=0>

       Here, we didn't need to use a tearDown function because we
       modified the test globals.  The test globals are
       automatically cleared for us after a test.

       Tests in a file run using `DocFileSuite` can also access the
       `__file__` global, which is set to the name of the file
       containing the tests:

         >>> suite = doctest.DocFileSuite('test_doctest3.txt')
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=1 errors=0 failures=0>

       If the tests contain non-ASCII characters, we have to specify which
       encoding the file is encoded with. We do so by using the `encoding`
       parameter:

         >>> suite = doctest.DocFileSuite('test_doctest.txt',
         ...                              'test_doctest2.txt',
         ...                              'test_doctest4.txt',
         ...                              encoding='utf-8')
         >>> suite.run(unittest.TestResult())
         <unittest.result.TestResult run=3 errors=0 failures=2>

       """

def test_trailing_space_in_test():
    """
    Trailing spaces in expected output are significant:

      >>> x, y = 'foo', ''
      >>> print(x, y)
      foo \n
    """

class Wrapper:
    def __init__(self, func):
        self.func = func
        functools.update_wrapper(self, func)

    def __call__(self, *args, **kwargs):
        self.func(*args, **kwargs)

@Wrapper
def test_look_in_unwrapped():
    """
    Docstrings in wrapped functions must be detected as well.

    >>> 'one other test'
    'one other test'
    """

def test_unittest_reportflags():
    """Default unittest reporting flags can be set to control reporting

    Here, we'll set the REPORT_ONLY_FIRST_FAILURE option so we see
    only the first failure of each test.  First, we'll look at the
    output without the flag.  The file test_doctest.txt file has two
    tests. They both fail if blank lines are disabled:

      >>> suite = doctest.DocFileSuite('test_doctest.txt',
      ...                          optionflags=doctest.DONT_ACCEPT_BLANKLINE)
      >>> import unittest
      >>> result = suite.run(unittest.TestResult())
      >>> print(result.failures[0][1]) # doctest: +ELLIPSIS
      Traceback ...
      Failed example:
          favorite_color
      ...
      Failed example:
          if 1:
      ...

    Note that we see both failures displayed.

      >>> old = doctest.set_unittest_reportflags(
      ...    doctest.REPORT_ONLY_FIRST_FAILURE)

    Now, when we run the test:

      >>> result = suite.run(unittest.TestResult())
      >>> print(result.failures[0][1]) # doctest: +ELLIPSIS
      Traceback ...
      Failed example:
          favorite_color
      Exception raised:
          ...
          NameError: name 'favorite_color' is not defined
      <BLANKLINE>
      <BLANKLINE>

    We get only the first failure.

    If we give any reporting options when we set up the tests,
    however:

      >>> suite = doctest.DocFileSuite('test_doctest.txt',
      ...     optionflags=doctest.DONT_ACCEPT_BLANKLINE | doctest.REPORT_NDIFF)

    Then the default eporting options are ignored:

      >>> result = suite.run(unittest.TestResult())

    *NOTE*: These doctest are intentionally not placed in raw string to depict
    the trailing whitespace using `\x20` in the diff below.

      >>> print(result.failures[0][1]) # doctest: +ELLIPSIS
      Traceback ...
      Failed example:
          favorite_color
      ...
      Failed example:
          if 1:
             print('a')
             print()
             print('b')
      Differences (ndiff with -expected +actual):
            a
          - <BLANKLINE>
          +\x20
            b
      <BLANKLINE>
      <BLANKLINE>


    Test runners can restore the formatting flags after they run:

      >>> ignored = doctest.set_unittest_reportflags(old)

    """

def test_testfile(): r"""
Tests for the `testfile()` function.  This function runs all the
doctest examples in a given file.  In its simple invokation, it is
called with the name of a file, which is taken to be relative to the
calling module.  The return value is (#failures, #tests).

We don't want `-v` in sys.argv for these tests.

    >>> save_argv = sys.argv
    >>> if '-v' in sys.argv:
    ...     sys.argv = [arg for arg in save_argv if arg != '-v']


    >>> doctest.testfile('test_doctest.txt') # doctest: +ELLIPSIS
    **********************************************************************
    File "...", line 6, in test_doctest.txt
    Failed example:
        favorite_color
    Exception raised:
        ...
        NameError: name 'favorite_color' is not defined
    **********************************************************************
    1 items had failures:
       1 of   2 in test_doctest.txt
    ***Test Failed*** 1 failures.
    TestResults(failed=1, attempted=2)
    >>> doctest.master = None  # Reset master.

(Note: we'll be clearing doctest.master after each call to
`doctest.testfile`, to suppress warnings about multiple tests with the
same name.)

Globals may be specified with the `globs` and `extraglobs` parameters:

    >>> globs = {'favorite_color': 'blue'}
    >>> doctest.testfile('test_doctest.txt', globs=globs)
    TestResults(failed=0, attempted=2)
    >>> doctest.master = None  # Reset master.

    >>> extraglobs = {'favorite_color': 'red'}
    >>> doctest.testfile('test_doctest.txt', globs=globs,
    ...                  extraglobs=extraglobs) # doctest: +ELLIPSIS
    **********************************************************************
    File "...", line 6, in test_doctest.txt
    Failed example:
        favorite_color
    Expected:
        'blue'
    Got:
        'red'
    **********************************************************************
    1 items had failures:
       1 of   2 in test_doctest.txt
    ***Test Failed*** 1 failures.
    TestResults(failed=1, attempted=2)
    >>> doctest.master = None  # Reset master.

The file may be made relative to a given module or package, using the
optional `module_relative` parameter:

    >>> doctest.testfile('test_doctest.txt', globs=globs,
    ...                  module_relative='test')
    TestResults(failed=0, attempted=2)
    >>> doctest.master = None  # Reset master.

Verbosity can be increased with the optional `verbose` parameter:

    >>> doctest.testfile('test_doctest.txt', globs=globs, verbose=True)
    Trying:
        favorite_color
    Expecting:
        'blue'
    ok
    Trying:
        if 1:
           print('a')
           print()
           print('b')
    Expecting:
        a
        <BLANKLINE>
        b
    ok
    1 items passed all tests:
       2 tests in test_doctest.txt
    2 tests in 1 items.
    2 passed and 0 failed.
    Test passed.
    TestResults(failed=0, attempted=2)
    >>> doctest.master = None  # Reset master.

The name of the test may be specified with the optional `name`
parameter:

    >>> doctest.testfile('test_doctest.txt', name='newname')
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File "...", line 6, in newname
    ...
    TestResults(failed=1, attempted=2)
    >>> doctest.master = None  # Reset master.

The summary report may be suppressed with the optional `report`
parameter:

    >>> doctest.testfile('test_doctest.txt', report=False)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File "...", line 6, in test_doctest.txt
    Failed example:
        favorite_color
    Exception raised:
        ...
        NameError: name 'favorite_color' is not defined
    TestResults(failed=1, attempted=2)
    >>> doctest.master = None  # Reset master.

The optional keyword argument `raise_on_error` can be used to raise an
exception on the first error (which may be useful for postmortem
debugging):

    >>> doctest.testfile('test_doctest.txt', raise_on_error=True)
    ... # doctest: +ELLIPSIS
    Traceback (most recent call last):
    doctest.UnexpectedException: ...
    >>> doctest.master = None  # Reset master.

If the tests contain non-ASCII characters, the tests might fail, since
it's unknown which encoding is used. The encoding can be specified
using the optional keyword argument `encoding`:

    >>> doctest.testfile('test_doctest4.txt', encoding='latin-1') # doctest: +ELLIPSIS
    **********************************************************************
    File "...", line 7, in test_doctest4.txt
    Failed example:
        '...'
    Expected:
        'f\xf6\xf6'
    Got:
        'f\xc3\xb6\xc3\xb6'
    **********************************************************************
    ...
    **********************************************************************
    1 items had failures:
       2 of   2 in test_doctest4.txt
    ***Test Failed*** 2 failures.
    TestResults(failed=2, attempted=2)
    >>> doctest.master = None  # Reset master.

    >>> doctest.testfile('test_doctest4.txt', encoding='utf-8')
    TestResults(failed=0, attempted=2)
    >>> doctest.master = None  # Reset master.

Test the verbose output:

    >>> doctest.testfile('test_doctest4.txt', encoding='utf-8', verbose=True)
    Trying:
        'föö'
    Expecting:
        'f\xf6\xf6'
    ok
    Trying:
        'bąr'
    Expecting:
        'b\u0105r'
    ok
    1 items passed all tests:
       2 tests in test_doctest4.txt
    2 tests in 1 items.
    2 passed and 0 failed.
    Test passed.
    TestResults(failed=0, attempted=2)
    >>> doctest.master = None  # Reset master.
    >>> sys.argv = save_argv
"""

def test_lineendings(): r"""
*nix systems use \n line endings, while Windows systems use \r\n.  Python
handles this using universal newline mode for reading files.  Let's make
sure doctest does so (issue 8473) by creating temporary test files using each
of the two line disciplines.  One of the two will be the "wrong" one for the
platform the test is run on.

Windows line endings first:

    >>> import tempfile, os
    >>> fn = tempfile.mktemp()
    >>> with open(fn, 'wb') as f:
    ...    f.write(b'Test:\r\n\r\n  >>> x = 1 + 1\r\n\r\nDone.\r\n')
    35
    >>> doctest.testfile(fn, module_relative=False, verbose=False)
    TestResults(failed=0, attempted=1)
    >>> os.remove(fn)

And now *nix line endings:

    >>> fn = tempfile.mktemp()
    >>> with open(fn, 'wb') as f:
    ...     f.write(b'Test:\n\n  >>> x = 1 + 1\n\nDone.\n')
    30
    >>> doctest.testfile(fn, module_relative=False, verbose=False)
    TestResults(failed=0, attempted=1)
    >>> os.remove(fn)

"""

def test_testmod(): r"""
Tests for the testmod function.  More might be useful, but for now we're just
testing the case raised by Issue 6195, where trying to doctest a C module would
fail with a UnicodeDecodeError because doctest tried to read the "source" lines
out of the binary module.

    >>> import unicodedata
    >>> doctest.testmod(unicodedata, verbose=False)
    TestResults(failed=0, attempted=0)
"""

try:
    os.fsencode("foo-bär@baz.py")
except UnicodeEncodeError:
    # Skip the test: the filesystem encoding is unable to encode the filename
    pass
else:
    def test_unicode(): """
Check doctest with a non-ascii filename:

    >>> doc = '''
    ... >>> raise Exception('clé')
    ... '''
    ...
    >>> parser = doctest.DocTestParser()
    >>> test = parser.get_doctest(doc, {}, "foo-bär@baz", "foo-bär@baz.py", 0)
    >>> test
    <DocTest foo-bär@baz from foo-bär@baz.py:0 (1 example)>
    >>> runner = doctest.DocTestRunner(verbose=False)
    >>> runner.run(test) # doctest: +ELLIPSIS
    **********************************************************************
    File "foo-bär@baz.py", line 2, in foo-bär@baz
    Failed example:
        raise Exception('clé')
    Exception raised:
        Traceback (most recent call last):
          File ...
            compileflags, 1), test.globs)
          File "<doctest foo-bär@baz[0]>", line 1, in <module>
            raise Exception('clé')
        Exception: clé
    TestResults(failed=1, attempted=1)
    """

def test_CLI(): r"""
The doctest module can be used to run doctests against an arbitrary file.
These tests test this CLI functionality.

We'll use the support module's script_helpers for this, and write a test files
to a temp dir to run the command against.  Due to a current limitation in
script_helpers, though, we need a little utility function to turn the returned
output into something we can doctest against:

    >>> def normalize(s):
    ...     return '\n'.join(s.decode().splitlines())

With those preliminaries out of the way, we'll start with a file with two
simple tests and no errors.  We'll run both the unadorned doctest command, and
the verbose version, and then check the output:

    >>> from test.support import script_helper, temp_dir
    >>> with temp_dir() as tmpdir:
    ...     fn = os.path.join(tmpdir, 'myfile.doc')
    ...     with open(fn, 'w') as f:
    ...         _ = f.write('This is a very simple test file.\n')
    ...         _ = f.write('   >>> 1 + 1\n')
    ...         _ = f.write('   2\n')
    ...         _ = f.write('   >>> "a"\n')
    ...         _ = f.write("   'a'\n")
    ...         _ = f.write('\n')
    ...         _ = f.write('And that is it.\n')
    ...     rc1, out1, err1 = script_helper.assert_python_ok(
    ...             '-m', 'doctest', fn)
    ...     rc2, out2, err2 = script_helper.assert_python_ok(
    ...             '-m', 'doctest', '-v', fn)

With no arguments and passing tests, we should get no output:

    >>> rc1, out1, err1
    (0, b'', b'')

With the verbose flag, we should see the test output, but no error output:

    >>> rc2, err2
    (0, b'')
    >>> print(normalize(out2))
    Trying:
        1 + 1
    Expecting:
        2
    ok
    Trying:
        "a"
    Expecting:
        'a'
    ok
    1 items passed all tests:
       2 tests in myfile.doc
    2 tests in 1 items.
    2 passed and 0 failed.
    Test passed.

Now we'll write a couple files, one with three tests, the other a python module
with two tests, both of the files having "errors" in the tests that can be made
non-errors by applying the appropriate doctest options to the run (ELLIPSIS in
the first file, NORMALIZE_WHITESPACE in the second).  This combination will
allow thoroughly testing the -f and -o flags, as well as the doctest command's
ability to process more than one file on the command line and, since the second
file ends in '.py', its handling of python module files (as opposed to straight
text files).

    >>> from test.support import script_helper, temp_dir
    >>> with temp_dir() as tmpdir:
    ...     fn = os.path.join(tmpdir, 'myfile.doc')
    ...     with open(fn, 'w') as f:
    ...         _ = f.write('This is another simple test file.\n')
    ...         _ = f.write('   >>> 1 + 1\n')
    ...         _ = f.write('   2\n')
    ...         _ = f.write('   >>> "abcdef"\n')
    ...         _ = f.write("   'a...f'\n")
    ...         _ = f.write('   >>> "ajkml"\n')
    ...         _ = f.write("   'a...l'\n")
    ...         _ = f.write('\n')
    ...         _ = f.write('And that is it.\n')
    ...     fn2 = os.path.join(tmpdir, 'myfile2.py')
    ...     with open(fn2, 'w') as f:
    ...         _ = f.write('def test_func():\n')
    ...         _ = f.write('   \"\"\"\n')
    ...         _ = f.write('   This is simple python test function.\n')
    ...         _ = f.write('       >>> 1 + 1\n')
    ...         _ = f.write('       2\n')
    ...         _ = f.write('       >>> "abc   def"\n')
    ...         _ = f.write("       'abc def'\n")
    ...         _ = f.write("\n")
    ...         _ = f.write('   \"\"\"\n')
    ...     rc1, out1, err1 = script_helper.assert_python_failure(
    ...             '-m', 'doctest', fn, fn2)
    ...     rc2, out2, err2 = script_helper.assert_python_ok(
    ...             '-m', 'doctest', '-o', 'ELLIPSIS', fn)
    ...     rc3, out3, err3 = script_helper.assert_python_ok(
    ...             '-m', 'doctest', '-o', 'ELLIPSIS',
    ...             '-o', 'NORMALIZE_WHITESPACE', fn, fn2)
    ...     rc4, out4, err4 = script_helper.assert_python_failure(
    ...             '-m', 'doctest', '-f', fn, fn2)
    ...     rc5, out5, err5 = script_helper.assert_python_ok(
    ...             '-m', 'doctest', '-v', '-o', 'ELLIPSIS',
    ...             '-o', 'NORMALIZE_WHITESPACE', fn, fn2)

Our first test run will show the errors from the first file (doctest stops if a
file has errors).  Note that doctest test-run error output appears on stdout,
not stderr:

    >>> rc1, err1
    (1, b'')
    >>> print(normalize(out1))                # doctest: +ELLIPSIS
    **********************************************************************
    File "...myfile.doc", line 4, in myfile.doc
    Failed example:
        "abcdef"
    Expected:
        'a...f'
    Got:
        'abcdef'
    **********************************************************************
    File "...myfile.doc", line 6, in myfile.doc
    Failed example:
        "ajkml"
    Expected:
        'a...l'
    Got:
        'ajkml'
    **********************************************************************
    1 items had failures:
       2 of   3 in myfile.doc
    ***Test Failed*** 2 failures.

With -o ELLIPSIS specified, the second run, against just the first file, should
produce no errors, and with -o NORMALIZE_WHITESPACE also specified, neither
should the third, which ran against both files:

    >>> rc2, out2, err2
    (0, b'', b'')
    >>> rc3, out3, err3
    (0, b'', b'')

The fourth run uses FAIL_FAST, so we should see only one error:

    >>> rc4, err4
    (1, b'')
    >>> print(normalize(out4))                # doctest: +ELLIPSIS
    **********************************************************************
    File "...myfile.doc", line 4, in myfile.doc
    Failed example:
        "abcdef"
    Expected:
        'a...f'
    Got:
        'abcdef'
    **********************************************************************
    1 items had failures:
       1 of   2 in myfile.doc
    ***Test Failed*** 1 failures.

The fifth test uses verbose with the two options, so we should get verbose
success output for the tests in both files:

    >>> rc5, err5
    (0, b'')
    >>> print(normalize(out5))
    Trying:
        1 + 1
    Expecting:
        2
    ok
    Trying:
        "abcdef"
    Expecting:
        'a...f'
    ok
    Trying:
        "ajkml"
    Expecting:
        'a...l'
    ok
    1 items passed all tests:
       3 tests in myfile.doc
    3 tests in 1 items.
    3 passed and 0 failed.
    Test passed.
    Trying:
        1 + 1
    Expecting:
        2
    ok
    Trying:
        "abc   def"
    Expecting:
        'abc def'
    ok
    1 items had no tests:
        myfile2
    1 items passed all tests:
       2 tests in myfile2.test_func
    2 tests in 2 items.
    2 passed and 0 failed.
    Test passed.

We should also check some typical error cases.

Invalid file name:

    >>> rc, out, err = script_helper.assert_python_failure(
    ...         '-m', 'doctest', 'nosuchfile')
    >>> rc, out
    (1, b'')
    >>> print(normalize(err))                    # doctest: +ELLIPSIS
    Traceback (most recent call last):
      ...
    FileNotFoundError: [Errno ...] No such file or directory: 'nosuchfile'

Invalid doctest option:

    >>> rc, out, err = script_helper.assert_python_failure(
    ...         '-m', 'doctest', '-o', 'nosuchoption')
    >>> rc, out
    (2, b'')
    >>> print(normalize(err))                    # doctest: +ELLIPSIS
    usage...invalid...nosuchoption...

"""

def test_no_trailing_whitespace_stripping():
    r"""
    The fancy reports had a bug for a long time where any trailing whitespace on
    the reported diff lines was stripped, making it impossible to see the
    differences in line reported as different that differed only in the amount of
    trailing whitespace.  The whitespace still isn't particularly visible unless
    you use NDIFF, but at least it is now there to be found.

    *NOTE*: This snippet was intentionally put inside a raw string to get rid of
    leading whitespace error in executing the example below

    >>> def f(x):
    ...     r'''
    ...     >>> print('\n'.join(['a    ', 'b']))
    ...     a
    ...     b
    ...     '''
    """
    """
    *NOTE*: These doctest are not placed in raw string to depict the trailing whitespace
    using `\x20`

    >>> test = doctest.DocTestFinder().find(f)[0]
    >>> flags = doctest.REPORT_NDIFF
    >>> doctest.DocTestRunner(verbose=False, optionflags=flags).run(test)
    ... # doctest: +ELLIPSIS
    **********************************************************************
    File ..., line 3, in f
    Failed example:
        print('\n'.join(['a    ', 'b']))
    Differences (ndiff with -expected +actual):
        - a
        + a
          b
    TestResults(failed=1, attempted=1)

    *NOTE*: `\x20` is for checking the trailing whitespace on the +a line above.
    We cannot use actual spaces there, as a commit hook prevents from committing
    patches that contain trailing whitespace. More info on Issue 24746.
    """

######################################################################
## Main
######################################################################

def test_main():
    # Check the doctest cases in doctest itself:
    ret = support.run_doctest(doctest, verbosity=True)

    # Check the doctest cases defined here:
    from test import test_doctest
    support.run_doctest(test_doctest, verbosity=True)

    # Run unittests
    support.run_unittest(__name__)


def test_coverage(coverdir):
    trace = support.import_module('trace')
    tracer = trace.Trace(ignoredirs=[sys.base_prefix, sys.base_exec_prefix,],
                         trace=0, count=1)
    tracer.run('test_main()')
    r = tracer.results()
    print('Writing coverage results...')
    r.write_results(show_missing=True, summary=True,
                    coverdir=coverdir)

if __name__ == '__main__':
    if '-c' in sys.argv:
        test_coverage('/tmp/doctest.cover')
    else:
        test_main()
