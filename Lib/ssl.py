# Wrapper module for _ssl, providing some additional facilities
# implemented in Python.  Written by Bill Janssen.

"""\
This module provides some more Pythonic support for SSL.

Object types:

  sslsocket -- subtype of socket.socket which does SSL over the socket

Exceptions:

  sslerror -- exception raised for I/O errors

Functions:

  cert_time_to_seconds -- convert time string used for certificate
                          notBefore and notAfter functions to integer
                          seconds past the Epoch (the time values
                          returned from time.time())

  fetch_server_certificate (HOST, PORT) -- fetch the certificate provided
                          by the server running on HOST at port PORT.  No
                          validation of the certificate is performed.

Integer constants:

SSL_ERROR_ZERO_RETURN
SSL_ERROR_WANT_READ
SSL_ERROR_WANT_WRITE
SSL_ERROR_WANT_X509_LOOKUP
SSL_ERROR_SYSCALL
SSL_ERROR_SSL
SSL_ERROR_WANT_CONNECT

SSL_ERROR_EOF
SSL_ERROR_INVALID_ERROR_CODE

The following group define certificate requirements that one side is
allowing/requiring from the other side:

CERT_NONE - no certificates from the other side are required (or will
            be looked at if provided)
CERT_OPTIONAL - certificates are not required, but if provided will be
                validated, and if validation fails, the connection will
                also fail
CERT_REQUIRED - certificates are required, and will be validated, and
                if validation fails, the connection will also fail

The following constants identify various SSL protocol variants:

PROTOCOL_SSLv2
PROTOCOL_SSLv3
PROTOCOL_SSLv23
PROTOCOL_TLSv1
"""

import os, sys

import _ssl             # if we can't import it, let the error propagate
from socket import socket
from _ssl import sslerror
from _ssl import CERT_NONE, CERT_OPTIONAL, CERT_REQUIRED
from _ssl import PROTOCOL_SSLv2, PROTOCOL_SSLv3, PROTOCOL_SSLv23, PROTOCOL_TLSv1

# Root certs:
#
# The "ca_certs" argument to sslsocket() expects a file containing one or more
# certificates that are roots of various certificate signing chains.  This file
# contains the certificates in PEM format (RFC ) where each certificate is
# encoded in base64 encoding and surrounded with a header and footer:
# -----BEGIN CERTIFICATE-----
# ... (CA certificate in base64 encoding) ...
# -----END CERTIFICATE-----
# The various certificates in the file are just concatenated together:
# -----BEGIN CERTIFICATE-----
# ... (CA certificate in base64 encoding) ...
# -----END CERTIFICATE-----
# -----BEGIN CERTIFICATE-----
# ... (a second CA certificate in base64 encoding) ...
# -----END CERTIFICATE-----
#
# Some "standard" root certificates are available at
#
# http://www.thawte.com/roots/  (for Thawte roots)
# http://www.verisign.com/support/roots.html  (for Verisign)

