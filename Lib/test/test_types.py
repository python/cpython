# Python test set -- part 6, built-in types

from test.test_support import *

print '6. Built-in types'

print '6.1 Truth value testing'
if None: raise TestFailed, 'None is true instead of false'
if 0: raise TestFailed, '0 is true instead of false'
if 0L: raise TestFailed, '0L is true instead of false'
if 0.0: raise TestFailed, '0.0 is true instead of false'
if '': raise TestFailed, '\'\' is true instead of false'
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
# Ensure the first 256 integers are shared
a = 256
b = 128*2
if a is not b: raise TestFailed, '256 is not shared'
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

print '6.6 Mappings == Dictionaries [see test_dict.py]'


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
cmp(buffer("abc"), buffer("def")) # used to raise a warning: tp_compare didn't return -1, 0, or 1

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
if str(buffer(a)) != 'asdf':
    raise TestFailed, 'composing buffers failed'
if str(buffer(a, 2)) != 'df':
    raise TestFailed, 'specifying buffer offset failed'
if str(buffer(a, 0, 2)) != 'as':
    raise TestFailed, 'specifying buffer size failed'
if str(buffer(a, 1, 2)) != 'sd':
    raise TestFailed, 'specifying buffer offset and size failed'
try: buffer(buffer('asdf', 1), -1)
except ValueError: pass
else: raise TestFailed, "buffer(buffer('asdf', 1), -1) should raise ValueError"
if str(buffer(buffer('asdf', 0, 2), 0)) != 'as':
    raise TestFailed, 'composing length-specified buffer failed'
if str(buffer(buffer('asdf', 0, 2), 0, 5000)) != 'as':
    raise TestFailed, 'composing length-specified buffer failed'
if str(buffer(buffer('asdf', 0, 2), 0, -1)) != 'as':
    raise TestFailed, 'composing length-specified buffer failed'
if str(buffer(buffer('asdf', 0, 2), 1, 2)) != 's':
    raise TestFailed, 'composing length-specified buffer failed'

try: a[1] = 'g'
except TypeError: pass
else: raise TestFailed, "buffer assignment should raise TypeError"

try: a[0:1] = 'g'
except TypeError: pass
else: raise TestFailed, "buffer slice assignment should raise TypeError"

# array.array() returns an object that does not implement a char buffer,
# something which int() uses for conversion.
import array
try: int(buffer(array.array('c')))
except TypeError :pass
else: raise TestFailed, "char buffer (at C level) not working"
