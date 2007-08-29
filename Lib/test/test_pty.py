import errno
import fcntl
import pty
import os
import sys
import signal
from test.test_support import verbose, TestSkipped, run_unittest
import unittest

TEST_STRING_1 = "I wish to buy a fish license.\n"
TEST_STRING_2 = "For my pet fish, Eric.\n"

if verbose:
    def debug(msg):
        print(msg)
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
    if data.endswith(b'\r\r\n'):
        return data.replace(b'\r\r\n', b'\n')

    # IRIX apparently turns \n into \r\n.
    if data.endswith(b'\r\n'):
        return data.replace(b'\r\n', b'\n')

    return data


# Marginal testing of pty suite. Cannot do extensive 'do or fail' testing
# because pty code is not too portable.
# XXX(nnorwitz):  these tests leak fds when there is an error.
class PtyTest(unittest.TestCase):
    def setUp(self):
        # isatty() and close() can hang on some platforms.  Set an alarm
        # before running the test to make sure we don't hang forever.
        self.old_alarm = signal.signal(signal.SIGALRM, self.handle_sig)
        signal.alarm(10)

    def tearDown(self):
        # remove alarm, restore old alarm handler
        signal.alarm(0)
        signal.signal(signal.SIGALRM, self.old_alarm)

    def handle_sig(self, sig, frame):
        self.fail("isatty hung")

    def test_basic(self):
        try:
            debug("Calling master_open()")
            master_fd, slave_name = pty.master_open()
            debug("Got master_fd '%d', slave_name '%s'" %
                  (master_fd, slave_name))
            debug("Calling slave_open(%r)" % (slave_name,))
            slave_fd = pty.slave_open(slave_name)
            debug("Got slave_fd '%d'" % slave_fd)
        except OSError:
            # " An optional feature could not be imported " ... ?
            raise TestSkipped("Pseudo-terminals (seemingly) not functional.")

        self.assertTrue(os.isatty(slave_fd), 'slave_fd is not a tty')

        # Solaris requires reading the fd before anything is returned.
        # My guess is that since we open and close the slave fd
        # in master_open(), we need to read the EOF.

        # Ensure the fd is non-blocking in case there's nothing to read.
        orig_flags = fcntl.fcntl(master_fd, fcntl.F_GETFL)
        fcntl.fcntl(master_fd, fcntl.F_SETFL, orig_flags | os.O_NONBLOCK)
        try:
            s1 = os.read(master_fd, 1024)
            self.assertEquals(b'', s1)
        except OSError as e:
            if e.errno != errno.EAGAIN:
                raise
        # Restore the original flags.
        fcntl.fcntl(master_fd, fcntl.F_SETFL, orig_flags)

        debug("Writing to slave_fd")
        os.write(slave_fd, TEST_STRING_1)
        s1 = os.read(master_fd, 1024)
        self.assertEquals(b'I wish to buy a fish license.\n',
                          normalize_output(s1))

        debug("Writing chunked output")
        os.write(slave_fd, TEST_STRING_2[:5])
        os.write(slave_fd, TEST_STRING_2[5:])
        s2 = os.read(master_fd, 1024)
        self.assertEquals(b'For my pet fish, Eric.\n', normalize_output(s2))

        os.close(slave_fd)
        os.close(master_fd)


    def test_fork(self):
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
                # Have pty, but not setsid()?
                debug("No setsid() available?")
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
            debug("Waiting for child (%d) to finish." % pid)
            # In verbose mode, we have to consume the debug output from the
            # child or the child will block, causing this test to hang in the
            # parent's waitpid() call.  The child blocks after a
            # platform-dependent amount of data is written to its fd.  On
            # Linux 2.6, it's 4000 bytes and the child won't block, but on OS
            # X even the small writes in the child above will block it.  Also
            # on Linux, the read() will throw an OSError (input/output error)
            # when it tries to read past the end of the buffer but the child's
            # already exited, so catch and discard those exceptions.  It's not
            # worth checking for EIO.
            while True:
                try:
                    data = os.read(master_fd, 80)
                except OSError:
                    break
                if not data:
                    break
                sys.stdout.write(data.replace('\r\n', '\n').decode('ascii'))

            ##line = os.read(master_fd, 80)
            ##lines = line.replace('\r\n', '\n').split('\n')
            ##if False and lines != ['In child, calling os.setsid()',
            ##             'Good: OSError was raised.', '']:
            ##    raise TestFailed("Unexpected output from child: %r" % line)

            (pid, status) = os.waitpid(pid, 0)
            res = status >> 8
            debug("Child (%d) exited with status %d (%d)." % (pid, res, status))
            if res == 1:
                self.fail("Child raised an unexpected exception in os.setsid()")
            elif res == 2:
                self.fail("pty.fork() failed to make child a session leader.")
            elif res == 3:
                self.fail("Child spawned by pty.fork() did not have a tty as stdout")
            elif res != 4:
                self.fail("pty.fork() failed for unknown reasons.")

            ##debug("Reading from master_fd now that the child has exited")
            ##try:
            ##    s1 = os.read(master_fd, 1024)
            ##except os.error:
            ##    pass
            ##else:
            ##    raise TestFailed("Read from master_fd did not raise exception")

        os.close(master_fd)

        # pty.fork() passed.

def test_main(verbose=None):
    run_unittest(PtyTest)

if __name__ == "__main__":
    test_main()
