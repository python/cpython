# Module doctest.
# Released to the public domain 16-Jan-2001,
# by Tim Peters (tim.one@home.com).

# Provided as-is; use at your own risk; no warranty; no promises; enjoy!

"""Module doctest -- a framework for running examples in docstrings.

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

+ If you continue a line via backslashing in an interactive session, or for
  any other reason use a backslash, you need to double the backslash in the
  docstring version.  This is simply because you're in a string, and so the
  backslash must be escaped for it to survive intact.  Like:

>>> if "yes" == \\
...     "y" +   \\
...     "es":   # in the source code you'll see the doubled backslashes
...     print 'yes'
yes

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
    'testmod',
    'run_docstring_examples',
    'is_private',
    'Tester',
    'DocTestTestFailure',
    'DocTestSuite',
    'testsource',
    'debug',
    'master',
]

import __future__

import re
PS1 = ">>>"
PS2 = "..."
_isPS1 = re.compile(r"(\s*)" + re.escape(PS1)).match
_isPS2 = re.compile(r"(\s*)" + re.escape(PS2)).match
_isEmpty = re.compile(r"\s*$").match
_isComment = re.compile(r"\s*#").match
del re

from types import StringTypes as _StringTypes

from inspect import isclass    as _isclass
from inspect import isfunction as _isfunction
from inspect import ismethod as _ismethod
from inspect import ismodule   as _ismodule
from inspect import classify_class_attrs as _classify_class_attrs

# Option constants.
DONT_ACCEPT_TRUE_FOR_1 = 1 << 0

# Extract interactive examples from a string.  Return a list of triples,
# (source, outcome, lineno).  "source" is the source code, and ends
# with a newline iff the source spans more than one line.  "outcome" is
# the expected output if any, else an empty string.  When not empty,
# outcome always ends with a newline.  "lineno" is the line number,
# 0-based wrt the start of the string, of the first source line.

def _extract_examples(s):
    isPS1, isPS2 = _isPS1, _isPS2
    isEmpty, isComment = _isEmpty, _isComment
    examples = []
    lines = s.split("\n")
    i, n = 0, len(lines)
    while i < n:
        line = lines[i]
        i = i + 1
        m = isPS1(line)
        if m is None:
            continue
        j = m.end(0)  # beyond the prompt
        if isEmpty(line, j) or isComment(line, j):
            # a bare prompt or comment -- not interesting
            continue
        lineno = i - 1
        if line[j] != " ":
            raise ValueError("line " + `lineno` + " of docstring lacks "
                "blank after " + PS1 + ": " + line)
        j = j + 1
        blanks = m.group(1)
        nblanks = len(blanks)
        # suck up this and following PS2 lines
        source = []
        while 1:
            source.append(line[j:])
            line = lines[i]
            m = isPS2(line)
            if m:
                if m.group(1) != blanks:
                    raise ValueError("inconsistent leading whitespace "
                        "in line " + `i` + " of docstring: " + line)
                i = i + 1
            else:
                break
        if len(source) == 1:
            source = source[0]
        else:
            # get rid of useless null line from trailing empty "..."
            if source[-1] == "":
                del source[-1]
            source = "\n".join(source) + "\n"
        # suck up response
        if isPS1(line) or isEmpty(line):
            expect = ""
        else:
            expect = []
            while 1:
                if line[:nblanks] != blanks:
                    raise ValueError("inconsistent leading whitespace "
                        "in line " + `i` + " of docstring: " + line)
                expect.append(line[nblanks:])
                i = i + 1
                line = lines[i]
                if isPS1(line) or isEmpty(line):
                    break
            expect = "\n".join(expect) + "\n"
        examples.append( (source, expect, lineno) )
    return examples

# Capture stdout when running examples.

class _SpoofOut:
    def __init__(self):
        self.clear()
    def write(self, s):
        self.buf.append(s)
    def get(self):
        guts = "".join(self.buf)
        # If anything at all was written, make sure there's a trailing
        # newline.  There's no way for the expected output to indicate
        # that a trailing newline is missing.
        if guts and not guts.endswith("\n"):
            guts = guts + "\n"
        # Prevent softspace from screwing up the next test case, in
        # case they used print with a trailing comma in an example.
        if hasattr(self, "softspace"):
            del self.softspace
        return guts
    def clear(self):
        self.buf = []
        if hasattr(self, "softspace"):
            del self.softspace
    def flush(self):
        # JPython calls flush
        pass

# Display some tag-and-msg pairs nicely, keeping the tag and its msg
# on the same line when that makes sense.

def _tag_out(printer, *tag_msg_pairs):
    for tag, msg in tag_msg_pairs:
        printer(tag + ":")
        msg_has_nl = msg[-1:] == "\n"
        msg_has_two_nl = msg_has_nl and \
                        msg.find("\n") < len(msg) - 1
        if len(tag) + len(msg) < 76 and not msg_has_two_nl:
            printer(" ")
        else:
            printer("\n")
        printer(msg)
        if not msg_has_nl:
            printer("\n")

# Run list of examples, in context globs.  "out" can be used to display
# stuff to "the real" stdout, and fakeout is an instance of _SpoofOut
# that captures the examples' std output.  Return (#failures, #tries).

def _run_examples_inner(out, fakeout, examples, globs, verbose, name,
                        compileflags, optionflags):
    import sys, traceback
    OK, BOOM, FAIL = range(3)
    NADA = "nothing"
    stderr = _SpoofOut()
    failures = 0
    for source, want, lineno in examples:
        if verbose:
            _tag_out(out, ("Trying", source),
                          ("Expecting", want or NADA))
        fakeout.clear()
        try:
            exec compile(source, "<string>", "single",
                         compileflags, 1) in globs
            got = fakeout.get()
            state = OK
        except KeyboardInterrupt:
            raise
        except:
            # See whether the exception was expected.
            if want.find("Traceback (innermost last):\n") == 0 or \
               want.find("Traceback (most recent call last):\n") == 0:
                # Only compare exception type and value - the rest of
                # the traceback isn't necessary.
                want = want.split('\n')[-2] + '\n'
                exc_type, exc_val = sys.exc_info()[:2]
                got = traceback.format_exception_only(exc_type, exc_val)[-1]
                state = OK
            else:
                # unexpected exception
                stderr.clear()
                traceback.print_exc(file=stderr)
                state = BOOM

        if state == OK:
            if (got == want or
                (not (optionflags & DONT_ACCEPT_TRUE_FOR_1) and
                 (got, want) in (("True\n", "1\n"), ("False\n", "0\n"))
                )
               ):
                if verbose:
                    out("ok\n")
                continue
            state = FAIL

        assert state in (FAIL, BOOM)
        failures = failures + 1
        out("*" * 65 + "\n")
        _tag_out(out, ("Failure in example", source))
        out("from line #" + `lineno` + " of " + name + "\n")
        if state == FAIL:
            _tag_out(out, ("Expected", want or NADA), ("Got", got))
        else:
            assert state == BOOM
            _tag_out(out, ("Exception raised", stderr.get()))

    return failures, len(examples)

# Get the future-flags associated with the future features that have been
# imported into globs.

def _extract_future_flags(globs):
    flags = 0
    for fname in __future__.all_feature_names:
        feature = globs.get(fname, None)
        if feature is getattr(__future__, fname):
            flags |= feature.compiler_flag
    return flags

# Run list of examples, in a shallow copy of context (dict) globs.
# Return (#failures, #tries).

def _run_examples(examples, globs, verbose, name, compileflags,
                  optionflags):
    import sys
    saveout = sys.stdout
    globs = globs.copy()
    try:
        sys.stdout = fakeout = _SpoofOut()
        x = _run_examples_inner(saveout.write, fakeout, examples,
                                globs, verbose, name, compileflags,
                                optionflags)
    finally:
        sys.stdout = saveout
        # While Python gc can clean up most cycles on its own, it doesn't
        # chase frame objects.  This is especially irksome when running
        # generator tests that raise exceptions, because a named generator-
        # iterator gets an entry in globs, and the generator-iterator
        # object's frame's traceback info points back to globs.  This is
        # easy to break just by clearing the namespace.  This can also
        # help to break other kinds of cycles, and even for cycles that
        # gc can break itself it's better to break them ASAP.
        globs.clear()
    return x

def run_docstring_examples(f, globs, verbose=0, name="NoName",
                           compileflags=None, optionflags=0):
    """f, globs, verbose=0, name="NoName" -> run examples from f.__doc__.

    Use (a shallow copy of) dict globs as the globals for execution.
    Return (#failures, #tries).

    If optional arg verbose is true, print stuff even if there are no
    failures.
    Use string name in failure msgs.
    """

    try:
        doc = f.__doc__
        if not doc:
            # docstring empty or None
            return 0, 0
        # just in case CT invents a doc object that has to be forced
        # to look like a string <0.9 wink>
        doc = str(doc)
    except KeyboardInterrupt:
        raise
    except:
        return 0, 0

    e = _extract_examples(doc)
    if not e:
        return 0, 0
    if compileflags is None:
        compileflags = _extract_future_flags(globs)
    return _run_examples(e, globs, verbose, name, compileflags, optionflags)

def is_private(prefix, base):
    """prefix, base -> true iff name prefix + "." + base is "private".

    Prefix may be an empty string, and base does not contain a period.
    Prefix is ignored (although functions you write conforming to this
    protocol may make use of it).
    Return true iff base begins with an (at least one) underscore, but
    does not both begin and end with (at least) two underscores.

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

    return base[:1] == "_" and not base[:2] == "__" == base[-2:]

# Determine if a class of function was defined in the given module.

def _from_module(module, object):
    if _isfunction(object):
        return module.__dict__ is object.func_globals
    if _isclass(object):
        return module.__name__ == object.__module__
    raise ValueError("object must be a class or function")

class Tester:
    """Class Tester -- runs docstring examples and accumulates stats.

In normal use, function doctest.testmod() hides all this from you,
so use that if you can.  Create your own instances of Tester to do
fancier things.

Methods:
    runstring(s, name)
        Search string s for examples to run; use name for logging.
        Return (#failures, #tries).

    rundoc(object, name=None)
        Search object.__doc__ for examples to run; use name (or
        object.__name__) for logging.  Return (#failures, #tries).

    rundict(d, name, module=None)
        Search for examples in docstrings in all of d.values(); use name
        for logging.  Exclude functions and classes not defined in module
        if specified.  Return (#failures, #tries).

    run__test__(d, name)
        Treat dict d like module.__test__.  Return (#failures, #tries).

    summarize(verbose=None)
        Display summary of testing results, to stdout.  Return
        (#failures, #tries).

    merge(other)
        Merge in the test results from Tester instance "other".

>>> from doctest import Tester
>>> t = Tester(globs={'x': 42}, verbose=0)
>>> t.runstring(r'''
...      >>> x = x * 2
...      >>> print x
...      42
... ''', 'XYZ')
*****************************************************************
Failure in example: print x
from line #2 of XYZ
Expected: 42
Got: 84
(1, 2)
>>> t.runstring(">>> x = x * 2\\n>>> print x\\n84\\n", 'example2')
(0, 2)
>>> t.summarize()
*****************************************************************
1 items had failures:
   1 of   2 in XYZ
***Test Failed*** 1 failures.
(1, 4)
>>> t.summarize(verbose=1)
1 items passed all tests:
   2 tests in example2
*****************************************************************
1 items had failures:
   1 of   2 in XYZ
4 tests in 2 items.
3 passed and 1 failed.
***Test Failed*** 1 failures.
(1, 4)
>>>
"""

    def __init__(self, mod=None, globs=None, verbose=None,
                 isprivate=None, optionflags=0):
        """mod=None, globs=None, verbose=None, isprivate=None,
optionflags=0

See doctest.__doc__ for an overview.

Optional keyword arg "mod" is a module, whose globals are used for
executing examples.  If not specified, globs must be specified.

Optional keyword arg "globs" gives a dict to be used as the globals
when executing examples; if not specified, use the globals from
module mod.

In either case, a copy of the dict is used for each docstring
examined.

Optional keyword arg "verbose" prints lots of stuff if true, only
failures if false; by default, it's true iff "-v" is in sys.argv.

Optional keyword arg "isprivate" specifies a function used to determine
whether a name is private.  The default function is to assume that
no functions are private.  The "isprivate" arg may be set to
doctest.is_private in order to skip over functions marked as private
using an underscore naming convention; see its docs for details.

See doctest.testmod docs for the meaning of optionflags.
"""

        if mod is None and globs is None:
            raise TypeError("Tester.__init__: must specify mod or globs")
        if mod is not None and not _ismodule(mod):
            raise TypeError("Tester.__init__: mod must be a module; " +
                            `mod`)
        if globs is None:
            globs = mod.__dict__
        self.globs = globs

        if verbose is None:
            import sys
            verbose = "-v" in sys.argv
        self.verbose = verbose

        # By default, assume that nothing is private
        if isprivate is None:
            isprivate = lambda prefix, base:  0
        self.isprivate = isprivate

        self.optionflags = optionflags

        self.name2ft = {}   # map name to (#failures, #trials) pair

        self.compileflags = _extract_future_flags(globs)

    def runstring(self, s, name):
        """
        s, name -> search string s for examples to run, logging as name.

        Use string name as the key for logging the outcome.
        Return (#failures, #examples).

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

        if self.verbose:
            print "Running string", name
        f = t = 0
        e = _extract_examples(s)
        if e:
            f, t = _run_examples(e, self.globs, self.verbose, name,
                                 self.compileflags, self.optionflags)
        if self.verbose:
            print f, "of", t, "examples failed in string", name
        self.__record_outcome(name, f, t)
        return f, t

    def rundoc(self, object, name=None):
        """
        object, name=None -> search object.__doc__ for examples to run.

        Use optional string name as the key for logging the outcome;
        by default use object.__name__.
        Return (#failures, #examples).
        If object is a class object, search recursively for method
        docstrings too.
        object.__doc__ is examined regardless of name, but if object is
        a class, whether private names reached from object are searched
        depends on the constructor's "isprivate" argument.

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

        if name is None:
            try:
                name = object.__name__
            except AttributeError:
                raise ValueError("Tester.rundoc: name must be given "
                    "when object.__name__ doesn't exist; " + `object`)
        if self.verbose:
            print "Running", name + ".__doc__"
        f, t = run_docstring_examples(object, self.globs, self.verbose, name,
                                      self.compileflags, self.optionflags)
        if self.verbose:
            print f, "of", t, "examples failed in", name + ".__doc__"
        self.__record_outcome(name, f, t)
        if _isclass(object):
            # In 2.2, class and static methods complicate life.  Build
            # a dict "that works", by hook or by crook.
            d = {}
            for tag, kind, homecls, value in _classify_class_attrs(object):

                if homecls is not object:
                    # Only look at names defined immediately by the class.
                    continue

                elif self.isprivate(name, tag):
                    continue

                elif kind == "method":
                    # value is already a function
                    d[tag] = value

                elif kind == "static method":
                    # value isn't a function, but getattr reveals one
                    d[tag] = getattr(object, tag)

                elif kind == "class method":
                    # Hmm.  A classmethod object doesn't seem to reveal
                    # enough.  But getattr turns it into a bound method,
                    # and from there .im_func retrieves the underlying
                    # function.
                    d[tag] = getattr(object, tag).im_func

                elif kind == "property":
                    # The methods implementing the property have their
                    # own docstrings -- but the property may have one too.
                    if value.__doc__ is not None:
                        d[tag] = str(value.__doc__)

                elif kind == "data":
                    # Grab nested classes.
                    if _isclass(value):
                        d[tag] = value

                else:
                    raise ValueError("teach doctest about %r" % kind)

            f2, t2 = self.run__test__(d, name)
            f += f2
            t += t2

        return f, t

    def rundict(self, d, name, module=None):
        """
        d, name, module=None -> search for docstring examples in d.values().

        For k, v in d.items() such that v is a function or class,
        do self.rundoc(v, name + "." + k).  Whether this includes
        objects with private names depends on the constructor's
        "isprivate" argument.  If module is specified, functions and
        classes that are not defined in module are excluded.
        Return aggregate (#failures, #examples).

        Build and populate two modules with sample functions to test that
        exclusion of external functions and classes works.

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

        >>> t = Tester(globs={}, verbose=0, isprivate=is_private)
        >>> t.rundict(m1.__dict__, "rundict_test", m1)  # _f, f2 and g2 and h2 skipped
        (0, 3)

        Again, but with the default isprivate function allowing _f:

        >>> t = Tester(globs={}, verbose=0)
        >>> t.rundict(m1.__dict__, "rundict_test_pvt", m1)  # Only f2, g2 and h2 skipped
        (0, 4)

        And once more, not excluding stuff outside m1:

        >>> t = Tester(globs={}, verbose=0)
        >>> t.rundict(m1.__dict__, "rundict_test_pvt")  # None are skipped.
        (0, 8)

        The exclusion of objects from outside the designated module is
        meant to be invoked automagically by testmod.

        >>> testmod(m1, isprivate=is_private)
        (0, 3)

        """

        if not hasattr(d, "items"):
            raise TypeError("Tester.rundict: d must support .items(); " +
                            `d`)
        f = t = 0
        # Run the tests by alpha order of names, for consistency in
        # verbose-mode output.
        names = d.keys()
        names.sort()
        for thisname in names:
            value = d[thisname]
            if _isfunction(value) or _isclass(value):
                if module and not _from_module(module, value):
                    continue
                f2, t2 = self.__runone(value, name + "." + thisname)
                f = f + f2
                t = t + t2
        return f, t

    def run__test__(self, d, name):
        """d, name -> Treat dict d like module.__test__.

        Return (#failures, #tries).
        See testmod.__doc__ for details.
        """

        failures = tries = 0
        prefix = name + "."
        savepvt = self.isprivate
        try:
            self.isprivate = lambda *args: 0
            # Run the tests by alpha order of names, for consistency in
            # verbose-mode output.
            keys = d.keys()
            keys.sort()
            for k in keys:
                v = d[k]
                thisname = prefix + k
                if type(v) in _StringTypes:
                    f, t = self.runstring(v, thisname)
                elif _isfunction(v) or _isclass(v) or _ismethod(v):
                    f, t = self.rundoc(v, thisname)
                else:
                    raise TypeError("Tester.run__test__: values in "
                            "dict must be strings, functions, methods, "
                            "or classes; " + `v`)
                failures = failures + f
                tries = tries + t
        finally:
            self.isprivate = savepvt
        return failures, tries

    def summarize(self, verbose=None):
        """
        verbose=None -> summarize results, return (#failures, #tests).

        Print summary of test results to stdout.
        Optional arg 'verbose' controls how wordy this is.  By
        default, use the verbose setting established by the
        constructor.
        """

        if verbose is None:
            verbose = self.verbose
        notests = []
        passed = []
        failed = []
        totalt = totalf = 0
        for x in self.name2ft.items():
            name, (f, t) = x
            assert f <= t
            totalt = totalt + t
            totalf = totalf + f
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
            print "*" * 65
            print len(failed), "items had failures:"
            failed.sort()
            for thing, (f, t) in failed:
                print " %3d of %3d in %s" % (f, t, thing)
        if verbose:
            print totalt, "tests in", len(self.name2ft), "items."
            print totalt - totalf, "passed and", totalf, "failed."
        if totalf:
            print "***Test Failed***", totalf, "failures."
        elif verbose:
            print "Test passed."
        return totalf, totalt

    def merge(self, other):
        """
        other -> merge in test results from the other Tester instance.

        If self and other both have a test result for something
        with the same name, the (#failures, #tests) results are
        summed, and a warning is printed to stdout.

        >>> from doctest import Tester
        >>> t1 = Tester(globs={}, verbose=0)
        >>> t1.runstring('''
        ... >>> x = 12
        ... >>> print x
        ... 12
        ... ''', "t1example")
        (0, 2)
        >>>
        >>> t2 = Tester(globs={}, verbose=0)
        >>> t2.runstring('''
        ... >>> x = 13
        ... >>> print x
        ... 13
        ... ''', "t2example")
        (0, 2)
        >>> common = ">>> assert 1 + 2 == 3\\n"
        >>> t1.runstring(common, "common")
        (0, 1)
        >>> t2.runstring(common, "common")
        (0, 1)
        >>> t1.merge(t2)
        *** Tester.merge: 'common' in both testers; summing outcomes.
        >>> t1.summarize(1)
        3 items passed all tests:
           2 tests in common
           2 tests in t1example
           2 tests in t2example
        6 tests in 3 items.
        6 passed and 0 failed.
        Test passed.
        (0, 6)
        >>>
        """

        d = self.name2ft
        for name, (f, t) in other.name2ft.items():
            if name in d:
                print "*** Tester.merge: '" + name + "' in both" \
                    " testers; summing outcomes."
                f2, t2 = d[name]
                f = f + f2
                t = t + t2
            d[name] = f, t

    def __record_outcome(self, name, f, t):
        if name in self.name2ft:
            print "*** Warning: '" + name + "' was tested before;", \
                "summing outcomes."
            f2, t2 = self.name2ft[name]
            f = f + f2
            t = t + t2
        self.name2ft[name] = f, t

    def __runone(self, target, name):
        if "." in name:
            i = name.rindex(".")
            prefix, base = name[:i], name[i+1:]
        else:
            prefix, base = "", base
        if self.isprivate(prefix, base):
            return 0, 0
        return self.rundoc(target, name)

master = None

def testmod(m=None, name=None, globs=None, verbose=None, isprivate=None,
               report=True, optionflags=0):
    """m=None, name=None, globs=None, verbose=None, isprivate=None,
       report=True, optionflags=0

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

    Optional keyword arg "verbose" prints lots of stuff if true, prints
    only failures if false; by default, it's true iff "-v" is in sys.argv.

    Optional keyword arg "isprivate" specifies a function used to
    determine whether a name is private.  The default function is
    treat all functions as public.  Optionally, "isprivate" can be
    set to doctest.is_private to skip over functions marked as private
    using the underscore naming convention; see its docs for details.

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

    Advanced tomfoolery:  testmod runs methods of a local instance of
    class doctest.Tester, then merges the results into (or creates)
    global Tester instance doctest.master.  Methods of doctest.master
    can be called directly too, if you want to do something unusual.
    Passing report=0 to testmod is especially useful then, to delay
    displaying a summary.  Invoke doctest.master.summarize(verbose)
    when you're done fiddling.
    """

    global master

    if m is None:
        import sys
        # DWA - m will still be None if this wasn't invoked from the command
        # line, in which case the following TypeError is about as good an error
        # as we should expect
        m = sys.modules.get('__main__')

    if not _ismodule(m):
        raise TypeError("testmod: module required; " + `m`)
    if name is None:
        name = m.__name__
    tester = Tester(m, globs=globs, verbose=verbose, isprivate=isprivate,
                    optionflags=optionflags)
    failures, tries = tester.rundoc(m, name)
    f, t = tester.rundict(m.__dict__, name, m)
    failures += f
    tries += t
    if hasattr(m, "__test__"):
        testdict = m.__test__
        if testdict:
            if not hasattr(testdict, "items"):
                raise TypeError("testmod: module.__test__ must support "
                                ".items(); " + `testdict`)
            f, t = tester.run__test__(testdict, name + ".__test__")
            failures += f
            tries += t
    if report:
        tester.summarize()
    if master is None:
        master = tester
    else:
        master.merge(tester)
    return failures, tries

###########################################################################
# Various doctest extensions, to make using doctest with unittest
# easier, and to help debugging when a doctest goes wrong.  Original
# code by Jim Fulton.

# Utilities.

# If module is None, return the calling module (the module that called
# the routine that called _normalize_module -- this normally won't be
# doctest!).  If module is a string, it should be the (possibly dotted)
# name of a module, and the (rightmost) module object is returned.  Else
# module is returned untouched; the intent appears to be that module is
# already a module object in this case (although this isn't checked).

def _normalize_module(module):
    import sys

    if module is None:
        # Get our caller's caller's module.
        module = sys._getframe(2).f_globals['__name__']
        module = sys.modules[module]

    elif isinstance(module, (str, unicode)):
        # The ["*"] at the end is a mostly meaningless incantation with
        # a crucial property:  if, e.g., module is 'a.b.c', it convinces
        # __import__ to return c instead of a.
        module = __import__(module, globals(), locals(), ["*"])

    return module

# tests is a list of (testname, docstring, filename, lineno) tuples.
# If object has a __doc__ attr, and the __doc__ attr looks like it
# contains a doctest (specifically, if it contains an instance of '>>>'),
# then tuple
#     prefix + name, object.__doc__, filename, lineno
# is appended to tests.  Else tests is left alone.
# There is no return value.

def _get_doctest(name, object, tests, prefix, filename='', lineno=''):
    doc = getattr(object, '__doc__', '')
    if isinstance(doc, basestring) and '>>>' in doc:
        tests.append((prefix + name, doc, filename, lineno))

# tests is a list of (testname, docstring, filename, lineno) tuples.
# docstrings containing doctests are appended to tests (if any are found).
# items is a dict, like a module or class dict, mapping strings to objects.
# mdict is the global dict of a "home" module -- only objects belonging
# to this module are searched for docstrings.  module is the module to
# which mdict belongs.
# prefix is a string to be prepended to an object's name when adding a
# tuple to tests.
# The objects (values) in items are examined (recursively), and doctests
# belonging to functions and classes in the home module are appended to
# tests.
# minlineno is a gimmick to try to guess the file-relative line number
# at which a doctest probably begins.

def _extract_doctests(items, module, mdict, tests, prefix, minlineno=0):

    for name, object in items:
        # Only interested in named objects.
        if not hasattr(object, '__name__'):
            continue

        elif hasattr(object, 'func_globals'):
            # Looks like a function.
            if object.func_globals is not mdict:
                # Non-local function.
                continue
            code = getattr(object, 'func_code', None)
            filename = getattr(code, 'co_filename', '')
            lineno = getattr(code, 'co_firstlineno', -1) + 1
            if minlineno:
                minlineno = min(lineno, minlineno)
            else:
                minlineno = lineno
            _get_doctest(name, object, tests, prefix, filename, lineno)

        elif hasattr(object, "__module__"):
            # Maybe a class-like thing, in which case we care.
            if object.__module__ != module.__name__:
                # Not the same module.
                continue
            if not (hasattr(object, '__dict__')
                    and hasattr(object, '__bases__')):
                # Not a class.
                continue

            lineno = _extract_doctests(object.__dict__.items(),
                                       module,
                                       mdict,
                                       tests,
                                       prefix + name + ".")
            # XXX "-3" is unclear.
            _get_doctest(name, object, tests, prefix,
                         lineno="%s (or above)" % (lineno - 3))

    return minlineno

# Find all the doctests belonging to the module object.
# Return a list of
#     (testname, docstring, filename, lineno)
# tuples.

def _find_tests(module, prefix=None):
    if prefix is None:
        prefix = module.__name__
    mdict = module.__dict__
    tests = []
    # Get the module-level doctest (if any).
    _get_doctest(prefix, module, tests, '', lineno="1 (or above)")
    # Recursively search the module __dict__ for doctests.
    if prefix:
        prefix += "."
    _extract_doctests(mdict.items(), module, mdict, tests, prefix)
    return tests

# unittest helpers.

# A function passed to unittest, for unittest to drive.
# tester is doctest Tester instance.  doc is the docstring whose
# doctests are to be run.

def _utest(tester, name, doc, filename, lineno):
    import sys
    from StringIO import StringIO

    old = sys.stdout
    sys.stdout = new = StringIO()
    try:
        failures, tries = tester.runstring(doc, name)
    finally:
        sys.stdout = old

    if failures:
        msg = new.getvalue()
        lname = '.'.join(name.split('.')[-1:])
        if not lineno:
            lineno = "0 (don't know line number)"
        # Don't change this format!  It was designed so that Emacs can
        # parse it naturally.
        raise DocTestTestFailure('Failed doctest test for %s\n'
                                 '  File "%s", line %s, in %s\n\n%s' %
                                 (name, filename, lineno, lname, msg))

class DocTestTestFailure(Exception):
    """A doctest test failed"""

def DocTestSuite(module=None):
    """Convert doctest tests for a module to a unittest TestSuite.

    The returned TestSuite is to be run by the unittest framework, and
    runs each doctest in the module.  If any of the doctests fail,
    then the synthesized unit test fails, and an error is raised showing
    the name of the file containing the test and a (sometimes approximate)
    line number.

    The optional module argument provides the module to be tested.  It
    can be a module object or a (possibly dotted) module name.  If not
    specified, the module calling DocTestSuite() is used.

    Example (although note that unittest supplies many ways to use the
    TestSuite returned; see the unittest docs):

        import unittest
        import doctest
        import my_module_with_doctests

        suite = doctest.DocTestSuite(my_module_with_doctests)
        runner = unittest.TextTestRunner()
        runner.run(suite)
    """

    import unittest

    module = _normalize_module(module)
    tests = _find_tests(module)
    if not tests:
        raise ValueError(module, "has no tests")

    tests.sort()
    suite = unittest.TestSuite()
    tester = Tester(module)
    for name, doc, filename, lineno in tests:
        if not filename:
            filename = module.__file__
            if filename.endswith(".pyc"):
                filename = filename[:-1]
            elif filename.endswith(".pyo"):
                filename = filename[:-1]
        def runit(name=name, doc=doc, filename=filename, lineno=lineno):
            _utest(tester, name, doc, filename, lineno)
        suite.addTest(unittest.FunctionTestCase(
                                    runit,
                                    description="doctest of " + name))
    return suite

# Debugging support.

def _expect(expect):
    # Return the expected output (if any), formatted as a Python
    # comment block.
    if expect:
        expect = "\n# ".join(expect.split("\n"))
        expect = "\n# Expect:\n# %s" % expect
    return expect

def testsource(module, name):
    """Extract the doctest examples from a docstring.

    Provide the module (or dotted name of the module) containing the
    tests to be extracted, and the name (within the module) of the object
    with the docstring containing the tests to be extracted.

    The doctest examples are returned as a string containing Python
    code.  The expected output blocks in the examples are converted
    to Python comments.
    """

    module = _normalize_module(module)
    tests = _find_tests(module, "")
    test = [doc for (tname, doc, dummy, dummy) in tests
                if tname == name]
    if not test:
        raise ValueError(name, "not found in tests")
    test = test[0]
    examples = [source + _expect(expect)
                for source, expect, dummy in _extract_examples(test)]
    return '\n'.join(examples)

def debug(module, name):
    """Debug a single docstring containing doctests.

    Provide the module (or dotted name of the module) containing the
    docstring to be debugged, and the name (within the module) of the
    object with the docstring to be debugged.

    The doctest examples are extracted (see function testsource()),
    and written to a temp file.  The Python debugger (pdb) is then
    invoked on that file.
    """

    import os
    import pdb
    import tempfile

    module = _normalize_module(module)
    testsrc = testsource(module, name)
    srcfilename = tempfile.mktemp("doctestdebug.py")
    f = file(srcfilename, 'w')
    f.write(testsrc)
    f.close()

    globs = {}
    globs.update(module.__dict__)
    try:
        # Note that %r is vital here.  '%s' instead can, e.g., cause
        # backslashes to get treated as metacharacters on Windows.
        pdb.run("execfile(%r)" % srcfilename, globs, globs)
    finally:
        os.remove(srcfilename)



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
           }

def _test():
    import doctest
    return doctest.testmod(doctest)

if __name__ == "__main__":
    _test()
