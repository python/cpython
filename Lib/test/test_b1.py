# Python test set -- part 4a, built-in functions a-m

from test_support import *

print '__import__'
__import__('sys')
__import__('time')
__import__('string')
try: __import__('spamspam')
except ImportError: pass
else: raise TestFailed, "__import__('spamspam') should fail"

print 'abs'
if abs(0) != 0: raise TestFailed, 'abs(0)'
if abs(1234) != 1234: raise TestFailed, 'abs(1234)'
if abs(-1234) != 1234: raise TestFailed, 'abs(-1234)'
#
if abs(0.0) != 0.0: raise TestFailed, 'abs(0.0)'
if abs(3.14) != 3.14: raise TestFailed, 'abs(3.14)'
if abs(-3.14) != 3.14: raise TestFailed, 'abs(-3.14)'
#
if abs(0L) != 0L: raise TestFailed, 'abs(0L)'
if abs(1234L) != 1234L: raise TestFailed, 'abs(1234L)'
if abs(-1234L) != 1234L: raise TestFailed, 'abs(-1234L)'

try: abs('a')
except TypeError: pass
else: raise TestFailed, 'abs("a")'

print 'apply'
def f0(*args):
    if args != (): raise TestFailed, 'f0 called with ' + `args`
def f1(a1):
    if a1 != 1: raise TestFailed, 'f1 called with ' + `a1`
def f2(a1, a2):
    if a1 != 1 or a2 != 2:
        raise TestFailed, 'f2 called with ' + `a1, a2`
def f3(a1, a2, a3):
    if a1 != 1 or a2 != 2 or a3 != 3:
        raise TestFailed, 'f3 called with ' + `a1, a2, a3`
apply(f0, ())
apply(f1, (1,))
apply(f2, (1, 2))
apply(f3, (1, 2, 3))

# A PyCFunction that takes only positional parameters should allow an
# empty keyword dictionary to pass without a complaint, but raise a
# TypeError if the dictionary is non-empty.
apply(id, (1,), {})
try:
    apply(id, (1,), {"foo": 1})
except TypeError:
    pass
else:
    raise TestFailed, 'expected TypeError; no exception raised'

print 'callable'
if not callable(len):raise TestFailed, 'callable(len)'
def f(): pass
if not callable(f): raise TestFailed, 'callable(f)'
class C:
    def meth(self): pass
if not callable(C): raise TestFailed, 'callable(C)'
x = C()
if not callable(x.meth): raise TestFailed, 'callable(x.meth)'
if callable(x): raise TestFailed, 'callable(x)'
class D(C):
    def __call__(self): pass
y = D()
if not callable(y): raise TestFailed, 'callable(y)'
y()

print 'chr'
if chr(32) != ' ': raise TestFailed, 'chr(32)'
if chr(65) != 'A': raise TestFailed, 'chr(65)'
if chr(97) != 'a': raise TestFailed, 'chr(97)'

# cmp
print 'cmp'
if cmp(-1, 1) != -1: raise TestFailed, 'cmp(-1, 1)'
if cmp(1, -1) != 1: raise TestFailed, 'cmp(1, -1)'
if cmp(1, 1) != 0: raise TestFailed, 'cmp(1, 1)'
# verify that circular objects are handled
a = []; a.append(a)
b = []; b.append(b)
from UserList import UserList
c = UserList(); c.append(c)
if cmp(a, b) != 0: raise TestFailed, "cmp(%s, %s)" % (a, b)
if cmp(b, c) != 0: raise TestFailed, "cmp(%s, %s)" % (b, c)
if cmp(c, a) != 0: raise TestFailed, "cmp(%s, %s)" % (c, a)
if cmp(a, c) != 0: raise TestFailed, "cmp(%s, %s)" % (a, c)
# okay, now break the cycles
a.pop(); b.pop(); c.pop()

print 'coerce'
if fcmp(coerce(1, 1.1), (1.0, 1.1)): raise TestFailed, 'coerce(1, 1.1)'
if coerce(1, 1L) != (1L, 1L): raise TestFailed, 'coerce(1, 1L)'
if fcmp(coerce(1L, 1.1), (1.0, 1.1)): raise TestFailed, 'coerce(1L, 1.1)'

