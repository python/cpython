# SRE test harness for the Python regression suite

# this is based on test_re.py, but uses a test function instead
# of all those asserts

import sys
sys.path=['.']+sys.path

from test.test_support import verbose, TestFailed, have_unicode
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
