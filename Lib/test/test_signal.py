# Test the signal module
from test_support import verbose, TestSkipped
import signal
import os
import sys

if sys.platform[:3] in ('win', 'os2') or sys.platform=='riscos':
    raise TestSkipped, "Can't test signal on %s" % sys.platform

if verbose:
    x = '-x'
else:
    x = '+x'
pid = os.getpid()

# Shell script that will send us asynchronous signals
script = """
 (
        set %(x)s
        sleep 2
        kill -5 %(pid)d
        sleep 2
        kill -2 %(pid)d
        sleep 2
        kill -3 %(pid)d
 ) &
""" % vars()

def handlerA(*args):
    if verbose:
        print "handlerA", args

HandlerBCalled = "HandlerBCalled"       # Exception

def handlerB(*args):
    if verbose:
        print "handlerB", args
    raise HandlerBCalled, args

signal.alarm(20)                        # Entire test lasts at most 20 sec.
signal.signal(5, handlerA)
signal.signal(2, handlerB)
signal.signal(3, signal.SIG_IGN)
signal.signal(signal.SIGALRM, signal.default_int_handler)

os.system(script)

print "starting pause() loop..."

try:
    while 1:
        if verbose:
            print "call pause()..."
        try:
            signal.pause()
            if verbose:
                print "pause() returned"
        except HandlerBCalled:
            if verbose:
                print "HandlerBCalled exception caught"
            else:
                pass

except KeyboardInterrupt:
    if verbose:
        print "KeyboardInterrupt (assume the alarm() went off)"