try: coerce(0.5, long("12345" * 1000))
except OverflowError: pass
else: raise TestFailed, 'coerce(0.5, long("12345" * 1000))'

print 'compile'
compile('print 1\n', '', 'exec')

print 'complex'
if complex(1,10) != 1+10j: raise TestFailed, 'complex(1,10)'
if complex(1,10L) != 1+10j: raise TestFailed, 'complex(1,10L)'
if complex(1,10.0) != 1+10j: raise TestFailed, 'complex(1,10.0)'
if complex(1L,10) != 1+10j: raise TestFailed, 'complex(1L,10)'
if complex(1L,10L) != 1+10j: raise TestFailed, 'complex(1L,10L)'
if complex(1L,10.0) != 1+10j: raise TestFailed, 'complex(1L,10.0)'
if complex(1.0,10) != 1+10j: raise TestFailed, 'complex(1.0,10)'
if complex(1.0,10L) != 1+10j: raise TestFailed, 'complex(1.0,10L)'
if complex(1.0,10.0) != 1+10j: raise TestFailed, 'complex(1.0,10.0)'
if complex(3.14+0j) != 3.14+0j: raise TestFailed, 'complex(3.14)'
if complex(3.14) != 3.14+0j: raise TestFailed, 'complex(3.14)'
if complex(314) != 314.0+0j: raise TestFailed, 'complex(314)'
if complex(314L) != 314.0+0j: raise TestFailed, 'complex(314L)'
if complex(3.14+0j, 0j) != 3.14+0j: raise TestFailed, 'complex(3.14, 0j)'
if complex(3.14, 0.0) != 3.14+0j: raise TestFailed, 'complex(3.14, 0.0)'
if complex(314, 0) != 314.0+0j: raise TestFailed, 'complex(314, 0)'
if complex(314L, 0L) != 314.0+0j: raise TestFailed, 'complex(314L, 0L)'
if complex(0j, 3.14j) != -3.14+0j: raise TestFailed, 'complex(0j, 3.14j)'
if complex(0.0, 3.14j) != -3.14+0j: raise TestFailed, 'complex(0.0, 3.14j)'
if complex(0j, 3.14) != 3.14j: raise TestFailed, 'complex(0j, 3.14)'
if complex(0.0, 3.14) != 3.14j: raise TestFailed, 'complex(0.0, 3.14)'
if complex("1") != 1+0j: raise TestFailed, 'complex("1")'
if complex("1j") != 1j: raise TestFailed, 'complex("1j")'

try: complex("1", "1")
except TypeError: pass
else: raise TestFailed, 'complex("1", "1")'

try: complex(1, "1")
except TypeError: pass
else: raise TestFailed, 'complex(1, "1")'

if complex("  3.14+J  ") != 3.14+1j:  raise TestFailed, 'complex("  3.14+J  )"'
if have_unicode:
    if complex(unicode("  3.14+J  ")) != 3.14+1j:
        raise TestFailed, 'complex(u"  3.14+J  )"'

# SF bug 543840:  complex(string) accepts strings with \0
# Fixed in 2.3.
try:
    complex('1+1j\0j')
except ValueError:
    pass
else:
    raise TestFailed("complex('1+1j\0j') should have raised ValueError")

class Z:
    def __complex__(self): return 3.14j
z = Z()
if complex(z) != 3.14j: raise TestFailed, 'complex(classinstance)'

print 'delattr'
import sys
sys.spam = 1
delattr(sys, 'spam')

print 'dir'
x = 1
if 'x' not in dir(): raise TestFailed, 'dir()'
import sys
if 'modules' not in dir(sys): raise TestFailed, 'dir(sys)'

