# Python test set -- part 6, built-in types

from test.test_support import *

print '6. Built-in types'

print '6.1 Truth value testing'
if None: raise TestFailed, 'None is true instead of false'
if 0: raise TestFailed, '0 is true instead of false'
if 0L: raise TestFailed, '0L is true instead of false'
if 0.0: raise TestFailed, '0.0 is true instead of false'
if '': raise TestFailed, '\'\' is true instead of false'
if {}: raise TestFailed, '{} is true instead of false'
if not 1: raise TestFailed, '1 is false instead of true'
if not 1L: raise TestFailed, '1L is false instead of true'
if not 1.0: raise TestFailed, '1.0 is false instead of true'
if not 'x': raise TestFailed, '\'x\' is false instead of true'
if not {'x': 1}: raise TestFailed, '{\'x\': 1} is false instead of true'
def f(): pass
class C: pass
import sys
x = C()
if not f: raise TestFailed, 'f is false instead of true'
if not C: raise TestFailed, 'C is false instead of true'
if not sys: raise TestFailed, 'sys is false instead of true'
if not x: raise TestFailed, 'x is false instead of true'

print '6.2 Boolean operations'
if 0 or 0: raise TestFailed, '0 or 0 is true instead of false'
if 1 and 1: pass
else: raise TestFailed, '1 and 1 is false instead of true'
if not 1: raise TestFailed, 'not 1 is true instead of false'

print '6.3 Comparisons'
if 0 < 1 <= 1 == 1 >= 1 > 0 != 1: pass
else: raise TestFailed, 'int comparisons failed'
if 0L < 1L <= 1L == 1L >= 1L > 0L != 1L: pass
else: raise TestFailed, 'long int comparisons failed'
if 0.0 < 1.0 <= 1.0 == 1.0 >= 1.0 > 0.0 != 1.0: pass
else: raise TestFailed, 'float comparisons failed'
if '' < 'a' <= 'a' == 'a' < 'abc' < 'abd' < 'b': pass
else: raise TestFailed, 'string comparisons failed'
if None is None: pass
else: raise TestFailed, 'identity test failed'

try: float('')
except ValueError: pass
else: raise TestFailed, "float('') didn't raise ValueError"

try: float('5\0')
except ValueError: pass
else: raise TestFailed, "float('5\0') didn't raise ValueError"

try: 5.0 / 0.0
except ZeroDivisionError: pass
else: raise TestFailed, "5.0 / 0.0 didn't raise ZeroDivisionError"

try: 5.0 // 0.0
except ZeroDivisionError: pass
else: raise TestFailed, "5.0 // 0.0 didn't raise ZeroDivisionError"

try: 5.0 % 0.0
except ZeroDivisionError: pass
else: raise TestFailed, "5.0 % 0.0 didn't raise ZeroDivisionError"

try: 5 / 0L
except ZeroDivisionError: pass
else: raise TestFailed, "5 / 0L didn't raise ZeroDivisionError"

try: 5 // 0L
except ZeroDivisionError: pass
else: raise TestFailed, "5 // 0L didn't raise ZeroDivisionError"

try: 5 % 0L
except ZeroDivisionError: pass
else: raise TestFailed, "5 % 0L didn't raise ZeroDivisionError"

print '6.4 Numeric types (mostly conversions)'
if 0 != 0L or 0 != 0.0 or 0L != 0.0: raise TestFailed, 'mixed comparisons'
if 1 != 1L or 1 != 1.0 or 1L != 1.0: raise TestFailed, 'mixed comparisons'
if -1 != -1L or -1 != -1.0 or -1L != -1.0:
    raise TestFailed, 'int/long/float value not equal'
