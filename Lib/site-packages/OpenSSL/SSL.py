import os
import socket
from sys import platform
from functools import wraps, partial
from itertools import count, chain
from weakref import WeakValueDictionary
from errno import errorcode

from six import integer_types, int2byte, indexbytes

from OpenSSL._util import (
    UNSPECIFIED as _UNSPECIFIED,
    exception_from_error_queue as _exception_from_error_queue,
    ffi as _ffi,
    from_buffer as _from_buffer,
    lib as _lib,
    make_assert as _make_assert,
    native as _native,
    path_string as _path_string,
    text_to_bytes_and_warn as _text_to_bytes_and_warn,
    no_zero_allocator as _no_zero_allocator,
)

from OpenSSL.crypto import (
    FILETYPE_PEM,
    _PassphraseHelper,
    PKey,
    X509Name,
    X509,
    X509Store,
)

__all__ = [
    "OPENSSL_VERSION_NUMBER",
    "SSLEAY_VERSION",
    "SSLEAY_CFLAGS",
    "SSLEAY_PLATFORM",
    "SSLEAY_DIR",
    "SSLEAY_BUILT_ON",
    "SENT_SHUTDOWN",
    "RECEIVED_SHUTDOWN",
    "SSLv2_METHOD",
    "SSLv3_METHOD",
    "SSLv23_METHOD",
    "TLSv1_METHOD",
    "TLSv1_1_METHOD",
    "TLSv1_2_METHOD",
    "OP_NO_SSLv2",
    "OP_NO_SSLv3",
    "OP_NO_TLSv1",
    "OP_NO_TLSv1_1",
    "OP_NO_TLSv1_2",
    "OP_NO_TLSv1_3",
    "MODE_RELEASE_BUFFERS",
    "OP_SINGLE_DH_USE",
    "OP_SINGLE_ECDH_USE",
    "OP_EPHEMERAL_RSA",
    "OP_MICROSOFT_SESS_ID_BUG",
    "OP_NETSCAPE_CHALLENGE_BUG",
    "OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG",
    "OP_SSLREF2_REUSE_CERT_TYPE_BUG",
    "OP_MICROSOFT_BIG_SSLV3_BUFFER",
    "OP_MSIE_SSLV2_RSA_PADDING",
    "OP_SSLEAY_080_CLIENT_DH_BUG",
    "OP_TLS_D5_BUG",
    "OP_TLS_BLOCK_PADDING_BUG",
    "OP_DONT_INSERT_EMPTY_FRAGMENTS",
    "OP_CIPHER_SERVER_PREFERENCE",
    "OP_TLS_ROLLBACK_BUG",
    "OP_PKCS1_CHECK_1",
    "OP_PKCS1_CHECK_2",
    "OP_NETSCAPE_CA_DN_BUG",
    "OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG",
    "OP_NO_COMPRESSION",
    "OP_NO_QUERY_MTU",
    "OP_COOKIE_EXCHANGE",
    "OP_NO_TICKET",
    "OP_ALL",
    "VERIFY_PEER",
    "VERIFY_FAIL_IF_NO_PEER_CERT",
    "VERIFY_CLIENT_ONCE",
    "VERIFY_NONE",
    "SESS_CACHE_OFF",
    "SESS_CACHE_CLIENT",
    "SESS_CACHE_SERVER",
    "SESS_CACHE_BOTH",
    "SESS_CACHE_NO_AUTO_CLEAR",
    "SESS_CACHE_NO_INTERNAL_LOOKUP",
    "SESS_CACHE_NO_INTERNAL_STORE",
    "SESS_CACHE_NO_INTERNAL",
    "SSL_ST_CONNECT",
    "SSL_ST_ACCEPT",
    "SSL_ST_MASK",
    "SSL_CB_LOOP",
    "SSL_CB_EXIT",
    "SSL_CB_READ",
    "SSL_CB_WRITE",
    "SSL_CB_ALERT",
    "SSL_CB_READ_ALERT",
    "SSL_CB_WRITE_ALERT",
    "SSL_CB_ACCEPT_LOOP",
    "SSL_CB_ACCEPT_EXIT",
    "SSL_CB_CONNECT_LOOP",
    "SSL_CB_CONNECT_EXIT",
    "SSL_CB_HANDSHAKE_START",
    "SSL_CB_HANDSHAKE_DONE",
    "Error",
    "WantReadError",
    "WantWriteError",
    "WantX509LookupError",
    "ZeroReturnError",
    "SysCallError",
    "SSLeay_version",
    "Session",
    "Context",
    "Connection",
]

try:
    _buffer = buffer
except NameError:

    class _buffer(object):
        pass


OPENSSL_VERSION_NUMBER = _lib.OPENSSL_VERSION_NUMBER
SSLEAY_VERSION = _lib.SSLEAY_VERSION
SSLEAY_CFLAGS = _lib.SSLEAY_CFLAGS
SSLEAY_PLATFORM = _lib.SSLEAY_PLATFORM
SSLEAY_DIR = _lib.SSLEAY_DIR
SSLEAY_BUILT_ON = _lib.SSLEAY_BUILT_ON

SENT_SHUTDOWN = _lib.SSL_SENT_SHUTDOWN
RECEIVED_SHUTDOWN = _lib.SSL_RECEIVED_SHUTDOWN

SSLv2_METHOD = 1
SSLv3_METHOD = 2
SSLv23_METHOD = 3
TLSv1_METHOD = 4
TLSv1_1_METHOD = 5
TLSv1_2_METHOD = 6

OP_NO_SSLv2 = _lib.SSL_OP_NO_SSLv2
OP_NO_SSLv3 = _lib.SSL_OP_NO_SSLv3
OP_NO_TLSv1 = _lib.SSL_OP_NO_TLSv1
OP_NO_TLSv1_1 = _lib.SSL_OP_NO_TLSv1_1
OP_NO_TLSv1_2 = _lib.SSL_OP_NO_TLSv1_2
try:
    OP_NO_TLSv1_3 = _lib.SSL_OP_NO_TLSv1_3
except AttributeError:
    pass

MODE_RELEASE_BUFFERS = _lib.SSL_MODE_RELEASE_BUFFERS

OP_SINGLE_DH_USE = _lib.SSL_OP_SINGLE_DH_USE
OP_SINGLE_ECDH_USE = _lib.SSL_OP_SINGLE_ECDH_USE
OP_EPHEMERAL_RSA = _lib.SSL_OP_EPHEMERAL_RSA
OP_MICROSOFT_SESS_ID_BUG = _lib.SSL_OP_MICROSOFT_SESS_ID_BUG
OP_NETSCAPE_CHALLENGE_BUG = _lib.SSL_OP_NETSCAPE_CHALLENGE_BUG
OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG = (
    _lib.SSL_OP_NETSCAPE_REUSE_CIPHER_CHANGE_BUG
)
OP_SSLREF2_REUSE_CERT_TYPE_BUG = _lib.SSL_OP_SSLREF2_REUSE_CERT_TYPE_BUG
OP_MICROSOFT_BIG_SSLV3_BUFFER = _lib.SSL_OP_MICROSOFT_BIG_SSLV3_BUFFER
OP_MSIE_SSLV2_RSA_PADDING = _lib.SSL_OP_MSIE_SSLV2_RSA_PADDING
OP_SSLEAY_080_CLIENT_DH_BUG = _lib.SSL_OP_SSLEAY_080_CLIENT_DH_BUG
OP_TLS_D5_BUG = _lib.SSL_OP_TLS_D5_BUG
OP_TLS_BLOCK_PADDING_BUG = _lib.SSL_OP_TLS_BLOCK_PADDING_BUG
OP_DONT_INSERT_EMPTY_FRAGMENTS = _lib.SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS
OP_CIPHER_SERVER_PREFERENCE = _lib.SSL_OP_CIPHER_SERVER_PREFERENCE
OP_TLS_ROLLBACK_BUG = _lib.SSL_OP_TLS_ROLLBACK_BUG
OP_PKCS1_CHECK_1 = _lib.SSL_OP_PKCS1_CHECK_1
OP_PKCS1_CHECK_2 = _lib.SSL_OP_PKCS1_CHECK_2
OP_NETSCAPE_CA_DN_BUG = _lib.SSL_OP_NETSCAPE_CA_DN_BUG
OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG = (
    _lib.SSL_OP_NETSCAPE_DEMO_CIPHER_CHANGE_BUG
)
OP_NO_COMPRESSION = _lib.SSL_OP_NO_COMPRESSION

OP_NO_QUERY_MTU = _lib.SSL_OP_NO_QUERY_MTU
OP_COOKIE_EXCHANGE = _lib.SSL_OP_COOKIE_EXCHANGE
OP_NO_TICKET = _lib.SSL_OP_NO_TICKET

OP_ALL = _lib.SSL_OP_ALL

VERIFY_PEER = _lib.SSL_VERIFY_PEER
VERIFY_FAIL_IF_NO_PEER_CERT = _lib.SSL_VERIFY_FAIL_IF_NO_PEER_CERT
VERIFY_CLIENT_ONCE = _lib.SSL_VERIFY_CLIENT_ONCE
VERIFY_NONE = _lib.SSL_VERIFY_NONE

SESS_CACHE_OFF = _lib.SSL_SESS_CACHE_OFF
SESS_CACHE_CLIENT = _lib.SSL_SESS_CACHE_CLIENT
SESS_CACHE_SERVER = _lib.SSL_SESS_CACHE_SERVER
SESS_CACHE_BOTH = _lib.SSL_SESS_CACHE_BOTH
SESS_CACHE_NO_AUTO_CLEAR = _lib.SSL_SESS_CACHE_NO_AUTO_CLEAR
SESS_CACHE_NO_INTERNAL_LOOKUP = _lib.SSL_SESS_CACHE_NO_INTERNAL_LOOKUP
SESS_CACHE_NO_INTERNAL_STORE = _lib.SSL_SESS_CACHE_NO_INTERNAL_STORE
SESS_CACHE_NO_INTERNAL = _lib.SSL_SESS_CACHE_NO_INTERNAL

