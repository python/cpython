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
if  2147483647   != 017777777777: raise TestFailed, 'large positive int'
try:
	from sys import maxint
except ImportError:
	maxint = 2147483647
if maxint == 2147483647:
	if -2147483647-1 != 020000000000: raise TestFailed, 'max negative int'
	# XXX -2147483648
	if 037777777777 != -1: raise TestFailed, 'oct -1'
	if 0xffffffff != -1: raise TestFailed, 'hex -1'
	for s in '2147483648', '040000000000', '0x100000000':
		try:
			x = eval(s)
		except OverflowError:
			continue
##		raise TestFailed, \
		print \
			  'No OverflowError on huge integer literal ' + `s`
elif eval('maxint == 9223372036854775807'):
	if eval('-9223372036854775807-1 != 01000000000000000000000'):
		raise TestFailed, 'max negative int'
	if eval('01777777777777777777777') != -1: raise TestFailed, 'oct -1'
	if eval('0xffffffffffffffff') != -1: raise TestFailed, 'hex -1'
	for s in '9223372036854775808', '02000000000000000000000', \
		 '0x10000000000000000':
		try:
			x = eval(s)
		except OverflowError:
			continue
		raise TestFailed, \
			  'No OverflowError on huge integer literal ' + `s`
else:
	print 'Weird maxint value', maxint

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

print '1.1.3 String literals'

##def assert(s):
##	if not s: raise TestFailed, 'see traceback'

x = ''; y = ""; assert(len(x) == 0 and x == y)
x = '\''; y = "'"; assert(len(x) == 1 and x == y and ord(x) == 39)
x = '"'; y = "\""; assert(len(x) == 1 and x == y and ord(x) == 34)
x = "doesn't \"shrink\" does it"
y = 'doesn\'t "shrink" does it'
assert(len(x) == 24 and x == y)
x = "does \"shrink\" doesn't it"
y = 'does "shrink" doesn\'t it'
assert(len(x) == 24 and x == y)
x = """
The "quick"
brown fox
jumps over
the 'lazy' dog.
"""
y = '\nThe "quick"\nbrown fox\njumps over\nthe \'lazy\' dog.\n'
assert(x == y)
y = '''
The "quick"
brown fox
jumps over
the 'lazy' dog.
'''; assert(x == y)
y = "\n\
The \"quick\"\n\
brown fox\n\
jumps over\n\
the 'lazy' dog.\n\
"; assert(x == y)
y = '\n\
The \"quick\"\n\
brown fox\n\
jumps over\n\
the \'lazy\' dog.\n\
'; assert(x == y)


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
### varargslist: (fpdef ['=' test] ',')* ('*' NAME [',' ('**'|'*' '*') NAME]
###            | ('**'|'*' '*') NAME)
###            | fpdef ['=' test] (',' fpdef ['=' test])* [',']  
### fpdef: NAME | '(' fplist ')'
### fplist: fpdef (',' fpdef)* [',']
### arglist: (argument ',')* (argument | *' test [',' '**' test] | '**' test)
### argument: [test '='] test	# Really [keyword '='] test
def f1(): pass
f1()
f1(*())
f1(*(), **{})
def f2(one_argument): pass
def f3(two, arguments): pass
def f4(two, (compound, (argument, list))): pass
def a1(one_arg,): pass
def a2(two, args,): pass
def v0(*rest): pass
def v1(a, *rest): pass
def v2(a, b, *rest): pass
def v3(a, (b, c), *rest): pass
def d01(a=1): pass
d01()
d01(1)
d01(*(1,))
d01(**{'a':2})
def d11(a, b=1): pass
d11(1)
d11(1, 2)
d11(1, **{'b':2})
def d21(a, b, c=1): pass
d21(1, 2)
d21(1, 2, 3)
d21(*(1, 2, 3))
d21(1, *(2, 3))
d21(1, 2, *(3,))
d21(1, 2, **{'c':3})
def d02(a=1, b=2): pass
d02()
d02(1)
d02(1, 2)
d02(*(1, 2))
d02(1, *(2,))
d02(1, **{'b':2})
d02(**{'a': 1, 'b': 2})
def d12(a, b=1, c=2): pass
d12(1)
d12(1, 2)
d12(1, 2, 3)
def d22(a, b, c=1, d=2): pass
d22(1, 2)
d22(1, 2, 3)
d22(1, 2, 3, 4)
def d01v(a=1, *rest): pass
d01v()
d01v(1)
d01v(1, 2)
d01v(*(1, 2, 3, 4))
d01v(*(1,))
d01v(**{'a':2})
def d11v(a, b=1, *rest): pass
d11v(1)
d11v(1, 2)
d11v(1, 2, 3)
def d21v(a, b, c=1, *rest): pass
d21v(1, 2)
d21v(1, 2, 3)
d21v(1, 2, 3, 4)
d21v(*(1, 2, 3, 4))
d21v(1, 2, **{'c': 3})
def d02v(a=1, b=2, *rest): pass
d02v()
d02v(1)
d02v(1, 2)
d02v(1, 2, 3)
d02v(1, *(2, 3, 4))
d02v(**{'a': 1, 'b': 2})
def d12v(a, b=1, c=2, *rest): pass
d12v(1)
d12v(1, 2)
d12v(1, 2, 3)
d12v(1, 2, 3, 4)
d12v(*(1, 2, 3, 4))
d12v(1, 2, *(3, 4, 5))
d12v(1, *(2,), **{'c': 3})
def d22v(a, b, c=1, d=2, *rest): pass
d22v(1, 2)
d22v(1, 2, 3)
d22v(1, 2, 3, 4)
d22v(1, 2, 3, 4, 5)
d22v(*(1, 2, 3, 4))
d22v(1, 2, *(3, 4, 5))
d22v(1, *(2, 3), **{'d': 4})

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

