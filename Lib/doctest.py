# Module doctest.
# Released to the public domain 16-Jan-2001, by Tim Peters (tim@python.org).
# Major enhancements and refactoring by:
#     Jim Fulton
#     Edward Loper

# Provided as-is; use at your own risk; no warranty; no promises; enjoy!

# [XX] This docstring is out-of-date:
r"""Module doctest -- a framework for running examples in docstrings.

NORMAL USAGE

In normal use, end each module M with:

def _test():
    import doctest, M           # replace M with your module's name
    return doctest.testmod(M)   # ditto

if __name__ == "__main__":
    _test()

Then running the module as a script will cause the examples in the
docstrings to get executed and verified:

python M.py

This won't display anything unless an example fails, in which case the
failing example(s) and the cause(s) of the failure(s) are printed to stdout
(why not stderr? because stderr is a lame hack <0.2 wink>), and the final
line of output is "Test failed.".

Run it with the -v switch instead:

python M.py -v

and a detailed report of all examples tried is printed to stdout, along
with assorted summaries at the end.

You can force verbose mode by passing "verbose=1" to testmod, or prohibit
it by passing "verbose=0".  In either of those cases, sys.argv is not
examined by testmod.

In any case, testmod returns a 2-tuple of ints (f, t), where f is the
number of docstring examples that failed and t is the total number of
docstring examples attempted.


WHICH DOCSTRINGS ARE EXAMINED?

+ M.__doc__.

+ f.__doc__ for all functions f in M.__dict__.values(), except those
  defined in other modules.

+ C.__doc__ for all classes C in M.__dict__.values(), except those
  defined in other modules.

+ If M.__test__ exists and "is true", it must be a dict, and
  each entry maps a (string) name to a function object, class object, or
  string.  Function and class object docstrings found from M.__test__
  are searched even if the name is private, and strings are searched
  directly as if they were docstrings.  In output, a key K in M.__test__
  appears with name
      <name of M>.__test__.K

Any classes found are recursively searched similarly, to test docstrings in
their contained methods and nested classes.  All names reached from
M.__test__ are searched.

Optionally, functions with private names can be skipped (unless listed in
M.__test__) by supplying a function to the "isprivate" argument that will
identify private functions.  For convenience, one such function is
supplied.  docttest.is_private considers a name to be private if it begins
with an underscore (like "_my_func") but doesn't both begin and end with
(at least) two underscores (like "__init__").  By supplying this function
or your own "isprivate" function to testmod, the behavior can be customized.

If you want to test docstrings in objects with private names too, stuff
them into an M.__test__ dict, or see ADVANCED USAGE below (e.g., pass your
own isprivate function to Tester's constructor, or call the rundoc method
of a Tester instance).

WHAT'S THE EXECUTION CONTEXT?

By default, each time testmod finds a docstring to test, it uses a *copy*
of M's globals (so that running tests on a module doesn't change the
module's real globals, and so that one test in M can't leave behind crumbs
that accidentally allow another test to work).  This means examples can
freely use any names defined at top-level in M.  It also means that sloppy
imports (see above) can cause examples in external docstrings to use
globals inappropriate for them.

You can force use of your own dict as the execution context by passing
"globs=your_dict" to testmod instead.  Presumably this would be a copy of
M.__dict__ merged with the globals from other imported modules.


WHAT IF I WANT TO TEST A WHOLE PACKAGE?

Piece o' cake, provided the modules do their testing from docstrings.
Here's the test.py I use for the world's most elaborate Rational/
floating-base-conversion pkg (which I'll distribute some day):

from Rational import Cvt
from Rational import Format
from Rational import machprec
from Rational import Rat
from Rational import Round
from Rational import utils

modules = (Cvt,
           Format,
           machprec,
           Rat,
           Round,
           utils)

def _test():
    import doctest
    import sys
    verbose = "-v" in sys.argv
    for mod in modules:
        doctest.testmod(mod, verbose=verbose, report=0)
    doctest.master.summarize()

if __name__ == "__main__":
    _test()

IOW, it just runs testmod on all the pkg modules.  testmod remembers the
names and outcomes (# of failures, # of tries) for each item it's seen, and
passing "report=0" prevents it from printing a summary in verbose mode.
Instead, the summary is delayed until all modules have been tested, and
then "doctest.master.summarize()" forces the summary at the end.

So this is very nice in practice:  each module can be tested individually
with almost no work beyond writing up docstring examples, and collections
of modules can be tested too as a unit with no more work than the above.


WHAT ABOUT EXCEPTIONS?

No problem, as long as the only output generated by the example is the
traceback itself.  For example:

    >>> [1, 2, 3].remove(42)
    Traceback (most recent call last):
      File "<stdin>", line 1, in ?
    ValueError: list.remove(x): x not in list
    >>>

Note that only the exception type and value are compared (specifically,
only the last line in the traceback).


ADVANCED USAGE

doctest.testmod() captures the testing policy I find most useful most
often.  You may want other policies.

testmod() actually creates a local instance of class doctest.Tester, runs
appropriate methods of that class, and merges the results into global
Tester instance doctest.master.

You can create your own instances of doctest.Tester, and so build your own
policies, or even run methods of doctest.master directly.  See
doctest.Tester.__doc__ for details.


SO WHAT DOES A DOCSTRING EXAMPLE LOOK LIKE ALREADY!?

Oh ya.  It's easy!  In most cases a copy-and-paste of an interactive
console session works fine -- just make sure the leading whitespace is
rigidly consistent (you can mix tabs and spaces if you're too lazy to do it
right, but doctest is not in the business of guessing what you think a tab
means).

    >>> # comments are ignored
    >>> x = 12
    >>> x
    12
    >>> if x == 13:
    ...     print "yes"
    ... else:
    ...     print "no"
    ...     print "NO"
    ...     print "NO!!!"
    ...
    no
    NO
    NO!!!
    >>>

Any expected output must immediately follow the final ">>>" or "..." line
containing the code, and the expected output (if any) extends to the next
">>>" or all-whitespace line.  That's it.

Bummers:

+ Expected output cannot contain an all-whitespace line, since such a line
  is taken to signal the end of expected output.

+ Output to stdout is captured, but not output to stderr (exception
  tracebacks are captured via a different means).

+ If you continue a line via backslashing in an interactive session,
  or for any other reason use a backslash, you should use a raw
  docstring, which will preserve your backslahses exactly as you type
  them:

      >>> def f(x):
      ...     r'''Backslashes in a raw docstring: m\n'''
      >>> print f.__doc__
      Backslashes in a raw docstring: m\n

  Otherwise, the backslash will be interpreted as part of the string.
  E.g., the "\n" above would be interpreted as a newline character.
  Alternatively, you can double each backslash in the doctest version
  (and not use a raw string):

      >>> def f(x):
      ...     '''Backslashes in a raw docstring: m\\n'''
      >>> print f.__doc__
      Backslashes in a raw docstring: m\n

The starting column doesn't matter:

>>> assert "Easy!"
     >>> import math
            >>> math.floor(1.9)
            1.0

and as many leading whitespace characters are stripped from the expected
output as appeared in the initial ">>>" line that triggered it.

If you execute this very file, the examples above will be found and
executed, leading to this output in verbose mode:

Running doctest.__doc__
Trying: [1, 2, 3].remove(42)
Expecting:
Traceback (most recent call last):
  File "<stdin>", line 1, in ?
ValueError: list.remove(x): x not in list
ok
Trying: x = 12
Expecting: nothing
ok
Trying: x
Expecting: 12
ok
Trying:
if x == 13:
    print "yes"
else:
    print "no"
    print "NO"
    print "NO!!!"
Expecting:
no
NO
NO!!!
ok
... and a bunch more like that, with this summary at the end:

5 items had no tests:
    doctest.Tester.__init__
    doctest.Tester.run__test__
    doctest.Tester.summarize
    doctest.run_docstring_examples
    doctest.testmod
12 items passed all tests:
   8 tests in doctest
   6 tests in doctest.Tester
  10 tests in doctest.Tester.merge
  14 tests in doctest.Tester.rundict
   3 tests in doctest.Tester.rundoc
   3 tests in doctest.Tester.runstring
   2 tests in doctest.__test__._TestClass
   2 tests in doctest.__test__._TestClass.__init__
   2 tests in doctest.__test__._TestClass.get
   1 tests in doctest.__test__._TestClass.square
   2 tests in doctest.__test__.string
   7 tests in doctest.is_private
60 tests in 17 items.
60 passed and 0 failed.
Test passed.
"""

__all__ = [
    'is_private',
    'Example',
    'DocTest',
    'DocTestFinder',
    'DocTestRunner',
    'testmod',
    'run_docstring_examples',
    'Tester',
    'DocTestCase',
    'DocTestSuite',
    'testsource',
    'debug',
#    'master',
]

import __future__

import sys, traceback, inspect, linecache, os, re, types
import unittest, difflib, tempfile
import warnings
from StringIO import StringIO

# Option constants.
DONT_ACCEPT_TRUE_FOR_1 = 1 << 0
DONT_ACCEPT_BLANKLINE = 1 << 1
NORMALIZE_WHITESPACE = 1 << 2
ELLIPSIS = 1 << 3
UNIFIED_DIFF = 1 << 4
CONTEXT_DIFF = 1 << 5