# calling built-in types without argument must return 0
if int() != 0: raise TestFailed, 'int() does not return 0'
if long() != 0L: raise TestFailed, 'long() does not return 0L'
if float() != 0.0: raise TestFailed, 'float() does not return 0.0'
if int(1.9) == 1 == int(1.1) and int(-1.1) == -1 == int(-1.9): pass
else: raise TestFailed, 'int() does not round properly'
if long(1.9) == 1L == long(1.1) and long(-1.1) == -1L == long(-1.9): pass
else: raise TestFailed, 'long() does not round properly'
if float(1) == 1.0 and float(-1) == -1.0 and float(0) == 0.0: pass
else: raise TestFailed, 'float() does not work properly'
print '6.4.1 32-bit integers'
if 12 + 24 != 36: raise TestFailed, 'int op'
if 12 + (-24) != -12: raise TestFailed, 'int op'
if (-12) + 24 != 12: raise TestFailed, 'int op'
if (-12) + (-24) != -36: raise TestFailed, 'int op'
if not 12 < 24: raise TestFailed, 'int op'
if not -24 < -12: raise TestFailed, 'int op'
# Test for a particular bug in integer multiply
xsize, ysize, zsize = 238, 356, 4
if not (xsize*ysize*zsize == zsize*xsize*ysize == 338912):
    raise TestFailed, 'int mul commutativity'
# And another.
m = -sys.maxint - 1
for divisor in 1, 2, 4, 8, 16, 32:
    j = m // divisor
    prod = divisor * j
    if prod != m:
        raise TestFailed, "%r * %r == %r != %r" % (divisor, j, prod, m)
    if type(prod) is not int:
        raise TestFailed, ("expected type(prod) to be int, not %r" %
                           type(prod))
# Check for expected * overflow to long.
for divisor in 1, 2, 4, 8, 16, 32:
    j = m // divisor - 1
    prod = divisor * j
    if type(prod) is not long:
        raise TestFailed, ("expected type(%r) to be long, not %r" %
                           (prod, type(prod)))
# Check for expected * overflow to long.
m = sys.maxint
for divisor in 1, 2, 4, 8, 16, 32:
    j = m // divisor + 1
    prod = divisor * j
    if type(prod) is not long:
        raise TestFailed, ("expected type(%r) to be long, not %r" %
                           (prod, type(prod)))

print '6.4.2 Long integers'
if 12L + 24L != 36L: raise TestFailed, 'long op'
if 12L + (-24L) != -12L: raise TestFailed, 'long op'
if (-12L) + 24L != 12L: raise TestFailed, 'long op'
if (-12L) + (-24L) != -36L: raise TestFailed, 'long op'
if not 12L < 24L: raise TestFailed, 'long op'
if not -24L < -12L: raise TestFailed, 'long op'
x = sys.maxint
if int(long(x)) != x: raise TestFailed, 'long op'
try: y = int(long(x)+1L)
except OverflowError: raise TestFailed, 'long op'
if not isinstance(y, long): raise TestFailed, 'long op'
x = -x
if int(long(x)) != x: raise TestFailed, 'long op'
x = x-1
if int(long(x)) != x: raise TestFailed, 'long op'
try: y = int(long(x)-1L)
except OverflowError: raise TestFailed, 'long op'
if not isinstance(y, long): raise TestFailed, 'long op'

try: 5 << -5
except ValueError: pass
else: raise TestFailed, 'int negative shift <<'

try: 5L << -5L
except ValueError: pass
else: raise TestFailed, 'long negative shift <<'

try: 5 >> -5
except ValueError: pass
else: raise TestFailed, 'int negative shift >>'

try: 5L >> -5L
except ValueError: pass
else: raise TestFailed, 'long negative shift >>'

print '6.4.3 Floating point numbers'
if 12.0 + 24.0 != 36.0: raise TestFailed, 'float op'
if 12.0 + (-24.0) != -12.0: raise TestFailed, 'float op'
if (-12.0) + 24.0 != 12.0: raise TestFailed, 'float op'
if (-12.0) + (-24.0) != -36.0: raise TestFailed, 'float op'
if not 12.0 < 24.0: raise TestFailed, 'float op'
if not -24.0 < -12.0: raise TestFailed, 'float op'

print '6.5 Sequence types'

