# SRE test harness for the Python regression suite

# this is based on test_re.py, but uses a test function instead
# of all those asserts

import sys
sys.path=['.']+sys.path

from test_support import verbose, TestFailed, have_unicode
import sre
import sys, os, string, traceback

#
# test support

def test(expression, result, exception=None):
    try:
        r = eval(expression)
    except:
        if exception:
            if not isinstance(sys.exc_value, exception):
                print expression, "FAILED"
                # display name, not actual value
                if exception is sre.error:
                    print "expected", "sre.error"
                else:
                    print "expected", exception.__name__
                print "got", sys.exc_type.__name__, str(sys.exc_value)
        else:
            print expression, "FAILED"
            traceback.print_exc(file=sys.stdout)
    else:
        if exception:
            print expression, "FAILED"
            if exception is sre.error:
                print "expected", "sre.error"
            else:
                print "expected", exception.__name__
            print "got result", repr(r)
        else:
            if r != result:
                print expression, "FAILED"
                print "expected", repr(result)
                print "got result", repr(r)

if verbose:
    print 'Running tests on character literals'

for i in [0, 8, 16, 32, 64, 127, 128, 255]:
    test(r"""sre.match(r"\%03o" % i, chr(i)) is not None""", 1)
    test(r"""sre.match(r"\%03o0" % i, chr(i)+"0") is not None""", 1)
    test(r"""sre.match(r"\%03o8" % i, chr(i)+"8") is not None""", 1)
    test(r"""sre.match(r"\x%02x" % i, chr(i)) is not None""", 1)
    test(r"""sre.match(r"\x%02x0" % i, chr(i)+"0") is not None""", 1)
    test(r"""sre.match(r"\x%02xz" % i, chr(i)+"z") is not None""", 1)
test(r"""sre.match("\911", "")""", None, sre.error)

#
# Misc tests from Tim Peters' re.doc

if verbose:
    print 'Running tests on sre.search and sre.match'

test(r"""sre.search(r'x*', 'axx').span(0)""", (0, 0))
test(r"""sre.search(r'x*', 'axx').span()""", (0, 0))
test(r"""sre.search(r'x+', 'axx').span(0)""", (1, 3))
test(r"""sre.search(r'x+', 'axx').span()""", (1, 3))
test(r"""sre.search(r'x', 'aaa')""", None)

test(r"""sre.match(r'a*', 'xxx').span(0)""", (0, 0))
test(r"""sre.match(r'a*', 'xxx').span()""", (0, 0))
test(r"""sre.match(r'x*', 'xxxa').span(0)""", (0, 3))
test(r"""sre.match(r'x*', 'xxxa').span()""", (0, 3))
test(r"""sre.match(r'a+', 'xxx')""", None)

# bug 113254
test(r"""sre.match(r'(a)|(b)', 'b').start(1)""", -1)
test(r"""sre.match(r'(a)|(b)', 'b').end(1)""", -1)
test(r"""sre.match(r'(a)|(b)', 'b').span(1)""", (-1, -1))

# bug 612074
pat=u"["+sre.escape(u"\u2039")+u"]"
test(r"""sre.compile(pat) and 1""", 1, None)

if verbose:
    print 'Running tests on sre.sub'

test(r"""sre.sub(r"(?i)b+", "x", "bbbb BBBB")""", 'x x')

def bump_num(matchobj):
    int_value = int(matchobj.group(0))
    return str(int_value + 1)

test(r"""sre.sub(r'\d+', bump_num, '08.2 -2 23x99y')""", '9.3 -3 24x100y')
test(r"""sre.sub(r'\d+', bump_num, '08.2 -2 23x99y', 3)""", '9.3 -3 23x99y')

test(r"""sre.sub(r'.', lambda m: r"\n", 'x')""", '\\n')
test(r"""sre.sub(r'.', r"\n", 'x')""", '\n')

s = r"\1\1"

