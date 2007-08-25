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
import string
import traceback

# Optionally test SSL support, if we have it in the tested platform
skip_expected = False
try:
    import ssl
except ImportError:
    skip_expected = True

CERTFILE = None
GMAIL_POP_CERTFILE = None

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
        s1.connect(('127.0.0.1', 10024))
        c1 = ssl.sslsocket(s1, ssl_version=ssl.PROTOCOL_TLSv1)
        indata = "FOO\n"
        c1.write(indata)
        outdata = c1.read()
        if outdata != indata.lower():
            sys.stderr.write("bad data <<%s>> received\n" % data)
        c1.close()

    def testReadCert(self):

        s2 = socket.socket()
        s2.connect(('127.0.0.1', 10024))
        c2 = ssl.sslsocket(s2, ssl_version=ssl.PROTOCOL_TLSv1,
                           cert_reqs=ssl.CERT_REQUIRED, ca_certs=CERTFILE)
        cert = c2.getpeercert()
        if not cert:
            raise test_support.TestFailed("Can't get peer certificate.")
        if not cert.has_key('subject'):
            raise test_support.TestFailed(
                "No subject field in certificate: %s." %
                pprint.pformat(cert))
        if not (cert['subject'].has_key('organizationName')):
            raise test_support.TestFailed(
                "No 'organizationName' field in certificate subject: %s." %
                pprint.pformat(cert))
        if (cert['subject']['organizationName'] !=
              "Python Software Foundation"):
            raise test_support.TestFailed(
                "Invalid 'organizationName' field in certificate subject; "
                "should be 'Python Software Foundation'.");
        c2.close()


class threadedEchoServer(threading.Thread):

    class connectionHandler(threading.Thread):

        def __init__(self, server, connsock):
            self.server = server
            self.running = False
            self.sock = connsock
            threading.Thread.__init__(self)
            self.setDaemon(True)

        def run (self):
            self.running = True
            sslconn = ssl.sslsocket(self.sock, server_side=True,
                                    certfile=self.server.certificate,
                                    ssl_version=self.server.protocol,
                                    cert_reqs=self.server.certreqs)
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
                        # print "server:", msg.strip().lower()
                        sslconn.write(msg.lower())
                except ssl.sslerror:
                    sys.stderr.write(string.join(
                        traceback.format_exception(*sys.exc_info())))
                    sslconn.close()
                    self.running = False
                except:
                    sys.stderr.write(string.join(
                        traceback.format_exception(*sys.exc_info())))

    def __init__(self, port, certificate, ssl_version=ssl.PROTOCOL_TLSv1,
                 certreqs=ssl.CERT_NONE, cacerts=None):
        self.certificate = certificate
        self.protocol = ssl_version
        self.certreqs = certreqs
        self.cacerts = cacerts
        self.sock = socket.socket()
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
        self.sock.bind(('127.0.0.1', port))
        self.active = False
        threading.Thread.__init__(self)
        self.setDaemon(False)

    def run (self):
        self.sock.settimeout(0.5)
        self.sock.listen(5)
        self.active = True
        while self.active:
            try:
                newconn, connaddr = self.sock.accept()
                # sys.stderr.write('new connection from ' + str(connaddr))
                handler = self.connectionHandler(self, newconn)
                handler.start()
            except socket.timeout:
                pass
            except KeyboardInterrupt:
                self.active = False
            except:
                sys.stderr.write(string.join(
                    traceback.format_exception(*sys.exc_info())))

    def stop (self):
        self.active = False


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

def create_cert_files():

    import tempfile, socket, os
    d = tempfile.mkdtemp()
    # now create a configuration file for the CA signing cert
    fqdn = socket.getfqdn()
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
    os.system(
        "openssl req -batch -new -x509 -days 10 -nodes -config %s "
        "-keyout \"%s\" -out \"%s\" > /dev/null < /dev/null 2>&1" %
        (conffile, crtfile, crtfile))
    # now we have a self-signed server cert in crtfile
    os.unlink(conffile)
    #sf_certfile = os.path.join(d, "sourceforge-imap.pem")
    #sf_cert = ssl.fetch_server_certificate('pop.gmail.com', 995)
    #open(sf_certfile, 'w').write(sf_cert)
    #return d, crtfile, sf_certfile
    # sys.stderr.write(open(crtfile, 'r').read() + '\n')
    return d, crtfile

def test_main():
    if skip_expected:
        raise test_support.TestSkipped("socket module has no ssl support")

    global CERTFILE
    tdir, CERTFILE = create_cert_files()

    tests = [BasicTests]

    server = None
    if test_support.is_resource_enabled('network'):
        server = threadedEchoServer(10024, CERTFILE)
        server.start()
        time.sleep(1)
        tests.append(ConnectedTests)

    thread_info = test_support.threading_setup()

    try:
        test_support.run_unittest(*tests)
    finally:
        if server is not None and server.active:
            server.stop()
            # wait for it to stop
            server.join()

    shutil.rmtree(tdir)
    test_support.threading_cleanup(*thread_info)

if __name__ == "__main__":
    test_main()