SSL_ST_CONNECT = _lib.SSL_ST_CONNECT
SSL_ST_ACCEPT = _lib.SSL_ST_ACCEPT
SSL_ST_MASK = _lib.SSL_ST_MASK

SSL_CB_LOOP = _lib.SSL_CB_LOOP
SSL_CB_EXIT = _lib.SSL_CB_EXIT
SSL_CB_READ = _lib.SSL_CB_READ
SSL_CB_WRITE = _lib.SSL_CB_WRITE
SSL_CB_ALERT = _lib.SSL_CB_ALERT
SSL_CB_READ_ALERT = _lib.SSL_CB_READ_ALERT
SSL_CB_WRITE_ALERT = _lib.SSL_CB_WRITE_ALERT
SSL_CB_ACCEPT_LOOP = _lib.SSL_CB_ACCEPT_LOOP
SSL_CB_ACCEPT_EXIT = _lib.SSL_CB_ACCEPT_EXIT
SSL_CB_CONNECT_LOOP = _lib.SSL_CB_CONNECT_LOOP
SSL_CB_CONNECT_EXIT = _lib.SSL_CB_CONNECT_EXIT
SSL_CB_HANDSHAKE_START = _lib.SSL_CB_HANDSHAKE_START
SSL_CB_HANDSHAKE_DONE = _lib.SSL_CB_HANDSHAKE_DONE

# Taken from https://golang.org/src/crypto/x509/root_linux.go
_CERTIFICATE_FILE_LOCATIONS = [
    "/etc/ssl/certs/ca-certificates.crt",  # Debian/Ubuntu/Gentoo etc.
    "/etc/pki/tls/certs/ca-bundle.crt",  # Fedora/RHEL 6
    "/etc/ssl/ca-bundle.pem",  # OpenSUSE
    "/etc/pki/tls/cacert.pem",  # OpenELEC
    "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",  # CentOS/RHEL 7
]

_CERTIFICATE_PATH_LOCATIONS = [
    "/etc/ssl/certs",  # SLES10/SLES11
]

# These values are compared to output from cffi's ffi.string so they must be
# byte strings.
_CRYPTOGRAPHY_MANYLINUX1_CA_DIR = b"/opt/pyca/cryptography/openssl/certs"
_CRYPTOGRAPHY_MANYLINUX1_CA_FILE = b"/opt/pyca/cryptography/openssl/cert.pem"


class Error(Exception):
    """
    An error occurred in an `OpenSSL.SSL` API.
    """


_raise_current_error = partial(_exception_from_error_queue, Error)
_openssl_assert = _make_assert(Error)


class WantReadError(Error):
    pass


class WantWriteError(Error):
    pass


class WantX509LookupError(Error):
    pass


class ZeroReturnError(Error):
    pass


class SysCallError(Error):
    pass


class _CallbackExceptionHelper(object):
    """
    A base class for wrapper classes that allow for intelligent exception
    handling in OpenSSL callbacks.

    :ivar list _problems: Any exceptions that occurred while executing in a
        context where they could not be raised in the normal way.  Typically
        this is because OpenSSL has called into some Python code and requires a
        return value.  The exceptions are saved to be raised later when it is
        possible to do so.
    """

    def __init__(self):
        self._problems = []

    def raise_if_problem(self):
        """
        Raise an exception from the OpenSSL error queue or that was previously
        captured whe running a callback.
        """
        if self._problems:
            try:
                _raise_current_error()
            except Error:
                pass
            raise self._problems.pop(0)


class _VerifyHelper(_CallbackExceptionHelper):
    """
    Wrap a callback such that it can be used as a certificate verification
    callback.
    """

    def __init__(self, callback):
        _CallbackExceptionHelper.__init__(self)

        @wraps(callback)
        def wrapper(ok, store_ctx):
            x509 = _lib.X509_STORE_CTX_get_current_cert(store_ctx)
            _lib.X509_up_ref(x509)
            cert = X509._from_raw_x509_ptr(x509)
            error_number = _lib.X509_STORE_CTX_get_error(store_ctx)
            error_depth = _lib.X509_STORE_CTX_get_error_depth(store_ctx)

            index = _lib.SSL_get_ex_data_X509_STORE_CTX_idx()
            ssl = _lib.X509_STORE_CTX_get_ex_data(store_ctx, index)
            connection = Connection._reverse_mapping[ssl]

            try:
                result = callback(
                    connection, cert, error_number, error_depth, ok
                )
            except Exception as e:
                self._problems.append(e)
                return 0
            else:
                if result:
                    _lib.X509_STORE_CTX_set_error(store_ctx, _lib.X509_V_OK)
                    return 1
                else:
                    return 0

        self.callback = _ffi.callback(
            "int (*)(int, X509_STORE_CTX *)", wrapper
        )


NO_OVERLAPPING_PROTOCOLS = object()


class _ALPNSelectHelper(_CallbackExceptionHelper):
    """
    Wrap a callback such that it can be used as an ALPN selection callback.
    """

    def __init__(self, callback):
        _CallbackExceptionHelper.__init__(self)

        @wraps(callback)
        def wrapper(ssl, out, outlen, in_, inlen, arg):
            try:
                conn = Connection._reverse_mapping[ssl]

                # The string passed to us is made up of multiple
                # length-prefixed bytestrings. We need to split that into a
                # list.
                instr = _ffi.buffer(in_, inlen)[:]
                protolist = []
                while instr:
                    encoded_len = indexbytes(instr, 0)
                    proto = instr[1 : encoded_len + 1]
                    protolist.append(proto)
                    instr = instr[encoded_len + 1 :]

                # Call the callback
                outbytes = callback(conn, protolist)
                any_accepted = True
                if outbytes is NO_OVERLAPPING_PROTOCOLS:
                    outbytes = b""
                    any_accepted = False
                elif not isinstance(outbytes, bytes):
                    raise TypeError(
                        "ALPN callback must return a bytestring or the "
                        "special NO_OVERLAPPING_PROTOCOLS sentinel value."
                    )

                # Save our callback arguments on the connection object to make
                # sure that they don't get freed before OpenSSL can use them.
                # Then, return them in the appropriate output parameters.
                conn._alpn_select_callback_args = [
                    _ffi.new("unsigned char *", len(outbytes)),
                    _ffi.new("unsigned char[]", outbytes),
                ]
                outlen[0] = conn._alpn_select_callback_args[0][0]
                out[0] = conn._alpn_select_callback_args[1]
                if not any_accepted:
                    return _lib.SSL_TLSEXT_ERR_NOACK
                return _lib.SSL_TLSEXT_ERR_OK
            except Exception as e:
                self._problems.append(e)
                return _lib.SSL_TLSEXT_ERR_ALERT_FATAL

        self.callback = _ffi.callback(
            (
                "int (*)(SSL *, unsigned char **, unsigned char *, "
                "const unsigned char *, unsigned int, void *)"
            ),
            wrapper,
        )


class _OCSPServerCallbackHelper(_CallbackExceptionHelper):
    """
    Wrap a callback such that it can be used as an OCSP callback for the server
    side.

    Annoyingly, OpenSSL defines one OCSP callback but uses it in two different
    ways. For servers, that callback is expected to retrieve some OCSP data and
    hand it to OpenSSL, and may return only SSL_TLSEXT_ERR_OK,
    SSL_TLSEXT_ERR_FATAL, and SSL_TLSEXT_ERR_NOACK. For clients, that callback
    is expected to check the OCSP data, and returns a negative value on error,
    0 if the response is not acceptable, or positive if it is. These are
    mutually exclusive return code behaviours, and they mean that we need two
    helpers so that we always return an appropriate error code if the user's
    code throws an exception.

    Given that we have to have two helpers anyway, these helpers are a bit more
    helpery than most: specifically, they hide a few more of the OpenSSL
    functions so that the user has an easier time writing these callbacks.

    This helper implements the server side.
    """

    def __init__(self, callback):
        _CallbackExceptionHelper.__init__(self)

        @wraps(callback)
        def wrapper(ssl, cdata):
            try:
                conn = Connection._reverse_mapping[ssl]

                # Extract the data if any was provided.
                if cdata != _ffi.NULL:
                    data = _ffi.from_handle(cdata)
                else:
                    data = None

                # Call the callback.
                ocsp_data = callback(conn, data)

                if not isinstance(ocsp_data, bytes):
                    raise TypeError("OCSP callback must return a bytestring.")

                # If the OCSP data was provided, we will pass it to OpenSSL.
                # However, we have an early exit here: if no OCSP data was
                # provided we will just exit out and tell OpenSSL that there
                # is nothing to do.
                if not ocsp_data:
                    return 3  # SSL_TLSEXT_ERR_NOACK

                # OpenSSL takes ownership of this data and expects it to have
                # been allocated by OPENSSL_malloc.
                ocsp_data_length = len(ocsp_data)
                data_ptr = _lib.OPENSSL_malloc(ocsp_data_length)
                _ffi.buffer(data_ptr, ocsp_data_length)[:] = ocsp_data

                _lib.SSL_set_tlsext_status_ocsp_resp(
                    ssl, data_ptr, ocsp_data_length
                )

                return 0
            except Exception as e:
                self._problems.append(e)
                return 2  # SSL_TLSEXT_ERR_ALERT_FATAL

        self.callback = _ffi.callback("int (*)(SSL *, void *)", wrapper)


