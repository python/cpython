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
if 0 < 1 <= 1 == 1 >= 1 > 0 <> 1: pass
else: raise TestFailed, 'int comparisons failed'
if 0L < 1L <= 1L == 1L >= 1L > 0L <> 1L: pass
else: raise TestFailed, 'long int comparisons failed'
if 0.0 < 1.0 <= 1.0 == 1.0 >= 1.0 > 0.0 <> 1.0: pass
else: raise TestFailed, 'float comparisons failed'
if '' < 'a' <= 'a' == 'a' < 'abc' < 'abd' < 'b': pass
else: raise TestFailed, 'string comparisons failed'
if 0 in [0] and 0 not in [1]: pass
else: raise TestFailed, 'membership test failed'
if None is None and [] is not []: pass
else: raise TestFailed, 'identity test failed'

print '6.4 Numeric types (mostly conversions)'
if 0 <> 0L or 0 <> 0.0 or 0L <> 0.0: raise TestFailed, 'mixed comparisons'
if 1 <> 1L or 1 <> 1.0 or 1L <> 1.0: raise TestFailed, 'mixed comparisons'
if -1 <> -1L or -1 <> -1.0 or -1L <> -1.0:
	raise TestFailed, 'int/long/float value not equal'
if int(1.9) == 1 == int(1.1) and int(-1.1) == -1 == int(-1.9): pass
else: raise TestFailed, 'int() does not round properly'
if long(1.9) == 1L == long(1.1) and long(-1.1) == -1L == long(-1.9): pass
else: raise TestFailed, 'long() does not round properly'
if float(1) == 1.0 and float(-1) == -1.0 and float(0) == 0.0: pass
else: raise TestFailed, 'float() does not work properly'
print '6.4.1 32-bit integers'
if 12 + 24 <> 36: raise TestFailed, 'int op'
if 12 + (-24) <> -12: raise TestFailed, 'int op'
if (-12) + 24 <> 12: raise TestFailed, 'int op'
if (-12) + (-24) <> -36: raise TestFailed, 'int op'
if not 12 < 24: raise TestFailed, 'int op'
if not -24 < -12: raise TestFailed, 'int op'
# Test for a particular bug in integer multiply
xsize, ysize, zsize = 238, 356, 4
if not (xsize*ysize*zsize == zsize*xsize*ysize == 338912):
	raise TestFailed, 'int mul commutativity'
print '6.4.2 Long integers'
if 12L + 24L <> 36L: raise TestFailed, 'long op'
if 12L + (-24L) <> -12L: raise TestFailed, 'long op'
if (-12L) + 24L <> 12L: raise TestFailed, 'long op'
if (-12L) + (-24L) <> -36L: raise TestFailed, 'long op'
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
print '6.4.3 Floating point numbers'
if 12.0 + 24.0 <> 36.0: raise TestFailed, 'float op'
if 12.0 + (-24.0) <> -12.0: raise TestFailed, 'float op'
if (-12.0) + 24.0 <> 12.0: raise TestFailed, 'float op'
if (-12.0) + (-24.0) <> -36.0: raise TestFailed, 'float op'
if not 12.0 < 24.0: raise TestFailed, 'float op'
if not -24.0 < -12.0: raise TestFailed, 'float op'

print '6.5 Sequence types'

print '6.5.1 Strings'
if len('') <> 0: raise TestFailed, 'len(\'\')'
if len('a') <> 1: raise TestFailed, 'len(\'a\')'
if len('abcdef') <> 6: raise TestFailed, 'len(\'abcdef\')'
if 'xyz' + 'abcde' <> 'xyzabcde': raise TestFailed, 'string concatenation'
if 'xyz'*3 <> 'xyzxyzxyz': raise TestFailed, 'string repetition *3'
if 0*'abcde' <> '': raise TestFailed, 'string repetition 0*'
if min('abc') <> 'a' or max('abc') <> 'c': raise TestFailed, 'min/max string'
if 'a' in 'abc' and 'b' in 'abc' and 'c' in 'abc' and 'd' not in 'abc': pass
else: raise TestFailed, 'in/not in string'
x = 'x'*103
if '%s!'%x != x+'!': raise TestFailed, 'nasty string formatting bug'

