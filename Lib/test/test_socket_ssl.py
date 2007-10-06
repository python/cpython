# Test just the SSL support in the socket module, in a moderately bogus way.

import sys
from test import test_support
import socket
import errno

# Optionally test SSL support.  This requires the 'network' resource as given
# on the regrtest command line.
skip_expected = not (test_support.is_resource_enabled('network') and
                     hasattr(socket, "ssl"))

def test_basic():
    test_support.requires('network')

    import urllib

    if test_support.verbose:
        print "test_basic ..."

    socket.RAND_status()
    try:
        socket.RAND_egd(1)
    except TypeError:
        pass
    else:
        print "didn't raise TypeError"
    socket.RAND_add("this is a random string", 75.0)

    try:
        f = urllib.urlopen('https://sf.net')
    except IOError, exc:
        if exc.errno == errno.ETIMEDOUT:
            raise test_support.ResourceDenied('HTTPS connection is timing out')
        else:
            raise
    buf = f.read()
    f.close()

def test_timeout():
    test_support.requires('network')

    def error_msg(extra_msg):
        print >> sys.stderr, """\
    WARNING:  an attempt to connect to %r %s, in
    test_timeout.  That may be legitimate, but is not the outcome we hoped
    for.  If this message is seen often, test_timeout should be changed to
    use a more reliable address.""" % (ADDR, extra_msg)

    if test_support.verbose:
        print "test_timeout ..."

    # A service which issues a welcome banner (without need to write
    # anything).
    ADDR = "pop.gmail.com", 995

    s = socket.socket()
    s.settimeout(30.0)
    try:
        s.connect(ADDR)
    except socket.timeout:
        error_msg('timed out')
        return
    except socket.error, exc:  # In case connection is refused.
        if exc.args[0] == errno.ECONNREFUSED:
            error_msg('was refused')
            return
        else:
            raise

    ss = socket.ssl(s)
    # Read part of return welcome banner twice.
    ss.read(1)
    ss.read(1)
    s.close()

def test_rude_shutdown():
    if test_support.verbose:
        print "test_rude_shutdown ..."

    try:
        import threading
    except ImportError:
        return

    # Some random port to connect to.
    PORT = [9934]

    listener_ready = threading.Event()
    listener_gone = threading.Event()

    # `listener` runs in a thread.  It opens a socket listening on PORT, and
    # sits in an accept() until the main thread connects.  Then it rudely
    # closes the socket, and sets Event `listener_gone` to let the main thread
    # know the socket is gone.
    def listener():
        s = socket.socket()
        PORT[0] = test_support.bind_port(s, '', PORT[0])
        s.listen(5)
        listener_ready.set()
        s.accept()
        s = None # reclaim the socket object, which also closes it
        listener_gone.set()

    def connector():
        listener_ready.wait()
        s = socket.socket()
        s.connect(('localhost', PORT[0]))
        listener_gone.wait()
        try:
            ssl_sock = socket.ssl(s)
        except socket.sslerror:
            pass
        else:
            raise test_support.TestFailed(
                      'connecting to closed SSL socket should have failed')

    t = threading.Thread(target=listener)
    t.start()
    connector()
    t.join()

def test_main():
    if not hasattr(socket, "ssl"):
        raise test_support.TestSkipped("socket module has no ssl support")
    test_rude_shutdown()
    test_basic()
    test_timeout()

if __name__ == "__main__":
    test_main()