OPTIONFLAGS_BY_NAME = {
    'DONT_ACCEPT_TRUE_FOR_1': DONT_ACCEPT_TRUE_FOR_1,
    'DONT_ACCEPT_BLANKLINE': DONT_ACCEPT_BLANKLINE,
    'NORMALIZE_WHITESPACE': NORMALIZE_WHITESPACE,
    'ELLIPSIS': ELLIPSIS,
    'UNIFIED_DIFF': UNIFIED_DIFF,
    'CONTEXT_DIFF': CONTEXT_DIFF,
    }

# Special string markers for use in `want` strings:
BLANKLINE_MARKER = '<BLANKLINE>'
ELLIPSIS_MARKER = '...'


# There are 4 basic classes:
#  - Example: a <source, want> pair, plus an intra-docstring line number.
#  - DocTest: a collection of examples, parsed from a docstring, plus
#    info about where the docstring came from (name, filename, lineno).
#  - DocTestFinder: extracts DocTests from a given object's docstring and
#    its contained objects' docstrings.
#  - DocTestRunner: runs DocTest cases, and accumulates statistics.
#
# So the basic picture is:
#
#                             list of:
# +------+                   +---------+                   +-------+
# |object| --DocTestFinder-> | DocTest | --DocTestRunner-> |results|
# +------+                   +---------+                   +-------+
#                            | Example |
#                            |   ...   |
#                            | Example |
#                            +---------+

######################################################################
## Table of Contents
######################################################################
#  1. Utility Functions
#  2. Example & DocTest -- store test cases
#  3. DocTest Parser -- extracts examples from strings
#  4. DocTest Finder -- extracts test cases from objects
#  5. DocTest Runner -- runs test cases
#  6. Test Functions -- convenient wrappers for testing
#  7. Tester Class -- for backwards compatibility
#  8. Unittest Support
#  9. Debugging Support
# 10. Example Usage

######################################################################
## 1. Utility Functions
######################################################################

def is_private(prefix, base):
    """prefix, base -> true iff name prefix + "." + base is "private".

    Prefix may be an empty string, and base does not contain a period.
    Prefix is ignored (although functions you write conforming to this
    protocol may make use of it).
    Return true iff base begins with an (at least one) underscore, but
    does not both begin and end with (at least) two underscores.

    >>> warnings.filterwarnings("ignore", "is_private", DeprecationWarning,
    ...                         "doctest", 0)
    >>> is_private("a.b", "my_func")
    False
    >>> is_private("____", "_my_func")
    True
    >>> is_private("someclass", "__init__")
    False
    >>> is_private("sometypo", "__init_")
    True
    >>> is_private("x.y.z", "_")
    True
    >>> is_private("_x.y.z", "__")
    False
    >>> is_private("", "")  # senseless but consistent
    False
    """
    warnings.warn("is_private is deprecated; it wasn't useful; "
                  "examine DocTestFinder.find() lists instead",
                  DeprecationWarning, stacklevel=2)
    return base[:1] == "_" and not base[:2] == "__" == base[-2:]

def _extract_future_flags(globs):
    """
    Return the compiler-flags associated with the future features that
    have been imported into the given namespace (globs).
    """
    flags = 0
    for fname in __future__.all_feature_names:
        feature = globs.get(fname, None)
        if feature is getattr(__future__, fname):
            flags |= feature.compiler_flag
    return flags

def _normalize_module(module, depth=2):
    """
    Return the module specified by `module`.  In particular:
      - If `module` is a module, then return module.
      - If `module` is a string, then import and return the
        module with that name.
      - If `module` is None, then return the calling module.
        The calling module is assumed to be the module of
        the stack frame at the given depth in the call stack.
    """
    if inspect.ismodule(module):
        return module
    elif isinstance(module, (str, unicode)):
        return __import__(module, globals(), locals(), ["*"])
    elif module is None:
        return sys.modules[sys._getframe(depth).f_globals['__name__']]
    else:
        raise TypeError("Expected a module, string, or None")

def _tag_msg(tag, msg, indent_msg=True):
    """
    Return a string that displays a tag-and-message pair nicely,
    keeping the tag and its message on the same line when that
    makes sense.  If `indent_msg` is true, then messages that are
    put on separate lines will be indented.
    """
    # What string should we use to indent contents?
    INDENT = '    '

    # If the message doesn't end in a newline, then add one.
    if msg[-1:] != '\n':
        msg += '\n'
    # If the message is short enough, and contains no internal
    # newlines, then display it on the same line as the tag.
    # Otherwise, display the tag on its own line.
    if (len(tag) + len(msg) < 75 and
        msg.find('\n', 0, len(msg)-1) == -1):
        return '%s: %s' % (tag, msg)
    else:
        if indent_msg:
            msg = '\n'.join([INDENT+l for l in msg.split('\n')])
            msg = msg[:-len(INDENT)]
        return '%s:\n%s' % (tag, msg)

# Override some StringIO methods.
class _SpoofOut(StringIO):
    def getvalue(self):
        result = StringIO.getvalue(self)
        # If anything at all was written, make sure there's a trailing
        # newline.  There's no way for the expected output to indicate
        # that a trailing newline is missing.
        if result and not result.endswith("\n"):
            result += "\n"
        # Prevent softspace from screwing up the next test case, in
        # case they used print with a trailing comma in an example.
        if hasattr(self, "softspace"):
            del self.softspace
        return result

    def truncate(self,   size=None):
        StringIO.truncate(self, size)
        if hasattr(self, "softspace"):
            del self.softspace

######################################################################
## 2. Example & DocTest
######################################################################
## - An "example" is a <source, want> pair, where "source" is a
##   fragment of source code, and "want" is the expected output for
##   "source."  The Example class also includes information about
##   where the example was extracted from.
##
## - A "doctest" is a collection of examples extracted from a string
##   (such as an object's docstring).  The DocTest class also includes
##   information about where the string was extracted from.

class Example:
    """
    A single doctest example, consisting of source code and expected
    output.  Example defines the following attributes:

      - source:  A single Python statement, always ending with a newline.
        The constructor adds a newline if needed.

      - want:  The expected output from running the source code (either
        from stdout, or a traceback in case of exception).  `want` ends
        with a newline unless it's empty, in which case it's an empty
        string.  The constructor adds a newline if needed.

      - lineno:  The line number within the DocTest string containing
        this Example where the Example begins.  This line number is
        zero-based, with respect to the beginning of the DocTest.
    """
    def __init__(self, source, want, lineno):
        # Normalize inputs.
        if not source.endswith('\n'):
            source += '\n'
        if want and not want.endswith('\n'):
            want += '\n'
        # Store properties.
        self.source = source
        self.want = want
        self.lineno = lineno

class DocTest:
    """
    A collection of doctest examples that should be run in a single
    namespace.  Each DocTest defines the following attributes:

      - examples: the list of examples.

      - globs: The namespace (aka globals) that the examples should
        be run in.

      - name: A name identifying the DocTest (typically, the name of
        the object whose docstring this DocTest was extracted from).

      - docstring: The docstring being tested

      - filename: The name of the file that this DocTest was extracted
        from.

      - lineno: The line number within filename where this DocTest
        begins.  This line number is zero-based, with respect to the
        beginning of the file.
    """
    def __init__(self, docstring, globs, name, filename, lineno):
        """
        Create a new DocTest, by extracting examples from `docstring`.
        The DocTest's globals are initialized with a copy of `globs`.
        """
        # Store a copy of the globals
        self.globs = globs.copy()
        # Store identifying information
        self.name = name
        self.filename = filename
        self.lineno = lineno
        # Parse the docstring.
        self.docstring = docstring
        self.examples = Parser(name, docstring).get_examples()

    def __repr__(self):
        if len(self.examples) == 0:
            examples = 'no examples'
        elif len(self.examples) == 1:
            examples = '1 example'
        else:
            examples = '%d examples' % len(self.examples)
        return ('<DocTest %s from %s:%s (%s)>' %
                (self.name, self.filename, self.lineno, examples))


    # This lets us sort tests by name:
    def __cmp__(self, other):
        if not isinstance(other, DocTest):
            return -1
        return cmp((self.name, self.filename, self.lineno, id(self)),
                   (other.name, other.filename, other.lineno, id(other)))

######################################################################
## 2. Example Parser
######################################################################