class _OCSPClientCallbackHelper(_CallbackExceptionHelper):
    """
    Wrap a callback such that it can be used as an OCSP callback for the client
    side.

    Annoyingly, OpenSSL defines one OCSP callback but uses it in two different
    ways. For servers, that callback is expected to retrieve some OCSP data and
    hand it to OpenSSL, and may return only SSL_TLSEXT_ERR_OK,
    SSL_TLSEXT_ERR_FATAL, and SSL_TLSEXT_ERR_NOACK. For clients, that callback
    is expected to check the OCSP data, and returns a negative value on error,
    0 if the response is not acceptable, or positive if it is. These are
    mutually exclusive return code behaviours, and they mean that we need two
    helpers so that we always return an appropriate error code if the user's
    code throws an exception.

    Given that we have to have two helpers anyway, these helpers are a bit more
    helpery than most: specifically, they hide a few more of the OpenSSL
    functions so that the user has an easier time writing these callbacks.

    This helper implements the client side.
    """

    def __init__(self, callback):
        _CallbackExceptionHelper.__init__(self)

        @wraps(callback)
        def wrapper(ssl, cdata):
            try:
                conn = Connection._reverse_mapping[ssl]

                # Extract the data if any was provided.
                if cdata != _ffi.NULL:
                    data = _ffi.from_handle(cdata)
                else:
                    data = None

                # Get the OCSP data.
                ocsp_ptr = _ffi.new("unsigned char **")
                ocsp_len = _lib.SSL_get_tlsext_status_ocsp_resp(ssl, ocsp_ptr)
                if ocsp_len < 0:
                    # No OCSP data.
                    ocsp_data = b""
                else:
                    # Copy the OCSP data, then pass it to the callback.
                    ocsp_data = _ffi.buffer(ocsp_ptr[0], ocsp_len)[:]

                valid = callback(conn, ocsp_data, data)

                # Return 1 on success or 0 on error.
                return int(bool(valid))

            except Exception as e:
                self._problems.append(e)
                # Return negative value if an exception is hit.
                return -1

        self.callback = _ffi.callback("int (*)(SSL *, void *)", wrapper)


def _asFileDescriptor(obj):
    fd = None
    if not isinstance(obj, integer_types):
        meth = getattr(obj, "fileno", None)
        if meth is not None:
            obj = meth()

    if isinstance(obj, integer_types):
        fd = obj

    if not isinstance(fd, integer_types):
        raise TypeError("argument must be an int, or have a fileno() method.")
    elif fd < 0:
        raise ValueError(
            "file descriptor cannot be a negative integer (%i)" % (fd,)
        )

    return fd


def SSLeay_version(type):
    """
    Return a string describing the version of OpenSSL in use.

    :param type: One of the :const:`SSLEAY_` constants defined in this module.
    """
    return _ffi.string(_lib.SSLeay_version(type))


def _make_requires(flag, error):
    """
    Builds a decorator that ensures that functions that rely on OpenSSL
    functions that are not present in this build raise NotImplementedError,
    rather than AttributeError coming out of cryptography.

    :param flag: A cryptography flag that guards the functions, e.g.
        ``Cryptography_HAS_NEXTPROTONEG``.
    :param error: The string to be used in the exception if the flag is false.
    """

    def _requires_decorator(func):
        if not flag:

            @wraps(func)
            def explode(*args, **kwargs):
                raise NotImplementedError(error)

            return explode
        else:
            return func

    return _requires_decorator


_requires_alpn = _make_requires(
    _lib.Cryptography_HAS_ALPN, "ALPN not available"
)


_requires_keylog = _make_requires(
    getattr(_lib, "Cryptography_HAS_KEYLOG", None), "Key logging not available"
)


class Session(object):
    """
    A class representing an SSL session.  A session defines certain connection
    parameters which may be re-used to speed up the setup of subsequent
    connections.

    .. versionadded:: 0.14
    """

    pass


