# test_getopt.py
# David Goodger <dgoodger@bigfoot.com> 2000-08-19

import getopt
from getopt import GetoptError
from test.test_support import verify, verbose, run_doctest
import os

def expectException(teststr, expected, failure=AssertionError):
    """Executes a statement passed in teststr, and raises an exception
       (failure) if the expected exception is *not* raised."""
    try:
        exec teststr
    except expected:
        pass
    else:
        raise failure

old_posixly_correct = os.environ.get("POSIXLY_CORRECT")
if old_posixly_correct is not None:
    del os.environ["POSIXLY_CORRECT"]

if verbose:
    print 'Running tests on getopt.short_has_arg'
verify(getopt.short_has_arg('a', 'a:'))
verify(not getopt.short_has_arg('a', 'a'))
expectException("tmp = getopt.short_has_arg('a', 'b')", GetoptError)
expectException("tmp = getopt.short_has_arg('a', '')", GetoptError)

if verbose:
    print 'Running tests on getopt.long_has_args'
has_arg, option = getopt.long_has_args('abc', ['abc='])
verify(has_arg)
verify(option == 'abc')
has_arg, option = getopt.long_has_args('abc', ['abc'])
verify(not has_arg)
verify(option == 'abc')
has_arg, option = getopt.long_has_args('abc', ['abcd'])
verify(not has_arg)
verify(option == 'abcd')
expectException("has_arg, option = getopt.long_has_args('abc', ['def'])",
                GetoptError)
expectException("has_arg, option = getopt.long_has_args('abc', [])",
                GetoptError)
expectException("has_arg, option = " + \
                     "getopt.long_has_args('abc', ['abcd','abcde'])",
                GetoptError)

if verbose:
    print 'Running tests on getopt.do_shorts'
opts, args = getopt.do_shorts([], 'a', 'a', [])
verify(opts == [('-a', '')])
verify(args == [])
opts, args = getopt.do_shorts([], 'a1', 'a:', [])
verify(opts == [('-a', '1')])
verify(args == [])
#opts, args = getopt.do_shorts([], 'a=1', 'a:', [])
#verify(opts == [('-a', '1')])
#verify(args == [])
opts, args = getopt.do_shorts([], 'a', 'a:', ['1'])
verify(opts == [('-a', '1')])
verify(args == [])
opts, args = getopt.do_shorts([], 'a', 'a:', ['1', '2'])
verify(opts == [('-a', '1')])
verify(args == ['2'])
expectException("opts, args = getopt.do_shorts([], 'a1', 'a', [])",
                GetoptError)
expectException("opts, args = getopt.do_shorts([], 'a', 'a:', [])",
                GetoptError)

if verbose:
    print 'Running tests on getopt.do_longs'
opts, args = getopt.do_longs([], 'abc', ['abc'], [])
verify(opts == [('--abc', '')])
verify(args == [])
opts, args = getopt.do_longs([], 'abc=1', ['abc='], [])
verify(opts == [('--abc', '1')])
verify(args == [])
opts, args = getopt.do_longs([], 'abc=1', ['abcd='], [])
verify(opts == [('--abcd', '1')])
verify(args == [])
opts, args = getopt.do_longs([], 'abc', ['ab', 'abc', 'abcd'], [])
verify(opts == [('--abc', '')])
verify(args == [])
# Much like the preceding, except with a non-alpha character ("-") in
# option name that precedes "="; failed in
# http://sourceforge.net/bugs/?func=detailbug&bug_id=126863&group_id=5470
opts, args = getopt.do_longs([], 'foo=42', ['foo-bar', 'foo=',], [])
verify(opts == [('--foo', '42')])
verify(args == [])
expectException("opts, args = getopt.do_longs([], 'abc=1', ['abc'], [])",
                GetoptError)
expectException("opts, args = getopt.do_longs([], 'abc', ['abc='], [])",
                GetoptError)

# note: the empty string between '-a' and '--beta' is significant:
# it simulates an empty string option argument ('-a ""') on the command line.
cmdline = ['-a', '1', '-b', '--alpha=2', '--beta', '-a', '3', '-a', '',
           '--beta', 'arg1', 'arg2']

if verbose:
    print 'Running tests on getopt.getopt'
opts, args = getopt.getopt(cmdline, 'a:b', ['alpha=', 'beta'])
verify(opts == [('-a', '1'), ('-b', ''), ('--alpha', '2'), ('--beta', ''),
                ('-a', '3'), ('-a', ''), ('--beta', '')] )
# Note ambiguity of ('-b', '') and ('-a', '') above. This must be
# accounted for in the code that calls getopt().
verify(args == ['arg1', 'arg2'])

expectException(
    "opts, args = getopt.getopt(cmdline, 'a:b', ['alpha', 'beta'])",
    GetoptError)

# Test handling of GNU style scanning mode.
if verbose:
    print 'Running tests on getopt.gnu_getopt'
cmdline = ['-a', 'arg1', '-b', '1', '--alpha', '--beta=2']
# GNU style
opts, args = getopt.gnu_getopt(cmdline, 'ab:', ['alpha', 'beta='])
verify(opts == [('-a', ''), ('-b', '1'), ('--alpha', ''), ('--beta', '2')])
verify(args == ['arg1'])
# Posix style via +
opts, args = getopt.gnu_getopt(cmdline, '+ab:', ['alpha', 'beta='])
verify(opts == [('-a', '')])
verify(args == ['arg1', '-b', '1', '--alpha', '--beta=2'])
# Posix style via POSIXLY_CORRECT
os.environ["POSIXLY_CORRECT"] = "1"
opts, args = getopt.gnu_getopt(cmdline, 'ab:', ['alpha', 'beta='])
verify(opts == [('-a', '')])
verify(args == ['arg1', '-b', '1', '--alpha', '--beta=2'])


if old_posixly_correct is None:
    del os.environ["POSIXLY_CORRECT"]
else:
    os.environ["POSIXLY_CORRECT"] = old_posixly_correct

#------------------------------------------------------------------------------

libreftest = """
Examples from the Library Reference:  Doc/lib/libgetopt.tex

An example using only Unix style options:


>>> import getopt
>>> args = '-a -b -cfoo -d bar a1 a2'.split()
>>> args
['-a', '-b', '-cfoo', '-d', 'bar', 'a1', 'a2']
>>> optlist, args = getopt.getopt(args, 'abc:d:')
>>> optlist
[('-a', ''), ('-b', ''), ('-c', 'foo'), ('-d', 'bar')]
>>> args
['a1', 'a2']

Using long option names is equally easy:


>>> s = '--condition=foo --testing --output-file abc.def -x a1 a2'
>>> args = s.split()
>>> args
['--condition=foo', '--testing', '--output-file', 'abc.def', '-x', 'a1', 'a2']
>>> optlist, args = getopt.getopt(args, 'x', [
...     'condition=', 'output-file=', 'testing'])
>>> optlist
[('--condition', 'foo'), ('--testing', ''), ('--output-file', 'abc.def'), ('-x', '')]
>>> args
['a1', 'a2']

"""

__test__ = {'libreftest' : libreftest}

import sys
run_doctest(sys.modules[__name__], verbose)

#------------------------------------------------------------------------------

if verbose:
    print "Module getopt: tests completed successfully."