print '6.5.1 Strings'
if len('') != 0: raise TestFailed, 'len(\'\')'
if len('a') != 1: raise TestFailed, 'len(\'a\')'
if len('abcdef') != 6: raise TestFailed, 'len(\'abcdef\')'
if 'xyz' + 'abcde' != 'xyzabcde': raise TestFailed, 'string concatenation'
if 'xyz'*3 != 'xyzxyzxyz': raise TestFailed, 'string repetition *3'
if 0*'abcde' != '': raise TestFailed, 'string repetition 0*'
if min('abc') != 'a' or max('abc') != 'c': raise TestFailed, 'min/max string'
if 'a' in 'abc' and 'b' in 'abc' and 'c' in 'abc' and 'd' not in 'abc': pass
else: raise TestFailed, 'in/not in string'
x = 'x'*103
if '%s!'%x != x+'!': raise TestFailed, 'nasty string formatting bug'

#extended slices for strings
a = '0123456789'
vereq(a[::], a)
vereq(a[::2], '02468')
vereq(a[1::2], '13579')
vereq(a[::-1],'9876543210')
vereq(a[::-2], '97531')
vereq(a[3::-2], '31')
vereq(a[-100:100:], a)
vereq(a[100:-100:-1], a[::-1])
vereq(a[-100L:100L:2L], '02468')

if have_unicode:
    a = unicode('0123456789', 'ascii')
    vereq(a[::], a)
    vereq(a[::2], unicode('02468', 'ascii'))
    vereq(a[1::2], unicode('13579', 'ascii'))
    vereq(a[::-1], unicode('9876543210', 'ascii'))
    vereq(a[::-2], unicode('97531', 'ascii'))
    vereq(a[3::-2], unicode('31', 'ascii'))
    vereq(a[-100:100:], a)
    vereq(a[100:-100:-1], a[::-1])
    vereq(a[-100L:100L:2L], unicode('02468', 'ascii'))


print '6.5.2 Tuples [see test_tuple.py]'

print '6.5.3 Lists [see test_list.py]'


print '6.6 Mappings == Dictionaries'
# calling built-in types without argument must return empty
if dict() != {}: raise TestFailed,'dict() does not return {}'
d = {}
if d.keys() != []: raise TestFailed, '{}.keys()'
if d.values() != []: raise TestFailed, '{}.values()'
if d.items() != []: raise TestFailed, '{}.items()'
if d.has_key('a') != 0: raise TestFailed, '{}.has_key(\'a\')'
if ('a' in d) != 0: raise TestFailed, "'a' in {}"
if ('a' not in d) != 1: raise TestFailed, "'a' not in {}"
if len(d) != 0: raise TestFailed, 'len({})'
d = {'a': 1, 'b': 2}
if len(d) != 2: raise TestFailed, 'len(dict)'
k = d.keys()
k.sort()
if k != ['a', 'b']: raise TestFailed, 'dict keys()'
if d.has_key('a') and d.has_key('b') and not d.has_key('c'): pass
else: raise TestFailed, 'dict keys()'
if 'a' in d and 'b' in d and 'c' not in d: pass
else: raise TestFailed, 'dict keys() # in/not in version'
if d['a'] != 1 or d['b'] != 2: raise TestFailed, 'dict item'
d['c'] = 3
d['a'] = 4
if d['c'] != 3 or d['a'] != 4: raise TestFailed, 'dict item assignment'
del d['b']
if d != {'a': 4, 'c': 3}: raise TestFailed, 'dict item deletion'
# dict.clear()
d = {1:1, 2:2, 3:3}
d.clear()
if d != {}: raise TestFailed, 'dict clear'
# dict.update()
d.update({1:100})
d.update({2:20})
d.update({1:1, 2:2, 3:3})
if d != {1:1, 2:2, 3:3}: raise TestFailed, 'dict update'
d.clear()
try: d.update(None)
except AttributeError: pass
else: raise TestFailed, 'dict.update(None), AttributeError expected'
class SimpleUserDict:
    def __init__(self):
        self.d = {1:1, 2:2, 3:3}
    def keys(self):
        return self.d.keys()
    def __getitem__(self, i):
        return self.d[i]
d.update(SimpleUserDict())
if d != {1:1, 2:2, 3:3}: raise TestFailed, 'dict.update(instance)'
d.clear()
class FailingUserDict:
    def keys(self):
        raise ValueError
