# Python test set -- part 5, built-in exceptions

from test.test_support import TestFailed, TESTFN, unlink
from types import ClassType
import warnings
import sys, traceback, os

print '5. Built-in exceptions'
# XXX This is not really enough, each *operation* should be tested!

# Reloading the built-in exceptions module failed prior to Py2.2, while it
# should act the same as reloading built-in sys.
try:
    import exceptions
    reload(exceptions)
except ImportError, e:
    raise TestFailed, e

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
# XXX
# Obscure:  in 2.2 and 2.3, this test relied on changing OverflowWarning
# into an error, in order to trigger OverflowError.  In 2.4, OverflowWarning
# should no longer be generated, so the focus of the test shifts to showing
# that OverflowError *isn't* generated.  OverflowWarning should be gone
# in Python 2.5, and then the filterwarnings() call, and this comment,
# should go away.
warnings.filterwarnings("error", "", OverflowWarning, __name__)
x = 1
for dummy in range(128):
    x += x  # this simply shouldn't blow up

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
        pass
    finally:
        continue
'''
if sys.platform.startswith('java'):
    print "'continue' not supported inside 'finally' clause"
    print "ok"
else:
    ckmsg(s, "'continue' not supported inside 'finally' clause")
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

# test that setting an exception at the C level works even if the
# exception object can't be constructed.

class BadException:
    def __init__(self):
        raise RuntimeError, "can't instantiate BadException"

def test_capi1():
    import _testcapi
    try:
        _testcapi.raise_exception(BadException, 1)
    except TypeError, err:
        exc, err, tb = sys.exc_info()
        co = tb.tb_frame.f_code
        assert co.co_name == "test_capi1"
        assert co.co_filename.endswith('test_exceptions'+os.extsep+'py')
    else:
        print "Expected exception"

def test_capi2():
    import _testcapi
    try:
        _testcapi.raise_exception(BadException, 0)
    except RuntimeError, err:
        exc, err, tb = sys.exc_info()
        co = tb.tb_frame.f_code
        assert co.co_name == "__init__"
        assert co.co_filename.endswith('test_exceptions'+os.extsep+'py')
        co2 = tb.tb_frame.f_back.f_code
        assert co2.co_name == "test_capi2"
    else:
        print "Expected exception"

if not sys.platform.startswith('java'):
    test_capi1()
    test_capi2()

unlink(TESTFN)
