# Test just the SSL support in the socket module, in a moderately bogus way.

import sys
import unittest
from test import test_support
import socket
import errno
import threading
import subprocess
import time
import ctypes
import os
import urllib

# Optionally test SSL support, if we have it in the tested platform
skip_expected = not hasattr(socket, "ssl")

class ConnectedTests(unittest.TestCase):

    def testBasic(self):
        socket.RAND_status()
        try:
            socket.RAND_egd(1)
        except TypeError:
            pass
        else:
            print "didn't raise TypeError"
        socket.RAND_add("this is a random string", 75.0)
    
        with test_support.transient_internet():
            f = urllib.urlopen('https://sf.net')
        buf = f.read()
        f.close()
    
    def testTimeout(self):
        def error_msg(extra_msg):
            print >> sys.stderr, """\
        WARNING:  an attempt to connect to %r %s, in
        test_timeout.  That may be legitimate, but is not the outcome we
        hoped for.  If this message is seen often, test_timeout should be
        changed to use a more reliable address.""" % (ADDR, extra_msg)
    
        # A service which issues a welcome banner (without need to write
        # anything).
        # XXX ("gmail.org", 995) has been unreliable so far, from time to
        # XXX time non-responsive for hours on end (& across all buildbot 
        # XXX slaves, so that's not just a local thing).
        ADDR = "gmail.org", 995
    
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

class BasicTests(unittest.TestCase):

    def testRudeShutdown(self):
        # Some random port to connect to.
        PORT = [9934]
    
        listener_ready = threading.Event()
        listener_gone = threading.Event()
    
        # `listener` runs in a thread.  It opens a socket listening on 
        # PORT, and sits in an accept() until the main thread connects.
        # Then it rudely closes the socket, and sets Event `listener_gone`
        # to let the main thread know the socket is gone.
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

class OpenSSLTests(unittest.TestCase):

    def testBasic(self):
        s = socket.socket()
        s.connect(("localhost", 4433))
        ss = socket.ssl(s)
        ss.write("Foo\n")
        i = ss.read(4)
        self.assertEqual(i, "Foo\n")
        s.close()


class OpenSSLServer(threading.Thread):
    def __init__(self):
        self.s = None
        self.keepServing = True
        self._external()
        if self.haveServer:
            threading.Thread.__init__(self)

    def _external(self):
        if os.access("ssl_cert.pem", os.F_OK):
            cert_file = "ssl_cert.pem"
        elif os.access("./Lib/test/ssl_cert.pem", os.F_OK):
            cert_file = "./Lib/test/ssl_cert.pem"
        else:
            raise ValueError("No cert file found!")
        if os.access("ssl_key.pem", os.F_OK):
            key_file = "ssl_key.pem"
        elif os.access("./Lib/test/ssl_key.pem", os.F_OK):
            key_file = "./Lib/test/ssl_key.pem"
        else:
            raise ValueError("No cert file found!")

        try:
            cmd = "openssl s_server -cert %s -key %s -quiet" % (cert_file, key_file)
            self.s = subprocess.Popen(cmd.split(), stdin=subprocess.PIPE, 
                                       stdout=subprocess.PIPE, 
                                       stderr=subprocess.STDOUT)
            time.sleep(1)
        except:
            self.haveServer = False
        else:
            # let's try if it is actually up
            try:
                s = socket.socket()
                s.connect(("localhost", 4433))
                s.close()
                assert self.s.stdout.readline() == "ERROR\n"
            except:
                self.haveServer = False
            else:
                self.haveServer = True
                
    def run(self):
        while self.keepServing:
            time.sleep(.5)
            l = self.s.stdout.readline()
            self.s.stdin.write(l)

    def shutdown(self):
        self.keepServing = False
        if not self.s:
            return
        if sys.platform == "win32":
            handle = ctypes.windll.kernel32.OpenProcess(1, False, self.s.pid)
            ctypes.windll.kernel32.TerminateProcess(handle, -1)
            ctypes.windll.kernel32.CloseHandle(handle)
        else:
            os.kill(self.s.pid, 15)
 
def test_main():
    if not hasattr(socket, "ssl"):
        raise test_support.TestSkipped("socket module has no ssl support")

    tests = [BasicTests]

    if test_support.is_resource_enabled('network'):
        tests.append(ConnectedTests)

    # in these platforms we can kill the openssl process
    if sys.platform in ("sunos5", "darwin", "linux1",
                        "linux2", "win32", "hp-ux11"):

        server = OpenSSLServer()
        if server.haveServer:
            tests.append(OpenSSLTests)
            server.start()
    else:
        server = None

    thread_info = test_support.threading_setup()

    try:
        test_support.run_unittest(*tests)
    finally:
        if server is not None and server.haveServer:
            server.shutdown()

    test_support.threading_cleanup(*thread_info)

if __name__ == "__main__":
    test_main()