print 'divmod'
if divmod(12, 7) != (1, 5): raise TestFailed, 'divmod(12, 7)'
if divmod(-12, 7) != (-2, 2): raise TestFailed, 'divmod(-12, 7)'
if divmod(12, -7) != (-2, -2): raise TestFailed, 'divmod(12, -7)'
if divmod(-12, -7) != (1, -5): raise TestFailed, 'divmod(-12, -7)'
#
if divmod(12L, 7L) != (1L, 5L): raise TestFailed, 'divmod(12L, 7L)'
if divmod(-12L, 7L) != (-2L, 2L): raise TestFailed, 'divmod(-12L, 7L)'
if divmod(12L, -7L) != (-2L, -2L): raise TestFailed, 'divmod(12L, -7L)'
if divmod(-12L, -7L) != (1L, -5L): raise TestFailed, 'divmod(-12L, -7L)'
#
if divmod(12, 7L) != (1, 5L): raise TestFailed, 'divmod(12, 7L)'
if divmod(-12, 7L) != (-2, 2L): raise TestFailed, 'divmod(-12, 7L)'
if divmod(12L, -7) != (-2L, -2): raise TestFailed, 'divmod(12L, -7)'
if divmod(-12L, -7) != (1L, -5): raise TestFailed, 'divmod(-12L, -7)'
#
if fcmp(divmod(3.25, 1.0), (3.0, 0.25)):
    raise TestFailed, 'divmod(3.25, 1.0)'
if fcmp(divmod(-3.25, 1.0), (-4.0, 0.75)):
    raise TestFailed, 'divmod(-3.25, 1.0)'
if fcmp(divmod(3.25, -1.0), (-4.0, -0.75)):
    raise TestFailed, 'divmod(3.25, -1.0)'
if fcmp(divmod(-3.25, -1.0), (3.0, -0.25)):
    raise TestFailed, 'divmod(-3.25, -1.0)'

print 'eval'
if eval('1+1') != 2: raise TestFailed, 'eval(\'1+1\')'
if eval(' 1+1\n') != 2: raise TestFailed, 'eval(\' 1+1\\n\')'
globals = {'a': 1, 'b': 2}
locals = {'b': 200, 'c': 300}
if eval('a', globals) != 1:
    raise TestFailed, "eval(1) == %s" % eval('a', globals)
if eval('a', globals, locals) != 1:
    raise TestFailed, "eval(2)"
if eval('b', globals, locals) != 200:
    raise TestFailed, "eval(3)"
if eval('c', globals, locals) != 300:
    raise TestFailed, "eval(4)"
if have_unicode:
    if eval(unicode('1+1')) != 2: raise TestFailed, 'eval(u\'1+1\')'
    if eval(unicode(' 1+1\n')) != 2: raise TestFailed, 'eval(u\' 1+1\\n\')'
globals = {'a': 1, 'b': 2}
locals = {'b': 200, 'c': 300}
if have_unicode:
    if eval(unicode('a'), globals) != 1:
        raise TestFailed, "eval(1) == %s" % eval(unicode('a'), globals)
    if eval(unicode('a'), globals, locals) != 1:
        raise TestFailed, "eval(2)"
    if eval(unicode('b'), globals, locals) != 200:
        raise TestFailed, "eval(3)"
    if eval(unicode('c'), globals, locals) != 300:
        raise TestFailed, "eval(4)"

print 'execfile'
z = 0
f = open(TESTFN, 'w')
f.write('z = z+1\n')
f.write('z = z*2\n')
f.close()
execfile(TESTFN)
if z != 2: raise TestFailed, "execfile(1)"
globals['z'] = 0
execfile(TESTFN, globals)
if globals['z'] != 2: raise TestFailed, "execfile(1)"
locals['z'] = 0
execfile(TESTFN, globals, locals)
if locals['z'] != 2: raise TestFailed, "execfile(1)"
unlink(TESTFN)

print 'filter'
if filter(lambda c: 'a' <= c <= 'z', 'Hello World') != 'elloorld':
    raise TestFailed, 'filter (filter a string)'
if filter(None, [1, 'hello', [], [3], '', None, 9, 0]) != [1, 'hello', [3], 9]:
    raise TestFailed, 'filter (remove false values)'
if filter(lambda x: x > 0, [1, -3, 9, 0, 2]) != [1, 9, 2]:
    raise TestFailed, 'filter (keep positives)'