print '6.5.2 Tuples'
if len(()) <> 0: raise TestFailed, 'len(())'
if len((1,)) <> 1: raise TestFailed, 'len((1,))'
if len((1,2,3,4,5,6)) <> 6: raise TestFailed, 'len((1,2,3,4,5,6))'
if (1,2)+(3,4) <> (1,2,3,4): raise TestFailed, 'tuple concatenation'
if (1,2)*3 <> (1,2,1,2,1,2): raise TestFailed, 'tuple repetition *3'
if 0*(1,2,3) <> (): raise TestFailed, 'tuple repetition 0*'
if min((1,2)) <> 1 or max((1,2)) <> 2: raise TestFailed, 'min/max tuple'
if 0 in (0,1,2) and 1 in (0,1,2) and 2 in (0,1,2) and 3 not in (0,1,2): pass
else: raise TestFailed, 'in/not in tuple'

print '6.5.3 Lists'
if len([]) <> 0: raise TestFailed, 'len([])'
if len([1,]) <> 1: raise TestFailed, 'len([1,])'
if len([1,2,3,4,5,6]) <> 6: raise TestFailed, 'len([1,2,3,4,5,6])'
if [1,2]+[3,4] <> [1,2,3,4]: raise TestFailed, 'list concatenation'
if [1,2]*3 <> [1,2,1,2,1,2]: raise TestFailed, 'list repetition *3'
if [1,2]*3L <> [1,2,1,2,1,2]: raise TestFailed, 'list repetition *3L'
if 0*[1,2,3] <> []: raise TestFailed, 'list repetition 0*'
if 0L*[1,2,3] <> []: raise TestFailed, 'list repetition 0L*'
if min([1,2]) <> 1 or max([1,2]) <> 2: raise TestFailed, 'min/max list'
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


print '6.5.3a Additional list operations'
a = [0,1,2,3,4]
a[0L] = 1
a[1L] = 2
a[2L] = 3
if a <> [1,2,3,3,4]: raise TestFailed, 'list item assignment [0L], [1L], [2L]'
a[0] = 5
a[1] = 6
a[2] = 7
if a <> [5,6,7,3,4]: raise TestFailed, 'list item assignment [0], [1], [2]'
a[-2L] = 88
a[-1L] = 99
if a <> [5,6,7,88,99]: raise TestFailed, 'list item assignment [-2L], [-1L]'
a[-2] = 8
a[-1] = 9
if a <> [5,6,7,8,9]: raise TestFailed, 'list item assignment [-2], [-1]'
a[:2] = [0,4]
a[-3:] = []
a[1:1] = [1,2,3]
if a <> [0,1,2,3,4]: raise TestFailed, 'list slice assignment'
a[ 1L : 4L] = [7,8,9]
if a <> [0,7,8,9,4]: raise TestFailed, 'list slice assignment using long ints'
del a[1:4]
if a <> [0,4]: raise TestFailed, 'list slice deletion'
del a[0]
if a <> [4]: raise TestFailed, 'list item deletion [0]'
del a[-1]
if a <> []: raise TestFailed, 'list item deletion [-1]'
a=range(0,5)
del a[1L:4L]
if a <> [0,4]: raise TestFailed, 'list slice deletion'
del a[0L]
if a <> [4]: raise TestFailed, 'list item deletion [0]'
del a[-1L]
if a <> []: raise TestFailed, 'list item deletion [-1]'
a.append(0)
a.append(1)
a.append(2)
if a <> [0,1,2]: raise TestFailed, 'list append'
a.insert(0, -2)
a.insert(1, -1)
a.insert(2,0)
if a <> [-2,-1,0,0,1,2]: raise TestFailed, 'list insert'
if a.count(0) <> 2: raise TestFailed, ' list count'
if a.index(0) <> 2: raise TestFailed, 'list index'
a.remove(0)
if a <> [-2,-1,0,1,2]: raise TestFailed, 'list remove'
a.reverse()
if a <> [2,1,0,-1,-2]: raise TestFailed, 'list reverse'
a.sort()
if a <> [-2,-1,0,1,2]: raise TestFailed, 'list sort'
def revcmp(a, b): return cmp(b, a)
a.sort(revcmp)
if a <> [2,1,0,-1,-2]: raise TestFailed, 'list sort with cmp func'
# The following dumps core in unpatched Python 1.5:
def myComparison(x,y):
    return cmp(x%3, y%7)