class Context(object):
    """
    :class:`OpenSSL.SSL.Context` instances define the parameters for setting
    up new SSL connections.

    :param method: One of SSLv2_METHOD, SSLv3_METHOD, SSLv23_METHOD, or
        TLSv1_METHOD.
    """

    _methods = {
        SSLv2_METHOD: "SSLv2_method",
        SSLv3_METHOD: "SSLv3_method",
        SSLv23_METHOD: "SSLv23_method",
        TLSv1_METHOD: "TLSv1_method",
        TLSv1_1_METHOD: "TLSv1_1_method",
        TLSv1_2_METHOD: "TLSv1_2_method",
    }
    _methods = dict(
        (identifier, getattr(_lib, name))
        for (identifier, name) in _methods.items()
        if getattr(_lib, name, None) is not None
    )

    def __init__(self, method):
        if not isinstance(method, integer_types):
            raise TypeError("method must be an integer")

        try:
            method_func = self._methods[method]
        except KeyError:
            raise ValueError("No such protocol")

        method_obj = method_func()
        _openssl_assert(method_obj != _ffi.NULL)

        context = _lib.SSL_CTX_new(method_obj)
        _openssl_assert(context != _ffi.NULL)
        context = _ffi.gc(context, _lib.SSL_CTX_free)

        # Set SSL_CTX_set_ecdh_auto so that the ECDH curve will be
        # auto-selected. This function was added in 1.0.2 and made a noop in
        # 1.1.0+ (where it is set automatically).
        res = _lib.SSL_CTX_set_ecdh_auto(context, 1)
        _openssl_assert(res == 1)

        self._context = context
        self._passphrase_helper = None
        self._passphrase_callback = None
        self._passphrase_userdata = None
        self._verify_helper = None
        self._verify_callback = None
        self._info_callback = None
        self._keylog_callback = None
        self._tlsext_servername_callback = None
        self._app_data = None
        self._alpn_select_helper = None
        self._alpn_select_callback = None
        self._ocsp_helper = None
        self._ocsp_callback = None
        self._ocsp_data = None

        self.set_mode(_lib.SSL_MODE_ENABLE_PARTIAL_WRITE)

    def load_verify_locations(self, cafile, capath=None):
        """
        Let SSL know where we can find trusted certificates for the certificate
        chain.  Note that the certificates have to be in PEM format.

        If capath is passed, it must be a directory prepared using the
        ``c_rehash`` tool included with OpenSSL.  Either, but not both, of
        *pemfile* or *capath* may be :data:`None`.

        :param cafile: In which file we can find the certificates (``bytes`` or
            ``unicode``).
        :param capath: In which directory we can find the certificates
            (``bytes`` or ``unicode``).

        :return: None
        """
        if cafile is None:
            cafile = _ffi.NULL
        else:
            cafile = _path_string(cafile)

        if capath is None:
            capath = _ffi.NULL
        else:
            capath = _path_string(capath)

        load_result = _lib.SSL_CTX_load_verify_locations(
            self._context, cafile, capath
        )
        if not load_result:
            _raise_current_error()

    def _wrap_callback(self, callback):
        @wraps(callback)
        def wrapper(size, verify, userdata):
            return callback(size, verify, self._passphrase_userdata)

        return _PassphraseHelper(
            FILETYPE_PEM, wrapper, more_args=True, truncate=True
        )

    def set_passwd_cb(self, callback, userdata=None):
        """
        Set the passphrase callback.  This function will be called
        when a private key with a passphrase is loaded.

        :param callback: The Python callback to use.  This must accept three
            positional arguments.  First, an integer giving the maximum length
            of the passphrase it may return.  If the returned passphrase is
            longer than this, it will be truncated.  Second, a boolean value
            which will be true if the user should be prompted for the
            passphrase twice and the callback should verify that the two values
            supplied are equal. Third, the value given as the *userdata*
            parameter to :meth:`set_passwd_cb`.  The *callback* must return
            a byte string. If an error occurs, *callback* should return a false
            value (e.g. an empty string).
        :param userdata: (optional) A Python object which will be given as
                         argument to the callback
        :return: None
        """
        if not callable(callback):
            raise TypeError("callback must be callable")

        self._passphrase_helper = self._wrap_callback(callback)
        self._passphrase_callback = self._passphrase_helper.callback
        _lib.SSL_CTX_set_default_passwd_cb(
            self._context, self._passphrase_callback
        )
        self._passphrase_userdata = userdata

    def set_default_verify_paths(self):
        """
        Specify that the platform provided CA certificates are to be used for
        verification purposes. This method has some caveats related to the
        binary wheels that cryptography (pyOpenSSL's primary dependency) ships:

        *   macOS will only load certificates using this method if the user has
            the ``openssl@1.1`` `Homebrew <https://brew.sh>`_ formula installed
            in the default location.
        *   Windows will not work.
        *   manylinux1 cryptography wheels will work on most common Linux
            distributions in pyOpenSSL 17.1.0 and above.  pyOpenSSL detects the
            manylinux1 wheel and attempts to load roots via a fallback path.

        :return: None
        """
        # SSL_CTX_set_default_verify_paths will attempt to load certs from
        # both a cafile and capath that are set at compile time. However,
        # it will first check environment variables and, if present, load
        # those paths instead
        set_result = _lib.SSL_CTX_set_default_verify_paths(self._context)
        _openssl_assert(set_result == 1)
        # After attempting to set default_verify_paths we need to know whether
        # to go down the fallback path.
        # First we'll check to see if any env vars have been set. If so,
        # we won't try to do anything else because the user has set the path
        # themselves.
        dir_env_var = _ffi.string(_lib.X509_get_default_cert_dir_env()).decode(
            "ascii"
        )
        file_env_var = _ffi.string(
            _lib.X509_get_default_cert_file_env()
        ).decode("ascii")
        if not self._check_env_vars_set(dir_env_var, file_env_var):
            default_dir = _ffi.string(_lib.X509_get_default_cert_dir())
            default_file = _ffi.string(_lib.X509_get_default_cert_file())
            # Now we check to see if the default_dir and default_file are set
            # to the exact values we use in our manylinux1 builds. If they are
            # then we know to load the fallbacks
            if (
                default_dir == _CRYPTOGRAPHY_MANYLINUX1_CA_DIR
                and default_file == _CRYPTOGRAPHY_MANYLINUX1_CA_FILE
            ):
                # This is manylinux1, let's load our fallback paths
                self._fallback_default_verify_paths(
                    _CERTIFICATE_FILE_LOCATIONS, _CERTIFICATE_PATH_LOCATIONS
                )

    def _check_env_vars_set(self, dir_env_var, file_env_var):
        """
        Check to see if the default cert dir/file environment vars are present.

        :return: bool
        """
        return (
            os.environ.get(file_env_var) is not None
            or os.environ.get(dir_env_var) is not None
        )

    def _fallback_default_verify_paths(self, file_path, dir_path):
        """
        Default verify paths are based on the compiled version of OpenSSL.
        However, when pyca/cryptography is compiled as a manylinux1 wheel
        that compiled location can potentially be wrong. So, like Go, we
        will try a predefined set of paths and attempt to load roots
        from there.

        :return: None
        """
        for cafile in file_path:
            if os.path.isfile(cafile):
                self.load_verify_locations(cafile)
                break

        for capath in dir_path:
            if os.path.isdir(capath):
                self.load_verify_locations(None, capath)
                break

    def use_certificate_chain_file(self, certfile):
        """
        Load a certificate chain from a file.

        :param certfile: The name of the certificate chain file (``bytes`` or
            ``unicode``).  Must be PEM encoded.

        :return: None
        """
        certfile = _path_string(certfile)

        result = _lib.SSL_CTX_use_certificate_chain_file(
            self._context, certfile
        )
        if not result:
            _raise_current_error()

    def use_certificate_file(self, certfile, filetype=FILETYPE_PEM):
        """
        Load a certificate from a file

        :param certfile: The name of the certificate file (``bytes`` or
            ``unicode``).
        :param filetype: (optional) The encoding of the file, which is either
            :const:`FILETYPE_PEM` or :const:`FILETYPE_ASN1`.  The default is
            :const:`FILETYPE_PEM`.

        :return: None
        """
        certfile = _path_string(certfile)
        if not isinstance(filetype, integer_types):
            raise TypeError("filetype must be an integer")

        use_result = _lib.SSL_CTX_use_certificate_file(
            self._context, certfile, filetype
        )
        if not use_result:
            _raise_current_error()

    def use_certificate(self, cert):
        """
        Load a certificate from a X509 object

        :param cert: The X509 object
        :return: None
        """
        if not isinstance(cert, X509):
            raise TypeError("cert must be an X509 instance")

        use_result = _lib.SSL_CTX_use_certificate(self._context, cert._x509)
        if not use_result:
            _raise_current_error()

    def add_extra_chain_cert(self, certobj):
        """
        Add certificate to chain

        :param certobj: The X509 certificate object to add to the chain
        :return: None
        """
        if not isinstance(certobj, X509):
            raise TypeError("certobj must be an X509 instance")

        copy = _lib.X509_dup(certobj._x509)
        add_result = _lib.SSL_CTX_add_extra_chain_cert(self._context, copy)
        if not add_result:
            # TODO: This is untested.
            _lib.X509_free(copy)
            _raise_current_error()

    def _raise_passphrase_exception(self):
        if self._passphrase_helper is not None:
            self._passphrase_helper.raise_if_problem(Error)

        _raise_current_error()

    def use_privatekey_file(self, keyfile, filetype=_UNSPECIFIED):
        """
        Load a private key from a file

        :param keyfile: The name of the key file (``bytes`` or ``unicode``)
        :param filetype: (optional) The encoding of the file, which is either
            :const:`FILETYPE_PEM` or :const:`FILETYPE_ASN1`.  The default is
            :const:`FILETYPE_PEM`.

        :return: None
        """
        keyfile = _path_string(keyfile)

        if filetype is _UNSPECIFIED:
            filetype = FILETYPE_PEM
        elif not isinstance(filetype, integer_types):
            raise TypeError("filetype must be an integer")

        use_result = _lib.SSL_CTX_use_PrivateKey_file(
            self._context, keyfile, filetype
        )
        if not use_result:
            self._raise_passphrase_exception()

    def use_privatekey(self, pkey):
        """
        Load a private key from a PKey object

        :param pkey: The PKey object
        :return: None
        """
        if not isinstance(pkey, PKey):
            raise TypeError("pkey must be a PKey instance")

        use_result = _lib.SSL_CTX_use_PrivateKey(self._context, pkey._pkey)
        if not use_result:
            self._raise_passphrase_exception()

    def check_privatekey(self):
        """
        Check if the private key (loaded with :meth:`use_privatekey`) matches
        the certificate (loaded with :meth:`use_certificate`)

        :return: :data:`None` (raises :exc:`Error` if something's wrong)
        """
        if not _lib.SSL_CTX_check_private_key(self._context):
            _raise_current_error()

    def load_client_ca(self, cafile):
        """
        Load the trusted certificates that will be sent to the client.  Does
        not actually imply any of the certificates are trusted; that must be
        configured separately.

        :param bytes cafile: The path to a certificates file in PEM format.
        :return: None
        """
        ca_list = _lib.SSL_load_client_CA_file(
            _text_to_bytes_and_warn("cafile", cafile)
        )
        _openssl_assert(ca_list != _ffi.NULL)
        _lib.SSL_CTX_set_client_CA_list(self._context, ca_list)

    def set_session_id(self, buf):
        """
        Set the session id to *buf* within which a session can be reused for
        this Context object.  This is needed when doing session resumption,
        because there is no way for a stored session to know which Context
        object it is associated with.

        :param bytes buf: The session id.

        :returns: None
        """
        buf = _text_to_bytes_and_warn("buf", buf)
        _openssl_assert(
            _lib.SSL_CTX_set_session_id_context(self._context, buf, len(buf))
            == 1
        )

    def set_session_cache_mode(self, mode):
        """
        Set the behavior of the session cache used by all connections using
        this Context.  The previously set mode is returned.  See
        :const:`SESS_CACHE_*` for details about particular modes.

        :param mode: One or more of the SESS_CACHE_* flags (combine using
            bitwise or)
        :returns: The previously set caching mode.

        .. versionadded:: 0.14
        """
        if not isinstance(mode, integer_types):
            raise TypeError("mode must be an integer")

        return _lib.SSL_CTX_set_session_cache_mode(self._context, mode)

    def get_session_cache_mode(self):
        """
        Get the current session cache mode.

        :returns: The currently used cache mode.

        .. versionadded:: 0.14
        """
        return _lib.SSL_CTX_get_session_cache_mode(self._context)

    def set_verify(self, mode, callback=None):
        """
        Set the verification flags for this Context object to *mode* and
        specify that *callback* should be used for verification callbacks.

        :param mode: The verify mode, this should be one of
            :const:`VERIFY_NONE` and :const:`VERIFY_PEER`. If
            :const:`VERIFY_PEER` is used, *mode* can be OR:ed with
            :const:`VERIFY_FAIL_IF_NO_PEER_CERT` and
            :const:`VERIFY_CLIENT_ONCE` to further control the behaviour.
        :param callback: The optional Python verification callback to use.
            This should take five arguments: A Connection object, an X509
            object, and three integer variables, which are in turn potential
            error number, error depth and return code. *callback* should
            return True if verification passes and False otherwise.
            If omitted, OpenSSL's default verification is used.
        :return: None

        See SSL_CTX_set_verify(3SSL) for further details.
        """
        if not isinstance(mode, integer_types):
            raise TypeError("mode must be an integer")

        if callback is None:
            self._verify_helper = None
            self._verify_callback = None
            _lib.SSL_CTX_set_verify(self._context, mode, _ffi.NULL)
        else:
            if not callable(callback):
                raise TypeError("callback must be callable")

            self._verify_helper = _VerifyHelper(callback)
            self._verify_callback = self._verify_helper.callback
            _lib.SSL_CTX_set_verify(self._context, mode, self._verify_callback)

    def set_verify_depth(self, depth):
        """
        Set the maximum depth for the certificate chain verification that shall
        be allowed for this Context object.

        :param depth: An integer specifying the verify depth
        :return: None
        """
        if not isinstance(depth, integer_types):
            raise TypeError("depth must be an integer")

        _lib.SSL_CTX_set_verify_depth(self._context, depth)

    def get_verify_mode(self):
        """
        Retrieve the Context object's verify mode, as set by
        :meth:`set_verify`.

        :return: The verify mode
        """
        return _lib.SSL_CTX_get_verify_mode(self._context)

    def get_verify_depth(self):
        """
        Retrieve the Context object's verify depth, as set by
        :meth:`set_verify_depth`.

        :return: The verify depth
        """
        return _lib.SSL_CTX_get_verify_depth(self._context)

    def load_tmp_dh(self, dhfile):
        """
        Load parameters for Ephemeral Diffie-Hellman

        :param dhfile: The file to load EDH parameters from (``bytes`` or
            ``unicode``).

        :return: None
        """
        dhfile = _path_string(dhfile)

        bio = _lib.BIO_new_file(dhfile, b"r")
        if bio == _ffi.NULL:
            _raise_current_error()
        bio = _ffi.gc(bio, _lib.BIO_free)

        dh = _lib.PEM_read_bio_DHparams(bio, _ffi.NULL, _ffi.NULL, _ffi.NULL)
        dh = _ffi.gc(dh, _lib.DH_free)
        res = _lib.SSL_CTX_set_tmp_dh(self._context, dh)
        _openssl_assert(res == 1)

    def set_tmp_ecdh(self, curve):
        """
        Select a curve to use for ECDHE key exchange.

        :param curve: A curve object to use as returned by either
            :meth:`OpenSSL.crypto.get_elliptic_curve` or
            :meth:`OpenSSL.crypto.get_elliptic_curves`.

        :return: None
        """
        _lib.SSL_CTX_set_tmp_ecdh(self._context, curve._to_EC_KEY())

    def set_cipher_list(self, cipher_list):
        """
        Set the list of ciphers to be used in this context.

        See the OpenSSL manual for more information (e.g.
        :manpage:`ciphers(1)`).

        :param bytes cipher_list: An OpenSSL cipher string.
        :return: None
        """
        cipher_list = _text_to_bytes_and_warn("cipher_list", cipher_list)

        if not isinstance(cipher_list, bytes):
            raise TypeError("cipher_list must be a byte string.")

        _openssl_assert(
            _lib.SSL_CTX_set_cipher_list(self._context, cipher_list) == 1
        )
        # In OpenSSL 1.1.1 setting the cipher list will always return TLS 1.3
        # ciphers even if you pass an invalid cipher. Applications (like
        # Twisted) have tests that depend on an error being raised if an
        # invalid cipher string is passed, but without the following check
        # for the TLS 1.3 specific cipher suites it would never error.
        tmpconn = Connection(self, None)
        if tmpconn.get_cipher_list() == [
            "TLS_AES_256_GCM_SHA384",
            "TLS_CHACHA20_POLY1305_SHA256",
            "TLS_AES_128_GCM_SHA256",
        ]:
            raise Error(
                [
                    (
                        "SSL routines",
                        "SSL_CTX_set_cipher_list",
                        "no cipher match",
                    ),
                ],
            )

    def set_client_ca_list(self, certificate_authorities):
        """
        Set the list of preferred client certificate signers for this server
        context.

        This list of certificate authorities will be sent to the client when
        the server requests a client certificate.

        :param certificate_authorities: a sequence of X509Names.
        :return: None

        .. versionadded:: 0.10
        """
        name_stack = _lib.sk_X509_NAME_new_null()
        _openssl_assert(name_stack != _ffi.NULL)

        try:
            for ca_name in certificate_authorities:
                if not isinstance(ca_name, X509Name):
                    raise TypeError(
                        "client CAs must be X509Name objects, not %s "
                        "objects" % (type(ca_name).__name__,)
                    )
                copy = _lib.X509_NAME_dup(ca_name._name)
                _openssl_assert(copy != _ffi.NULL)
                push_result = _lib.sk_X509_NAME_push(name_stack, copy)
                if not push_result:
                    _lib.X509_NAME_free(copy)
                    _raise_current_error()
        except Exception:
            _lib.sk_X509_NAME_free(name_stack)
            raise

        _lib.SSL_CTX_set_client_CA_list(self._context, name_stack)

    def add_client_ca(self, certificate_authority):
        """
        Add the CA certificate to the list of preferred signers for this
        context.

        The list of certificate authorities will be sent to the client when the
        server requests a client certificate.

        :param certificate_authority: certificate authority's X509 certificate.
        :return: None

        .. versionadded:: 0.10
        """
        if not isinstance(certificate_authority, X509):
            raise TypeError("certificate_authority must be an X509 instance")

        add_result = _lib.SSL_CTX_add_client_CA(
            self._context, certificate_authority._x509
        )
        _openssl_assert(add_result == 1)

    def set_timeout(self, timeout):
        """
        Set the timeout for newly created sessions for this Context object to
        *timeout*.  The default value is 300 seconds. See the OpenSSL manual
        for more information (e.g. :manpage:`SSL_CTX_set_timeout(3)`).

        :param timeout: The timeout in (whole) seconds
        :return: The previous session timeout
        """
        if not isinstance(timeout, integer_types):
            raise TypeError("timeout must be an integer")

        return _lib.SSL_CTX_set_timeout(self._context, timeout)

    def get_timeout(self):
        """
        Retrieve session timeout, as set by :meth:`set_timeout`. The default
        is 300 seconds.

        :return: The session timeout
        """
        return _lib.SSL_CTX_get_timeout(self._context)

    def set_info_callback(self, callback):
        """
        Set the information callback to *callback*. This function will be
        called from time to time during SSL handshakes.

        :param callback: The Python callback to use.  This should take three
            arguments: a Connection object and two integers.  The first integer
            specifies where in the SSL handshake the function was called, and
            the other the return code from a (possibly failed) internal
            function call.
        :return: None
        """

        @wraps(callback)
        def wrapper(ssl, where, return_code):
            callback(Connection._reverse_mapping[ssl], where, return_code)

        self._info_callback = _ffi.callback(
            "void (*)(const SSL *, int, int)", wrapper
        )
        _lib.SSL_CTX_set_info_callback(self._context, self._info_callback)

    @_requires_keylog
    def set_keylog_callback(self, callback):
        """
        Set the TLS key logging callback to *callback*. This function will be
        called whenever TLS key material is generated or received, in order
        to allow applications to store this keying material for debugging
        purposes.

        :param callback: The Python callback to use.  This should take two
            arguments: a Connection object and a bytestring that contains
            the key material in the format used by NSS for its SSLKEYLOGFILE
            debugging output.
        :return: None
        """

        @wraps(callback)
        def wrapper(ssl, line):
            line = _ffi.string(line)
            callback(Connection._reverse_mapping[ssl], line)

        self._keylog_callback = _ffi.callback(
            "void (*)(const SSL *, const char *)", wrapper
        )
        _lib.SSL_CTX_set_keylog_callback(self._context, self._keylog_callback)

    def get_app_data(self):
        """
        Get the application data (supplied via :meth:`set_app_data()`)

        :return: The application data
        """
        return self._app_data

    def set_app_data(self, data):
        """
        Set the application data (will be returned from get_app_data())

        :param data: Any Python object
        :return: None
        """
        self._app_data = data

    def get_cert_store(self):
        """
        Get the certificate store for the context.  This can be used to add
        "trusted" certificates without using the
        :meth:`load_verify_locations` method.

        :return: A X509Store object or None if it does not have one.
        """
        store = _lib.SSL_CTX_get_cert_store(self._context)
        if store == _ffi.NULL:
            # TODO: This is untested.
            return None

        pystore = X509Store.__new__(X509Store)
        pystore._store = store
        return pystore

    def set_options(self, options):
        """
        Add options. Options set before are not cleared!
        This method should be used with the :const:`OP_*` constants.

        :param options: The options to add.
        :return: The new option bitmask.
        """
        if not isinstance(options, integer_types):
            raise TypeError("options must be an integer")

        return _lib.SSL_CTX_set_options(self._context, options)

    def set_mode(self, mode):
        """
        Add modes via bitmask. Modes set before are not cleared!  This method
        should be used with the :const:`MODE_*` constants.

        :param mode: The mode to add.
        :return: The new mode bitmask.
        """
        if not isinstance(mode, integer_types):
            raise TypeError("mode must be an integer")

        return _lib.SSL_CTX_set_mode(self._context, mode)

    def set_tlsext_servername_callback(self, callback):
        """
        Specify a callback function to be called when clients specify a server
        name.

        :param callback: The callback function.  It will be invoked with one
            argument, the Connection instance.

        .. versionadded:: 0.13
        """

        @wraps(callback)
        def wrapper(ssl, alert, arg):
            callback(Connection._reverse_mapping[ssl])
            return 0

        self._tlsext_servername_callback = _ffi.callback(
            "int (*)(SSL *, int *, void *)", wrapper
        )
        _lib.SSL_CTX_set_tlsext_servername_callback(
            self._context, self._tlsext_servername_callback
        )

    def set_tlsext_use_srtp(self, profiles):
        """
        Enable support for negotiating SRTP keying material.

        :param bytes profiles: A colon delimited list of protection profile
            names, like ``b'SRTP_AES128_CM_SHA1_80:SRTP_AES128_CM_SHA1_32'``.
        :return: None
        """
        if not isinstance(profiles, bytes):
            raise TypeError("profiles must be a byte string.")

        _openssl_assert(
            _lib.SSL_CTX_set_tlsext_use_srtp(self._context, profiles) == 0
        )

    @_requires_alpn
    def set_alpn_protos(self, protos):
        """
        Specify the protocols that the client is prepared to speak after the
        TLS connection has been negotiated using Application Layer Protocol
        Negotiation.

        :param protos: A list of the protocols to be offered to the server.
            This list should be a Python list of bytestrings representing the
            protocols to offer, e.g. ``[b'http/1.1', b'spdy/2']``.
        """
        # Take the list of protocols and join them together, prefixing them
        # with their lengths.
        protostr = b"".join(
            chain.from_iterable((int2byte(len(p)), p) for p in protos)
        )

        # Build a C string from the list. We don't need to save this off
        # because OpenSSL immediately copies the data out.
        input_str = _ffi.new("unsigned char[]", protostr)
        _lib.SSL_CTX_set_alpn_protos(self._context, input_str, len(protostr))

    @_requires_alpn
    def set_alpn_select_callback(self, callback):
        """
        Specify a callback function that will be called on the server when a
        client offers protocols using ALPN.

        :param callback: The callback function.  It will be invoked with two
            arguments: the Connection, and a list of offered protocols as
            bytestrings, e.g ``[b'http/1.1', b'spdy/2']``.  It can return
            one of those bytestrings to indicate the chosen protocol, the
            empty bytestring to terminate the TLS connection, or the
            :py:obj:`NO_OVERLAPPING_PROTOCOLS` to indicate that no offered
            protocol was selected, but that the connection should not be
            aborted.
        """
        self._alpn_select_helper = _ALPNSelectHelper(callback)
        self._alpn_select_callback = self._alpn_select_helper.callback
        _lib.SSL_CTX_set_alpn_select_cb(
            self._context, self._alpn_select_callback, _ffi.NULL
        )

    def _set_ocsp_callback(self, helper, data):
        """
        This internal helper does the common work for
        ``set_ocsp_server_callback`` and ``set_ocsp_client_callback``, which is
        almost all of it.
        """
        self._ocsp_helper = helper
        self._ocsp_callback = helper.callback
        if data is None:
            self._ocsp_data = _ffi.NULL
        else:
            self._ocsp_data = _ffi.new_handle(data)

        rc = _lib.SSL_CTX_set_tlsext_status_cb(
            self._context, self._ocsp_callback
        )
        _openssl_assert(rc == 1)
        rc = _lib.SSL_CTX_set_tlsext_status_arg(self._context, self._ocsp_data)
        _openssl_assert(rc == 1)

    def set_ocsp_server_callback(self, callback, data=None):
        """
        Set a callback to provide OCSP data to be stapled to the TLS handshake
        on the server side.

        :param callback: The callback function. It will be invoked with two
            arguments: the Connection, and the optional arbitrary data you have
            provided. The callback must return a bytestring that contains the
            OCSP data to staple to the handshake. If no OCSP data is available
            for this connection, return the empty bytestring.
        :param data: Some opaque data that will be passed into the callback
            function when called. This can be used to avoid needing to do
            complex data lookups or to keep track of what context is being
            used. This parameter is optional.
        """
        helper = _OCSPServerCallbackHelper(callback)
        self._set_ocsp_callback(helper, data)

    def set_ocsp_client_callback(self, callback, data=None):
        """
        Set a callback to validate OCSP data stapled to the TLS handshake on
        the client side.

        :param callback: The callback function. It will be invoked with three
            arguments: the Connection, a bytestring containing the stapled OCSP
            assertion, and the optional arbitrary data you have provided. The
            callback must return a boolean that indicates the result of
            validating the OCSP data: ``True`` if the OCSP data is valid and
            the certificate can be trusted, or ``False`` if either the OCSP
            data is invalid or the certificate has been revoked.
        :param data: Some opaque data that will be passed into the callback
            function when called. This can be used to avoid needing to do
            complex data lookups or to keep track of what context is being
            used. This parameter is optional.
        """
        helper = _OCSPClientCallbackHelper(callback)
        self._set_ocsp_callback(helper, data)


