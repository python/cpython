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

def normalize_output(data):
    # Some operating systems do conversions on newline.  We could possibly
    # fix that by doing the appropriate termios.tcsetattr()s.  I couldn't
    # figure out the right combo on Tru64 and I don't have an IRIX box.
    # So just normalize the output and doc the problem O/Ses by allowing
    # certain combinations for some platforms, but avoid allowing other
    # differences (like extra whitespace, trailing garbage, etc.)

    # This is about the best we can do without getting some feedback
    # from someone more knowledgable.

    # OSF/1 (Tru64) apparently turns \n into \r\r\n.
    if data.endswith('\r\r\n'):
        return data.replace('\r\r\n', '\n')

    # IRIX apparently turns \n into \r\n.
    if data.endswith('\r\n'):
        return data.replace('\r\n', '\n')

    return data

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

    debug("Writing to slave_fd")
    os.write(slave_fd, TEST_STRING_1)
    s1 = os.read(master_fd, 1024)
    sys.stdout.write(normalize_output(s1))

    debug("Writing chunked output")
    os.write(slave_fd, TEST_STRING_2[:5])
    os.write(slave_fd, TEST_STRING_2[5:])
    s2 = os.read(master_fd, 1024)
    sys.stdout.write(normalize_output(s2))

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
    # In verbose mode, we have to consume the debug output from the child or
    # the child will block, causing this test to hang in the parent's
    # waitpid() call.  The child blocks after a platform-dependent amount of
    # data is written to its fd.  On Linux 2.6, it's 4000 bytes and the child
    # won't block, but on OS X even the small writes in the child above will
    # block it.  Also on Linux, the read() will throw an OSError (input/output
    # error) when it tries to read past the end of the buffer but the child's
    # already exited, so catch and discard those exceptions.  It's not worth
    # checking for EIO.
    while True:
        try:
            data = os.read(master_fd, 80)
        except OSError:
            break
        if not data:
            break
        sys.stdout.write(data.replace('\r\n', '\n'))

    ##line = os.read(master_fd, 80)
    ##lines = line.replace('\r\n', '\n').split('\n')
    ##if False and lines != ['In child, calling os.setsid()',
    ##             'Good: OSError was raised.', '']:
    ##    raise TestFailed("Unexpected output from child: %r" % line)

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

    ##debug("Reading from master_fd now that the child has exited")
    ##try:
    ##    s1 = os.read(master_fd, 1024)
    ##except os.error:
    ##    pass
    ##else:
    ##    raise TestFailed("Read from master_fd did not raise exception")


os.close(master_fd)

# pty.fork() passed.
