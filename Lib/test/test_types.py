# Python test set -- part 6, built-in types

from test_support import *

print '6. Built-in types'

print '6.1 Truth value testing'
if None: raise TestFailed, 'None is true instead of false'
if 0: raise TestFailed, '0 is true instead of false'
if 0L: raise TestFailed, '0L is true instead of false'
if 0.0: raise TestFailed, '0.0 is true instead of false'
if '': raise TestFailed, '\'\' is true instead of false'
if (): raise TestFailed, '() is true instead of false'
if []: raise TestFailed, '[] is true instead of false'
if {}: raise TestFailed, '{} is true instead of false'
if not 1: raise TestFailed, '1 is false instead of true'
if not 1L: raise TestFailed, '1L is false instead of true'
if not 1.0: raise TestFailed, '1.0 is false instead of true'
if not 'x': raise TestFailed, '\'x\' is false instead of true'
if not (1, 1): raise TestFailed, '(1, 1) is false instead of true'
if not [1]: raise TestFailed, '[1] is false instead of true'
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
else: raise TestFailed, '1 and 1 is false instead of false'
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
if 0 in [0] and 0 not in [1]: pass
else: raise TestFailed, 'membership test failed'
if None is None and [] is not []: pass
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
try: int(long(x)+1L)
except OverflowError: pass
else:raise TestFailed, 'long op'
x = -x
if int(long(x)) != x: raise TestFailed, 'long op'
x = x-1
if int(long(x)) != x: raise TestFailed, 'long op'
try: int(long(x)-1L)
except OverflowError: pass
else:raise TestFailed, 'long op'

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
    

print '6.5.2 Tuples'
if len(()) != 0: raise TestFailed, 'len(())'
if len((1,)) != 1: raise TestFailed, 'len((1,))'
if len((1,2,3,4,5,6)) != 6: raise TestFailed, 'len((1,2,3,4,5,6))'
if (1,2)+(3,4) != (1,2,3,4): raise TestFailed, 'tuple concatenation'
if (1,2)*3 != (1,2,1,2,1,2): raise TestFailed, 'tuple repetition *3'
if 0*(1,2,3) != (): raise TestFailed, 'tuple repetition 0*'
if min((1,2)) != 1 or max((1,2)) != 2: raise TestFailed, 'min/max tuple'
if 0 in (0,1,2) and 1 in (0,1,2) and 2 in (0,1,2) and 3 not in (0,1,2): pass
else: raise TestFailed, 'in/not in tuple'
try: ()[0]
except IndexError: pass
else: raise TestFailed, "tuple index error didn't raise IndexError"
x = ()
x += ()
if x != (): raise TestFailed, 'tuple inplace add from () to () failed'
x += (1,)
if x != (1,): raise TestFailed, 'tuple resize from () failed'

# extended slicing - subscript only for tuples
a = (0,1,2,3,4)
vereq(a[::], a)
vereq(a[::2], (0,2,4))
vereq(a[1::2], (1,3))
vereq(a[::-1], (4,3,2,1,0))
vereq(a[::-2], (4,2,0))
vereq(a[3::-2], (3,1))
vereq(a[-100:100:], a)
vereq(a[100:-100:-1], a[::-1])
vereq(a[-100L:100L:2L], (0,2,4))

# Check that a specific bug in _PyTuple_Resize() is squashed.
def f():
    for i in range(1000):
        yield i
vereq(list(tuple(f())), range(1000))

print '6.5.3 Lists'
if len([]) != 0: raise TestFailed, 'len([])'
if len([1,]) != 1: raise TestFailed, 'len([1,])'
if len([1,2,3,4,5,6]) != 6: raise TestFailed, 'len([1,2,3,4,5,6])'
if [1,2]+[3,4] != [1,2,3,4]: raise TestFailed, 'list concatenation'
if [1,2]*3 != [1,2,1,2,1,2]: raise TestFailed, 'list repetition *3'
if [1,2]*3L != [1,2,1,2,1,2]: raise TestFailed, 'list repetition *3L'
if 0*[1,2,3] != []: raise TestFailed, 'list repetition 0*'
if 0L*[1,2,3] != []: raise TestFailed, 'list repetition 0L*'
if min([1,2]) != 1 or max([1,2]) != 2: raise TestFailed, 'min/max list'
if 0 in [0,1,2] and 1 in [0,1,2] and 2 in [0,1,2] and 3 not in [0,1,2]: pass
else: raise TestFailed, 'in/not in list'
a = [1, 2, 3, 4, 5]
a[:-1] = a
if a != [1, 2, 3, 4, 5, 5]:
    raise TestFailed, "list self-slice-assign (head)"
a = [1, 2, 3, 4, 5]
a[1:] = a
if a != [1, 1, 2, 3, 4, 5]:
    raise TestFailed, "list self-slice-assign (tail)"
a = [1, 2, 3, 4, 5]
a[1:-1] = a
if a != [1, 1, 2, 3, 4, 5, 5]:
    raise TestFailed, "list self-slice-assign (center)"
