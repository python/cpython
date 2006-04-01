# Test just the SSL support in the socket module, in a moderately bogus way.

import sys
from test import test_support
import socket
import time

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
if 0: #not sys.platform.startswith('win'):
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
        import thread
    except ImportError:
        return

    # some random port to connect to
    PORT = 9934
    def listener():
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind(('', PORT))
        s.listen(5)
        s.accept()
        del s
        thread.exit()

    def connector():
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(('localhost', PORT))
        try:
            ssl_sock = socket.ssl(s)
        except socket.sslerror:
            pass
        else:
            raise test_support.TestFailed, \
                        'connecting to closed SSL socket failed'

    thread.start_new_thread(listener, ())
    time.sleep(1)
    connector()

def test_main():
    if not hasattr(socket, "ssl"):
        raise test_support.TestSkipped("socket module has no ssl support")
    test_rude_shutdown()
    test_basic()
    test_timeout()

if __name__ == "__main__":
    test_main()
