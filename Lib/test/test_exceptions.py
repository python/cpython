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
    print getattr(thing, '__name__', thing)

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
        import marshal
        marshal.loads('')
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

class BadException(Exception):
    def __init__(self):
        raise RuntimeError, "can't instantiate BadException"

# Exceptions must inherit from BaseException, raising invalid exception
# should instead raise SystemError
class InvalidException:
    pass

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

def test_capi3():
    import _testcapi
    try:
        _testcapi.raise_exception(InvalidException, 1)
    except SystemError:
        pass
    except InvalidException:
        raise AssertionError("Managed to raise InvalidException");
    else:
        print "Expected SystemError exception"
    

if not sys.platform.startswith('java'):
    test_capi1()
    test_capi2()
    test_capi3()

unlink(TESTFN)

#  test that exception attributes are happy.
try: str(u'Hello \u00E1')
except Exception, e: sampleUnicodeEncodeError = e
try: unicode('\xff')
except Exception, e: sampleUnicodeDecodeError = e
exceptionList = [
        ( BaseException, (), { 'message' : '', 'args' : () }),
        ( BaseException, (1, ), { 'message' : 1, 'args' : ( 1, ) }),
        ( BaseException, ('foo', ), { 'message' : 'foo', 'args' : ( 'foo', ) }),
        ( BaseException, ('foo', 1), { 'message' : '', 'args' : ( 'foo', 1 ) }),
        ( SystemExit, ('foo',), { 'message' : 'foo', 'args' : ( 'foo', ),
                'code' : 'foo' }),
        ( IOError, ('foo',), { 'message' : 'foo', 'args' : ( 'foo', ), }),
        ( IOError, ('foo', 'bar'), { 'message' : '',
                'args' : ('foo', 'bar'), }),
        ( IOError, ('foo', 'bar', 'baz'),
                 { 'message' : '', 'args' : ('foo', 'bar'), }),
        ( EnvironmentError, ('errnoStr', 'strErrorStr', 'filenameStr'),
                { 'message' : '', 'args' : ('errnoStr', 'strErrorStr'),
                    'strerror' : 'strErrorStr',
                    'errno' : 'errnoStr', 'filename' : 'filenameStr' }),
        ( EnvironmentError, (1, 'strErrorStr', 'filenameStr'),
                { 'message' : '', 'args' : (1, 'strErrorStr'),
                    'strerror' : 'strErrorStr', 'errno' : 1,
                    'filename' : 'filenameStr' }),
        ( SyntaxError, ('msgStr',),
                { 'message' : 'msgStr', 'args' : ('msgStr', ),
                    'print_file_and_line' : None, 'msg' : 'msgStr',
                    'filename' : None, 'lineno' : None, 'offset' : None,
                    'text' : None }),
        ( SyntaxError, ('msgStr', ('filenameStr', 'linenoStr', 'offsetStr',
                        'textStr')),
                { 'message' : '', 'args' : ('msgStr', ('filenameStr',
                        'linenoStr', 'offsetStr', 'textStr' )),
                    'print_file_and_line' : None, 'msg' : 'msgStr',
                    'filename' : 'filenameStr', 'lineno' : 'linenoStr',
                    'offset' : 'offsetStr', 'text' : 'textStr' }),
        ( SyntaxError, ('msgStr', 'filenameStr', 'linenoStr', 'offsetStr',
                    'textStr', 'print_file_and_lineStr'),
                { 'message' : '', 'args' : ('msgStr', 'filenameStr',
                        'linenoStr', 'offsetStr', 'textStr',
                        'print_file_and_lineStr'),
                    'print_file_and_line' : None, 'msg' : 'msgStr',
                    'filename' : None, 'lineno' : None, 'offset' : None,
                    'text' : None }),
        ( UnicodeError, (),
                { 'message' : '', 'args' : (), }),
        ( sampleUnicodeEncodeError,
                { 'message' : '', 'args' : ('ascii', u'Hello \xe1', 6, 7,
                        'ordinal not in range(128)'),
                    'encoding' : 'ascii', 'object' : u'Hello \xe1',
                    'start' : 6, 'reason' : 'ordinal not in range(128)' }),
        ( sampleUnicodeDecodeError,
                { 'message' : '', 'args' : ('ascii', '\xff', 0, 1,
                        'ordinal not in range(128)'),
                    'encoding' : 'ascii', 'object' : '\xff',
                    'start' : 0, 'reason' : 'ordinal not in range(128)' }),
        ( UnicodeTranslateError, (u"\u3042", 0, 1, "ouch"),
                { 'message' : '', 'args' : (u'\u3042', 0, 1, 'ouch'),
                    'object' : u'\u3042', 'reason' : 'ouch',
                    'start' : 0, 'end' : 1 }),
        ]
try:
    exceptionList.append(
            ( WindowsError, (1, 'strErrorStr', 'filenameStr'),
                    { 'message' : '', 'args' : (1, 'strErrorStr'),
                        'strerror' : 'strErrorStr',
                        'errno' : 22, 'filename' : 'filenameStr',
                        'winerror' : 1 }))
except NameError: pass

for args in exceptionList:
    expected = args[-1]
    try:
        if len(args) == 2: raise args[0]
        else: raise apply(args[0], args[1])
    except BaseException, e:
        for checkArgName in expected.keys():
            if repr(getattr(e, checkArgName)) != repr(expected[checkArgName]):
                raise TestFailed('Checking exception arguments, exception '
                        '"%s", attribute "%s" expected %s got %s.' %
                        ( repr(e), checkArgName,
                            repr(expected[checkArgName]),
                            repr(getattr(e, checkArgName)) ))