test(r"""sre.sub(r'(.)', s, 'x')""", 'xx')
test(r"""sre.sub(r'(.)', sre.escape(s), 'x')""", s)
test(r"""sre.sub(r'(.)', lambda m: s, 'x')""", s)

test(r"""sre.sub(r'(?P<a>x)', '\g<a>\g<a>', 'xx')""", 'xxxx')
test(r"""sre.sub(r'(?P<a>x)', '\g<a>\g<1>', 'xx')""", 'xxxx')
test(r"""sre.sub(r'(?P<unk>x)', '\g<unk>\g<unk>', 'xx')""", 'xxxx')
test(r"""sre.sub(r'(?P<unk>x)', '\g<1>\g<1>', 'xx')""", 'xxxx')

# bug 449964: fails for group followed by other escape
test(r"""sre.sub(r'(?P<unk>x)', '\g<1>\g<1>\\b', 'xx')""", 'xx\bxx\b')

test(r"""sre.sub(r'a', r'\t\n\v\r\f\a\b\B\Z\a\A\w\W\s\S\d\D', 'a')""", '\t\n\v\r\f\a\b\\B\\Z\a\\A\\w\\W\\s\\S\\d\\D')
test(r"""sre.sub(r'a', '\t\n\v\r\f\a', 'a')""", '\t\n\v\r\f\a')
test(r"""sre.sub(r'a', '\t\n\v\r\f\a', 'a')""", (chr(9)+chr(10)+chr(11)+chr(13)+chr(12)+chr(7)))

test(r"""sre.sub(r'^\s*', 'X', 'test')""", 'Xtest')

# qualified sub
test(r"""sre.sub(r'a', 'b', 'aaaaa')""", 'bbbbb')
test(r"""sre.sub(r'a', 'b', 'aaaaa', 1)""", 'baaaa')

# bug 114660
test(r"""sre.sub(r'(\S)\s+(\S)', r'\1 \2', 'hello  there')""", 'hello there')

# Test for sub() on escaped characters, see SF bug #449000
test(r"""sre.sub(r'\r\n', r'\n', 'abc\r\ndef\r\n')""", 'abc\ndef\n')
test(r"""sre.sub('\r\n', r'\n', 'abc\r\ndef\r\n')""", 'abc\ndef\n')
test(r"""sre.sub(r'\r\n', '\n', 'abc\r\ndef\r\n')""", 'abc\ndef\n')
test(r"""sre.sub('\r\n', '\n', 'abc\r\ndef\r\n')""", 'abc\ndef\n')

# Test for empty sub() behaviour, see SF bug #462270
test(r"""sre.sub('x*', '-', 'abxd')""", '-a-b-d-')
test(r"""sre.sub('x+', '-', 'abxd')""", 'ab-d')

if verbose:
    print 'Running tests on symbolic references'

test(r"""sre.sub(r'(?P<a>x)', '\g<a', 'xx')""", None, sre.error)
test(r"""sre.sub(r'(?P<a>x)', '\g<', 'xx')""", None, sre.error)
test(r"""sre.sub(r'(?P<a>x)', '\g', 'xx')""", None, sre.error)
test(r"""sre.sub(r'(?P<a>x)', '\g<a a>', 'xx')""", None, sre.error)
test(r"""sre.sub(r'(?P<a>x)', '\g<1a1>', 'xx')""", None, sre.error)
test(r"""sre.sub(r'(?P<a>x)', '\g<ab>', 'xx')""", None, IndexError)
test(r"""sre.sub(r'(?P<a>x)|(?P<b>y)', '\g<b>', 'xx')""", None, sre.error)
test(r"""sre.sub(r'(?P<a>x)|(?P<b>y)', '\\2', 'xx')""", None, sre.error)

if verbose:
    print 'Running tests on sre.subn'

