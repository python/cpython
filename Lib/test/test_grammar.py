# Python test set -- part 1, grammar.
# This just tests whether the parser accepts them all.

from test_support import *

print '1. Parser'

print '1.1 Tokens'

print '1.1.1 Backslashes'

# Backslash means line continuation:
x = 1 \
+ 1
if x <> 2: raise TestFailed, 'backslash for line continuation'

# Backslash does not means continuation in comments :\
x = 0
if x <> 0: raise TestFailed, 'backslash ending comment'

print '1.1.2 Numeric literals'

print '1.1.2.1 Plain integers'
if 0xff <> 255: raise TestFailed, 'hex int'
if 0377 <> 255: raise TestFailed, 'octal int'
if  2147483647   != 017777777777: raise TestFailed, 'max positive int'
if -2147483647-1 != 020000000000: raise TestFailed, 'max negative int'
# XXX -2147483648
if 037777777777 != -1: raise TestFailed, 'oct -1'
if 0xffffffff != -1: raise TestFailed, 'hex -1'
for s in '2147483648', '040000000000', '0x100000000':
	try:
		x = eval(s)
	except OverflowError:
		continue
	raise TestFailed, 'No OverflowError on huge integer literal ' + `s`

print '1.1.2.2 Long integers'
x = 0L
x = 0l
x = 0xffffffffffffffffL
x = 0xffffffffffffffffl
x = 077777777777777777L
x = 077777777777777777l
x = 123456789012345678901234567890L
x = 123456789012345678901234567890l

print '1.1.2.3 Floating point'
x = 3.14
x = 314.
x = 0.314
# XXX x = 000.314
x = .314
x = 3e14
x = 3E14
x = 3e-14
x = 3e+14
x = 3.e14
x = .3e14
x = 3.1e4

print '1.2 Grammar'

print 'single_input' # NEWLINE | simple_stmt | compound_stmt NEWLINE
# XXX can't test in a script -- this rule is only used when interactive

print 'file_input' # (NEWLINE | stmt)* ENDMARKER
# Being tested as this very moment this very module

print 'expr_input' # testlist NEWLINE
# XXX Hard to test -- used only in calls to input()

print 'eval_input' # testlist ENDMARKER
x = eval('1, 0 or 1')

print 'funcdef'
### 'def' NAME parameters ':' suite
### parameters: '(' [varargslist] ')'
### varargslist: (fpdef ',')* '*' NAME | fpdef (',' fpdef)* [',']
### fpdef: NAME | '(' fplist ')'
### fplist: fpdef (',' fpdef)* [',']
def f1(): pass
def f2(one_argument): pass
def f3(two, arguments): pass
def f4(two, (compound, (argument, list))): pass
def a1(one_arg,): pass
def a2(two, args,): pass
def v0(*rest): pass
def v1(a, *rest): pass
def v2(a, b, *rest): pass
def v3(a, (b, c), *rest): pass

### stmt: simple_stmt | compound_stmt
# Tested below

### simple_stmt: small_stmt (';' small_stmt)* [';']
print 'simple_stmt'
x = 1; pass; del x

### small_stmt: expr_stmt | print_stmt  | pass_stmt | del_stmt | flow_stmt | import_stmt | global_stmt | access_stmt | exec_stmt
# Tested below

print 'expr_stmt' # (exprlist '=')* exprlist
1
1, 2, 3
x = 1
x = 1, 2, 3
x = y = z = 1, 2, 3
x, y, z = 1, 2, 3
abc = a, b, c = x, y, z = xyz = 1, 2, (3, 4)
# NB these variables are deleted below

print 'print_stmt' # 'print' (test ',')* [test]
print 1, 2, 3
print 1, 2, 3,
print
print 0 or 1, 0 or 1,
print 0 or 1

print 'del_stmt' # 'del' exprlist
del abc
del x, y, (z, xyz)

print 'pass_stmt' # 'pass'
pass

print 'flow_stmt' # break_stmt | continue_stmt | return_stmt | raise_stmt
# Tested below

print 'break_stmt' # 'break'
while 1: break

print 'continue_stmt' # 'continue'
i = 1
while i: i = 0; continue

print 'return_stmt' # 'return' [testlist]
def g1(): return
def g2(): return 1
g1()
x = g2()

print 'raise_stmt' # 'raise' test [',' test]
try: raise RuntimeError, 'just testing'
except RuntimeError: pass
try: raise KeyboardInterrupt
except KeyboardInterrupt: pass

print 'import_stmt' # 'import' NAME (',' NAME)* | 'from' NAME 'import' ('*' | NAME (',' NAME)*)
[1]
import sys
[2]
import time, math
[3]
from time import sleep
[4]
from sys import *
[5]
from math import sin, cos
[6]

print 'global_stmt' # 'global' NAME (',' NAME)*
def f():
	global a
	global a, b
	global one, two, three, four, five, six, seven, eight, nine, ten

print 'exec_stmt' # 'exec' expr ['in' expr [',' expr]]
def f():
	z = None
	del z
	exec 'z=1+1\n'
	if z <> 2: raise TestFailed, 'exec \'z=1+1\'\\n'
	del z
	exec 'z=1+1'
	if z <> 2: raise TestFailed, 'exec \'z=1+1\''
f()
g = {}
exec 'z = 1' in g
if g <> {'z': 1}: raise TestFailed, 'exec \'z = 1\' in g'
g = {}
l = {}
exec 'global a; a = 1; b = 2' in g, l
if (g, l) <> ({'a':1}, {'b':2}): raise TestFailed, 'exec ... in g, l'