class sslsocket (socket):

    def __init__(self, sock, keyfile=None, certfile=None,
                 server_side=False, cert_reqs=CERT_NONE,
                 ssl_version=PROTOCOL_SSLv23, ca_certs=None):
        socket.__init__(self, _sock=sock._sock)
        if certfile and not keyfile:
            keyfile = certfile
        if server_side:
            self._sslobj = _ssl.sslwrap(self._sock, 1, keyfile, certfile,
                                        cert_reqs, ssl_version, ca_certs)
        else:
            # see if it's connected
            try:
                socket.getpeername(self)
            except:
                # no, no connection yet
                self._sslobj = None
            else:
                # yes, create the SSL object
                self._sslobj = _ssl.sslwrap(self._sock, 0, keyfile, certfile,
                                            cert_reqs, ssl_version, ca_certs)
        self.keyfile = keyfile
        self.certfile = certfile
        self.cert_reqs = cert_reqs
        self.ssl_version = ssl_version
        self.ca_certs = ca_certs

    def read(self, len=1024):
        return self._sslobj.read(len)

    def write(self, data):
        return self._sslobj.write(data)

    def getpeercert(self):
        return self._sslobj.peer_certificate()

    def send (self, data, flags=0):
        if flags != 0:
            raise ValueError(
                "non-zero flags not allowed in calls to send() on %s" %
                self.__class__)
        return self._sslobj.write(data)

    def send_to (self, data, addr, flags=0):
        raise ValueError("send_to not allowed on instances of %s" %
                         self.__class__)

    def sendall (self, data, flags=0):
        if flags != 0:
            raise ValueError(
                "non-zero flags not allowed in calls to sendall() on %s" %
                self.__class__)
        return self._sslobj.write(data)

    def recv (self, buflen=1024, flags=0):
        if flags != 0:
            raise ValueError(
                "non-zero flags not allowed in calls to sendall() on %s" %
                self.__class__)
        return self._sslobj.read(data, buflen)

    def recv_from (self, addr, buflen=1024, flags=0):
        raise ValueError("recv_from not allowed on instances of %s" %
                         self.__class__)

    def shutdown(self):
        if self._sslobj:
            self._sslobj.shutdown()
            self._sslobj = None
        else:
            socket.shutdown(self)

    def close(self):
        if self._sslobj:
            self.shutdown()
        else:
            socket.close(self)

    def connect(self, addr):
        # Here we assume that the socket is client-side, and not
        # connected at the time of the call.  We connect it, then wrap it.
        if self._sslobj or (self.getsockname()[1] != 0):
            raise ValueError("attempt to connect already-connected sslsocket!")
        socket.connect(self, addr)
        self._sslobj = _ssl.sslwrap(self._sock, 0, self.keyfile, self.certfile,
                                    self.cert_reqs, self.ssl_version,
                                    self.ca_certs)

    def accept(self):
        raise ValueError("accept() not supported on an sslsocket")


# some utility functions

def cert_time_to_seconds(cert_time):
    import time
    return time.mktime(time.strptime(cert_time, "%b %d %H:%M:%S %Y GMT"))

# a replacement for the old socket.ssl function

def sslwrap_simple (sock, keyfile=None, certfile=None):

    return _ssl.sslwrap(sock._sock, 0, keyfile, certfile, CERT_NONE,
                        PROTOCOL_SSLv23, None)

# fetch the certificate that the server is providing in PEM form

def fetch_server_certificate (host, port):

    import re, tempfile, os

    def subproc(cmd):
        from subprocess import Popen, PIPE, STDOUT
        proc = Popen(cmd, stdout=PIPE, stderr=STDOUT, shell=True)
        status = proc.wait()
        output = proc.stdout.read()
        return status, output

    def strip_to_x509_cert(certfile_contents, outfile=None):
        m = re.search(r"^([-]+BEGIN CERTIFICATE[-]+[\r]*\n"
                      r".*[\r]*^[-]+END CERTIFICATE[-]+)$",
                      certfile_contents, re.MULTILINE | re.DOTALL)
        if not m:
            return None
        else:
            tn = tempfile.mktemp()
            fp = open(tn, "w")
            fp.write(m.group(1) + "\n")
            fp.close()
            try:
                tn2 = (outfile or tempfile.mktemp())
                status, output = subproc(r'openssl x509 -in "%s" -out "%s"' %
                                         (tn, tn2))
                if status != 0:
                    raise OperationError(status, tsig, output)
                fp = open(tn2, 'rb')
                data = fp.read()
                fp.close()
                os.unlink(tn2)
                return data
            finally:
                os.unlink(tn)

    if sys.platform.startswith("win"):
        tfile = tempfile.mktemp()
        fp = open(tfile, "w")
        fp.write("quit\n")
        fp.close()
        try:
            status, output = subproc(
                'openssl s_client -connect "%s:%s" -showcerts < "%s"' %
                (host, port, tfile))
        finally:
            os.unlink(tfile)
    else:
        status, output = subproc(
            'openssl s_client -connect "%s:%s" -showcerts < /dev/null' %
            (host, port))
    if status != 0:
        raise OSError(status)
    certtext = strip_to_x509_cert(output)
    if not certtext:
        raise ValueError("Invalid response received from server at %s:%s" %
                         (host, port))
    return certtext