class Parser:
    """
    Extract doctests from a string.
    """
    def __init__(self, name, string):
        """
        Prepare to extract doctests from string `string`.

        `name` is an arbitrary (string) name associated with the string,
        and is used only in error messages.
        """
        self.name = name
        self.string = string.expandtabs()

    _EXAMPLE_RE = re.compile(r'''
        # Source consists of a PS1 line followed by zero or more PS2 lines.
        (?P<source>
            (?:^(?P<indent> [ ]*) >>>    .*)    # PS1 line
            (?:\n           [ ]*  \.\.\. .*)*)  # PS2 lines
        \n?
        # Want consists of any non-blank lines that do not start with PS1.
        (?P<want> (?:(?![ ]*$)    # Not a blank line
                     (?![ ]*>>>)  # Not a line starting with PS1
                     .*$\n?       # But any other line
                  )*)
        ''', re.MULTILINE | re.VERBOSE)
    _IS_BLANK_OR_COMMENT = re.compile('^[ ]*(#.*)?$').match

    def get_examples(self):
        """
        Extract all doctest examples, from the string, and return them
        as a list of `Example` objects.  Line numbers are 0-based,
        because it's most common in doctests that nothing interesting
        appears on the same line as opening triple-quote, and so the
        first interesting line is called \"line 1\" then.

        >>> text = '''
        ...        >>> x, y = 2, 3  # no output expected
        ...        >>> if 1:
        ...        ...     print x
        ...        ...     print y
        ...        2
        ...        3
        ...
        ...        Some text.
        ...        >>> x+y
        ...        5
        ...        '''
        >>> for x in Parser('<string>', text).get_examples():
        ...     print (x.source, x.want, x.lineno)
        ('x, y = 2, 3  # no output expected\\n', '', 1)
        ('if 1:\\n    print x\\n    print y\\n', '2\\n3\\n', 2)
        ('x+y\\n', '5\\n', 9)
        """
        examples = []
        charno, lineno = 0, 0
        # Find all doctest examples in the string:
        for m in self._EXAMPLE_RE.finditer(self.string):
            # Update lineno (lines before this example)
            lineno += self.string.count('\n', charno, m.start())

            # Extract source/want from the regexp match.
            (source, want) = self._parse_example(m, lineno)
            if self._IS_BLANK_OR_COMMENT(source):
                continue
            examples.append( Example(source, want, lineno) )

            # Update lineno (lines inside this example)
            lineno += self.string.count('\n', m.start(), m.end())
            # Update charno.
            charno = m.end()
        return examples

    def get_program(self):
        """
        Return an executable program from the string, as a string.

        The format of this isn't rigidly defined.  In general, doctest
        examples become the executable statements in the result, and
        their expected outputs become comments, preceded by an \"#Expected:\"
        comment.  Everything else (text, comments, everything not part of
        a doctest test) is also placed in comments.

        >>> text = '''
        ...        >>> x, y = 2, 3  # no output expected
        ...        >>> if 1:
        ...        ...     print x
        ...        ...     print y
        ...        2
        ...        3
        ...
        ...        Some text.
        ...        >>> x+y
        ...        5
        ...        '''
        >>> print Parser('<string>', text).get_program()
        x, y = 2, 3  # no output expected
        if 1:
            print x
            print y
        # Expected:
        #     2
        #     3
        #
        #         Some text.
        x+y
        # Expected:
        #     5
        """
        output = []
        charnum, lineno = 0, 0
        # Find all doctest examples in the string:
        for m in self._EXAMPLE_RE.finditer(self.string):
            # Add any text before this example, as a comment.
            if m.start() > charnum:
                lines = self.string[charnum:m.start()-1].split('\n')
                output.extend([self._comment_line(l) for l in lines])
                lineno += len(lines)

            # Extract source/want from the regexp match.
            (source, want) = self._parse_example(m, lineno, False)
            # Display the source
            output.append(source)
            # Display the expected output, if any
            if want:
                output.append('# Expected:')
                output.extend(['#     '+l for l in want.split('\n')])

            # Update the line number & char number.
            lineno += self.string.count('\n', m.start(), m.end())
            charnum = m.end()
        # Add any remaining text, as comments.
        output.extend([self._comment_line(l)
                       for l in self.string[charnum:].split('\n')])
        # Trim junk on both ends.
        while output and output[-1] == '#':
            output.pop()
        while output and output[0] == '#':
            output.pop(0)
        # Combine the output, and return it.
        return '\n'.join(output)

    def _parse_example(self, m, lineno, add_newlines=True):
        # Get the example's indentation level.
        indent = len(m.group('indent'))

        # Divide source into lines; check that they're properly
        # indented; and then strip their indentation & prompts.
        source_lines = m.group('source').split('\n')
        self._check_prompt_blank(source_lines, indent, lineno)
        self._check_prefix(source_lines[1:], ' '*indent+'.', lineno)
        source = '\n'.join([sl[indent+4:] for sl in source_lines])
        if len(source_lines) > 1 and add_newlines:
            source += '\n'

        # Divide want into lines; check that it's properly
        # indented; and then strip the indentation.
        want_lines = m.group('want').rstrip().split('\n')
        self._check_prefix(want_lines, ' '*indent,
                           lineno+len(source_lines))
        want = '\n'.join([wl[indent:] for wl in want_lines])
        if len(want) > 0 and add_newlines:
            want += '\n'

        return source, want

    def _comment_line(self, line):
        line = line.rstrip()
        if line:
            return '#  '+line
        else:
            return '#'

    def _check_prompt_blank(self, lines, indent, lineno):
        for i, line in enumerate(lines):
            if len(line) >= indent+4 and line[indent+3] != ' ':
                raise ValueError('line %r of the docstring for %s '
                                 'lacks blank after %s: %r' %
                                 (lineno+i+1, self.name,
                                  line[indent:indent+3], line))

    def _check_prefix(self, lines, prefix, lineno):
        for i, line in enumerate(lines):
            if line and not line.startswith(prefix):
                raise ValueError('line %r of the docstring for %s has '
                                 'inconsistent leading whitespace: %r' %
                                 (lineno+i+1, self.name, line))


######################################################################
## 4. DocTest Finder
######################################################################

class DocTestFinder:
    """
    A class used to extract the DocTests that are relevant to a given
    object, from its docstring and the docstrings of its contained
    objects.  Doctests can currently be extracted from the following
    object types: modules, functions, classes, methods, staticmethods,
    classmethods, and properties.
    """

    def __init__(self, verbose=False, doctest_factory=DocTest,
                 recurse=True, _namefilter=None):
        """
        Create a new doctest finder.

        The optional argument `doctest_factory` specifies a class or
        function that should be used to create new DocTest objects (or
        objects that implement the same interface as DocTest).  The
        signature for this factory function should match the signature
        of the DocTest constructor.

        If the optional argument `recurse` is false, then `find` will
        only examine the given object, and not any contained objects.
        """
        self._doctest_factory = doctest_factory
        self._verbose = verbose
        self._recurse = recurse
        # _namefilter is undocumented, and exists only for temporary backward-
        # compatibility support of testmod's deprecated isprivate mess.
        self._namefilter = _namefilter

    def find(self, obj, name=None, module=None, globs=None,
             extraglobs=None):
        """
        Return a list of the DocTests that are defined by the given
        object's docstring, or by any of its contained objects'
        docstrings.

        The optional parameter `module` is the module that contains
        the given object.  If the module is not specified or is None, then
        the test finder will attempt to automatically determine the
        correct module.  The object's module is used:

            - As a default namespace, if `globs` is not specified.
            - To prevent the DocTestFinder from extracting DocTests
              from objects that are imported from other modules.
            - To find the name of the file containing the object.
            - To help find the line number of the object within its
              file.

        Contained objects whose module does not match `module` are ignored.

        If `module` is False, no attempt to find the module will be made.
        This is obscure, of use mostly in tests:  if `module` is False, or
        is None but cannot be found automatically, then all objects are
        considered to belong to the (non-existent) module, so all contained
        objects will (recursively) be searched for doctests.

        The globals for each DocTest is formed by combining `globs`
        and `extraglobs` (bindings in `extraglobs` override bindings
        in `globs`).  A new copy of the globals dictionary is created
        for each DocTest.  If `globs` is not specified, then it
        defaults to the module's `__dict__`, if specified, or {}
        otherwise.  If `extraglobs` is not specified, then it defaults
        to {}.

        """
        # If name was not specified, then extract it from the object.
        if name is None:
            name = getattr(obj, '__name__', None)
            if name is None:
                raise ValueError("DocTestFinder.find: name must be given "
                        "when obj.__name__ doesn't exist: %r" %
                                 (type(obj),))

        # Find the module that contains the given object (if obj is
        # a module, then module=obj.).  Note: this may fail, in which
        # case module will be None.
        if module is False:
            module = None
        elif module is None:
            module = inspect.getmodule(obj)

        # Read the module's source code.  This is used by
        # DocTestFinder._find_lineno to find the line number for a
        # given object's docstring.
        try:
            file = inspect.getsourcefile(obj) or inspect.getfile(obj)
            source_lines = linecache.getlines(file)
            if not source_lines:
                source_lines = None
        except TypeError:
            source_lines = None

        # Initialize globals, and merge in extraglobs.
        if globs is None:
            if module is None:
                globs = {}
            else:
                globs = module.__dict__.copy()
        else:
            globs = globs.copy()
        if extraglobs is not None:
            globs.update(extraglobs)

        # Recursively expore `obj`, extracting DocTests.
        tests = []
        self._find(tests, obj, name, module, source_lines, globs, {})
        return tests

    def _filter(self, obj, prefix, base):
        """
        Return true if the given object should not be examined.
        """
        return (self._namefilter is not None and
                self._namefilter(prefix, base))

    def _from_module(self, module, object):
        """
        Return true if the given object is defined in the given
        module.
        """
        if module is None:
            return True
        elif inspect.isfunction(object):
            return module.__dict__ is object.func_globals
        elif inspect.isclass(object):
            return module.__name__ == object.__module__
        elif inspect.getmodule(object) is not None:
            return module is inspect.getmodule(object)
        elif hasattr(object, '__module__'):
            return module.__name__ == object.__module__
        elif isinstance(object, property):
            return True # [XX] no way not be sure.
        else:
            raise ValueError("object must be a class or function")

    def _find(self, tests, obj, name, module, source_lines, globs, seen):
        """
        Find tests for the given object and any contained objects, and
        add them to `tests`.
        """
        if self._verbose:
            print 'Finding tests in %s' % name

        # If we've already processed this object, then ignore it.
        if id(obj) in seen:
            return
        seen[id(obj)] = 1

        # Find a test for this object, and add it to the list of tests.
        test = self._get_test(obj, name, module, globs, source_lines)
        if test is not None:
            tests.append(test)

        # Look for tests in a module's contained objects.
        if inspect.ismodule(obj) and self._recurse:
            for valname, val in obj.__dict__.items():
                # Check if this contained object should be ignored.
                if self._filter(val, name, valname):
                    continue
                valname = '%s.%s' % (name, valname)
                # Recurse to functions & classes.
                if ((inspect.isfunction(val) or inspect.isclass(val)) and
                    self._from_module(module, val)):
                    self._find(tests, val, valname, module, source_lines,
                               globs, seen)

        # Look for tests in a module's __test__ dictionary.
        if inspect.ismodule(obj) and self._recurse:
            for valname, val in getattr(obj, '__test__', {}).items():
                if not isinstance(valname, basestring):
                    raise ValueError("DocTestFinder.find: __test__ keys "
                                     "must be strings: %r" %
                                     (type(valname),))
                if not (inspect.isfunction(val) or inspect.isclass(val) or
                        inspect.ismethod(val) or inspect.ismodule(val) or
                        isinstance(val, basestring)):
                    raise ValueError("DocTestFinder.find: __test__ values "
                                     "must be strings, functions, methods, "
                                     "classes, or modules: %r" %
                                     (type(val),))
                valname = '%s.%s' % (name, valname)
                self._find(tests, val, valname, module, source_lines,
                           globs, seen)

        # Look for tests in a class's contained objects.
        if inspect.isclass(obj) and self._recurse:
            for valname, val in obj.__dict__.items():
                # Check if this contained object should be ignored.
                if self._filter(val, name, valname):
                    continue
                # Special handling for staticmethod/classmethod.
                if isinstance(val, staticmethod):
                    val = getattr(obj, valname)
                if isinstance(val, classmethod):
                    val = getattr(obj, valname).im_func

                # Recurse to methods, properties, and nested classes.
                if ((inspect.isfunction(val) or inspect.isclass(val) or
                      isinstance(val, property)) and
                      self._from_module(module, val)):
                    valname = '%s.%s' % (name, valname)
                    self._find(tests, val, valname, module, source_lines,
                               globs, seen)

    def _get_test(self, obj, name, module, globs, source_lines):
        """
        Return a DocTest for the given object, if it defines a docstring;
        otherwise, return None.
        """
        # Extract the object's docstring.  If it doesn't have one,
        # then return None (no test for this object).
        if isinstance(obj, basestring):
            docstring = obj
        else:
            try:
                if obj.__doc__ is None:
                    return None
                docstring = str(obj.__doc__)
            except (TypeError, AttributeError):
                return None

        # Don't bother if the docstring is empty.
        if not docstring:
            return None

        # Find the docstring's location in the file.
        lineno = self._find_lineno(obj, source_lines)

        # Return a DocTest for this object.
        if module is None:
            filename = None
        else:
            filename = getattr(module, '__file__', module.__name__)
        return self._doctest_factory(docstring, globs, name, filename, lineno)

    def _find_lineno(self, obj, source_lines):
        """
        Return a line number of the given object's docstring.  Note:
        this method assumes that the object has a docstring.
        """
        lineno = None

        # Find the line number for modules.
        if inspect.ismodule(obj):
            lineno = 0

        # Find the line number for classes.
        # Note: this could be fooled if a class is defined multiple
        # times in a single file.
        if inspect.isclass(obj):
            if source_lines is None:
                return None
            pat = re.compile(r'^\s*class\s*%s\b' %
                             getattr(obj, '__name__', '-'))
            for i, line in enumerate(source_lines):
                if pat.match(line):
                    lineno = i
                    break

        # Find the line number for functions & methods.
        if inspect.ismethod(obj): obj = obj.im_func
        if inspect.isfunction(obj): obj = obj.func_code
        if inspect.istraceback(obj): obj = obj.tb_frame
        if inspect.isframe(obj): obj = obj.f_code
        if inspect.iscode(obj):
            lineno = getattr(obj, 'co_firstlineno', None)-1

        # Find the line number where the docstring starts.  Assume
        # that it's the first line that begins with a quote mark.
        # Note: this could be fooled by a multiline function
        # signature, where a continuation line begins with a quote
        # mark.
        if lineno is not None:
            if source_lines is None:
                return lineno+1
            pat = re.compile('(^|.*:)\s*\w*("|\')')
            for lineno in range(lineno, len(source_lines)):
                if pat.match(source_lines[lineno]):
                    return lineno

        # We couldn't find the line number.
        return None

