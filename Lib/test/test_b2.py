# Python test set -- part 4b, built-in functions n-z

from test_support import *

print 'oct'
if oct(100) != '0144': raise TestFailed, 'oct(100)'
if oct(100L) != '0144L': raise TestFailed, 'oct(100L)'
if oct(-100) not in ('037777777634', '01777777777777777777634'):
	raise TestFailed, 'oct(-100)'
if oct(-100L) != '-0144L': raise TestFailed, 'oct(-100L)'

print 'open'
# NB the first 4 lines are also used to test input and raw_input, below
fp = open(TESTFN, 'w')
try:
	fp.write('1+1\n')
	fp.write('1+1\n')
	fp.write('The quick brown fox jumps over the lazy dog')
	fp.write('.\n')
	fp.write('Dear John\n')
	fp.write('XXX'*100)
	fp.write('YYY'*100)
finally:
	fp.close()
#
fp = open(TESTFN, 'r')
try:
	if fp.readline(4) <> '1+1\n': raise TestFailed, 'readline(4) # exact'
	if fp.readline(4) <> '1+1\n': raise TestFailed, 'readline(4) # exact'
	if fp.readline() <> 'The quick brown fox jumps over the lazy dog.\n':
		raise TestFailed, 'readline() # default'
	if fp.readline(4) <> 'Dear': raise TestFailed, 'readline(4) # short'
	if fp.readline(100) <> ' John\n': raise TestFailed, 'readline(100)'
	if fp.read(300) <> 'XXX'*100: raise TestFailed, 'read(300)'
	if fp.read(1000) <> 'YYY'*100: raise TestFailed, 'read(1000) # truncate'
finally:
	fp.close()

print 'ord'
if ord(' ') <> 32: raise TestFailed, 'ord(\' \')'
if ord('A') <> 65: raise TestFailed, 'ord(\'A\')'
if ord('a') <> 97: raise TestFailed, 'ord(\'a\')'

print 'pow'
if pow(0,0) <> 1: raise TestFailed, 'pow(0,0)'
if pow(0,1) <> 0: raise TestFailed, 'pow(0,1)'
if pow(1,0) <> 1: raise TestFailed, 'pow(1,0)'
if pow(1,1) <> 1: raise TestFailed, 'pow(1,1)'
#
if pow(2,0) <> 1: raise TestFailed, 'pow(2,0)'
if pow(2,10) <> 1024: raise TestFailed, 'pow(2,10)'
if pow(2,20) <> 1024*1024: raise TestFailed, 'pow(2,20)'
if pow(2,30) <> 1024*1024*1024: raise TestFailed, 'pow(2,30)'
#
if pow(-2,0) <> 1: raise TestFailed, 'pow(-2,0)'
if pow(-2,1) <> -2: raise TestFailed, 'pow(-2,1)'
if pow(-2,2) <> 4: raise TestFailed, 'pow(-2,2)'
if pow(-2,3) <> -8: raise TestFailed, 'pow(-2,3)'
#
if pow(0L,0) <> 1: raise TestFailed, 'pow(0L,0)'
if pow(0L,1) <> 0: raise TestFailed, 'pow(0L,1)'
if pow(1L,0) <> 1: raise TestFailed, 'pow(1L,0)'
if pow(1L,1) <> 1: raise TestFailed, 'pow(1L,1)'
#
if pow(2L,0) <> 1: raise TestFailed, 'pow(2L,0)'
if pow(2L,10) <> 1024: raise TestFailed, 'pow(2L,10)'
if pow(2L,20) <> 1024*1024: raise TestFailed, 'pow(2L,20)'
if pow(2L,30) <> 1024*1024*1024: raise TestFailed, 'pow(2L,30)'
#
if pow(-2L,0) <> 1: raise TestFailed, 'pow(-2L,0)'
if pow(-2L,1) <> -2: raise TestFailed, 'pow(-2L,1)'
if pow(-2L,2) <> 4: raise TestFailed, 'pow(-2L,2)'
if pow(-2L,3) <> -8: raise TestFailed, 'pow(-2L,3)'
#
if fcmp(pow(0.,0), 1.): raise TestFailed, 'pow(0.,0)'
if fcmp(pow(0.,1), 0.): raise TestFailed, 'pow(0.,1)'
if fcmp(pow(1.,0), 1.): raise TestFailed, 'pow(1.,0)'
if fcmp(pow(1.,1), 1.): raise TestFailed, 'pow(1.,1)'
#
if fcmp(pow(2.,0), 1.): raise TestFailed, 'pow(2.,0)'
if fcmp(pow(2.,10), 1024.): raise TestFailed, 'pow(2.,10)'
if fcmp(pow(2.,20), 1024.*1024.): raise TestFailed, 'pow(2.,20)'
if fcmp(pow(2.,30), 1024.*1024.*1024.): raise TestFailed, 'pow(2.,30)'
#
# XXX These don't work -- negative float to the float power...
#if fcmp(pow(-2.,0), 1.): raise TestFailed, 'pow(-2.,0)'
#if fcmp(pow(-2.,1), -2.): raise TestFailed, 'pow(-2.,1)'
#if fcmp(pow(-2.,2), 4.): raise TestFailed, 'pow(-2.,2)'
#if fcmp(pow(-2.,3), -8.): raise TestFailed, 'pow(-2.,3)'
#
for x in 2, 2L, 2.0:
	for y in 10, 10L, 10.0:
		for z in 1000, 1000L, 1000.0:
			if fcmp(pow(x, y, z), 24.0):
				raise TestFailed, 'pow(%s, %s, %s)' % (x, y, z)

