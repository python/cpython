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
if verbose:
    print "test runner's pid is", pid

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

MAX_DURATION = 20
signal.alarm(MAX_DURATION)   # Entire test should last at most 20 sec.
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

# Set up a child to send an alarm signal to us (the parent) after waiting
# long enough to receive the alarm.  It seems we miss the alarm for some
# reason.  This will hopefully stop the hangs on Tru64/Alpha.
def force_test_exit():
    # Sigh, both imports seem necessary to avoid errors.
    import os
    fork_pid = os.fork()
    if fork_pid == 0:
        # In child
        import os, time
        try:
            # Wait 5 seconds longer than the expected alarm to give enough
            # time for the normal sequence of events to occur.  This is
            # just a stop-gap to prevent the test from hanging.
            time.sleep(MAX_DURATION + 5)
            print >> sys.__stdout__, '  child should not have to kill parent'
            for i in range(3):
                os.kill(pid, signal.SIGALRM)
                print >> sys.__stdout__, "    child sent SIGALRM to", pid
        finally:
            os._exit(0)
    # In parent (or error)
    return fork_pid

try:
    os.system(script)

    # Try to ensure this test exits even if there is some problem with alarm.
    # Tru64/Alpha sometimes hangs and is ultimately killed by the buildbot.
    fork_pid = force_test_exit()
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

    # Forcibly kill the child we created to ping us if there was a test error.
    try:
        # Make sure we don't kill ourself if there was a fork error.
        if fork_pid > 0:
            os.kill(fork_pid, signal.SIGKILL)
    except:
        # If the child killed us, it has probably exited.  Killing a
        # non-existant process will raise an error which we don't care about.
        pass

    if not a_called:
        print 'HandlerA not called'

    if not b_called:
        print 'HandlerB not called'

finally:
    signal.signal(signal.SIGHUP, hup)
    signal.signal(signal.SIGUSR1, usr1)
    signal.signal(signal.SIGUSR2, usr2)
    signal.signal(signal.SIGALRM, alrm)