######################################################################
## 5. DocTest Runner
######################################################################

class DocTestRunner:
    """
    A class used to run DocTest test cases, and accumulate statistics.
    The `run` method is used to process a single DocTest case.  It
    returns a tuple `(f, t)`, where `t` is the number of test cases
    tried, and `f` is the number of test cases that failed.

        >>> tests = DocTestFinder().find(_TestClass)
        >>> runner = DocTestRunner(verbose=False)
        >>> for test in tests:
        ...     print runner.run(test)
        (0, 2)
        (0, 1)
        (0, 2)
        (0, 2)

    The `summarize` method prints a summary of all the test cases that
    have been run by the runner, and returns an aggregated `(f, t)`
    tuple:

        >>> runner.summarize(verbose=1)
        4 items passed all tests:
           2 tests in _TestClass
           2 tests in _TestClass.__init__
           2 tests in _TestClass.get
           1 tests in _TestClass.square
        7 tests in 4 items.
        7 passed and 0 failed.
        Test passed.
        (0, 7)

    The aggregated number of tried examples and failed examples is
    also available via the `tries` and `failures` attributes:

        >>> runner.tries
        7
        >>> runner.failures
        0

    The comparison between expected outputs and actual outputs is done
    by an `OutputChecker`.  This comparison may be customized with a
    number of option flags; see the documentation for `testmod` for
    more information.  If the option flags are insufficient, then the
    comparison may also be customized by passing a subclass of
    `OutputChecker` to the constructor.

    The test runner's display output can be controlled in two ways.
    First, an output function (`out) can be passed to
    `TestRunner.run`; this function will be called with strings that
    should be displayed.  It defaults to `sys.stdout.write`.  If
    capturing the output is not sufficient, then the display output
    can be also customized by subclassing DocTestRunner, and
    overriding the methods `report_start`, `report_success`,
    `report_unexpected_exception`, and `report_failure`.
    """
    # This divider string is used to separate failure messages, and to
    # separate sections of the summary.
    DIVIDER = "*" * 70

    def __init__(self, checker=None, verbose=None, optionflags=0):
        """
        Create a new test runner.

        Optional keyword arg `checker` is the `OutputChecker` that
        should be used to compare the expected outputs and actual
        outputs of doctest examples.

        Optional keyword arg 'verbose' prints lots of stuff if true,
        only failures if false; by default, it's true iff '-v' is in
        sys.argv.

        Optional argument `optionflags` can be used to control how the
        test runner compares expected output to actual output, and how
        it displays failures.  See the documentation for `testmod` for
        more information.
        """
        self._checker = checker or OutputChecker()
        if verbose is None:
            verbose = '-v' in sys.argv
        self._verbose = verbose
        self.optionflags = optionflags

        # Keep track of the examples we've run.
        self.tries = 0
        self.failures = 0
        self._name2ft = {}

        # Create a fake output target for capturing doctest output.
        self._fakeout = _SpoofOut()

    #/////////////////////////////////////////////////////////////////
    # Reporting methods
    #/////////////////////////////////////////////////////////////////

    def report_start(self, out, test, example):
        """
        Report that the test runner is about to process the given
        example.  (Only displays a message if verbose=True)
        """
        if self._verbose:
            out(_tag_msg("Trying", example.source) +
                _tag_msg("Expecting", example.want or "nothing"))

    def report_success(self, out, test, example, got):
        """
        Report that the given example ran successfully.  (Only
        displays a message if verbose=True)
        """
        if self._verbose:
            out("ok\n")

    def report_failure(self, out, test, example, got):
        """
        Report that the given example failed.
        """
        # Print an error message.
        out(self.__failure_header(test, example) +
            self._checker.output_difference(example.want, got,
                                            self.optionflags))

    def report_unexpected_exception(self, out, test, example, exc_info):
        """
        Report that the given example raised an unexpected exception.
        """
        # Get a traceback message.
        excout = StringIO()
        exc_type, exc_val, exc_tb = exc_info
        traceback.print_exception(exc_type, exc_val, exc_tb, file=excout)
        exception_tb = excout.getvalue()
        # Print an error message.
        out(self.__failure_header(test, example) +
            _tag_msg("Exception raised", exception_tb))

    def __failure_header(self, test, example):
        s = (self.DIVIDER + "\n" +
             _tag_msg("Failure in example", example.source))
        if test.filename is None:
            # [XX] I'm not putting +1 here, to give the same output
            # as the old version.  But I think it *should* go here.
            return s + ("from line #%s of %s\n" %
                        (example.lineno, test.name))
        elif test.lineno is None:
            return s + ("from line #%s of %s in %s\n" %
                        (example.lineno+1, test.name, test.filename))
        else:
            lineno = test.lineno+example.lineno+1
            return s + ("from line #%s of %s (%s)\n" %
                        (lineno, test.filename, test.name))

    #/////////////////////////////////////////////////////////////////
    # DocTest Running
    #/////////////////////////////////////////////////////////////////

    # A regular expression for handling `want` strings that contain
    # expected exceptions.  It divides `want` into two pieces: the
    # pre-exception output (`out`) and the exception message (`exc`),
    # as generated by traceback.format_exception_only().  (I assume
    # that the exception_only message is the first non-indented line
    # starting with word characters after the "Traceback ...".)
    _EXCEPTION_RE = re.compile(('^(?P<out>.*)'
                                '^(?P<hdr>Traceback \((?:%s|%s)\):)\s*$.*?'
                                '^(?P<exc>\w+.*)') %
                               ('most recent call last', 'innermost last'),
                               re.MULTILINE | re.DOTALL)

    _OPTION_DIRECTIVE_RE = re.compile('\s*doctest:\s*(?P<flags>[^#\n]*)')

    def __handle_directive(self, example):
        """
        Check if the given example is actually a directive to doctest
        (to turn an optionflag on or off); and if it is, then handle
        the directive.

        Return true iff the example is actually a directive (and so
        should not be executed).

        """
        m = self._OPTION_DIRECTIVE_RE.match(example.source)
        if m is None:
            return False

        for flag in m.group('flags').upper().split():
            if (flag[:1] not in '+-' or
                flag[1:] not in OPTIONFLAGS_BY_NAME):
                raise ValueError('Bad doctest option directive: '+flag)
            if flag[0] == '+':
                self.optionflags |= OPTIONFLAGS_BY_NAME[flag[1:]]
            else:
                self.optionflags &= ~OPTIONFLAGS_BY_NAME[flag[1:]]
        return True

    def __run(self, test, compileflags, out):
        """
        Run the examples in `test`.  Write the outcome of each example
        with one of the `DocTestRunner.report_*` methods, using the
        writer function `out`.  `compileflags` is the set of compiler
        flags that should be used to execute examples.  Return a tuple
        `(f, t)`, where `t` is the number of examples tried, and `f`
        is the number of examples that failed.  The examples are run
        in the namespace `test.globs`.
        """
        # Keep track of the number of failures and tries.
        failures = tries = 0

        # Save the option flags (since option directives can be used
        # to modify them).
        original_optionflags = self.optionflags

        # Process each example.
        for example in test.examples:
            # Check if it's an option directive.  If it is, then handle
            # it, and go on to the next example.
            if self.__handle_directive(example):
                continue

            # Record that we started this example.
            tries += 1
            self.report_start(out, test, example)

            # Run the example in the given context (globs), and record
            # any exception that gets raised.  (But don't intercept
            # keyboard interrupts.)
            try:
                # Don't blink!  This is where the user's code gets run.
                exec compile(example.source, "<string>", "single",
                             compileflags, 1) in test.globs
                exception = None
            except KeyboardInterrupt:
                raise
            except:
                exception = sys.exc_info()

            got = self._fakeout.getvalue()  # the actual output
            self._fakeout.truncate(0)

            # If the example executed without raising any exceptions,
            # then verify its output and report its outcome.
            if exception is None:
                if self._checker.check_output(example.want, got,
                                              self.optionflags):
                    self.report_success(out, test, example, got)
                else:
                    self.report_failure(out, test, example, got)
                    failures += 1

            # If the example raised an exception, then check if it was
            # expected.
            else:
                exc_info = sys.exc_info()
                exc_msg = traceback.format_exception_only(*exc_info[:2])[-1]

                # Search the `want` string for an exception.  If we don't
                # find one, then report an unexpected exception.
                m = self._EXCEPTION_RE.match(example.want)
                if m is None:
                    self.report_unexpected_exception(out, test, example,
                                                     exc_info)
                    failures += 1
                else:
                    exc_hdr = m.group('hdr')+'\n' # Exception header
                    # The test passes iff the pre-exception output and
                    # the exception description match the values given
                    # in `want`.
                    if (self._checker.check_output(m.group('out'), got,
                                                   self.optionflags) and
                        self._checker.check_output(m.group('exc'), exc_msg,
                                                   self.optionflags)):
                        # Is +exc_msg the right thing here??
                        self.report_success(out, test, example,
                                            got+exc_hdr+exc_msg)
                    else:
                        self.report_failure(out, test, example,
                                            got+exc_hdr+exc_msg)
                        failures += 1

        # Restore the option flags (in case they were modified)
        self.optionflags = original_optionflags

        # Record and return the number of failures and tries.
        self.__record_outcome(test, failures, tries)
        return failures, tries

    def __record_outcome(self, test, f, t):
        """
        Record the fact that the given DocTest (`test`) generated `f`
        failures out of `t` tried examples.
        """
        f2, t2 = self._name2ft.get(test.name, (0,0))
        self._name2ft[test.name] = (f+f2, t+t2)
        self.failures += f
        self.tries += t

    def run(self, test, compileflags=None, out=None, clear_globs=True):
        """
        Run the examples in `test`, and display the results using the
        writer function `out`.

        The examples are run in the namespace `test.globs`.  If
        `clear_globs` is true (the default), then this namespace will
        be cleared after the test runs, to help with garbage
        collection.  If you would like to examine the namespace after
        the test completes, then use `clear_globs=False`.

        `compileflags` gives the set of flags that should be used by
        the Python compiler when running the examples.  If not
        specified, then it will default to the set of future-import
        flags that apply to `globs`.

        The output of each example is checked using
        `DocTestRunner.check_output`, and the results are formatted by
        the `DocTestRunner.report_*` methods.
        """
        if compileflags is None:
            compileflags = _extract_future_flags(test.globs)
        if out is None:
            out = sys.stdout.write
        saveout = sys.stdout

        try:
            sys.stdout = self._fakeout
            return self.__run(test, compileflags, out)
        finally:
            sys.stdout = saveout
            if clear_globs:
                test.globs.clear()

    #/////////////////////////////////////////////////////////////////
    # Summarization
    #/////////////////////////////////////////////////////////////////
    def summarize(self, verbose=None):
        """
        Print a summary of all the test cases that have been run by
        this DocTestRunner, and return a tuple `(f, t)`, where `f` is
        the total number of failed examples, and `t` is the total
        number of tried examples.

        The optional `verbose` argument controls how detailed the
        summary is.  If the verbosity is not specified, then the
        DocTestRunner's verbosity is used.
        """
        if verbose is None:
            verbose = self._verbose
        notests = []
        passed = []
        failed = []
        totalt = totalf = 0
        for x in self._name2ft.items():
            name, (f, t) = x
            assert f <= t
            totalt += t
            totalf += f
            if t == 0:
                notests.append(name)
            elif f == 0:
                passed.append( (name, t) )
            else:
                failed.append(x)
        if verbose:
            if notests:
                print len(notests), "items had no tests:"
                notests.sort()
                for thing in notests:
                    print "   ", thing
            if passed:
                print len(passed), "items passed all tests:"
                passed.sort()
                for thing, count in passed:
                    print " %3d tests in %s" % (count, thing)
        if failed:
            print self.DIVIDER
            print len(failed), "items had failures:"
            failed.sort()
            for thing, (f, t) in failed:
                print " %3d of %3d in %s" % (f, t, thing)
        if verbose:
            print totalt, "tests in", len(self._name2ft), "items."
            print totalt - totalf, "passed and", totalf, "failed."
        if totalf:
            print "***Test Failed***", totalf, "failures."
        elif verbose:
            print "Test passed."
        return totalf, totalt

