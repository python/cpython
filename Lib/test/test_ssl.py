# Test the support for SSL and sockets

import sys
import unittest
from test import test_support
import socket
import errno
import subprocess
import time
import os
import pprint
import urllib
import shutil
import traceback

# Optionally test SSL support, if we have it in the tested platform
skip_expected = False
try:
    import ssl
except ImportError:
    skip_expected = True

CERTFILE = None

TESTPORT = 10025

def handle_error(prefix):
    exc_format = ' '.join(traceback.format_exception(*sys.exc_info()))
    if test_support.verbose:
        sys.stdout.write(prefix + exc_format)


class BasicTests(unittest.TestCase):

    def testSSLconnect(self):
        import os
        with test_support.transient_internet():
            s = ssl.wrap_socket(socket.socket(socket.AF_INET),
                                cert_reqs=ssl.CERT_NONE)
            s.connect(("pop.gmail.com", 995))
            c = s.getpeercert()
            if c:
                raise test_support.TestFailed("Peer cert %s shouldn't be here!")
            s.close()

            # this should fail because we have no verification certs
            s = ssl.wrap_socket(socket.socket(socket.AF_INET),
                                cert_reqs=ssl.CERT_REQUIRED)
            try:
                s.connect(("pop.gmail.com", 995))
            except ssl.SSLError:
                pass
            finally:
                s.close()

    def testCrucialConstants(self):
        ssl.PROTOCOL_SSLv2
        ssl.PROTOCOL_SSLv23
        ssl.PROTOCOL_SSLv3
        ssl.PROTOCOL_TLSv1
        ssl.CERT_NONE
        ssl.CERT_OPTIONAL
        ssl.CERT_REQUIRED

    def testRAND(self):
        v = ssl.RAND_status()
        if test_support.verbose:
            sys.stdout.write("\n RAND_status is %d (%s)\n"
                             % (v, (v and "sufficient randomness") or
                                "insufficient randomness"))
        try:
            ssl.RAND_egd(1)
        except TypeError:
            pass
        else:
            print "didn't raise TypeError"
        ssl.RAND_add("this is a random string", 75.0)

    def testParseCert(self):
        # note that this uses an 'unofficial' function in _ssl.c,
        # provided solely for this test, to exercise the certificate
        # parsing code
        p = ssl._ssl._test_decode_cert(CERTFILE, False)
        if test_support.verbose:
            sys.stdout.write("\n" + pprint.pformat(p) + "\n")

try:
    import threading
except ImportError:
    _have_threads = False
