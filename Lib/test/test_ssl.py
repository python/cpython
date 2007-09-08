# Test the support for SSL and sockets

import sys
import unittest
from test import test_support
import socket
import errno
import threading
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


def handle_error(prefix):
    exc_format = ' '.join(traceback.format_exception(*sys.exc_info()))
    sys.stdout.write(prefix + exc_format)


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

    def testSSLconnect(self):
        import os
        with test_support.transient_internet():
            s = ssl.sslsocket(socket.socket(socket.AF_INET),
                              cert_reqs=ssl.CERT_NONE)
            s.connect(("pop.gmail.com", 995))
            c = s.getpeercert()
            if c:
                raise test_support.TestFailed("Peer cert %s shouldn't be here!")
            s.close()

            # this should fail because we have no verification certs
            s = ssl.sslsocket(socket.socket(socket.AF_INET),
                              cert_reqs=ssl.CERT_REQUIRED)
            try:
                s.connect(("pop.gmail.com", 995))
            except ssl.sslerror:
                pass
            finally:
                s.close()

class ConnectedTests(unittest.TestCase):

    def testTLSecho (self):

        s1 = socket.socket()
        try:
            s1.connect(('127.0.0.1', 10024))
        except:
            handle_error("connection failure:\n")
            raise test_support.TestFailed("Can't connect to test server")
        else:
            try:
                c1 = ssl.sslsocket(s1, ssl_version=ssl.PROTOCOL_TLSv1)
            except:
                handle_error("SSL handshake failure:\n")
                raise test_support.TestFailed("Can't SSL-handshake with test server")
            else:
                if not c1:
                    raise test_support.TestFailed("Can't SSL-handshake with test server")
                indata = "FOO\n"
                c1.write(indata)
                outdata = c1.read()
                if outdata != indata.lower():
                    raise test_support.TestFailed("bad data <<%s>> received; expected <<%s>>\n" % (data, indata.lower()))
                c1.close()

    def testReadCert(self):

        s2 = socket.socket()
        try:
            s2.connect(('127.0.0.1', 10024))
        except:
            handle_error("connection failure:\n")
            raise test_support.TestFailed("Can't connect to test server")
        else:
            try:
                c2 = ssl.sslsocket(s2, ssl_version=ssl.PROTOCOL_TLSv1,
                                   cert_reqs=ssl.CERT_REQUIRED, ca_certs=CERTFILE)
            except:
                handle_error("SSL handshake failure:\n")
                raise test_support.TestFailed("Can't SSL-handshake with test server")
            else:
                if not c2:
                    raise test_support.TestFailed("Can't SSL-handshake with test server")
                cert = c2.getpeercert()
                if not cert:
                    raise test_support.TestFailed("Can't get peer certificate.")
                if test_support.verbose:
                    sys.stdout.write(pprint.pformat(cert) + '\n')
                if not cert.has_key('subject'):
                    raise test_support.TestFailed(
                        "No subject field in certificate: %s." %
                        pprint.pformat(cert))
                if not ('organizationName', 'Python Software Foundation') in cert['subject']:
                    raise test_support.TestFailed(
                        "Missing or invalid 'organizationName' field in certificate subject; "
                        "should be 'Python Software Foundation'.");
                c2.close()


class ThreadedEchoServer(threading.Thread):

    class ConnectionHandler(threading.Thread):

        def __init__(self, server, connsock):
            self.server = server
            self.running = False
            self.sock = connsock
            threading.Thread.__init__(self)
            self.setDaemon(True)

        def run (self):
            self.running = True
            try:
                sslconn = ssl.sslsocket(self.sock, server_side=True,
                                        certfile=self.server.certificate,
                                        ssl_version=self.server.protocol,
                                        cert_reqs=self.server.certreqs)
            except:
                # here, we want to stop the server, because this shouldn't
                # happen in the context of our test case
                handle_error("Test server failure:\n")
                self.running = False
                # normally, we'd just stop here, but for the test
                # harness, we want to stop the server
                self.server.stop()
                return

            while self.running:
                try:
                    msg = sslconn.read()
                    if not msg:
                        # eof, so quit this handler
                        self.running = False
                        sslconn.close()
                    elif msg.strip() == 'over':
                        sslconn.close()
                        self.server.stop()
                        self.running = False
                    else:
                        if test_support.verbose:
                            sys.stdout.write("\nserver: %s\n" % msg.strip().lower())
                        sslconn.write(msg.lower())
                except ssl.sslerror:
                    handle_error("Test server failure:\n")
                    sslconn.close()
                    self.running = False
                    # normally, we'd just stop here, but for the test
                    # harness, we want to stop the server
                    self.server.stop()
                except:
                    handle_error('')

    def __init__(self, port, certificate, ssl_version=None,
                 certreqs=None, cacerts=None):
        if ssl_version is None:
            ssl_version = ssl.PROTOCOL_TLSv1
        if certreqs is None:
            certreqs = ssl.CERT_NONE
        self.certificate = certificate
        self.protocol = ssl_version
        self.certreqs = certreqs
        self.cacerts = cacerts
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
                if test_support.verbose:
                    sys.stdout.write('\nserver:  new connection from ' + str(connaddr) + '\n')
                handler = self.ConnectionHandler(self, newconn)
                handler.start()
            except socket.timeout:
                pass
            except KeyboardInterrupt:
                self.stop()
            except:
                handle_error("Test server failure:\n")

    def stop (self):
        self.active = False
        self.sock.close()

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


def test_main(verbose=False):
    if skip_expected:
        raise test_support.TestSkipped("No SSL support")

    global CERTFILE
    CERTFILE = os.path.join(os.path.dirname(__file__) or os.curdir,
                            "keycert.pem")
    if not CERTFILE:
        sys.__stdout__.write("Skipping test_ssl ConnectedTests; "
                             "couldn't create a certificate.\n")

    tests = [BasicTests]

    server = None
    if CERTFILE and test_support.is_resource_enabled('network'):
        server = ThreadedEchoServer(10024, CERTFILE)
        flag = threading.Event()
        server.start(flag)
        # wait for it to start
        flag.wait()
        tests.append(ConnectedTests)

    thread_info = test_support.threading_setup()

    try:
        test_support.run_unittest(*tests)
    finally:
        if server is not None and server.active:
            server.stop()
            # wait for it to stop
            server.join()

    test_support.threading_cleanup(*thread_info)

if __name__ == "__main__":
    test_main()
