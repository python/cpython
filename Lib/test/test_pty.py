import pty, os, sys, string
from test_support import verbose, TestFailed, TestSkipped

TEST_STRING_1 = "I wish to buy a fish license."
TEST_STRING_2 = "For my pet fish, Eric."
TEST_STRING_3 = "And now for something completely different..."
TEST_STRING_4 = "but you pronounce it throatwobbler mangrove."

if verbose:
    def debug(msg):
        print msg
else:
    def debug(msg):
        pass

# Marginal testing of pty suite. Cannot do extensive 'do or fail' testing
# because pty code is not too portable.

try:
    debug("Calling master_open()")
    master_fd, slave_name = pty.master_open()
    debug("Got master_fd '%d', slave_name '%s'"%(master_fd, slave_name))
    debug("Calling slave_open(%s)"%`slave_name`)
    slave_fd = pty.slave_open(slave_name)
    debug("Got slave_fd '%d'"%slave_fd)
except OSError:
    # " An optional feature could not be imported " ... ?
    raise TestSkipped, "Pseudo-terminals (seemingly) not functional."

if not os.isatty(slave_fd):
    raise TestFailed, "slave_fd is not a tty"

debug("Writing to slave_fd")
os.write(slave_fd, TEST_STRING_1) # should check return value
print os.read(master_fd, 1024)

os.write(slave_fd, TEST_STRING_2[:5])
os.write(slave_fd, TEST_STRING_2[5:])
print os.read(master_fd, 1024)

os.close(slave_fd)
os.close(master_fd)

# basic pty passed.

debug("calling pty.fork()")
pid, master_fd = pty.fork()
if pid == pty.CHILD:
    ## # Please uncomment these when os.isatty() is added.
    ## if not os.isatty(1):
    ##     debug("Child's fd 1 is not a tty?!")
    ##     os._exit(3)
    try:
        debug("In child, calling os.setsid()")
        os.setsid()
    except OSError:
        # Good, we already were session leader
        debug("OSError was raised.")
        pass
    except AttributeError:
        # Have pty, but not setsid() ?
        debug("AttributeError was raised.")
        pass
    except:
        # We don't want this error to propagate, escape the call to
        # os._exit(), and cause very peculiar behavior in the calling
        # regrtest.py !
        debug("Some other error was raised.")
        os._exit(1)
    else:
        debug("os.setsid() succeeded! (bad!)")
        os._exit(2)
    os._exit(4)
else:
    debug("Waiting for child (%d) to finish."%pid)
    (pid, status) = os.waitpid(pid, 0)
    debug("Child (%d) exited with status %d."%(pid, status))
    if status / 256 == 1:
        raise TestFailed, "Child raised an unexpected exception in os.setsid()"
    elif status / 256 == 2:
        raise TestFailed, "pty.fork() failed to make child a session leader."
    elif status / 256 == 3:
        raise TestFailed, "Child spawned by pty.fork() did not have a tty as stdout"
    elif status / 256 <> 4:
        raise TestFailed, "pty.fork() failed for unknown reasons:"
        print os.read(master_fd, 65536)

os.close(master_fd)

# pty.fork() passed.