else:

    _have_threads = True

    class ThreadedEchoServer(threading.Thread):

        class ConnectionHandler(threading.Thread):

            """A mildly complicated class, because we want it to work both
            with and without the SSL wrapper around the socket connection, so
            that we can test the STARTTLS functionality."""

            def __init__(self, server, connsock):
                self.server = server
                self.running = False
                self.sock = connsock
                self.sock.setblocking(1)
                self.sslconn = None
                threading.Thread.__init__(self)
                self.setDaemon(True)

            def wrap_conn (self):
                try:
                    self.sslconn = ssl.wrap_socket(self.sock, server_side=True,
                                                   certfile=self.server.certificate,
                                                   ssl_version=self.server.protocol,
                                                   ca_certs=self.server.cacerts,
                                                   cert_reqs=self.server.certreqs)
                except:
                    if self.server.chatty:
                        handle_error("\n server:  bad connection attempt from " +
                                     str(self.sock.getpeername()) + ":\n")
                    if not self.server.expect_bad_connects:
                        # here, we want to stop the server, because this shouldn't
                        # happen in the context of our test case
                        self.running = False
                        # normally, we'd just stop here, but for the test
                        # harness, we want to stop the server
                        self.server.stop()
                    return False

                else:
                    if self.server.certreqs == ssl.CERT_REQUIRED:
                        cert = self.sslconn.getpeercert()
                        if test_support.verbose and self.server.chatty:
                            sys.stdout.write(" client cert is " + pprint.pformat(cert) + "\n")
                        cert_binary = self.sslconn.getpeercert(True)
                        if test_support.verbose and self.server.chatty:
                            sys.stdout.write(" cert binary is " + str(len(cert_binary)) + " bytes\n")
                    cipher = self.sslconn.cipher()
                    if test_support.verbose and self.server.chatty:
                        sys.stdout.write(" server: connection cipher is now " + str(cipher) + "\n")
                    return True

            def read(self):
                if self.sslconn:
                    return self.sslconn.read()
                else:
                    return self.sock.recv(1024)

            def write(self, bytes):
                if self.sslconn:
                    return self.sslconn.write(bytes)
                else:
                    return self.sock.send(bytes)

            def close(self):
                if self.sslconn:
                    self.sslconn.close()
                else:
                    self.sock.close()

            def run (self):
                self.running = True
                if not self.server.starttls_server:
                    if not self.wrap_conn():
                        return
                while self.running:
                    try:
                        msg = self.read()
                        if not msg:
                            # eof, so quit this handler
                            self.running = False
                            self.close()
                        elif msg.strip() == 'over':
                            if test_support.verbose and self.server.connectionchatty:
                                sys.stdout.write(" server: client closed connection\n")
                            self.close()
                            return
                        elif self.server.starttls_server and msg.strip() == 'STARTTLS':
                            if test_support.verbose and self.server.connectionchatty:
                                sys.stdout.write(" server: read STARTTLS from client, sending OK...\n")
                            self.write("OK\n")
                            if not self.wrap_conn():
                                return
                        else:
                            if (test_support.verbose and
                                self.server.connectionchatty):
                                ctype = (self.sslconn and "encrypted") or "unencrypted"
                                sys.stdout.write(" server: read %s (%s), sending back %s (%s)...\n"
                                                 % (repr(msg), ctype, repr(msg.lower()), ctype))
                            self.write(msg.lower())
                    except ssl.SSLError:
                        if self.server.chatty:
                            handle_error("Test server failure:\n")
                        self.close()
                        self.running = False
                        # normally, we'd just stop here, but for the test
                        # harness, we want to stop the server
                        self.server.stop()
                    except:
                        handle_error('')

        def __init__(self, port, certificate, ssl_version=None,
                     certreqs=None, cacerts=None, expect_bad_connects=False,
                     chatty=True, connectionchatty=False, starttls_server=False):
            if ssl_version is None:
                ssl_version = ssl.PROTOCOL_TLSv1
            if certreqs is None:
                certreqs = ssl.CERT_NONE
            self.certificate = certificate
            self.protocol = ssl_version
            self.certreqs = certreqs
            self.cacerts = cacerts
            self.expect_bad_connects = expect_bad_connects
            self.chatty = chatty
            self.connectionchatty = connectionchatty
            self.starttls_server = starttls_server
            self.sock = socket.socket()
            self.flag = None
            if hasattr(socket, 'SO_REUSEADDR'):
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            if hasattr(socket, 'SO_REUSEPORT'):
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
            self.sock.bind(('127.0.0.1', port))
            self.active = False
            threading.Thread.__init__(self)
            self.setDaemon(False)

        def start (self, flag=None):
            self.flag = flag
            threading.Thread.start(self)

        def run (self):
            self.sock.settimeout(0.5)
            self.sock.listen(5)
            self.active = True
            if self.flag:
                # signal an event
                self.flag.set()
            while self.active:
                try:
                    newconn, connaddr = self.sock.accept()
                    if test_support.verbose and self.chatty:
                        sys.stdout.write(' server:  new connection from '
                                         + str(connaddr) + '\n')
                    handler = self.ConnectionHandler(self, newconn)
                    handler.start()
                except socket.timeout:
                    pass
                except KeyboardInterrupt:
                    self.stop()
                except:
                    if self.chatty:
                        handle_error("Test server failure:\n")

        def stop (self):
            self.active = False
            self.sock.close()

    def badCertTest (certfile):
        server = ThreadedEchoServer(TESTPORT, CERTFILE,
                                    certreqs=ssl.CERT_REQUIRED,
                                    cacerts=CERTFILE, chatty=False)
        flag = threading.Event()
        server.start(flag)
        # wait for it to start
        flag.wait()
        # try to connect
        try:
            try:
                s = ssl.wrap_socket(socket.socket(),
                                    certfile=certfile,
                                    ssl_version=ssl.PROTOCOL_TLSv1)
                s.connect(('127.0.0.1', TESTPORT))
            except ssl.SSLError, x:
                if test_support.verbose:
                    sys.stdout.write("\nSSLError is %s\n" % x[1])
            else:
                raise test_support.TestFailed(
                    "Use of invalid cert should have failed!")
        finally:
            server.stop()
            server.join()

    def serverParamsTest (certfile, protocol, certreqs, cacertsfile,
                          client_certfile, client_protocol=None, indata="FOO\n",
                          chatty=True, connectionchatty=False):

        server = ThreadedEchoServer(TESTPORT, certfile,
                                    certreqs=certreqs,
                                    ssl_version=protocol,
                                    cacerts=cacertsfile,
                                    chatty=chatty,
                                    connectionchatty=connectionchatty)
        flag = threading.Event()
        server.start(flag)
        # wait for it to start
        flag.wait()
        # try to connect
        if client_protocol is None:
            client_protocol = protocol
        try:
            try:
                s = ssl.wrap_socket(socket.socket(),
                                    certfile=client_certfile,
                                    ca_certs=cacertsfile,
                                    cert_reqs=certreqs,
                                    ssl_version=client_protocol)
                s.connect(('127.0.0.1', TESTPORT))
            except ssl.SSLError, x:
                raise test_support.TestFailed("Unexpected SSL error:  " + str(x))
            except Exception, x:
                raise test_support.TestFailed("Unexpected exception:  " + str(x))
            else:
                if connectionchatty:
                    if test_support.verbose:
                        sys.stdout.write(
                            " client:  sending %s...\n" % (repr(indata)))
                s.write(indata)
                outdata = s.read()
                if connectionchatty:
                    if test_support.verbose:
                        sys.stdout.write(" client:  read %s\n" % repr(outdata))
                if outdata != indata.lower():
                    raise test_support.TestFailed(
                        "bad data <<%s>> (%d) received; expected <<%s>> (%d)\n"
                        % (outdata[:min(len(outdata),20)], len(outdata),
                           indata[:min(len(indata),20)].lower(), len(indata)))
                s.write("over\n")
                if connectionchatty:
                    if test_support.verbose:
                        sys.stdout.write(" client:  closing connection.\n")
                s.ssl_shutdown()
                s.close()
        finally:
            server.stop()
            server.join()

    def tryProtocolCombo (server_protocol,
                          client_protocol,
                          expectedToWork,
                          certsreqs=ssl.CERT_NONE):

        if certsreqs == ssl.CERT_NONE:
            certtype = "CERT_NONE"
        elif certsreqs == ssl.CERT_OPTIONAL:
            certtype = "CERT_OPTIONAL"
        elif certsreqs == ssl.CERT_REQUIRED:
            certtype = "CERT_REQUIRED"
        if test_support.verbose:
            formatstr = (expectedToWork and " %s->%s %s\n") or " {%s->%s} %s\n"
            sys.stdout.write(formatstr %
                             (ssl.get_protocol_name(client_protocol),
                              ssl.get_protocol_name(server_protocol),
                              certtype))
        try:
            serverParamsTest(CERTFILE, server_protocol, certsreqs,
                             CERTFILE, CERTFILE, client_protocol, chatty=False)
        except test_support.TestFailed:
            if expectedToWork:
                raise
        else:
            if not expectedToWork:
                raise test_support.TestFailed(
                    "Client protocol %s succeeded with server protocol %s!"
                    % (ssl.get_protocol_name(client_protocol),
                       ssl.get_protocol_name(server_protocol)))


    class ConnectedTests(unittest.TestCase):

        def testRudeShutdown(self):

            listener_ready = threading.Event()
            listener_gone = threading.Event()

            # `listener` runs in a thread.  It opens a socket listening on
            # PORT, and sits in an accept() until the main thread connects.
            # Then it rudely closes the socket, and sets Event `listener_gone`
            # to let the main thread know the socket is gone.
            def listener():
                s = socket.socket()
                if hasattr(socket, 'SO_REUSEADDR'):
                    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                if hasattr(socket, 'SO_REUSEPORT'):
                    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
                port = test_support.bind_port(s, '127.0.0.1', TESTPORT)
                s.listen(5)
                listener_ready.set()
                s.accept()
                s = None # reclaim the socket object, which also closes it
                listener_gone.set()

            def connector():
                listener_ready.wait()
                s = socket.socket()
                s.connect(('127.0.0.1', TESTPORT))
                listener_gone.wait()
                try:
                    ssl_sock = ssl.wrap_socket(s)
                except socket.sslerror:
                    pass
                else:
                    raise test_support.TestFailed(
                          'connecting to closed SSL socket should have failed')

            t = threading.Thread(target=listener)
            t.start()
            connector()
            t.join()

        def testEcho (self):

            if test_support.verbose:
                sys.stdout.write("\n")
            serverParamsTest(CERTFILE, ssl.PROTOCOL_TLSv1, ssl.CERT_NONE,
                             CERTFILE, CERTFILE, ssl.PROTOCOL_TLSv1,
                             chatty=True, connectionchatty=True)

        def testReadCert(self):

            if test_support.verbose:
                sys.stdout.write("\n")
            s2 = socket.socket()
            server = ThreadedEchoServer(TESTPORT, CERTFILE,
                                        certreqs=ssl.CERT_NONE,
                                        ssl_version=ssl.PROTOCOL_SSLv23,
                                        cacerts=CERTFILE,
                                        chatty=False)
            flag = threading.Event()
            server.start(flag)
            # wait for it to start
            flag.wait()
            # try to connect
            try:
                try:
                    s = ssl.wrap_socket(socket.socket(),
                                        certfile=CERTFILE,
                                        ca_certs=CERTFILE,
                                        cert_reqs=ssl.CERT_REQUIRED,
                                        ssl_version=ssl.PROTOCOL_SSLv23)
                    s.connect(('127.0.0.1', TESTPORT))
                except ssl.SSLError, x:
                    raise test_support.TestFailed(
                        "Unexpected SSL error:  " + str(x))
                except Exception, x:
                    raise test_support.TestFailed(
                        "Unexpected exception:  " + str(x))
                else:
                    if not s:
                        raise test_support.TestFailed(
                            "Can't SSL-handshake with test server")
                    cert = s.getpeercert()
                    if not cert:
                        raise test_support.TestFailed(
                            "Can't get peer certificate.")
                    cipher = s.cipher()
                    if test_support.verbose:
                        sys.stdout.write(pprint.pformat(cert) + '\n')
                        sys.stdout.write("Connection cipher is " + str(cipher) + '.\n')
                    if not cert.has_key('subject'):
                        raise test_support.TestFailed(
                            "No subject field in certificate: %s." %
                            pprint.pformat(cert))
                    if ((('organizationName', 'Python Software Foundation'),)
                        not in cert['subject']):
                        raise test_support.TestFailed(
                            "Missing or invalid 'organizationName' field in certificate subject; "
                            "should be 'Python Software Foundation'.");
                    s.close()
            finally:
                server.stop()
                server.join()

        def testNULLcert(self):
            badCertTest(os.path.join(os.path.dirname(__file__) or os.curdir,
                                     "nullcert.pem"))
        def testMalformedCert(self):
            badCertTest(os.path.join(os.path.dirname(__file__) or os.curdir,
                                     "badcert.pem"))
        def testMalformedKey(self):
            badCertTest(os.path.join(os.path.dirname(__file__) or os.curdir,
                                     "badkey.pem"))

        def testProtocolSSL2(self):
            if test_support.verbose:
                sys.stdout.write("\n")
            tryProtocolCombo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv2, True)
            tryProtocolCombo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv2, True, ssl.CERT_OPTIONAL)
            tryProtocolCombo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv2, True, ssl.CERT_REQUIRED)
            tryProtocolCombo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv23, True)
            tryProtocolCombo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv3, False)
            tryProtocolCombo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_TLSv1, False)

        def testProtocolSSL23(self):
            if test_support.verbose:
                sys.stdout.write("\n")
            try:
                tryProtocolCombo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv2, True)
            except test_support.TestFailed, x:
                # this fails on some older versions of OpenSSL (0.9.7l, for instance)
                if test_support.verbose:
                    sys.stdout.write(
                        " SSL2 client to SSL23 server test unexpectedly failed:\n %s\n"
                        % str(x))
            tryProtocolCombo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv3, True)
            tryProtocolCombo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv23, True)
            tryProtocolCombo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_TLSv1, True)

            tryProtocolCombo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv3, True, ssl.CERT_OPTIONAL)
            tryProtocolCombo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv23, True, ssl.CERT_OPTIONAL)
            tryProtocolCombo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_TLSv1, True, ssl.CERT_OPTIONAL)

            tryProtocolCombo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv3, True, ssl.CERT_REQUIRED)
            tryProtocolCombo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv23, True, ssl.CERT_REQUIRED)
            tryProtocolCombo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_TLSv1, True, ssl.CERT_REQUIRED)

        def testProtocolSSL3(self):
            if test_support.verbose:
                sys.stdout.write("\n")
            tryProtocolCombo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv3, True)
            tryProtocolCombo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv3, True, ssl.CERT_OPTIONAL)
            tryProtocolCombo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv3, True, ssl.CERT_REQUIRED)
            tryProtocolCombo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv2, False)
            tryProtocolCombo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv23, False)
            tryProtocolCombo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_TLSv1, False)

        def testProtocolTLS1(self):
            if test_support.verbose:
                sys.stdout.write("\n")
            tryProtocolCombo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_TLSv1, True)
            tryProtocolCombo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_TLSv1, True, ssl.CERT_OPTIONAL)
            tryProtocolCombo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_TLSv1, True, ssl.CERT_REQUIRED)
            tryProtocolCombo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_SSLv2, False)
            tryProtocolCombo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_SSLv3, False)
            tryProtocolCombo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_SSLv23, False)

        def testSTARTTLS (self):

            msgs = ("msg 1", "MSG 2", "STARTTLS", "MSG 3", "msg 4")

            server = ThreadedEchoServer(TESTPORT, CERTFILE,
                                        ssl_version=ssl.PROTOCOL_TLSv1,
                                        starttls_server=True,
                                        chatty=True,
                                        connectionchatty=True)
            flag = threading.Event()
            server.start(flag)
            # wait for it to start
            flag.wait()
            # try to connect
            wrapped = False
            try:
                try:
                    s = socket.socket()
                    s.setblocking(1)
                    s.connect(('127.0.0.1', TESTPORT))
                except Exception, x:
                    raise test_support.TestFailed("Unexpected exception:  " + str(x))
                else:
                    if test_support.verbose:
                        sys.stdout.write("\n")
                    for indata in msgs:
                        if test_support.verbose:
                            sys.stdout.write(" client:  sending %s...\n" % repr(indata))
                        if wrapped:
                            conn.write(indata)
                            outdata = conn.read()
                        else:
                            s.send(indata)
                            outdata = s.recv(1024)
                        if indata == "STARTTLS" and outdata.strip().lower().startswith("ok"):
                            if test_support.verbose:
                                sys.stdout.write(" client:  read %s from server, starting TLS...\n" % repr(outdata))
                            conn = ssl.wrap_socket(s, ssl_version=ssl.PROTOCOL_TLSv1)

                            wrapped = True
                        else:
                            if test_support.verbose:
                                sys.stdout.write(" client:  read %s from server\n" % repr(outdata))
                    if test_support.verbose:
                        sys.stdout.write(" client:  closing connection.\n")
                    if wrapped:
                        conn.write("over\n")
                        conn.ssl_shutdown()
                    else:
                        s.send("over\n")
                    s.close()
            finally:
                server.stop()
                server.join()


