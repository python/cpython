# Wrapper module for _ssl, providing some additional facilities
# implemented in Python.  Written by Bill Janssen.

"""This module provides some more Pythonic support for SSL.

Object types:

  SSLSocket -- subtype of socket.socket which does SSL over the socket

Exceptions:

  SSLError -- exception raised for I/O errors

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

import os, sys, textwrap

import _ssl             # if we can't import it, let the error propagate

from _ssl import SSLError
from _ssl import CERT_NONE, CERT_OPTIONAL, CERT_REQUIRED
from _ssl import (PROTOCOL_SSLv2, PROTOCOL_SSLv3, PROTOCOL_SSLv23,
                  PROTOCOL_TLSv1)
from _ssl import RAND_status, RAND_egd, RAND_add
from _ssl import (
    SSL_ERROR_ZERO_RETURN,
    SSL_ERROR_WANT_READ,
    SSL_ERROR_WANT_WRITE,
    SSL_ERROR_WANT_X509_LOOKUP,
    SSL_ERROR_SYSCALL,
    SSL_ERROR_SSL,
    SSL_ERROR_WANT_CONNECT,
    SSL_ERROR_EOF,
    SSL_ERROR_INVALID_ERROR_CODE,
    )

from socket import socket, AF_INET, SOCK_STREAM, error
from socket import getnameinfo as _getnameinfo
from socket import error as socket_error
from socket import dup as _dup
import base64        # for DER-to-PEM translation

class SSLSocket(socket):

    """This class implements a subtype of socket.socket that wraps
    the underlying OS socket in an SSL context when necessary, and
    provides read and write methods over that channel."""

    def __init__(self, sock=None, keyfile=None, certfile=None,
                 server_side=False, cert_reqs=CERT_NONE,
                 ssl_version=PROTOCOL_SSLv23, ca_certs=None,
                 do_handshake_on_connect=True,
                 family=AF_INET, type=SOCK_STREAM, proto=0, fileno=None,
                 suppress_ragged_eofs=True):

        self._base = None

        if sock is not None:
            # copied this code from socket.accept()
            fd = sock.fileno()
            nfd = _dup(fd)
            socket.__init__(self, family=sock.family, type=sock.type,
                            proto=sock.proto, fileno=nfd)
            sock.close()
            sock = None
        elif fileno is not None:
            socket.__init__(self, fileno=fileno)
        else:
            socket.__init__(self, family=family, type=type, proto=proto)

        self._closed = False

        if certfile and not keyfile:
            keyfile = certfile
        # see if it's connected
        try:
            socket.getpeername(self)
        except:
            # no, no connection yet
            self._sslobj = None
        else:
            # yes, create the SSL object
            try:
                self._sslobj = _ssl.sslwrap(self, server_side,
                                            keyfile, certfile,
                                            cert_reqs, ssl_version, ca_certs)
                if do_handshake_on_connect:
                    self.do_handshake()
            except socket_error as x:
                self.close()
                raise x

        self._base = sock
        self.keyfile = keyfile
        self.certfile = certfile
        self.cert_reqs = cert_reqs
        self.ssl_version = ssl_version
        self.ca_certs = ca_certs
        self.do_handshake_on_connect = do_handshake_on_connect
        self.suppress_ragged_eofs = suppress_ragged_eofs

    def _checkClosed(self, msg=None):
        # raise an exception here if you wish to check for spurious closes
        pass

    def read(self, len=1024, buffer=None):
        """Read up to LEN bytes and return them.
        Return zero-length string on EOF."""

        self._checkClosed()
        try:
            if buffer:
                return self._sslobj.read(buffer, len)
            else:
                return self._sslobj.read(len)
        except SSLError as x:
            if x.args[0] == SSL_ERROR_EOF and self.suppress_ragged_eofs:
                return b''
            else:
                raise

    def write(self, data):
        """Write DATA to the underlying SSL channel.  Returns
        number of bytes of DATA actually transmitted."""

        self._checkClosed()
        return self._sslobj.write(data)

    def getpeercert(self, binary_form=False):
        """Returns a formatted version of the data in the
        certificate provided by the other end of the SSL channel.
        Return None if no certificate was provided, {} if a
        certificate was provided, but not validated."""

        self._checkClosed()
        return self._sslobj.peer_certificate(binary_form)

    def cipher(self):
        self._checkClosed()
        if not self._sslobj:
            return None
        else:
            return self._sslobj.cipher()

    def send(self, data, flags=0):
        self._checkClosed()
        if self._sslobj:
            if flags != 0:
                raise ValueError(
                    "non-zero flags not allowed in calls to send() on %s" %
                    self.__class__)
            while True:
                try:
                    v = self._sslobj.write(data)
                except SSLError as x:
                    if x.args[0] == SSL_ERROR_WANT_READ:
                        return 0
                    elif x.args[0] == SSL_ERROR_WANT_WRITE:
                        return 0
                    else:
                        raise
                else:
                    return v
        else:
            return socket.send(self, data, flags)

    def send_to(self, data, addr, flags=0):
        self._checkClosed()
        if self._sslobj:
            raise ValueError("send_to not allowed on instances of %s" %
                             self.__class__)
        else:
            return socket.send_to(self, data, addr, flags)

    def sendall(self, data, flags=0):
        self._checkClosed()
        if self._sslobj:
            amount = len(data)
            count = 0
            while (count < amount):
                v = self.send(data[count:])
                count += v
            return amount
        else:
            return socket.sendall(self, data, flags)

    def recv(self, buflen=1024, flags=0):
        self._checkClosed()
        if self._sslobj:
            if flags != 0:
                raise ValueError(
                  "non-zero flags not allowed in calls to recv_into() on %s" %
                  self.__class__)
            while True:
                try:
                    return self.read(buflen)
                except SSLError as x:
                    if x.args[0] == SSL_ERROR_WANT_READ:
                        continue
                    else:
                        raise x
        else:
            return socket.recv(self, buflen, flags)

    def recv_into(self, buffer, nbytes=None, flags=0):
        self._checkClosed()
        if buffer and (nbytes is None):
            nbytes = len(buffer)
        elif nbytes is None:
            nbytes = 1024
        if self._sslobj:
            if flags != 0:
                raise ValueError(
                  "non-zero flags not allowed in calls to recv_into() on %s" %
                  self.__class__)
            while True:
                try:
                    v = self.read(nbytes, buffer)
                    sys.stdout.flush()
                    return v
                except SSLError as x:
                    if x.args[0] == SSL_ERROR_WANT_READ:
                        continue
                    else:
                        raise x
        else:
            return socket.recv_into(self, buffer, nbytes, flags)

    def recv_from(self, addr, buflen=1024, flags=0):
        self._checkClosed()
        if self._sslobj:
            raise ValueError("recv_from not allowed on instances of %s" %
                             self.__class__)
        else:
            return socket.recv_from(self, addr, buflen, flags)

    def pending(self):
        self._checkClosed()
        if self._sslobj:
            return self._sslobj.pending()
        else:
            return 0

    def shutdown(self, how):
        self._checkClosed()
        self._sslobj = None
        socket.shutdown(self, how)

    def _real_close(self):
        self._sslobj = None
        # self._closed = True
        if self._base:
            self._base.close()
        socket._real_close(self)

    def do_handshake(self):
        """Perform a TLS/SSL handshake."""

        try:
            self._sslobj.do_handshake()
        except:
            self._sslobj = None
            raise

    def connect(self, addr):
        """Connects to remote ADDR, and then wraps the connection in
        an SSL channel."""

        # Here we assume that the socket is client-side, and not
        # connected at the time of the call.  We connect it, then wrap it.
        if self._sslobj:
            raise ValueError("attempt to connect already-connected SSLSocket!")
        socket.connect(self, addr)
        self._sslobj = _ssl.sslwrap(self, False, self.keyfile, self.certfile,
                                    self.cert_reqs, self.ssl_version,
                                    self.ca_certs)
        if self.do_handshake_on_connect:
            self.do_handshake()

    def accept(self):
        """Accepts a new connection from a remote client, and returns
        a tuple containing that new connection wrapped with a server-side
        SSL channel, and the address of the remote client."""

        newsock, addr = socket.accept(self)
        return (SSLSocket(sock=newsock,
                          keyfile=self.keyfile, certfile=self.certfile,
                          server_side=True,
                          cert_reqs=self.cert_reqs,
                          ssl_version=self.ssl_version,
                          ca_certs=self.ca_certs,
                          do_handshake_on_connect=
                              self.do_handshake_on_connect),
                addr)


def wrap_socket(sock, keyfile=None, certfile=None,
                server_side=False, cert_reqs=CERT_NONE,
                ssl_version=PROTOCOL_SSLv23, ca_certs=None,
                do_handshake_on_connect=True):

    return SSLSocket(sock=sock, keyfile=keyfile, certfile=certfile,
                     server_side=server_side, cert_reqs=cert_reqs,
                     ssl_version=ssl_version, ca_certs=ca_certs,
                     do_handshake_on_connect=do_handshake_on_connect)

# some utility functions

def cert_time_to_seconds(cert_time):
    """Takes a date-time string in standard ASN1_print form
    ("MON DAY 24HOUR:MINUTE:SEC YEAR TIMEZONE") and return
    a Python time value in seconds past the epoch."""

    import time
    return time.mktime(time.strptime(cert_time, "%b %d %H:%M:%S %Y GMT"))

PEM_HEADER = "-----BEGIN CERTIFICATE-----"
PEM_FOOTER = "-----END CERTIFICATE-----"

def DER_cert_to_PEM_cert(der_cert_bytes):
    """Takes a certificate in binary DER format and returns the
    PEM version of it as a string."""

    f = str(base64.standard_b64encode(der_cert_bytes), 'ASCII', 'strict')
    return (PEM_HEADER + '\n' +
            textwrap.fill(f, 64) + '\n' +
            PEM_FOOTER + '\n')

def PEM_cert_to_DER_cert(pem_cert_string):
    """Takes a certificate in ASCII PEM format and returns the
    DER-encoded version of it as a byte sequence"""

    if not pem_cert_string.startswith(PEM_HEADER):
        raise ValueError("Invalid PEM encoding; must start with %s"
                         % PEM_HEADER)
    if not pem_cert_string.strip().endswith(PEM_FOOTER):
        raise ValueError("Invalid PEM encoding; must end with %s"
                         % PEM_FOOTER)
    d = pem_cert_string.strip()[len(PEM_HEADER):-len(PEM_FOOTER)]
    return base64.decodestring(d.encode('ASCII', 'strict'))

def get_server_certificate(addr, ssl_version=PROTOCOL_SSLv3, ca_certs=None):
    """Retrieve the certificate from the server at the specified address,
    and return it as a PEM-encoded string.
    If 'ca_certs' is specified, validate the server cert against it.
    If 'ssl_version' is specified, use it in the connection attempt."""

    host, port = addr
    if (ca_certs is not None):
        cert_reqs = CERT_REQUIRED
    else:
        cert_reqs = CERT_NONE
    s = wrap_socket(socket(), ssl_version=ssl_version,
                    cert_reqs=cert_reqs, ca_certs=ca_certs)
    s.connect(addr)
    dercert = s.getpeercert(True)
    s.close()
    return DER_cert_to_PEM_cert(dercert)

def get_protocol_name(protocol_code):
    if protocol_code == PROTOCOL_TLSv1:
        return "TLSv1"
    elif protocol_code == PROTOCOL_SSLv23:
        return "SSLv23"
    elif protocol_code == PROTOCOL_SSLv2:
        return "SSLv2"
    elif protocol_code == PROTOCOL_SSLv3:
        return "SSLv3"
    else:
        return "<unknown>"
