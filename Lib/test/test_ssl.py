# Test the support for SSL and sockets

import sys
import unittest
import unittest.mock
from ast import literal_eval
from threading import Thread
from test import support
from test.support import import_helper
from test.support import os_helper
from test.support import socket_helper
from test.support import threading_helper
from test.support import warnings_helper
from test.support import asyncore
import array
import re
import socket
import select
import struct
import time
import enum
import gc
import http.client
import os
import errno
import pprint
import urllib.request
import threading
import traceback
import weakref
import platform
import sysconfig
import functools
from contextlib import nullcontext
try:
    import ctypes
except ImportError:
    ctypes = None


ssl = import_helper.import_module("ssl")
import _ssl

from ssl import Purpose, TLSVersion, _TLSContentType, _TLSMessageType, _TLSAlertType

Py_DEBUG_WIN32 = support.Py_DEBUG and sys.platform == 'win32'

PROTOCOLS = sorted(ssl._PROTOCOL_NAMES)
HOST = socket_helper.HOST
IS_OPENSSL_3_0_0 = ssl.OPENSSL_VERSION_INFO >= (3, 0, 0)
PY_SSL_DEFAULT_CIPHERS = sysconfig.get_config_var('PY_SSL_DEFAULT_CIPHERS')

PROTOCOL_TO_TLS_VERSION = {}
for proto, ver in (
    ("PROTOCOL_SSLv3", "SSLv3"),
    ("PROTOCOL_TLSv1", "TLSv1"),
    ("PROTOCOL_TLSv1_1", "TLSv1_1"),
):
    try:
        proto = getattr(ssl, proto)
        ver = getattr(ssl.TLSVersion, ver)
    except AttributeError:
        continue
    PROTOCOL_TO_TLS_VERSION[proto] = ver

def data_file(*name):
    return os.path.join(os.path.dirname(__file__), "certdata", *name)

# The custom key and certificate files used in test_ssl are generated
# using Lib/test/certdata/make_ssl_certs.py.
# Other certificates are simply fetched from the internet servers they
# are meant to authenticate.

CERTFILE = data_file("keycert.pem")
BYTES_CERTFILE = os.fsencode(CERTFILE)
ONLYCERT = data_file("ssl_cert.pem")
ONLYKEY = data_file("ssl_key.pem")
BYTES_ONLYCERT = os.fsencode(ONLYCERT)
BYTES_ONLYKEY = os.fsencode(ONLYKEY)
CERTFILE_PROTECTED = data_file("keycert.passwd.pem")
ONLYKEY_PROTECTED = data_file("ssl_key.passwd.pem")
KEY_PASSWORD = "somepass"
CAPATH = data_file("capath")
BYTES_CAPATH = os.fsencode(CAPATH)
CAFILE_NEURONIO = data_file("capath", "4e1295a3.0")
CAFILE_CACERT = data_file("capath", "5ed36f99.0")

with open(data_file('keycert.pem.reference')) as file:
    CERTFILE_INFO = literal_eval(file.read())

# empty CRL
CRLFILE = data_file("revocation.crl")

# Two keys and certs signed by the same CA (for SNI tests)
SIGNED_CERTFILE = data_file("keycert3.pem")
SINGED_CERTFILE_ONLY = data_file("cert3.pem")
SIGNED_CERTFILE_HOSTNAME = 'localhost'

with open(data_file('keycert3.pem.reference')) as file:
    SIGNED_CERTFILE_INFO = literal_eval(file.read())

SIGNED_CERTFILE2 = data_file("keycert4.pem")
SIGNED_CERTFILE2_HOSTNAME = 'fakehostname'
SIGNED_CERTFILE_ECC = data_file("keycertecc.pem")
SIGNED_CERTFILE_ECC_HOSTNAME = 'localhost-ecc'

# A custom testcase, extracted from `rfc5280::aki::leaf-missing-aki` in x509-limbo:
# The leaf (server) certificate has no AKI, which is forbidden under RFC 5280.
# See: https://x509-limbo.com/testcases/rfc5280/#rfc5280akileaf-missing-aki
LEAF_MISSING_AKI_CERTFILE = data_file("leaf-missing-aki.keycert.pem")
LEAF_MISSING_AKI_CERTFILE_HOSTNAME = "example.com"
LEAF_MISSING_AKI_CA = data_file("leaf-missing-aki.ca.pem")

# Same certificate as pycacert.pem, but without extra text in file
SIGNING_CA = data_file("capath", "ceff1710.0")
# cert with all kinds of subject alt names
ALLSANFILE = data_file("allsans.pem")
IDNSANSFILE = data_file("idnsans.pem")
NOSANFILE = data_file("nosan.pem")
NOSAN_HOSTNAME = 'localhost'

REMOTE_HOST = "self-signed.pythontest.net"

EMPTYCERT = data_file("nullcert.pem")
BADCERT = data_file("badcert.pem")
NONEXISTINGCERT = data_file("XXXnonexisting.pem")
BADKEY = data_file("badkey.pem")
NOKIACERT = data_file("nokia.pem")
NULLBYTECERT = data_file("nullbytecert.pem")
TALOS_INVALID_CRLDP = data_file("talos-2019-0758.pem")

DHFILE = data_file("ffdh3072.pem")
BYTES_DHFILE = os.fsencode(DHFILE)

# Not defined in all versions of OpenSSL
OP_NO_COMPRESSION = getattr(ssl, "OP_NO_COMPRESSION", 0)
OP_SINGLE_DH_USE = getattr(ssl, "OP_SINGLE_DH_USE", 0)
OP_SINGLE_ECDH_USE = getattr(ssl, "OP_SINGLE_ECDH_USE", 0)
OP_CIPHER_SERVER_PREFERENCE = getattr(ssl, "OP_CIPHER_SERVER_PREFERENCE", 0)
OP_ENABLE_MIDDLEBOX_COMPAT = getattr(ssl, "OP_ENABLE_MIDDLEBOX_COMPAT", 0)

# Ubuntu has patched OpenSSL and changed behavior of security level 2
# see https://bugs.python.org/issue41561#msg389003
def is_ubuntu():
    try:
        # Assume that any references of "ubuntu" implies Ubuntu-like distro
        # The workaround is not required for 18.04, but doesn't hurt either.
        with open("/etc/os-release", encoding="utf-8") as f:
            return "ubuntu" in f.read()
    except FileNotFoundError:
        return False

if is_ubuntu():
    def seclevel_workaround(*ctxs):
        """Lower security level to '1' and allow all ciphers for TLS 1.0/1"""
        for ctx in ctxs:
            if (
                hasattr(ctx, "minimum_version") and
                ctx.minimum_version <= ssl.TLSVersion.TLSv1_1 and
                ctx.security_level > 1
            ):
                ctx.set_ciphers("@SECLEVEL=1:ALL")
else:
    def seclevel_workaround(*ctxs):
        pass


def has_tls_protocol(protocol):
    """Check if a TLS protocol is available and enabled

    :param protocol: enum ssl._SSLMethod member or name
    :return: bool
    """
    if isinstance(protocol, str):
        assert protocol.startswith('PROTOCOL_')
        protocol = getattr(ssl, protocol, None)
        if protocol is None:
            return False
    if protocol in {
        ssl.PROTOCOL_TLS, ssl.PROTOCOL_TLS_SERVER,
        ssl.PROTOCOL_TLS_CLIENT
    }:
        # auto-negotiate protocols are always available
        return True
    name = protocol.name
    return has_tls_version(name[len('PROTOCOL_'):])


@functools.lru_cache
def has_tls_version(version):
    """Check if a TLS/SSL version is enabled

    :param version: TLS version name or ssl.TLSVersion member
    :return: bool
    """
    if isinstance(version, str):
        version = ssl.TLSVersion.__members__[version]

    # check compile time flags like ssl.HAS_TLSv1_2
    if not getattr(ssl, f'HAS_{version.name}'):
        return False

    if IS_OPENSSL_3_0_0 and version < ssl.TLSVersion.TLSv1_2:
        # bpo43791: 3.0.0-alpha14 fails with TLSV1_ALERT_INTERNAL_ERROR
        return False

    # check runtime and dynamic crypto policy settings. A TLS version may
    # be compiled in but disabled by a policy or config option.
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    if (
            hasattr(ctx, 'minimum_version') and
            ctx.minimum_version != ssl.TLSVersion.MINIMUM_SUPPORTED and
            version < ctx.minimum_version
    ):
        return False
    if (
        hasattr(ctx, 'maximum_version') and
        ctx.maximum_version != ssl.TLSVersion.MAXIMUM_SUPPORTED and
        version > ctx.maximum_version
    ):
        return False

    return True


def requires_tls_version(version):
    """Decorator to skip tests when a required TLS version is not available

    :param version: TLS version name or ssl.TLSVersion member
    :return:
    """
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kw):
            if not has_tls_version(version):
                raise unittest.SkipTest(f"{version} is not available.")
            else:
                return func(*args, **kw)
        return wrapper
    return decorator


def handle_error(prefix):
    exc_format = ' '.join(traceback.format_exception(sys.exception()))
    if support.verbose:
        sys.stdout.write(prefix + exc_format)


def utc_offset(): #NOTE: ignore issues like #1647654
    # local time = utc time + utc offset
    if time.daylight and time.localtime().tm_isdst > 0:
        return -time.altzone  # seconds
    return -time.timezone


ignore_deprecation = warnings_helper.ignore_warnings(
    category=DeprecationWarning
)


def test_wrap_socket(sock, *,
                     cert_reqs=ssl.CERT_NONE, ca_certs=None,
                     ciphers=None, certfile=None, keyfile=None,
                     **kwargs):
    if not kwargs.get("server_side"):
        kwargs["server_hostname"] = SIGNED_CERTFILE_HOSTNAME
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    else:
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    if cert_reqs is not None:
        if cert_reqs == ssl.CERT_NONE:
            context.check_hostname = False
        context.verify_mode = cert_reqs
    if ca_certs is not None:
        context.load_verify_locations(ca_certs)
    if certfile is not None or keyfile is not None:
        context.load_cert_chain(certfile, keyfile)
    if ciphers is not None:
        context.set_ciphers(ciphers)
    return context.wrap_socket(sock, **kwargs)


USE_SAME_TEST_CONTEXT = False
_TEST_CONTEXT = None

def testing_context(server_cert=SIGNED_CERTFILE, *, server_chain=True):
    """Create context

    client_context, server_context, hostname = testing_context()
    """
    global _TEST_CONTEXT
    if USE_SAME_TEST_CONTEXT:
        if _TEST_CONTEXT is not None:
            return _TEST_CONTEXT

    if server_cert == SIGNED_CERTFILE:
        hostname = SIGNED_CERTFILE_HOSTNAME
    elif server_cert == SIGNED_CERTFILE2:
        hostname = SIGNED_CERTFILE2_HOSTNAME
    elif server_cert == NOSANFILE:
        hostname = NOSAN_HOSTNAME
    else:
        raise ValueError(server_cert)

    client_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    client_context.load_verify_locations(SIGNING_CA)

    server_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    server_context.load_cert_chain(server_cert)
    if server_chain:
        server_context.load_verify_locations(SIGNING_CA)

    if USE_SAME_TEST_CONTEXT:
        if _TEST_CONTEXT is not None:
            _TEST_CONTEXT = client_context, server_context, hostname

    return client_context, server_context, hostname


