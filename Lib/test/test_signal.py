# Test the signal module
from test_support import verbose, TestSkipped, TestFailed
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


if hasattr(signal, "sigprocmask"):
    class HupDelivered(Exception):
        pass
    def hup(signum, frame):
        raise HupDelivered
    def hup2(signum, frame):
        signal.signal(signal.SIGHUP, hup)
        return
    signal.signal(signal.SIGHUP, hup)

    if verbose:
        print "blocking SIGHUP"

    defaultmask = signal.sigprocmask(signal.SIG_BLOCK, [signal.SIGHUP])

    if verbose:
        print "sending SIGHUP"

    try:
        os.kill(pid, signal.SIGHUP)
    except HupDelivered:
        raise TestFailed, "HUP not blocked"

    if signal.SIGHUP not in signal.sigpending():
        raise TestFailed, "HUP not pending"

    if verbose:
        print "unblocking SIGHUP"

    try:
        signal.sigprocmask(signal.SIG_UNBLOCK, [signal.SIGHUP])
    except HupDelivered:
        pass
    else:
        raise TestFailed, "HUP not delivered"

    if verbose:
        print "testing sigsuspend"

    signal.sigprocmask(signal.SIG_BLOCK, [signal.SIGHUP])
    signal.signal(signal.SIGHUP, hup2)

    if not os.fork():
        time.sleep(2)
        os.kill(pid, signal.SIGHUP)
        time.sleep(2)
        os.kill(pid, signal.SIGHUP)
        os._exit(0)
    else:
        try:
            signal.sigsuspend(defaultmask)
        except:
            raise TestFailed, "sigsuspend erroneously raised"

        try:
            signal.sigsuspend(defaultmask)
        except HupDelivered:
            pass
        else:
            raise TestFailed, "sigsupsend didn't raise"
    
