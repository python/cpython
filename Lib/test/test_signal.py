# Test the signal module
from test.test_support import verbose, TestSkipped, TestFailed
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

def handlerA(*args):
    if verbose:
        print "handlerA", args

class HandlerBCalled(Exception):
    pass

def handlerB(*args):
    if verbose:
        print "handlerB", args
    raise HandlerBCalled, args

signal.alarm(20)                        # Entire test lasts at most 20 sec.
hup = signal.signal(signal.SIGHUP, handlerA)
usr1 = signal.signal(signal.SIGUSR1, handlerB)
usr2 = signal.signal(signal.SIGUSR2, signal.SIG_IGN)
alrm = signal.signal(signal.SIGALRM, signal.default_int_handler)

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

finally:
    signal.signal(signal.SIGHUP, hup)
    signal.signal(signal.SIGUSR1, usr1)
    signal.signal(signal.SIGUSR2, usr2)
    signal.signal(signal.SIGALRM, alrm)