class OutputChecker:
    """
    A class used to check the whether the actual output from a doctest
    example matches the expected output.  `OutputChecker` defines two
    methods: `check_output`, which compares a given pair of outputs,
    and returns true if they match; and `output_difference`, which
    returns a string describing the differences between two outputs.
    """
    def check_output(self, want, got, optionflags):
        """
        Return True iff the actual output (`got`) matches the expected
        output (`want`).  These strings are always considered to match
        if they are identical; but depending on what option flags the
        test runner is using, several non-exact match types are also
        possible.  See the documentation for `TestRunner` for more
        information about option flags.
        """
        # Handle the common case first, for efficiency:
        # if they're string-identical, always return true.
        if got == want:
            return True

        # The values True and False replaced 1 and 0 as the return
        # value for boolean comparisons in Python 2.3.
        if not (optionflags & DONT_ACCEPT_TRUE_FOR_1):
            if (got,want) == ("True\n", "1\n"):
                return True
            if (got,want) == ("False\n", "0\n"):
                return True

        # <BLANKLINE> can be used as a special sequence to signify a
        # blank line, unless the DONT_ACCEPT_BLANKLINE flag is used.
        if not (optionflags & DONT_ACCEPT_BLANKLINE):
            # Replace <BLANKLINE> in want with a blank line.
            want = re.sub('(?m)^%s\s*?$' % re.escape(BLANKLINE_MARKER),
                          '', want)
            # If a line in got contains only spaces, then remove the
            # spaces.
            got = re.sub('(?m)^\s*?$', '', got)
            if got == want:
                return True

        # This flag causes doctest to ignore any differences in the
        # contents of whitespace strings.  Note that this can be used
        # in conjunction with the ELLISPIS flag.
        if (optionflags & NORMALIZE_WHITESPACE):
            got = ' '.join(got.split())
            want = ' '.join(want.split())
            if got == want:
                return True

        # The ELLIPSIS flag says to let the sequence "..." in `want`
        # match any substring in `got`.  We implement this by
        # transforming `want` into a regular expression.
        if (optionflags & ELLIPSIS):
            # Escape any special regexp characters
            want_re = re.escape(want)
            # Replace ellipsis markers ('...') with .*
            want_re = want_re.replace(re.escape(ELLIPSIS_MARKER), '.*')
            # Require that it matches the entire string; and set the
            # re.DOTALL flag (with '(?s)').
            want_re = '(?s)^%s$' % want_re
            # Check if the `want_re` regexp matches got.
            if re.match(want_re, got):
                return True

        # We didn't find any match; return false.
        return False

    def output_difference(self, want, got, optionflags):
        """
        Return a string describing the differences between the
        expected output (`want`) and the actual output (`got`).
        """
        # If <BLANKLINE>s are being used, then replace <BLANKLINE>
        # with blank lines in the expected output string.
        if not (optionflags & DONT_ACCEPT_BLANKLINE):
            want = re.sub('(?m)^%s$' % re.escape(BLANKLINE_MARKER), '', want)

        # Check if we should use diff.  Don't use diff if the actual
        # or expected outputs are too short, or if the expected output
        # contains an ellipsis marker.
        if ((optionflags & (UNIFIED_DIFF | CONTEXT_DIFF)) and
            want.count('\n') > 2 and got.count('\n') > 2 and
            not (optionflags & ELLIPSIS and '...' in want)):
            # Split want & got into lines.
            want_lines = [l+'\n' for l in want.split('\n')]
            got_lines = [l+'\n' for l in got.split('\n')]
            # Use difflib to find their differences.
            if optionflags & UNIFIED_DIFF:
                diff = difflib.unified_diff(want_lines, got_lines, n=2,
                                            fromfile='Expected', tofile='Got')
                kind = 'unified'
            elif optionflags & CONTEXT_DIFF:
                diff = difflib.context_diff(want_lines, got_lines, n=2,
                                            fromfile='Expected', tofile='Got')
                kind = 'context'
            else:
                assert 0, 'Bad diff option'
            # Remove trailing whitespace on diff output.
            diff = [line.rstrip() + '\n' for line in diff]
            return _tag_msg("Differences (" + kind + " diff)",
                            ''.join(diff))

        # If we're not using diff, then simply list the expected
        # output followed by the actual output.
        return (_tag_msg("Expected", want or "Nothing") +
                _tag_msg("Got", got))