class BasicSocketTests(unittest.TestCase):

    def test_constants(self):
        ssl.CERT_NONE
        ssl.CERT_OPTIONAL
        ssl.CERT_REQUIRED
        ssl.OP_CIPHER_SERVER_PREFERENCE
        ssl.OP_SINGLE_DH_USE
        ssl.OP_SINGLE_ECDH_USE
        ssl.OP_NO_COMPRESSION
        self.assertEqual(ssl.HAS_SNI, True)
        self.assertEqual(ssl.HAS_ECDH, True)
        self.assertEqual(ssl.HAS_TLSv1_2, True)
        self.assertEqual(ssl.HAS_TLSv1_3, True)
        ssl.OP_NO_SSLv2
        ssl.OP_NO_SSLv3
        ssl.OP_NO_TLSv1
        ssl.OP_NO_TLSv1_3
        ssl.OP_NO_TLSv1_1
        ssl.OP_NO_TLSv1_2
        self.assertEqual(ssl.PROTOCOL_TLS, ssl.PROTOCOL_SSLv23)

    def test_options(self):
        # gh-106687: SSL options values are unsigned integer (uint64_t)
        for name in dir(ssl):
            if not name.startswith('OP_'):
                continue
            with self.subTest(option=name):
                value = getattr(ssl, name)
                self.assertGreaterEqual(value, 0, f"ssl.{name}")

    def test_ssl_types(self):
        ssl_types = [
            _ssl._SSLContext,
            _ssl._SSLSocket,
            _ssl.MemoryBIO,
            _ssl.Certificate,
            _ssl.SSLSession,
            _ssl.SSLError,
        ]
        for ssl_type in ssl_types:
            with self.subTest(ssl_type=ssl_type):
                with self.assertRaisesRegex(TypeError, "immutable type"):
                    ssl_type.value = None
        support.check_disallow_instantiation(self, _ssl.Certificate)

    def test_private_init(self):
        with self.assertRaisesRegex(TypeError, "public constructor"):
            with socket.socket() as s:
                ssl.SSLSocket(s)

    def test_str_for_enums(self):
        # Make sure that the PROTOCOL_* constants have enum-like string
        # reprs.
        proto = ssl.PROTOCOL_TLS_CLIENT
        self.assertEqual(repr(proto), '<_SSLMethod.PROTOCOL_TLS_CLIENT: %r>' % proto.value)
        self.assertEqual(str(proto), str(proto.value))
        ctx = ssl.SSLContext(proto)
        self.assertIs(ctx.protocol, proto)

    def test_random(self):
        v = ssl.RAND_status()
        if support.verbose:
            sys.stdout.write("\n RAND_status is %d (%s)\n"
                             % (v, (v and "sufficient randomness") or
                                "insufficient randomness"))

        if v:
            data = ssl.RAND_bytes(16)
            self.assertEqual(len(data), 16)
        else:
            self.assertRaises(ssl.SSLError, ssl.RAND_bytes, 16)

        # negative num is invalid
        self.assertRaises(ValueError, ssl.RAND_bytes, -5)

        ssl.RAND_add("this is a random string", 75.0)
        ssl.RAND_add(b"this is a random bytes object", 75.0)
        ssl.RAND_add(bytearray(b"this is a random bytearray object"), 75.0)

    def test_parse_cert(self):
        self.maxDiff = None
        # note that this uses an 'unofficial' function in _ssl.c,
        # provided solely for this test, to exercise the certificate
        # parsing code
        self.assertEqual(
            ssl._ssl._test_decode_cert(CERTFILE),
            CERTFILE_INFO
        )
        self.assertEqual(
            ssl._ssl._test_decode_cert(SIGNED_CERTFILE),
            SIGNED_CERTFILE_INFO
        )

        # Issue #13034: the subjectAltName in some certificates
        # (notably projects.developer.nokia.com:443) wasn't parsed
        p = ssl._ssl._test_decode_cert(NOKIACERT)
        if support.verbose:
            sys.stdout.write("\n" + pprint.pformat(p) + "\n")
        self.assertEqual(p['subjectAltName'],
                         (('DNS', 'projects.developer.nokia.com'),
                          ('DNS', 'projects.forum.nokia.com'))
                        )
        # extra OCSP and AIA fields
        self.assertEqual(p['OCSP'], ('http://ocsp.verisign.com',))
        self.assertEqual(p['caIssuers'],
                         ('http://SVRIntl-G3-aia.verisign.com/SVRIntlG3.cer',))
        self.assertEqual(p['crlDistributionPoints'],
                         ('http://SVRIntl-G3-crl.verisign.com/SVRIntlG3.crl',))

    def test_parse_cert_CVE_2019_5010(self):
        p = ssl._ssl._test_decode_cert(TALOS_INVALID_CRLDP)
        if support.verbose:
            sys.stdout.write("\n" + pprint.pformat(p) + "\n")
        self.assertEqual(
            p,
            {
                'issuer': (
                    (('countryName', 'UK'),), (('commonName', 'cody-ca'),)),
                'notAfter': 'Jun 14 18:00:58 2028 GMT',
                'notBefore': 'Jun 18 18:00:58 2018 GMT',
                'serialNumber': '02',
                'subject': ((('countryName', 'UK'),),
                            (('commonName',
                              'codenomicon-vm-2.test.lal.cisco.com'),)),
                'subjectAltName': (
                    ('DNS', 'codenomicon-vm-2.test.lal.cisco.com'),),
                'version': 3
            }
        )

    def test_parse_cert_CVE_2013_4238(self):
        p = ssl._ssl._test_decode_cert(NULLBYTECERT)
        if support.verbose:
            sys.stdout.write("\n" + pprint.pformat(p) + "\n")
        subject = ((('countryName', 'US'),),
                   (('stateOrProvinceName', 'Oregon'),),
                   (('localityName', 'Beaverton'),),
                   (('organizationName', 'Python Software Foundation'),),
                   (('organizationalUnitName', 'Python Core Development'),),
                   (('commonName', 'null.python.org\x00example.org'),),
                   (('emailAddress', 'python-dev@python.org'),))
        self.assertEqual(p['subject'], subject)
        self.assertEqual(p['issuer'], subject)
        if ssl._OPENSSL_API_VERSION >= (0, 9, 8):
            san = (('DNS', 'altnull.python.org\x00example.com'),
                   ('email', 'null@python.org\x00user@example.org'),
                   ('URI', 'http://null.python.org\x00http://example.org'),
                   ('IP Address', '192.0.2.1'),
                   ('IP Address', '2001:DB8:0:0:0:0:0:1'))
        else:
            # OpenSSL 0.9.7 doesn't support IPv6 addresses in subjectAltName
            san = (('DNS', 'altnull.python.org\x00example.com'),
                   ('email', 'null@python.org\x00user@example.org'),
                   ('URI', 'http://null.python.org\x00http://example.org'),
                   ('IP Address', '192.0.2.1'),
                   ('IP Address', '<invalid>'))

        self.assertEqual(p['subjectAltName'], san)

    def test_parse_all_sans(self):
        p = ssl._ssl._test_decode_cert(ALLSANFILE)
        self.assertEqual(p['subjectAltName'],
            (
                ('DNS', 'allsans'),
                ('othername', '<unsupported>'),
                ('othername', '<unsupported>'),
                ('email', 'user@example.org'),
                ('DNS', 'www.example.org'),
                ('DirName',
                    ((('countryName', 'XY'),),
                    (('localityName', 'Castle Anthrax'),),
                    (('organizationName', 'Python Software Foundation'),),
                    (('commonName', 'dirname example'),))),
                ('URI', 'https://www.python.org/'),
                ('IP Address', '127.0.0.1'),
                ('IP Address', '0:0:0:0:0:0:0:1'),
                ('Registered ID', '1.2.3.4.5')
            )
        )

    def test_DER_to_PEM(self):
        with open(CAFILE_CACERT, 'r') as f:
            pem = f.read()
        d1 = ssl.PEM_cert_to_DER_cert(pem)
        p2 = ssl.DER_cert_to_PEM_cert(d1)
        d2 = ssl.PEM_cert_to_DER_cert(p2)
        self.assertEqual(d1, d2)
        if not p2.startswith(ssl.PEM_HEADER + '\n'):
            self.fail("DER-to-PEM didn't include correct header:\n%r\n" % p2)
        if not p2.endswith('\n' + ssl.PEM_FOOTER + '\n'):
            self.fail("DER-to-PEM didn't include correct footer:\n%r\n" % p2)

    def test_openssl_version(self):
        n = ssl.OPENSSL_VERSION_NUMBER
        t = ssl.OPENSSL_VERSION_INFO
        s = ssl.OPENSSL_VERSION
        self.assertIsInstance(n, int)
        self.assertIsInstance(t, tuple)
        self.assertIsInstance(s, str)
        # Some sanity checks follow
        # >= 1.1.1
        self.assertGreaterEqual(n, 0x10101000)
        # < 4.0
        self.assertLess(n, 0x40000000)
        major, minor, fix, patch, status = t
        self.assertGreaterEqual(major, 1)
        self.assertLess(major, 4)
        self.assertGreaterEqual(minor, 0)
        self.assertLess(minor, 256)
        self.assertGreaterEqual(fix, 0)
        self.assertLess(fix, 256)
        self.assertGreaterEqual(patch, 0)
        self.assertLessEqual(patch, 63)
        self.assertGreaterEqual(status, 0)
        self.assertLessEqual(status, 15)

        libressl_ver = f"LibreSSL {major:d}"
        if major >= 3:
            # 3.x uses 0xMNN00PP0L
            openssl_ver = f"OpenSSL {major:d}.{minor:d}.{patch:d}"
        else:
            openssl_ver = f"OpenSSL {major:d}.{minor:d}.{fix:d}"
        self.assertStartsWith(
            s, (openssl_ver, libressl_ver, "AWS-LC"),
            (t, hex(n))
        )

    @support.cpython_only
    def test_refcycle(self):
        # Issue #7943: an SSL object doesn't create reference cycles with
        # itself.
        s = socket.socket(socket.AF_INET)
        ss = test_wrap_socket(s)
        wr = weakref.ref(ss)
        with warnings_helper.check_warnings(("", ResourceWarning)):
            del ss
        self.assertEqual(wr(), None)

    def test_wrapped_unconnected(self):
        # Methods on an unconnected SSLSocket propagate the original
        # OSError raise by the underlying socket object.
        s = socket.socket(socket.AF_INET)
        with test_wrap_socket(s) as ss:
            self.assertRaises(OSError, ss.recv, 1)
            self.assertRaises(OSError, ss.recv_into, bytearray(b'x'))
            self.assertRaises(OSError, ss.recvfrom, 1)
            self.assertRaises(OSError, ss.recvfrom_into, bytearray(b'x'), 1)
            self.assertRaises(OSError, ss.send, b'x')
            self.assertRaises(OSError, ss.sendto, b'x', ('0.0.0.0', 0))
            self.assertRaises(NotImplementedError, ss.dup)
            self.assertRaises(NotImplementedError, ss.sendmsg,
                              [b'x'], (), 0, ('0.0.0.0', 0))
            self.assertRaises(NotImplementedError, ss.recvmsg, 100)
            self.assertRaises(NotImplementedError, ss.recvmsg_into,
                              [bytearray(100)])

    def test_timeout(self):
        # Issue #8524: when creating an SSL socket, the timeout of the
        # original socket should be retained.
        for timeout in (None, 0.0, 5.0):
            s = socket.socket(socket.AF_INET)
            s.settimeout(timeout)
            with test_wrap_socket(s) as ss:
                self.assertEqual(timeout, ss.gettimeout())

    def test_openssl111_deprecations(self):
        options = [
            ssl.OP_NO_TLSv1,
            ssl.OP_NO_TLSv1_1,
            ssl.OP_NO_TLSv1_2,
            ssl.OP_NO_TLSv1_3
        ]
        protocols = [
            ssl.PROTOCOL_TLSv1,
            ssl.PROTOCOL_TLSv1_1,
            ssl.PROTOCOL_TLSv1_2,
            ssl.PROTOCOL_TLS
        ]
        versions = [
            ssl.TLSVersion.SSLv3,
            ssl.TLSVersion.TLSv1,
            ssl.TLSVersion.TLSv1_1,
        ]

        for option in options:
            with self.subTest(option=option):
                ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
                with self.assertWarns(DeprecationWarning) as cm:
                    ctx.options |= option
                self.assertEqual(
                    'ssl.OP_NO_SSL*/ssl.OP_NO_TLS* options are deprecated',
                    str(cm.warning)
                )

        for protocol in protocols:
            if not has_tls_protocol(protocol):
                continue
            with self.subTest(protocol=protocol):
                with self.assertWarns(DeprecationWarning) as cm:
                    ssl.SSLContext(protocol)
                self.assertEqual(
                    f'ssl.{protocol.name} is deprecated',
                    str(cm.warning)
                )

        for version in versions:
            if not has_tls_version(version):
                continue
            with self.subTest(version=version):
                ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
                with self.assertWarns(DeprecationWarning) as cm:
                    ctx.minimum_version = version
                version_text = '%s.%s' % (version.__class__.__name__, version.name)
                self.assertEqual(
                    f'ssl.{version_text} is deprecated',
                    str(cm.warning)
                )

    def bad_cert_test(self, certfile):
        """Check that trying to use the given client certificate fails"""
        certfile = os.path.join(os.path.dirname(__file__) or os.curdir,
                                "certdata", certfile)
        sock = socket.socket()
        self.addCleanup(sock.close)
        with self.assertRaises(ssl.SSLError):
            test_wrap_socket(sock,
                             certfile=certfile)

    def test_empty_cert(self):
        """Wrapping with an empty cert file"""
        self.bad_cert_test("nullcert.pem")

    def test_malformed_cert(self):
        """Wrapping with a badly formatted certificate (syntax error)"""
        self.bad_cert_test("badcert.pem")

    def test_malformed_key(self):
        """Wrapping with a badly formatted key (syntax error)"""
        self.bad_cert_test("badkey.pem")

    def test_server_side(self):
        # server_hostname doesn't work for server sockets
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        with socket.socket() as sock:
            self.assertRaises(ValueError, ctx.wrap_socket, sock, True,
                              server_hostname="some.hostname")

    def test_unknown_channel_binding(self):
        # should raise ValueError for unknown type
        s = socket.create_server(('127.0.0.1', 0))
        c = socket.socket(socket.AF_INET)
        c.connect(s.getsockname())
        with test_wrap_socket(c, do_handshake_on_connect=False) as ss:
            with self.assertRaises(ValueError):
                ss.get_channel_binding("unknown-type")
        s.close()

    @unittest.skipUnless("tls-unique" in ssl.CHANNEL_BINDING_TYPES,
                         "'tls-unique' channel binding not available")
    def test_tls_unique_channel_binding(self):
        # unconnected should return None for known type
        s = socket.socket(socket.AF_INET)
        with test_wrap_socket(s) as ss:
            self.assertIsNone(ss.get_channel_binding("tls-unique"))
        # the same for server-side
        s = socket.socket(socket.AF_INET)
        with test_wrap_socket(s, server_side=True, certfile=CERTFILE) as ss:
            self.assertIsNone(ss.get_channel_binding("tls-unique"))

    def test_dealloc_warn(self):
        ss = test_wrap_socket(socket.socket(socket.AF_INET))
        r = repr(ss)
        with self.assertWarns(ResourceWarning) as cm:
            ss = None
            support.gc_collect()
        self.assertIn(r, str(cm.warning.args[0]))

    def test_get_default_verify_paths(self):
        paths = ssl.get_default_verify_paths()
        self.assertEqual(len(paths), 6)
        self.assertIsInstance(paths, ssl.DefaultVerifyPaths)

        with os_helper.EnvironmentVarGuard() as env:
            env["SSL_CERT_DIR"] = CAPATH
            env["SSL_CERT_FILE"] = CERTFILE
            paths = ssl.get_default_verify_paths()
            self.assertEqual(paths.cafile, CERTFILE)
            self.assertEqual(paths.capath, CAPATH)

    @unittest.skipUnless(sys.platform == "win32", "Windows specific")
    def test_enum_certificates(self):
        self.assertTrue(ssl.enum_certificates("CA"))
        self.assertTrue(ssl.enum_certificates("ROOT"))

        self.assertRaises(TypeError, ssl.enum_certificates)
        self.assertRaises(WindowsError, ssl.enum_certificates, "")

        trust_oids = set()
        for storename in ("CA", "ROOT"):
            store = ssl.enum_certificates(storename)
            self.assertIsInstance(store, list)
            for element in store:
                self.assertIsInstance(element, tuple)
                self.assertEqual(len(element), 3)
                cert, enc, trust = element
                self.assertIsInstance(cert, bytes)
                self.assertIn(enc, {"x509_asn", "pkcs_7_asn"})
                self.assertIsInstance(trust, (frozenset, set, bool))
                if isinstance(trust, (frozenset, set)):
                    trust_oids.update(trust)

        serverAuth = "1.3.6.1.5.5.7.3.1"
        self.assertIn(serverAuth, trust_oids)

    @unittest.skipUnless(sys.platform == "win32", "Windows specific")
    def test_enum_crls(self):
        self.assertTrue(ssl.enum_crls("CA"))
        self.assertRaises(TypeError, ssl.enum_crls)
        self.assertRaises(WindowsError, ssl.enum_crls, "")

        crls = ssl.enum_crls("CA")
        self.assertIsInstance(crls, list)
        for element in crls:
            self.assertIsInstance(element, tuple)
            self.assertEqual(len(element), 2)
            self.assertIsInstance(element[0], bytes)
            self.assertIn(element[1], {"x509_asn", "pkcs_7_asn"})


    def test_asn1object(self):
        expected = (129, 'serverAuth', 'TLS Web Server Authentication',
                    '1.3.6.1.5.5.7.3.1')

        val = ssl._ASN1Object('1.3.6.1.5.5.7.3.1')
        self.assertEqual(val, expected)
        self.assertEqual(val.nid, 129)
        self.assertEqual(val.shortname, 'serverAuth')
        self.assertEqual(val.longname, 'TLS Web Server Authentication')
        self.assertEqual(val.oid, '1.3.6.1.5.5.7.3.1')
        self.assertIsInstance(val, ssl._ASN1Object)
        self.assertRaises(ValueError, ssl._ASN1Object, 'serverAuth')

        val = ssl._ASN1Object.fromnid(129)
        self.assertEqual(val, expected)
        self.assertIsInstance(val, ssl._ASN1Object)
        self.assertRaises(ValueError, ssl._ASN1Object.fromnid, -1)
        with self.assertRaisesRegex(ValueError, "unknown NID 100000"):
            ssl._ASN1Object.fromnid(100000)
        for i in range(1000):
            try:
                obj = ssl._ASN1Object.fromnid(i)
            except ValueError:
                pass
            else:
                self.assertIsInstance(obj.nid, int)
                self.assertIsInstance(obj.shortname, str)
                self.assertIsInstance(obj.longname, str)
                self.assertIsInstance(obj.oid, (str, type(None)))

        val = ssl._ASN1Object.fromname('TLS Web Server Authentication')
        self.assertEqual(val, expected)
        self.assertIsInstance(val, ssl._ASN1Object)
        self.assertEqual(ssl._ASN1Object.fromname('serverAuth'), expected)
        self.assertEqual(ssl._ASN1Object.fromname('1.3.6.1.5.5.7.3.1'),
                         expected)
        with self.assertRaisesRegex(ValueError, "unknown object 'serverauth'"):
            ssl._ASN1Object.fromname('serverauth')

    def test_purpose_enum(self):
        val = ssl._ASN1Object('1.3.6.1.5.5.7.3.1')
        self.assertIsInstance(ssl.Purpose.SERVER_AUTH, ssl._ASN1Object)
        self.assertEqual(ssl.Purpose.SERVER_AUTH, val)
        self.assertEqual(ssl.Purpose.SERVER_AUTH.nid, 129)
        self.assertEqual(ssl.Purpose.SERVER_AUTH.shortname, 'serverAuth')
        self.assertEqual(ssl.Purpose.SERVER_AUTH.oid,
                              '1.3.6.1.5.5.7.3.1')

        val = ssl._ASN1Object('1.3.6.1.5.5.7.3.2')
        self.assertIsInstance(ssl.Purpose.CLIENT_AUTH, ssl._ASN1Object)
        self.assertEqual(ssl.Purpose.CLIENT_AUTH, val)
        self.assertEqual(ssl.Purpose.CLIENT_AUTH.nid, 130)
        self.assertEqual(ssl.Purpose.CLIENT_AUTH.shortname, 'clientAuth')
        self.assertEqual(ssl.Purpose.CLIENT_AUTH.oid,
                              '1.3.6.1.5.5.7.3.2')

    def test_unsupported_dtls(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.addCleanup(s.close)
        with self.assertRaises(NotImplementedError) as cx:
            test_wrap_socket(s, cert_reqs=ssl.CERT_NONE)
        self.assertEqual(str(cx.exception), "only stream sockets are supported")
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        with self.assertRaises(NotImplementedError) as cx:
            ctx.wrap_socket(s)
        self.assertEqual(str(cx.exception), "only stream sockets are supported")

    def cert_time_ok(self, timestring, timestamp):
        self.assertEqual(ssl.cert_time_to_seconds(timestring), timestamp)

    def cert_time_fail(self, timestring):
        with self.assertRaises(ValueError):
            ssl.cert_time_to_seconds(timestring)

    @unittest.skipUnless(utc_offset(),
                         'local time needs to be different from UTC')
    def test_cert_time_to_seconds_timezone(self):
        # Issue #19940: ssl.cert_time_to_seconds() returns wrong
        #               results if local timezone is not UTC
        self.cert_time_ok("May  9 00:00:00 2007 GMT", 1178668800.0)
        self.cert_time_ok("Jan  5 09:34:43 2018 GMT", 1515144883.0)

    def test_cert_time_to_seconds(self):
        timestring = "Jan  5 09:34:43 2018 GMT"
        ts = 1515144883.0
        self.cert_time_ok(timestring, ts)
        # accept keyword parameter, assert its name
        self.assertEqual(ssl.cert_time_to_seconds(cert_time=timestring), ts)
        # accept both %e and %d (space or zero generated by strftime)
        self.cert_time_ok("Jan 05 09:34:43 2018 GMT", ts)
        # case-insensitive
        self.cert_time_ok("JaN  5 09:34:43 2018 GmT", ts)
        self.cert_time_fail("Jan  5 09:34 2018 GMT")     # no seconds
        self.cert_time_fail("Jan  5 09:34:43 2018")      # no GMT
        self.cert_time_fail("Jan  5 09:34:43 2018 UTC")  # not GMT timezone
        self.cert_time_fail("Jan 35 09:34:43 2018 GMT")  # invalid day
        self.cert_time_fail("Jon  5 09:34:43 2018 GMT")  # invalid month
        self.cert_time_fail("Jan  5 24:00:00 2018 GMT")  # invalid hour
        self.cert_time_fail("Jan  5 09:60:43 2018 GMT")  # invalid minute

        newyear_ts = 1230768000.0
        # leap seconds
        self.cert_time_ok("Dec 31 23:59:60 2008 GMT", newyear_ts)
        # same timestamp
        self.cert_time_ok("Jan  1 00:00:00 2009 GMT", newyear_ts)

        self.cert_time_ok("Jan  5 09:34:59 2018 GMT", 1515144899)
        #  allow 60th second (even if it is not a leap second)
        self.cert_time_ok("Jan  5 09:34:60 2018 GMT", 1515144900)
        #  allow 2nd leap second for compatibility with time.strptime()
        self.cert_time_ok("Jan  5 09:34:61 2018 GMT", 1515144901)
        self.cert_time_fail("Jan  5 09:34:62 2018 GMT")  # invalid seconds

        # no special treatment for the special value:
        #   99991231235959Z (rfc 5280)
        self.cert_time_ok("Dec 31 23:59:59 9999 GMT", 253402300799.0)

    @support.run_with_locale('LC_ALL', '')
    def test_cert_time_to_seconds_locale(self):
        # `cert_time_to_seconds()` should be locale independent

        def local_february_name():
            return time.strftime('%b', (1, 2, 3, 4, 5, 6, 0, 0, 0))

        if local_february_name().lower() == 'feb':
            self.skipTest("locale-specific month name needs to be "
                          "different from C locale")

        # locale-independent
        self.cert_time_ok("Feb  9 00:00:00 2007 GMT", 1170979200.0)
        self.cert_time_fail(local_february_name() + "  9 00:00:00 2007 GMT")

    def test_connect_ex_error(self):
        server = socket.socket(socket.AF_INET)
        self.addCleanup(server.close)
        port = socket_helper.bind_port(server)  # Reserve port but don't listen
        s = test_wrap_socket(socket.socket(socket.AF_INET),
                            cert_reqs=ssl.CERT_REQUIRED)
        self.addCleanup(s.close)
        rc = s.connect_ex((HOST, port))
        # Issue #19919: Windows machines or VMs hosted on Windows
        # machines sometimes return EWOULDBLOCK.
        errors = (
            errno.ECONNREFUSED, errno.EHOSTUNREACH, errno.ETIMEDOUT,
            errno.EWOULDBLOCK,
        )
        self.assertIn(rc, errors)

    def test_read_write_zero(self):
        # empty reads and writes now work, bpo-42854, bpo-31711
        client_context, server_context, hostname = testing_context()
        server = ThreadedEchoServer(context=server_context)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                self.assertEqual(s.recv(0), b"")
                self.assertEqual(s.send(b""), 0)


class ContextTests(unittest.TestCase):

    def test_constructor(self):
        for protocol in PROTOCOLS:
            if has_tls_protocol(protocol):
                with warnings_helper.check_warnings():
                    ctx = ssl.SSLContext(protocol)
                self.assertEqual(ctx.protocol, protocol)
        with warnings_helper.check_warnings():
            ctx = ssl.SSLContext()
        self.assertEqual(ctx.protocol, ssl.PROTOCOL_TLS)
        self.assertRaises(ValueError, ssl.SSLContext, -1)
        self.assertRaises(ValueError, ssl.SSLContext, 42)

    def test_ciphers(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.set_ciphers("ALL")
        ctx.set_ciphers("DEFAULT")
        with self.assertRaisesRegex(ssl.SSLError, "No cipher can be selected"):
            ctx.set_ciphers("^$:,;?*'dorothyx")

    @unittest.skipUnless(PY_SSL_DEFAULT_CIPHERS == 1,
                         "Test applies only to Python default ciphers")
    def test_python_ciphers(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ciphers = ctx.get_ciphers()
        for suite in ciphers:
            name = suite['name']
            self.assertNotIn("PSK", name)
            self.assertNotIn("SRP", name)
            self.assertNotIn("MD5", name)
            self.assertNotIn("RC4", name)
            self.assertNotIn("3DES", name)

    def test_get_ciphers(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.set_ciphers('AESGCM')
        names = set(d['name'] for d in ctx.get_ciphers())
        expected = {
            'AES128-GCM-SHA256',
            'ECDHE-ECDSA-AES128-GCM-SHA256',
            'ECDHE-RSA-AES128-GCM-SHA256',
            'DHE-RSA-AES128-GCM-SHA256',
            'AES256-GCM-SHA384',
            'ECDHE-ECDSA-AES256-GCM-SHA384',
            'ECDHE-RSA-AES256-GCM-SHA384',
            'DHE-RSA-AES256-GCM-SHA384',
        }
        intersection = names.intersection(expected)
        self.assertGreaterEqual(
            len(intersection), 2, f"\ngot: {sorted(names)}\nexpected: {sorted(expected)}"
        )

    def test_options(self):
        # Test default SSLContext options
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        # OP_ALL | OP_NO_SSLv2 | OP_NO_SSLv3 is the default value
        default = (ssl.OP_ALL | ssl.OP_NO_SSLv2 | ssl.OP_NO_SSLv3)
        # SSLContext also enables these by default
        default |= (OP_NO_COMPRESSION | OP_CIPHER_SERVER_PREFERENCE |
                    OP_SINGLE_DH_USE | OP_SINGLE_ECDH_USE |
                    OP_ENABLE_MIDDLEBOX_COMPAT)
        self.assertEqual(default, ctx.options)

        # disallow TLSv1
        with warnings_helper.check_warnings():
            ctx.options |= ssl.OP_NO_TLSv1
        self.assertEqual(default | ssl.OP_NO_TLSv1, ctx.options)

        # allow TLSv1
        with warnings_helper.check_warnings():
            ctx.options = (ctx.options & ~ssl.OP_NO_TLSv1)
        self.assertEqual(default, ctx.options)

        # clear all options
        ctx.options = 0
        # Ubuntu has OP_NO_SSLv3 forced on by default
        self.assertEqual(0, ctx.options & ~ssl.OP_NO_SSLv3)

        # invalid options
        with self.assertRaises(ValueError):
            ctx.options = -1
        with self.assertRaises(OverflowError):
            ctx.options = 2 ** 100
        with self.assertRaises(TypeError):
            ctx.options = "abc"

    def test_verify_mode_protocol(self):
        with warnings_helper.check_warnings():
            ctx = ssl.SSLContext(ssl.PROTOCOL_TLS)
        # Default value
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)
        ctx.verify_mode = ssl.CERT_OPTIONAL
        self.assertEqual(ctx.verify_mode, ssl.CERT_OPTIONAL)
        ctx.verify_mode = ssl.CERT_REQUIRED
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
        ctx.verify_mode = ssl.CERT_NONE
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)
        with self.assertRaises(TypeError):
            ctx.verify_mode = None
        with self.assertRaises(ValueError):
            ctx.verify_mode = 42

        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)
        self.assertFalse(ctx.check_hostname)

        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
        self.assertTrue(ctx.check_hostname)

    def test_hostname_checks_common_name(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertTrue(ctx.hostname_checks_common_name)
        if ssl.HAS_NEVER_CHECK_COMMON_NAME:
            ctx.hostname_checks_common_name = True
            self.assertTrue(ctx.hostname_checks_common_name)
            ctx.hostname_checks_common_name = False
            self.assertFalse(ctx.hostname_checks_common_name)
            ctx.hostname_checks_common_name = True
            self.assertTrue(ctx.hostname_checks_common_name)
        else:
            with self.assertRaises(AttributeError):
                ctx.hostname_checks_common_name = True

    @ignore_deprecation
    def test_min_max_version(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        # OpenSSL default is MINIMUM_SUPPORTED, however some vendors like
        # Fedora override the setting to TLS 1.0.
        minimum_range = {
            # stock OpenSSL
            ssl.TLSVersion.MINIMUM_SUPPORTED,
            # Fedora 29 uses TLS 1.0 by default
            ssl.TLSVersion.TLSv1,
            # RHEL 8 uses TLS 1.2 by default
            ssl.TLSVersion.TLSv1_2
        }
        maximum_range = {
            # stock OpenSSL
            ssl.TLSVersion.MAXIMUM_SUPPORTED,
            # Fedora 32 uses TLS 1.3 by default
            ssl.TLSVersion.TLSv1_3
        }

        self.assertIn(
            ctx.minimum_version, minimum_range
        )
        self.assertIn(
            ctx.maximum_version, maximum_range
        )

        ctx.minimum_version = ssl.TLSVersion.TLSv1_1
        ctx.maximum_version = ssl.TLSVersion.TLSv1_2
        self.assertEqual(
            ctx.minimum_version, ssl.TLSVersion.TLSv1_1
        )
        self.assertEqual(
            ctx.maximum_version, ssl.TLSVersion.TLSv1_2
        )

        ctx.minimum_version = ssl.TLSVersion.MINIMUM_SUPPORTED
        ctx.maximum_version = ssl.TLSVersion.TLSv1
        self.assertEqual(
            ctx.minimum_version, ssl.TLSVersion.MINIMUM_SUPPORTED
        )
        self.assertEqual(
            ctx.maximum_version, ssl.TLSVersion.TLSv1
        )

        ctx.maximum_version = ssl.TLSVersion.MAXIMUM_SUPPORTED
        self.assertEqual(
            ctx.maximum_version, ssl.TLSVersion.MAXIMUM_SUPPORTED
        )

        ctx.maximum_version = ssl.TLSVersion.MINIMUM_SUPPORTED
        self.assertIn(
            ctx.maximum_version,
            {ssl.TLSVersion.TLSv1, ssl.TLSVersion.TLSv1_1, ssl.TLSVersion.SSLv3}
        )

        ctx.minimum_version = ssl.TLSVersion.MAXIMUM_SUPPORTED
        self.assertIn(
            ctx.minimum_version,
            {ssl.TLSVersion.TLSv1_2, ssl.TLSVersion.TLSv1_3}
        )

        with self.assertRaises(ValueError):
            ctx.minimum_version = 42

        if has_tls_protocol(ssl.PROTOCOL_TLSv1_1):
            ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1_1)

            self.assertIn(
                ctx.minimum_version, minimum_range
            )
            self.assertEqual(
                ctx.maximum_version, ssl.TLSVersion.MAXIMUM_SUPPORTED
            )
            with self.assertRaises(ValueError):
                ctx.minimum_version = ssl.TLSVersion.MINIMUM_SUPPORTED
            with self.assertRaises(ValueError):
                ctx.maximum_version = ssl.TLSVersion.TLSv1

    @unittest.skipUnless(
        hasattr(ssl.SSLContext, 'security_level'),
        "requires OpenSSL >= 1.1.0"
    )
    def test_security_level(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        # The default security callback allows for levels between 0-5
        # with OpenSSL defaulting to 1, however some vendors override the
        # default value (e.g. Debian defaults to 2)
        security_level_range = {
            0,
            1, # OpenSSL default
            2, # Debian
            3,
            4,
            5,
        }
        self.assertIn(ctx.security_level, security_level_range)

    def test_verify_flags(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        # default value
        tf = getattr(ssl, "VERIFY_X509_TRUSTED_FIRST", 0)
        self.assertEqual(ctx.verify_flags, ssl.VERIFY_DEFAULT | tf)
        ctx.verify_flags = ssl.VERIFY_CRL_CHECK_LEAF
        self.assertEqual(ctx.verify_flags, ssl.VERIFY_CRL_CHECK_LEAF)
        ctx.verify_flags = ssl.VERIFY_CRL_CHECK_CHAIN
        self.assertEqual(ctx.verify_flags, ssl.VERIFY_CRL_CHECK_CHAIN)
        ctx.verify_flags = ssl.VERIFY_DEFAULT
        self.assertEqual(ctx.verify_flags, ssl.VERIFY_DEFAULT)
        ctx.verify_flags = ssl.VERIFY_ALLOW_PROXY_CERTS
        self.assertEqual(ctx.verify_flags, ssl.VERIFY_ALLOW_PROXY_CERTS)
        # supports any value
        ctx.verify_flags = ssl.VERIFY_CRL_CHECK_LEAF | ssl.VERIFY_X509_STRICT
        self.assertEqual(ctx.verify_flags,
                         ssl.VERIFY_CRL_CHECK_LEAF | ssl.VERIFY_X509_STRICT)
        with self.assertRaises(TypeError):
            ctx.verify_flags = None

    def test_load_cert_chain(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        # Combined key and cert in a single file
        ctx.load_cert_chain(CERTFILE, keyfile=None)
        ctx.load_cert_chain(CERTFILE, keyfile=CERTFILE)
        self.assertRaises(TypeError, ctx.load_cert_chain, keyfile=CERTFILE)
        with self.assertRaises(OSError) as cm:
            ctx.load_cert_chain(NONEXISTINGCERT)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        with self.assertRaisesRegex(ssl.SSLError, "PEM (lib|routines)"):
            ctx.load_cert_chain(BADCERT)
        with self.assertRaisesRegex(ssl.SSLError, "PEM (lib|routines)"):
            ctx.load_cert_chain(EMPTYCERT)
        # Separate key and cert
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ctx.load_cert_chain(ONLYCERT, ONLYKEY)
        ctx.load_cert_chain(certfile=ONLYCERT, keyfile=ONLYKEY)
        ctx.load_cert_chain(certfile=BYTES_ONLYCERT, keyfile=BYTES_ONLYKEY)
        with self.assertRaisesRegex(ssl.SSLError, "PEM (lib|routines)"):
            ctx.load_cert_chain(ONLYCERT)
        with self.assertRaisesRegex(ssl.SSLError, "PEM (lib|routines)"):
            ctx.load_cert_chain(ONLYKEY)
        with self.assertRaisesRegex(ssl.SSLError, "PEM (lib|routines)"):
            ctx.load_cert_chain(certfile=ONLYKEY, keyfile=ONLYCERT)
        # Mismatching key and cert
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        # Allow for flexible libssl error messages.
        regex = re.compile(r"""(
            key values mismatch         # OpenSSL
            |
            KEY_VALUES_MISMATCH         # AWS-LC
        )""", re.X)
        with self.assertRaisesRegex(ssl.SSLError, regex):
            ctx.load_cert_chain(CAFILE_CACERT, ONLYKEY)
        # Password protected key and cert
        ctx.load_cert_chain(CERTFILE_PROTECTED, password=KEY_PASSWORD)
        ctx.load_cert_chain(CERTFILE_PROTECTED, password=KEY_PASSWORD.encode())
        ctx.load_cert_chain(CERTFILE_PROTECTED,
                            password=bytearray(KEY_PASSWORD.encode()))
        ctx.load_cert_chain(ONLYCERT, ONLYKEY_PROTECTED, KEY_PASSWORD)
        ctx.load_cert_chain(ONLYCERT, ONLYKEY_PROTECTED, KEY_PASSWORD.encode())
        ctx.load_cert_chain(ONLYCERT, ONLYKEY_PROTECTED,
                            bytearray(KEY_PASSWORD.encode()))
        with self.assertRaisesRegex(TypeError, "should be a string"):
            ctx.load_cert_chain(CERTFILE_PROTECTED, password=True)
        with self.assertRaises(ssl.SSLError):
            ctx.load_cert_chain(CERTFILE_PROTECTED, password="badpass")
        with self.assertRaisesRegex(ValueError, "cannot be longer"):
            # openssl has a fixed limit on the password buffer.
            # PEM_BUFSIZE is generally set to 1kb.
            # Return a string larger than this.
            ctx.load_cert_chain(CERTFILE_PROTECTED, password=b'a' * 102400)
        # Password callback
        def getpass_unicode():
            return KEY_PASSWORD
        def getpass_bytes():
            return KEY_PASSWORD.encode()
        def getpass_bytearray():
            return bytearray(KEY_PASSWORD.encode())
        def getpass_badpass():
            return "badpass"
        def getpass_huge():
            return b'a' * (1024 * 1024)
        def getpass_bad_type():
            return 9
        def getpass_exception():
            raise Exception('getpass error')
        class GetPassCallable:
            def __call__(self):
                return KEY_PASSWORD
            def getpass(self):
                return KEY_PASSWORD
        ctx.load_cert_chain(CERTFILE_PROTECTED, password=getpass_unicode)
        ctx.load_cert_chain(CERTFILE_PROTECTED, password=getpass_bytes)
        ctx.load_cert_chain(CERTFILE_PROTECTED, password=getpass_bytearray)
        ctx.load_cert_chain(CERTFILE_PROTECTED, password=GetPassCallable())
        ctx.load_cert_chain(CERTFILE_PROTECTED,
                            password=GetPassCallable().getpass)
        with self.assertRaises(ssl.SSLError):
            ctx.load_cert_chain(CERTFILE_PROTECTED, password=getpass_badpass)
        with self.assertRaisesRegex(ValueError, "cannot be longer"):
            ctx.load_cert_chain(CERTFILE_PROTECTED, password=getpass_huge)
        with self.assertRaisesRegex(TypeError, "must return a string"):
            ctx.load_cert_chain(CERTFILE_PROTECTED, password=getpass_bad_type)
        with self.assertRaisesRegex(Exception, "getpass error"):
            ctx.load_cert_chain(CERTFILE_PROTECTED, password=getpass_exception)
        # Make sure the password function isn't called if it isn't needed
        ctx.load_cert_chain(CERTFILE, password=getpass_exception)

    def test_load_verify_locations(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ctx.load_verify_locations(CERTFILE)
        ctx.load_verify_locations(cafile=CERTFILE, capath=None)
        ctx.load_verify_locations(BYTES_CERTFILE)
        ctx.load_verify_locations(cafile=BYTES_CERTFILE, capath=None)
        self.assertRaises(TypeError, ctx.load_verify_locations)
        self.assertRaises(TypeError, ctx.load_verify_locations, None, None, None)
        with self.assertRaises(OSError) as cm:
            ctx.load_verify_locations(NONEXISTINGCERT)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        with self.assertRaisesRegex(ssl.SSLError, "PEM (lib|routines)"):
            ctx.load_verify_locations(BADCERT)
        ctx.load_verify_locations(CERTFILE, CAPATH)
        ctx.load_verify_locations(CERTFILE, capath=BYTES_CAPATH)

        # Issue #10989: crash if the second argument type is invalid
        self.assertRaises(TypeError, ctx.load_verify_locations, None, True)

    def test_load_verify_cadata(self):
        # test cadata
        with open(CAFILE_CACERT) as f:
            cacert_pem = f.read()
        cacert_der = ssl.PEM_cert_to_DER_cert(cacert_pem)
        with open(CAFILE_NEURONIO) as f:
            neuronio_pem = f.read()
        neuronio_der = ssl.PEM_cert_to_DER_cert(neuronio_pem)

        # test PEM
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 0)
        ctx.load_verify_locations(cadata=cacert_pem)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 1)
        ctx.load_verify_locations(cadata=neuronio_pem)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)
        # cert already in hash table
        ctx.load_verify_locations(cadata=neuronio_pem)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)

        # combined
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        combined = "\n".join((cacert_pem, neuronio_pem))
        ctx.load_verify_locations(cadata=combined)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)

        # with junk around the certs
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        combined = ["head", cacert_pem, "other", neuronio_pem, "again",
                    neuronio_pem, "tail"]
        ctx.load_verify_locations(cadata="\n".join(combined))
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)

        # test DER
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.load_verify_locations(cadata=cacert_der)
        ctx.load_verify_locations(cadata=neuronio_der)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)
        # cert already in hash table
        ctx.load_verify_locations(cadata=cacert_der)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)

        # combined
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        combined = b"".join((cacert_der, neuronio_der))
        ctx.load_verify_locations(cadata=combined)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)

        # error cases
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertRaises(TypeError, ctx.load_verify_locations, cadata=object)

        with self.assertRaisesRegex(
            ssl.SSLError,
            "no start line: cadata does not contain a certificate"
        ):
            ctx.load_verify_locations(cadata="broken")
        with self.assertRaisesRegex(
            ssl.SSLError,
            "not enough data: cadata does not contain a certificate"
        ):
            ctx.load_verify_locations(cadata=b"broken")
        with self.assertRaises(ssl.SSLError):
            ctx.load_verify_locations(cadata=cacert_der + b"A")

    def test_load_dh_params(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        try:
            ctx.load_dh_params(DHFILE)
        except RuntimeError:
            if Py_DEBUG_WIN32:
                self.skipTest("not supported on Win32 debug build")
            raise
        ctx.load_dh_params(BYTES_DHFILE)
        self.assertRaises(TypeError, ctx.load_dh_params)
        self.assertRaises(TypeError, ctx.load_dh_params, None)
        with self.assertRaises(FileNotFoundError) as cm:
            ctx.load_dh_params(NONEXISTINGCERT)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        with self.assertRaises(ssl.SSLError) as cm:
            ctx.load_dh_params(CERTFILE)

    def test_session_stats(self):
        for proto in {ssl.PROTOCOL_TLS_CLIENT, ssl.PROTOCOL_TLS_SERVER}:
            ctx = ssl.SSLContext(proto)
            self.assertEqual(ctx.session_stats(), {
                'number': 0,
                'connect': 0,
                'connect_good': 0,
                'connect_renegotiate': 0,
                'accept': 0,
                'accept_good': 0,
                'accept_renegotiate': 0,
                'hits': 0,
                'misses': 0,
                'timeouts': 0,
                'cache_full': 0,
            })

    def test_set_default_verify_paths(self):
        # There's not much we can do to test that it acts as expected,
        # so just check it doesn't crash or raise an exception.
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.set_default_verify_paths()

    @unittest.skipUnless(ssl.HAS_ECDH, "ECDH disabled on this OpenSSL build")
    def test_set_ecdh_curve(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ctx.set_ecdh_curve("prime256v1")
        ctx.set_ecdh_curve(b"prime256v1")
        self.assertRaises(TypeError, ctx.set_ecdh_curve)
        self.assertRaises(TypeError, ctx.set_ecdh_curve, None)
        self.assertRaises(ValueError, ctx.set_ecdh_curve, "foo")
        self.assertRaises(ValueError, ctx.set_ecdh_curve, b"foo")

    def test_sni_callback(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)

        # set_servername_callback expects a callable, or None
        self.assertRaises(TypeError, ctx.set_servername_callback)
        self.assertRaises(TypeError, ctx.set_servername_callback, 4)
        self.assertRaises(TypeError, ctx.set_servername_callback, "")
        self.assertRaises(TypeError, ctx.set_servername_callback, ctx)

        def dummycallback(sock, servername, ctx):
            pass
        ctx.set_servername_callback(None)
        ctx.set_servername_callback(dummycallback)

    def test_sni_callback_refcycle(self):
        # Reference cycles through the servername callback are detected
        # and cleared.
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        def dummycallback(sock, servername, ctx, cycle=ctx):
            pass
        ctx.set_servername_callback(dummycallback)
        wr = weakref.ref(ctx)
        del ctx, dummycallback
        gc.collect()
        self.assertIs(wr(), None)

    def test_cert_store_stats(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertEqual(ctx.cert_store_stats(),
            {'x509_ca': 0, 'crl': 0, 'x509': 0})
        ctx.load_cert_chain(CERTFILE)
        self.assertEqual(ctx.cert_store_stats(),
            {'x509_ca': 0, 'crl': 0, 'x509': 0})
        ctx.load_verify_locations(CERTFILE)
        self.assertEqual(ctx.cert_store_stats(),
            {'x509_ca': 0, 'crl': 0, 'x509': 1})
        ctx.load_verify_locations(CAFILE_CACERT)
        self.assertEqual(ctx.cert_store_stats(),
            {'x509_ca': 1, 'crl': 0, 'x509': 2})

    def test_get_ca_certs(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertEqual(ctx.get_ca_certs(), [])
        # CERTFILE is not flagged as X509v3 Basic Constraints: CA:TRUE
        ctx.load_verify_locations(CERTFILE)
        self.assertEqual(ctx.get_ca_certs(), [])
        # but CAFILE_CACERT is a CA cert
        ctx.load_verify_locations(CAFILE_CACERT)
        self.assertEqual(ctx.get_ca_certs(),
            [{'issuer': ((('organizationName', 'Root CA'),),
                         (('organizationalUnitName', 'http://www.cacert.org'),),
                         (('commonName', 'CA Cert Signing Authority'),),
                         (('emailAddress', 'support@cacert.org'),)),
              'notAfter': 'Mar 29 12:29:49 2033 GMT',
              'notBefore': 'Mar 30 12:29:49 2003 GMT',
              'serialNumber': '00',
              'crlDistributionPoints': ('https://www.cacert.org/revoke.crl',),
              'subject': ((('organizationName', 'Root CA'),),
                          (('organizationalUnitName', 'http://www.cacert.org'),),
                          (('commonName', 'CA Cert Signing Authority'),),
                          (('emailAddress', 'support@cacert.org'),)),
              'version': 3}])

        with open(CAFILE_CACERT) as f:
            pem = f.read()
        der = ssl.PEM_cert_to_DER_cert(pem)
        self.assertEqual(ctx.get_ca_certs(True), [der])

    def test_load_default_certs(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.load_default_certs()

        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.load_default_certs(ssl.Purpose.SERVER_AUTH)
        ctx.load_default_certs()

        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.load_default_certs(ssl.Purpose.CLIENT_AUTH)

        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertRaises(TypeError, ctx.load_default_certs, None)
        self.assertRaises(TypeError, ctx.load_default_certs, 'SERVER_AUTH')

    @unittest.skipIf(sys.platform == "win32", "not-Windows specific")
    def test_load_default_certs_env(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        with os_helper.EnvironmentVarGuard() as env:
            env["SSL_CERT_DIR"] = CAPATH
            env["SSL_CERT_FILE"] = CERTFILE
            ctx.load_default_certs()
            self.assertEqual(ctx.cert_store_stats(), {"crl": 0, "x509": 1, "x509_ca": 0})

    @unittest.skipUnless(sys.platform == "win32", "Windows specific")
    @unittest.skipIf(support.Py_DEBUG,
                     "Debug build does not share environment between CRTs")
    def test_load_default_certs_env_windows(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.load_default_certs()
        stats = ctx.cert_store_stats()

        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        with os_helper.EnvironmentVarGuard() as env:
            env["SSL_CERT_DIR"] = CAPATH
            env["SSL_CERT_FILE"] = CERTFILE
            ctx.load_default_certs()
            stats["x509"] += 1
            self.assertEqual(ctx.cert_store_stats(), stats)

    def _assert_context_options(self, ctx):
        self.assertEqual(ctx.options & ssl.OP_NO_SSLv2, ssl.OP_NO_SSLv2)
        if OP_NO_COMPRESSION != 0:
            self.assertEqual(ctx.options & OP_NO_COMPRESSION,
                             OP_NO_COMPRESSION)
        if OP_SINGLE_DH_USE != 0:
            self.assertEqual(ctx.options & OP_SINGLE_DH_USE,
                             OP_SINGLE_DH_USE)
        if OP_SINGLE_ECDH_USE != 0:
            self.assertEqual(ctx.options & OP_SINGLE_ECDH_USE,
                             OP_SINGLE_ECDH_USE)
        if OP_CIPHER_SERVER_PREFERENCE != 0:
            self.assertEqual(ctx.options & OP_CIPHER_SERVER_PREFERENCE,
                             OP_CIPHER_SERVER_PREFERENCE)
        self.assertEqual(ctx.options & ssl.OP_LEGACY_SERVER_CONNECT,
                         0 if IS_OPENSSL_3_0_0 else ssl.OP_LEGACY_SERVER_CONNECT)

    def test_create_default_context(self):
        ctx = ssl.create_default_context()

        self.assertEqual(ctx.protocol, ssl.PROTOCOL_TLS_CLIENT)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
        self.assertEqual(ctx.verify_flags & ssl.VERIFY_X509_PARTIAL_CHAIN,
                         ssl.VERIFY_X509_PARTIAL_CHAIN)
        self.assertEqual(ctx.verify_flags & ssl.VERIFY_X509_STRICT,
                    ssl.VERIFY_X509_STRICT)
        self.assertTrue(ctx.check_hostname)
        self._assert_context_options(ctx)

        with open(SIGNING_CA) as f:
            cadata = f.read()
        ctx = ssl.create_default_context(cafile=SIGNING_CA, capath=CAPATH,
                                         cadata=cadata)
        self.assertEqual(ctx.protocol, ssl.PROTOCOL_TLS_CLIENT)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
        self._assert_context_options(ctx)

        ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
        self.assertEqual(ctx.protocol, ssl.PROTOCOL_TLS_SERVER)
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)
        self._assert_context_options(ctx)

    def test__create_stdlib_context(self):
        ctx = ssl._create_stdlib_context()
        self.assertEqual(ctx.protocol, ssl.PROTOCOL_TLS_CLIENT)
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)
        self.assertFalse(ctx.check_hostname)
        self._assert_context_options(ctx)

        if has_tls_protocol(ssl.PROTOCOL_TLSv1):
            with warnings_helper.check_warnings():
                ctx = ssl._create_stdlib_context(ssl.PROTOCOL_TLSv1)
            self.assertEqual(ctx.protocol, ssl.PROTOCOL_TLSv1)
            self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)
            self._assert_context_options(ctx)

        with warnings_helper.check_warnings():
            ctx = ssl._create_stdlib_context(
                ssl.PROTOCOL_TLSv1_2,
                cert_reqs=ssl.CERT_REQUIRED,
                check_hostname=True
            )
        self.assertEqual(ctx.protocol, ssl.PROTOCOL_TLSv1_2)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
        self.assertTrue(ctx.check_hostname)
        self._assert_context_options(ctx)

        ctx = ssl._create_stdlib_context(purpose=ssl.Purpose.CLIENT_AUTH)
        self.assertEqual(ctx.protocol, ssl.PROTOCOL_TLS_SERVER)
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)
        self._assert_context_options(ctx)

    def test_check_hostname(self):
        with warnings_helper.check_warnings():
            ctx = ssl.SSLContext(ssl.PROTOCOL_TLS)
        self.assertFalse(ctx.check_hostname)
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)

        # Auto set CERT_REQUIRED
        ctx.check_hostname = True
        self.assertTrue(ctx.check_hostname)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_REQUIRED
        self.assertFalse(ctx.check_hostname)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)

        # Changing verify_mode does not affect check_hostname
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        ctx.check_hostname = False
        self.assertFalse(ctx.check_hostname)
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)
        # Auto set
        ctx.check_hostname = True
        self.assertTrue(ctx.check_hostname)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)

        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_OPTIONAL
        ctx.check_hostname = False
        self.assertFalse(ctx.check_hostname)
        self.assertEqual(ctx.verify_mode, ssl.CERT_OPTIONAL)
        # keep CERT_OPTIONAL
        ctx.check_hostname = True
        self.assertTrue(ctx.check_hostname)
        self.assertEqual(ctx.verify_mode, ssl.CERT_OPTIONAL)

        # Cannot set CERT_NONE with check_hostname enabled
        with self.assertRaises(ValueError):
            ctx.verify_mode = ssl.CERT_NONE
        ctx.check_hostname = False
        self.assertFalse(ctx.check_hostname)
        ctx.verify_mode = ssl.CERT_NONE
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)

    def test_context_client_server(self):
        # PROTOCOL_TLS_CLIENT has sane defaults
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertTrue(ctx.check_hostname)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)

        # PROTOCOL_TLS_SERVER has different but also sane defaults
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        self.assertFalse(ctx.check_hostname)
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)

    def test_context_custom_class(self):
        class MySSLSocket(ssl.SSLSocket):
            pass

        class MySSLObject(ssl.SSLObject):
            pass

        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        ctx.sslsocket_class = MySSLSocket
        ctx.sslobject_class = MySSLObject

        with ctx.wrap_socket(socket.socket(), server_side=True) as sock:
            self.assertIsInstance(sock, MySSLSocket)
        obj = ctx.wrap_bio(ssl.MemoryBIO(), ssl.MemoryBIO(), server_side=True)
        self.assertIsInstance(obj, MySSLObject)

    def test_num_tickest(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        self.assertEqual(ctx.num_tickets, 2)
        ctx.num_tickets = 1
        self.assertEqual(ctx.num_tickets, 1)
        ctx.num_tickets = 0
        self.assertEqual(ctx.num_tickets, 0)
        with self.assertRaises(ValueError):
            ctx.num_tickets = -1
        with self.assertRaises(TypeError):
            ctx.num_tickets = None

        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertEqual(ctx.num_tickets, 2)
        with self.assertRaises(ValueError):
            ctx.num_tickets = 1


class SSLErrorTests(unittest.TestCase):

    def test_str(self):
        # The str() of a SSLError doesn't include the errno
        e = ssl.SSLError(1, "foo")
        self.assertEqual(str(e), "foo")
        self.assertEqual(e.errno, 1)
        # Same for a subclass
        e = ssl.SSLZeroReturnError(1, "foo")
        self.assertEqual(str(e), "foo")
        self.assertEqual(e.errno, 1)

    def test_lib_reason(self):
        # Test the library and reason attributes
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        try:
            with self.assertRaises(ssl.SSLError) as cm:
                ctx.load_dh_params(CERTFILE)
        except RuntimeError:
            if Py_DEBUG_WIN32:
                self.skipTest("not supported on Win32 debug build")
            raise

        self.assertEqual(cm.exception.library, 'PEM')
        regex = "(NO_START_LINE|UNSUPPORTED_PUBLIC_KEY_TYPE)"
        self.assertRegex(cm.exception.reason, regex)
        s = str(cm.exception)
        self.assertIn("NO_START_LINE", s)

    def test_subclass(self):
        # Check that the appropriate SSLError subclass is raised
        # (this only tests one of them)
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        with socket.create_server(("127.0.0.1", 0)) as s:
            c = socket.create_connection(s.getsockname())
            c.setblocking(False)
            with ctx.wrap_socket(c, False, do_handshake_on_connect=False) as c:
                with self.assertRaises(ssl.SSLWantReadError) as cm:
                    c.do_handshake()
                s = str(cm.exception)
                self.assertStartsWith(s, "The operation did not complete (read)")
                # For compatibility
                self.assertEqual(cm.exception.errno, ssl.SSL_ERROR_WANT_READ)


    def test_bad_server_hostname(self):
        ctx = ssl.create_default_context()
        with self.assertRaises(ValueError):
            ctx.wrap_bio(ssl.MemoryBIO(), ssl.MemoryBIO(),
                         server_hostname="")
        with self.assertRaises(ValueError):
            ctx.wrap_bio(ssl.MemoryBIO(), ssl.MemoryBIO(),
                         server_hostname=".example.org")
        with self.assertRaises(TypeError):
            ctx.wrap_bio(ssl.MemoryBIO(), ssl.MemoryBIO(),
                         server_hostname="example.org\x00evil.com")


class MemoryBIOTests(unittest.TestCase):

    def test_read_write(self):
        bio = ssl.MemoryBIO()
        bio.write(b'foo')
        self.assertEqual(bio.read(), b'foo')
        self.assertEqual(bio.read(), b'')
        bio.write(b'foo')
        bio.write(b'bar')
        self.assertEqual(bio.read(), b'foobar')
        self.assertEqual(bio.read(), b'')
        bio.write(b'baz')
        self.assertEqual(bio.read(2), b'ba')
        self.assertEqual(bio.read(1), b'z')
        self.assertEqual(bio.read(1), b'')

    def test_eof(self):
        bio = ssl.MemoryBIO()
        self.assertFalse(bio.eof)
        self.assertEqual(bio.read(), b'')
        self.assertFalse(bio.eof)
        bio.write(b'foo')
        self.assertFalse(bio.eof)
        bio.write_eof()
        self.assertFalse(bio.eof)
        self.assertEqual(bio.read(2), b'fo')
        self.assertFalse(bio.eof)
        self.assertEqual(bio.read(1), b'o')
        self.assertTrue(bio.eof)
        self.assertEqual(bio.read(), b'')
        self.assertTrue(bio.eof)

    def test_pending(self):
        bio = ssl.MemoryBIO()
        self.assertEqual(bio.pending, 0)
        bio.write(b'foo')
        self.assertEqual(bio.pending, 3)
        for i in range(3):
            bio.read(1)
            self.assertEqual(bio.pending, 3-i-1)
        for i in range(3):
            bio.write(b'x')
            self.assertEqual(bio.pending, i+1)
        bio.read()
        self.assertEqual(bio.pending, 0)

    def test_buffer_types(self):
        bio = ssl.MemoryBIO()
        bio.write(b'foo')
        self.assertEqual(bio.read(), b'foo')
        bio.write(bytearray(b'bar'))
        self.assertEqual(bio.read(), b'bar')
        bio.write(memoryview(b'baz'))
        self.assertEqual(bio.read(), b'baz')
        m = memoryview(bytearray(b'noncontig'))
        noncontig_writable = m[::-2]
        with self.assertRaises(BufferError):
            bio.write(memoryview(noncontig_writable))

    def test_error_types(self):
        bio = ssl.MemoryBIO()
        self.assertRaises(TypeError, bio.write, 'foo')
        self.assertRaises(TypeError, bio.write, None)
        self.assertRaises(TypeError, bio.write, True)
        self.assertRaises(TypeError, bio.write, 1)


class SSLObjectTests(unittest.TestCase):
    def test_private_init(self):
        bio = ssl.MemoryBIO()
        with self.assertRaisesRegex(TypeError, "public constructor"):
            ssl.SSLObject(bio, bio)

    def test_unwrap(self):
        client_ctx, server_ctx, hostname = testing_context()
        c_in = ssl.MemoryBIO()
        c_out = ssl.MemoryBIO()
        s_in = ssl.MemoryBIO()
        s_out = ssl.MemoryBIO()
        client = client_ctx.wrap_bio(c_in, c_out, server_hostname=hostname)
        server = server_ctx.wrap_bio(s_in, s_out, server_side=True)

        # Loop on the handshake for a bit to get it settled
        for _ in range(5):
            try:
                client.do_handshake()
            except ssl.SSLWantReadError:
                pass
            if c_out.pending:
                s_in.write(c_out.read())
            try:
                server.do_handshake()
            except ssl.SSLWantReadError:
                pass
            if s_out.pending:
                c_in.write(s_out.read())
        # Now the handshakes should be complete (don't raise WantReadError)
        client.do_handshake()
        server.do_handshake()

        # Now if we unwrap one side unilaterally, it should send close-notify
        # and raise WantReadError:
        with self.assertRaises(ssl.SSLWantReadError):
            client.unwrap()

        # But server.unwrap() does not raise, because it reads the client's
        # close-notify:
        s_in.write(c_out.read())
        server.unwrap()

        # And now that the client gets the server's close-notify, it doesn't
        # raise either.
        c_in.write(s_out.read())
        client.unwrap()

class SimpleBackgroundTests(unittest.TestCase):
    """Tests that connect to a simple server running in the background"""

    def setUp(self):
        self.server_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        self.server_context.load_cert_chain(SIGNED_CERTFILE)
        server = ThreadedEchoServer(context=self.server_context)
        self.enterContext(server)
        self.server_addr = (HOST, server.port)

    def test_connect(self):
        with test_wrap_socket(socket.socket(socket.AF_INET),
                            cert_reqs=ssl.CERT_NONE) as s:
            s.connect(self.server_addr)
            self.assertEqual({}, s.getpeercert())
            self.assertFalse(s.server_side)

        # this should succeed because we specify the root cert
        with test_wrap_socket(socket.socket(socket.AF_INET),
                            cert_reqs=ssl.CERT_REQUIRED,
                            ca_certs=SIGNING_CA) as s:
            s.connect(self.server_addr)
            self.assertTrue(s.getpeercert())
            self.assertFalse(s.server_side)

    def test_connect_fail(self):
        # This should fail because we have no verification certs. Connection
        # failure crashes ThreadedEchoServer, so run this in an independent
        # test method.
        s = test_wrap_socket(socket.socket(socket.AF_INET),
                            cert_reqs=ssl.CERT_REQUIRED)
        self.addCleanup(s.close)
        # Allow for flexible libssl error messages.
        regex = re.compile(r"""(
            certificate verify failed   # OpenSSL
            |
            CERTIFICATE_VERIFY_FAILED   # AWS-LC
        )""", re.X)
        self.assertRaisesRegex(ssl.SSLError, regex,
                               s.connect, self.server_addr)

    def test_connect_ex(self):
        # Issue #11326: check connect_ex() implementation
        s = test_wrap_socket(socket.socket(socket.AF_INET),
                            cert_reqs=ssl.CERT_REQUIRED,
                            ca_certs=SIGNING_CA)
        self.addCleanup(s.close)
        self.assertEqual(0, s.connect_ex(self.server_addr))
        self.assertTrue(s.getpeercert())

    def test_non_blocking_connect_ex(self):
        # Issue #11326: non-blocking connect_ex() should allow handshake
        # to proceed after the socket gets ready.
        s = test_wrap_socket(socket.socket(socket.AF_INET),
                            cert_reqs=ssl.CERT_REQUIRED,
                            ca_certs=SIGNING_CA,
                            do_handshake_on_connect=False)
        self.addCleanup(s.close)
        s.setblocking(False)
        rc = s.connect_ex(self.server_addr)
        # EWOULDBLOCK under Windows, EINPROGRESS elsewhere
        self.assertIn(rc, (0, errno.EINPROGRESS, errno.EWOULDBLOCK))
        # Wait for connect to finish
        select.select([], [s], [], 5.0)
        # Non-blocking handshake
        while True:
            try:
                s.do_handshake()
                break
            except ssl.SSLWantReadError:
                select.select([s], [], [], 5.0)
            except ssl.SSLWantWriteError:
                select.select([], [s], [], 5.0)
        # SSL established
        self.assertTrue(s.getpeercert())

    def test_connect_with_context(self):
        # Same as test_connect, but with a separately created context
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        with ctx.wrap_socket(socket.socket(socket.AF_INET)) as s:
            s.connect(self.server_addr)
            self.assertEqual({}, s.getpeercert())
        # Same with a server hostname
        with ctx.wrap_socket(socket.socket(socket.AF_INET),
                            server_hostname="dummy") as s:
            s.connect(self.server_addr)
        ctx.verify_mode = ssl.CERT_REQUIRED
        # This should succeed because we specify the root cert
        ctx.load_verify_locations(SIGNING_CA)
        with ctx.wrap_socket(socket.socket(socket.AF_INET)) as s:
            s.connect(self.server_addr)
            cert = s.getpeercert()
            self.assertTrue(cert)

    def test_connect_with_context_fail(self):
        # This should fail because we have no verification certs. Connection
        # failure crashes ThreadedEchoServer, so run this in an independent
        # test method.
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        s = ctx.wrap_socket(
            socket.socket(socket.AF_INET),
            server_hostname=SIGNED_CERTFILE_HOSTNAME
        )
        self.addCleanup(s.close)
        # Allow for flexible libssl error messages.
        regex = re.compile(r"""(
            certificate verify failed   # OpenSSL
            |
            CERTIFICATE_VERIFY_FAILED   # AWS-LC
        )""", re.X)
        self.assertRaisesRegex(ssl.SSLError, regex,
                                s.connect, self.server_addr)

    def test_connect_capath(self):
        # Verify server certificates using the `capath` argument
        # NOTE: the subject hashing algorithm has been changed between
        # OpenSSL 0.9.8n and 1.0.0, as a result the capath directory must
        # contain both versions of each certificate (same content, different
        # filename) for this test to be portable across OpenSSL releases.
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.load_verify_locations(capath=CAPATH)
        with ctx.wrap_socket(socket.socket(socket.AF_INET),
                             server_hostname=SIGNED_CERTFILE_HOSTNAME) as s:
            s.connect(self.server_addr)
            cert = s.getpeercert()
            self.assertTrue(cert)

        # Same with a bytes `capath` argument
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.load_verify_locations(capath=BYTES_CAPATH)
        with ctx.wrap_socket(socket.socket(socket.AF_INET),
                             server_hostname=SIGNED_CERTFILE_HOSTNAME) as s:
            s.connect(self.server_addr)
            cert = s.getpeercert()
            self.assertTrue(cert)

    def test_connect_cadata(self):
        with open(SIGNING_CA) as f:
            pem = f.read()
        der = ssl.PEM_cert_to_DER_cert(pem)
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.load_verify_locations(cadata=pem)
        with ctx.wrap_socket(socket.socket(socket.AF_INET),
                             server_hostname=SIGNED_CERTFILE_HOSTNAME) as s:
            s.connect(self.server_addr)
            cert = s.getpeercert()
            self.assertTrue(cert)

        # same with DER
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.load_verify_locations(cadata=der)
        with ctx.wrap_socket(socket.socket(socket.AF_INET),
                             server_hostname=SIGNED_CERTFILE_HOSTNAME) as s:
            s.connect(self.server_addr)
            cert = s.getpeercert()
            self.assertTrue(cert)

    @unittest.skipIf(os.name == "nt", "Can't use a socket as a file under Windows")
    def test_makefile_close(self):
        # Issue #5238: creating a file-like object with makefile() shouldn't
        # delay closing the underlying "real socket" (here tested with its
        # file descriptor, hence skipping the test under Windows).
        ss = test_wrap_socket(socket.socket(socket.AF_INET))
        ss.connect(self.server_addr)
        fd = ss.fileno()
        f = ss.makefile()
        f.close()
        # The fd is still open
        os.read(fd, 0)
        # Closing the SSL socket should close the fd too
        ss.close()
        gc.collect()
        with self.assertRaises(OSError) as e:
            os.read(fd, 0)
        self.assertEqual(e.exception.errno, errno.EBADF)

    def test_non_blocking_handshake(self):
        s = socket.socket(socket.AF_INET)
        s.connect(self.server_addr)
        s.setblocking(False)
        s = test_wrap_socket(s,
                            cert_reqs=ssl.CERT_NONE,
                            do_handshake_on_connect=False)
        self.addCleanup(s.close)
        count = 0
        while True:
            try:
                count += 1
                s.do_handshake()
                break
            except ssl.SSLWantReadError:
                select.select([s], [], [])
            except ssl.SSLWantWriteError:
                select.select([], [s], [])
        if support.verbose:
            sys.stdout.write("\nNeeded %d calls to do_handshake() to establish session.\n" % count)

    def test_get_server_certificate(self):
        _test_get_server_certificate(self, *self.server_addr, cert=SIGNING_CA)

    def test_get_server_certificate_sni(self):
        host, port = self.server_addr
        server_names = []

        # We store servername_cb arguments to make sure they match the host
        def servername_cb(ssl_sock, server_name, initial_context):
            server_names.append(server_name)
        self.server_context.set_servername_callback(servername_cb)

        pem = ssl.get_server_certificate((host, port))
        if not pem:
            self.fail("No server certificate on %s:%s!" % (host, port))

        pem = ssl.get_server_certificate((host, port), ca_certs=SIGNING_CA)
        if not pem:
            self.fail("No server certificate on %s:%s!" % (host, port))
        if support.verbose:
            sys.stdout.write("\nVerified certificate for %s:%s is\n%s\n" % (host, port, pem))

        self.assertEqual(server_names, [host, host])

    def test_get_server_certificate_fail(self):
        # Connection failure crashes ThreadedEchoServer, so run this in an
        # independent test method
        _test_get_server_certificate_fail(self, *self.server_addr)

    def test_get_server_certificate_timeout(self):
        def servername_cb(ssl_sock, server_name, initial_context):
            time.sleep(0.2)
        self.server_context.set_servername_callback(servername_cb)

        with self.assertRaises(socket.timeout):
            ssl.get_server_certificate(self.server_addr, ca_certs=SIGNING_CA,
                                       timeout=0.1)

    def test_ciphers(self):
        with test_wrap_socket(socket.socket(socket.AF_INET),
                             cert_reqs=ssl.CERT_NONE, ciphers="ALL") as s:
            s.connect(self.server_addr)
        with test_wrap_socket(socket.socket(socket.AF_INET),
                             cert_reqs=ssl.CERT_NONE, ciphers="DEFAULT") as s:
            s.connect(self.server_addr)
        # Error checking can happen at instantiation or when connecting
        with self.assertRaisesRegex(ssl.SSLError, "No cipher can be selected"):
            with socket.socket(socket.AF_INET) as sock:
                s = test_wrap_socket(sock,
                                    cert_reqs=ssl.CERT_NONE, ciphers="^$:,;?*'dorothyx")
                s.connect(self.server_addr)

    def test_get_ca_certs_capath(self):
        # capath certs are loaded on request
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.load_verify_locations(capath=CAPATH)
        self.assertEqual(ctx.get_ca_certs(), [])
        with ctx.wrap_socket(socket.socket(socket.AF_INET),
                             server_hostname='localhost') as s:
            s.connect(self.server_addr)
            cert = s.getpeercert()
            self.assertTrue(cert)
        self.assertEqual(len(ctx.get_ca_certs()), 1)

    def test_context_setget(self):
        # Check that the context of a connected socket can be replaced.
        ctx1 = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx1.load_verify_locations(capath=CAPATH)
        ctx2 = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx2.load_verify_locations(capath=CAPATH)
        s = socket.socket(socket.AF_INET)
        with ctx1.wrap_socket(s, server_hostname='localhost') as ss:
            ss.connect(self.server_addr)
            self.assertIs(ss.context, ctx1)
            self.assertIs(ss._sslobj.context, ctx1)
            ss.context = ctx2
            self.assertIs(ss.context, ctx2)
            self.assertIs(ss._sslobj.context, ctx2)

    def ssl_io_loop(self, sock, incoming, outgoing, func, *args, **kwargs):
        # A simple IO loop. Call func(*args) depending on the error we get
        # (WANT_READ or WANT_WRITE) move data between the socket and the BIOs.
        timeout = kwargs.get('timeout', support.SHORT_TIMEOUT)
        count = 0
        for _ in support.busy_retry(timeout):
            errno = None
            count += 1
            try:
                ret = func(*args)
            except ssl.SSLError as e:
                if e.errno not in (ssl.SSL_ERROR_WANT_READ,
                                   ssl.SSL_ERROR_WANT_WRITE):
                    raise
                errno = e.errno
            # Get any data from the outgoing BIO irrespective of any error, and
            # send it to the socket.
            buf = outgoing.read()
            sock.sendall(buf)
            # If there's no error, we're done. For WANT_READ, we need to get
            # data from the socket and put it in the incoming BIO.
            if errno is None:
                break
            elif errno == ssl.SSL_ERROR_WANT_READ:
                buf = sock.recv(32768)
                if buf:
                    incoming.write(buf)
                else:
                    incoming.write_eof()
        if support.verbose:
            sys.stdout.write("Needed %d calls to complete %s().\n"
                             % (count, func.__name__))
        return ret

    def test_bio_handshake(self):
        sock = socket.socket(socket.AF_INET)
        self.addCleanup(sock.close)
        sock.connect(self.server_addr)
        incoming = ssl.MemoryBIO()
        outgoing = ssl.MemoryBIO()
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertTrue(ctx.check_hostname)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
        ctx.load_verify_locations(SIGNING_CA)
        sslobj = ctx.wrap_bio(incoming, outgoing, False,
                              SIGNED_CERTFILE_HOSTNAME)
        self.assertIs(sslobj._sslobj.owner, sslobj)
        self.assertIsNone(sslobj.cipher())
        self.assertIsNone(sslobj.version())
        self.assertIsNone(sslobj.shared_ciphers())
        self.assertRaises(ValueError, sslobj.getpeercert)
        # tls-unique is not defined for TLSv1.3
        # https://datatracker.ietf.org/doc/html/rfc8446#appendix-C.5
        if 'tls-unique' in ssl.CHANNEL_BINDING_TYPES and sslobj.version() != "TLSv1.3":
            self.assertIsNone(sslobj.get_channel_binding('tls-unique'))
        self.ssl_io_loop(sock, incoming, outgoing, sslobj.do_handshake)
        self.assertTrue(sslobj.cipher())
        self.assertIsNone(sslobj.shared_ciphers())
        self.assertIsNotNone(sslobj.version())
        self.assertTrue(sslobj.getpeercert())
        if 'tls-unique' in ssl.CHANNEL_BINDING_TYPES and sslobj.version() != "TLSv1.3":
            self.assertTrue(sslobj.get_channel_binding('tls-unique'))
        try:
            self.ssl_io_loop(sock, incoming, outgoing, sslobj.unwrap)
        except ssl.SSLSyscallError:
            # If the server shuts down the TCP connection without sending a
            # secure shutdown message, this is reported as SSL_ERROR_SYSCALL
            pass
        self.assertRaises(ssl.SSLError, sslobj.write, b'foo')

    def test_bio_read_write_data(self):
        sock = socket.socket(socket.AF_INET)
        self.addCleanup(sock.close)
        sock.connect(self.server_addr)
        incoming = ssl.MemoryBIO()
        outgoing = ssl.MemoryBIO()
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        sslobj = ctx.wrap_bio(incoming, outgoing, False)
        self.ssl_io_loop(sock, incoming, outgoing, sslobj.do_handshake)
        req = b'FOO\n'
        self.ssl_io_loop(sock, incoming, outgoing, sslobj.write, req)
        buf = self.ssl_io_loop(sock, incoming, outgoing, sslobj.read, 1024)
        self.assertEqual(buf, b'foo\n')
        self.ssl_io_loop(sock, incoming, outgoing, sslobj.unwrap)

    def test_transport_eof(self):
        client_context, server_context, hostname = testing_context()
        with socket.socket(socket.AF_INET) as sock:
            sock.connect(self.server_addr)
            incoming = ssl.MemoryBIO()
            outgoing = ssl.MemoryBIO()
            sslobj = client_context.wrap_bio(incoming, outgoing,
                                             server_hostname=hostname)
            self.ssl_io_loop(sock, incoming, outgoing, sslobj.do_handshake)

            # Simulate EOF from the transport.
            incoming.write_eof()
            self.assertRaises(ssl.SSLEOFError, sslobj.read)


@support.requires_resource('network')
class NetworkedTests(unittest.TestCase):

    def test_timeout_connect_ex(self):
        # Issue #12065: on a timeout, connect_ex() should return the original
        # errno (mimicking the behaviour of non-SSL sockets).
        with socket_helper.transient_internet(REMOTE_HOST):
            s = test_wrap_socket(socket.socket(socket.AF_INET),
                                cert_reqs=ssl.CERT_REQUIRED,
                                do_handshake_on_connect=False)
            self.addCleanup(s.close)
            s.settimeout(0.0000001)
            rc = s.connect_ex((REMOTE_HOST, 443))
            if rc == 0:
                self.skipTest("REMOTE_HOST responded too quickly")
            elif rc == errno.ENETUNREACH:
                self.skipTest("Network unreachable.")
            self.assertIn(rc, (errno.EAGAIN, errno.EWOULDBLOCK))

    @unittest.skipUnless(socket_helper.IPV6_ENABLED, 'Needs IPv6')
    @support.requires_resource('walltime')
    def test_get_server_certificate_ipv6(self):
        with socket_helper.transient_internet('ipv6.google.com'):
            _test_get_server_certificate(self, 'ipv6.google.com', 443)
            _test_get_server_certificate_fail(self, 'ipv6.google.com', 443)


def _test_get_server_certificate(test, host, port, cert=None):
    pem = ssl.get_server_certificate((host, port))
    if not pem:
        test.fail("No server certificate on %s:%s!" % (host, port))

    pem = ssl.get_server_certificate((host, port), ca_certs=cert)
    if not pem:
        test.fail("No server certificate on %s:%s!" % (host, port))
    if support.verbose:
        sys.stdout.write("\nVerified certificate for %s:%s is\n%s\n" % (host, port ,pem))

def _test_get_server_certificate_fail(test, host, port):
    with warnings_helper.check_no_resource_warning(test):
        try:
            pem = ssl.get_server_certificate((host, port), ca_certs=CERTFILE)
        except ssl.SSLError as x:
            #should fail
            if support.verbose:
                sys.stdout.write("%s\n" % x)
        else:
            test.fail("Got server certificate %s for %s:%s!" % (pem, host, port))


from test.ssl_servers import make_https_server

class ThreadedEchoServer(threading.Thread):

    class ConnectionHandler(threading.Thread):

        """A mildly complicated class, because we want it to work both
        with and without the SSL wrapper around the socket connection, so
        that we can test the STARTTLS functionality."""

        def __init__(self, server, connsock, addr):
            self.server = server
            self.running = False
            self.sock = connsock
            self.addr = addr
            self.sock.setblocking(True)
            self.sslconn = None
            threading.Thread.__init__(self)
            self.daemon = True

        def wrap_conn(self):
            try:
                self.sslconn = self.server.context.wrap_socket(
                    self.sock, server_side=True)
                self.server.selected_alpn_protocols.append(self.sslconn.selected_alpn_protocol())
            except (ConnectionResetError, BrokenPipeError, ConnectionAbortedError) as e:
                # We treat ConnectionResetError as though it were an
                # SSLError - OpenSSL on Ubuntu abruptly closes the
                # connection when asked to use an unsupported protocol.
                #
                # BrokenPipeError is raised in TLS 1.3 mode, when OpenSSL
                # tries to send session tickets after handshake.
                # https://github.com/openssl/openssl/issues/6342
                #
                # ConnectionAbortedError is raised in TLS 1.3 mode, when OpenSSL
                # tries to send session tickets after handshake when using WinSock.
                self.server.conn_errors.append(str(e))
                if self.server.chatty:
                    handle_error("\n server:  bad connection attempt from " + repr(self.addr) + ":\n")
                self.running = False
                self.close()
                return False
            except (ssl.SSLError, OSError) as e:
                # OSError may occur with wrong protocols, e.g. both
                # sides use PROTOCOL_TLS_SERVER.
                #
                # XXX Various errors can have happened here, for example
                # a mismatching protocol version, an invalid certificate,
                # or a low-level bug. This should be made more discriminating.
                #
                # bpo-31323: Store the exception as string to prevent
                # a reference leak: server -> conn_errors -> exception
                # -> traceback -> self (ConnectionHandler) -> server
                self.server.conn_errors.append(str(e))
                if self.server.chatty:
                    handle_error("\n server:  bad connection attempt from " + repr(self.addr) + ":\n")

                # bpo-44229, bpo-43855, bpo-44237, and bpo-33450:
                # Ignore spurious EPROTOTYPE returned by write() on macOS.
                # See also http://erickt.github.io/blog/2014/11/19/adventures-in-debugging-a-potential-osx-kernel-bug/
                if e.errno != errno.EPROTOTYPE and sys.platform != "darwin":
                    self.running = False
                    self.close()
                return False
            else:
                self.server.shared_ciphers.append(self.sslconn.shared_ciphers())
                if self.server.context.verify_mode == ssl.CERT_REQUIRED:
                    cert = self.sslconn.getpeercert()
                    if support.verbose and self.server.chatty:
                        sys.stdout.write(" client cert is " + pprint.pformat(cert) + "\n")
                    cert_binary = self.sslconn.getpeercert(True)
                    if support.verbose and self.server.chatty:
                        if cert_binary is None:
                            sys.stdout.write(" client did not provide a cert\n")
                        else:
                            sys.stdout.write(f" cert binary is {len(cert_binary)}b\n")
                cipher = self.sslconn.cipher()
                if support.verbose and self.server.chatty:
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

        def run(self):
            self.running = True
            if not self.server.starttls_server:
                if not self.wrap_conn():
                    return
            while self.running:
                try:
                    msg = self.read()
                    stripped = msg.strip()
                    if not stripped:
                        # eof, so quit this handler
                        self.running = False
                        try:
                            self.sock = self.sslconn.unwrap()
                        except OSError:
                            # Many tests shut the TCP connection down
                            # without an SSL shutdown. This causes
                            # unwrap() to raise OSError with errno=0!
                            pass
                        else:
                            self.sslconn = None
                        self.close()
                    elif stripped == b'over':
                        if support.verbose and self.server.connectionchatty:
                            sys.stdout.write(" server: client closed connection\n")
                        self.close()
                        return
                    elif (self.server.starttls_server and
                          stripped == b'STARTTLS'):
                        if support.verbose and self.server.connectionchatty:
                            sys.stdout.write(" server: read STARTTLS from client, sending OK...\n")
                        self.write(b"OK\n")
                        if not self.wrap_conn():
                            return
                    elif (self.server.starttls_server and self.sslconn
                          and stripped == b'ENDTLS'):
                        if support.verbose and self.server.connectionchatty:
                            sys.stdout.write(" server: read ENDTLS from client, sending OK...\n")
                        self.write(b"OK\n")
                        self.sock = self.sslconn.unwrap()
                        self.sslconn = None
                        if support.verbose and self.server.connectionchatty:
                            sys.stdout.write(" server: connection is now unencrypted...\n")
                    elif stripped == b'CB tls-unique':
                        if support.verbose and self.server.connectionchatty:
                            sys.stdout.write(" server: read CB tls-unique from client, sending our CB data...\n")
                        data = self.sslconn.get_channel_binding("tls-unique")
                        self.write(repr(data).encode("us-ascii") + b"\n")
                    elif stripped == b'PHA':
                        if support.verbose and self.server.connectionchatty:
                            sys.stdout.write(" server: initiating post handshake auth\n")
                        try:
                            self.sslconn.verify_client_post_handshake()
                        except ssl.SSLError as e:
                            self.write(repr(e).encode("us-ascii") + b"\n")
                        else:
                            self.write(b"OK\n")
                    elif stripped == b'HASCERT':
                        if self.sslconn.getpeercert() is not None:
                            self.write(b'TRUE\n')
                        else:
                            self.write(b'FALSE\n')
                    elif stripped == b'GETCERT':
                        cert = self.sslconn.getpeercert()
                        self.write(repr(cert).encode("us-ascii") + b"\n")
                    elif stripped == b'VERIFIEDCHAIN':
                        certs = self.sslconn._sslobj.get_verified_chain()
                        self.write(len(certs).to_bytes(1, "big") + b"\n")
                    elif stripped == b'UNVERIFIEDCHAIN':
                        certs = self.sslconn._sslobj.get_unverified_chain()
                        self.write(len(certs).to_bytes(1, "big") + b"\n")
                    else:
                        if (support.verbose and
                            self.server.connectionchatty):
                            ctype = (self.sslconn and "encrypted") or "unencrypted"
                            sys.stdout.write(" server: read %r (%s), sending back %r (%s)...\n"
                                             % (msg, ctype, msg.lower(), ctype))
                        self.write(msg.lower())
                except OSError as e:
                    # handles SSLError and socket errors
                    if isinstance(e, ConnectionError):
                        # OpenSSL 1.1.1 sometimes raises
                        # ConnectionResetError when connection is not
                        # shut down gracefully.
                        if self.server.chatty and support.verbose:
                            print(f" Connection reset by peer: {self.addr}")

                        self.close()
                        self.running = False
                        return
                    if self.server.chatty and support.verbose:
                        handle_error("Test server failure:\n")
                    try:
                        self.write(b"ERROR\n")
                    except OSError:
                        pass
                    self.close()
                    self.running = False

    def __init__(self, certificate=None, ssl_version=None,
                 certreqs=None, cacerts=None,
                 chatty=True, connectionchatty=False, starttls_server=False,
                 alpn_protocols=None,
                 ciphers=None, context=None):
        if context:
            self.context = context
        else:
            self.context = ssl.SSLContext(ssl_version
                                          if ssl_version is not None
                                          else ssl.PROTOCOL_TLS_SERVER)
            self.context.verify_mode = (certreqs if certreqs is not None
                                        else ssl.CERT_NONE)
            if cacerts:
                self.context.load_verify_locations(cacerts)
            if certificate:
                self.context.load_cert_chain(certificate)
            if alpn_protocols:
                self.context.set_alpn_protocols(alpn_protocols)
            if ciphers:
                self.context.set_ciphers(ciphers)
        self.chatty = chatty
        self.connectionchatty = connectionchatty
        self.starttls_server = starttls_server
        self.sock = socket.socket()
        self.port = socket_helper.bind_port(self.sock)
        self.flag = None
        self.active = False
        self.selected_alpn_protocols = []
        self.shared_ciphers = []
        self.conn_errors = []
        threading.Thread.__init__(self)
        self.daemon = True
        self._in_context = False

    def __enter__(self):
        if self._in_context:
            raise ValueError('Re-entering ThreadedEchoServer context')
        self._in_context = True
        self.start(threading.Event())
        self.flag.wait()
        return self

    def __exit__(self, *args):
        assert self._in_context
        self._in_context = False
        self.stop()
        self.join()

    def start(self, flag=None):
        if not self._in_context:
            raise ValueError(
                'ThreadedEchoServer must be used as a context manager')
        self.flag = flag
        threading.Thread.start(self)

    def run(self):
        if not self._in_context:
            raise ValueError(
                'ThreadedEchoServer must be used as a context manager')
        self.sock.settimeout(1.0)
        self.sock.listen(5)
        self.active = True
        if self.flag:
            # signal an event
            self.flag.set()
        while self.active:
            try:
                newconn, connaddr = self.sock.accept()
                if support.verbose and self.chatty:
                    sys.stdout.write(' server:  new connection from '
                                     + repr(connaddr) + '\n')
                handler = self.ConnectionHandler(self, newconn, connaddr)
                handler.start()
                handler.join()
            except TimeoutError as e:
                if support.verbose:
                    sys.stdout.write(f' connection timeout {e!r}\n')
            except KeyboardInterrupt:
                self.stop()
            except BaseException as e:
                if support.verbose and self.chatty:
                    sys.stdout.write(
                        ' connection handling failed: ' + repr(e) + '\n')

        self.close()

    def close(self):
        if self.sock is not None:
            self.sock.close()
            self.sock = None

    def stop(self):
        self.active = False

class AsyncoreEchoServer(threading.Thread):

    # this one's based on asyncore.dispatcher

    class EchoServer (asyncore.dispatcher):

        class ConnectionHandler(asyncore.dispatcher_with_send):

            def __init__(self, conn, certfile):
                self.socket = test_wrap_socket(conn, server_side=True,
                                              certfile=certfile,
                                              do_handshake_on_connect=False)
                asyncore.dispatcher_with_send.__init__(self, self.socket)
                self._ssl_accepting = True
                self._do_ssl_handshake()

            def readable(self):
                if isinstance(self.socket, ssl.SSLSocket):
                    while self.socket.pending() > 0:
                        self.handle_read_event()
                return True

            def _do_ssl_handshake(self):
                try:
                    self.socket.do_handshake()
                except (ssl.SSLWantReadError, ssl.SSLWantWriteError):
                    return
                except ssl.SSLEOFError:
                    return self.handle_close()
                except ssl.SSLError:
                    raise
                except OSError as err:
                    if err.args[0] == errno.ECONNABORTED:
                        return self.handle_close()
                else:
                    self._ssl_accepting = False

            def handle_read(self):
                if self._ssl_accepting:
                    self._do_ssl_handshake()
                else:
                    data = self.recv(1024)
                    if support.verbose:
                        sys.stdout.write(" server:  read %s from client\n" % repr(data))
                    if not data:
                        self.close()
                    else:
                        self.send(data.lower())

            def handle_close(self):
                self.close()
                if support.verbose:
                    sys.stdout.write(" server:  closed connection %s\n" % self.socket)

            def handle_error(self):
                raise

        def __init__(self, certfile):
            self.certfile = certfile
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.port = socket_helper.bind_port(sock, '')
            asyncore.dispatcher.__init__(self, sock)
            self.listen(5)

        def handle_accepted(self, sock_obj, addr):
            if support.verbose:
                sys.stdout.write(" server:  new connection from %s:%s\n" %addr)
            self.ConnectionHandler(sock_obj, self.certfile)

        def handle_error(self):
            raise

    def __init__(self, certfile):
        self.flag = None
        self.active = False
        self.server = self.EchoServer(certfile)
        self.port = self.server.port
        threading.Thread.__init__(self)
        self.daemon = True

    def __str__(self):
        return "<%s %s>" % (self.__class__.__name__, self.server)

    def __enter__(self):
        self.start(threading.Event())
        self.flag.wait()
        return self

    def __exit__(self, *args):
        if support.verbose:
            sys.stdout.write(" cleanup: stopping server.\n")
        self.stop()
        if support.verbose:
            sys.stdout.write(" cleanup: joining server thread.\n")
        self.join()
        if support.verbose:
            sys.stdout.write(" cleanup: successfully joined.\n")
        # make sure that ConnectionHandler is removed from socket_map
        asyncore.close_all(ignore_all=True)

    def start (self, flag=None):
        self.flag = flag
        threading.Thread.start(self)

    def run(self):
        self.active = True
        if self.flag:
            self.flag.set()
        while self.active:
            try:
                asyncore.loop(1)
            except:
                pass

    def stop(self):
        self.active = False
        self.server.close()

def server_params_test(client_context, server_context, indata=b"FOO\n",
                       chatty=True, connectionchatty=False, sni_name=None,
                       session=None):
    """
    Launch a server, connect a client to it and try various reads
    and writes.
    """
    stats = {}
    server = ThreadedEchoServer(context=server_context,
                                chatty=chatty,
                                connectionchatty=False)
    with server:
        with client_context.wrap_socket(socket.socket(),
                server_hostname=sni_name, session=session) as s:
            s.connect((HOST, server.port))
            for arg in [indata, bytearray(indata), memoryview(indata)]:
                if connectionchatty:
                    if support.verbose:
                        sys.stdout.write(
                            " client:  sending %r...\n" % indata)
                s.write(arg)
                outdata = s.read()
                if connectionchatty:
                    if support.verbose:
                        sys.stdout.write(" client:  read %r\n" % outdata)
                if outdata != indata.lower():
                    raise AssertionError(
                        "bad data <<%r>> (%d) received; expected <<%r>> (%d)\n"
                        % (outdata[:20], len(outdata),
                           indata[:20].lower(), len(indata)))
            s.write(b"over\n")
            if connectionchatty:
                if support.verbose:
                    sys.stdout.write(" client:  closing connection.\n")
            stats.update({
                'compression': s.compression(),
                'cipher': s.cipher(),
                'peercert': s.getpeercert(),
                'client_alpn_protocol': s.selected_alpn_protocol(),
                'version': s.version(),
                'session_reused': s.session_reused,
                'session': s.session,
            })
            s.close()
        stats['server_alpn_protocols'] = server.selected_alpn_protocols
        stats['server_shared_ciphers'] = server.shared_ciphers
    return stats

def try_protocol_combo(server_protocol, client_protocol, expect_success,
                       certsreqs=None, server_options=0, client_options=0):
    """
    Try to SSL-connect using *client_protocol* to *server_protocol*.
    If *expect_success* is true, assert that the connection succeeds,
    if it's false, assert that the connection fails.
    Also, if *expect_success* is a string, assert that it is the protocol
    version actually used by the connection.
    """
    if certsreqs is None:
        certsreqs = ssl.CERT_NONE
    certtype = {
        ssl.CERT_NONE: "CERT_NONE",
        ssl.CERT_OPTIONAL: "CERT_OPTIONAL",
        ssl.CERT_REQUIRED: "CERT_REQUIRED",
    }[certsreqs]
    if support.verbose:
        formatstr = (expect_success and " %s->%s %s\n") or " {%s->%s} %s\n"
        sys.stdout.write(formatstr %
                         (ssl.get_protocol_name(client_protocol),
                          ssl.get_protocol_name(server_protocol),
                          certtype))

    with warnings_helper.check_warnings():
        # ignore Deprecation warnings
        client_context = ssl.SSLContext(client_protocol)
        client_context.options |= client_options
        server_context = ssl.SSLContext(server_protocol)
        server_context.options |= server_options

    min_version = PROTOCOL_TO_TLS_VERSION.get(client_protocol, None)
    if (min_version is not None
        # SSLContext.minimum_version is only available on recent OpenSSL
        # (setter added in OpenSSL 1.1.0, getter added in OpenSSL 1.1.1)
        and hasattr(server_context, 'minimum_version')
        and server_protocol == ssl.PROTOCOL_TLS
        and server_context.minimum_version > min_version
    ):
        # If OpenSSL configuration is strict and requires more recent TLS
        # version, we have to change the minimum to test old TLS versions.
        with warnings_helper.check_warnings():
            server_context.minimum_version = min_version

    # NOTE: we must enable "ALL" ciphers on the client, otherwise an
    # SSLv23 client will send an SSLv3 hello (rather than SSLv2)
    # starting from OpenSSL 1.0.0 (see issue #8322).
    if client_context.protocol == ssl.PROTOCOL_TLS:
        client_context.set_ciphers("ALL")

    seclevel_workaround(server_context, client_context)

    for ctx in (client_context, server_context):
        ctx.verify_mode = certsreqs
        ctx.load_cert_chain(SIGNED_CERTFILE)
        ctx.load_verify_locations(SIGNING_CA)
    try:
        stats = server_params_test(client_context, server_context,
                                   chatty=False, connectionchatty=False)
    # Protocol mismatch can result in either an SSLError, or a
    # "Connection reset by peer" error.
    except ssl.SSLError:
        if expect_success:
            raise
    except OSError as e:
        if expect_success or e.errno != errno.ECONNRESET:
            raise
    else:
        if not expect_success:
            raise AssertionError(
                "Client protocol %s succeeded with server protocol %s!"
                % (ssl.get_protocol_name(client_protocol),
                   ssl.get_protocol_name(server_protocol)))
        elif (expect_success is not True
              and expect_success != stats['version']):
            raise AssertionError("version mismatch: expected %r, got %r"
                                 % (expect_success, stats['version']))


def supports_kx_alias(ctx, aliases):
    for cipher in ctx.get_ciphers():
        for alias in aliases:
            if f"Kx={alias}" in cipher['description']:
                return True
    return False


class ThreadedTests(unittest.TestCase):

    @support.requires_resource('walltime')
    def test_echo(self):
        """Basic test of an SSL client connecting to a server"""
        if support.verbose:
            sys.stdout.write("\n")

        client_context, server_context, hostname = testing_context()

        with self.subTest(client=ssl.PROTOCOL_TLS_CLIENT, server=ssl.PROTOCOL_TLS_SERVER):
            server_params_test(client_context=client_context,
                               server_context=server_context,
                               chatty=True, connectionchatty=True,
                               sni_name=hostname)

        client_context.check_hostname = False
        with self.subTest(client=ssl.PROTOCOL_TLS_SERVER, server=ssl.PROTOCOL_TLS_CLIENT):
            with self.assertRaises(ssl.SSLError) as e:
                server_params_test(client_context=server_context,
                                   server_context=client_context,
                                   chatty=True, connectionchatty=True,
                                   sni_name=hostname)
            self.assertIn(
                'Cannot create a client socket with a PROTOCOL_TLS_SERVER context',
                str(e.exception)
            )

        with self.subTest(client=ssl.PROTOCOL_TLS_SERVER, server=ssl.PROTOCOL_TLS_SERVER):
            with self.assertRaises(ssl.SSLError) as e:
                server_params_test(client_context=server_context,
                                   server_context=server_context,
                                   chatty=True, connectionchatty=True)
            self.assertIn(
                'Cannot create a client socket with a PROTOCOL_TLS_SERVER context',
                str(e.exception)
            )

        with self.subTest(client=ssl.PROTOCOL_TLS_CLIENT, server=ssl.PROTOCOL_TLS_CLIENT):
            with self.assertRaises(ssl.SSLError) as e:
                server_params_test(client_context=server_context,
                                   server_context=client_context,
                                   chatty=True, connectionchatty=True)
            self.assertIn(
                'Cannot create a client socket with a PROTOCOL_TLS_SERVER context',
                str(e.exception))

    @unittest.skipUnless(support.Py_GIL_DISABLED, "test is only useful if the GIL is disabled")
    def test_ssl_in_multiple_threads(self):
        # See GH-124984: OpenSSL is not thread safe.
        threads = []

        warnings_filters = sys.flags.context_aware_warnings
        global USE_SAME_TEST_CONTEXT
        USE_SAME_TEST_CONTEXT = True
        try:
            for func in (
                self.test_echo,
                self.test_alpn_protocols,
                self.test_getpeercert,
                self.test_crl_check,
                functools.partial(
                    self.test_check_hostname_idn,
                    warnings_filters=warnings_filters,
                ),
                self.test_wrong_cert_tls12,
                self.test_wrong_cert_tls13,
            ):
                # Be careful with the number of threads here.
                # Too many can result in failing tests.
                for num in range(5):
                    with self.subTest(func=func, num=num):
                        threads.append(Thread(target=func))

            with threading_helper.catch_threading_exception() as cm:
                for thread in threads:
                    with self.subTest(thread=thread):
                        thread.start()

                for thread in threads:
                    with self.subTest(thread=thread):
                        thread.join()
                if cm.exc_value is not None:
                    # Some threads can skip their test
                    if not isinstance(cm.exc_value, unittest.SkipTest):
                        raise cm.exc_value
        finally:
            USE_SAME_TEST_CONTEXT = False

    def test_getpeercert(self):
        if support.verbose:
            sys.stdout.write("\n")

        client_context, server_context, hostname = testing_context()
        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            do_handshake_on_connect=False,
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                # getpeercert() raise ValueError while the handshake isn't
                # done.
                with self.assertRaises(ValueError):
                    s.getpeercert()
                s.do_handshake()
                cert = s.getpeercert()
                self.assertTrue(cert, "Can't get peer certificate.")
                cipher = s.cipher()
                if support.verbose:
                    sys.stdout.write(pprint.pformat(cert) + '\n')
                    sys.stdout.write("Connection cipher is " + str(cipher) + '.\n')
                if 'subject' not in cert:
                    self.fail("No subject field in certificate: %s." %
                              pprint.pformat(cert))
                if ((('organizationName', 'Python Software Foundation'),)
                    not in cert['subject']):
                    self.fail(
                        "Missing or invalid 'organizationName' field in certificate subject; "
                        "should be 'Python Software Foundation'.")
                self.assertIn('notBefore', cert)
                self.assertIn('notAfter', cert)
                before = ssl.cert_time_to_seconds(cert['notBefore'])
                after = ssl.cert_time_to_seconds(cert['notAfter'])
                self.assertLess(before, after)

    def test_crl_check(self):
        if support.verbose:
            sys.stdout.write("\n")

        client_context, server_context, hostname = testing_context()

        tf = getattr(ssl, "VERIFY_X509_TRUSTED_FIRST", 0)
        self.assertEqual(client_context.verify_flags, ssl.VERIFY_DEFAULT | tf)

        # VERIFY_DEFAULT should pass
        server = ThreadedEchoServer(context=server_context, chatty=True)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                cert = s.getpeercert()
                self.assertTrue(cert, "Can't get peer certificate.")

        # VERIFY_CRL_CHECK_LEAF without a loaded CRL file fails
        client_context.verify_flags |= ssl.VERIFY_CRL_CHECK_LEAF

        server = ThreadedEchoServer(context=server_context, chatty=True)
        # Allow for flexible libssl error messages.
        regex = re.compile(r"""(
            certificate verify failed   # OpenSSL
            |
            CERTIFICATE_VERIFY_FAILED   # AWS-LC
        )""", re.X)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                with self.assertRaisesRegex(ssl.SSLError, regex):
                    s.connect((HOST, server.port))

        # now load a CRL file. The CRL file is signed by the CA.
        client_context.load_verify_locations(CRLFILE)

        server = ThreadedEchoServer(context=server_context, chatty=True)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                cert = s.getpeercert()
                self.assertTrue(cert, "Can't get peer certificate.")

    def test_check_hostname(self):
        if support.verbose:
            sys.stdout.write("\n")

        client_context, server_context, hostname = testing_context()

        # correct hostname should verify
        server = ThreadedEchoServer(context=server_context, chatty=True)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                cert = s.getpeercert()
                self.assertTrue(cert, "Can't get peer certificate.")

        # incorrect hostname should raise an exception
        server = ThreadedEchoServer(context=server_context, chatty=True)
        # Allow for flexible libssl error messages.
        regex = re.compile(r"""(
            certificate verify failed   # OpenSSL
            |
            CERTIFICATE_VERIFY_FAILED   # AWS-LC
        )""", re.X)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname="invalid") as s:
                with self.assertRaisesRegex(ssl.CertificateError, regex):
                    s.connect((HOST, server.port))

        # missing server_hostname arg should cause an exception, too
        server = ThreadedEchoServer(context=server_context, chatty=True)
        with server:
            with socket.socket() as s:
                with self.assertRaisesRegex(ValueError,
                                            "check_hostname requires server_hostname"):
                    client_context.wrap_socket(s)

    @unittest.skipUnless(
        ssl.HAS_NEVER_CHECK_COMMON_NAME, "test requires hostname_checks_common_name"
    )
    def test_hostname_checks_common_name(self):
        client_context, server_context, hostname = testing_context()
        assert client_context.hostname_checks_common_name
        client_context.hostname_checks_common_name = False

        # default cert has a SAN
        server = ThreadedEchoServer(context=server_context, chatty=True)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))

        client_context, server_context, hostname = testing_context(NOSANFILE)
        client_context.hostname_checks_common_name = False
        server = ThreadedEchoServer(context=server_context, chatty=True)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                with self.assertRaises(ssl.SSLCertVerificationError):
                    s.connect((HOST, server.port))

    def test_ecc_cert(self):
        client_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        client_context.load_verify_locations(SIGNING_CA)
        client_context.set_ciphers('ECDHE:ECDSA:!NULL:!aRSA')
        hostname = SIGNED_CERTFILE_ECC_HOSTNAME

        server_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        # load ECC cert
        server_context.load_cert_chain(SIGNED_CERTFILE_ECC)

        # correct hostname should verify
        server = ThreadedEchoServer(context=server_context, chatty=True)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                cert = s.getpeercert()
                self.assertTrue(cert, "Can't get peer certificate.")
                cipher = s.cipher()[0].split('-')
                self.assertTrue(cipher[:2], ('ECDHE', 'ECDSA'))

    @unittest.skipUnless(IS_OPENSSL_3_0_0,
                         "test requires RFC 5280 check added in OpenSSL 3.0+")
    def test_verify_strict(self):
        # verification fails by default, since the server cert is non-conforming
        client_context = ssl.create_default_context()
        client_context.load_verify_locations(LEAF_MISSING_AKI_CA)
        hostname = LEAF_MISSING_AKI_CERTFILE_HOSTNAME

        server_context = ssl.create_default_context(purpose=Purpose.CLIENT_AUTH)
        server_context.load_cert_chain(LEAF_MISSING_AKI_CERTFILE)
        server = ThreadedEchoServer(context=server_context, chatty=True)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                with self.assertRaises(ssl.SSLError):
                    s.connect((HOST, server.port))

        # explicitly disabling VERIFY_X509_STRICT allows it to succeed
        client_context = ssl.create_default_context()
        client_context.load_verify_locations(LEAF_MISSING_AKI_CA)
        client_context.verify_flags &= ~ssl.VERIFY_X509_STRICT

        server_context = ssl.create_default_context(purpose=Purpose.CLIENT_AUTH)
        server_context.load_cert_chain(LEAF_MISSING_AKI_CERTFILE)
        server = ThreadedEchoServer(context=server_context, chatty=True)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                cert = s.getpeercert()
                self.assertTrue(cert, "Can't get peer certificate.")

    def test_dual_rsa_ecc(self):
        client_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        client_context.load_verify_locations(SIGNING_CA)
        # TODO: fix TLSv1.3 once SSLContext can restrict signature
        #       algorithms.
        client_context.maximum_version = ssl.TLSVersion.TLSv1_2
        # only ECDSA certs
        client_context.set_ciphers('ECDHE:ECDSA:!NULL:!aRSA')
        hostname = SIGNED_CERTFILE_ECC_HOSTNAME

        server_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        # load ECC and RSA key/cert pairs
        server_context.load_cert_chain(SIGNED_CERTFILE_ECC)
        server_context.load_cert_chain(SIGNED_CERTFILE)

        # correct hostname should verify
        server = ThreadedEchoServer(context=server_context, chatty=True)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                cert = s.getpeercert()
                self.assertTrue(cert, "Can't get peer certificate.")
                cipher = s.cipher()[0].split('-')
                self.assertTrue(cipher[:2], ('ECDHE', 'ECDSA'))

    def test_check_hostname_idn(self, warnings_filters=True):
        if support.verbose:
            sys.stdout.write("\n")

        server_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        server_context.load_cert_chain(IDNSANSFILE)

        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        context.verify_mode = ssl.CERT_REQUIRED
        context.check_hostname = True
        context.load_verify_locations(SIGNING_CA)

        # correct hostname should verify, when specified in several
        # different ways
        idn_hostnames = [
            ('könig.idn.pythontest.net',
             'xn--knig-5qa.idn.pythontest.net'),
            ('xn--knig-5qa.idn.pythontest.net',
             'xn--knig-5qa.idn.pythontest.net'),
            (b'xn--knig-5qa.idn.pythontest.net',
             'xn--knig-5qa.idn.pythontest.net'),

            ('königsgäßchen.idna2003.pythontest.net',
             'xn--knigsgsschen-lcb0w.idna2003.pythontest.net'),
            ('xn--knigsgsschen-lcb0w.idna2003.pythontest.net',
             'xn--knigsgsschen-lcb0w.idna2003.pythontest.net'),
            (b'xn--knigsgsschen-lcb0w.idna2003.pythontest.net',
             'xn--knigsgsschen-lcb0w.idna2003.pythontest.net'),

            # ('königsgäßchen.idna2008.pythontest.net',
            #  'xn--knigsgchen-b4a3dun.idna2008.pythontest.net'),
            ('xn--knigsgchen-b4a3dun.idna2008.pythontest.net',
             'xn--knigsgchen-b4a3dun.idna2008.pythontest.net'),
            (b'xn--knigsgchen-b4a3dun.idna2008.pythontest.net',
             'xn--knigsgchen-b4a3dun.idna2008.pythontest.net'),

        ]
        for server_hostname, expected_hostname in idn_hostnames:
            server = ThreadedEchoServer(context=server_context, chatty=True)
            with server:
                with context.wrap_socket(socket.socket(),
                                         server_hostname=server_hostname) as s:
                    self.assertEqual(s.server_hostname, expected_hostname)
                    s.connect((HOST, server.port))
                    cert = s.getpeercert()
                    self.assertEqual(s.server_hostname, expected_hostname)
                    self.assertTrue(cert, "Can't get peer certificate.")

        # incorrect hostname should raise an exception
        server = ThreadedEchoServer(context=server_context, chatty=True)
        with server:
            with context.wrap_socket(socket.socket(),
                                     server_hostname="python.example.org") as s:
                with self.assertRaises(ssl.CertificateError):
                    s.connect((HOST, server.port))
        with (
            ThreadedEchoServer(context=server_context, chatty=True) as server,
            (
                warnings_helper.check_no_resource_warning(self)
                if warnings_filters
                else nullcontext()
            ),
            self.assertRaises(UnicodeError),
        ):
            context.wrap_socket(socket.socket(), server_hostname='.pythontest.net')

        with (
            ThreadedEchoServer(context=server_context, chatty=True) as server,
            (
                warnings_helper.check_no_resource_warning(self)
                if warnings_filters
                else nullcontext()
            ),
            self.assertRaises(UnicodeDecodeError),
        ):
            context.wrap_socket(
                socket.socket(),
                server_hostname=b'k\xf6nig.idn.pythontest.net',
            )

    def test_wrong_cert_tls12(self):
        """Connecting when the server rejects the client's certificate

        Launch a server with CERT_REQUIRED, and check that trying to
        connect to it with a wrong client certificate fails.
        """
        client_context, server_context, hostname = testing_context()
        # load client cert that is not signed by trusted CA
        client_context.load_cert_chain(CERTFILE)
        # require TLS client authentication
        server_context.verify_mode = ssl.CERT_REQUIRED
        # TLS 1.3 has different handshake
        client_context.maximum_version = ssl.TLSVersion.TLSv1_2

        server = ThreadedEchoServer(
            context=server_context, chatty=True, connectionchatty=True,
        )

        with server, \
                client_context.wrap_socket(socket.socket(),
                                           server_hostname=hostname) as s:
            try:
                # Expect either an SSL error about the server rejecting
                # the connection, or a low-level connection reset (which
                # sometimes happens on Windows)
                s.connect((HOST, server.port))
            except ssl.SSLError as e:
                if support.verbose:
                    sys.stdout.write("\nSSLError is %r\n" % e)
            except OSError as e:
                if e.errno != errno.ECONNRESET:
                    raise
                if support.verbose:
                    sys.stdout.write("\nsocket.error is %r\n" % e)
            else:
                self.fail("Use of invalid cert should have failed!")

    @requires_tls_version('TLSv1_3')
    def test_wrong_cert_tls13(self):
        client_context, server_context, hostname = testing_context()
        # load client cert that is not signed by trusted CA
        client_context.load_cert_chain(CERTFILE)
        server_context.verify_mode = ssl.CERT_REQUIRED
        server_context.minimum_version = ssl.TLSVersion.TLSv1_3
        client_context.minimum_version = ssl.TLSVersion.TLSv1_3

        server = ThreadedEchoServer(
            context=server_context, chatty=True, connectionchatty=True,
        )
        with server, \
             client_context.wrap_socket(socket.socket(),
                                        server_hostname=hostname,
                                        suppress_ragged_eofs=False) as s:
            s.connect((HOST, server.port))
            with self.assertRaisesRegex(
                OSError,
                'alert unknown ca|EOF occurred|TLSV1_ALERT_UNKNOWN_CA|'
                'closed by the remote host|Connection reset by peer|'
                'Broken pipe'
            ):
                # TLS 1.3 perform client cert exchange after handshake
                s.write(b'data')
                s.read(1000)
                s.write(b'should have failed already')
                s.read(1000)

    def test_rude_shutdown(self):
        """A brutal shutdown of an SSL server should raise an OSError
        in the client when attempting handshake.
        """
        listener_ready = threading.Event()
        listener_gone = threading.Event()

        s = socket.socket()
        port = socket_helper.bind_port(s, HOST)

        # `listener` runs in a thread.  It sits in an accept() until
        # the main thread connects.  Then it rudely closes the socket,
        # and sets Event `listener_gone` to let the main thread know
        # the socket is gone.
        def listener():
            s.listen()
            listener_ready.set()
            newsock, addr = s.accept()
            newsock.close()
            s.close()
            listener_gone.set()

        def connector():
            listener_ready.wait()
            with socket.socket() as c:
                c.connect((HOST, port))
                listener_gone.wait()
                try:
                    ssl_sock = test_wrap_socket(c)
                except OSError:
                    pass
                else:
                    self.fail('connecting to closed SSL socket should have failed')

        t = threading.Thread(target=listener)
        t.start()
        try:
            connector()
        finally:
            t.join()

    def test_ssl_cert_verify_error(self):
        if support.verbose:
            sys.stdout.write("\n")

        server_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        server_context.load_cert_chain(SIGNED_CERTFILE)

        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)

        server = ThreadedEchoServer(context=server_context, chatty=True)
        with server:
            with context.wrap_socket(socket.socket(),
                                     server_hostname=SIGNED_CERTFILE_HOSTNAME) as s:
                try:
                    s.connect((HOST, server.port))
                    self.fail("Expected connection failure")
                except ssl.SSLError as e:
                    msg = 'unable to get local issuer certificate'
                    self.assertIsInstance(e, ssl.SSLCertVerificationError)
                    self.assertEqual(e.verify_code, 20)
                    self.assertEqual(e.verify_message, msg)
                    # Allow for flexible libssl error messages.
                    regex = f"({msg}|CERTIFICATE_VERIFY_FAILED)"
                    self.assertRegex(repr(e), regex)
                    regex = re.compile(r"""(
                        certificate verify failed   # OpenSSL
                        |
                        CERTIFICATE_VERIFY_FAILED   # AWS-LC
                    )""", re.X)
                    self.assertRegex(repr(e), regex)

    def test_PROTOCOL_TLS(self):
        """Connecting to an SSLv23 server with various client options"""
        if support.verbose:
            sys.stdout.write("\n")
        if has_tls_version('SSLv3'):
            try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_SSLv3, False)
        try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_TLS, True)
        if has_tls_version('TLSv1'):
            try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_TLSv1, 'TLSv1')

        if has_tls_version('SSLv3'):
            try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_SSLv3, False, ssl.CERT_OPTIONAL)
        try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_TLS, True, ssl.CERT_OPTIONAL)
        if has_tls_version('TLSv1'):
            try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_TLSv1, 'TLSv1', ssl.CERT_OPTIONAL)

        if has_tls_version('SSLv3'):
            try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_SSLv3, False, ssl.CERT_REQUIRED)
        try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_TLS, True, ssl.CERT_REQUIRED)
        if has_tls_version('TLSv1'):
            try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_TLSv1, 'TLSv1', ssl.CERT_REQUIRED)

        # Server with specific SSL options
        if has_tls_version('SSLv3'):
            try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_SSLv3, False,
                           server_options=ssl.OP_NO_SSLv3)
        # Will choose TLSv1
        try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_TLS, True,
                           server_options=ssl.OP_NO_SSLv2 | ssl.OP_NO_SSLv3)
        if has_tls_version('TLSv1'):
            try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_TLSv1, False,
                               server_options=ssl.OP_NO_TLSv1)

    @requires_tls_version('SSLv3')
    def test_protocol_sslv3(self):
        """Connecting to an SSLv3 server with various client options"""
        if support.verbose:
            sys.stdout.write("\n")
        try_protocol_combo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv3, 'SSLv3')
        try_protocol_combo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv3, 'SSLv3', ssl.CERT_OPTIONAL)
        try_protocol_combo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv3, 'SSLv3', ssl.CERT_REQUIRED)
        try_protocol_combo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_TLS, False,
                           client_options=ssl.OP_NO_SSLv3)
        try_protocol_combo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_TLSv1, False)

    @requires_tls_version('TLSv1')
    def test_protocol_tlsv1(self):
        """Connecting to a TLSv1 server with various client options"""
        if support.verbose:
            sys.stdout.write("\n")
        try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_TLSv1, 'TLSv1')
        try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_TLSv1, 'TLSv1', ssl.CERT_OPTIONAL)
        try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_TLSv1, 'TLSv1', ssl.CERT_REQUIRED)
        if has_tls_version('SSLv3'):
            try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_SSLv3, False)
        try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_TLS, False,
                           client_options=ssl.OP_NO_TLSv1)

    @requires_tls_version('TLSv1_1')
    def test_protocol_tlsv1_1(self):
        """Connecting to a TLSv1.1 server with various client options.
           Testing against older TLS versions."""
        if support.verbose:
            sys.stdout.write("\n")
        try_protocol_combo(ssl.PROTOCOL_TLSv1_1, ssl.PROTOCOL_TLSv1_1, 'TLSv1.1')
        if has_tls_version('SSLv3'):
            try_protocol_combo(ssl.PROTOCOL_TLSv1_1, ssl.PROTOCOL_SSLv3, False)
        try_protocol_combo(ssl.PROTOCOL_TLSv1_1, ssl.PROTOCOL_TLS, False,
                           client_options=ssl.OP_NO_TLSv1_1)

        try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_TLSv1_1, 'TLSv1.1')
        try_protocol_combo(ssl.PROTOCOL_TLSv1_1, ssl.PROTOCOL_TLSv1_2, False)
        try_protocol_combo(ssl.PROTOCOL_TLSv1_2, ssl.PROTOCOL_TLSv1_1, False)

    @requires_tls_version('TLSv1_2')
    def test_protocol_tlsv1_2(self):
        """Connecting to a TLSv1.2 server with various client options.
           Testing against older TLS versions."""
        if support.verbose:
            sys.stdout.write("\n")
        try_protocol_combo(ssl.PROTOCOL_TLSv1_2, ssl.PROTOCOL_TLSv1_2, 'TLSv1.2',
                           server_options=ssl.OP_NO_SSLv3|ssl.OP_NO_SSLv2,
                           client_options=ssl.OP_NO_SSLv3|ssl.OP_NO_SSLv2,)
        if has_tls_version('SSLv3'):
            try_protocol_combo(ssl.PROTOCOL_TLSv1_2, ssl.PROTOCOL_SSLv3, False)
        try_protocol_combo(ssl.PROTOCOL_TLSv1_2, ssl.PROTOCOL_TLS, False,
                           client_options=ssl.OP_NO_TLSv1_2)

        try_protocol_combo(ssl.PROTOCOL_TLS, ssl.PROTOCOL_TLSv1_2, 'TLSv1.2')
        if has_tls_protocol(ssl.PROTOCOL_TLSv1):
            try_protocol_combo(ssl.PROTOCOL_TLSv1_2, ssl.PROTOCOL_TLSv1, False)
            try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_TLSv1_2, False)
        if has_tls_protocol(ssl.PROTOCOL_TLSv1_1):
            try_protocol_combo(ssl.PROTOCOL_TLSv1_2, ssl.PROTOCOL_TLSv1_1, False)
            try_protocol_combo(ssl.PROTOCOL_TLSv1_1, ssl.PROTOCOL_TLSv1_2, False)

    def test_starttls(self):
        """Switching from clear text to encrypted and back again."""
        msgs = (b"msg 1", b"MSG 2", b"STARTTLS", b"MSG 3", b"msg 4", b"ENDTLS", b"msg 5", b"msg 6")

        server = ThreadedEchoServer(CERTFILE,
                                    starttls_server=True,
                                    chatty=True,
                                    connectionchatty=True)
        wrapped = False
        with server:
            s = socket.socket()
            s.setblocking(True)
            s.connect((HOST, server.port))
            if support.verbose:
                sys.stdout.write("\n")
            for indata in msgs:
                if support.verbose:
                    sys.stdout.write(
                        " client:  sending %r...\n" % indata)
                if wrapped:
                    conn.write(indata)
                    outdata = conn.read()
                else:
                    s.send(indata)
                    outdata = s.recv(1024)
                msg = outdata.strip().lower()
                if indata == b"STARTTLS" and msg.startswith(b"ok"):
                    # STARTTLS ok, switch to secure mode
                    if support.verbose:
                        sys.stdout.write(
                            " client:  read %r from server, starting TLS...\n"
                            % msg)
                    conn = test_wrap_socket(s)
                    wrapped = True
                elif indata == b"ENDTLS" and msg.startswith(b"ok"):
                    # ENDTLS ok, switch back to clear text
                    if support.verbose:
                        sys.stdout.write(
                            " client:  read %r from server, ending TLS...\n"
                            % msg)
                    s = conn.unwrap()
                    wrapped = False
                else:
                    if support.verbose:
                        sys.stdout.write(
                            " client:  read %r from server\n" % msg)
            if support.verbose:
                sys.stdout.write(" client:  closing connection.\n")
            if wrapped:
                conn.write(b"over\n")
            else:
                s.send(b"over\n")
            if wrapped:
                conn.close()
            else:
                s.close()

    def test_socketserver(self):
        """Using socketserver to create and manage SSL connections."""
        server = make_https_server(self, certfile=SIGNED_CERTFILE)
        # try to connect
        if support.verbose:
            sys.stdout.write('\n')
        # Get this test file itself:
        with open(__file__, 'rb') as f:
            d1 = f.read()
        d2 = ''
        # now fetch the same data from the HTTPS server
        url = f'https://localhost:{server.port}/test_ssl.py'
        context = ssl.create_default_context(cafile=SIGNING_CA)
        f = urllib.request.urlopen(url, context=context)
        try:
            dlen = f.info().get("content-length")
            if dlen and (int(dlen) > 0):
                d2 = f.read(int(dlen))
                if support.verbose:
                    sys.stdout.write(
                        " client: read %d bytes from remote server '%s'\n"
                        % (len(d2), server))
        finally:
            f.close()
        self.assertEqual(d1, d2)

    def test_asyncore_server(self):
        """Check the example asyncore integration."""
        if support.verbose:
            sys.stdout.write("\n")

        indata = b"FOO\n"
        server = AsyncoreEchoServer(CERTFILE)
        with server:
            s = test_wrap_socket(socket.socket())
            s.connect(('127.0.0.1', server.port))
            if support.verbose:
                sys.stdout.write(
                    " client:  sending %r...\n" % indata)
            s.write(indata)
            outdata = s.read()
            if support.verbose:
                sys.stdout.write(" client:  read %r\n" % outdata)
            if outdata != indata.lower():
                self.fail(
                    "bad data <<%r>> (%d) received; expected <<%r>> (%d)\n"
                    % (outdata[:20], len(outdata),
                       indata[:20].lower(), len(indata)))
            s.write(b"over\n")
            if support.verbose:
                sys.stdout.write(" client:  closing connection.\n")
            s.close()
            if support.verbose:
                sys.stdout.write(" client:  connection closed.\n")

    def test_recv_send(self):
        """Test recv(), send() and friends."""
        if support.verbose:
            sys.stdout.write("\n")

        server = ThreadedEchoServer(CERTFILE,
                                    certreqs=ssl.CERT_NONE,
                                    ssl_version=ssl.PROTOCOL_TLS_SERVER,
                                    cacerts=CERTFILE,
                                    chatty=True,
                                    connectionchatty=False)
        with server:
            s = test_wrap_socket(socket.socket(),
                                server_side=False,
                                certfile=CERTFILE,
                                ca_certs=CERTFILE,
                                cert_reqs=ssl.CERT_NONE)
            s.connect((HOST, server.port))
            # helper methods for standardising recv* method signatures
            def _recv_into():
                b = bytearray(b"\0"*100)
                count = s.recv_into(b)
                return b[:count]

            def _recvfrom_into():
                b = bytearray(b"\0"*100)
                count, addr = s.recvfrom_into(b)
                return b[:count]

            # (name, method, expect success?, *args, return value func)
            send_methods = [
                ('send', s.send, True, [], len),
                ('sendto', s.sendto, False, ["some.address"], len),
                ('sendall', s.sendall, True, [], lambda x: None),
            ]
            # (name, method, whether to expect success, *args)
            recv_methods = [
                ('recv', s.recv, True, []),
                ('recvfrom', s.recvfrom, False, ["some.address"]),
                ('recv_into', _recv_into, True, []),
                ('recvfrom_into', _recvfrom_into, False, []),
            ]
            data_prefix = "PREFIX_"

            for (meth_name, send_meth, expect_success, args,
                    ret_val_meth) in send_methods:
                indata = (data_prefix + meth_name).encode('ascii')
                try:
                    ret = send_meth(indata, *args)
                    msg = "sending with {}".format(meth_name)
                    self.assertEqual(ret, ret_val_meth(indata), msg=msg)
                    outdata = s.read()
                    if outdata != indata.lower():
                        self.fail(
                            "While sending with <<{name:s}>> bad data "
                            "<<{outdata:r}>> ({nout:d}) received; "
                            "expected <<{indata:r}>> ({nin:d})\n".format(
                                name=meth_name, outdata=outdata[:20],
                                nout=len(outdata),
                                indata=indata[:20], nin=len(indata)
                            )
                        )
                except ValueError as e:
                    if expect_success:
                        self.fail(
                            "Failed to send with method <<{name:s}>>; "
                            "expected to succeed.\n".format(name=meth_name)
                        )
                    if not str(e).startswith(meth_name):
                        self.fail(
                            "Method <<{name:s}>> failed with unexpected "
                            "exception message: {exp:s}\n".format(
                                name=meth_name, exp=e
                            )
                        )

            for meth_name, recv_meth, expect_success, args in recv_methods:
                indata = (data_prefix + meth_name).encode('ascii')
                try:
                    s.send(indata)
                    outdata = recv_meth(*args)
                    if outdata != indata.lower():
                        self.fail(
                            "While receiving with <<{name:s}>> bad data "
                            "<<{outdata:r}>> ({nout:d}) received; "
                            "expected <<{indata:r}>> ({nin:d})\n".format(
                                name=meth_name, outdata=outdata[:20],
                                nout=len(outdata),
                                indata=indata[:20], nin=len(indata)
                            )
                        )
                except ValueError as e:
                    if expect_success:
                        self.fail(
                            "Failed to receive with method <<{name:s}>>; "
                            "expected to succeed.\n".format(name=meth_name)
                        )
                    if not str(e).startswith(meth_name):
                        self.fail(
                            "Method <<{name:s}>> failed with unexpected "
                            "exception message: {exp:s}\n".format(
                                name=meth_name, exp=e
                            )
                        )
                    # consume data
                    s.read()

            # read(-1, buffer) is supported, even though read(-1) is not
            data = b"data"
            s.send(data)
            buffer = bytearray(len(data))
            self.assertEqual(s.read(-1, buffer), len(data))
            self.assertEqual(buffer, data)

            # sendall accepts bytes-like objects
            if ctypes is not None:
                ubyte = ctypes.c_ubyte * len(data)
                byteslike = ubyte.from_buffer_copy(data)
                s.sendall(byteslike)
                self.assertEqual(s.read(), data)

            # Make sure sendmsg et al are disallowed to avoid
            # inadvertent disclosure of data and/or corruption
            # of the encrypted data stream
            self.assertRaises(NotImplementedError, s.dup)
            self.assertRaises(NotImplementedError, s.sendmsg, [b"data"])
            self.assertRaises(NotImplementedError, s.recvmsg, 100)
            self.assertRaises(NotImplementedError,
                              s.recvmsg_into, [bytearray(100)])
            s.write(b"over\n")

            self.assertRaises(ValueError, s.recv, -1)
            self.assertRaises(ValueError, s.read, -1)

            s.close()

    def test_recv_zero(self):
        server = ThreadedEchoServer(CERTFILE)
        self.enterContext(server)
        s = socket.create_connection((HOST, server.port))
        self.addCleanup(s.close)
        s = test_wrap_socket(s, suppress_ragged_eofs=False)
        self.addCleanup(s.close)

        # recv/read(0) should return no data
        s.send(b"data")
        self.assertEqual(s.recv(0), b"")
        self.assertEqual(s.read(0), b"")
        self.assertEqual(s.read(), b"data")

        # Should not block if the other end sends no data
        s.setblocking(False)
        self.assertEqual(s.recv(0), b"")
        self.assertEqual(s.recv_into(bytearray()), 0)

    def test_recv_into_buffer_protocol_len(self):
        server = ThreadedEchoServer(CERTFILE)
        self.enterContext(server)
        s = socket.create_connection((HOST, server.port))
        self.addCleanup(s.close)
        s = test_wrap_socket(s, suppress_ragged_eofs=False)
        self.addCleanup(s.close)

        s.send(b"data")
        buf = array.array('I', [0, 0])
        self.assertEqual(s.recv_into(buf), 4)
        self.assertEqual(bytes(buf)[:4], b"data")

        class B(bytearray):
            def __len__(self):
                1/0
        s.send(b"data")
        buf = B(6)
        self.assertEqual(s.recv_into(buf), 4)
        self.assertEqual(bytes(buf), b"data\0\0")

    def test_nonblocking_send(self):
        server = ThreadedEchoServer(CERTFILE,
                                    certreqs=ssl.CERT_NONE,
                                    ssl_version=ssl.PROTOCOL_TLS_SERVER,
                                    cacerts=CERTFILE,
                                    chatty=True,
                                    connectionchatty=False)
        with server:
            s = test_wrap_socket(socket.socket(),
                                server_side=False,
                                certfile=CERTFILE,
                                ca_certs=CERTFILE,
                                cert_reqs=ssl.CERT_NONE)
            s.connect((HOST, server.port))
            s.setblocking(False)

            # If we keep sending data, at some point the buffers
            # will be full and the call will block
            buf = bytearray(8192)
            def fill_buffer():
                while True:
                    s.send(buf)
            self.assertRaises((ssl.SSLWantWriteError,
                               ssl.SSLWantReadError), fill_buffer)

            # Now read all the output and discard it
            s.setblocking(True)
            s.close()

    def test_handshake_timeout(self):
        # Issue #5103: SSL handshake must respect the socket timeout
        server = socket.socket(socket.AF_INET)
        host = "127.0.0.1"
        port = socket_helper.bind_port(server)
        started = threading.Event()
        finish = False

        def serve():
            server.listen()
            started.set()
            conns = []
            while not finish:
                r, w, e = select.select([server], [], [], 0.1)
                if server in r:
                    # Let the socket hang around rather than having
                    # it closed by garbage collection.
                    conns.append(server.accept()[0])
            for sock in conns:
                sock.close()

        t = threading.Thread(target=serve)
        t.start()
        started.wait()

        try:
            try:
                c = socket.socket(socket.AF_INET)
                c.settimeout(0.2)
                c.connect((host, port))
                # Will attempt handshake and time out
                self.assertRaisesRegex(TimeoutError, "timed out",
                                       test_wrap_socket, c)
            finally:
                c.close()
            try:
                c = socket.socket(socket.AF_INET)
                c = test_wrap_socket(c)
                c.settimeout(0.2)
                # Will attempt handshake and time out
                self.assertRaisesRegex(TimeoutError, "timed out",
                                       c.connect, (host, port))
            finally:
                c.close()
        finally:
            finish = True
            t.join()
            server.close()

    def test_server_accept(self):
        # Issue #16357: accept() on a SSLSocket created through
        # SSLContext.wrap_socket().
        client_ctx, server_ctx, hostname = testing_context()
        server = socket.socket(socket.AF_INET)
        host = "127.0.0.1"
        port = socket_helper.bind_port(server)
        server = server_ctx.wrap_socket(server, server_side=True)
        self.assertTrue(server.server_side)

        evt = threading.Event()
        remote = None
        peer = None
        def serve():
            nonlocal remote, peer
            server.listen()
            # Block on the accept and wait on the connection to close.
            evt.set()
            remote, peer = server.accept()
            remote.send(remote.recv(4))

        t = threading.Thread(target=serve)
        t.start()
        # Client wait until server setup and perform a connect.
        evt.wait()
        client = client_ctx.wrap_socket(
            socket.socket(), server_hostname=hostname
        )
        client.connect((hostname, port))
        client.send(b'data')
        client.recv()
        client_addr = client.getsockname()
        client.close()
        t.join()
        remote.close()
        server.close()
        # Sanity checks.
        self.assertIsInstance(remote, ssl.SSLSocket)
        self.assertEqual(peer, client_addr)

    def test_getpeercert_enotconn(self):
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        context.check_hostname = False
        with context.wrap_socket(socket.socket()) as sock:
            with self.assertRaises(OSError) as cm:
                sock.getpeercert()
            self.assertEqual(cm.exception.errno, errno.ENOTCONN)

    def test_do_handshake_enotconn(self):
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        context.check_hostname = False
        with context.wrap_socket(socket.socket()) as sock:
            with self.assertRaises(OSError) as cm:
                sock.do_handshake()
            self.assertEqual(cm.exception.errno, errno.ENOTCONN)

    def test_no_shared_ciphers(self):
        client_context, server_context, hostname = testing_context()
        # OpenSSL enables all TLS 1.3 ciphers, enforce TLS 1.2 for test
        client_context.maximum_version = ssl.TLSVersion.TLSv1_2
        # Force different suites on client and server
        client_context.set_ciphers("AES128")
        server_context.set_ciphers("AES256")
        with ThreadedEchoServer(context=server_context) as server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                with self.assertRaises(OSError):
                    s.connect((HOST, server.port))
        self.assertIn("NO_SHARED_CIPHER", server.conn_errors[0])

    def test_version_basic(self):
        """
        Basic tests for SSLSocket.version().
        More tests are done in the test_protocol_*() methods.
        """
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE
        with ThreadedEchoServer(CERTFILE,
                                ssl_version=ssl.PROTOCOL_TLS_SERVER,
                                chatty=False) as server:
            with context.wrap_socket(socket.socket()) as s:
                self.assertIs(s.version(), None)
                self.assertIs(s._sslobj, None)
                s.connect((HOST, server.port))
                self.assertEqual(s.version(), 'TLSv1.3')
            self.assertIs(s._sslobj, None)
            self.assertIs(s.version(), None)

    @requires_tls_version('TLSv1_3')
    def test_tls1_3(self):
        client_context, server_context, hostname = testing_context()
        client_context.minimum_version = ssl.TLSVersion.TLSv1_3
        with ThreadedEchoServer(context=server_context) as server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                self.assertIn(s.cipher()[0], {
                    'TLS_AES_256_GCM_SHA384',
                    'TLS_CHACHA20_POLY1305_SHA256',
                    'TLS_AES_128_GCM_SHA256',
                })
                self.assertEqual(s.version(), 'TLSv1.3')

    @requires_tls_version('TLSv1_2')
    @requires_tls_version('TLSv1')
    @ignore_deprecation
    def test_min_max_version_tlsv1_2(self):
        client_context, server_context, hostname = testing_context()
        # client TLSv1.0 to 1.2
        client_context.minimum_version = ssl.TLSVersion.TLSv1
        client_context.maximum_version = ssl.TLSVersion.TLSv1_2
        # server only TLSv1.2
        server_context.minimum_version = ssl.TLSVersion.TLSv1_2
        server_context.maximum_version = ssl.TLSVersion.TLSv1_2

        with ThreadedEchoServer(context=server_context) as server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                self.assertEqual(s.version(), 'TLSv1.2')

    @requires_tls_version('TLSv1_1')
    @ignore_deprecation
    def test_min_max_version_tlsv1_1(self):
        client_context, server_context, hostname = testing_context()
        # client 1.0 to 1.2, server 1.0 to 1.1
        client_context.minimum_version = ssl.TLSVersion.TLSv1
        client_context.maximum_version = ssl.TLSVersion.TLSv1_2
        server_context.minimum_version = ssl.TLSVersion.TLSv1
        server_context.maximum_version = ssl.TLSVersion.TLSv1_1
        seclevel_workaround(client_context, server_context)

        with ThreadedEchoServer(context=server_context) as server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                self.assertEqual(s.version(), 'TLSv1.1')

    @requires_tls_version('TLSv1_2')
    @requires_tls_version('TLSv1')
    @ignore_deprecation
    def test_min_max_version_mismatch(self):
        client_context, server_context, hostname = testing_context()
        # client 1.0, server 1.2 (mismatch)
        server_context.maximum_version = ssl.TLSVersion.TLSv1_2
        server_context.minimum_version = ssl.TLSVersion.TLSv1_2
        client_context.maximum_version = ssl.TLSVersion.TLSv1
        client_context.minimum_version = ssl.TLSVersion.TLSv1
        seclevel_workaround(client_context, server_context)

        with ThreadedEchoServer(context=server_context) as server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                with self.assertRaises(ssl.SSLError) as e:
                    s.connect((HOST, server.port))
                self.assertRegex(str(e.exception), "(alert|ALERT)")

    @requires_tls_version('SSLv3')
    def test_min_max_version_sslv3(self):
        client_context, server_context, hostname = testing_context()
        server_context.minimum_version = ssl.TLSVersion.SSLv3
        client_context.minimum_version = ssl.TLSVersion.SSLv3
        client_context.maximum_version = ssl.TLSVersion.SSLv3
        seclevel_workaround(client_context, server_context)

        with ThreadedEchoServer(context=server_context) as server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                self.assertEqual(s.version(), 'SSLv3')

    def test_default_ecdh_curve(self):
        # Issue #21015: elliptic curve-based Diffie Hellman key exchange
        # should be enabled by default on SSL contexts.
        client_context, server_context, hostname = testing_context()
        # TLSv1.3 defaults to PFS key agreement and no longer has KEA in
        # cipher name.
        client_context.maximum_version = ssl.TLSVersion.TLSv1_2
        # Prior to OpenSSL 1.0.0, ECDH ciphers have to be enabled
        # explicitly using the 'ECCdraft' cipher alias.  Otherwise,
        # our default cipher list should prefer ECDH-based ciphers
        # automatically.
        with ThreadedEchoServer(context=server_context) as server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                self.assertIn("ECDH", s.cipher()[0])

    @unittest.skipUnless("tls-unique" in ssl.CHANNEL_BINDING_TYPES,
                         "'tls-unique' channel binding not available")
    def test_tls_unique_channel_binding(self):
        """Test tls-unique channel binding."""
        if support.verbose:
            sys.stdout.write("\n")

        client_context, server_context, hostname = testing_context()

        # tls-unique is not defined for TLSv1.3
        # https://datatracker.ietf.org/doc/html/rfc8446#appendix-C.5
        client_context.maximum_version = ssl.TLSVersion.TLSv1_2

        server = ThreadedEchoServer(context=server_context,
                                    chatty=True,
                                    connectionchatty=False)

        with server:
            with client_context.wrap_socket(
                    socket.socket(),
                    server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                # get the data
                cb_data = s.get_channel_binding("tls-unique")
                if support.verbose:
                    sys.stdout.write(
                        " got channel binding data: {0!r}\n".format(cb_data))

                # check if it is sane
                self.assertIsNotNone(cb_data)
                if s.version() == 'TLSv1.3':
                    self.assertEqual(len(cb_data), 48)
                else:
                    self.assertEqual(len(cb_data), 12)  # True for TLSv1

                # and compare with the peers version
                s.write(b"CB tls-unique\n")
                peer_data_repr = s.read().strip()
                self.assertEqual(peer_data_repr,
                                 repr(cb_data).encode("us-ascii"))

            # now, again
            with client_context.wrap_socket(
                    socket.socket(),
                    server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                new_cb_data = s.get_channel_binding("tls-unique")
                if support.verbose:
                    sys.stdout.write(
                        "got another channel binding data: {0!r}\n".format(
                            new_cb_data)
                    )
                # is it really unique
                self.assertNotEqual(cb_data, new_cb_data)
                self.assertIsNotNone(cb_data)
                if s.version() == 'TLSv1.3':
                    self.assertEqual(len(cb_data), 48)
                else:
                    self.assertEqual(len(cb_data), 12)  # True for TLSv1
                s.write(b"CB tls-unique\n")
                peer_data_repr = s.read().strip()
                self.assertEqual(peer_data_repr,
                                 repr(new_cb_data).encode("us-ascii"))

    def test_compression(self):
        client_context, server_context, hostname = testing_context()
        stats = server_params_test(client_context, server_context,
                                   chatty=True, connectionchatty=True,
                                   sni_name=hostname)
        if support.verbose:
            sys.stdout.write(" got compression: {!r}\n".format(stats['compression']))
        self.assertIn(stats['compression'], { None, 'ZLIB', 'RLE' })

    @unittest.skipUnless(hasattr(ssl, 'OP_NO_COMPRESSION'),
                         "ssl.OP_NO_COMPRESSION needed for this test")
    def test_compression_disabled(self):
        client_context, server_context, hostname = testing_context()
        client_context.options |= ssl.OP_NO_COMPRESSION
        server_context.options |= ssl.OP_NO_COMPRESSION
        stats = server_params_test(client_context, server_context,
                                   chatty=True, connectionchatty=True,
                                   sni_name=hostname)
        self.assertIs(stats['compression'], None)

    def test_legacy_server_connect(self):
        client_context, server_context, hostname = testing_context()
        client_context.options |= ssl.OP_LEGACY_SERVER_CONNECT
        server_params_test(client_context, server_context,
                                   chatty=True, connectionchatty=True,
                                   sni_name=hostname)

    def test_no_legacy_server_connect(self):
        client_context, server_context, hostname = testing_context()
        client_context.options &= ~ssl.OP_LEGACY_SERVER_CONNECT
        server_params_test(client_context, server_context,
                                   chatty=True, connectionchatty=True,
                                   sni_name=hostname)

    def test_dh_params(self):
        # Check we can get a connection with ephemeral finite-field
        # Diffie-Hellman (if supported).
        client_context, server_context, hostname = testing_context()
        dhe_aliases = {"ADH", "EDH", "DHE"}
        if not (supports_kx_alias(client_context, dhe_aliases)
                and supports_kx_alias(server_context, dhe_aliases)):
            self.skipTest("libssl doesn't support ephemeral DH")
        # test scenario needs TLS <= 1.2
        client_context.maximum_version = ssl.TLSVersion.TLSv1_2
        try:
            server_context.load_dh_params(DHFILE)
        except RuntimeError:
            if Py_DEBUG_WIN32:
                self.skipTest("not supported on Win32 debug build")
            raise
        server_context.set_ciphers("kEDH")
        server_context.maximum_version = ssl.TLSVersion.TLSv1_2
        stats = server_params_test(client_context, server_context,
                                   chatty=True, connectionchatty=True,
                                   sni_name=hostname)
        cipher = stats["cipher"][0]
        parts = cipher.split("-")
        if not dhe_aliases.intersection(parts):
            self.fail("Non-DH key exchange: " + cipher[0])

    def test_ecdh_curve(self):
        # server secp384r1, client auto
        client_context, server_context, hostname = testing_context()

        server_context.set_ecdh_curve("secp384r1")
        server_context.set_ciphers("ECDHE:!eNULL:!aNULL")
        server_context.minimum_version = ssl.TLSVersion.TLSv1_2
        stats = server_params_test(client_context, server_context,
                                   chatty=True, connectionchatty=True,
                                   sni_name=hostname)

        # server auto, client secp384r1
        client_context, server_context, hostname = testing_context()
        client_context.set_ecdh_curve("secp384r1")
        server_context.set_ciphers("ECDHE:!eNULL:!aNULL")
        server_context.minimum_version = ssl.TLSVersion.TLSv1_2
        stats = server_params_test(client_context, server_context,
                                   chatty=True, connectionchatty=True,
                                   sni_name=hostname)

        # server / client curve mismatch
        client_context, server_context, hostname = testing_context()
        client_context.set_ecdh_curve("prime256v1")
        server_context.set_ecdh_curve("secp384r1")
        server_context.set_ciphers("ECDHE:!eNULL:!aNULL")
        server_context.minimum_version = ssl.TLSVersion.TLSv1_2
        with self.assertRaises(ssl.SSLError):
            server_params_test(client_context, server_context,
                               chatty=True, connectionchatty=True,
                               sni_name=hostname)

    def test_selected_alpn_protocol(self):
        # selected_alpn_protocol() is None unless ALPN is used.
        client_context, server_context, hostname = testing_context()
        stats = server_params_test(client_context, server_context,
                                   chatty=True, connectionchatty=True,
                                   sni_name=hostname)
        self.assertIs(stats['client_alpn_protocol'], None)

    def test_selected_alpn_protocol_if_server_uses_alpn(self):
        # selected_alpn_protocol() is None unless ALPN is used by the client.
        client_context, server_context, hostname = testing_context()
        server_context.set_alpn_protocols(['foo', 'bar'])
        stats = server_params_test(client_context, server_context,
                                   chatty=True, connectionchatty=True,
                                   sni_name=hostname)
        self.assertIs(stats['client_alpn_protocol'], None)

    def test_alpn_protocols(self):
        server_protocols = ['foo', 'bar', 'milkshake']
        protocol_tests = [
            (['foo', 'bar'], 'foo'),
            (['bar', 'foo'], 'foo'),
            (['milkshake'], 'milkshake'),
            (['http/3.0', 'http/4.0'], None)
        ]
        for client_protocols, expected in protocol_tests:
            client_context, server_context, hostname = testing_context()
            server_context.set_alpn_protocols(server_protocols)
            client_context.set_alpn_protocols(client_protocols)

            try:
                stats = server_params_test(client_context,
                                           server_context,
                                           chatty=True,
                                           connectionchatty=True,
                                           sni_name=hostname)
            except ssl.SSLError as e:
                stats = e

            msg = "failed trying %s (s) and %s (c).\n" \
                "was expecting %s, but got %%s from the %%s" \
                    % (str(server_protocols), str(client_protocols),
                        str(expected))
            client_result = stats['client_alpn_protocol']
            self.assertEqual(client_result, expected,
                             msg % (client_result, "client"))
            server_result = stats['server_alpn_protocols'][-1] \
                if len(stats['server_alpn_protocols']) else 'nothing'
            self.assertEqual(server_result, expected,
                             msg % (server_result, "server"))

    def test_npn_protocols(self):
        assert not ssl.HAS_NPN

    def sni_contexts(self):
        server_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        server_context.load_cert_chain(SIGNED_CERTFILE)
        other_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        other_context.load_cert_chain(SIGNED_CERTFILE2)
        client_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        client_context.load_verify_locations(SIGNING_CA)
        return server_context, other_context, client_context

    def check_common_name(self, stats, name):
        cert = stats['peercert']
        self.assertIn((('commonName', name),), cert['subject'])

    def test_sni_callback(self):
        calls = []
        server_context, other_context, client_context = self.sni_contexts()

        client_context.check_hostname = False

        def servername_cb(ssl_sock, server_name, initial_context):
            calls.append((server_name, initial_context))
            if server_name is not None:
                ssl_sock.context = other_context
        server_context.set_servername_callback(servername_cb)

        stats = server_params_test(client_context, server_context,
                                   chatty=True,
                                   sni_name='supermessage')
        # The hostname was fetched properly, and the certificate was
        # changed for the connection.
        self.assertEqual(calls, [("supermessage", server_context)])
        # CERTFILE4 was selected
        self.check_common_name(stats, 'fakehostname')

        calls = []
        # The callback is called with server_name=None
        stats = server_params_test(client_context, server_context,
                                   chatty=True,
                                   sni_name=None)
        self.assertEqual(calls, [(None, server_context)])
        self.check_common_name(stats, SIGNED_CERTFILE_HOSTNAME)

        # Check disabling the callback
        calls = []
        server_context.set_servername_callback(None)

        stats = server_params_test(client_context, server_context,
                                   chatty=True,
                                   sni_name='notfunny')
        # Certificate didn't change
        self.check_common_name(stats, SIGNED_CERTFILE_HOSTNAME)
        self.assertEqual(calls, [])

    def test_sni_callback_alert(self):
        # Returning a TLS alert is reflected to the connecting client
        server_context, other_context, client_context = self.sni_contexts()

        def cb_returning_alert(ssl_sock, server_name, initial_context):
            return ssl.ALERT_DESCRIPTION_ACCESS_DENIED
        server_context.set_servername_callback(cb_returning_alert)
        with self.assertRaises(ssl.SSLError) as cm:
            stats = server_params_test(client_context, server_context,
                                       chatty=False,
                                       sni_name='supermessage')
        self.assertEqual(cm.exception.reason, 'TLSV1_ALERT_ACCESS_DENIED')

    def test_sni_callback_raising(self):
        # Raising fails the connection with a TLS handshake failure alert.
        server_context, other_context, client_context = self.sni_contexts()

        def cb_raising(ssl_sock, server_name, initial_context):
            1/0
        server_context.set_servername_callback(cb_raising)

        with support.catch_unraisable_exception() as catch:
            with self.assertRaises(ssl.SSLError) as cm:
                stats = server_params_test(client_context, server_context,
                                           chatty=False,
                                           sni_name='supermessage')

            # Allow for flexible libssl error messages.
            regex = "(SSLV3_ALERT_HANDSHAKE_FAILURE|NO_PRIVATE_VALUE)"
            self.assertRegex(cm.exception.reason, regex)
            self.assertEqual(catch.unraisable.exc_type, ZeroDivisionError)

    def test_sni_callback_wrong_return_type(self):
        # Returning the wrong return type terminates the TLS connection
        # with an internal error alert.
        server_context, other_context, client_context = self.sni_contexts()

        def cb_wrong_return_type(ssl_sock, server_name, initial_context):
            return "foo"
        server_context.set_servername_callback(cb_wrong_return_type)

        with support.catch_unraisable_exception() as catch:
            with self.assertRaises(ssl.SSLError) as cm:
                stats = server_params_test(client_context, server_context,
                                           chatty=False,
                                           sni_name='supermessage')


            self.assertEqual(cm.exception.reason, 'TLSV1_ALERT_INTERNAL_ERROR')
            self.assertEqual(catch.unraisable.exc_type, TypeError)

    def test_shared_ciphers(self):
        client_context, server_context, hostname = testing_context()
        client_context.set_ciphers("AES128:AES256")
        server_context.set_ciphers("AES256:eNULL")
        expected_algs = [
            "AES256", "AES-256",
            # TLS 1.3 ciphers are always enabled
            "TLS_CHACHA20", "TLS_AES",
        ]

        stats = server_params_test(client_context, server_context,
                                   sni_name=hostname)
        ciphers = stats['server_shared_ciphers'][0]
        self.assertGreater(len(ciphers), 0)
        for name, tls_version, bits in ciphers:
            if not any(alg in name for alg in expected_algs):
                self.fail(name)

    def test_read_write_after_close_raises_valuerror(self):
        client_context, server_context, hostname = testing_context()
        server = ThreadedEchoServer(context=server_context, chatty=False)

        with server:
            s = client_context.wrap_socket(socket.socket(),
                                           server_hostname=hostname)
            s.connect((HOST, server.port))
            s.close()

            self.assertRaises(ValueError, s.read, 1024)
            self.assertRaises(ValueError, s.write, b'hello')

    def test_sendfile(self):
        """Try to send a file using kTLS if possible."""
        TEST_DATA = b"x" * 512
        with open(os_helper.TESTFN, 'wb') as f:
            f.write(TEST_DATA)
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        client_context, server_context, hostname = testing_context()
        client_context.options |= getattr(ssl, 'OP_ENABLE_KTLS', 0)
        server = ThreadedEchoServer(context=server_context, chatty=False)
        # kTLS seems to work only with a connection created before
        # wrapping `sock` by the SSL context in contrast to calling
        # `sock.connect()` after the wrapping.
        with server, socket.create_connection((HOST, server.port)) as sock:
            with client_context.wrap_socket(
                sock, server_hostname=hostname
            ) as ssock:
                if support.verbose:
                    ktls_used = ssock._sslobj.uses_ktls_for_send()
                    print(
                        'kTLS is',
                        'available' if ktls_used else 'unavailable',
                    )
                with open(os_helper.TESTFN, 'rb') as file:
                    ssock.sendfile(file)
                self.assertEqual(ssock.recv(1024), TEST_DATA)

    def test_session(self):
        client_context, server_context, hostname = testing_context()
        # TODO: sessions aren't compatible with TLSv1.3 yet
        client_context.maximum_version = ssl.TLSVersion.TLSv1_2

        # first connection without session
        stats = server_params_test(client_context, server_context,
                                   sni_name=hostname)
        session = stats['session']
        self.assertTrue(session.id)
        self.assertGreater(session.time, 0)
        self.assertGreater(session.timeout, 0)
        self.assertTrue(session.has_ticket)
        self.assertGreater(session.ticket_lifetime_hint, 0)
        self.assertFalse(stats['session_reused'])
        sess_stat = server_context.session_stats()
        self.assertEqual(sess_stat['accept'], 1)
        self.assertEqual(sess_stat['hits'], 0)

        # reuse session
        stats = server_params_test(client_context, server_context,
                                   session=session, sni_name=hostname)
        sess_stat = server_context.session_stats()
        self.assertEqual(sess_stat['accept'], 2)
        self.assertEqual(sess_stat['hits'], 1)
        self.assertTrue(stats['session_reused'])
        session2 = stats['session']
        self.assertEqual(session2.id, session.id)
        self.assertEqual(session2, session)
        self.assertIsNot(session2, session)
        self.assertGreaterEqual(session2.time, session.time)
        self.assertGreaterEqual(session2.timeout, session.timeout)

        # another one without session
        stats = server_params_test(client_context, server_context,
                                   sni_name=hostname)
        self.assertFalse(stats['session_reused'])
        session3 = stats['session']
        self.assertNotEqual(session3.id, session.id)
        self.assertNotEqual(session3, session)
        sess_stat = server_context.session_stats()
        self.assertEqual(sess_stat['accept'], 3)
        self.assertEqual(sess_stat['hits'], 1)

        # reuse session again
        stats = server_params_test(client_context, server_context,
                                   session=session, sni_name=hostname)
        self.assertTrue(stats['session_reused'])
        session4 = stats['session']
        self.assertEqual(session4.id, session.id)
        self.assertEqual(session4, session)
        self.assertGreaterEqual(session4.time, session.time)
        self.assertGreaterEqual(session4.timeout, session.timeout)
        sess_stat = server_context.session_stats()
        self.assertEqual(sess_stat['accept'], 4)
        self.assertEqual(sess_stat['hits'], 2)

    def test_session_handling(self):
        client_context, server_context, hostname = testing_context()
        client_context2, _, _ = testing_context()

        # TODO: session reuse does not work with TLSv1.3
        client_context.maximum_version = ssl.TLSVersion.TLSv1_2
        client_context2.maximum_version = ssl.TLSVersion.TLSv1_2

        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                # session is None before handshake
                self.assertEqual(s.session, None)
                self.assertEqual(s.session_reused, None)
                s.connect((HOST, server.port))
                session = s.session
                self.assertTrue(session)
                with self.assertRaises(TypeError) as e:
                    s.session = object
                self.assertEqual(str(e.exception), 'Value is not a SSLSession.')

            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                # cannot set session after handshake
                with self.assertRaises(ValueError) as e:
                    s.session = session
                self.assertEqual(str(e.exception),
                                 'Cannot set session after handshake.')

            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                # can set session before handshake and before the
                # connection was established
                s.session = session
                s.connect((HOST, server.port))
                self.assertEqual(s.session.id, session.id)
                self.assertEqual(s.session, session)
                self.assertEqual(s.session_reused, True)

            with client_context2.wrap_socket(socket.socket(),
                                             server_hostname=hostname) as s:
                # cannot re-use session with a different SSLContext
                with self.assertRaises(ValueError) as e:
                    s.session = session
                    s.connect((HOST, server.port))
                self.assertEqual(str(e.exception),
                                 'Session refers to a different SSLContext.')

    @requires_tls_version('TLSv1_2')
    @unittest.skipUnless(ssl.HAS_PSK, 'TLS-PSK disabled on this OpenSSL build')
    def test_psk(self):
        psk = bytes.fromhex('deadbeef')

        client_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        client_context.check_hostname = False
        client_context.verify_mode = ssl.CERT_NONE
        client_context.maximum_version = ssl.TLSVersion.TLSv1_2
        client_context.set_ciphers('PSK')
        client_context.set_psk_client_callback(lambda hint: (None, psk))

        server_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        server_context.maximum_version = ssl.TLSVersion.TLSv1_2
        server_context.set_ciphers('PSK')
        server_context.set_psk_server_callback(lambda identity: psk)

        # correct PSK should connect
        server = ThreadedEchoServer(context=server_context)
        with server:
            with client_context.wrap_socket(socket.socket()) as s:
                s.connect((HOST, server.port))

        # incorrect PSK should fail
        incorrect_psk = bytes.fromhex('cafebabe')
        client_context.set_psk_client_callback(lambda hint: (None, incorrect_psk))
        server = ThreadedEchoServer(context=server_context)
        with server:
            with client_context.wrap_socket(socket.socket()) as s:
                with self.assertRaises(ssl.SSLError):
                    s.connect((HOST, server.port))

        # identity_hint and client_identity should be sent to the other side
        identity_hint = 'identity-hint'
        client_identity = 'client-identity'

        def client_callback(hint):
            self.assertEqual(hint, identity_hint)
            return client_identity, psk

        def server_callback(identity):
            self.assertEqual(identity, client_identity)
            return psk

        client_context.set_psk_client_callback(client_callback)
        server_context.set_psk_server_callback(server_callback, identity_hint)
        server = ThreadedEchoServer(context=server_context)
        with server:
            with client_context.wrap_socket(socket.socket()) as s:
                s.connect((HOST, server.port))

        # adding client callback to server or vice versa raises an exception
        with self.assertRaisesRegex(ssl.SSLError, 'Cannot add PSK server callback'):
            client_context.set_psk_server_callback(server_callback, identity_hint)
        with self.assertRaisesRegex(ssl.SSLError, 'Cannot add PSK client callback'):
            server_context.set_psk_client_callback(client_callback)

        # test with UTF-8 identities
        identity_hint = '身份暗示'  # Translation: "Identity hint"
        client_identity = '客户身份'  # Translation: "Customer identity"

        client_context.set_psk_client_callback(client_callback)
        server_context.set_psk_server_callback(server_callback, identity_hint)
        server = ThreadedEchoServer(context=server_context)
        with server:
            with client_context.wrap_socket(socket.socket()) as s:
                s.connect((HOST, server.port))

    @requires_tls_version('TLSv1_3')
    @unittest.skipUnless(ssl.HAS_PSK, 'TLS-PSK disabled on this OpenSSL build')
    @unittest.skipUnless(ssl.HAS_PSK_TLS13, 'TLS 1.3 PSK disabled on this OpenSSL build')
    def test_psk_tls1_3(self):
        psk = bytes.fromhex('deadbeef')
        identity_hint = 'identity-hint'
        client_identity = 'client-identity'

        def client_callback(hint):
            # identity_hint is not sent to the client in TLS 1.3
            self.assertIsNone(hint)
            return client_identity, psk

        def server_callback(identity):
            self.assertEqual(identity, client_identity)
            return psk

        client_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        client_context.check_hostname = False
        client_context.verify_mode = ssl.CERT_NONE
        client_context.minimum_version = ssl.TLSVersion.TLSv1_3
        client_context.set_ciphers('PSK')
        client_context.set_psk_client_callback(client_callback)

        server_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        server_context.minimum_version = ssl.TLSVersion.TLSv1_3
        server_context.set_ciphers('PSK')
        server_context.set_psk_server_callback(server_callback, identity_hint)

        server = ThreadedEchoServer(context=server_context)
        with server:
            with client_context.wrap_socket(socket.socket()) as s:
                s.connect((HOST, server.port))


@unittest.skipUnless(has_tls_version('TLSv1_3') and ssl.HAS_PHA,
                     "Test needs TLS 1.3 PHA")
class TestPostHandshakeAuth(unittest.TestCase):
    def test_pha_setter(self):
        protocols = [
            ssl.PROTOCOL_TLS_SERVER, ssl.PROTOCOL_TLS_CLIENT
        ]
        for protocol in protocols:
            ctx = ssl.SSLContext(protocol)
            self.assertEqual(ctx.post_handshake_auth, False)

            ctx.post_handshake_auth = True
            self.assertEqual(ctx.post_handshake_auth, True)

            ctx.verify_mode = ssl.CERT_REQUIRED
            self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
            self.assertEqual(ctx.post_handshake_auth, True)

            ctx.post_handshake_auth = False
            self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
            self.assertEqual(ctx.post_handshake_auth, False)

            ctx.verify_mode = ssl.CERT_OPTIONAL
            ctx.post_handshake_auth = True
            self.assertEqual(ctx.verify_mode, ssl.CERT_OPTIONAL)
            self.assertEqual(ctx.post_handshake_auth, True)

    def test_pha_required(self):
        client_context, server_context, hostname = testing_context()
        server_context.post_handshake_auth = True
        server_context.verify_mode = ssl.CERT_REQUIRED
        client_context.post_handshake_auth = True
        client_context.load_cert_chain(SIGNED_CERTFILE)

        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                s.write(b'HASCERT')
                self.assertEqual(s.recv(1024), b'FALSE\n')
                s.write(b'PHA')
                self.assertEqual(s.recv(1024), b'OK\n')
                s.write(b'HASCERT')
                self.assertEqual(s.recv(1024), b'TRUE\n')
                # PHA method just returns true when cert is already available
                s.write(b'PHA')
                self.assertEqual(s.recv(1024), b'OK\n')
                s.write(b'GETCERT')
                cert_text = s.recv(4096).decode('us-ascii')
                self.assertIn('Python Software Foundation CA', cert_text)

    def test_pha_required_nocert(self):
        client_context, server_context, hostname = testing_context()
        server_context.post_handshake_auth = True
        server_context.verify_mode = ssl.CERT_REQUIRED
        client_context.post_handshake_auth = True

        def msg_cb(conn, direction, version, content_type, msg_type, data):
            if support.verbose and content_type == _TLSContentType.ALERT:
                info = (conn, direction, version, content_type, msg_type, data)
                sys.stdout.write(f"TLS: {info!r}\n")

        server_context._msg_callback = msg_cb
        client_context._msg_callback = msg_cb

        server = ThreadedEchoServer(context=server_context, chatty=True)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname,
                                            suppress_ragged_eofs=False) as s:
                s.connect((HOST, server.port))
                s.write(b'PHA')
                # test sometimes fails with EOF error. Test passes as long as
                # server aborts connection with an error.
                with self.assertRaisesRegex(
                    OSError,
                    ('certificate required'
                     '|EOF occurred'
                     '|closed by the remote host'
                     '|Connection reset by peer'
                     '|Broken pipe')
                ):
                    # receive CertificateRequest
                    data = s.recv(1024)
                    self.assertEqual(data, b'OK\n')

                    # send empty Certificate + Finish
                    s.write(b'HASCERT')

                    # receive alert
                    s.recv(1024)

    def test_pha_optional(self):
        if support.verbose:
            sys.stdout.write("\n")

        client_context, server_context, hostname = testing_context()
        server_context.post_handshake_auth = True
        server_context.verify_mode = ssl.CERT_REQUIRED
        client_context.post_handshake_auth = True
        client_context.load_cert_chain(SIGNED_CERTFILE)

        # check CERT_OPTIONAL
        server_context.verify_mode = ssl.CERT_OPTIONAL
        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                s.write(b'HASCERT')
                self.assertEqual(s.recv(1024), b'FALSE\n')
                s.write(b'PHA')
                self.assertEqual(s.recv(1024), b'OK\n')
                s.write(b'HASCERT')
                self.assertEqual(s.recv(1024), b'TRUE\n')

    def test_pha_optional_nocert(self):
        if support.verbose:
            sys.stdout.write("\n")

        client_context, server_context, hostname = testing_context()
        server_context.post_handshake_auth = True
        server_context.verify_mode = ssl.CERT_OPTIONAL
        client_context.post_handshake_auth = True

        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                s.write(b'HASCERT')
                self.assertEqual(s.recv(1024), b'FALSE\n')
                s.write(b'PHA')
                self.assertEqual(s.recv(1024), b'OK\n')
                # optional doesn't fail when client does not have a cert
                s.write(b'HASCERT')
                self.assertEqual(s.recv(1024), b'FALSE\n')

    def test_pha_no_pha_client(self):
        client_context, server_context, hostname = testing_context()
        server_context.post_handshake_auth = True
        server_context.verify_mode = ssl.CERT_REQUIRED
        client_context.load_cert_chain(SIGNED_CERTFILE)

        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                with self.assertRaisesRegex(ssl.SSLError, 'not server'):
                    s.verify_client_post_handshake()
                s.write(b'PHA')
                self.assertIn(b'extension not received', s.recv(1024))

    def test_pha_no_pha_server(self):
        # server doesn't have PHA enabled, cert is requested in handshake
        client_context, server_context, hostname = testing_context()
        server_context.verify_mode = ssl.CERT_REQUIRED
        client_context.post_handshake_auth = True
        client_context.load_cert_chain(SIGNED_CERTFILE)

        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                s.write(b'HASCERT')
                self.assertEqual(s.recv(1024), b'TRUE\n')
                # PHA doesn't fail if there is already a cert
                s.write(b'PHA')
                self.assertEqual(s.recv(1024), b'OK\n')
                s.write(b'HASCERT')
                self.assertEqual(s.recv(1024), b'TRUE\n')

    def test_pha_not_tls13(self):
        # TLS 1.2
        client_context, server_context, hostname = testing_context()
        server_context.verify_mode = ssl.CERT_REQUIRED
        client_context.maximum_version = ssl.TLSVersion.TLSv1_2
        client_context.post_handshake_auth = True
        client_context.load_cert_chain(SIGNED_CERTFILE)

        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                # PHA fails for TLS != 1.3
                s.write(b'PHA')
                self.assertIn(b'WRONG_SSL_VERSION', s.recv(1024))

    def test_bpo37428_pha_cert_none(self):
        # verify that post_handshake_auth does not implicitly enable cert
        # validation.
        hostname = SIGNED_CERTFILE_HOSTNAME
        client_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        client_context.post_handshake_auth = True
        client_context.load_cert_chain(SIGNED_CERTFILE)
        # no cert validation and CA on client side
        client_context.check_hostname = False
        client_context.verify_mode = ssl.CERT_NONE

        server_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        server_context.load_cert_chain(SIGNED_CERTFILE)
        server_context.load_verify_locations(SIGNING_CA)
        server_context.post_handshake_auth = True
        server_context.verify_mode = ssl.CERT_REQUIRED

        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
                s.write(b'HASCERT')
                self.assertEqual(s.recv(1024), b'FALSE\n')
                s.write(b'PHA')
                self.assertEqual(s.recv(1024), b'OK\n')
                s.write(b'HASCERT')
                self.assertEqual(s.recv(1024), b'TRUE\n')
                # server cert has not been validated
                self.assertEqual(s.getpeercert(), {})

    def test_internal_chain_client(self):
        client_context, server_context, hostname = testing_context(
            server_chain=False
        )
        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(
                socket.socket(),
                server_hostname=hostname
            ) as s:
                s.connect((HOST, server.port))
                vc = s._sslobj.get_verified_chain()
                self.assertEqual(len(vc), 2)
                ee, ca = vc
                uvc = s._sslobj.get_unverified_chain()
                self.assertEqual(len(uvc), 1)

                self.assertEqual(ee, uvc[0])
                self.assertEqual(hash(ee), hash(uvc[0]))
                self.assertEqual(repr(ee), repr(uvc[0]))

                self.assertNotEqual(ee, ca)
                self.assertNotEqual(hash(ee), hash(ca))
                self.assertNotEqual(repr(ee), repr(ca))
                self.assertNotEqual(ee.get_info(), ca.get_info())
                self.assertIn("CN=localhost", repr(ee))
                self.assertIn("CN=our-ca-server", repr(ca))

                pem = ee.public_bytes(_ssl.ENCODING_PEM)
                der = ee.public_bytes(_ssl.ENCODING_DER)
                self.assertIsInstance(pem, str)
                self.assertIn("-----BEGIN CERTIFICATE-----", pem)
                self.assertIsInstance(der, bytes)
                self.assertEqual(
                    ssl.PEM_cert_to_DER_cert(pem), der
                )

    def test_certificate_chain(self):
        client_context, server_context, hostname = testing_context(
            server_chain=False
        )
        server = ThreadedEchoServer(context=server_context, chatty=False)

        with open(SIGNING_CA) as f:
            expected_ca_cert = ssl.PEM_cert_to_DER_cert(f.read())

        with open(SINGED_CERTFILE_ONLY) as f:
            expected_ee_cert = ssl.PEM_cert_to_DER_cert(f.read())

        with server:
            with client_context.wrap_socket(
                socket.socket(),
                server_hostname=hostname
            ) as s:
                s.connect((HOST, server.port))
                vc = s.get_verified_chain()
                self.assertEqual(len(vc), 2)

                ee, ca = vc
                self.assertIsInstance(ee, bytes)
                self.assertIsInstance(ca, bytes)
                self.assertEqual(expected_ca_cert, ca)
                self.assertEqual(expected_ee_cert, ee)

                uvc = s.get_unverified_chain()
                self.assertEqual(len(uvc), 1)
                self.assertIsInstance(uvc[0], bytes)

                self.assertEqual(ee, uvc[0])
                self.assertNotEqual(ee, ca)

    def test_internal_chain_server(self):
        client_context, server_context, hostname = testing_context()
        client_context.load_cert_chain(SIGNED_CERTFILE)
        server_context.verify_mode = ssl.CERT_REQUIRED
        server_context.maximum_version = ssl.TLSVersion.TLSv1_2

        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(
                socket.socket(),
                server_hostname=hostname
            ) as s:
                s.connect((HOST, server.port))
                s.write(b'VERIFIEDCHAIN\n')
                res = s.recv(1024)
                self.assertEqual(res, b'\x02\n')
                s.write(b'UNVERIFIEDCHAIN\n')
                res = s.recv(1024)
                self.assertEqual(res, b'\x02\n')


HAS_KEYLOG = hasattr(ssl.SSLContext, 'keylog_filename')
requires_keylog = unittest.skipUnless(
    HAS_KEYLOG, 'test requires OpenSSL 1.1.1 with keylog callback')

class TestSSLDebug(unittest.TestCase):

    def keylog_lines(self, fname=os_helper.TESTFN):
        with open(fname) as f:
            return len(list(f))

    @requires_keylog
    def test_keylog_defaults(self):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertEqual(ctx.keylog_filename, None)

        self.assertFalse(os.path.isfile(os_helper.TESTFN))
        try:
            ctx.keylog_filename = os_helper.TESTFN
        except RuntimeError:
            if Py_DEBUG_WIN32:
                self.skipTest("not supported on Win32 debug build")
            raise
        self.assertEqual(ctx.keylog_filename, os_helper.TESTFN)
        self.assertTrue(os.path.isfile(os_helper.TESTFN))
        self.assertEqual(self.keylog_lines(), 1)

        ctx.keylog_filename = None
        self.assertEqual(ctx.keylog_filename, None)

        with self.assertRaises((IsADirectoryError, PermissionError)):
            # Windows raises PermissionError
            ctx.keylog_filename = os.path.dirname(
                os.path.abspath(os_helper.TESTFN))

        with self.assertRaises(TypeError):
            ctx.keylog_filename = 1

    @requires_keylog
    def test_keylog_filename(self):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        client_context, server_context, hostname = testing_context()

        try:
            client_context.keylog_filename = os_helper.TESTFN
        except RuntimeError:
            if Py_DEBUG_WIN32:
                self.skipTest("not supported on Win32 debug build")
            raise

        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
        # header, 5 lines for TLS 1.3
        self.assertEqual(self.keylog_lines(), 6)

        client_context.keylog_filename = None
        server_context.keylog_filename = os_helper.TESTFN
        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
        self.assertGreaterEqual(self.keylog_lines(), 11)

        client_context.keylog_filename = os_helper.TESTFN
        server_context.keylog_filename = os_helper.TESTFN
        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
        self.assertGreaterEqual(self.keylog_lines(), 21)

        client_context.keylog_filename = None
        server_context.keylog_filename = None

    @requires_keylog
    @unittest.skipIf(sys.flags.ignore_environment,
                     "test is not compatible with ignore_environment")
    def test_keylog_env(self):
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        with unittest.mock.patch.dict(os.environ):
            os.environ['SSLKEYLOGFILE'] = os_helper.TESTFN
            self.assertEqual(os.environ['SSLKEYLOGFILE'], os_helper.TESTFN)

            ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
            self.assertEqual(ctx.keylog_filename, None)

            try:
                ctx = ssl.create_default_context()
            except RuntimeError:
                if Py_DEBUG_WIN32:
                    self.skipTest("not supported on Win32 debug build")
                raise
            self.assertEqual(ctx.keylog_filename, os_helper.TESTFN)

            ctx = ssl._create_stdlib_context()
            self.assertEqual(ctx.keylog_filename, os_helper.TESTFN)

    def test_msg_callback(self):
        client_context, server_context, hostname = testing_context()

        def msg_cb(conn, direction, version, content_type, msg_type, data):
            pass

        self.assertIs(client_context._msg_callback, None)
        client_context._msg_callback = msg_cb
        self.assertIs(client_context._msg_callback, msg_cb)
        with self.assertRaises(TypeError):
            client_context._msg_callback = object()

    def test_msg_callback_tls12(self):
        client_context, server_context, hostname = testing_context()
        client_context.maximum_version = ssl.TLSVersion.TLSv1_2

        msg = []

        def msg_cb(conn, direction, version, content_type, msg_type, data):
            self.assertIsInstance(conn, ssl.SSLSocket)
            self.assertIsInstance(data, bytes)
            self.assertIn(direction, {'read', 'write'})
            msg.append((direction, version, content_type, msg_type))

        client_context._msg_callback = msg_cb

        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))

        self.assertIn(
            ("read", TLSVersion.TLSv1_2, _TLSContentType.HANDSHAKE,
             _TLSMessageType.SERVER_KEY_EXCHANGE),
            msg
        )
        self.assertIn(
            ("write", TLSVersion.TLSv1_2, _TLSContentType.CHANGE_CIPHER_SPEC,
             _TLSMessageType.CHANGE_CIPHER_SPEC),
            msg
        )

    def test_msg_callback_deadlock_bpo43577(self):
        client_context, server_context, hostname = testing_context()
        server_context2 = testing_context()[1]

        def msg_cb(conn, direction, version, content_type, msg_type, data):
            pass

        def sni_cb(sock, servername, ctx):
            sock.context = server_context2

        server_context._msg_callback = msg_cb
        server_context.sni_callback = sni_cb

        server = ThreadedEchoServer(context=server_context, chatty=False)
        with server:
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))
            with client_context.wrap_socket(socket.socket(),
                                            server_hostname=hostname) as s:
                s.connect((HOST, server.port))


