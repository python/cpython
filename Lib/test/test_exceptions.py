# Python test set -- part 5, built-in exceptions

from test_support import *
from types import ClassType

print '5. Built-in exceptions'
# XXX This is not really enough, each *operation* should be tested!

def test_raise_catch(exc):
    try:
        raise exc, "spam"
    except exc, err:
        buf = str(err)
    try:
        raise exc("spam")
    except exc, err:
        buf = str(err)
    print buf

def r(thing):
    test_raise_catch(thing)
    if isinstance(thing, ClassType):
        print thing.__name__
    else:
        print thing

r(AttributeError)
import sys
try: x = sys.undefined_attribute
except AttributeError: pass

r(EOFError)
import sys
fp = open(TESTFN, 'w')
fp.close()
fp = open(TESTFN, 'r')
savestdin = sys.stdin
try:
        try:
                sys.stdin = fp
                x = raw_input()
        except EOFError:
                pass
finally:
        sys.stdin = savestdin
        fp.close()

r(IOError)
try: open('this file does not exist', 'r')
except IOError: pass

r(ImportError)
try: import undefined_module
except ImportError: pass

r(IndexError)
x = []
try: a = x[10]
except IndexError: pass

r(KeyError)
x = {}
try: a = x['key']
except KeyError: pass

r(KeyboardInterrupt)
print '(not testable in a script)'

r(MemoryError)
print '(not safe to test)'

r(NameError)
try: x = undefined_variable
except NameError: pass

r(OverflowError)
x = 1
try:
        while 1: x = x+x
except OverflowError: pass

r(RuntimeError)
print '(not used any more?)'

r(SyntaxError)
try: exec '/\n'
except SyntaxError: pass

# make sure the right exception message is raised for each of these
# code fragments:

def ckmsg(src, msg):
    try:
        compile(src, '<fragment>', 'exec')
    except SyntaxError, e:
        print e.msg
        if e.msg == msg:
            print "ok"
        else:
            print "expected:", msg
    else:
        print "failed to get expected SyntaxError"

s = '''\
while 1:
    try:
        continue
    except:
        pass
'''
ckmsg(s, "'continue' not supported inside 'try' clause")
s = '''\
while 1:
    try:
        continue
    finally:
        pass
'''
ckmsg(s, "'continue' not supported inside 'try' clause")
s = '''\
while 1:
    try:
        if 1:
            continue
    finally:
        pass
'''
ckmsg(s, "'continue' not supported inside 'try' clause")
s = '''\
try:
    continue
except:
    pass
'''
ckmsg(s, "'continue' not properly in loop")
ckmsg("continue\n", "'continue' not properly in loop")

r(IndentationError)

r(TabError)
# can only be tested under -tt, and is the only test for -tt
#try: compile("try:\n\t1/0\n    \t1/0\nfinally:\n pass\n", '<string>', 'exec')
#except TabError: pass
#else: raise TestFailed

r(SystemError)
print '(hard to reproduce)'

r(SystemExit)
import sys
try: sys.exit(0)
except SystemExit: pass

r(TypeError)
try: [] + ()
except TypeError: pass

r(ValueError)
try: x = chr(10000)
except ValueError: pass

r(ZeroDivisionError)
try: x = 1/0
except ZeroDivisionError: pass

r(Exception)
try: x = 1/0
except Exception, e: pass

unlink(TESTFN)
