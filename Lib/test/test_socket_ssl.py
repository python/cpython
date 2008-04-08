# Test just the SSL support in the socket module, in a moderately bogus way.

import sys
import unittest
from test import test_support
import socket
import errno
import threading
import subprocess
import time
import os
import urllib
import warnings

warnings.filterwarnings(
    'ignore',
    'socket.ssl.. is deprecated.  Use ssl.wrap_socket.. instead.',
    DeprecationWarning)

# Optionally test SSL support, if we have it in the tested platform
skip_expected = not hasattr(socket, "ssl")

HOST = test_support.HOST

class ConnectedTests(unittest.TestCase):

    def urlopen(self, host, *args, **kwargs):
        # Connecting to remote hosts is flaky.  Make it more robust
        # by retrying the connection several times.
        for i in range(3):
            try:
                return urllib.urlopen(host, *args, **kwargs)
            except IOError, e:
                last_exc = e
                continue
            except:
                raise
        raise last_exc

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
            f = self.urlopen('https://sf.net')
        buf = f.read()
        f.close()

    def testTimeout(self):
        def error_msg(extra_msg):
            print >> sys.stderr, """\
        WARNING:  an attempt to connect to %r %s, in
        testTimeout.  That may be legitimate, but is not the outcome we
        hoped for.  If this message is seen often, testTimeout should be
        changed to use a more reliable address.""" % (ADDR, extra_msg)

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

class BasicTests(unittest.TestCase):

    def testRudeShutdown(self):
        listener_ready = threading.Event()
        listener_gone = threading.Event()
        sock = socket.socket()
        port = test_support.bind_port(sock)

        # `listener` runs in a thread.  It opens a socket and sits in accept()
        # until the main thread connects.  Then it rudely closes the socket,
        # and sets Event `listener_gone` to let the main thread know the socket
        # is gone.
        def listener(s):
            s.listen(5)
            listener_ready.set()
            s.accept()
            s = None # reclaim the socket object, which also closes it
            listener_gone.set()

        def connector():
            listener_ready.wait()
            s = socket.socket()
            s.connect((HOST, port))
            listener_gone.wait()
            try:
                ssl_sock = socket.ssl(s)
            except socket.sslerror:
                pass
            else:
                raise test_support.TestFailed(
                      'connecting to closed SSL socket should have failed')

        t = threading.Thread(target=listener, args=(sock,))
        t.start()
        connector()
        t.join()

    def connect(self, s, host_port):
        # Connecting to remote hosts is flaky.  Make it more robust
        # by retrying the connection several times.
        for i in range(3):
            try:
                return s.connect(host_port)
            except IOError, e:
                last_exc = e
                continue
            except:
                raise
        raise last_exc

    def test_978833(self):
        if not test_support.is_resource_enabled("network"):
            return
        if test_support.verbose:
            print "test_978833 ..."

        import os, httplib, ssl
        with test_support.transient_internet():
            s = socket.socket(socket.AF_INET)
            try:
                self.connect(s, ("svn.python.org", 443))
            except IOError:
                print >> sys.stderr, """\
        WARNING:  an attempt to connect to svn.python.org:443 failed, in
        test_978833.  That may be legitimate, but is not the outcome we
        hoped for.  If this message is seen often, test_978833 should be
        changed to use a more reliable address."""
                return
            fd = s._sock.fileno()
            sock = ssl.wrap_socket(s)
            s = None
            sock.close()
            try:
                os.fstat(fd)
            except OSError:
                pass
            else:
                raise test_support.TestFailed("Failed to close socket")

class OpenSSLTests(unittest.TestCase):

    def testBasic(self):
        s = socket.socket()
        s.connect((HOST, OpenSSLServer.PORT))
        ss = socket.ssl(s)
        ss.write("Foo\n")
        i = ss.read(4)
        self.assertEqual(i, "Foo\n")
        s.close()

    def testMethods(self):
        # read & write is already tried in the Basic test
        # now we'll try to get the server info about certificates
        # this came from the certificate I used, one I found in /usr/share/openssl
        info = "/C=PT/ST=Queensland/L=Lisboa/O=Neuronio, Lda./OU=Desenvolvimento/CN=brutus.neuronio.pt/emailAddress=sampo@iki.fi"

        s = socket.socket()
        s.connect((HOST, OpenSSLServer.PORT))
        ss = socket.ssl(s)
        cert = ss.server()
        self.assertEqual(cert, info)
        cert = ss.issuer()
        self.assertEqual(cert, info)
        s.close()


class OpenSSLServer(threading.Thread):
    PORT = None
    def __init__(self):
        self.s = None
        self.keepServing = True
        self._external()
        if self.haveServer:
            threading.Thread.__init__(self)

    def _external(self):
        # let's find the .pem files
        curdir = os.path.dirname(__file__) or os.curdir
        cert_file = os.path.join(curdir, "ssl_cert.pem")
        if not os.access(cert_file, os.F_OK):
            raise ValueError("No cert file found! (tried %r)" % cert_file)
        key_file = os.path.join(curdir, "ssl_key.pem")
        if not os.access(key_file, os.F_OK):
            raise ValueError("No key file found! (tried %r)" % key_file)

        try:
            # XXX TODO: on Windows, this should make more effort to use the
            # openssl.exe that would have been built by the pcbuild.sln.
            self.PORT = test_support.find_unused_port()
            args = (self.PORT, cert_file, key_file)
            cmd = "openssl s_server -accept %d -cert %s -key %s -quiet" % args
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
                s.connect((HOST, self.PORT))
                s.close()
                if self.s.stdout.readline() != "ERROR\n":
                    raise ValueError
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
            subprocess.TerminateProcess(int(self.s._handle), -1)
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
