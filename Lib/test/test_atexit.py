# Test the exit module
from test_support import verbose
import atexit

def handler1():
    print "handler1"

def handler2(*args, **kargs):
    print "handler2", args, kargs

# save any exit functions that may have been registered as part of the
# test framework
_exithandlers = atexit._exithandlers
atexit._exithandlers = []

atexit.register(handler1)
atexit.register(handler2)
atexit.register(handler2, 7, kw="abc")

# simulate exit behavior by calling atexit._run_exitfuncs directly...
atexit._run_exitfuncs()

# restore exit handlers
atexit._exithandlers = _exithandlers