class Squares:
    def __init__(self, max):
        self.max = max
        self.sofar = []
    def __len__(self): return len(self.sofar)
    def __getitem__(self, i):
        if not 0 <= i < self.max: raise IndexError
        n = len(self.sofar)
        while n <= i:
            self.sofar.append(n*n)
            n = n+1
        return self.sofar[i]
if filter(None, Squares(10)) != [1, 4, 9, 16, 25, 36, 49, 64, 81]:
    raise TestFailed, 'filter(None, Squares(10))'
if filter(lambda x: x%2, Squares(10)) != [1, 9, 25, 49, 81]:
    raise TestFailed, 'filter(oddp, Squares(10))'
class StrSquares:
    def __init__(self, max):
        self.max = max
        self.sofar = []
    def __len__(self):
        return len(self.sofar)
    def __getitem__(self, i):
        if not 0 <= i < self.max:
            raise IndexError
        n = len(self.sofar)
        while n <= i:
            self.sofar.append(str(n*n))
            n = n+1
        return self.sofar[i]
def identity(item):
    return 1
filter(identity, Squares(5))

print 'float'
if float(3.14) != 3.14: raise TestFailed, 'float(3.14)'
if float(314) != 314.0: raise TestFailed, 'float(314)'
if float(314L) != 314.0: raise TestFailed, 'float(314L)'
if float("  3.14  ") != 3.14:  raise TestFailed, 'float("  3.14  ")'
if have_unicode:
    if float(unicode("  3.14  ")) != 3.14:
        raise TestFailed, 'float(u"  3.14  ")'
    if float(unicode("  \u0663.\u0661\u0664  ",'raw-unicode-escape')) != 3.14:
        raise TestFailed, 'float(u"  \u0663.\u0661\u0664  ")'

print 'getattr'
import sys
if getattr(sys, 'stdout') is not sys.stdout: raise TestFailed, 'getattr'
try:
    getattr(sys, 1)
except TypeError:
    pass
else:
    raise TestFailed, "getattr(sys, 1) should raise an exception"
try:
    getattr(sys, 1, "foo")
except TypeError:
    pass
else:
    raise TestFailed, 'getattr(sys, 1, "foo") should raise an exception'

print 'hasattr'
import sys
if not hasattr(sys, 'stdout'): raise TestFailed, 'hasattr'
try:
    hasattr(sys, 1)
except TypeError:
    pass
else:
    raise TestFailed, "hasattr(sys, 1) should raise an exception"

print 'hash'
hash(None)
if not hash(1) == hash(1L) == hash(1.0): raise TestFailed, 'numeric hash()'
hash('spam')
hash((0,1,2,3))
def f(): pass
try: hash([])
except TypeError: pass
else: raise TestFailed, "hash([]) should raise an exception"
try: hash({})
except TypeError: pass
else: raise TestFailed, "hash({}) should raise an exception"

print 'hex'
if hex(16) != '0x10': raise TestFailed, 'hex(16)'
if hex(16L) != '0x10L': raise TestFailed, 'hex(16L)'
if len(hex(-1)) != len(hex(sys.maxint)): raise TestFailed, 'len(hex(-1))'
if hex(-16) not in ('0xfffffff0', '0xfffffffffffffff0'):
    raise TestFailed, 'hex(-16)'
if hex(-16L) != '-0x10L': raise TestFailed, 'hex(-16L)'

print 'id'
id(None)
id(1)
id(1L)
id(1.0)
id('spam')
id((0,1,2,3))
id([0,1,2,3])
id({'spam': 1, 'eggs': 2, 'ham': 3})

# Test input() later, together with raw_input

print 'int'
if int(314) != 314: raise TestFailed, 'int(314)'
if int(3.14) != 3: raise TestFailed, 'int(3.14)'
if int(314L) != 314: raise TestFailed, 'int(314L)'
# Check that conversion from float truncates towards zero
if int(-3.14) != -3: raise TestFailed, 'int(-3.14)'
if int(3.9) != 3: raise TestFailed, 'int(3.9)'
if int(-3.9) != -3: raise TestFailed, 'int(-3.9)'
if int(3.5) != 3: raise TestFailed, 'int(3.5)'
if int(-3.5) != -3: raise TestFailed, 'int(-3.5)'
# Different base:
if int("10",16) != 16L: raise TestFailed, 'int("10",16)'
if have_unicode:
    if int(unicode("10"),16) != 16L:
        raise TestFailed, 'int(u"10",16)'