test(r"""sre.subn(r"(?i)b+", "x", "bbbb BBBB")""", ('x x', 2))
test(r"""sre.subn(r"b+", "x", "bbbb BBBB")""", ('x BBBB', 1))
test(r"""sre.subn(r"b+", "x", "xyz")""", ('xyz', 0))
test(r"""sre.subn(r"b*", "x", "xyz")""", ('xxxyxzx', 4))
test(r"""sre.subn(r"b*", "x", "xyz", 2)""", ('xxxyz', 2))

if verbose:
    print 'Running tests on sre.split'

test(r"""sre.split(r":", ":a:b::c")""", ['', 'a', 'b', '', 'c'])
test(r"""sre.split(r":+", ":a:b:::")""", ['', 'a', 'b', ''])
test(r"""sre.split(r":*", ":a:b::c")""", ['', 'a', 'b', 'c'])
test(r"""sre.split(r"(:*)", ":a:b::c")""", ['', ':', 'a', ':', 'b', '::', 'c'])
test(r"""sre.split(r"(?::*)", ":a:b::c")""", ['', 'a', 'b', 'c'])
test(r"""sre.split(r"(:)*", ":a:b::c")""", ['', ':', 'a', ':', 'b', ':', 'c'])
test(r"""sre.split(r"([b:]+)", ":a:b::c")""", ['', ':', 'a', ':b::', 'c'])
test(r"""sre.split(r"(b)|(:+)", ":a:b::c")""",
     ['', None, ':', 'a', None, ':', '', 'b', None, '', None, '::', 'c'])
test(r"""sre.split(r"(?:b)|(?::+)", ":a:b::c")""", ['', 'a', '', '', 'c'])

test(r"""sre.split(r":", ":a:b::c", 2)""", ['', 'a', 'b::c'])
test(r"""sre.split(r':', 'a:b:c:d', 2)""", ['a', 'b', 'c:d'])

test(r"""sre.split(r"(:)", ":a:b::c", 2)""", ['', ':', 'a', ':', 'b::c'])
test(r"""sre.split(r"(:*)", ":a:b::c", 2)""", ['', ':', 'a', ':', 'b::c'])

if verbose:
    print "Running tests on sre.findall"

test(r"""sre.findall(r":+", "abc")""", [])
test(r"""sre.findall(r":+", "a:b::c:::d")""", [":", "::", ":::"])
test(r"""sre.findall(r"(:+)", "a:b::c:::d")""", [":", "::", ":::"])
test(r"""sre.findall(r"(:)(:*)", "a:b::c:::d")""",
     [(":", ""), (":", ":"), (":", "::")])
test(r"""sre.findall(r"(a)|(b)", "abc")""", [("a", ""), ("", "b")])

# bug 117612
test(r"""sre.findall(r"(a|(b))", "aba")""", [("a", ""),("b", "b"),("a", "")])

if sys.hexversion >= 0x02020000:
    if verbose:
        print "Running tests on sre.finditer"
    def fixup(seq):
        # convert iterator to list
        if not hasattr(seq, "next") or not hasattr(seq, "__iter__"):
            print "finditer returned", type(seq)
        return map(lambda item: item.group(0), seq)
    # sanity
    test(r"""fixup(sre.finditer(r":+", "a:b::c:::d"))""", [":", "::", ":::"])

if verbose:
    print "Running tests on sre.match"

test(r"""sre.match(r'a', 'a').groups()""", ())
test(r"""sre.match(r'(a)', 'a').groups()""", ('a',))
test(r"""sre.match(r'(a)', 'a').group(0)""", 'a')
test(r"""sre.match(r'(a)', 'a').group(1)""", 'a')
test(r"""sre.match(r'(a)', 'a').group(1, 1)""", ('a', 'a'))