### compound_stmt: if_stmt | while_stmt | for_stmt | try_stmt | funcdef | classdef
# Tested below

print 'if_stmt' # 'if' test ':' suite ('elif' test ':' suite)* ['else' ':' suite]
if 1: pass
if 1: pass
else: pass
if 0: pass
elif 0: pass
if 0: pass
elif 0: pass
elif 0: pass
elif 0: pass
else: pass

print 'while_stmt' # 'while' test ':' suite ['else' ':' suite]
while 0: pass
while 0: pass
else: pass

print 'for_stmt' # 'for' exprlist 'in' exprlist ':' suite ['else' ':' suite]
[1]
for i in 1, 2, 3: pass
[2]
for i, j, k in (): pass
else: pass
[3]

print 'try_stmt' # 'try' ':' suite (except_clause ':' suite)+ | 'try' ':' suite 'finally' ':' suite
### except_clause: 'except' [expr [',' expr]]
try:
	1/0
except ZeroDivisionError:
	pass
try: 1/0
except EOFError: pass
except TypeError, msg: pass
except RuntimeError, msg: pass
except: pass
try: 1/0
except (EOFError, TypeError, ZeroDivisionError): pass
try: 1/0
except (EOFError, TypeError, ZeroDivisionError), msg: pass
try: pass
finally: pass

print 'suite' # simple_stmt | NEWLINE INDENT NEWLINE* (stmt NEWLINE*)+ DEDENT
if 1: pass
if 1:
	pass
if 1:
	#
	#
	#
	pass
	pass
	#
	pass
	#

print 'test'
### and_test ('or' and_test)*
### and_test: not_test ('and' not_test)*
### not_test: 'not' not_test | comparison
if not 1: pass
if 1 and 1: pass
if 1 or 1: pass
if not not not 1: pass
if not 1 and 1 and 1: pass
if 1 and 1 or 1 and 1 and 1 or not 1 and 1: pass

print 'comparison'
### comparison: expr (comp_op expr)*
### comp_op: '<'|'>'|'=='|'>='|'<='|'<>'|'!='|'in'|'not' 'in'|'is'|'is' 'not'
if 1: pass
x = (1 == 1)
if 1 == 1: pass
if 1 != 1: pass
if 1 <> 1: pass
if 1 < 1: pass
if 1 > 1: pass
if 1 <= 1: pass
if 1 >= 1: pass
if 1 is 1: pass
if 1 is not 1: pass
if 1 in (): pass
if 1 not in (): pass
if 1 < 1 > 1 == 1 >= 1 <= 1 <> 1 != 1 in 1 not in 1 is 1 is not 1: pass

print 'binary mask ops'
x = 1 & 1
x = 1 ^ 1
x = 1 | 1

print 'shift ops'
x = 1 << 1
x = 1 >> 1
x = 1 << 1 >> 1

print 'additive ops'
x = 1
x = 1 + 1
x = 1 - 1 - 1
x = 1 - 1 + 1 - 1 + 1

print 'multiplicative ops'
x = 1 * 1
x = 1 / 1
x = 1 % 1
x = 1 / 1 * 1 % 1

print 'unary ops'
x = +1
x = -1
x = ~1
x = ~1 ^ 1 & 1 | 1 & 1 ^ -1
x = -1*1/1 + 1*1 - ---1*1

print 'selectors'
### trailer: '(' [testlist] ')' | '[' subscript ']' | '.' NAME
### subscript: expr | [expr] ':' [expr]
f1()
f2(1)
f2(1,)
f3(1, 2)
f3(1, 2,)
f4(1, (2, (3, 4)))
v0()
v0(1)
v0(1,)
v0(1,2)
v0(1,2,3,4,5,6,7,8,9,0)
v1(1)
v1(1,)
v1(1,2)
v1(1,2,3)
v1(1,2,3,4,5,6,7,8,9,0)
v2(1,2)
v2(1,2,3)
v2(1,2,3,4)
v2(1,2,3,4,5,6,7,8,9,0)
v3(1,(2,3))
v3(1,(2,3),4)
v3(1,(2,3),4,5,6,7,8,9,0)
import sys, time
c = sys.path[0]
x = time.time()
x = sys.modules['time'].time()
a = '01234'
c = a[0]
c = a[-1]
s = a[0:5]
s = a[:5]
s = a[0:]
s = a[:]
s = a[-5:]
s = a[:-1]
s = a[-4:-3]

print 'atoms'
### atom: '(' [testlist] ')' | '[' [testlist] ']' | '{' [dictmaker] '}' | '`' testlist '`' | NAME | NUMBER | STRING
### dictmaker: test ':' test (',' test ':' test)* [',']

x = (1)
x = (1 or 2 or 3)
x = (1 or 2 or 3, 2, 3)

x = []
x = [1]
x = [1 or 2 or 3]
x = [1 or 2 or 3, 2, 3]
x = []

x = {}
x = {'one': 1}
x = {'one': 1,}
x = {'one' or 'two': 1 or 2}
x = {'one': 1, 'two': 2}
x = {'one': 1, 'two': 2,}
x = {'one': 1, 'two': 2, 'three': 3, 'four': 4, 'five': 5, 'six': 6}

x = `x`
x = `1 or 2 or 3`
x = x
x = 'x'
x = 123

### exprlist: expr (',' expr)* [',']
### testlist: test (',' test)* [',']
# These have been exercised enough above

print 'classdef' # 'class' NAME ['(' testlist ')'] ':' suite
class B: pass
class C1(B): pass
class C2(B): pass
class D(C1, C2, B): pass
class C:
	def meth1(self): pass
	def meth2(self, arg): pass
	def meth3(self, a1, a2): pass