try: [][0]
except IndexError: pass
else: raise TestFailed, "list index error didn't raise IndexError"
try: [][0] = 5
except IndexError: pass
else: raise TestFailed, "list assignment index error didn't raise IndexError"
try: [].pop()
except IndexError: pass
else: raise TestFailed, "empty list.pop() didn't raise IndexError"
try: [1].pop(5)
except IndexError: pass
else: raise TestFailed, "[1].pop(5) didn't raise IndexError"
try: [][0:1] = 5
except TypeError: pass
else: raise TestFailed, "bad list slice assignment didn't raise TypeError"
try: [].extend(None)
except TypeError: pass
else: raise TestFailed, "list.extend(None) didn't raise TypeError"
a = [1, 2, 3, 4]
a *= 0
if a != []:
    raise TestFailed, "list inplace repeat"


print '6.5.3a Additional list operations'
a = [0,1,2,3,4]
a[0L] = 1
a[1L] = 2
a[2L] = 3
if a != [1,2,3,3,4]: raise TestFailed, 'list item assignment [0L], [1L], [2L]'
a[0] = 5
a[1] = 6
a[2] = 7
if a != [5,6,7,3,4]: raise TestFailed, 'list item assignment [0], [1], [2]'
a[-2L] = 88
a[-1L] = 99
if a != [5,6,7,88,99]: raise TestFailed, 'list item assignment [-2L], [-1L]'
a[-2] = 8
a[-1] = 9
if a != [5,6,7,8,9]: raise TestFailed, 'list item assignment [-2], [-1]'
a[:2] = [0,4]
a[-3:] = []
a[1:1] = [1,2,3]
if a != [0,1,2,3,4]: raise TestFailed, 'list slice assignment'
a[ 1L : 4L] = [7,8,9]
if a != [0,7,8,9,4]: raise TestFailed, 'list slice assignment using long ints'
del a[1:4]
if a != [0,4]: raise TestFailed, 'list slice deletion'
del a[0]
if a != [4]: raise TestFailed, 'list item deletion [0]'
del a[-1]
if a != []: raise TestFailed, 'list item deletion [-1]'
a=range(0,5)
del a[1L:4L]
if a != [0,4]: raise TestFailed, 'list slice deletion'
del a[0L]
if a != [4]: raise TestFailed, 'list item deletion [0]'
del a[-1L]
if a != []: raise TestFailed, 'list item deletion [-1]'
a.append(0)
a.append(1)
a.append(2)
if a != [0,1,2]: raise TestFailed, 'list append'
a.insert(0, -2)
a.insert(1, -1)
a.insert(2,0)
if a != [-2,-1,0,0,1,2]: raise TestFailed, 'list insert'
if a.count(0) != 2: raise TestFailed, ' list count'
if a.index(0) != 2: raise TestFailed, 'list index'
a.remove(0)
if a != [-2,-1,0,1,2]: raise TestFailed, 'list remove'
a.reverse()
if a != [2,1,0,-1,-2]: raise TestFailed, 'list reverse'
a.sort()
if a != [-2,-1,0,1,2]: raise TestFailed, 'list sort'
def revcmp(a, b): return cmp(b, a)
a.sort(revcmp)
if a != [2,1,0,-1,-2]: raise TestFailed, 'list sort with cmp func'
# The following dumps core in unpatched Python 1.5:
def myComparison(x,y):
    return cmp(x%3, y%7)
z = range(12)
z.sort(myComparison)

try: z.sort(2)
except TypeError: pass
else: raise TestFailed, 'list sort compare function is not callable'

def selfmodifyingComparison(x,y):
    z[0] = 1
    return cmp(x, y)
try: z.sort(selfmodifyingComparison)
except TypeError: pass
else: raise TestFailed, 'modifying list during sort'

try: z.sort(lambda x, y: 's')
except TypeError: pass
else: raise TestFailed, 'list sort compare function does not return int'

# Test extreme cases with long ints
a = [0,1,2,3,4]
if a[ -pow(2,128L): 3 ] != [0,1,2]:
    raise TestFailed, "list slicing with too-small long integer"
if a[ 3: pow(2,145L) ] != [3,4]:
    raise TestFailed, "list slicing with too-large long integer"


# extended slicing

#  subscript
a = [0,1,2,3,4]
vereq(a[::], a)
vereq(a[::2], [0,2,4])
vereq(a[1::2], [1,3])
vereq(a[::-1], [4,3,2,1,0])
vereq(a[::-2], [4,2,0])
vereq(a[3::-2], [3,1])
vereq(a[-100:100:], a)
vereq(a[100:-100:-1], a[::-1])
vereq(a[-100L:100L:2L], [0,2,4])
vereq(a[1000:2000:2], [])
vereq(a[-1000:-2000:-2], [])
#  deletion
del a[::2]
vereq(a, [1,3])
a = range(5)
del a[1::2]
vereq(a, [0,2,4])
a = range(5)
del a[1::-2]
vereq(a, [0,2,3,4])
#  assignment
a = range(10)
a[::2] = [-1]*5
vereq(a, [-1, 1, -1, 3, -1, 5, -1, 7, -1, 9])
a = range(10)
a[::-4] = [10]*3
vereq(a, [0, 10, 2, 3, 4, 10, 6, 7, 8 ,10])
a = range(4)
a[::-1] = a
vereq(a, [3, 2, 1, 0])
a = range(10)
b = a[:]
c = a[:]
a[2:3] = ["two", "elements"]
b[slice(2,3)] = ["two", "elements"]
c[2:3:] = ["two", "elements"]
vereq(a, b)
vereq(a, c)

print '6.6 Mappings == Dictionaries'
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
            a[`i`] = i
            if copymode < 0:
                b[`i`] = i
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
