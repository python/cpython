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
PROTOCOL_TLSv1_1
PROTOCOL_TLSv1_2

The following constants identify various SSL alert message descriptions as per
http://www.iana.org/assignments/tls-parameters/tls-parameters.xml#tls-parameters-6

ALERT_DESCRIPTION_CLOSE_NOTIFY
ALERT_DESCRIPTION_UNEXPECTED_MESSAGE
ALERT_DESCRIPTION_BAD_RECORD_MAC
ALERT_DESCRIPTION_RECORD_OVERFLOW
ALERT_DESCRIPTION_DECOMPRESSION_FAILURE
ALERT_DESCRIPTION_HANDSHAKE_FAILURE
ALERT_DESCRIPTION_BAD_CERTIFICATE
ALERT_DESCRIPTION_UNSUPPORTED_CERTIFICATE
ALERT_DESCRIPTION_CERTIFICATE_REVOKED
ALERT_DESCRIPTION_CERTIFICATE_EXPIRED
ALERT_DESCRIPTION_CERTIFICATE_UNKNOWN
ALERT_DESCRIPTION_ILLEGAL_PARAMETER
ALERT_DESCRIPTION_UNKNOWN_CA
ALERT_DESCRIPTION_ACCESS_DENIED
ALERT_DESCRIPTION_DECODE_ERROR
ALERT_DESCRIPTION_DECRYPT_ERROR
ALERT_DESCRIPTION_PROTOCOL_VERSION
ALERT_DESCRIPTION_INSUFFICIENT_SECURITY
ALERT_DESCRIPTION_INTERNAL_ERROR
ALERT_DESCRIPTION_USER_CANCELLED
ALERT_DESCRIPTION_NO_RENEGOTIATION
ALERT_DESCRIPTION_UNSUPPORTED_EXTENSION
ALERT_DESCRIPTION_CERTIFICATE_UNOBTAINABLE
ALERT_DESCRIPTION_UNRECOGNIZED_NAME
ALERT_DESCRIPTION_BAD_CERTIFICATE_STATUS_RESPONSE
ALERT_DESCRIPTION_BAD_CERTIFICATE_HASH_VALUE
ALERT_DESCRIPTION_UNKNOWN_PSK_IDENTITY
"""

import textwrap
import re
import sys
import os
from collections import namedtuple
from contextlib import closing

import _ssl             # if we can't import it, let the error propagate

from _ssl import OPENSSL_VERSION_NUMBER, OPENSSL_VERSION_INFO, OPENSSL_VERSION
from _ssl import _SSLContext
from _ssl import (
    SSLError, SSLZeroReturnError, SSLWantReadError, SSLWantWriteError,
    SSLSyscallError, SSLEOFError,
    )
from _ssl import CERT_NONE, CERT_OPTIONAL, CERT_REQUIRED
from _ssl import txt2obj as _txt2obj, nid2obj as _nid2obj
from _ssl import RAND_status, RAND_add
try:
    from _ssl import RAND_egd
except ImportError:
    # LibreSSL does not provide RAND_egd
    pass

def _import_symbols(prefix):
    for n in dir(_ssl):
        if n.startswith(prefix):
            globals()[n] = getattr(_ssl, n)

_import_symbols('OP_')
_import_symbols('ALERT_DESCRIPTION_')
_import_symbols('SSL_ERROR_')
_import_symbols('PROTOCOL_')
_import_symbols('VERIFY_')

from _ssl import HAS_SNI, HAS_ECDH, HAS_NPN, HAS_ALPN

from _ssl import _OPENSSL_API_VERSION

_PROTOCOL_NAMES = {value: name for name, value in globals().items() if name.startswith('PROTOCOL_')}

try:
    _SSLv2_IF_EXISTS = PROTOCOL_SSLv2
except NameError:
    _SSLv2_IF_EXISTS = None

from socket import socket, _fileobject, _delegate_methods, error as socket_error
if sys.platform == "win32":
    from _ssl import enum_certificates, enum_crls

from socket import socket, AF_INET, SOCK_STREAM, create_connection
from socket import SOL_SOCKET, SO_TYPE
import base64        # for DER-to-PEM translation
import errno

if _ssl.HAS_TLS_UNIQUE:
    CHANNEL_BINDING_TYPES = ['tls-unique']
else:
    CHANNEL_BINDING_TYPES = []

# Disable weak or insecure ciphers by default
# (OpenSSL's default setting is 'DEFAULT:!aNULL:!eNULL')
# Enable a better set of ciphers by default
# This list has been explicitly chosen to:
#   * Prefer cipher suites that offer perfect forward secrecy (DHE/ECDHE)
#   * Prefer ECDHE over DHE for better performance
#   * Prefer any AES-GCM over any AES-CBC for better performance and security
#   * Then Use HIGH cipher suites as a fallback
#   * Then Use 3DES as fallback which is secure but slow
#   * Disable NULL authentication, NULL encryption, and MD5 MACs for security
#     reasons
_DEFAULT_CIPHERS = (
    'ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:ECDH+HIGH:'
    'DH+HIGH:ECDH+3DES:DH+3DES:RSA+AESGCM:RSA+AES:RSA+HIGH:RSA+3DES:!aNULL:'
    '!eNULL:!MD5'
)

# Restricted and more secure ciphers for the server side
# This list has been explicitly chosen to:
#   * Prefer cipher suites that offer perfect forward secrecy (DHE/ECDHE)
#   * Prefer ECDHE over DHE for better performance
#   * Prefer any AES-GCM over any AES-CBC for better performance and security
#   * Then Use HIGH cipher suites as a fallback
#   * Then Use 3DES as fallback which is secure but slow
#   * Disable NULL authentication, NULL encryption, MD5 MACs, DSS, and RC4 for
#     security reasons
_RESTRICTED_SERVER_CIPHERS = (
    'ECDH+AESGCM:DH+AESGCM:ECDH+AES256:DH+AES256:ECDH+AES128:DH+AES:ECDH+HIGH:'
    'DH+HIGH:ECDH+3DES:DH+3DES:RSA+AESGCM:RSA+AES:RSA+HIGH:RSA+3DES:!aNULL:'
    '!eNULL:!MD5:!DSS:!RC4'
)


class CertificateError(ValueError):
    pass


def _dnsname_match(dn, hostname, max_wildcards=1):
    """Matching according to RFC 6125, section 6.4.3

    http://tools.ietf.org/html/rfc6125#section-6.4.3
    """
    pats = []
    if not dn:
        return False

    pieces = dn.split(r'.')
    leftmost = pieces[0]
    remainder = pieces[1:]

    wildcards = leftmost.count('*')
    if wildcards > max_wildcards:
        # Issue #17980: avoid denials of service by refusing more
        # than one wildcard per fragment.  A survery of established
        # policy among SSL implementations showed it to be a
        # reasonable choice.
        raise CertificateError(
            "too many wildcards in certificate DNS name: " + repr(dn))

    # speed up common case w/o wildcards
    if not wildcards:
        return dn.lower() == hostname.lower()

    # RFC 6125, section 6.4.3, subitem 1.
    # The client SHOULD NOT attempt to match a presented identifier in which
    # the wildcard character comprises a label other than the left-most label.
    if leftmost == '*':
        # When '*' is a fragment by itself, it matches a non-empty dotless
        # fragment.
        pats.append('[^.]+')
    elif leftmost.startswith('xn--') or hostname.startswith('xn--'):
        # RFC 6125, section 6.4.3, subitem 3.
        # The client SHOULD NOT attempt to match a presented identifier
        # where the wildcard character is embedded within an A-label or
        # U-label of an internationalized domain name.
        pats.append(re.escape(leftmost))
    else:
        # Otherwise, '*' matches any dotless string, e.g. www*
        pats.append(re.escape(leftmost).replace(r'\*', '[^.]*'))

    # add the remaining fragments, ignore any wildcards
    for frag in remainder:
        pats.append(re.escape(frag))

    pat = re.compile(r'\A' + r'\.'.join(pats) + r'\Z', re.IGNORECASE)
    return pat.match(hostname)


def match_hostname(cert, hostname):
    """Verify that *cert* (in decoded format as returned by
    SSLSocket.getpeercert()) matches the *hostname*.  RFC 2818 and RFC 6125
    rules are followed, but IP addresses are not accepted for *hostname*.

    CertificateError is raised on failure. On success, the function
    returns nothing.
    """
    if not cert:
        raise ValueError("empty or no certificate, match_hostname needs a "
                         "SSL socket or SSL context with either "
                         "CERT_OPTIONAL or CERT_REQUIRED")
    dnsnames = []
    san = cert.get('subjectAltName', ())
    for key, value in san:
        if key == 'DNS':
            if _dnsname_match(value, hostname):
                return
            dnsnames.append(value)
    if not dnsnames:
        # The subject is only checked when there is no dNSName entry
        # in subjectAltName
        for sub in cert.get('subject', ()):
            for key, value in sub:
                # XXX according to RFC 2818, the most specific Common Name
                # must be used.
                if key == 'commonName':
                    if _dnsname_match(value, hostname):
                        return
                    dnsnames.append(value)
    if len(dnsnames) > 1:
        raise CertificateError("hostname %r "
            "doesn't match either of %s"
            % (hostname, ', '.join(map(repr, dnsnames))))
    elif len(dnsnames) == 1:
        raise CertificateError("hostname %r "
            "doesn't match %r"
            % (hostname, dnsnames[0]))
    else:
        raise CertificateError("no appropriate commonName or "
            "subjectAltName fields were found")


DefaultVerifyPaths = namedtuple("DefaultVerifyPaths",
    "cafile capath openssl_cafile_env openssl_cafile openssl_capath_env "
    "openssl_capath")

def get_default_verify_paths():
    """Return paths to default cafile and capath.
    """
    parts = _ssl.get_default_verify_paths()

    # environment vars shadow paths
    cafile = os.environ.get(parts[0], parts[1])
    capath = os.environ.get(parts[2], parts[3])

    return DefaultVerifyPaths(cafile if os.path.isfile(cafile) else None,
                              capath if os.path.isdir(capath) else None,
                              *parts)


class _ASN1Object(namedtuple("_ASN1Object", "nid shortname longname oid")):
    """ASN.1 object identifier lookup
    """
    __slots__ = ()

    def __new__(cls, oid):
        return super(_ASN1Object, cls).__new__(cls, *_txt2obj(oid, name=False))

    @classmethod
    def fromnid(cls, nid):
        """Create _ASN1Object from OpenSSL numeric ID
        """
        return super(_ASN1Object, cls).__new__(cls, *_nid2obj(nid))

    @classmethod
    def fromname(cls, name):
        """Create _ASN1Object from short name, long name or OID
        """
        return super(_ASN1Object, cls).__new__(cls, *_txt2obj(name, name=True))


class Purpose(_ASN1Object):
    """SSLContext purpose flags with X509v3 Extended Key Usage objects
    """

Purpose.SERVER_AUTH = Purpose('1.3.6.1.5.5.7.3.1')
Purpose.CLIENT_AUTH = Purpose('1.3.6.1.5.5.7.3.2')


class SSLContext(_SSLContext):
    """An SSLContext holds various SSL-related configuration options and
    data, such as certificates and possibly a private key."""

    __slots__ = ('protocol', '__weakref__')
    _windows_cert_stores = ("CA", "ROOT")

    def __new__(cls, protocol, *args, **kwargs):
        self = _SSLContext.__new__(cls, protocol)
        if protocol != _SSLv2_IF_EXISTS:
            self.set_ciphers(_DEFAULT_CIPHERS)
        return self

    def __init__(self, protocol):
        self.protocol = protocol

    def wrap_socket(self, sock, server_side=False,
                    do_handshake_on_connect=True,
                    suppress_ragged_eofs=True,
                    server_hostname=None):
        return SSLSocket(sock=sock, server_side=server_side,
                         do_handshake_on_connect=do_handshake_on_connect,
                         suppress_ragged_eofs=suppress_ragged_eofs,
                         server_hostname=server_hostname,
                         _context=self)

    def set_npn_protocols(self, npn_protocols):
        protos = bytearray()
        for protocol in npn_protocols:
            b = protocol.encode('ascii')
            if len(b) == 0 or len(b) > 255:
                raise SSLError('NPN protocols must be 1 to 255 in length')
            protos.append(len(b))
            protos.extend(b)

        self._set_npn_protocols(protos)

    def set_alpn_protocols(self, alpn_protocols):
        protos = bytearray()
        for protocol in alpn_protocols:
            b = protocol.encode('ascii')
            if len(b) == 0 or len(b) > 255:
                raise SSLError('ALPN protocols must be 1 to 255 in length')
            protos.append(len(b))
            protos.extend(b)

        self._set_alpn_protocols(protos)

    def _load_windows_store_certs(self, storename, purpose):
        certs = bytearray()
        for cert, encoding, trust in enum_certificates(storename):
            # CA certs are never PKCS#7 encoded
            if encoding == "x509_asn":
                if trust is True or purpose.oid in trust:
                    certs.extend(cert)
        if certs:
            self.load_verify_locations(cadata=certs)
        return certs

    def load_default_certs(self, purpose=Purpose.SERVER_AUTH):
        if not isinstance(purpose, _ASN1Object):
            raise TypeError(purpose)
        if sys.platform == "win32":
            for storename in self._windows_cert_stores:
                self._load_windows_store_certs(storename, purpose)
        self.set_default_verify_paths()


def create_default_context(purpose=Purpose.SERVER_AUTH, cafile=None,
                           capath=None, cadata=None):
    """Create a SSLContext object with default settings.

    NOTE: The protocol and settings may change anytime without prior
          deprecation. The values represent a fair balance between maximum
          compatibility and security.
    """
    if not isinstance(purpose, _ASN1Object):
        raise TypeError(purpose)

    context = SSLContext(PROTOCOL_SSLv23)

    # SSLv2 considered harmful.
    context.options |= OP_NO_SSLv2

    # SSLv3 has problematic security and is only required for really old
    # clients such as IE6 on Windows XP
    context.options |= OP_NO_SSLv3

    # disable compression to prevent CRIME attacks (OpenSSL 1.0+)
    context.options |= getattr(_ssl, "OP_NO_COMPRESSION", 0)

    if purpose == Purpose.SERVER_AUTH:
        # verify certs and host name in client mode
        context.verify_mode = CERT_REQUIRED
        context.check_hostname = True
    elif purpose == Purpose.CLIENT_AUTH:
        # Prefer the server's ciphers by default so that we get stronger
        # encryption
        context.options |= getattr(_ssl, "OP_CIPHER_SERVER_PREFERENCE", 0)

        # Use single use keys in order to improve forward secrecy
        context.options |= getattr(_ssl, "OP_SINGLE_DH_USE", 0)
        context.options |= getattr(_ssl, "OP_SINGLE_ECDH_USE", 0)

        # disallow ciphers with known vulnerabilities
        context.set_ciphers(_RESTRICTED_SERVER_CIPHERS)

    if cafile or capath or cadata:
        context.load_verify_locations(cafile, capath, cadata)
    elif context.verify_mode != CERT_NONE:
        # no explicit cafile, capath or cadata but the verify mode is
        # CERT_OPTIONAL or CERT_REQUIRED. Let's try to load default system
        # root CA certificates for the given purpose. This may fail silently.
        context.load_default_certs(purpose)
    return context

def _create_unverified_context(protocol=PROTOCOL_SSLv23, cert_reqs=None,
                           check_hostname=False, purpose=Purpose.SERVER_AUTH,
                           certfile=None, keyfile=None,
                           cafile=None, capath=None, cadata=None):
    """Create a SSLContext object for Python stdlib modules

    All Python stdlib modules shall use this function to create SSLContext
    objects in order to keep common settings in one place. The configuration
    is less restrict than create_default_context()'s to increase backward
    compatibility.
    """
    if not isinstance(purpose, _ASN1Object):
        raise TypeError(purpose)

    context = SSLContext(protocol)
    # SSLv2 considered harmful.
    context.options |= OP_NO_SSLv2
    # SSLv3 has problematic security and is only required for really old
    # clients such as IE6 on Windows XP
    context.options |= OP_NO_SSLv3

    if cert_reqs is not None:
        context.verify_mode = cert_reqs
    context.check_hostname = check_hostname

    if keyfile and not certfile:
        raise ValueError("certfile must be specified")
    if certfile or keyfile:
        context.load_cert_chain(certfile, keyfile)

    # load CA root certs
    if cafile or capath or cadata:
        context.load_verify_locations(cafile, capath, cadata)
    elif context.verify_mode != CERT_NONE:
        # no explicit cafile, capath or cadata but the verify mode is
        # CERT_OPTIONAL or CERT_REQUIRED. Let's try to load default system
        # root CA certificates for the given purpose. This may fail silently.
        context.load_default_certs(purpose)

    return context

# Used by http.client if no context is explicitly passed.
_create_default_https_context = create_default_context


# Backwards compatibility alias, even though it's not a public name.
_create_stdlib_context = _create_unverified_context


class SSLSocket(socket):
    """This class implements a subtype of socket.socket that wraps
    the underlying OS socket in an SSL context when necessary, and
    provides read and write methods over that channel."""

    def __init__(self, sock=None, keyfile=None, certfile=None,
                 server_side=False, cert_reqs=CERT_NONE,
                 ssl_version=PROTOCOL_SSLv23, ca_certs=None,
                 do_handshake_on_connect=True,
                 family=AF_INET, type=SOCK_STREAM, proto=0, fileno=None,
                 suppress_ragged_eofs=True, npn_protocols=None, ciphers=None,
                 server_hostname=None,
                 _context=None):

        self._makefile_refs = 0
        if _context:
            self._context = _context
        else:
            if server_side and not certfile:
                raise ValueError("certfile must be specified for server-side "
                                 "operations")
            if keyfile and not certfile:
                raise ValueError("certfile must be specified")
            if certfile and not keyfile:
                keyfile = certfile
            self._context = SSLContext(ssl_version)
            self._context.verify_mode = cert_reqs
            if ca_certs:
                self._context.load_verify_locations(ca_certs)
            if certfile:
                self._context.load_cert_chain(certfile, keyfile)
            if npn_protocols:
                self._context.set_npn_protocols(npn_protocols)
            if ciphers:
                self._context.set_ciphers(ciphers)
            self.keyfile = keyfile
            self.certfile = certfile
            self.cert_reqs = cert_reqs
            self.ssl_version = ssl_version
            self.ca_certs = ca_certs
            self.ciphers = ciphers
        # Can't use sock.type as other flags (such as SOCK_NONBLOCK) get
        # mixed in.
        if sock.getsockopt(SOL_SOCKET, SO_TYPE) != SOCK_STREAM:
            raise NotImplementedError("only stream sockets are supported")
        socket.__init__(self, _sock=sock._sock)
        # The initializer for socket overrides the methods send(), recv(), etc.
        # in the instancce, which we don't need -- but we want to provide the
        # methods defined in SSLSocket.
        for attr in _delegate_methods:
            try:
                delattr(self, attr)
            except AttributeError:
                pass
        if server_side and server_hostname:
            raise ValueError("server_hostname can only be specified "
                             "in client mode")
        if self._context.check_hostname and not server_hostname:
            raise ValueError("check_hostname requires server_hostname")
        self.server_side = server_side
        self.server_hostname = server_hostname
        self.do_handshake_on_connect = do_handshake_on_connect
        self.suppress_ragged_eofs = suppress_ragged_eofs

        # See if we are connected
        try:
            self.getpeername()
        except socket_error as e:
            if e.errno != errno.ENOTCONN:
                raise
            connected = False
        else:
            connected = True

        self._closed = False
        self._sslobj = None
        self._connected = connected
        if connected:
            # create the SSL object
            try:
                self._sslobj = self._context._wrap_socket(self._sock, server_side,
                                                          server_hostname, ssl_sock=self)
                if do_handshake_on_connect:
                    timeout = self.gettimeout()
                    if timeout == 0.0:
                        # non-blocking
                        raise ValueError("do_handshake_on_connect should not be specified for non-blocking sockets")
                    self.do_handshake()

            except (OSError, ValueError):
                self.close()
                raise

    @property
    def context(self):
        return self._context

    @context.setter
    def context(self, ctx):
        self._context = ctx
        self._sslobj.context = ctx

    def dup(self):
        raise NotImplemented("Can't dup() %s instances" %
                             self.__class__.__name__)

    def _checkClosed(self, msg=None):
        # raise an exception here if you wish to check for spurious closes
        pass

    def _check_connected(self):
        if not self._connected:
            # getpeername() will raise ENOTCONN if the socket is really
            # not connected; note that we can be connected even without
            # _connected being set, e.g. if connect() first returned
            # EAGAIN.
            self.getpeername()

    def read(self, len=0, buffer=None):
        """Read up to LEN bytes and return them.
        Return zero-length string on EOF."""

        self._checkClosed()
        if not self._sslobj:
            raise ValueError("Read on closed or unwrapped SSL socket.")
        try:
            if buffer is not None:
                v = self._sslobj.read(len, buffer)
            else:
                v = self._sslobj.read(len or 1024)
            return v
        except SSLError as x:
            if x.args[0] == SSL_ERROR_EOF and self.suppress_ragged_eofs:
                if buffer is not None:
                    return 0
                else:
                    return b''
            else:
                raise

    def write(self, data):
        """Write DATA to the underlying SSL channel.  Returns
        number of bytes of DATA actually transmitted."""

        self._checkClosed()
        if not self._sslobj:
            raise ValueError("Write on closed or unwrapped SSL socket.")
        return self._sslobj.write(data)

    def getpeercert(self, binary_form=False):
        """Returns a formatted version of the data in the
        certificate provided by the other end of the SSL channel.
        Return None if no certificate was provided, {} if a
        certificate was provided, but not validated."""

        self._checkClosed()
        self._check_connected()
        return self._sslobj.peer_certificate(binary_form)

    def selected_npn_protocol(self):
        self._checkClosed()
        if not self._sslobj or not _ssl.HAS_NPN:
            return None
        else:
            return self._sslobj.selected_npn_protocol()

    def selected_alpn_protocol(self):
        self._checkClosed()
        if not self._sslobj or not _ssl.HAS_ALPN:
            return None
        else:
            return self._sslobj.selected_alpn_protocol()

    def cipher(self):
        self._checkClosed()
        if not self._sslobj:
            return None
        else:
            return self._sslobj.cipher()

    def compression(self):
        self._checkClosed()
        if not self._sslobj:
            return None
        else:
            return self._sslobj.compression()

    def send(self, data, flags=0):
        self._checkClosed()
        if self._sslobj:
            if flags != 0:
                raise ValueError(
                    "non-zero flags not allowed in calls to send() on %s" %
                    self.__class__)
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
            return self._sock.send(data, flags)

    def sendto(self, data, flags_or_addr, addr=None):
        self._checkClosed()
        if self._sslobj:
            raise ValueError("sendto not allowed on instances of %s" %
                             self.__class__)
        elif addr is None:
            return self._sock.sendto(data, flags_or_addr)
        else:
            return self._sock.sendto(data, flags_or_addr, addr)


    def sendall(self, data, flags=0):
        self._checkClosed()
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

    def recv(self, buflen=1024, flags=0):
        self._checkClosed()
        if self._sslobj:
            if flags != 0:
                raise ValueError(
                    "non-zero flags not allowed in calls to recv() on %s" %
                    self.__class__)
            return self.read(buflen)
        else:
            return self._sock.recv(buflen, flags)

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
            return self.read(nbytes, buffer)
        else:
            return self._sock.recv_into(buffer, nbytes, flags)

    def recvfrom(self, buflen=1024, flags=0):
        self._checkClosed()
        if self._sslobj:
            raise ValueError("recvfrom not allowed on instances of %s" %
                             self.__class__)
        else:
            return self._sock.recvfrom(buflen, flags)

    def recvfrom_into(self, buffer, nbytes=None, flags=0):
        self._checkClosed()
        if self._sslobj:
            raise ValueError("recvfrom_into not allowed on instances of %s" %
                             self.__class__)
        else:
            return self._sock.recvfrom_into(buffer, nbytes, flags)


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

    def close(self):
        if self._makefile_refs < 1:
            self._sslobj = None
            socket.close(self)
        else:
            self._makefile_refs -= 1

    def unwrap(self):
        if self._sslobj:
            s = self._sslobj.shutdown()
            self._sslobj = None
            return s
        else:
            raise ValueError("No SSL wrapper around " + str(self))

    def _real_close(self):
        self._sslobj = None
        socket._real_close(self)

    def do_handshake(self, block=False):
        """Perform a TLS/SSL handshake."""
        self._check_connected()
        timeout = self.gettimeout()
        try:
            if timeout == 0.0 and block:
                self.settimeout(None)
            self._sslobj.do_handshake()
        finally:
            self.settimeout(timeout)

        if self.context.check_hostname:
            if not self.server_hostname:
                raise ValueError("check_hostname needs server_hostname "
                                 "argument")
            match_hostname(self.getpeercert(), self.server_hostname)

    def _real_connect(self, addr, connect_ex):
        if self.server_side:
            raise ValueError("can't connect in server-side mode")
        # Here we assume that the socket is client-side, and not
        # connected at the time of the call.  We connect it, then wrap it.
        if self._connected:
            raise ValueError("attempt to connect already-connected SSLSocket!")
        self._sslobj = self.context._wrap_socket(self._sock, False, self.server_hostname, ssl_sock=self)
        try:
            if connect_ex:
                rc = socket.connect_ex(self, addr)
            else:
                rc = None
                socket.connect(self, addr)
            if not rc:
                self._connected = True
                if self.do_handshake_on_connect:
                    self.do_handshake()
            return rc
        except (OSError, ValueError):
            self._sslobj = None
            raise

    def connect(self, addr):
        """Connects to remote ADDR, and then wraps the connection in
        an SSL channel."""
        self._real_connect(addr, False)

    def connect_ex(self, addr):
        """Connects to remote ADDR, and then wraps the connection in
        an SSL channel."""
        return self._real_connect(addr, True)

    def accept(self):
        """Accepts a new connection from a remote client, and returns
        a tuple containing that new connection wrapped with a server-side
        SSL channel, and the address of the remote client."""

        newsock, addr = socket.accept(self)
        newsock = self.context.wrap_socket(newsock,
                    do_handshake_on_connect=self.do_handshake_on_connect,
                    suppress_ragged_eofs=self.suppress_ragged_eofs,
                    server_side=True)
        return newsock, addr

    def makefile(self, mode='r', bufsize=-1):

        """Make and return a file-like object that
        works with the SSL connection.  Just use the code
        from the socket module."""

        self._makefile_refs += 1
        # close=True so as to decrement the reference count when done with
        # the file-like object.
        return _fileobject(self, mode, bufsize, close=True)

    def get_channel_binding(self, cb_type="tls-unique"):
        """Get channel binding data for current connection.  Raise ValueError
        if the requested `cb_type` is not supported.  Return bytes of the data
        or None if the data is not available (e.g. before the handshake).
        """
        if cb_type not in CHANNEL_BINDING_TYPES:
            raise ValueError("Unsupported channel binding type")
        if cb_type != "tls-unique":
            raise NotImplementedError(
                            "{0} channel binding type not implemented"
                            .format(cb_type))
        if self._sslobj is None:
            return None
        return self._sslobj.tls_unique_cb()

    def version(self):
        """
        Return a string identifying the protocol version used by the
        current SSL channel, or None if there is no established channel.
        """
        if self._sslobj is None:
            return None
        return self._sslobj.version()


def wrap_socket(sock, keyfile=None, certfile=None,
                server_side=False, cert_reqs=CERT_NONE,
                ssl_version=PROTOCOL_SSLv23, ca_certs=None,
                do_handshake_on_connect=True,
                suppress_ragged_eofs=True,
                ciphers=None):

    return SSLSocket(sock=sock, keyfile=keyfile, certfile=certfile,
                     server_side=server_side, cert_reqs=cert_reqs,
                     ssl_version=ssl_version, ca_certs=ca_certs,
                     do_handshake_on_connect=do_handshake_on_connect,
                     suppress_ragged_eofs=suppress_ragged_eofs,
                     ciphers=ciphers)

# some utility functions

def cert_time_to_seconds(cert_time):
    """Return the time in seconds since the Epoch, given the timestring
    representing the "notBefore" or "notAfter" date from a certificate
    in ``"%b %d %H:%M:%S %Y %Z"`` strptime format (C locale).

    "notBefore" or "notAfter" dates must use UTC (RFC 5280).

    Month is one of: Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
    UTC should be specified as GMT (see ASN1_TIME_print())
    """
    from time import strptime
    from calendar import timegm

    months = (
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    )
    time_format = ' %d %H:%M:%S %Y GMT' # NOTE: no month, fixed GMT
    try:
        month_number = months.index(cert_time[:3].title()) + 1
    except ValueError:
        raise ValueError('time data %r does not match '
                         'format "%%b%s"' % (cert_time, time_format))
    else:
        # found valid month
        tt = strptime(cert_time[3:], time_format)
        # return an integer, the previous mktime()-based implementation
        # returned a float (fractional seconds are always zero here).
        return timegm((tt[0], month_number) + tt[2:6])

PEM_HEADER = "-----BEGIN CERTIFICATE-----"
PEM_FOOTER = "-----END CERTIFICATE-----"

def DER_cert_to_PEM_cert(der_cert_bytes):
    """Takes a certificate in binary DER format and returns the
    PEM version of it as a string."""

    f = base64.standard_b64encode(der_cert_bytes).decode('ascii')
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

def get_server_certificate(addr, ssl_version=PROTOCOL_SSLv23, ca_certs=None):
    """Retrieve the certificate from the server at the specified address,
    and return it as a PEM-encoded string.
    If 'ca_certs' is specified, validate the server cert against it.
    If 'ssl_version' is specified, use it in the connection attempt."""

    host, port = addr
    if ca_certs is not None:
        cert_reqs = CERT_REQUIRED
    else:
        cert_reqs = CERT_NONE
    context = _create_stdlib_context(ssl_version,
                                     cert_reqs=cert_reqs,
                                     cafile=ca_certs)
    with closing(create_connection(addr)) as sock:
        with closing(context.wrap_socket(sock)) as sslsock:
            dercert = sslsock.getpeercert(True)
    return DER_cert_to_PEM_cert(dercert)

def get_protocol_name(protocol_code):
    return _PROTOCOL_NAMES.get(protocol_code, '<unknown>')


# a replacement for the old socket.ssl function

def sslwrap_simple(sock, keyfile=None, certfile=None):
    """A replacement for the old socket.ssl function.  Designed
    for compability with Python 2.5 and earlier.  Will disappear in
    Python 3.0."""
    if hasattr(sock, "_sock"):
        sock = sock._sock

    ctx = SSLContext(PROTOCOL_SSLv23)
    if keyfile or certfile:
        ctx.load_cert_chain(certfile, keyfile)
    ssl_sock = ctx._wrap_socket(sock, server_side=False)
    try:
        sock.getpeername()
    except socket_error:
        # no, no connection yet
        pass
    else:
        # yes, do the handshake
        ssl_sock.do_handshake()

    return ssl_sock
