# Python test set -- part 4a, built-in functions a-m

from test_support import *

print 'abs'
if abs(0) <> 0: raise TestFailed, 'abs(0)'
if abs(1234) <> 1234: raise TestFailed, 'abs(1234)'
if abs(-1234) <> 1234: raise TestFailed, 'abs(-1234)'
#
if abs(0.0) <> 0.0: raise TestFailed, 'abs(0.0)'
if abs(3.14) <> 3.14: raise TestFailed, 'abs(3.14)'
if abs(-3.14) <> 3.14: raise TestFailed, 'abs(-3.14)'
#
if abs(0L) <> 0L: raise TestFailed, 'abs(0L)'
if abs(1234L) <> 1234L: raise TestFailed, 'abs(1234L)'
if abs(-1234L) <> 1234L: raise TestFailed, 'abs(-1234L)'

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

print 'chr'
if chr(32) <> ' ': raise TestFailed, 'chr(32)'
if chr(65) <> 'A': raise TestFailed, 'chr(65)'
if chr(97) <> 'a': raise TestFailed, 'chr(97)'

print 'cmp'
if cmp(-1, 1) <> -1: raise TestFailed, 'cmp(-1, 1)'
if cmp(1, -1) <> 1: raise TestFailed, 'cmp(1, -1)'
if cmp(1, 1) <> 0: raise TestFailed, 'cmp(1, 1)'

print 'coerce'
if fcmp(coerce(1, 1.1), (1.0, 1.1)): raise TestFailed, 'coerce(1, 1.1)'
if coerce(1, 1L) <> (1L, 1L): raise TestFailed, 'coerce(1, 1L)'
if fcmp(coerce(1L, 1.1), (1.0, 1.1)): raise TestFailed, 'coerce(1L, 1.1)'

print 'dir'
x = 1
if 'x' not in dir(): raise TestFailed, 'dir()'
import sys
if 'modules' not in dir(sys): raise TestFailed, 'dir(sys)'

print 'divmod'
if divmod(12, 7) <> (1, 5): raise TestFailed, 'divmod(12, 7)'
if divmod(-12, 7) <> (-2, 2): raise TestFailed, 'divmod(-12, 7)'
if divmod(12, -7) <> (-2, -2): raise TestFailed, 'divmod(12, -7)'
if divmod(-12, -7) <> (1, -5): raise TestFailed, 'divmod(-12, -7)'
#
if divmod(12L, 7L) <> (1L, 5L): raise TestFailed, 'divmod(12L, 7L)'
if divmod(-12L, 7L) <> (-2L, 2L): raise TestFailed, 'divmod(-12L, 7L)'
if divmod(12L, -7L) <> (-2L, -2L): raise TestFailed, 'divmod(12L, -7L)'
if divmod(-12L, -7L) <> (1L, -5L): raise TestFailed, 'divmod(-12L, -7L)'
#
if divmod(12, 7L) <> (1, 5L): raise TestFailed, 'divmod(12, 7L)'
if divmod(-12, 7L) <> (-2, 2L): raise TestFailed, 'divmod(-12, 7L)'
if divmod(12L, -7) <> (-2L, -2): raise TestFailed, 'divmod(12L, -7)'
if divmod(-12L, -7) <> (1L, -5): raise TestFailed, 'divmod(-12L, -7)'
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
if eval('1+1') <> 2: raise TestFailed, 'eval(\'1+1\')'
if eval(' 1+1\n') <> 2: raise TestFailed, 'eval(\' 1+1\\n\')'

print 'execfile'
z = 0
f = open(TESTFN, 'w')
f.write('z = z+1\n')
f.write('z = z*2\n')
f.close()
execfile(TESTFN)
unlink(TESTFN)

print 'float'
if float(3.14) <> 3.14: raise TestFailed, 'float(3.14)'
if float(314) <> 314.0: raise TestFailed, 'float(314)'
if float(314L) <> 314.0: raise TestFailed, 'float(314L)'

print 'getattr'
import sys
if getattr(sys, 'stdout') is not sys.stdout: raise TestFailed, 'getattr'

print 'hex'
if hex(16) != '0x10': raise TestFailed, 'hex(16)'
if hex(16L) != '0x10L': raise TestFailed, 'hex(16L)'
if hex(-16) != '-0x10': raise TestFailed, 'hex(-16)'
if hex(-16L) != '-0x10L': raise TestFailed, 'hex(-16L)'

# Test input() later, together with raw_input

print 'int'
if int(314) <> 314: raise TestFailed, 'int(314)'
if int(3.14) <> 3: raise TestFailed, 'int(3.14)'
if int(314L) <> 314: raise TestFailed, 'int(314L)'

print 'len'
if len('123') <> 3: raise TestFailed, 'len(\'123\')'
if len(()) <> 0: raise TestFailed, 'len(())'
if len((1, 2, 3, 4)) <> 4: raise TestFailed, 'len((1, 2, 3, 4))'
if len([1, 2, 3, 4]) <> 4: raise TestFailed, 'len([1, 2, 3, 4])'
if len({}) <> 0: raise TestFailed, 'len({})'
if len({'a':1, 'b': 2}) <> 2: raise TestFailed, 'len({\'a\':1, \'b\': 2})'

print 'long'
if long(314) <> 314L: raise TestFailed, 'long(314)'
if long(3.14) <> 3L: raise TestFailed, 'long(3.14)'
if long(314L) <> 314L: raise TestFailed, 'long(314L)'

print 'max'
if max('123123') <> '3': raise TestFailed, 'max(\'123123\')'
if max(1, 2, 3) <> 3: raise TestFailed, 'max(1, 2, 3)'
if max((1, 2, 3, 1, 2, 3)) <> 3: raise TestFailed, 'max((1, 2, 3, 1, 2, 3))'
if max([1, 2, 3, 1, 2, 3]) <> 3: raise TestFailed, 'max([1, 2, 3, 1, 2, 3])'
#
if max(1, 2L, 3.0) <> 3.0: raise TestFailed, 'max(1, 2L, 3.0)'
if max(1L, 2.0, 3) <> 3: raise TestFailed, 'max(1L, 2.0, 3)'
if max(1.0, 2, 3L) <> 3L: raise TestFailed, 'max(1.0, 2, 3L)'

print 'min'
if min('123123') <> '1': raise TestFailed, 'min(\'123123\')'
if min(1, 2, 3) <> 1: raise TestFailed, 'min(1, 2, 3)'
if min((1, 2, 3, 1, 2, 3)) <> 1: raise TestFailed, 'min((1, 2, 3, 1, 2, 3))'
if min([1, 2, 3, 1, 2, 3]) <> 1: raise TestFailed, 'min([1, 2, 3, 1, 2, 3])'
#
if min(1, 2L, 3.0) <> 1: raise TestFailed, 'min(1, 2L, 3.0)'
if min(1L, 2.0, 3) <> 1L: raise TestFailed, 'min(1L, 2.0, 3)'
if min(1.0, 2, 3L) <> 1.0: raise TestFailed, 'min(1.0, 2, 3L)'