# Test conversion from strings and various anomalies
L = [
        ('0', 0),
        ('1', 1),
        ('9', 9),
        ('10', 10),
        ('99', 99),
        ('100', 100),
        ('314', 314),
        (' 314', 314),
        ('314 ', 314),
        ('  \t\t  314  \t\t  ', 314),
        (`sys.maxint`, sys.maxint),
        ('  1x', ValueError),
        ('  1  ', 1),
        ('  1\02  ', ValueError),
        ('', ValueError),
        (' ', ValueError),
        ('  \t\t  ', ValueError)
]
if have_unicode:
    L += [
        (unicode('0'), 0),
        (unicode('1'), 1),
        (unicode('9'), 9),
        (unicode('10'), 10),
        (unicode('99'), 99),
        (unicode('100'), 100),
        (unicode('314'), 314),
        (unicode(' 314'), 314),
        (unicode('\u0663\u0661\u0664 ','raw-unicode-escape'), 314),
        (unicode('  \t\t  314  \t\t  '), 314),
        (unicode('  1x'), ValueError),
        (unicode('  1  '), 1),
        (unicode('  1\02  '), ValueError),
        (unicode(''), ValueError),
        (unicode(' '), ValueError),
        (unicode('  \t\t  '), ValueError),
]
for s, v in L:
    for sign in "", "+", "-":
        for prefix in "", " ", "\t", "  \t\t  ":
            ss = prefix + sign + s
            vv = v
            if sign == "-" and v is not ValueError:
                vv = -v
            try:
                if int(ss) != vv:
                    raise TestFailed, "int(%s)" % `ss`
            except v:
                pass
            except ValueError, e:
                raise TestFailed, "int(%s) raised ValueError: %s" % (`ss`, e)
s = `-1-sys.maxint`
if int(s)+1 != -sys.maxint:
    raise TestFailed, "int(%s)" % `s`
try:
    int(s[1:])
except ValueError:
    pass
else:
    raise TestFailed, "int(%s)" % `s[1:]` + " should raise ValueError"
try:
    int(1e100)
except OverflowError:
    pass
else:
    raise TestFailed("int(1e100) expected OverflowError")
try:
    int(-1e100)
except OverflowError:
    pass
else:
    raise TestFailed("int(-1e100) expected OverflowError")


# SF bug 434186:  0x80000000/2 != 0x80000000>>1.
# Worked by accident in Windows release build, but failed in debug build.
# Failed in all Linux builds.
x = -1-sys.maxint
if x >> 1 != x//2:
    raise TestFailed("x >> 1 != x/2 when x == -1-sys.maxint")

try: int('123\0')
except ValueError: pass
else: raise TestFailed("int('123\0') didn't raise exception")

print 'isinstance'
class C:
    pass
class D(C):
    pass
class E:
    pass
c = C()
d = D()
e = E()
if not isinstance(c, C): raise TestFailed, 'isinstance(c, C)'
if not isinstance(d, C): raise TestFailed, 'isinstance(d, C)'
if isinstance(e, C): raise TestFailed, 'isinstance(e, C)'
if isinstance(c, D): raise TestFailed, 'isinstance(c, D)'
if isinstance('foo', E): raise TestFailed, 'isinstance("Foo", E)'
try:
    isinstance(E, 'foo')
    raise TestFailed, 'isinstance(E, "foo")'
except TypeError:
    pass

print 'issubclass'
if not issubclass(D, C): raise TestFailed, 'issubclass(D, C)'
if not issubclass(C, C): raise TestFailed, 'issubclass(C, C)'
if issubclass(C, D): raise TestFailed, 'issubclass(C, D)'
try:
    issubclass('foo', E)
    raise TestFailed, 'issubclass("foo", E)'