def set_socket_so_linger_on_with_zero_timeout(sock):
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii', 1, 0))


class TestPreHandshakeClose(unittest.TestCase):
    """Verify behavior of close sockets with received data before to the handshake.
    """

    class SingleConnectionTestServerThread(threading.Thread):

        def __init__(self, *, name, call_after_accept, timeout=None):
            self.call_after_accept = call_after_accept
            self.received_data = b''  # set by .run()
            self.wrap_error = None  # set by .run()
            self.listener = None  # set by .start()
            self.port = None  # set by .start()
            if timeout is None:
                self.timeout = support.SHORT_TIMEOUT
            else:
                self.timeout = timeout
            super().__init__(name=name)

        def __enter__(self):
            self.start()
            return self

        def __exit__(self, *args):
            try:
                if self.listener:
                    self.listener.close()
            except OSError:
                pass
            self.join()
            self.wrap_error = None  # avoid dangling references

        def start(self):
            self.ssl_ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
            self.ssl_ctx.verify_mode = ssl.CERT_REQUIRED
            self.ssl_ctx.load_verify_locations(cafile=ONLYCERT)
            self.ssl_ctx.load_cert_chain(certfile=ONLYCERT, keyfile=ONLYKEY)
            self.listener = socket.socket()
            self.port = socket_helper.bind_port(self.listener)
            self.listener.settimeout(self.timeout)
            self.listener.listen(1)
            super().start()

        def run(self):
            try:
                conn, address = self.listener.accept()
            except TimeoutError:
                # on timeout, just close the listener
                return
            finally:
                self.listener.close()

            with conn:
                if self.call_after_accept(conn):
                    return
                try:
                    tls_socket = self.ssl_ctx.wrap_socket(conn, server_side=True)
                except OSError as err:  # ssl.SSLError inherits from OSError
                    self.wrap_error = err
                else:
                    try:
                        self.received_data = tls_socket.recv(400)
                    except OSError:
                        pass  # closed, protocol error, etc.

    def non_linux_skip_if_other_okay_error(self, err):
        if sys.platform in ("linux", "android"):
            return  # Expect the full test setup to always work on Linux.
        if (isinstance(err, ConnectionResetError) or
            (isinstance(err, OSError) and err.errno == errno.EINVAL) or
            re.search('wrong.version.number', str(getattr(err, "reason", "")), re.I)):
            # On Windows the TCP RST leads to a ConnectionResetError
            # (ECONNRESET) which Linux doesn't appear to surface to userspace.
            # If wrap_socket() winds up on the "if connected:" path and doing
            # the actual wrapping... we get an SSLError from OpenSSL. Typically
            # WRONG_VERSION_NUMBER. While appropriate, neither is the scenario
            # we're specifically trying to test. The way this test is written
            # is known to work on Linux. We'll skip it anywhere else that it
            # does not present as doing so.
            try:
                self.skipTest(f"Could not recreate conditions on {sys.platform}:"
                              f" {err=}")
            finally:
                # gh-108342: Explicitly break the reference cycle
                err = None

        # If maintaining this conditional winds up being a problem.
        # just turn this into an unconditional skip anything but Linux.
        # The important thing is that our CI has the logic covered.

    def test_preauth_data_to_tls_server(self):
        server_accept_called = threading.Event()
        ready_for_server_wrap_socket = threading.Event()

        def call_after_accept(unused):
            server_accept_called.set()
            if not ready_for_server_wrap_socket.wait(support.SHORT_TIMEOUT):
                raise RuntimeError("wrap_socket event never set, test may fail.")
            return False  # Tell the server thread to continue.

        server = self.SingleConnectionTestServerThread(
                call_after_accept=call_after_accept,
                name="preauth_data_to_tls_server")
        self.enterContext(server)  # starts it & unittest.TestCase stops it.

        with socket.socket() as client:
            client.connect(server.listener.getsockname())
            # This forces an immediate connection close via RST on .close().
            set_socket_so_linger_on_with_zero_timeout(client)
            client.setblocking(False)

            server_accept_called.wait()
            client.send(b"DELETE /data HTTP/1.0\r\n\r\n")
            client.close()  # RST

        ready_for_server_wrap_socket.set()
        server.join()

        wrap_error = server.wrap_error
        server.wrap_error = None
        try:
            self.assertEqual(b"", server.received_data)
            self.assertIsInstance(wrap_error, OSError)  # All platforms.
            self.non_linux_skip_if_other_okay_error(wrap_error)
            self.assertIsInstance(wrap_error, ssl.SSLError)
            self.assertIn("before TLS handshake with data", wrap_error.args[1])
            self.assertIn("before TLS handshake with data", wrap_error.reason)
            self.assertNotEqual(0, wrap_error.args[0])
            self.assertIsNone(wrap_error.library, msg="attr must exist")
        finally:
            # gh-108342: Explicitly break the reference cycle
            wrap_error = None
            server = None

    def test_preauth_data_to_tls_client(self):
        server_can_continue_with_wrap_socket = threading.Event()
        client_can_continue_with_wrap_socket = threading.Event()

        def call_after_accept(conn_to_client):
            if not server_can_continue_with_wrap_socket.wait(support.SHORT_TIMEOUT):
                print("ERROR: test client took too long")

            # This forces an immediate connection close via RST on .close().
            set_socket_so_linger_on_with_zero_timeout(conn_to_client)
            conn_to_client.send(
                    b"HTTP/1.0 307 Temporary Redirect\r\n"
                    b"Location: https://example.com/someone-elses-server\r\n"
                    b"\r\n")
            conn_to_client.close()  # RST
            client_can_continue_with_wrap_socket.set()
            return True  # Tell the server to stop.

        server = self.SingleConnectionTestServerThread(
                call_after_accept=call_after_accept,
                name="preauth_data_to_tls_client")
        self.enterContext(server)  # starts it & unittest.TestCase stops it.
        # Redundant; call_after_accept sets SO_LINGER on the accepted conn.
        set_socket_so_linger_on_with_zero_timeout(server.listener)

        with socket.socket() as client:
            client.connect(server.listener.getsockname())
            server_can_continue_with_wrap_socket.set()

            if not client_can_continue_with_wrap_socket.wait(support.SHORT_TIMEOUT):
                self.fail("test server took too long")
            ssl_ctx = ssl.create_default_context()
            try:
                tls_client = ssl_ctx.wrap_socket(
                        client, server_hostname="localhost")
            except OSError as err:  # SSLError inherits from OSError
                wrap_error = err
                received_data = b""
            else:
                wrap_error = None
                received_data = tls_client.recv(400)
                tls_client.close()

        server.join()
        try:
            self.assertEqual(b"", received_data)
            self.assertIsInstance(wrap_error, OSError)  # All platforms.
            self.non_linux_skip_if_other_okay_error(wrap_error)
            self.assertIsInstance(wrap_error, ssl.SSLError)
            self.assertIn("before TLS handshake with data", wrap_error.args[1])
            self.assertIn("before TLS handshake with data", wrap_error.reason)
            self.assertNotEqual(0, wrap_error.args[0])
            self.assertIsNone(wrap_error.library, msg="attr must exist")
        finally:
            # gh-108342: Explicitly break the reference cycle
            with warnings_helper.check_no_resource_warning(self):
                wrap_error = None
            server = None

    def test_https_client_non_tls_response_ignored(self):
        server_responding = threading.Event()

        class SynchronizedHTTPSConnection(http.client.HTTPSConnection):
            def connect(self):
                # Call clear text HTTP connect(), not the encrypted HTTPS (TLS)
                # connect(): wrap_socket() is called manually below.
                http.client.HTTPConnection.connect(self)

                # Wait for our fault injection server to have done its thing.
                if not server_responding.wait(support.SHORT_TIMEOUT) and support.verbose:
                    sys.stdout.write("server_responding event never set.")
                self.sock = self._context.wrap_socket(
                        self.sock, server_hostname=self.host)

        def call_after_accept(conn_to_client):
            # This forces an immediate connection close via RST on .close().
            set_socket_so_linger_on_with_zero_timeout(conn_to_client)
            conn_to_client.send(
                    b"HTTP/1.0 402 Payment Required\r\n"
                    b"\r\n")
            conn_to_client.close()  # RST
            server_responding.set()
            return True  # Tell the server to stop.

        timeout = 2.0
        server = self.SingleConnectionTestServerThread(
                call_after_accept=call_after_accept,
                name="non_tls_http_RST_responder",
                timeout=timeout)
        self.enterContext(server)  # starts it & unittest.TestCase stops it.
        # Redundant; call_after_accept sets SO_LINGER on the accepted conn.
        set_socket_so_linger_on_with_zero_timeout(server.listener)

        connection = SynchronizedHTTPSConnection(
                server.listener.getsockname()[0],
                port=server.port,
                context=ssl.create_default_context(),
                timeout=timeout,
        )

        # There are lots of reasons this raises as desired, long before this
        # test was added. Sending the request requires a successful TLS wrapped
        # socket; that fails if the connection is broken. It may seem pointless
        # to test this. It serves as an illustration of something that we never
        # want to happen... properly not happening.
        with warnings_helper.check_no_resource_warning(self), \
                self.assertRaises(OSError):
            connection.request("HEAD", "/test", headers={"Host": "localhost"})
            response = connection.getresponse()

        server.join()


