# Test just the SSL support in the socket module, in a moderately bogus way.

import sys
from test import test_support
import socket

# Optionally test SSL support.  This requires the 'network' resource as given
# on the regrtest command line.
skip_expected = not (test_support.is_resource_enabled('network') and
                     hasattr(socket, "ssl"))

def test_basic():
    test_support.requires('network')

    import urllib

    socket.RAND_status()
    try:
        socket.RAND_egd(1)
    except TypeError:
        pass
    else:
        print "didn't raise TypeError"
    socket.RAND_add("this is a random string", 75.0)

    f = urllib.urlopen('https://sf.net')
    buf = f.read()
    f.close()

# XXX Tim disabled this test on all platforms, for now, since the
# XXX s.connect(("gmail.org", 995))
# XXX line starting timing out on all the builbot slaves.
if 0: not sys.platform.startswith('win'):
    def test_timeout():
        test_support.requires('network')

        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(30.0)
        # connect to service which issues an welcome banner (without need to
        # write anything)
        s.connect(("gmail.org", 995))
        ss = socket.ssl(s)
        # read part of return welcome banner twice
        ss.read(1)
        ss.read(1)
        s.close()
else:
    def test_timeout():
        pass

def test_rude_shutdown():
    try:
        import threading
    except ImportError:
        return

    # Some random port to connect to.
    PORT = 9934

    listener_ready = threading.Event()
    listener_gone = threading.Event()

    # `listener` runs in a thread.  It opens a socket listening on PORT, and
    # sits in an accept() until the main thread connects.  Then it rudely
    # closes the socket, and sets Event `listener_gone` to let the main thread
    # know the socket is gone.
    def listener():
        s = socket.socket()
        s.bind(('', PORT))
        s.listen(5)
        listener_ready.set()
        s.accept()
        s = None # reclaim the socket object, which also closes it
        listener_gone.set()

    def connector():
        listener_ready.wait()
        s = socket.socket()
        s.connect(('localhost', PORT))
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
