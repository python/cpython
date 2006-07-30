# Test the signal module
from test.test_support import verbose, TestSkipped, TestFailed, vereq
import signal
import os, sys, time

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
        kill -HUP %(pid)d
        sleep 2
        kill -USR1 %(pid)d
        sleep 2
        kill -USR2 %(pid)d
 ) &
""" % vars()

a_called = b_called = False

def handlerA(*args):
    global a_called
    a_called = True
    if verbose:
        print "handlerA", args

class HandlerBCalled(Exception):
    pass

def handlerB(*args):
    global b_called
    b_called = True
    if verbose:
        print "handlerB", args
    raise HandlerBCalled, args

signal.alarm(20)                        # Entire test lasts at most 20 sec.
hup = signal.signal(signal.SIGHUP, handlerA)
usr1 = signal.signal(signal.SIGUSR1, handlerB)
usr2 = signal.signal(signal.SIGUSR2, signal.SIG_IGN)
alrm = signal.signal(signal.SIGALRM, signal.default_int_handler)

vereq(signal.getsignal(signal.SIGHUP), handlerA)
vereq(signal.getsignal(signal.SIGUSR1), handlerB)
vereq(signal.getsignal(signal.SIGUSR2), signal.SIG_IGN)

try:
    signal.signal(4242, handlerB)
    raise TestFailed, 'expected ValueError for invalid signal # to signal()'
except ValueError:
    pass

try:
    signal.getsignal(4242)
    raise TestFailed, 'expected ValueError for invalid signal # to getsignal()'
except ValueError:
    pass

try:
    signal.signal(signal.SIGUSR1, None)
    raise TestFailed, 'expected TypeError for non-callable'
except TypeError:
    pass

try:
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

    if not a_called:
        print 'HandlerA not called'

    if not b_called:
        print 'HandlerB not called'

finally:
    signal.signal(signal.SIGHUP, hup)
    signal.signal(signal.SIGUSR1, usr1)
    signal.signal(signal.SIGUSR2, usr2)
    signal.signal(signal.SIGALRM, alrm)