print 'extended print_stmt' # 'print' '>>' test ','
import sys
print >> sys.stdout, 1, 2, 3
print >> sys.stdout, 1, 2, 3,
print >> sys.stdout
print >> sys.stdout, 0 or 1, 0 or 1,
print >> sys.stdout, 0 or 1

# test printing to an instance
class Gulp:
	def write(self, msg): pass

gulp = Gulp()
print >> gulp, 1, 2, 3
print >> gulp, 1, 2, 3,
print >> gulp
print >> gulp, 0 or 1, 0 or 1,
print >> gulp, 0 or 1

# test print >> None
def driver():
	oldstdout = sys.stdout
	sys.stdout = Gulp()
	try:
		tellme(Gulp())
		tellme()
	finally:
		sys.stdout = oldstdout

# we should see this once
def tellme(file=sys.stdout):
	print >> file, 'hello world'

driver()

# we should not see this at all
def tellme(file=None):
	print >> file, 'goodbye universe'

driver()

# syntax errors
def check_syntax(statement):
	try:
		compile(statement, '<string>', 'exec')
	except SyntaxError:
		pass
	else:
		print 'Missing SyntaxError: "%s"' % statement
check_syntax('print ,')
check_syntax('print >> x,')

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
import sys
import time, sys
from time import time
from sys import *
from sys import path, argv

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
	z = None
	del z
	exec u'z=1+1\n'
	if z <> 2: raise TestFailed, 'exec u\'z=1+1\'\\n'
	del z
	exec u'z=1+1'
	if z <> 2: raise TestFailed, 'exec u\'z=1+1\''
f()
g = {}
exec 'z = 1' in g
if g.has_key('__builtins__'): del g['__builtins__']
if g <> {'z': 1}: raise TestFailed, 'exec \'z = 1\' in g'
g = {}
l = {}
exec 'global a; a = 1; b = 2' in g, l
if g.has_key('__builtins__'): del g['__builtins__']
if l.has_key('__builtins__'): del l['__builtins__']
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
for i in 1, 2, 3: pass
for i, j, k in (): pass
else: pass
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
n = 0
for x in Squares(10): n = n+x
if n != 285: raise TestFailed, 'for over growing sequence'

print 'try_stmt'
### try_stmt: 'try' ':' suite (except_clause ':' suite)+ ['else' ':' suite]
###         | 'try' ':' suite 'finally' ':' suite
### except_clause: 'except' [expr [',' expr]]
try:
	1/0
except ZeroDivisionError:
	pass
else:
	pass
try: 1/0
except EOFError: pass
except TypeError, msg: pass
except RuntimeError, msg: pass
except: pass
else: pass
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
print
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

# list comprehension tests
nums = [1, 2, 3, 4, 5]
strs = ["Apple", "Banana", "Coconut"]
spcs = ["  Apple", " Banana ", "Coco  nut  "]

print [s.strip() for s in spcs]
print [3 * x for x in nums]
print [x for x in nums if x > 2]
print [(i, s) for i in nums for s in strs]
print [(i, s) for i in nums for s in [f for f in strs if "n" in f]]
try:
    eval("[i, s for i in nums for s in strs]")
    print "FAIL: should have raised a SyntaxError!"
except SyntaxError:
    print "good: got a SyntaxError as expected"

try:
    eval("[x if y]")
    print "FAIL: should have raised a SyntaxError!"
except SyntaxError:
        print "good: got a SyntaxError as expected"

suppliers = [
  (1, "Boeing"),
  (2, "Ford"),
  (3, "Macdonalds")
]

parts = [
  (10, "Airliner"),
  (20, "Engine"),
  (30, "Cheeseburger")
]

suppart = [
  (1, 10), (1, 20), (2, 20), (3, 30)
]

print [
  (sname, pname)
    for (sno, sname) in suppliers
      for (pno, pname) in parts
        for (sp_sno, sp_pno) in suppart
          if sno == sp_sno and pno == sp_pno
]