class TestEnumerations(unittest.TestCase):

    def test_tlsversion(self):
        class CheckedTLSVersion(enum.IntEnum):
            MINIMUM_SUPPORTED = _ssl.PROTO_MINIMUM_SUPPORTED
            SSLv3 = _ssl.PROTO_SSLv3
            TLSv1 = _ssl.PROTO_TLSv1
            TLSv1_1 = _ssl.PROTO_TLSv1_1
            TLSv1_2 = _ssl.PROTO_TLSv1_2
            TLSv1_3 = _ssl.PROTO_TLSv1_3
            MAXIMUM_SUPPORTED = _ssl.PROTO_MAXIMUM_SUPPORTED
        enum._test_simple_enum(CheckedTLSVersion, TLSVersion)

    def test_tlscontenttype(self):
        class Checked_TLSContentType(enum.IntEnum):
            """Content types (record layer)

            See RFC 8446, section B.1
            """
            CHANGE_CIPHER_SPEC = 20
            ALERT = 21
            HANDSHAKE = 22
            APPLICATION_DATA = 23
            # pseudo content types
            HEADER = 0x100
            INNER_CONTENT_TYPE = 0x101
        enum._test_simple_enum(Checked_TLSContentType, _TLSContentType)

    def test_tlsalerttype(self):
        class Checked_TLSAlertType(enum.IntEnum):
            """Alert types for TLSContentType.ALERT messages

            See RFC 8466, section B.2
            """
            CLOSE_NOTIFY = 0
            UNEXPECTED_MESSAGE = 10
            BAD_RECORD_MAC = 20
            DECRYPTION_FAILED = 21
            RECORD_OVERFLOW = 22
            DECOMPRESSION_FAILURE = 30
            HANDSHAKE_FAILURE = 40
            NO_CERTIFICATE = 41
            BAD_CERTIFICATE = 42
            UNSUPPORTED_CERTIFICATE = 43
            CERTIFICATE_REVOKED = 44
            CERTIFICATE_EXPIRED = 45
            CERTIFICATE_UNKNOWN = 46
            ILLEGAL_PARAMETER = 47
            UNKNOWN_CA = 48
            ACCESS_DENIED = 49
            DECODE_ERROR = 50
            DECRYPT_ERROR = 51
            EXPORT_RESTRICTION = 60
            PROTOCOL_VERSION = 70
            INSUFFICIENT_SECURITY = 71
            INTERNAL_ERROR = 80
            INAPPROPRIATE_FALLBACK = 86
            USER_CANCELED = 90
            NO_RENEGOTIATION = 100
            MISSING_EXTENSION = 109
            UNSUPPORTED_EXTENSION = 110
            CERTIFICATE_UNOBTAINABLE = 111
            UNRECOGNIZED_NAME = 112
            BAD_CERTIFICATE_STATUS_RESPONSE = 113
            BAD_CERTIFICATE_HASH_VALUE = 114
            UNKNOWN_PSK_IDENTITY = 115
            CERTIFICATE_REQUIRED = 116
            NO_APPLICATION_PROTOCOL = 120
        enum._test_simple_enum(Checked_TLSAlertType, _TLSAlertType)

    def test_tlsmessagetype(self):
        class Checked_TLSMessageType(enum.IntEnum):
            """Message types (handshake protocol)

            See RFC 8446, section B.3
            """
            HELLO_REQUEST = 0
            CLIENT_HELLO = 1
            SERVER_HELLO = 2
            HELLO_VERIFY_REQUEST = 3
            NEWSESSION_TICKET = 4
            END_OF_EARLY_DATA = 5
            HELLO_RETRY_REQUEST = 6
            ENCRYPTED_EXTENSIONS = 8
            CERTIFICATE = 11
            SERVER_KEY_EXCHANGE = 12
            CERTIFICATE_REQUEST = 13
            SERVER_DONE = 14
            CERTIFICATE_VERIFY = 15
            CLIENT_KEY_EXCHANGE = 16
            FINISHED = 20
            CERTIFICATE_URL = 21
            CERTIFICATE_STATUS = 22
            SUPPLEMENTAL_DATA = 23
            KEY_UPDATE = 24
            NEXT_PROTO = 67
            MESSAGE_HASH = 254
            CHANGE_CIPHER_SPEC = 0x0101
        enum._test_simple_enum(Checked_TLSMessageType, _TLSMessageType)

    def test_sslmethod(self):
        Checked_SSLMethod = enum._old_convert_(
                enum.IntEnum, '_SSLMethod', 'ssl',
                lambda name: name.startswith('PROTOCOL_') and name != 'PROTOCOL_SSLv23',
                source=ssl._ssl,
                )
        # This member is assigned dynamically in `ssl.py`:
        Checked_SSLMethod.PROTOCOL_SSLv23 = Checked_SSLMethod.PROTOCOL_TLS
        enum._test_simple_enum(Checked_SSLMethod, ssl._SSLMethod)

    def test_options(self):
        CheckedOptions = enum._old_convert_(
                enum.IntFlag, 'Options', 'ssl',
                lambda name: name.startswith('OP_'),
                source=ssl._ssl,
                )
        enum._test_simple_enum(CheckedOptions, ssl.Options)

    def test_alertdescription(self):
        CheckedAlertDescription = enum._old_convert_(
                enum.IntEnum, 'AlertDescription', 'ssl',
                lambda name: name.startswith('ALERT_DESCRIPTION_'),
                source=ssl._ssl,
                )
        enum._test_simple_enum(CheckedAlertDescription, ssl.AlertDescription)

    def test_sslerrornumber(self):
        Checked_SSLErrorNumber = enum._old_convert_(
                enum.IntEnum, 'SSLErrorNumber', 'ssl',
                lambda name: name.startswith('SSL_ERROR_'),
                source=ssl._ssl,
                )
        enum._test_simple_enum(Checked_SSLErrorNumber, ssl.SSLErrorNumber)

    def test_verifyflags(self):
        CheckedVerifyFlags = enum._old_convert_(
                enum.IntFlag, 'VerifyFlags', 'ssl',
                lambda name: name.startswith('VERIFY_'),
                source=ssl._ssl,
                )
        enum._test_simple_enum(CheckedVerifyFlags, ssl.VerifyFlags)

    def test_verifymode(self):
        CheckedVerifyMode = enum._old_convert_(
                enum.IntEnum, 'VerifyMode', 'ssl',
                lambda name: name.startswith('CERT_'),
                source=ssl._ssl,
                )
        enum._test_simple_enum(CheckedVerifyMode, ssl.VerifyMode)