except TypeError:
    pass
try:
    issubclass(E, 'foo')
    raise TestFailed, 'issubclass(E, "foo")'
except TypeError:
    pass

print 'len'
if len('123') != 3: raise TestFailed, 'len(\'123\')'
if len(()) != 0: raise TestFailed, 'len(())'
if len((1, 2, 3, 4)) != 4: raise TestFailed, 'len((1, 2, 3, 4))'
if len([1, 2, 3, 4]) != 4: raise TestFailed, 'len([1, 2, 3, 4])'
if len({}) != 0: raise TestFailed, 'len({})'
if len({'a':1, 'b': 2}) != 2: raise TestFailed, 'len({\'a\':1, \'b\': 2})'

print 'list'
if list([]) != []: raise TestFailed, 'list([])'
l0_3 = [0, 1, 2, 3]
l0_3_bis = list(l0_3)
if l0_3 != l0_3_bis or l0_3 is l0_3_bis: raise TestFailed, 'list([0, 1, 2, 3])'
if list(()) != []: raise TestFailed, 'list(())'
if list((0, 1, 2, 3)) != [0, 1, 2, 3]: raise TestFailed, 'list((0, 1, 2, 3))'
if list('') != []: raise TestFailed, 'list('')'
if list('spam') != ['s', 'p', 'a', 'm']: raise TestFailed, "list('spam')"