CERTFILE_CONFIG_TEMPLATE = """
# create RSA certs - Server

[ req ]
default_bits = 1024
encrypt_key = yes
distinguished_name = req_dn
x509_extensions = cert_type

[ req_dn ]
countryName = Country Name (2 letter code)
countryName_default             = US
countryName_min                 = 2
countryName_max                 = 2

stateOrProvinceName             = State or Province Name (full name)
stateOrProvinceName_default     = %(state)s

localityName                    = Locality Name (eg, city)
localityName_default            = %(city)s

0.organizationName              = Organization Name (eg, company)
0.organizationName_default      = %(organization)s

organizationalUnitName          = Organizational Unit Name (eg, section)
organizationalUnitName_default  = %(unit)s

0.commonName                    = Common Name (FQDN of your server)
0.commonName_default            = %(common-name)s

# To create a certificate for more than one name uncomment:
# 1.commonName                  = DNS alias of your server
# 2.commonName                  = DNS alias of your server
# ...
# See http://home.netscape.com/eng/security/ssl_2.0_certificate.html
# to see how Netscape understands commonName.

[ cert_type ]
nsCertType = server
"""

def create_cert_files(hostname=None):

    """This is the routine that was run to create the certificate
    and private key contained in keycert.pem."""

    import tempfile, socket, os
    d = tempfile.mkdtemp()
    # now create a configuration file for the CA signing cert
    fqdn = hostname or socket.getfqdn()
    crtfile = os.path.join(d, "cert.pem")
    conffile = os.path.join(d, "ca.conf")
    fp = open(conffile, "w")
    fp.write(CERTFILE_CONFIG_TEMPLATE %
             {'state': "Delaware",
              'city': "Wilmington",
              'organization': "Python Software Foundation",
              'unit': "SSL",
              'common-name': fqdn,
              })
    fp.close()
    error = os.system(
        "openssl req -batch -new -x509 -days 2000 -nodes -config %s "
        "-keyout \"%s\" -out \"%s\" > /dev/null < /dev/null 2>&1" %
        (conffile, crtfile, crtfile))
    # now we have a self-signed server cert in crtfile
    os.unlink(conffile)
    if (os.WEXITSTATUS(error) or
        not os.path.exists(crtfile) or os.path.getsize(crtfile) == 0):
        if test_support.verbose:
            sys.stdout.write("Unable to create certificate for test, "
                             + "error status %d\n" % (error >> 8))
        crtfile = None
    elif test_support.verbose:
        sys.stdout.write(open(crtfile, 'r').read() + '\n')
    return d, crtfile


def findtestsocket(start, end):
    def testbind(i):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            s.bind(("127.0.0.1", i))
        except:
            return 0
        else:
            return 1
        finally:
            s.close()

    for i in range(start, end):
        if testbind(i) and testbind(i+1):
            return i
    return 0


def test_main(verbose=False):
    if skip_expected:
        raise test_support.TestSkipped("No SSL support")

    global CERTFILE, TESTPORT
    CERTFILE = os.path.join(os.path.dirname(__file__) or os.curdir,
                             "keycert.pem")
    if (not os.path.exists(CERTFILE)):
        raise test_support.TestFailed("Can't read certificate files!")
    TESTPORT = findtestsocket(10025, 12000)
    if not TESTPORT:
        raise test_support.TestFailed("Can't find open port to test servers on!")

    tests = [BasicTests]

    if _have_threads:
        thread_info = test_support.threading_setup()
        if CERTFILE and thread_info and test_support.is_resource_enabled('network'):
            tests.append(ConnectedTests)

    test_support.run_unittest(*tests)

    if _have_threads:
        test_support.threading_cleanup(*thread_info)

if __name__ == "__main__":
    test_main()