print 'range'
if range(3) <> [0, 1, 2]: raise TestFailed, 'range(3)'
if range(1, 5) <> [1, 2, 3, 4]: raise TestFailed, 'range(1, 5)'
if range(0) <> []: raise TestFailed, 'range(0)'
if range(-3) <> []: raise TestFailed, 'range(-3)'
if range(1, 10, 3) <> [1, 4, 7]: raise TestFailed, 'range(1, 10, 3)'
if range(5, -5, -3) <> [5, 2, -1, -4]: raise TestFailed, 'range(5, -5, -3)'

print 'input and raw_input'
import sys
fp = open(TESTFN, 'r')
savestdin = sys.stdin
try:
	sys.stdin = fp
	if input() <> 2: raise TestFailed, 'input()'
	if input('testing\n') <> 2: raise TestFailed, 'input()'
	if raw_input() <> 'The quick brown fox jumps over the lazy dog.':
		raise TestFailed, 'raw_input()'
	if raw_input('testing\n') <> 'Dear John':
		raise TestFailed, 'raw_input(\'testing\\n\')'
finally:
	sys.stdin = savestdin
	fp.close()

print 'reduce'
if reduce(lambda x, y: x+y, ['a', 'b', 'c'], '') <> 'abc':
	raise TestFailed, 'reduce(): implode a string'
if reduce(lambda x, y: x+y,
	  [['a', 'c'], [], ['d', 'w']], []) <> ['a','c','d','w']:
	raise TestFailed, 'reduce(): append'
if reduce(lambda x, y: x*y, range(2,8), 1) <> 5040:
	raise TestFailed, 'reduce(): compute 7!'
if reduce(lambda x, y: x*y, range(2,21), 1L) <> 2432902008176640000L:
	raise TestFailed, 'reduce(): compute 20!, use long'
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
if reduce(lambda x, y: x+y, Squares(10)) != 285:
	raise TestFailed, 'reduce(<+>, Squares(10))'
if reduce(lambda x, y: x+y, Squares(10), 0) != 285:
	raise TestFailed, 'reduce(<+>, Squares(10), 0)'
if reduce(lambda x, y: x+y, Squares(0), 0) != 0:
	raise TestFailed, 'reduce(<+>, Squares(0), 0)'


print 'reload'
import marshal
reload(marshal)
import string
reload(string)
## import sys
## try: reload(sys)
## except ImportError: pass
## else: raise TestFailed, 'reload(sys) should fail'

print 'repr'
if repr('') <> '\'\'': raise TestFailed, 'repr(\'\')'
if repr(0) <> '0': raise TestFailed, 'repr(0)'
if repr(0L) <> '0L': raise TestFailed, 'repr(0L)'
if repr(()) <> '()': raise TestFailed, 'repr(())'
if repr([]) <> '[]': raise TestFailed, 'repr([])'
if repr({}) <> '{}': raise TestFailed, 'repr({})'

print 'round'
if round(0.0) <> 0.0: raise TestFailed, 'round(0.0)'
if round(1.0) <> 1.0: raise TestFailed, 'round(1.0)'
if round(10.0) <> 10.0: raise TestFailed, 'round(10.0)'
if round(1000000000.0) <> 1000000000.0:
	raise TestFailed, 'round(1000000000.0)'
if round(1e20) <> 1e20: raise TestFailed, 'round(1e20)'

if round(-1.0) <> -1.0: raise TestFailed, 'round(-1.0)'
if round(-10.0) <> -10.0: raise TestFailed, 'round(-10.0)'
if round(-1000000000.0) <> -1000000000.0:
	raise TestFailed, 'round(-1000000000.0)'
if round(-1e20) <> -1e20: raise TestFailed, 'round(-1e20)'

if round(0.1) <> 0.0: raise TestFailed, 'round(0.0)'
if round(1.1) <> 1.0: raise TestFailed, 'round(1.0)'
if round(10.1) <> 10.0: raise TestFailed, 'round(10.0)'
if round(1000000000.1) <> 1000000000.0:
	raise TestFailed, 'round(1000000000.0)'