try: d.update(FailingUserDict())
except ValueError: pass
else: raise TestFailed, 'dict.keys() expected ValueError'
class FailingUserDict:
    def keys(self):
        class BogonIter:
            def __iter__(self):
                raise ValueError
        return BogonIter()
try: d.update(FailingUserDict())
except ValueError: pass
else: raise TestFailed, 'iter(dict.keys()) expected ValueError'
class FailingUserDict:
    def keys(self):
        class BogonIter:
            def __init__(self):
                self.i = 1
            def __iter__(self):
                return self
            def next(self):
                if self.i:
                    self.i = 0
                    return 'a'
                raise ValueError
        return BogonIter()
    def __getitem__(self, key):
        return key
try: d.update(FailingUserDict())
except ValueError: pass
else: raise TestFailed, 'iter(dict.keys()).next() expected ValueError'
class FailingUserDict:
    def keys(self):
        class BogonIter:
            def __init__(self):
                self.i = ord('a')
            def __iter__(self):
                return self
            def next(self):
                if self.i <= ord('z'):
                    rtn = chr(self.i)
                    self.i += 1
                    return rtn
                raise StopIteration
        return BogonIter()
    def __getitem__(self, key):
        raise ValueError
try: d.update(FailingUserDict())
except ValueError: pass
else: raise TestFailed, 'dict.update(), __getitem__ expected ValueError'
# dict.fromkeys()
if dict.fromkeys('abc') != {'a':None, 'b':None, 'c':None}:
    raise TestFailed, 'dict.fromkeys did not work as a class method'
d = {}
if d.fromkeys('abc') is d:
    raise TestFailed, 'dict.fromkeys did not return a new dict'
if d.fromkeys('abc') != {'a':None, 'b':None, 'c':None}:
    raise TestFailed, 'dict.fromkeys failed with default value'
if d.fromkeys((4,5),0) != {4:0, 5:0}:
    raise TestFailed, 'dict.fromkeys failed with specified value'
if d.fromkeys([]) != {}:
    raise TestFailed, 'dict.fromkeys failed with null sequence'
def g():
    yield 1
if d.fromkeys(g()) != {1:None}:
    raise TestFailed, 'dict.fromkeys failed with a generator'
try: {}.fromkeys(3)
except TypeError: pass
else: raise TestFailed, 'dict.fromkeys failed to raise TypeError'
class dictlike(dict): pass
if dictlike.fromkeys('a') != {'a':None}:
    raise TestFailed, 'dictsubclass.fromkeys did not inherit'
if dictlike().fromkeys('a') != {'a':None}:
    raise TestFailed, 'dictsubclass.fromkeys did not inherit'
if type(dictlike.fromkeys('a')) is not dictlike:
    raise TestFailed, 'dictsubclass.fromkeys created wrong type'
if type(dictlike().fromkeys('a')) is not dictlike:
    raise TestFailed, 'dictsubclass.fromkeys created wrong type'
from UserDict import UserDict
class mydict(dict):
    def __new__(cls):
        return UserDict()
ud = mydict.fromkeys('ab')
if ud != {'a':None, 'b':None} or not isinstance(ud,UserDict):
    raise TestFailed, 'fromkeys did not instantiate using  __new__'
# dict.copy()
d = {1:1, 2:2, 3:3}
if d.copy() != {1:1, 2:2, 3:3}: raise TestFailed, 'dict copy'
if {}.copy() != {}: raise TestFailed, 'empty dict copy'
# dict.get()
d = {}
if d.get('c') is not None: raise TestFailed, 'missing {} get, no 2nd arg'
if d.get('c', 3) != 3: raise TestFailed, 'missing {} get, w/ 2nd arg'
d = {'a' : 1, 'b' : 2}
if d.get('c') is not None: raise TestFailed, 'missing dict get, no 2nd arg'
if d.get('c', 3) != 3: raise TestFailed, 'missing dict get, w/ 2nd arg'
if d.get('a') != 1: raise TestFailed, 'present dict get, no 2nd arg'
if d.get('a', 3) != 1: raise TestFailed, 'present dict get, w/ 2nd arg'
# dict.setdefault()
d = {}
if d.setdefault('key0') is not None:
    raise TestFailed, 'missing {} setdefault, no 2nd arg'