class DocTestFailure(Exception):
    """A DocTest example has failed in debugging mode.

    The exception instance has variables:

    - test: the DocTest object being run

    - excample: the Example object that failed

    - got: the actual output
    """
    def __init__(self, test, example, got):
        self.test = test
        self.example = example
        self.got = got

    def __str__(self):
        return str(self.test)

class UnexpectedException(Exception):
    """A DocTest example has encountered an unexpected exception

    The exception instance has variables:

    - test: the DocTest object being run

    - excample: the Example object that failed

    - exc_info: the exception info
    """
    def __init__(self, test, example, exc_info):
        self.test = test
        self.example = example
        self.exc_info = exc_info

    def __str__(self):
        return str(self.test)

class DebugRunner(DocTestRunner):
    r"""Run doc tests but raise an exception as soon as there is a failure.

       If an unexpected exception occurs, an UnexpectedException is raised.
       It contains the test, the example, and the original exception:

         >>> runner = DebugRunner(verbose=False)
         >>> test = DocTest('>>> raise KeyError\n42', {}, 'foo', 'foo.py', 0)
         >>> try:
         ...     runner.run(test)
         ... except UnexpectedException, failure:
         ...     pass

         >>> failure.test is test
         True

         >>> failure.example.want
         '42\n'

         >>> exc_info = failure.exc_info
         >>> raise exc_info[0], exc_info[1], exc_info[2]
         Traceback (most recent call last):
         ...
         KeyError

       We wrap the original exception to give the calling application
       access to the test and example information.

       If the output doesn't match, then a DocTestFailure is raised:

         >>> test = DocTest('''
         ...      >>> x = 1
         ...      >>> x
         ...      2
         ...      ''', {}, 'foo', 'foo.py', 0)

         >>> try:
         ...    runner.run(test)
         ... except DocTestFailure, failure:
         ...    pass

       DocTestFailure objects provide access to the test:

         >>> failure.test is test
         True

       As well as to the example:

         >>> failure.example.want
         '2\n'

       and the actual output:

         >>> failure.got
         '1\n'

       If a failure or error occurs, the globals are left intact:

         >>> del test.globs['__builtins__']
         >>> test.globs
         {'x': 1}

         >>> test = DocTest('''
         ...      >>> x = 2
         ...      >>> raise KeyError
         ...      ''', {}, 'foo', 'foo.py', 0)

         >>> runner.run(test)
         Traceback (most recent call last):
         ...
         UnexpectedException: <DocTest foo from foo.py:0 (2 examples)>

         >>> del test.globs['__builtins__']
         >>> test.globs
         {'x': 2}

       But the globals are cleared if there is no error:

         >>> test = DocTest('''
         ...      >>> x = 2
         ...      ''', {}, 'foo', 'foo.py', 0)

         >>> runner.run(test)
         (0, 1)

         >>> test.globs
         {}

       """

    def run(self, test, compileflags=None, out=None, clear_globs=True):
        r = DocTestRunner.run(self, test, compileflags, out, False)
        if clear_globs:
            test.globs.clear()
        return r

    def report_unexpected_exception(self, out, test, example, exc_info):
        raise UnexpectedException(test, example, exc_info)

    def report_failure(self, out, test, example, got):
        raise DocTestFailure(test, example, got)

######################################################################
## 6. Test Functions
######################################################################
# These should be backwards compatible.

def testmod(m=None, name=None, globs=None, verbose=None, isprivate=None,
            report=True, optionflags=0, extraglobs=None,
            raise_on_error=False):
    """m=None, name=None, globs=None, verbose=None, isprivate=None,
       report=True, optionflags=0, extraglobs=None

    Test examples in docstrings in functions and classes reachable
    from module m (or the current module if m is not supplied), starting
    with m.__doc__.  Unless isprivate is specified, private names
    are not skipped.

    Also test examples reachable from dict m.__test__ if it exists and is
    not None.  m.__dict__ maps names to functions, classes and strings;
    function and class docstrings are tested even if the name is private;
    strings are tested directly, as if they were docstrings.

    Return (#failures, #tests).

    See doctest.__doc__ for an overview.

    Optional keyword arg "name" gives the name of the module; by default
    use m.__name__.

    Optional keyword arg "globs" gives a dict to be used as the globals
    when executing examples; by default, use m.__dict__.  A copy of this
    dict is actually used for each docstring, so that each docstring's
    examples start with a clean slate.

    Optional keyword arg "extraglobs" gives a dictionary that should be
    merged into the globals that are used to execute examples.  By
    default, no extra globals are used.  This is new in 2.4.

    Optional keyword arg "verbose" prints lots of stuff if true, prints
    only failures if false; by default, it's true iff "-v" is in sys.argv.

    Optional keyword arg "report" prints a summary at the end when true,
    else prints nothing at the end.  In verbose mode, the summary is
    detailed, else very brief (in fact, empty if all tests passed).

    Optional keyword arg "optionflags" or's together module constants,
    and defaults to 0.  This is new in 2.3.  Possible values:

        DONT_ACCEPT_TRUE_FOR_1
            By default, if an expected output block contains just "1",
            an actual output block containing just "True" is considered
            to be a match, and similarly for "0" versus "False".  When
            DONT_ACCEPT_TRUE_FOR_1 is specified, neither substitution
            is allowed.

        DONT_ACCEPT_BLANKLINE
            By default, if an expected output block contains a line
            containing only the string "<BLANKLINE>", then that line
            will match a blank line in the actual output.  When
            DONT_ACCEPT_BLANKLINE is specified, this substitution is
            not allowed.

        NORMALIZE_WHITESPACE
            When NORMALIZE_WHITESPACE is specified, all sequences of
            whitespace are treated as equal.  I.e., any sequence of
            whitespace within the expected output will match any
            sequence of whitespace within the actual output.

        ELLIPSIS
            When ELLIPSIS is specified, then an ellipsis marker
            ("...") in the expected output can match any substring in
            the actual output.

        UNIFIED_DIFF
            When UNIFIED_DIFF is specified, failures that involve
            multi-line expected and actual outputs will be displayed
            using a unified diff.

        CONTEXT_DIFF
            When CONTEXT_DIFF is specified, failures that involve
            multi-line expected and actual outputs will be displayed
            using a context diff.

    Optional keyword arg "raise_on_error" raises an exception on the
    first unexpected exception or failure. This allows failures to be
    post-mortem debugged.

    Deprecated in Python 2.4:
    Optional keyword arg "isprivate" specifies a function used to
    determine whether a name is private.  The default function is
    treat all functions as public.  Optionally, "isprivate" can be
    set to doctest.is_private to skip over functions marked as private
    using the underscore naming convention; see its docs for details.
    """

    """ [XX] This is no longer true:
    Advanced tomfoolery:  testmod runs methods of a local instance of
    class doctest.Tester, then merges the results into (or creates)
    global Tester instance doctest.master.  Methods of doctest.master
    can be called directly too, if you want to do something unusual.
    Passing report=0 to testmod is especially useful then, to delay
    displaying a summary.  Invoke doctest.master.summarize(verbose)
    when you're done fiddling.
    """
    if isprivate is not None:
        warnings.warn("the isprivate argument is deprecated; "
                      "examine DocTestFinder.find() lists instead",
                      DeprecationWarning)

    # If no module was given, then use __main__.
    if m is None:
        # DWA - m will still be None if this wasn't invoked from the command
        # line, in which case the following TypeError is about as good an error
        # as we should expect
        m = sys.modules.get('__main__')

    # Check that we were actually given a module.
    if not inspect.ismodule(m):
        raise TypeError("testmod: module required; %r" % (m,))

    # If no name was given, then use the module's name.
    if name is None:
        name = m.__name__

    # Find, parse, and run all tests in the given module.
    finder = DocTestFinder(_namefilter=isprivate)

    if raise_on_error:
        runner = DebugRunner(verbose=verbose, optionflags=optionflags)
    else:
        runner = DocTestRunner(verbose=verbose, optionflags=optionflags)

    for test in finder.find(m, name, globs=globs, extraglobs=extraglobs):
        runner.run(test)

    if report:
        runner.summarize()

    return runner.failures, runner.tries