pat = sre.compile(r'((a)|(b))(c)?')
test(r"""pat.match('a').groups()""", ('a', 'a', None, None))
test(r"""pat.match('b').groups()""", ('b', None, 'b', None))
test(r"""pat.match('ac').groups()""", ('a', 'a', None, 'c'))
test(r"""pat.match('bc').groups()""", ('b', None, 'b', 'c'))
test(r"""pat.match('bc').groups("")""", ('b', "", 'b', 'c'))

pat = sre.compile(r'(?:(?P<a1>a)|(?P<b2>b))(?P<c3>c)?')
test(r"""pat.match('a').group(1, 2, 3)""", ('a', None, None))
test(r"""pat.match('b').group('a1', 'b2', 'c3')""", (None, 'b', None))
test(r"""pat.match('ac').group(1, 'b2', 3)""", ('a', None, 'c'))

# bug 448951 (similar to 429357, but with single char match)
# (Also test greedy matches.)
for op in '','?','*':
    test(r"""sre.match(r'((.%s):)?z', 'z').groups()"""%op, (None, None))
    test(r"""sre.match(r'((.%s):)?z', 'a:z').groups()"""%op, ('a:', 'a'))

if verbose:
    print "Running tests on sre.escape"

p = ""
for i in range(0, 256):
    p = p + chr(i)
    test(r"""sre.match(sre.escape(chr(i)), chr(i)) is not None""", 1)
    test(r"""sre.match(sre.escape(chr(i)), chr(i)).span()""", (0,1))

pat = sre.compile(sre.escape(p))
test(r"""pat.match(p) is not None""", 1)
test(r"""pat.match(p).span()""", (0,256))

if verbose:
    print 'Running tests on sre.Scanner'

def s_ident(scanner, token): return token
def s_operator(scanner, token): return "op%s" % token
def s_float(scanner, token): return float(token)
def s_int(scanner, token): return int(token)

scanner = sre.Scanner([
    (r"[a-zA-Z_]\w*", s_ident),
    (r"\d+\.\d*", s_float),
    (r"\d+", s_int),
    (r"=|\+|-|\*|/", s_operator),
    (r"\s+", None),
    ])

# sanity check
test('scanner.scan("sum = 3*foo + 312.50 + bar")',
     (['sum', 'op=', 3, 'op*', 'foo', 'op+', 312.5, 'op+', 'bar'], ''))

if verbose:
    print 'Pickling a SRE_Pattern instance'

try:
    import pickle
    pat = sre.compile(r'a(?:b|(c|e){1,2}?|d)+?(.)')
    s = pickle.dumps(pat)
    pat = pickle.loads(s)
except:
    print TestFailed, 're module pickle'

try:
    import cPickle
    pat = sre.compile(r'a(?:b|(c|e){1,2}?|d)+?(.)')
    s = cPickle.dumps(pat)
    pat = cPickle.loads(s)
except:
    print TestFailed, 're module cPickle'

# constants
test(r"""sre.I""", sre.IGNORECASE)
test(r"""sre.L""", sre.LOCALE)
test(r"""sre.M""", sre.MULTILINE)
test(r"""sre.S""", sre.DOTALL)
test(r"""sre.X""", sre.VERBOSE)
test(r"""sre.T""", sre.TEMPLATE)
test(r"""sre.U""", sre.UNICODE)

for flags in [sre.I, sre.M, sre.X, sre.S, sre.L, sre.T, sre.U]:
    try:
        r = sre.compile('^pattern$', flags)
    except:
        print 'Exception raised on flag', flags

if verbose:
    print 'Test engine limitations'

# Try nasty case that overflows the straightforward recursive
# implementation of repeated groups.
test("sre.match('(x)*', 50000*'x').span()", (0, 50000), RuntimeError)
test("sre.match(r'(x)*y', 50000*'x'+'y').span()", (0, 50001), RuntimeError)
test("sre.match(r'(x)*?y', 50000*'x'+'y').span()", (0, 50001), RuntimeError)

from re_tests import *

if verbose:
    print 'Running re_tests test suite'