if round(-1.1) <> -1.0: raise TestFailed, 'round(-1.0)'
if round(-10.1) <> -10.0: raise TestFailed, 'round(-10.0)'
if round(-1000000000.1) <> -1000000000.0:
	raise TestFailed, 'round(-1000000000.0)'

if round(0.9) <> 1.0: raise TestFailed, 'round(0.9)'
if round(9.9) <> 10.0: raise TestFailed, 'round(9.9)'
if round(999999999.9) <> 1000000000.0:
	raise TestFailed, 'round(999999999.9)'

if round(-0.9) <> -1.0: raise TestFailed, 'round(-0.9)'
if round(-9.9) <> -10.0: raise TestFailed, 'round(-9.9)'
if round(-999999999.9) <> -1000000000.0:
	raise TestFailed, 'round(-999999999.9)'

print 'setattr'
import sys
setattr(sys, 'spam', 1)
if sys.spam != 1: raise TestFailed, 'setattr(sys, \'spam\', 1)'

print 'str'
if str('') <> '': raise TestFailed, 'str(\'\')'
if str(0) <> '0': raise TestFailed, 'str(0)'
if str(0L) <> '0': raise TestFailed, 'str(0L)'
if str(()) <> '()': raise TestFailed, 'str(())'
if str([]) <> '[]': raise TestFailed, 'str([])'
if str({}) <> '{}': raise TestFailed, 'str({})'

print 'tuple'
if tuple(()) <> (): raise TestFailed, 'tuple(())'
if tuple((0, 1, 2, 3)) <> (0, 1, 2, 3): raise TestFailed, 'tuple((0, 1, 2, 3))'
if tuple([]) <> (): raise TestFailed, 'tuple([])'
if tuple([0, 1, 2, 3]) <> (0, 1, 2, 3): raise TestFailed, 'tuple([0, 1, 2, 3])'
if tuple('') <> (): raise TestFailed, 'tuple('')'
if tuple('spam') <> ('s', 'p', 'a', 'm'): raise TestFailed, "tuple('spam')"

print 'type'
if type('') <> type('123') or type('') == type(()):
	raise TestFailed, 'type()'

print 'vars'
a = b = None
a = vars().keys()
b = dir()
a.sort()
b.sort()
if a <> b: raise TestFailed, 'vars()'
import sys
a = vars(sys).keys()
b = dir(sys)
a.sort()
b.sort()
if a <> b: raise TestFailed, 'vars(sys)'
def f0():
	if vars() != {}: raise TestFailed, 'vars() in f0()'
f0()
def f2():
	f0()
	a = 1
	b = 2
	if vars() != {'a': a, 'b': b}: raise TestFailed, 'vars() in f2()'
f2()

print 'xrange'
if tuple(xrange(10)) <> tuple(range(10)): raise TestFailed, 'xrange(10)'
if tuple(xrange(5,10)) <> tuple(range(5,10)): raise TestFailed, 'xrange(5,10)'
if tuple(xrange(0,10,2)) <> tuple(range(0,10,2)):
	raise TestFailed, 'xrange(0,10,2)'

print 'zip'
a = (1, 2, 3)
b = (4, 5, 6)
t = [(1, 4), (2, 5), (3, 6)]
if zip(a, b) <> t: raise TestFailed, 'zip(a, b) - same size, both tuples'
b = [4, 5, 6]
if zip(a, b) <> t: raise TestFailed, 'zip(a, b) - same size, tuple/list'
b = (4, 5, 6, 7)
if zip(a, b) <> t: raise TestFailed, 'zip(a, b) - b is longer'
class I:
	def __getitem__(self, i):
		if i < 0 or i > 2: raise IndexError
		return i + 4
if zip(a, I()) <> t: raise TestFailed, 'zip(a, b) - b is instance'
exc = 0
try:
	zip()
except TypeError:
	exc = 1
except:
	e = sys.exc_info()[0]
	raise TestFailed, 'zip() - no args, expected TypeError, got %s' % e
if not exc:
	raise TestFailed, 'zip() - no args, missing expected TypeError'

exc = 0
try:
	zip(None)
except TypeError:
	exc = 1
except:
	e = sys.exc_info()[0]
	raise TestFailed, 'zip(None) - expected TypeError, got %s' % e
if not exc:
	raise TestFailed, 'zip(None) - missing expected TypeError'
class G:
	pass
exc = 0
try:
	zip(a, G())
except AttributeError:
	exc = 1
except:
	e = sys.exc_info()[0]
	raise TestFailed, 'zip(a, b) - b instance w/o __getitem__'
if not exc:
	raise TestFailed, 'zip(a, b) - missing expected AttributeError'


# Epilogue -- unlink the temp file

unlink(TESTFN)