class Connection(object):
    _reverse_mapping = WeakValueDictionary()

    def __init__(self, context, socket=None):
        """
        Create a new Connection object, using the given OpenSSL.SSL.Context
        instance and socket.

        :param context: An SSL Context to use for this connection
        :param socket: The socket to use for transport layer
        """
        if not isinstance(context, Context):
            raise TypeError("context must be a Context instance")

        ssl = _lib.SSL_new(context._context)
        self._ssl = _ffi.gc(ssl, _lib.SSL_free)
        # We set SSL_MODE_AUTO_RETRY to handle situations where OpenSSL returns
        # an SSL_ERROR_WANT_READ when processing a non-application data packet
        # even though there is still data on the underlying transport.
        # See https://github.com/openssl/openssl/issues/6234 for more details.
        _lib.SSL_set_mode(self._ssl, _lib.SSL_MODE_AUTO_RETRY)
        self._context = context
        self._app_data = None

        # References to strings used for Application Layer Protocol
        # Negotiation. These strings get copied at some point but it's well
        # after the callback returns, so we have to hang them somewhere to
        # avoid them getting freed.
        self._alpn_select_callback_args = None

        # Reference the verify_callback of the Context. This ensures that if
        # set_verify is called again after the SSL object has been created we
        # do not point to a dangling reference
        self._verify_helper = context._verify_helper
        self._verify_callback = context._verify_callback

        self._reverse_mapping[self._ssl] = self

        if socket is None:
            self._socket = None
            # Don't set up any gc for these, SSL_free will take care of them.
            self._into_ssl = _lib.BIO_new(_lib.BIO_s_mem())
            _openssl_assert(self._into_ssl != _ffi.NULL)

            self._from_ssl = _lib.BIO_new(_lib.BIO_s_mem())
            _openssl_assert(self._from_ssl != _ffi.NULL)

            _lib.SSL_set_bio(self._ssl, self._into_ssl, self._from_ssl)
        else:
            self._into_ssl = None
            self._from_ssl = None
            self._socket = socket
            set_result = _lib.SSL_set_fd(
                self._ssl, _asFileDescriptor(self._socket)
            )
            _openssl_assert(set_result == 1)

    def __getattr__(self, name):
        """
        Look up attributes on the wrapped socket object if they are not found
        on the Connection object.
        """
        if self._socket is None:
            raise AttributeError(
                "'%s' object has no attribute '%s'"
                % (self.__class__.__name__, name)
            )
        else:
            return getattr(self._socket, name)

    def _raise_ssl_error(self, ssl, result):
        if self._context._verify_helper is not None:
            self._context._verify_helper.raise_if_problem()
        if self._context._alpn_select_helper is not None:
            self._context._alpn_select_helper.raise_if_problem()
        if self._context._ocsp_helper is not None:
            self._context._ocsp_helper.raise_if_problem()

        error = _lib.SSL_get_error(ssl, result)
        if error == _lib.SSL_ERROR_WANT_READ:
            raise WantReadError()
        elif error == _lib.SSL_ERROR_WANT_WRITE:
            raise WantWriteError()
        elif error == _lib.SSL_ERROR_ZERO_RETURN:
            raise ZeroReturnError()
        elif error == _lib.SSL_ERROR_WANT_X509_LOOKUP:
            # TODO: This is untested.
            raise WantX509LookupError()
        elif error == _lib.SSL_ERROR_SYSCALL:
            if _lib.ERR_peek_error() == 0:
                if result < 0:
                    if platform == "win32":
                        errno = _ffi.getwinerror()[0]
                    else:
                        errno = _ffi.errno

                    if errno != 0:
                        raise SysCallError(errno, errorcode.get(errno))
                raise SysCallError(-1, "Unexpected EOF")
            else:
                # TODO: This is untested.
                _raise_current_error()
        elif error == _lib.SSL_ERROR_NONE:
            pass
        else:
            _raise_current_error()

    def get_context(self):
        """
        Retrieve the :class:`Context` object associated with this
        :class:`Connection`.
        """
        return self._context

    def set_context(self, context):
        """
        Switch this connection to a new session context.

        :param context: A :class:`Context` instance giving the new session
            context to use.
        """
        if not isinstance(context, Context):
            raise TypeError("context must be a Context instance")

        _lib.SSL_set_SSL_CTX(self._ssl, context._context)
        self._context = context

    def get_servername(self):
        """
        Retrieve the servername extension value if provided in the client hello
        message, or None if there wasn't one.

        :return: A byte string giving the server name or :data:`None`.

        .. versionadded:: 0.13
        """
        name = _lib.SSL_get_servername(
            self._ssl, _lib.TLSEXT_NAMETYPE_host_name
        )
        if name == _ffi.NULL:
            return None

        return _ffi.string(name)

    def set_tlsext_host_name(self, name):
        """
        Set the value of the servername extension to send in the client hello.

        :param name: A byte string giving the name.

        .. versionadded:: 0.13
        """
        if not isinstance(name, bytes):
            raise TypeError("name must be a byte string")
        elif b"\0" in name:
            raise TypeError("name must not contain NUL byte")

        # XXX I guess this can fail sometimes?
        _lib.SSL_set_tlsext_host_name(self._ssl, name)

    def pending(self):
        """
        Get the number of bytes that can be safely read from the SSL buffer
        (**not** the underlying transport buffer).

        :return: The number of bytes available in the receive buffer.
        """
        return _lib.SSL_pending(self._ssl)

    def send(self, buf, flags=0):
        """
        Send data on the connection. NOTE: If you get one of the WantRead,
        WantWrite or WantX509Lookup exceptions on this, you have to call the
        method again with the SAME buffer.

        :param buf: The string, buffer or memoryview to send
        :param flags: (optional) Included for compatibility with the socket
                      API, the value is ignored
        :return: The number of bytes written
        """
        # Backward compatibility
        buf = _text_to_bytes_and_warn("buf", buf)

        with _from_buffer(buf) as data:
            # check len(buf) instead of len(data) for testability
            if len(buf) > 2147483647:
                raise ValueError(
                    "Cannot send more than 2**31-1 bytes at once."
                )

            result = _lib.SSL_write(self._ssl, data, len(data))
            self._raise_ssl_error(self._ssl, result)

            return result

    write = send

    def sendall(self, buf, flags=0):
        """
        Send "all" data on the connection. This calls send() repeatedly until
        all data is sent. If an error occurs, it's impossible to tell how much
        data has been sent.

        :param buf: The string, buffer or memoryview to send
        :param flags: (optional) Included for compatibility with the socket
                      API, the value is ignored
        :return: The number of bytes written
        """
        buf = _text_to_bytes_and_warn("buf", buf)

        with _from_buffer(buf) as data:

            left_to_send = len(buf)
            total_sent = 0

            while left_to_send:
                # SSL_write's num arg is an int,
                # so we cannot send more than 2**31-1 bytes at once.
                result = _lib.SSL_write(
                    self._ssl, data + total_sent, min(left_to_send, 2147483647)
                )
                self._raise_ssl_error(self._ssl, result)
                total_sent += result
                left_to_send -= result

            return total_sent

    def recv(self, bufsiz, flags=None):
        """
        Receive data on the connection.

        :param bufsiz: The maximum number of bytes to read
        :param flags: (optional) The only supported flag is ``MSG_PEEK``,
            all other flags are ignored.
        :return: The string read from the Connection
        """
        buf = _no_zero_allocator("char[]", bufsiz)
        if flags is not None and flags & socket.MSG_PEEK:
            result = _lib.SSL_peek(self._ssl, buf, bufsiz)
        else:
            result = _lib.SSL_read(self._ssl, buf, bufsiz)
        self._raise_ssl_error(self._ssl, result)
        return _ffi.buffer(buf, result)[:]

    read = recv

    def recv_into(self, buffer, nbytes=None, flags=None):
        """
        Receive data on the connection and copy it directly into the provided
        buffer, rather than creating a new string.

        :param buffer: The buffer to copy into.
        :param nbytes: (optional) The maximum number of bytes to read into the
            buffer. If not present, defaults to the size of the buffer. If
            larger than the size of the buffer, is reduced to the size of the
            buffer.
        :param flags: (optional) The only supported flag is ``MSG_PEEK``,
            all other flags are ignored.
        :return: The number of bytes read into the buffer.
        """
        if nbytes is None:
            nbytes = len(buffer)
        else:
            nbytes = min(nbytes, len(buffer))

        # We need to create a temporary buffer. This is annoying, it would be
        # better if we could pass memoryviews straight into the SSL_read call,
        # but right now we can't. Revisit this if CFFI gets that ability.
        buf = _no_zero_allocator("char[]", nbytes)
        if flags is not None and flags & socket.MSG_PEEK:
            result = _lib.SSL_peek(self._ssl, buf, nbytes)
        else:
            result = _lib.SSL_read(self._ssl, buf, nbytes)
        self._raise_ssl_error(self._ssl, result)

        # This strange line is all to avoid a memory copy. The buffer protocol
        # should allow us to assign a CFFI buffer to the LHS of this line, but
        # on CPython 3.3+ that segfaults. As a workaround, we can temporarily
        # wrap it in a memoryview.
        buffer[:result] = memoryview(_ffi.buffer(buf, result))

        return result

    def _handle_bio_errors(self, bio, result):
        if _lib.BIO_should_retry(bio):
            if _lib.BIO_should_read(bio):
                raise WantReadError()
            elif _lib.BIO_should_write(bio):
                # TODO: This is untested.
                raise WantWriteError()
            elif _lib.BIO_should_io_special(bio):
                # TODO: This is untested.  I think io_special means the socket
                # BIO has a not-yet connected socket.
                raise ValueError("BIO_should_io_special")
            else:
                # TODO: This is untested.
                raise ValueError("unknown bio failure")
        else:
            # TODO: This is untested.
            _raise_current_error()

    def bio_read(self, bufsiz):
        """
        If the Connection was created with a memory BIO, this method can be
        used to read bytes from the write end of that memory BIO.  Many
        Connection methods will add bytes which must be read in this manner or
        the buffer will eventually fill up and the Connection will be able to
        take no further actions.

        :param bufsiz: The maximum number of bytes to read
        :return: The string read.
        """
        if self._from_ssl is None:
            raise TypeError("Connection sock was not None")

        if not isinstance(bufsiz, integer_types):
            raise TypeError("bufsiz must be an integer")

        buf = _no_zero_allocator("char[]", bufsiz)
        result = _lib.BIO_read(self._from_ssl, buf, bufsiz)
        if result <= 0:
            self._handle_bio_errors(self._from_ssl, result)

        return _ffi.buffer(buf, result)[:]

    def bio_write(self, buf):
        """
        If the Connection was created with a memory BIO, this method can be
        used to add bytes to the read end of that memory BIO.  The Connection
        can then read the bytes (for example, in response to a call to
        :meth:`recv`).

        :param buf: The string to put into the memory BIO.
        :return: The number of bytes written
        """
        buf = _text_to_bytes_and_warn("buf", buf)

        if self._into_ssl is None:
            raise TypeError("Connection sock was not None")

        with _from_buffer(buf) as data:
            result = _lib.BIO_write(self._into_ssl, data, len(data))
            if result <= 0:
                self._handle_bio_errors(self._into_ssl, result)
            return result

    def renegotiate(self):
        """
        Renegotiate the session.

        :return: True if the renegotiation can be started, False otherwise
        :rtype: bool
        """
        if not self.renegotiate_pending():
            _openssl_assert(_lib.SSL_renegotiate(self._ssl) == 1)
            return True
        return False

    def do_handshake(self):
        """
        Perform an SSL handshake (usually called after :meth:`renegotiate` or
        one of :meth:`set_accept_state` or :meth:`set_connect_state`). This can
        raise the same exceptions as :meth:`send` and :meth:`recv`.

        :return: None.
        """
        result = _lib.SSL_do_handshake(self._ssl)
        self._raise_ssl_error(self._ssl, result)

    def renegotiate_pending(self):
        """
        Check if there's a renegotiation in progress, it will return False once
        a renegotiation is finished.

        :return: Whether there's a renegotiation in progress
        :rtype: bool
        """
        return _lib.SSL_renegotiate_pending(self._ssl) == 1

    def total_renegotiations(self):
        """
        Find out the total number of renegotiations.

        :return: The number of renegotiations.
        :rtype: int
        """
        return _lib.SSL_total_renegotiations(self._ssl)

    def connect(self, addr):
        """
        Call the :meth:`connect` method of the underlying socket and set up SSL
        on the socket, using the :class:`Context` object supplied to this
        :class:`Connection` object at creation.

        :param addr: A remote address
        :return: What the socket's connect method returns
        """
        _lib.SSL_set_connect_state(self._ssl)
        return self._socket.connect(addr)

    def connect_ex(self, addr):
        """
        Call the :meth:`connect_ex` method of the underlying socket and set up
        SSL on the socket, using the Context object supplied to this Connection
        object at creation. Note that if the :meth:`connect_ex` method of the
        socket doesn't return 0, SSL won't be initialized.

        :param addr: A remove address
        :return: What the socket's connect_ex method returns
        """
        connect_ex = self._socket.connect_ex
        self.set_connect_state()
        return connect_ex(addr)

    def accept(self):
        """
        Call the :meth:`accept` method of the underlying socket and set up SSL
        on the returned socket, using the Context object supplied to this
        :class:`Connection` object at creation.

        :return: A *(conn, addr)* pair where *conn* is the new
            :class:`Connection` object created, and *address* is as returned by
            the socket's :meth:`accept`.
        """
        client, addr = self._socket.accept()
        conn = Connection(self._context, client)
        conn.set_accept_state()
        return (conn, addr)

    def bio_shutdown(self):
        """
        If the Connection was created with a memory BIO, this method can be
        used to indicate that *end of file* has been reached on the read end of
        that memory BIO.

        :return: None
        """
        if self._from_ssl is None:
            raise TypeError("Connection sock was not None")

        _lib.BIO_set_mem_eof_return(self._into_ssl, 0)

    def shutdown(self):
        """
        Send the shutdown message to the Connection.

        :return: True if the shutdown completed successfully (i.e. both sides
                 have sent closure alerts), False otherwise (in which case you
                 call :meth:`recv` or :meth:`send` when the connection becomes
                 readable/writeable).
        """
        result = _lib.SSL_shutdown(self._ssl)
        if result < 0:
            self._raise_ssl_error(self._ssl, result)
        elif result > 0:
            return True
        else:
            return False

    def get_cipher_list(self):
        """
        Retrieve the list of ciphers used by the Connection object.

        :return: A list of native cipher strings.
        """
        ciphers = []
        for i in count():
            result = _lib.SSL_get_cipher_list(self._ssl, i)
            if result == _ffi.NULL:
                break
            ciphers.append(_native(_ffi.string(result)))
        return ciphers

    def get_client_ca_list(self):
        """
        Get CAs whose certificates are suggested for client authentication.

        :return: If this is a server connection, the list of certificate
            authorities that will be sent or has been sent to the client, as
            controlled by this :class:`Connection`'s :class:`Context`.

            If this is a client connection, the list will be empty until the
            connection with the server is established.

        .. versionadded:: 0.10
        """
        ca_names = _lib.SSL_get_client_CA_list(self._ssl)
        if ca_names == _ffi.NULL:
            # TODO: This is untested.
            return []

        result = []
        for i in range(_lib.sk_X509_NAME_num(ca_names)):
            name = _lib.sk_X509_NAME_value(ca_names, i)
            copy = _lib.X509_NAME_dup(name)
            _openssl_assert(copy != _ffi.NULL)

            pyname = X509Name.__new__(X509Name)
            pyname._name = _ffi.gc(copy, _lib.X509_NAME_free)
            result.append(pyname)
        return result

    def makefile(self, *args, **kwargs):
        """
        The makefile() method is not implemented, since there is no dup
        semantics for SSL connections

        :raise: NotImplementedError
        """
        raise NotImplementedError(
            "Cannot make file object of OpenSSL.SSL.Connection"
        )

    def get_app_data(self):
        """
        Retrieve application data as set by :meth:`set_app_data`.

        :return: The application data
        """
        return self._app_data

    def set_app_data(self, data):
        """
        Set application data

        :param data: The application data
        :return: None
        """
        self._app_data = data

    def get_shutdown(self):
        """
        Get the shutdown state of the Connection.

        :return: The shutdown state, a bitvector of SENT_SHUTDOWN,
            RECEIVED_SHUTDOWN.
        """
        return _lib.SSL_get_shutdown(self._ssl)

    def set_shutdown(self, state):
        """
        Set the shutdown state of the Connection.

        :param state: bitvector of SENT_SHUTDOWN, RECEIVED_SHUTDOWN.
        :return: None
        """
        if not isinstance(state, integer_types):
            raise TypeError("state must be an integer")

        _lib.SSL_set_shutdown(self._ssl, state)

    def get_state_string(self):
        """
        Retrieve a verbose string detailing the state of the Connection.

        :return: A string representing the state
        :rtype: bytes
        """
        return _ffi.string(_lib.SSL_state_string_long(self._ssl))

    def server_random(self):
        """
        Retrieve the random value used with the server hello message.

        :return: A string representing the state
        """
        session = _lib.SSL_get_session(self._ssl)
        if session == _ffi.NULL:
            return None
        length = _lib.SSL_get_server_random(self._ssl, _ffi.NULL, 0)
        _openssl_assert(length > 0)
        outp = _no_zero_allocator("unsigned char[]", length)
        _lib.SSL_get_server_random(self._ssl, outp, length)
        return _ffi.buffer(outp, length)[:]

    def client_random(self):
        """
        Retrieve the random value used with the client hello message.

        :return: A string representing the state
        """
        session = _lib.SSL_get_session(self._ssl)
        if session == _ffi.NULL:
            return None

        length = _lib.SSL_get_client_random(self._ssl, _ffi.NULL, 0)
        _openssl_assert(length > 0)
        outp = _no_zero_allocator("unsigned char[]", length)
        _lib.SSL_get_client_random(self._ssl, outp, length)
        return _ffi.buffer(outp, length)[:]

    def master_key(self):
        """
        Retrieve the value of the master key for this session.

        :return: A string representing the state
        """
        session = _lib.SSL_get_session(self._ssl)
        if session == _ffi.NULL:
            return None

        length = _lib.SSL_SESSION_get_master_key(session, _ffi.NULL, 0)
        _openssl_assert(length > 0)
        outp = _no_zero_allocator("unsigned char[]", length)
        _lib.SSL_SESSION_get_master_key(session, outp, length)
        return _ffi.buffer(outp, length)[:]

    def export_keying_material(self, label, olen, context=None):
        """
        Obtain keying material for application use.

        :param: label - a disambiguating label string as described in RFC 5705
        :param: olen - the length of the exported key material in bytes
        :param: context - a per-association context value
        :return: the exported key material bytes or None
        """
        outp = _no_zero_allocator("unsigned char[]", olen)
        context_buf = _ffi.NULL
        context_len = 0
        use_context = 0
        if context is not None:
            context_buf = context
            context_len = len(context)
            use_context = 1
        success = _lib.SSL_export_keying_material(
            self._ssl,
            outp,
            olen,
            label,
            len(label),
            context_buf,
            context_len,
            use_context,
        )
        _openssl_assert(success == 1)
        return _ffi.buffer(outp, olen)[:]

    def sock_shutdown(self, *args, **kwargs):
        """
        Call the :meth:`shutdown` method of the underlying socket.
        See :manpage:`shutdown(2)`.

        :return: What the socket's shutdown() method returns
        """
        return self._socket.shutdown(*args, **kwargs)

    def get_certificate(self):
        """
        Retrieve the local certificate (if any)

        :return: The local certificate
        """
        cert = _lib.SSL_get_certificate(self._ssl)
        if cert != _ffi.NULL:
            _lib.X509_up_ref(cert)
            return X509._from_raw_x509_ptr(cert)
        return None

    def get_peer_certificate(self):
        """
        Retrieve the other side's certificate (if any)

        :return: The peer's certificate
        """
        cert = _lib.SSL_get_peer_certificate(self._ssl)
        if cert != _ffi.NULL:
            return X509._from_raw_x509_ptr(cert)
        return None

    @staticmethod
    def _cert_stack_to_list(cert_stack):
        """
        Internal helper to convert a STACK_OF(X509) to a list of X509
        instances.
        """
        result = []
        for i in range(_lib.sk_X509_num(cert_stack)):
            cert = _lib.sk_X509_value(cert_stack, i)
            _openssl_assert(cert != _ffi.NULL)
            res = _lib.X509_up_ref(cert)
            _openssl_assert(res >= 1)
            pycert = X509._from_raw_x509_ptr(cert)
            result.append(pycert)
        return result

    def get_peer_cert_chain(self):
        """
        Retrieve the other side's certificate (if any)

        :return: A list of X509 instances giving the peer's certificate chain,
                 or None if it does not have one.
        """
        cert_stack = _lib.SSL_get_peer_cert_chain(self._ssl)
        if cert_stack == _ffi.NULL:
            return None

        return self._cert_stack_to_list(cert_stack)

    def get_verified_chain(self):
        """
        Retrieve the verified certificate chain of the peer including the
        peer's end entity certificate. It must be called after a session has
        been successfully established. If peer verification was not successful
        the chain may be incomplete, invalid, or None.

        :return: A list of X509 instances giving the peer's verified
                 certificate chain, or None if it does not have one.

        .. versionadded:: 20.0
        """
        # OpenSSL 1.1+
        cert_stack = _lib.SSL_get0_verified_chain(self._ssl)
        if cert_stack == _ffi.NULL:
            return None

        return self._cert_stack_to_list(cert_stack)

    def want_read(self):
        """
        Checks if more data has to be read from the transport layer to complete
        an operation.

        :return: True iff more data has to be read
        """
        return _lib.SSL_want_read(self._ssl)

    def want_write(self):
        """
        Checks if there is data to write to the transport layer to complete an
        operation.

        :return: True iff there is data to write
        """
        return _lib.SSL_want_write(self._ssl)

    def set_accept_state(self):
        """
        Set the connection to work in server mode. The handshake will be
        handled automatically by read/write.

        :return: None
        """
        _lib.SSL_set_accept_state(self._ssl)

    def set_connect_state(self):
        """
        Set the connection to work in client mode. The handshake will be
        handled automatically by read/write.

        :return: None
        """
        _lib.SSL_set_connect_state(self._ssl)

    def get_session(self):
        """
        Returns the Session currently used.

        :return: An instance of :class:`OpenSSL.SSL.Session` or
            :obj:`None` if no session exists.

        .. versionadded:: 0.14
        """
        session = _lib.SSL_get1_session(self._ssl)
        if session == _ffi.NULL:
            return None

        pysession = Session.__new__(Session)
        pysession._session = _ffi.gc(session, _lib.SSL_SESSION_free)
        return pysession

    def set_session(self, session):
        """
        Set the session to be used when the TLS/SSL connection is established.

        :param session: A Session instance representing the session to use.
        :returns: None

        .. versionadded:: 0.14
        """
        if not isinstance(session, Session):
            raise TypeError("session must be a Session instance")

        result = _lib.SSL_set_session(self._ssl, session._session)
        _openssl_assert(result == 1)

    def _get_finished_message(self, function):
        """
        Helper to implement :meth:`get_finished` and
        :meth:`get_peer_finished`.

        :param function: Either :data:`SSL_get_finished`: or
            :data:`SSL_get_peer_finished`.

        :return: :data:`None` if the desired message has not yet been
            received, otherwise the contents of the message.
        :rtype: :class:`bytes` or :class:`NoneType`
        """
        # The OpenSSL documentation says nothing about what might happen if the
        # count argument given is zero.  Specifically, it doesn't say whether
        # the output buffer may be NULL in that case or not.  Inspection of the
        # implementation reveals that it calls memcpy() unconditionally.
        # Section 7.1.4, paragraph 1 of the C standard suggests that
        # memcpy(NULL, source, 0) is not guaranteed to produce defined (let
        # alone desirable) behavior (though it probably does on just about
        # every implementation...)
        #
        # Allocate a tiny buffer to pass in (instead of just passing NULL as
        # one might expect) for the initial call so as to be safe against this
        # potentially undefined behavior.
        empty = _ffi.new("char[]", 0)
        size = function(self._ssl, empty, 0)
        if size == 0:
            # No Finished message so far.
            return None

        buf = _no_zero_allocator("char[]", size)
        function(self._ssl, buf, size)
        return _ffi.buffer(buf, size)[:]

    def get_finished(self):
        """
        Obtain the latest TLS Finished message that we sent.

        :return: The contents of the message or :obj:`None` if the TLS
            handshake has not yet completed.
        :rtype: :class:`bytes` or :class:`NoneType`

        .. versionadded:: 0.15
        """
        return self._get_finished_message(_lib.SSL_get_finished)

    def get_peer_finished(self):
        """
        Obtain the latest TLS Finished message that we received from the peer.

        :return: The contents of the message or :obj:`None` if the TLS
            handshake has not yet completed.
        :rtype: :class:`bytes` or :class:`NoneType`

        .. versionadded:: 0.15
        """
        return self._get_finished_message(_lib.SSL_get_peer_finished)

    def get_cipher_name(self):
        """
        Obtain the name of the currently used cipher.

        :returns: The name of the currently used cipher or :obj:`None`
            if no connection has been established.
        :rtype: :class:`unicode` or :class:`NoneType`

        .. versionadded:: 0.15
        """
        cipher = _lib.SSL_get_current_cipher(self._ssl)
        if cipher == _ffi.NULL:
            return None
        else:
            name = _ffi.string(_lib.SSL_CIPHER_get_name(cipher))
            return name.decode("utf-8")

    def get_cipher_bits(self):
        """
        Obtain the number of secret bits of the currently used cipher.

        :returns: The number of secret bits of the currently used cipher
            or :obj:`None` if no connection has been established.
        :rtype: :class:`int` or :class:`NoneType`

        .. versionadded:: 0.15
        """
        cipher = _lib.SSL_get_current_cipher(self._ssl)
        if cipher == _ffi.NULL:
            return None
        else:
            return _lib.SSL_CIPHER_get_bits(cipher, _ffi.NULL)

    def get_cipher_version(self):
        """
        Obtain the protocol version of the currently used cipher.

        :returns: The protocol name of the currently used cipher
            or :obj:`None` if no connection has been established.
        :rtype: :class:`unicode` or :class:`NoneType`

        .. versionadded:: 0.15
        """
        cipher = _lib.SSL_get_current_cipher(self._ssl)
        if cipher == _ffi.NULL:
            return None
        else:
            version = _ffi.string(_lib.SSL_CIPHER_get_version(cipher))
            return version.decode("utf-8")

    def get_protocol_version_name(self):
        """
        Retrieve the protocol version of the current connection.

        :returns: The TLS version of the current connection, for example
            the value for TLS 1.2 would be ``TLSv1.2``or ``Unknown``
            for connections that were not successfully established.
        :rtype: :class:`unicode`
        """
        version = _ffi.string(_lib.SSL_get_version(self._ssl))
        return version.decode("utf-8")

    def get_protocol_version(self):
        """
        Retrieve the SSL or TLS protocol version of the current connection.

        :returns: The TLS version of the current connection.  For example,
            it will return ``0x769`` for connections made over TLS version 1.
        :rtype: :class:`int`
        """
        version = _lib.SSL_version(self._ssl)
        return version

    @_requires_alpn
    def set_alpn_protos(self, protos):
        """
        Specify the client's ALPN protocol list.

        These protocols are offered to the server during protocol negotiation.

        :param protos: A list of the protocols to be offered to the server.
            This list should be a Python list of bytestrings representing the
            protocols to offer, e.g. ``[b'http/1.1', b'spdy/2']``.
        """
        # Take the list of protocols and join them together, prefixing them
        # with their lengths.
        protostr = b"".join(
            chain.from_iterable((int2byte(len(p)), p) for p in protos)
        )

        # Build a C string from the list. We don't need to save this off
        # because OpenSSL immediately copies the data out.
        input_str = _ffi.new("unsigned char[]", protostr)
        _lib.SSL_set_alpn_protos(self._ssl, input_str, len(protostr))

    @_requires_alpn
    def get_alpn_proto_negotiated(self):
        """
        Get the protocol that was negotiated by ALPN.

        :returns: A bytestring of the protocol name.  If no protocol has been
            negotiated yet, returns an empty string.
        """
        data = _ffi.new("unsigned char **")
        data_len = _ffi.new("unsigned int *")

        _lib.SSL_get0_alpn_selected(self._ssl, data, data_len)

        if not data_len:
            return b""

        return _ffi.buffer(data[0], data_len[0])[:]

    def request_ocsp(self):
        """
        Called to request that the server sends stapled OCSP data, if
        available. If this is not called on the client side then the server
        will not send OCSP data. Should be used in conjunction with
        :meth:`Context.set_ocsp_client_callback`.
        """
        rc = _lib.SSL_set_tlsext_status_type(
            self._ssl, _lib.TLSEXT_STATUSTYPE_ocsp
        )
        _openssl_assert(rc == 1)


# This is similar to the initialization calls at the end of OpenSSL/crypto.py
# but is exercised mostly by the Context initializer.
_lib.SSL_library_init()
