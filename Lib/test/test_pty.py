import pty, os, sys, signal
from test.test_support import verbose, TestFailed, TestSkipped

TEST_STRING_1 = "I wish to buy a fish license.\n"
TEST_STRING_2 = "For my pet fish, Eric.\n"

if verbose:
    def debug(msg):
        print msg
else:
    def debug(msg):
        pass

# Marginal testing of pty suite. Cannot do extensive 'do or fail' testing
# because pty code is not too portable.

def test_basic_pty():
    try:
        debug("Calling master_open()")
        master_fd, slave_name = pty.master_open()
        debug("Got master_fd '%d', slave_name '%s'"%(master_fd, slave_name))
        debug("Calling slave_open(%r)"%(slave_name,))
        slave_fd = pty.slave_open(slave_name)
        debug("Got slave_fd '%d'"%slave_fd)
    except OSError:
        # " An optional feature could not be imported " ... ?
        raise TestSkipped, "Pseudo-terminals (seemingly) not functional."

    if not os.isatty(slave_fd):
        raise TestFailed, "slave_fd is not a tty"

    # IRIX apparently turns \n into \r\n. Allow that, but avoid allowing other
    # differences (like extra whitespace, trailing garbage, etc.)

    debug("Writing to slave_fd")
    os.write(slave_fd, TEST_STRING_1)
    s1 = os.read(master_fd, 1024)
    sys.stdout.write(s1.replace("\r\n", "\n"))

    debug("Writing chunked output")
    os.write(slave_fd, TEST_STRING_2[:5])
    os.write(slave_fd, TEST_STRING_2[5:])
    s2 = os.read(master_fd, 1024)
    sys.stdout.write(s2.replace("\r\n", "\n"))

    os.close(slave_fd)
    os.close(master_fd)

def handle_sig(sig, frame):
    raise TestFailed, "isatty hung"

# isatty() and close() can hang on some platforms
# set an alarm before running the test to make sure we don't hang forever
old_alarm = signal.signal(signal.SIGALRM, handle_sig)
signal.alarm(10)

try:
    test_basic_pty()
finally:
    # remove alarm, restore old alarm handler
    signal.alarm(0)
    signal.signal(signal.SIGALRM, old_alarm)

# basic pty passed.

debug("calling pty.fork()")
pid, master_fd = pty.fork()
if pid == pty.CHILD:
    # stdout should be connected to a tty.
    if not os.isatty(1):
        debug("Child's fd 1 is not a tty?!")
        os._exit(3)

    # After pty.fork(), the child should already be a session leader.
    # (on those systems that have that concept.)
    debug("In child, calling os.setsid()")
    try:
        os.setsid()
    except OSError:
        # Good, we already were session leader
        debug("Good: OSError was raised.")
        pass
    except AttributeError:
        # Have pty, but not setsid() ?
        debug("No setsid() available ?")
        pass
    except:
        # We don't want this error to propagate, escaping the call to
        # os._exit() and causing very peculiar behavior in the calling
        # regrtest.py !
        # Note: could add traceback printing here.
        debug("An unexpected error was raised.")
        os._exit(1)
    else:
        debug("os.setsid() succeeded! (bad!)")
        os._exit(2)
    os._exit(4)
else:
    debug("Waiting for child (%d) to finish."%pid)
    (pid, status) = os.waitpid(pid, 0)
    res = status >> 8
    debug("Child (%d) exited with status %d (%d)."%(pid, res, status))
    if res == 1:
        raise TestFailed, "Child raised an unexpected exception in os.setsid()"
    elif res == 2:
        raise TestFailed, "pty.fork() failed to make child a session leader."
    elif res == 3:
        raise TestFailed, "Child spawned by pty.fork() did not have a tty as stdout"
    elif res != 4:
        raise TestFailed, "pty.fork() failed for unknown reasons."

os.close(master_fd)

# pty.fork() passed.