else:
    # To save time, only run the first and last 10 tests
    #tests = tests[:10] + tests[-10:]
    pass

for t in tests:
    sys.stdout.flush()
    pattern=s=outcome=repl=expected=None
    if len(t)==5:
        pattern, s, outcome, repl, expected = t
    elif len(t)==3:
        pattern, s, outcome = t
    else:
        raise ValueError, ('Test tuples should have 3 or 5 fields',t)

    try:
        obj=sre.compile(pattern)
    except sre.error:
        if outcome==SYNTAX_ERROR: pass  # Expected a syntax error
        else:
            print '=== Syntax error:', t
    except KeyboardInterrupt: raise KeyboardInterrupt
    except:
        print '*** Unexpected error ***', t
        if verbose:
            traceback.print_exc(file=sys.stdout)
    else:
        try:
            result=obj.search(s)
        except (sre.error), msg:
            print '=== Unexpected exception', t, repr(msg)
        if outcome==SYNTAX_ERROR:
            print '=== Compiled incorrectly', t
        elif outcome==FAIL:
            if result is None: pass   # No match, as expected
            else: print '=== Succeeded incorrectly', t
        elif outcome==SUCCEED:
            if result is not None:
                # Matched, as expected, so now we compute the
                # result string and compare it to our expected result.
                start, end = result.span(0)
                vardict={'found': result.group(0),
                         'groups': result.group(),
                         'flags': result.re.flags}
                for i in range(1, 100):
                    try:
                        gi = result.group(i)
                        # Special hack because else the string concat fails:
                        if gi is None:
                            gi = "None"
                    except IndexError:
                        gi = "Error"
                    vardict['g%d' % i] = gi
                for i in result.re.groupindex.keys():
                    try:
                        gi = result.group(i)
                        if gi is None:
                            gi = "None"
                    except IndexError:
                        gi = "Error"
                    vardict[i] = gi
                repl=eval(repl, vardict)
                if repl!=expected:
                    print '=== grouping error', t,
                    print repr(repl)+' should be '+repr(expected)
            else:
                print '=== Failed incorrectly', t
                continue

            # Try the match on a unicode string, and check that it
            # still succeeds.
            try:
                u = unicode(s, "latin-1")
            except NameError:
                pass
            except TypeError:
                continue # skip unicode test strings
            else:
                result=obj.search(u)
                if result==None:
                    print '=== Fails on unicode match', t

            # Try the match on a unicode pattern, and check that it
            # still succeeds.
            try:
                u = unicode(pattern, "latin-1")
            except NameError:
                pass
            else:
                obj=sre.compile(u)
                result=obj.search(s)
                if result==None:
                    print '=== Fails on unicode pattern match', t

            # Try the match with the search area limited to the extent
            # of the match and see if it still succeeds.  \B will
            # break (because it won't match at the end or start of a
            # string), so we'll ignore patterns that feature it.

            if pattern[:2]!='\\B' and pattern[-2:]!='\\B':
                obj=sre.compile(pattern)
                result=obj.search(s, result.start(0), result.end(0)+1)
                if result==None:
                    print '=== Failed on range-limited match', t

            # Try the match with IGNORECASE enabled, and check that it
            # still succeeds.
            obj=sre.compile(pattern, sre.IGNORECASE)
            result=obj.search(s)
            if result==None:
                print '=== Fails on case-insensitive match', t

            # Try the match with LOCALE enabled, and check that it
            # still succeeds.
            obj=sre.compile(pattern, sre.LOCALE)
            result=obj.search(s)
            if result==None:
                print '=== Fails on locale-sensitive match', t

            # Try the match with UNICODE locale enabled, and check
            # that it still succeeds.
            if have_unicode:
                obj=sre.compile(pattern, sre.UNICODE)
                result=obj.search(s)
                if result==None:
                    print '=== Fails on unicode-sensitive match', t