z = range(12)
z.sort(myComparison)

# Test extreme cases with long ints
a = [0,1,2,3,4]
if a[ -pow(2,128L): 3 ] != [0,1,2]: 
	raise TestFailed, "list slicing with too-small long integer"
if a[ 3: pow(2,145L) ] != [3,4]: 
	raise TestFailed, "list slicing with too-large long integer"

print '6.6 Mappings == Dictionaries'
d = {}
if d.keys() <> []: raise TestFailed, '{}.keys()'
if d.has_key('a') <> 0: raise TestFailed, '{}.has_key(\'a\')'
if len(d) <> 0: raise TestFailed, 'len({})'
d = {'a': 1, 'b': 2}
if len(d) <> 2: raise TestFailed, 'len(dict)'
k = d.keys()
k.sort()
if k <> ['a', 'b']: raise TestFailed, 'dict keys()'
if d.has_key('a') and d.has_key('b') and not d.has_key('c'): pass
else: raise TestFailed, 'dict keys()'
if d['a'] <> 1 or d['b'] <> 2: raise TestFailed, 'dict item'
d['c'] = 3
d['a'] = 4
if d['c'] <> 3 or d['a'] <> 4: raise TestFailed, 'dict item assignment'
del d['b']
if d <> {'a': 4, 'c': 3}: raise TestFailed, 'dict item deletion'
d = {1:1, 2:2, 3:3}
d.clear()
if d != {}: raise TestFailed, 'dict clear'
d.update({1:100})
d.update({2:20})
d.update({1:1, 2:2, 3:3})
if d != {1:1, 2:2, 3:3}: raise TestFailed, 'dict update'
if d.copy() != {1:1, 2:2, 3:3}: raise TestFailed, 'dict copy'
if {}.copy() != {}: raise TestFailed, 'empty dict copy'
# dict.get()
d = {}
if d.get('c') != None: raise TestFailed, 'missing {} get, no 2nd arg'
if d.get('c', 3) != 3: raise TestFailed, 'missing {} get, w/ 2nd arg'
d = {'a' : 1, 'b' : 2}
if d.get('c') != None: raise TestFailed, 'missing dict get, no 2nd arg'
if d.get('c', 3) != 3: raise TestFailed, 'missing dict get, w/ 2nd arg'
if d.get('a') != 1: raise TestFailed, 'present dict get, no 2nd arg'
if d.get('a', 3) != 1: raise TestFailed, 'present dict get, w/ 2nd arg'
# dict.setdefault()
d = {}
if d.setdefault('key0') <> None:
	raise TestFailed, 'missing {} setdefault, no 2nd arg'
if d.setdefault('key0') <> None:
	raise TestFailed, 'present {} setdefault, no 2nd arg'
d.setdefault('key', []).append(3)
if d['key'][0] <> 3:
	raise TestFailed, 'missing {} setdefault, w/ 2nd arg'
d.setdefault('key', []).append(4)
if len(d['key']) <> 2:
	raise TestFailed, 'present {} setdefault, w/ 2nd arg'