def run_docstring_examples(f, globs, verbose=False, name="NoName",
                           compileflags=None, optionflags=0):
    """
    Test examples in the given object's docstring (`f`), using `globs`
    as globals.  Optional argument `name` is used in failure messages.
    If the optional argument `verbose` is true, then generate output
    even if there are no failures.

    `compileflags` gives the set of flags that should be used by the
    Python compiler when running the examples.  If not specified, then
    it will default to the set of future-import flags that apply to
    `globs`.

    Optional keyword arg `optionflags` specifies options for the
    testing and output.  See the documentation for `testmod` for more
    information.
    """
    # Find, parse, and run all tests in the given module.
    finder = DocTestFinder(verbose=verbose, recurse=False)
    runner = DocTestRunner(verbose=verbose, optionflags=optionflags)
    for test in finder.find(f, name, globs=globs):
        runner.run(test, compileflags=compileflags)

######################################################################
## 7. Tester
######################################################################
# This is provided only for backwards compatibility.  It's not
# actually used in any way.

class Tester:
    def __init__(self, mod=None, globs=None, verbose=None,
                 isprivate=None, optionflags=0):

        warnings.warn("class Tester is deprecated; "
                      "use class doctest.DocTestRunner instead",
                      DeprecationWarning, stacklevel=2)
        if mod is None and globs is None:
            raise TypeError("Tester.__init__: must specify mod or globs")
        if mod is not None and not _ismodule(mod):
            raise TypeError("Tester.__init__: mod must be a module; %r" %
                            (mod,))
        if globs is None:
            globs = mod.__dict__
        self.globs = globs

        self.verbose = verbose
        self.isprivate = isprivate
        self.optionflags = optionflags
        self.testfinder = DocTestFinder(_namefilter=isprivate)
        self.testrunner = DocTestRunner(verbose=verbose,
                                        optionflags=optionflags)

    def runstring(self, s, name):
        test = DocTest(s, self.globs, name, None, None)
        if self.verbose:
            print "Running string", name
        (f,t) = self.testrunner.run(test)
        if self.verbose:
            print f, "of", t, "examples failed in string", name
        return (f,t)

    def rundoc(self, object, name=None, module=None):
        f = t = 0
        tests = self.testfinder.find(object, name, module=module,
                                     globs=self.globs)
        for test in tests:
            (f2, t2) = self.testrunner.run(test)
            (f,t) = (f+f2, t+t2)
        return (f,t)

    def rundict(self, d, name, module=None):
        import new
        m = new.module(name)
        m.__dict__.update(d)
        if module is None:
            module = False
        return self.rundoc(m, name, module)

    def run__test__(self, d, name):
        import new
        m = new.module(name)
        m.__test__ = d
        return self.rundoc(m, name, module)

    def summarize(self, verbose=None):
        return self.testrunner.summarize(verbose)

    def merge(self, other):
        d = self.testrunner._name2ft
        for name, (f, t) in other.testrunner._name2ft.items():
            if name in d:
                print "*** Tester.merge: '" + name + "' in both" \
                    " testers; summing outcomes."
                f2, t2 = d[name]
                f = f + f2
                t = t + t2
            d[name] = f, t

######################################################################
## 8. Unittest Support
######################################################################

class DocTestCase(unittest.TestCase):

    def __init__(self, test, optionflags=0, setUp=None, tearDown=None,
                 checker=None):
        unittest.TestCase.__init__(self)
        self._dt_optionflags = optionflags
        self._dt_checker = checker
        self._dt_test = test
        self._dt_setUp = setUp
        self._dt_tearDown = tearDown

    def setUp(self):
        if self._dt_setUp is not None:
            self._dt_setUp()

    def tearDown(self):
        if self._dt_tearDown is not None:
            self._dt_tearDown()

    def runTest(self):
        test = self._dt_test
        old = sys.stdout
        new = StringIO()
        runner = DocTestRunner(optionflags=self._dt_optionflags,
                               checker=self._dt_checker, verbose=False)

        try:
            runner.DIVIDER = "-"*70
            failures, tries = runner.run(test, out=new.write)
        finally:
            sys.stdout = old

        if failures:
            raise self.failureException(self.format_failure(new.getvalue()))

    def format_failure(self, err):
        test = self._dt_test
        if test.lineno is None:
            lineno = 'unknown line number'
        else:
            lineno = 'line %s' % test.lineno
        lname = '.'.join(test.name.split('.')[-1:])
        return ('Failed doctest test for %s\n'
                '  File "%s", line %s, in %s\n\n%s'
                % (test.name, test.filename, lineno, lname, err)
                )

    def debug(self):
        r"""Run the test case without results and without catching exceptions

           The unit test framework includes a debug method on test cases
           and test suites to support post-mortem debugging.  The test code
           is run in such a way that errors are not caught.  This way a
           caller can catch the errors and initiate post-mortem debugging.

           The DocTestCase provides a debug method that raises
           UnexpectedException errors if there is an unexepcted
           exception:

             >>> test = DocTest('>>> raise KeyError\n42',
             ...                {}, 'foo', 'foo.py', 0)
             >>> case = DocTestCase(test)
             >>> try:
             ...     case.debug()
             ... except UnexpectedException, failure:
             ...     pass

           The UnexpectedException contains the test, the example, and
           the original exception:

             >>> failure.test is test
             True

             >>> failure.example.want
             '42\n'

             >>> exc_info = failure.exc_info
             >>> raise exc_info[0], exc_info[1], exc_info[2]
             Traceback (most recent call last):
             ...
             KeyError

           If the output doesn't match, then a DocTestFailure is raised:

             >>> test = DocTest('''
             ...      >>> x = 1
             ...      >>> x
             ...      2
             ...      ''', {}, 'foo', 'foo.py', 0)
             >>> case = DocTestCase(test)

             >>> try:
             ...    case.debug()
             ... except DocTestFailure, failure:
             ...    pass

           DocTestFailure objects provide access to the test:

             >>> failure.test is test
             True

           As well as to the example:

             >>> failure.example.want
             '2\n'

           and the actual output:

             >>> failure.got
             '1\n'

           """

        runner = DebugRunner(optionflags=self._dt_optionflags,
                             checker=self._dt_checker, verbose=False)
        runner.run(self._dt_test, out=nooutput)

    def id(self):
        return self._dt_test.name

    def __repr__(self):
        name = self._dt_test.name.split('.')
        return "%s (%s)" % (name[-1], '.'.join(name[:-1]))

    __str__ = __repr__

    def shortDescription(self):
        return "Doctest: " + self._dt_test.name

def nooutput(*args):
    pass

def DocTestSuite(module=None, globs=None, extraglobs=None,
                 optionflags=0, test_finder=None,
                 setUp=lambda: None, tearDown=lambda: None,
                 checker=None):
    """
    Convert doctest tests for a mudule to a unittest test suite.

    This converts each documentation string in a module that
    contains doctest tests to a unittest test case.  If any of the
    tests in a doc string fail, then the test case fails.  An exception
    is raised showing the name of the file containing the test and a
    (sometimes approximate) line number.

    The `module` argument provides the module to be tested.  The argument
    can be either a module or a module name.

    If no argument is given, the calling module is used.
    """

    if test_finder is None:
        test_finder = DocTestFinder()

    module = _normalize_module(module)
    tests = test_finder.find(module, globs=globs, extraglobs=extraglobs)
    if globs is None:
        globs = module.__dict__
    if not tests: # [XX] why do we want to do this?
        raise ValueError(module, "has no tests")

    tests.sort()
    suite = unittest.TestSuite()
    for test in tests:
        if len(test.examples) == 0:
            continue
        if not test.filename:
            filename = module.__file__
            if filename.endswith(".pyc"):
                filename = filename[:-1]
            elif filename.endswith(".pyo"):
                filename = filename[:-1]
            test.filename = filename
        suite.addTest(DocTestCase(test, optionflags, setUp, tearDown,
                                  checker))

    return suite

class DocFileCase(DocTestCase):

    def id(self):
        return '_'.join(self._dt_test.name.split('.'))

    def __repr__(self):
        return self._dt_test.filename
    __str__ = __repr__

    def format_failure(self, err):
        return ('Failed doctest test for %s\n  File "%s", line 0\n\n%s'
                % (self._dt_test.name, self._dt_test.filename, err)
                )

def DocFileTest(path, package=None, globs=None,
                setUp=None, tearDown=None,
                optionflags=0):
    package = _normalize_module(package)
    name = path.split('/')[-1]
    dir = os.path.split(package.__file__)[0]
    path = os.path.join(dir, *(path.split('/')))
    doc = open(path).read()

    if globs is None:
        globs = {}

    test = DocTest(doc, globs, name, path, 0)

    return DocFileCase(test, optionflags, setUp, tearDown)

def DocFileSuite(*paths, **kw):
    """Creates a suite of doctest files.

    One or more text file paths are given as strings.  These should
    use "/" characters to separate path segments.  Paths are relative
    to the directory of the calling module, or relative to the package
    passed as a keyword argument.

    A number of options may be provided as keyword arguments:

    package
      The name of a Python package.  Text-file paths will be
      interpreted relative to the directory containing this package.
      The package may be supplied as a package object or as a dotted
      package name.

    setUp
      The name of a set-up function.  This is called before running the
      tests in each file.

    tearDown
      The name of a tear-down function.  This is called after running the
      tests in each file.

    globs
      A dictionary containing initial global variables for the tests.
    """
    suite = unittest.TestSuite()

    # We do this here so that _normalize_module is called at the right
    # level.  If it were called in DocFileTest, then this function
    # would be the caller and we might guess the package incorrectly.
    kw['package'] = _normalize_module(kw.get('package'))

    for path in paths:
        suite.addTest(DocFileTest(path, **kw))

    return suite