if d.setdefault('key0') is not None:
    raise TestFailed, 'present {} setdefault, no 2nd arg'
d.setdefault('key', []).append(3)
if d['key'][0] != 3:
    raise TestFailed, 'missing {} setdefault, w/ 2nd arg'
d.setdefault('key', []).append(4)
if len(d['key']) != 2:
    raise TestFailed, 'present {} setdefault, w/ 2nd arg'
# dict.popitem()
for copymode in -1, +1:
    # -1: b has same structure as a
    # +1: b is a.copy()
    for log2size in range(12):
        size = 2**log2size
        a = {}
        b = {}
        for i in range(size):
            a[repr(i)] = i
            if copymode < 0:
                b[repr(i)] = i
        if copymode > 0:
            b = a.copy()
        for i in range(size):
            ka, va = ta = a.popitem()
            if va != int(ka): raise TestFailed, "a.popitem: %s" % str(ta)
            kb, vb = tb = b.popitem()
            if vb != int(kb): raise TestFailed, "b.popitem: %s" % str(tb)
            if copymode < 0 and ta != tb:
                raise TestFailed, "a.popitem != b.popitem: %s, %s" % (
                    str(ta), str(tb))
        if a: raise TestFailed, 'a not empty after popitems: %s' % str(a)
        if b: raise TestFailed, 'b not empty after popitems: %s' % str(b)

d.clear()
try: d.popitem()
except KeyError: pass
else: raise TestFailed, "{}.popitem doesn't raise KeyError"

# Tests for pop with specified key
d.clear()
k, v = 'abc', 'def'
d[k] = v
try: d.pop('ghi')
except KeyError: pass
else: raise TestFailed, "{}.pop(k) doesn't raise KeyError when k not in dictionary"

if d.pop(k) != v: raise TestFailed, "{}.pop(k) doesn't find known key/value pair"
if len(d) > 0: raise TestFailed, "{}.pop(k) failed to remove the specified pair"

try: d.pop(k)
except KeyError: pass
else: raise TestFailed, "{}.pop(k) doesn't raise KeyError when dictionary is empty"

# verify longs/ints get same value when key > 32 bits (for 64-bit archs)
# see SF bug #689659
x = 4503599627370496L
y = 4503599627370496
h = {x: 'anything', y: 'something else'}
if h[x] != h[y]:
    raise TestFailed, "long/int key should match"

if d.pop(k, v) != v: raise TestFailed, "{}.pop(k, v) doesn't return default value"
d[k] = v
if d.pop(k, 1) != v: raise TestFailed, "{}.pop(k, v) doesn't find known key/value pair"

d[1] = 1
try:
    for i in d:
        d[i+1] = 1
except RuntimeError:
    pass
else:
    raise TestFailed, "changing dict size during iteration doesn't raise Error"

try: type(1, 2)
except TypeError: pass
else: raise TestFailed, 'type(), w/2 args expected TypeError'

try: type(1, 2, 3, 4)
except TypeError: pass
else: raise TestFailed, 'type(), w/4 args expected TypeError'

print 'Buffers'
try: buffer('asdf', -1)
except ValueError: pass
else: raise TestFailed, "buffer('asdf', -1) should raise ValueError"

try: buffer(None)
except TypeError: pass
else: raise TestFailed, "buffer(None) should raise TypeError"

a = buffer('asdf')
hash(a)
b = a * 5
if a == b:
    raise TestFailed, 'buffers should not be equal'
if str(b) != ('asdf' * 5):
    raise TestFailed, 'repeated buffer has wrong content'
if str(a * 0) != '':
    raise TestFailed, 'repeated buffer zero times has wrong content'
if str(a + buffer('def')) != 'asdfdef':
    raise TestFailed, 'concatenation of buffers yields wrong content'

try: a[1] = 'g'
except TypeError: pass
else: raise TestFailed, "buffer assignment should raise TypeError"

try: a[0:1] = 'g'
except TypeError: pass
else: raise TestFailed, "buffer slice assignment should raise TypeError"