def setUpModule():
    if support.verbose:
        plats = {
            'Mac': platform.mac_ver,
            'Windows': platform.win32_ver,
        }
        for name, func in plats.items():
            plat = func()
            if plat and plat[0]:
                plat = '%s %r' % (name, plat)
                break
        else:
            plat = repr(platform.platform())
        print("test_ssl: testing with %r %r" %
            (ssl.OPENSSL_VERSION, ssl.OPENSSL_VERSION_INFO))
        print("          under %s" % plat)
        print("          HAS_SNI = %r" % ssl.HAS_SNI)
        print("          OP_ALL = 0x%8x" % ssl.OP_ALL)
        try:
            print("          OP_NO_TLSv1_1 = 0x%8x" % ssl.OP_NO_TLSv1_1)
        except AttributeError:
            pass

    for filename in [
        CERTFILE, BYTES_CERTFILE,
        ONLYCERT, ONLYKEY, BYTES_ONLYCERT, BYTES_ONLYKEY,
        SIGNED_CERTFILE, SIGNED_CERTFILE2, SIGNING_CA,
        BADCERT, BADKEY, EMPTYCERT]:
        if not os.path.exists(filename):
            raise support.TestFailed("Can't read certificate file %r" % filename)

    thread_info = threading_helper.threading_setup()
    unittest.addModuleCleanup(threading_helper.threading_cleanup, *thread_info)


if __name__ == "__main__":
    unittest.main()