######################################################################
## 9. Debugging Support
######################################################################

def script_from_examples(s):
    r"""Extract script from text with examples.

       Converts text with examples to a Python script.  Example input is
       converted to regular code.  Example output and all other words
       are converted to comments:

       >>> text = '''
       ...       Here are examples of simple math.
       ...
       ...           Python has super accurate integer addition
       ...
       ...           >>> 2 + 2
       ...           5
       ...
       ...           And very friendly error messages:
       ...
       ...           >>> 1/0
       ...           To Infinity
       ...           And
       ...           Beyond
       ...
       ...           You can use logic if you want:
       ...
       ...           >>> if 0:
       ...           ...    blah
       ...           ...    blah
       ...           ...
       ...
       ...           Ho hum
       ...           '''

       >>> print script_from_examples(text)
       #        Here are examples of simple math.
       #
       #            Python has super accurate integer addition
       #
       2 + 2
       # Expected:
       #     5
       #
       #            And very friendly error messages:
       #
       1/0
       # Expected:
       #     To Infinity
       #     And
       #     Beyond
       #
       #            You can use logic if you want:
       #
       if 0:
          blah
          blah
       <BLANKLINE>
       #
       #            Ho hum
       """

    return Parser('<string>', s).get_program()

def _want_comment(example):
    """
    Return a comment containing the expected output for the given example.
    """
    # Return the expected output, if any
    want = example.want
    if want:
        if want[-1] == '\n':
            want = want[:-1]
        want = "\n#     ".join(want.split("\n"))
        want = "\n# Expected:\n#     %s" % want
    return want

def testsource(module, name):
    """Extract the test sources from a doctest docstring as a script.

    Provide the module (or dotted name of the module) containing the
    test to be debugged and the name (within the module) of the object
    with the doc string with tests to be debugged.
    """
    module = _normalize_module(module)
    tests = DocTestFinder().find(module)
    test = [t for t in tests if t.name == name]
    if not test:
        raise ValueError(name, "not found in tests")
    test = test[0]
    testsrc = script_from_examples(test.docstring)
    return testsrc

def debug_src(src, pm=False, globs=None):
    """Debug a single doctest docstring, in argument `src`'"""
    testsrc = script_from_examples(src)
    debug_script(testsrc, pm, globs)

def debug_script(src, pm=False, globs=None):
    "Debug a test script.  `src` is the script, as a string."
    import pdb

    srcfilename = tempfile.mktemp("doctestdebug.py")
    f = open(srcfilename, 'w')
    f.write(src)
    f.close()

    if globs:
        globs = globs.copy()
    else:
        globs = {}

    if pm:
        try:
            execfile(srcfilename, globs, globs)
        except:
            print sys.exc_info()[1]
            pdb.post_mortem(sys.exc_info()[2])
    else:
        # Note that %r is vital here.  '%s' instead can, e.g., cause
        # backslashes to get treated as metacharacters on Windows.
        pdb.run("execfile(%r)" % srcfilename, globs, globs)

def debug(module, name, pm=False):
    """Debug a single doctest docstring.

    Provide the module (or dotted name of the module) containing the
    test to be debugged and the name (within the module) of the object
    with the docstring with tests to be debugged.
    """
    module = _normalize_module(module)
    testsrc = testsource(module, name)
    debug_script(testsrc, pm, module.__dict__)

######################################################################
## 10. Example Usage
######################################################################
class _TestClass:
    """
    A pointless class, for sanity-checking of docstring testing.

    Methods:
        square()
        get()

    >>> _TestClass(13).get() + _TestClass(-12).get()
    1
    >>> hex(_TestClass(13).square().get())
    '0xa9'
    """

    def __init__(self, val):
        """val -> _TestClass object with associated value val.

        >>> t = _TestClass(123)
        >>> print t.get()
        123
        """

        self.val = val

    def square(self):
        """square() -> square TestClass's associated value

        >>> _TestClass(13).square().get()
        169
        """

        self.val = self.val ** 2
        return self

    def get(self):
        """get() -> return TestClass's associated value.

        >>> x = _TestClass(-42)
        >>> print x.get()
        -42
        """

        return self.val

__test__ = {"_TestClass": _TestClass,
            "string": r"""
                      Example of a string object, searched as-is.
                      >>> x = 1; y = 2
                      >>> x + y, x * y
                      (3, 2)
                      """,
            "bool-int equivalence": r"""
                                    In 2.2, boolean expressions displayed
                                    0 or 1.  By default, we still accept
                                    them.  This can be disabled by passing
                                    DONT_ACCEPT_TRUE_FOR_1 to the new
                                    optionflags argument.
                                    >>> 4 == 4
                                    1
                                    >>> 4 == 4
                                    True
                                    >>> 4 > 4
                                    0
                                    >>> 4 > 4
                                    False
                                    """,
            "blank lines": r"""
            Blank lines can be marked with <BLANKLINE>:
                >>> print 'foo\n\nbar\n'
                foo
                <BLANKLINE>
                bar
                <BLANKLINE>
            """,
            }
#             "ellipsis": r"""
#             If the ellipsis flag is used, then '...' can be used to
#             elide substrings in the desired output:
#                 >>> print range(1000)
#                 [0, 1, 2, ..., 999]
#             """,
#             "whitespace normalization": r"""
#             If the whitespace normalization flag is used, then
#             differences in whitespace are ignored.
#                 >>> print range(30)
#                 [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
#                  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
#                  27, 28, 29]
#             """,
#            }

def test1(): r"""
>>> warnings.filterwarnings("ignore", "class Tester", DeprecationWarning,
...                         "doctest", 0)
>>> from doctest import Tester
>>> t = Tester(globs={'x': 42}, verbose=0)
>>> t.runstring(r'''
...      >>> x = x * 2
...      >>> print x
...      42
... ''', 'XYZ')
**********************************************************************
Failure in example: print x
from line #2 of XYZ
Expected: 42
Got: 84
(1, 2)
>>> t.runstring(">>> x = x * 2\n>>> print x\n84\n", 'example2')
(0, 2)
>>> t.summarize()
**********************************************************************
1 items had failures:
   1 of   2 in XYZ
***Test Failed*** 1 failures.
(1, 4)
>>> t.summarize(verbose=1)
1 items passed all tests:
   2 tests in example2
**********************************************************************
1 items had failures:
   1 of   2 in XYZ
4 tests in 2 items.
3 passed and 1 failed.
***Test Failed*** 1 failures.
(1, 4)
"""

def test2(): r"""
        >>> warnings.filterwarnings("ignore", "class Tester",
        ...                         DeprecationWarning, "doctest", 0)
        >>> t = Tester(globs={}, verbose=1)
        >>> test = r'''
        ...    # just an example
        ...    >>> x = 1 + 2
        ...    >>> x
        ...    3
        ... '''
        >>> t.runstring(test, "Example")
        Running string Example
        Trying: x = 1 + 2
        Expecting: nothing
        ok
        Trying: x
        Expecting: 3
        ok
        0 of 2 examples failed in string Example
        (0, 2)
"""
def test3(): r"""
        >>> warnings.filterwarnings("ignore", "class Tester",
        ...                         DeprecationWarning, "doctest", 0)
        >>> t = Tester(globs={}, verbose=0)
        >>> def _f():
        ...     '''Trivial docstring example.
        ...     >>> assert 2 == 2
        ...     '''
        ...     return 32
        ...
        >>> t.rundoc(_f)  # expect 0 failures in 1 example
        (0, 1)
"""
def test4(): """
        >>> import new
        >>> m1 = new.module('_m1')
        >>> m2 = new.module('_m2')
        >>> test_data = \"""
        ... def _f():
        ...     '''>>> assert 1 == 1
        ...     '''
        ... def g():
        ...    '''>>> assert 2 != 1
        ...    '''
        ... class H:
        ...    '''>>> assert 2 > 1
        ...    '''
        ...    def bar(self):
        ...        '''>>> assert 1 < 2
        ...        '''
        ... \"""
        >>> exec test_data in m1.__dict__
        >>> exec test_data in m2.__dict__
        >>> m1.__dict__.update({"f2": m2._f, "g2": m2.g, "h2": m2.H})

        Tests that objects outside m1 are excluded:

        >>> warnings.filterwarnings("ignore", "class Tester",
        ...                         DeprecationWarning, "doctest", 0)
        >>> t = Tester(globs={}, verbose=0)
        >>> t.rundict(m1.__dict__, "rundict_test", m1)  # f2 and g2 and h2 skipped
        (0, 4)

        Once more, not excluding stuff outside m1:

        >>> t = Tester(globs={}, verbose=0)
        >>> t.rundict(m1.__dict__, "rundict_test_pvt")  # None are skipped.
        (0, 8)

        The exclusion of objects from outside the designated module is
        meant to be invoked automagically by testmod.

        >>> testmod(m1, verbose=False)
        (0, 4)
"""

def _test():
    #import doctest
    #doctest.testmod(doctest, verbose=False,
    #                optionflags=ELLIPSIS | NORMALIZE_WHITESPACE |
    #                UNIFIED_DIFF)
    #print '~'*70
    r = unittest.TextTestRunner()
    r.run(DocTestSuite())

if __name__ == "__main__":
    _test()
