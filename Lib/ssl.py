# Wrapper module for _ssl, providing some additional facilities
# implemented in Python.  Written by Bill Janssen.

"""\
This module provides some more Pythonic support for SSL.

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

import textwrap

import _ssl             # if we can't import it, let the error propagate

from _ssl import SSLError
from _ssl import CERT_NONE, CERT_OPTIONAL, CERT_REQUIRED
from _ssl import PROTOCOL_SSLv2, PROTOCOL_SSLv3, PROTOCOL_SSLv23, PROTOCOL_TLSv1
from _ssl import RAND_status, RAND_egd, RAND_add
from _ssl import \
     SSL_ERROR_ZERO_RETURN, \
     SSL_ERROR_WANT_READ, \
     SSL_ERROR_WANT_WRITE, \
     SSL_ERROR_WANT_X509_LOOKUP, \
     SSL_ERROR_SYSCALL, \
     SSL_ERROR_SSL, \
     SSL_ERROR_WANT_CONNECT, \
     SSL_ERROR_EOF, \
     SSL_ERROR_INVALID_ERROR_CODE

from socket import socket, _fileobject
from socket import getnameinfo as _getnameinfo
import base64        # for DER-to-PEM translation

class SSLSocket (socket):

    """This class implements a subtype of socket.socket that wraps
    the underlying OS socket in an SSL context when necessary, and
    provides read and write methods over that channel."""

    def __init__(self, sock, keyfile=None, certfile=None,
                 server_side=False, cert_reqs=CERT_NONE,
                 ssl_version=PROTOCOL_SSLv23, ca_certs=None,
                 do_handshake_on_connect=True,
                 suppress_ragged_eofs=True):
        socket.__init__(self, _sock=sock._sock)
        # the initializer for socket trashes the methods (tsk, tsk), so...
        self.send = lambda data, flags=0: SSLSocket.send(self, data, flags)
        self.sendto = lambda data, addr, flags=0: SSLSocket.sendto(self, data, addr, flags)
        self.recv = lambda buflen=1024, flags=0: SSLSocket.recv(self, buflen, flags)
        self.recvfrom = lambda addr, buflen=1024, flags=0: SSLSocket.recvfrom(self, addr, buflen, flags)
        self.recv_into = lambda buffer, nbytes=None, flags=0: SSLSocket.recv_into(self, buffer, nbytes, flags)
        self.recvfrom_into = lambda buffer, nbytes=None, flags=0: SSLSocket.recvfrom_into(self, buffer, nbytes, flags)

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
            self._sslobj = _ssl.sslwrap(self._sock, server_side,
                                        keyfile, certfile,
                                        cert_reqs, ssl_version, ca_certs)
            if do_handshake_on_connect:
                timeout = self.gettimeout()
                try:
                    self.settimeout(None)
                    self.do_handshake()
                finally:
                    self.settimeout(timeout)
        self.keyfile = keyfile
        self.certfile = certfile
        self.cert_reqs = cert_reqs
        self.ssl_version = ssl_version
        self.ca_certs = ca_certs
        self.do_handshake_on_connect = do_handshake_on_connect
        self.suppress_ragged_eofs = suppress_ragged_eofs
        self._makefile_refs = 0

    def read(self, len=1024):

        """Read up to LEN bytes and return them.
        Return zero-length string on EOF."""

        try:
            return self._sslobj.read(len)
        except SSLError, x:
            if x.args[0] == SSL_ERROR_EOF and self.suppress_ragged_eofs:
                return ''
            else:
                raise

    def write(self, data):

        """Write DATA to the underlying SSL channel.  Returns
        number of bytes of DATA actually transmitted."""

        return self._sslobj.write(data)

    def getpeercert(self, binary_form=False):

        """Returns a formatted version of the data in the
        certificate provided by the other end of the SSL channel.
        Return None if no certificate was provided, {} if a
        certificate was provided, but not validated."""

        return self._sslobj.peer_certificate(binary_form)

    def cipher (self):

        if not self._sslobj:
            return None
        else:
            return self._sslobj.cipher()

    def send (self, data, flags=0):
        if self._sslobj:
            if flags != 0:
                raise ValueError(
                    "non-zero flags not allowed in calls to send() on %s" %
                    self.__class__)
            while True:
                try:
                    v = self._sslobj.write(data)
                except SSLError, x:
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

    def sendto (self, data, addr, flags=0):
        if self._sslobj:
            raise ValueError("sendto not allowed on instances of %s" %
                             self.__class__)
        else:
            return socket.sendto(self, data, addr, flags)

    def sendall (self, data, flags=0):
        if self._sslobj:
            if flags != 0:
                raise ValueError(
                    "non-zero flags not allowed in calls to sendall() on %s" %
                    self.__class__)
            amount = len(data)
            count = 0
            while (count < amount):
                v = self.send(data[count:])
                count += v
            return amount
        else:
            return socket.sendall(self, data, flags)

    def recv (self, buflen=1024, flags=0):
        if self._sslobj:
            if flags != 0:
                raise ValueError(
                    "non-zero flags not allowed in calls to sendall() on %s" %
                    self.__class__)
            while True:
                try:
                    return self.read(buflen)
                except SSLError, x:
                    if x.args[0] == SSL_ERROR_WANT_READ:
                        continue
                    else:
                        raise x
        else:
            return socket.recv(self, buflen, flags)

    def recv_into (self, buffer, nbytes=None, flags=0):
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
                    tmp_buffer = self.read(nbytes)
                    v = len(tmp_buffer)
                    buffer[:v] = tmp_buffer
                    return v
                except SSLError as x:
                    if x.args[0] == SSL_ERROR_WANT_READ:
                        continue
                    else:
                        raise x
        else:
            return socket.recv_into(self, buffer, nbytes, flags)

    def recvfrom (self, addr, buflen=1024, flags=0):
        if self._sslobj:
            raise ValueError("recvfrom not allowed on instances of %s" %
                             self.__class__)
        else:
            return socket.recvfrom(self, addr, buflen, flags)

    def recvfrom_into (self, buffer, nbytes=None, flags=0):
        if self._sslobj:
            raise ValueError("recvfrom_into not allowed on instances of %s" %
                             self.__class__)
        else:
            return socket.recvfrom_into(self, buffer, nbytes, flags)

    def pending (self):
        if self._sslobj:
            return self._sslobj.pending()
        else:
            return 0

    def unwrap (self):
        if self._sslobj:
            s = self._sslobj.shutdown()
            self._sslobj = None
            return s
        else:
            raise ValueError("No SSL wrapper around " + str(self))

    def shutdown (self, how):
        self._sslobj = None
        socket.shutdown(self, how)

    def close (self):
        if self._makefile_refs < 1:
            self._sslobj = None
            socket.close(self)
        else:
            self._makefile_refs -= 1

    def do_handshake (self):

        """Perform a TLS/SSL handshake."""

        self._sslobj.do_handshake()

    def connect(self, addr):

        """Connects to remote ADDR, and then wraps the connection in
        an SSL channel."""

        # Here we assume that the socket is client-side, and not
        # connected at the time of the call.  We connect it, then wrap it.
        if self._sslobj:
            raise ValueError("attempt to connect already-connected SSLSocket!")
        socket.connect(self, addr)
        self._sslobj = _ssl.sslwrap(self._sock, False, self.keyfile, self.certfile,
                                    self.cert_reqs, self.ssl_version,
                                    self.ca_certs)
        if self.do_handshake_on_connect:
            self.do_handshake()

    def accept(self):

        """Accepts a new connection from a remote client, and returns
        a tuple containing that new connection wrapped with a server-side
        SSL channel, and the address of the remote client."""

        newsock, addr = socket.accept(self)
        return (SSLSocket(newsock,
                          keyfile=self.keyfile,
                          certfile=self.certfile,
                          server_side=True,
                          cert_reqs=self.cert_reqs,
                          ssl_version=self.ssl_version,
                          ca_certs=self.ca_certs,
                          do_handshake_on_connect=self.do_handshake_on_connect,
                          suppress_ragged_eofs=self.suppress_ragged_eofs),
                addr)

    def makefile(self, mode='r', bufsize=-1):

        """Make and return a file-like object that
        works with the SSL connection.  Just use the code
        from the socket module."""

        self._makefile_refs += 1
        return _fileobject(self, mode, bufsize)



def wrap_socket(sock, keyfile=None, certfile=None,
                server_side=False, cert_reqs=CERT_NONE,
                ssl_version=PROTOCOL_SSLv23, ca_certs=None,
                do_handshake_on_connect=True,
                suppress_ragged_eofs=True):

    return SSLSocket(sock, keyfile=keyfile, certfile=certfile,
                     server_side=server_side, cert_reqs=cert_reqs,
                     ssl_version=ssl_version, ca_certs=ca_certs,
                     do_handshake_on_connect=do_handshake_on_connect,
                     suppress_ragged_eofs=suppress_ragged_eofs)


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

    if hasattr(base64, 'standard_b64encode'):
        # preferred because older API gets line-length wrong
        f = base64.standard_b64encode(der_cert_bytes)
        return (PEM_HEADER + '\n' +
                textwrap.fill(f, 64) +
                PEM_FOOTER + '\n')
    else:
        return (PEM_HEADER + '\n' +
                base64.encodestring(der_cert_bytes) +
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
    return base64.decodestring(d)

def get_server_certificate (addr, ssl_version=PROTOCOL_SSLv3, ca_certs=None):

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

def get_protocol_name (protocol_code):
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


# a replacement for the old socket.ssl function

def sslwrap_simple (sock, keyfile=None, certfile=None):

    """A replacement for the old socket.ssl function.  Designed
    for compability with Python 2.5 and earlier.  Will disappear in
    Python 3.0."""

    if hasattr(sock, "_sock"):
        sock = sock._sock

    ssl_sock = _ssl.sslwrap(sock, 0, keyfile, certfile, CERT_NONE,
                            PROTOCOL_SSLv23, None)
    try:
        sock.getpeername()
    except:
        # no, no connection yet
        pass
    else:
        # yes, do the handshake
        ssl_sock.do_handshake()

    return ssl_sock
