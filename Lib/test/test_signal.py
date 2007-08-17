# Test the signal module
from test.test_support import verbose, TestSkipped, TestFailed, vereq
import signal
import os, sys, time

if sys.platform[:3] in ('win', 'os2'):
    raise TestSkipped, "Can't test signal on %s" % sys.platform

MAX_DURATION = 20   # Entire test should last at most 20 sec.

if verbose:
    x = '-x'
else:
    x = '+x'

pid = os.getpid()
if verbose:
    print("test runner's pid is", pid)

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
        print("handlerA invoked", args)

class HandlerBCalled(Exception):
    pass

def handlerB(*args):
    global b_called
    b_called = True
    if verbose:
        print("handlerB invoked", args)
    raise HandlerBCalled, args

# Set up a child to send signals to us (the parent) after waiting long
# enough to receive the alarm.  It seems we miss the alarm for some
# reason.  This will hopefully stop the hangs on Tru64/Alpha.
# Alas, it doesn't.  Tru64 appears to miss all the signals at times, or
# seemingly random subsets of them, and nothing done in force_test_exit
# so far has actually helped.
def force_test_exit():
    # Sigh, both imports seem necessary to avoid errors.
    import os
    fork_pid = os.fork()
    if fork_pid:
        # In parent.
        return fork_pid

    # In child.
    import os, time
    try:
        # Wait 5 seconds longer than the expected alarm to give enough
        # time for the normal sequence of events to occur.  This is
        # just a stop-gap to try to prevent the test from hanging.
        time.sleep(MAX_DURATION + 5)
        print('  child should not have to kill parent', file=sys.__stdout__)
        for signame in "SIGHUP", "SIGUSR1", "SIGUSR2", "SIGALRM":
            os.kill(pid, getattr(signal, signame))
            print("    child sent", signame, "to", pid, file=sys.__stdout__)
            time.sleep(1)
    finally:
        os._exit(0)

# Install handlers.
hup = signal.signal(signal.SIGHUP, handlerA)
usr1 = signal.signal(signal.SIGUSR1, handlerB)
usr2 = signal.signal(signal.SIGUSR2, signal.SIG_IGN)
alrm = signal.signal(signal.SIGALRM, signal.default_int_handler)

try:

    signal.alarm(MAX_DURATION)
    vereq(signal.getsignal(signal.SIGHUP), handlerA)
    vereq(signal.getsignal(signal.SIGUSR1), handlerB)
    vereq(signal.getsignal(signal.SIGUSR2), signal.SIG_IGN)
    vereq(signal.getsignal(signal.SIGALRM), signal.default_int_handler)

    # Try to ensure this test exits even if there is some problem with alarm.
    # Tru64/Alpha often hangs and is ultimately killed by the buildbot.
    fork_pid = force_test_exit()

    try:
        signal.getsignal(4242)
        raise TestFailed('expected ValueError for invalid signal # to '
                         'getsignal()')
    except ValueError:
        pass

    try:
        signal.signal(4242, handlerB)
        raise TestFailed('expected ValueError for invalid signal # to '
                         'signal()')
    except ValueError:
        pass

    try:
        signal.signal(signal.SIGUSR1, None)
        raise TestFailed('expected TypeError for non-callable')
    except TypeError:
        pass

    # Launch an external script to send us signals.
    # We expect the external script to:
    #    send HUP, which invokes handlerA to set a_called
    #    send USR1, which invokes handlerB to set b_called and raise
    #               HandlerBCalled
    #    send USR2, which is ignored
    #
    # Then we expect the alarm to go off, and its handler raises
    # KeyboardInterrupt, finally getting us out of the loop.
    os.system(script)
    try:
        print("starting pause() loop...")
        while 1:
            try:
                if verbose:
                    print("call pause()...")
                signal.pause()
                if verbose:
                    print("pause() returned")
            except HandlerBCalled:
                if verbose:
                    print("HandlerBCalled exception caught")

    except KeyboardInterrupt:
        if verbose:
            print("KeyboardInterrupt (the alarm() went off)")

    if not a_called:
        print('HandlerA not called')

    if not b_called:
        print('HandlerB not called')

finally:
    # Forcibly kill the child we created to ping us if there was a test error.
    try:
        # Make sure we don't kill ourself if there was a fork error.
        if fork_pid > 0:
            os.kill(fork_pid, signal.SIGKILL)
    except:
        # If the child killed us, it has probably exited.  Killing a
        # non-existent process will raise an error which we don't care about.
        pass

    # Restore handlers.
    signal.alarm(0) # cancel alarm in case we died early
    signal.signal(signal.SIGHUP, hup)
    signal.signal(signal.SIGUSR1, usr1)
    signal.signal(signal.SIGUSR2, usr2)
    signal.signal(signal.SIGALRM, alrm)