if sys.maxint == 0x7fffffff:
    # This test can currently only work on 32-bit machines.
    # XXX If/when PySequence_Length() returns a ssize_t, it should be
    # XXX re-enabled.
    try:
        # Verify clearing of bug #556025.
        # This assumes that the max data size (sys.maxint) == max
        # address size this also assumes that the address size is at
        # least 4 bytes with 8 byte addresses, the bug is not well
        # tested
        #
        # Note: This test is expected to SEGV under Cygwin 1.3.12 or
        # earlier due to a newlib bug.  See the following mailing list
        # thread for the details:

        #     http://sources.redhat.com/ml/newlib/2002/msg00369.html
        list(xrange(sys.maxint // 2))
    except MemoryError:
        pass
    else:
        raise TestFailed, 'list(xrange(sys.maxint / 4))'

print 'long'
if long(314) != 314L: raise TestFailed, 'long(314)'
if long(3.14) != 3L: raise TestFailed, 'long(3.14)'
if long(314L) != 314L: raise TestFailed, 'long(314L)'
# Check that conversion from float truncates towards zero
if long(-3.14) != -3L: raise TestFailed, 'long(-3.14)'
if long(3.9) != 3L: raise TestFailed, 'long(3.9)'
if long(-3.9) != -3L: raise TestFailed, 'long(-3.9)'
if long(3.5) != 3L: raise TestFailed, 'long(3.5)'
if long(-3.5) != -3L: raise TestFailed, 'long(-3.5)'
if long("-3") != -3L: raise TestFailed, 'long("-3")'
if have_unicode:
    if long(unicode("-3")) != -3L:
        raise TestFailed, 'long(u"-3")'
# Different base:
if long("10",16) != 16L: raise TestFailed, 'long("10",16)'
if have_unicode:
    if long(unicode("10"),16) != 16L:
        raise TestFailed, 'long(u"10",16)'
# Check conversions from string (same test set as for int(), and then some)
LL = [
        ('1' + '0'*20, 10L**20),
        ('1' + '0'*100, 10L**100)
]
if have_unicode:
    L+=[
        (unicode('1') + unicode('0')*20, 10L**20),
        (unicode('1') + unicode('0')*100, 10L**100),
]
for s, v in L + LL:
    for sign in "", "+", "-":
        for prefix in "", " ", "\t", "  \t\t  ":
            ss = prefix + sign + s
            vv = v
            if sign == "-" and v is not ValueError:
                vv = -v
            try:
                if long(ss) != long(vv):
                    raise TestFailed, "long(%s)" % `ss`
            except v:
                pass
            except ValueError, e:
                raise TestFailed, "long(%s) raised ValueError: %s" % (`ss`, e)

try: long('123\0')
except ValueError: pass
else: raise TestFailed("long('123\0') didn't raise exception")

print 'map'
if map(None, 'hello world') != ['h','e','l','l','o',' ','w','o','r','l','d']:
    raise TestFailed, 'map(None, \'hello world\')'
if map(None, 'abcd', 'efg') != \
   [('a', 'e'), ('b', 'f'), ('c', 'g'), ('d', None)]:
    raise TestFailed, 'map(None, \'abcd\', \'efg\')'
if map(None, range(10)) != [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]:
    raise TestFailed, 'map(None, range(10))'
if map(lambda x: x*x, range(1,4)) != [1, 4, 9]:
    raise TestFailed, 'map(lambda x: x*x, range(1,4))'
try:
    from math import sqrt
except ImportError:
    def sqrt(x):
        return pow(x, 0.5)
if map(lambda x: map(sqrt,x), [[16, 4], [81, 9]]) != [[4.0, 2.0], [9.0, 3.0]]:
    raise TestFailed, 'map(lambda x: map(sqrt,x), [[16, 4], [81, 9]])'
if map(lambda x, y: x+y, [1,3,2], [9,1,4]) != [10, 4, 6]:
    raise TestFailed, 'map(lambda x,y: x+y, [1,3,2], [9,1,4])'
def plus(*v):
    accu = 0
    for i in v: accu = accu + i
    return accu
if map(plus, [1, 3, 7]) != [1, 3, 7]:
    raise TestFailed, 'map(plus, [1, 3, 7])'
if map(plus, [1, 3, 7], [4, 9, 2]) != [1+4, 3+9, 7+2]:
    raise TestFailed, 'map(plus, [1, 3, 7], [4, 9, 2])'
if map(plus, [1, 3, 7], [4, 9, 2], [1, 1, 0]) != [1+4+1, 3+9+1, 7+2+0]:
    raise TestFailed, 'map(plus, [1, 3, 7], [4, 9, 2], [1, 1, 0])'
if map(None, Squares(10)) != [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]:
    raise TestFailed, 'map(None, Squares(10))'
if map(int, Squares(10)) != [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]:
    raise TestFailed, 'map(int, Squares(10))'
if map(None, Squares(3), Squares(2)) != [(0,0), (1,1), (4,None)]:
    raise TestFailed, 'map(None, Squares(3), Squares(2))'
if map(max, Squares(3), Squares(2)) != [0, 1, 4]:
    raise TestFailed, 'map(max, Squares(3), Squares(2))'

print 'max'
if max('123123') != '3': raise TestFailed, 'max(\'123123\')'
if max(1, 2, 3) != 3: raise TestFailed, 'max(1, 2, 3)'
if max((1, 2, 3, 1, 2, 3)) != 3: raise TestFailed, 'max((1, 2, 3, 1, 2, 3))'
if max([1, 2, 3, 1, 2, 3]) != 3: raise TestFailed, 'max([1, 2, 3, 1, 2, 3])'
#
if max(1, 2L, 3.0) != 3.0: raise TestFailed, 'max(1, 2L, 3.0)'
if max(1L, 2.0, 3) != 3: raise TestFailed, 'max(1L, 2.0, 3)'
if max(1.0, 2, 3L) != 3L: raise TestFailed, 'max(1.0, 2, 3L)'

print 'min'
if min('123123') != '1': raise TestFailed, 'min(\'123123\')'
if min(1, 2, 3) != 1: raise TestFailed, 'min(1, 2, 3)'
if min((1, 2, 3, 1, 2, 3)) != 1: raise TestFailed, 'min((1, 2, 3, 1, 2, 3))'
if min([1, 2, 3, 1, 2, 3]) != 1: raise TestFailed, 'min([1, 2, 3, 1, 2, 3])'
#
if min(1, 2L, 3.0) != 1: raise TestFailed, 'min(1, 2L, 3.0)'
if min(1L, 2.0, 3) != 1L: raise TestFailed, 'min(1L, 2.0, 3)'
if min(1.0, 2, 3L) != 1.0: raise TestFailed, 'min(1.0, 2, 3L)'
