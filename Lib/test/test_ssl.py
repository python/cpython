# Test the support for SSL and sockets

import sys
import unittest
from test import support
import socket
import select
import time
import datetime
import gc
import os
import errno
import pprint
import tempfile
import urllib.request
import traceback
import asyncore
import weakref
import platform
import functools
from unittest import mock

ssl = support.import_module("ssl")

PROTOCOLS = sorted(ssl._PROTOCOL_NAMES)
HOST = support.HOST

def data_file(*name):
    return os.path.join(os.path.dirname(__file__), *name)

# The custom key and certificate files used in test_ssl are generated
# using Lib/test/make_ssl_certs.py.
# Other certificates are simply fetched from the Internet servers they
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


# empty CRL
CRLFILE = data_file("revocation.crl")

# Two keys and certs signed by the same CA (for SNI tests)
SIGNED_CERTFILE = data_file("keycert3.pem")
SIGNED_CERTFILE2 = data_file("keycert4.pem")
SIGNING_CA = data_file("pycacert.pem")

SVN_PYTHON_ORG_ROOT_CERT = data_file("https_svn_python_org_root.pem")

EMPTYCERT = data_file("nullcert.pem")
BADCERT = data_file("badcert.pem")
WRONGCERT = data_file("XXXnonexisting.pem")
BADKEY = data_file("badkey.pem")
NOKIACERT = data_file("nokia.pem")
NULLBYTECERT = data_file("nullbytecert.pem")

DHFILE = data_file("dh512.pem")
BYTES_DHFILE = os.fsencode(DHFILE)


def handle_error(prefix):
    exc_format = ' '.join(traceback.format_exception(*sys.exc_info()))
    if support.verbose:
        sys.stdout.write(prefix + exc_format)

def can_clear_options():
    # 0.9.8m or higher
    return ssl._OPENSSL_API_VERSION >= (0, 9, 8, 13, 15)

def no_sslv2_implies_sslv3_hello():
    # 0.9.7h or higher
    return ssl.OPENSSL_VERSION_INFO >= (0, 9, 7, 8, 15)

def have_verify_flags():
    # 0.9.8 or higher
    return ssl.OPENSSL_VERSION_INFO >= (0, 9, 8, 0, 15)

def asn1time(cert_time):
    # Some versions of OpenSSL ignore seconds, see #18207
    # 0.9.8.i
    if ssl._OPENSSL_API_VERSION == (0, 9, 8, 9, 15):
        fmt = "%b %d %H:%M:%S %Y GMT"
        dt = datetime.datetime.strptime(cert_time, fmt)
        dt = dt.replace(second=0)
        cert_time = dt.strftime(fmt)
        # %d adds leading zero but ASN1_TIME_print() uses leading space
        if cert_time[4] == "0":
            cert_time = cert_time[:4] + " " + cert_time[5:]

    return cert_time

# Issue #9415: Ubuntu hijacks their OpenSSL and forcefully disables SSLv2
def skip_if_broken_ubuntu_ssl(func):
    if hasattr(ssl, 'PROTOCOL_SSLv2'):
        @functools.wraps(func)
        def f(*args, **kwargs):
            try:
                ssl.SSLContext(ssl.PROTOCOL_SSLv2)
            except ssl.SSLError:
                if (ssl.OPENSSL_VERSION_INFO == (0, 9, 8, 15, 15) and
                    platform.linux_distribution() == ('debian', 'squeeze/sid', '')):
                    raise unittest.SkipTest("Patched Ubuntu OpenSSL breaks behaviour")
            return func(*args, **kwargs)
        return f
    else:
        return func

needs_sni = unittest.skipUnless(ssl.HAS_SNI, "SNI support needed for this test")


class BasicSocketTests(unittest.TestCase):

    def test_constants(self):
        ssl.CERT_NONE
        ssl.CERT_OPTIONAL
        ssl.CERT_REQUIRED
        ssl.OP_CIPHER_SERVER_PREFERENCE
        ssl.OP_SINGLE_DH_USE
        if ssl.HAS_ECDH:
            ssl.OP_SINGLE_ECDH_USE
        if ssl.OPENSSL_VERSION_INFO >= (1, 0):
            ssl.OP_NO_COMPRESSION
        self.assertIn(ssl.HAS_SNI, {True, False})
        self.assertIn(ssl.HAS_ECDH, {True, False})

    def test_random(self):
        v = ssl.RAND_status()
        if support.verbose:
            sys.stdout.write("\n RAND_status is %d (%s)\n"
                             % (v, (v and "sufficient randomness") or
                                "insufficient randomness"))

        data, is_cryptographic = ssl.RAND_pseudo_bytes(16)
        self.assertEqual(len(data), 16)
        self.assertEqual(is_cryptographic, v == 1)
        if v:
            data = ssl.RAND_bytes(16)
            self.assertEqual(len(data), 16)
        else:
            self.assertRaises(ssl.SSLError, ssl.RAND_bytes, 16)

        # negative num is invalid
        self.assertRaises(ValueError, ssl.RAND_bytes, -5)
        self.assertRaises(ValueError, ssl.RAND_pseudo_bytes, -5)

        self.assertRaises(TypeError, ssl.RAND_egd, 1)
        self.assertRaises(TypeError, ssl.RAND_egd, 'foo', 1)
        ssl.RAND_add("this is a random string", 75.0)

    @unittest.skipUnless(os.name == 'posix', 'requires posix')
    def test_random_fork(self):
        status = ssl.RAND_status()
        if not status:
            self.fail("OpenSSL's PRNG has insufficient randomness")

        rfd, wfd = os.pipe()
        pid = os.fork()
        if pid == 0:
            try:
                os.close(rfd)
                child_random = ssl.RAND_pseudo_bytes(16)[0]
                self.assertEqual(len(child_random), 16)
                os.write(wfd, child_random)
                os.close(wfd)
            except BaseException:
                os._exit(1)
            else:
                os._exit(0)
        else:
            os.close(wfd)
            self.addCleanup(os.close, rfd)
            _, status = os.waitpid(pid, 0)
            self.assertEqual(status, 0)

            child_random = os.read(rfd, 16)
            self.assertEqual(len(child_random), 16)
            parent_random = ssl.RAND_pseudo_bytes(16)[0]
            self.assertEqual(len(parent_random), 16)

            self.assertNotEqual(child_random, parent_random)

    def test_parse_cert(self):
        # note that this uses an 'unofficial' function in _ssl.c,
        # provided solely for this test, to exercise the certificate
        # parsing code
        p = ssl._ssl._test_decode_cert(CERTFILE)
        if support.verbose:
            sys.stdout.write("\n" + pprint.pformat(p) + "\n")
        self.assertEqual(p['issuer'],
                         ((('countryName', 'XY'),),
                          (('localityName', 'Castle Anthrax'),),
                          (('organizationName', 'Python Software Foundation'),),
                          (('commonName', 'localhost'),))
                        )
        # Note the next three asserts will fail if the keys are regenerated
        self.assertEqual(p['notAfter'], asn1time('Oct  5 23:01:56 2020 GMT'))
        self.assertEqual(p['notBefore'], asn1time('Oct  8 23:01:56 2010 GMT'))
        self.assertEqual(p['serialNumber'], 'D7C7381919AFC24E')
        self.assertEqual(p['subject'],
                         ((('countryName', 'XY'),),
                          (('localityName', 'Castle Anthrax'),),
                          (('organizationName', 'Python Software Foundation'),),
                          (('commonName', 'localhost'),))
                        )
        self.assertEqual(p['subjectAltName'], (('DNS', 'localhost'),))
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
                   ('IP Address', '2001:DB8:0:0:0:0:0:1\n'))
        else:
            # OpenSSL 0.9.7 doesn't support IPv6 addresses in subjectAltName
            san = (('DNS', 'altnull.python.org\x00example.com'),
                   ('email', 'null@python.org\x00user@example.org'),
                   ('URI', 'http://null.python.org\x00http://example.org'),
                   ('IP Address', '192.0.2.1'),
                   ('IP Address', '<invalid>'))

        self.assertEqual(p['subjectAltName'], san)

    def test_DER_to_PEM(self):
        with open(SVN_PYTHON_ORG_ROOT_CERT, 'r') as f:
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
        # >= 0.9
        self.assertGreaterEqual(n, 0x900000)
        # < 2.0
        self.assertLess(n, 0x20000000)
        major, minor, fix, patch, status = t
        self.assertGreaterEqual(major, 0)
        self.assertLess(major, 2)
        self.assertGreaterEqual(minor, 0)
        self.assertLess(minor, 256)
        self.assertGreaterEqual(fix, 0)
        self.assertLess(fix, 256)
        self.assertGreaterEqual(patch, 0)
        self.assertLessEqual(patch, 26)
        self.assertGreaterEqual(status, 0)
        self.assertLessEqual(status, 15)
        # Version string as returned by OpenSSL, the format might change
        self.assertTrue(s.startswith("OpenSSL {:d}.{:d}.{:d}".format(major, minor, fix)),
                        (s, t))

    @support.cpython_only
    def test_refcycle(self):
        # Issue #7943: an SSL object doesn't create reference cycles with
        # itself.
        s = socket.socket(socket.AF_INET)
        ss = ssl.wrap_socket(s)
        wr = weakref.ref(ss)
        with support.check_warnings(("", ResourceWarning)):
            del ss
            self.assertEqual(wr(), None)

    def test_wrapped_unconnected(self):
        # Methods on an unconnected SSLSocket propagate the original
        # OSError raise by the underlying socket object.
        s = socket.socket(socket.AF_INET)
        with ssl.wrap_socket(s) as ss:
            self.assertRaises(OSError, ss.recv, 1)
            self.assertRaises(OSError, ss.recv_into, bytearray(b'x'))
            self.assertRaises(OSError, ss.recvfrom, 1)
            self.assertRaises(OSError, ss.recvfrom_into, bytearray(b'x'), 1)
            self.assertRaises(OSError, ss.send, b'x')
            self.assertRaises(OSError, ss.sendto, b'x', ('0.0.0.0', 0))

    def test_timeout(self):
        # Issue #8524: when creating an SSL socket, the timeout of the
        # original socket should be retained.
        for timeout in (None, 0.0, 5.0):
            s = socket.socket(socket.AF_INET)
            s.settimeout(timeout)
            with ssl.wrap_socket(s) as ss:
                self.assertEqual(timeout, ss.gettimeout())

    def test_errors(self):
        sock = socket.socket()
        self.assertRaisesRegex(ValueError,
                        "certfile must be specified",
                        ssl.wrap_socket, sock, keyfile=CERTFILE)
        self.assertRaisesRegex(ValueError,
                        "certfile must be specified for server-side operations",
                        ssl.wrap_socket, sock, server_side=True)
        self.assertRaisesRegex(ValueError,
                        "certfile must be specified for server-side operations",
                        ssl.wrap_socket, sock, server_side=True, certfile="")
        with ssl.wrap_socket(sock, server_side=True, certfile=CERTFILE) as s:
            self.assertRaisesRegex(ValueError, "can't connect in server-side mode",
                                    s.connect, (HOST, 8080))
        with self.assertRaises(OSError) as cm:
            with socket.socket() as sock:
                ssl.wrap_socket(sock, certfile=WRONGCERT)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        with self.assertRaises(OSError) as cm:
            with socket.socket() as sock:
                ssl.wrap_socket(sock, certfile=CERTFILE, keyfile=WRONGCERT)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        with self.assertRaises(OSError) as cm:
            with socket.socket() as sock:
                ssl.wrap_socket(sock, certfile=WRONGCERT, keyfile=WRONGCERT)
        self.assertEqual(cm.exception.errno, errno.ENOENT)

    def test_match_hostname(self):
        def ok(cert, hostname):
            ssl.match_hostname(cert, hostname)
        def fail(cert, hostname):
            self.assertRaises(ssl.CertificateError,
                              ssl.match_hostname, cert, hostname)

        cert = {'subject': ((('commonName', 'example.com'),),)}
        ok(cert, 'example.com')
        ok(cert, 'ExAmple.cOm')
        fail(cert, 'www.example.com')
        fail(cert, '.example.com')
        fail(cert, 'example.org')
        fail(cert, 'exampleXcom')

        cert = {'subject': ((('commonName', '*.a.com'),),)}
        ok(cert, 'foo.a.com')
        fail(cert, 'bar.foo.a.com')
        fail(cert, 'a.com')
        fail(cert, 'Xa.com')
        fail(cert, '.a.com')

        # only match one left-most wildcard
        cert = {'subject': ((('commonName', 'f*.com'),),)}
        ok(cert, 'foo.com')
        ok(cert, 'f.com')
        fail(cert, 'bar.com')
        fail(cert, 'foo.a.com')
        fail(cert, 'bar.foo.com')

        # NULL bytes are bad, CVE-2013-4073
        cert = {'subject': ((('commonName',
                              'null.python.org\x00example.org'),),)}
        ok(cert, 'null.python.org\x00example.org') # or raise an error?
        fail(cert, 'example.org')
        fail(cert, 'null.python.org')

        # error cases with wildcards
        cert = {'subject': ((('commonName', '*.*.a.com'),),)}
        fail(cert, 'bar.foo.a.com')
        fail(cert, 'a.com')
        fail(cert, 'Xa.com')
        fail(cert, '.a.com')

        cert = {'subject': ((('commonName', 'a.*.com'),),)}
        fail(cert, 'a.foo.com')
        fail(cert, 'a..com')
        fail(cert, 'a.com')

        # wildcard doesn't match IDNA prefix 'xn--'
        idna = 'püthon.python.org'.encode("idna").decode("ascii")
        cert = {'subject': ((('commonName', idna),),)}
        ok(cert, idna)
        cert = {'subject': ((('commonName', 'x*.python.org'),),)}
        fail(cert, idna)
        cert = {'subject': ((('commonName', 'xn--p*.python.org'),),)}
        fail(cert, idna)

        # wildcard in first fragment and  IDNA A-labels in sequent fragments
        # are supported.
        idna = 'www*.pythön.org'.encode("idna").decode("ascii")
        cert = {'subject': ((('commonName', idna),),)}
        ok(cert, 'www.pythön.org'.encode("idna").decode("ascii"))
        ok(cert, 'www1.pythön.org'.encode("idna").decode("ascii"))
        fail(cert, 'ftp.pythön.org'.encode("idna").decode("ascii"))
        fail(cert, 'pythön.org'.encode("idna").decode("ascii"))

        # Slightly fake real-world example
        cert = {'notAfter': 'Jun 26 21:41:46 2011 GMT',
                'subject': ((('commonName', 'linuxfrz.org'),),),
                'subjectAltName': (('DNS', 'linuxfr.org'),
                                   ('DNS', 'linuxfr.com'),
                                   ('othername', '<unsupported>'))}
        ok(cert, 'linuxfr.org')
        ok(cert, 'linuxfr.com')
        # Not a "DNS" entry
        fail(cert, '<unsupported>')
        # When there is a subjectAltName, commonName isn't used
        fail(cert, 'linuxfrz.org')

        # A pristine real-world example
        cert = {'notAfter': 'Dec 18 23:59:59 2011 GMT',
                'subject': ((('countryName', 'US'),),
                            (('stateOrProvinceName', 'California'),),
                            (('localityName', 'Mountain View'),),
                            (('organizationName', 'Google Inc'),),
                            (('commonName', 'mail.google.com'),))}
        ok(cert, 'mail.google.com')
        fail(cert, 'gmail.com')
        # Only commonName is considered
        fail(cert, 'California')

        # Neither commonName nor subjectAltName
        cert = {'notAfter': 'Dec 18 23:59:59 2011 GMT',
                'subject': ((('countryName', 'US'),),
                            (('stateOrProvinceName', 'California'),),
                            (('localityName', 'Mountain View'),),
                            (('organizationName', 'Google Inc'),))}
        fail(cert, 'mail.google.com')

        # No DNS entry in subjectAltName but a commonName
        cert = {'notAfter': 'Dec 18 23:59:59 2099 GMT',
                'subject': ((('countryName', 'US'),),
                            (('stateOrProvinceName', 'California'),),
                            (('localityName', 'Mountain View'),),
                            (('commonName', 'mail.google.com'),)),
                'subjectAltName': (('othername', 'blabla'), )}
        ok(cert, 'mail.google.com')

        # No DNS entry subjectAltName and no commonName
        cert = {'notAfter': 'Dec 18 23:59:59 2099 GMT',
                'subject': ((('countryName', 'US'),),
                            (('stateOrProvinceName', 'California'),),
                            (('localityName', 'Mountain View'),),
                            (('organizationName', 'Google Inc'),)),
                'subjectAltName': (('othername', 'blabla'),)}
        fail(cert, 'google.com')

        # Empty cert / no cert
        self.assertRaises(ValueError, ssl.match_hostname, None, 'example.com')
        self.assertRaises(ValueError, ssl.match_hostname, {}, 'example.com')

        # Issue #17980: avoid denials of service by refusing more than one
        # wildcard per fragment.
        cert = {'subject': ((('commonName', 'a*b.com'),),)}
        ok(cert, 'axxb.com')
        cert = {'subject': ((('commonName', 'a*b.co*'),),)}
        fail(cert, 'axxb.com')
        cert = {'subject': ((('commonName', 'a*b*.com'),),)}
        with self.assertRaises(ssl.CertificateError) as cm:
            ssl.match_hostname(cert, 'axxbxxc.com')
        self.assertIn("too many wildcards", str(cm.exception))

    def test_server_side(self):
        # server_hostname doesn't work for server sockets
        ctx = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
        with socket.socket() as sock:
            self.assertRaises(ValueError, ctx.wrap_socket, sock, True,
                              server_hostname="some.hostname")

    def test_unknown_channel_binding(self):
        # should raise ValueError for unknown type
        s = socket.socket(socket.AF_INET)
        with ssl.wrap_socket(s) as ss:
            with self.assertRaises(ValueError):
                ss.get_channel_binding("unknown-type")

    @unittest.skipUnless("tls-unique" in ssl.CHANNEL_BINDING_TYPES,
                         "'tls-unique' channel binding not available")
    def test_tls_unique_channel_binding(self):
        # unconnected should return None for known type
        s = socket.socket(socket.AF_INET)
        with ssl.wrap_socket(s) as ss:
            self.assertIsNone(ss.get_channel_binding("tls-unique"))
        # the same for server-side
        s = socket.socket(socket.AF_INET)
        with ssl.wrap_socket(s, server_side=True, certfile=CERTFILE) as ss:
            self.assertIsNone(ss.get_channel_binding("tls-unique"))

    def test_dealloc_warn(self):
        ss = ssl.wrap_socket(socket.socket(socket.AF_INET))
        r = repr(ss)
        with self.assertWarns(ResourceWarning) as cm:
            ss = None
            support.gc_collect()
        self.assertIn(r, str(cm.warning.args[0]))

    def test_get_default_verify_paths(self):
        paths = ssl.get_default_verify_paths()
        self.assertEqual(len(paths), 6)
        self.assertIsInstance(paths, ssl.DefaultVerifyPaths)

        with support.EnvironmentVarGuard() as env:
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
                self.assertIsInstance(trust, (set, bool))
                if isinstance(trust, set):
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
            ssl.wrap_socket(s, cert_reqs=ssl.CERT_NONE)
        self.assertEqual(str(cx.exception), "only stream sockets are supported")
        ctx = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
        with self.assertRaises(NotImplementedError) as cx:
            ctx.wrap_socket(s)
        self.assertEqual(str(cx.exception), "only stream sockets are supported")


class ContextTests(unittest.TestCase):

    @skip_if_broken_ubuntu_ssl
    def test_constructor(self):
        for protocol in PROTOCOLS:
            ssl.SSLContext(protocol)
        self.assertRaises(TypeError, ssl.SSLContext)
        self.assertRaises(ValueError, ssl.SSLContext, -1)
        self.assertRaises(ValueError, ssl.SSLContext, 42)

    @skip_if_broken_ubuntu_ssl
    def test_protocol(self):
        for proto in PROTOCOLS:
            ctx = ssl.SSLContext(proto)
            self.assertEqual(ctx.protocol, proto)

    def test_ciphers(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        ctx.set_ciphers("ALL")
        ctx.set_ciphers("DEFAULT")
        with self.assertRaisesRegex(ssl.SSLError, "No cipher can be selected"):
            ctx.set_ciphers("^$:,;?*'dorothyx")

    @skip_if_broken_ubuntu_ssl
    def test_options(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        # OP_ALL | OP_NO_SSLv2 is the default value
        self.assertEqual(ssl.OP_ALL | ssl.OP_NO_SSLv2,
                         ctx.options)
        ctx.options |= ssl.OP_NO_SSLv3
        self.assertEqual(ssl.OP_ALL | ssl.OP_NO_SSLv2 | ssl.OP_NO_SSLv3,
                         ctx.options)
        if can_clear_options():
            ctx.options = (ctx.options & ~ssl.OP_NO_SSLv2) | ssl.OP_NO_TLSv1
            self.assertEqual(ssl.OP_ALL | ssl.OP_NO_TLSv1 | ssl.OP_NO_SSLv3,
                             ctx.options)
            ctx.options = 0
            self.assertEqual(0, ctx.options)
        else:
            with self.assertRaises(ValueError):
                ctx.options = 0

    def test_verify_mode(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
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

    @unittest.skipUnless(have_verify_flags(),
                         "verify_flags need OpenSSL > 0.9.8")
    def test_verify_flags(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        # default value by OpenSSL
        self.assertEqual(ctx.verify_flags, ssl.VERIFY_DEFAULT)
        ctx.verify_flags = ssl.VERIFY_CRL_CHECK_LEAF
        self.assertEqual(ctx.verify_flags, ssl.VERIFY_CRL_CHECK_LEAF)
        ctx.verify_flags = ssl.VERIFY_CRL_CHECK_CHAIN
        self.assertEqual(ctx.verify_flags, ssl.VERIFY_CRL_CHECK_CHAIN)
        ctx.verify_flags = ssl.VERIFY_DEFAULT
        self.assertEqual(ctx.verify_flags, ssl.VERIFY_DEFAULT)
        # supports any value
        ctx.verify_flags = ssl.VERIFY_CRL_CHECK_LEAF | ssl.VERIFY_X509_STRICT
        self.assertEqual(ctx.verify_flags,
                         ssl.VERIFY_CRL_CHECK_LEAF | ssl.VERIFY_X509_STRICT)
        with self.assertRaises(TypeError):
            ctx.verify_flags = None

    def test_load_cert_chain(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        # Combined key and cert in a single file
        ctx.load_cert_chain(CERTFILE)
        ctx.load_cert_chain(CERTFILE, keyfile=CERTFILE)
        self.assertRaises(TypeError, ctx.load_cert_chain, keyfile=CERTFILE)
        with self.assertRaises(OSError) as cm:
            ctx.load_cert_chain(WRONGCERT)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        with self.assertRaisesRegex(ssl.SSLError, "PEM lib"):
            ctx.load_cert_chain(BADCERT)
        with self.assertRaisesRegex(ssl.SSLError, "PEM lib"):
            ctx.load_cert_chain(EMPTYCERT)
        # Separate key and cert
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        ctx.load_cert_chain(ONLYCERT, ONLYKEY)
        ctx.load_cert_chain(certfile=ONLYCERT, keyfile=ONLYKEY)
        ctx.load_cert_chain(certfile=BYTES_ONLYCERT, keyfile=BYTES_ONLYKEY)
        with self.assertRaisesRegex(ssl.SSLError, "PEM lib"):
            ctx.load_cert_chain(ONLYCERT)
        with self.assertRaisesRegex(ssl.SSLError, "PEM lib"):
            ctx.load_cert_chain(ONLYKEY)
        with self.assertRaisesRegex(ssl.SSLError, "PEM lib"):
            ctx.load_cert_chain(certfile=ONLYKEY, keyfile=ONLYCERT)
        # Mismatching key and cert
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        with self.assertRaisesRegex(ssl.SSLError, "key values mismatch"):
            ctx.load_cert_chain(SVN_PYTHON_ORG_ROOT_CERT, ONLYKEY)
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
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        ctx.load_verify_locations(CERTFILE)
        ctx.load_verify_locations(cafile=CERTFILE, capath=None)
        ctx.load_verify_locations(BYTES_CERTFILE)
        ctx.load_verify_locations(cafile=BYTES_CERTFILE, capath=None)
        self.assertRaises(TypeError, ctx.load_verify_locations)
        self.assertRaises(TypeError, ctx.load_verify_locations, None, None, None)
        with self.assertRaises(OSError) as cm:
            ctx.load_verify_locations(WRONGCERT)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        with self.assertRaisesRegex(ssl.SSLError, "PEM lib"):
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
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 0)
        ctx.load_verify_locations(cadata=cacert_pem)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 1)
        ctx.load_verify_locations(cadata=neuronio_pem)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)
        # cert already in hash table
        ctx.load_verify_locations(cadata=neuronio_pem)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)

        # combined
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        combined = "\n".join((cacert_pem, neuronio_pem))
        ctx.load_verify_locations(cadata=combined)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)

        # with junk around the certs
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        combined = ["head", cacert_pem, "other", neuronio_pem, "again",
                    neuronio_pem, "tail"]
        ctx.load_verify_locations(cadata="\n".join(combined))
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)

        # test DER
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        ctx.load_verify_locations(cadata=cacert_der)
        ctx.load_verify_locations(cadata=neuronio_der)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)
        # cert already in hash table
        ctx.load_verify_locations(cadata=cacert_der)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)

        # combined
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        combined = b"".join((cacert_der, neuronio_der))
        ctx.load_verify_locations(cadata=combined)
        self.assertEqual(ctx.cert_store_stats()["x509_ca"], 2)

        # error cases
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        self.assertRaises(TypeError, ctx.load_verify_locations, cadata=object)

        with self.assertRaisesRegex(ssl.SSLError, "no start line"):
            ctx.load_verify_locations(cadata="broken")
        with self.assertRaisesRegex(ssl.SSLError, "not enough data"):
            ctx.load_verify_locations(cadata=b"broken")


    def test_load_dh_params(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        ctx.load_dh_params(DHFILE)
        if os.name != 'nt':
            ctx.load_dh_params(BYTES_DHFILE)
        self.assertRaises(TypeError, ctx.load_dh_params)
        self.assertRaises(TypeError, ctx.load_dh_params, None)
        with self.assertRaises(FileNotFoundError) as cm:
            ctx.load_dh_params(WRONGCERT)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        with self.assertRaises(ssl.SSLError) as cm:
            ctx.load_dh_params(CERTFILE)

    @skip_if_broken_ubuntu_ssl
    def test_session_stats(self):
        for proto in PROTOCOLS:
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
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        ctx.set_default_verify_paths()

    @unittest.skipUnless(ssl.HAS_ECDH, "ECDH disabled on this OpenSSL build")
    def test_set_ecdh_curve(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        ctx.set_ecdh_curve("prime256v1")
        ctx.set_ecdh_curve(b"prime256v1")
        self.assertRaises(TypeError, ctx.set_ecdh_curve)
        self.assertRaises(TypeError, ctx.set_ecdh_curve, None)
        self.assertRaises(ValueError, ctx.set_ecdh_curve, "foo")
        self.assertRaises(ValueError, ctx.set_ecdh_curve, b"foo")

    @needs_sni
    def test_sni_callback(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)

        # set_servername_callback expects a callable, or None
        self.assertRaises(TypeError, ctx.set_servername_callback)
        self.assertRaises(TypeError, ctx.set_servername_callback, 4)
        self.assertRaises(TypeError, ctx.set_servername_callback, "")
        self.assertRaises(TypeError, ctx.set_servername_callback, ctx)

        def dummycallback(sock, servername, ctx):
            pass
        ctx.set_servername_callback(None)
        ctx.set_servername_callback(dummycallback)

    @needs_sni
    def test_sni_callback_refcycle(self):
        # Reference cycles through the servername callback are detected
        # and cleared.
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        def dummycallback(sock, servername, ctx, cycle=ctx):
            pass
        ctx.set_servername_callback(dummycallback)
        wr = weakref.ref(ctx)
        del ctx, dummycallback
        gc.collect()
        self.assertIs(wr(), None)

    def test_cert_store_stats(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        self.assertEqual(ctx.cert_store_stats(),
            {'x509_ca': 0, 'crl': 0, 'x509': 0})
        ctx.load_cert_chain(CERTFILE)
        self.assertEqual(ctx.cert_store_stats(),
            {'x509_ca': 0, 'crl': 0, 'x509': 0})
        ctx.load_verify_locations(CERTFILE)
        self.assertEqual(ctx.cert_store_stats(),
            {'x509_ca': 0, 'crl': 0, 'x509': 1})
        ctx.load_verify_locations(SVN_PYTHON_ORG_ROOT_CERT)
        self.assertEqual(ctx.cert_store_stats(),
            {'x509_ca': 1, 'crl': 0, 'x509': 2})

    def test_get_ca_certs(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        self.assertEqual(ctx.get_ca_certs(), [])
        # CERTFILE is not flagged as X509v3 Basic Constraints: CA:TRUE
        ctx.load_verify_locations(CERTFILE)
        self.assertEqual(ctx.get_ca_certs(), [])
        # but SVN_PYTHON_ORG_ROOT_CERT is a CA cert
        ctx.load_verify_locations(SVN_PYTHON_ORG_ROOT_CERT)
        self.assertEqual(ctx.get_ca_certs(),
            [{'issuer': ((('organizationName', 'Root CA'),),
                         (('organizationalUnitName', 'http://www.cacert.org'),),
                         (('commonName', 'CA Cert Signing Authority'),),
                         (('emailAddress', 'support@cacert.org'),)),
              'notAfter': asn1time('Mar 29 12:29:49 2033 GMT'),
              'notBefore': asn1time('Mar 30 12:29:49 2003 GMT'),
              'serialNumber': '00',
              'crlDistributionPoints': ('https://www.cacert.org/revoke.crl',),
              'subject': ((('organizationName', 'Root CA'),),
                          (('organizationalUnitName', 'http://www.cacert.org'),),
                          (('commonName', 'CA Cert Signing Authority'),),
                          (('emailAddress', 'support@cacert.org'),)),
              'version': 3}])

        with open(SVN_PYTHON_ORG_ROOT_CERT) as f:
            pem = f.read()
        der = ssl.PEM_cert_to_DER_cert(pem)
        self.assertEqual(ctx.get_ca_certs(True), [der])

    def test_load_default_certs(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        ctx.load_default_certs()

        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        ctx.load_default_certs(ssl.Purpose.SERVER_AUTH)
        ctx.load_default_certs()

        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        ctx.load_default_certs(ssl.Purpose.CLIENT_AUTH)

        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        self.assertRaises(TypeError, ctx.load_default_certs, None)
        self.assertRaises(TypeError, ctx.load_default_certs, 'SERVER_AUTH')

    def test_create_default_context(self):
        ctx = ssl.create_default_context()
        self.assertEqual(ctx.protocol, ssl.PROTOCOL_SSLv23)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
        self.assertTrue(ctx.check_hostname)
        self.assertEqual(ctx.options & ssl.OP_NO_SSLv2, ssl.OP_NO_SSLv2)
        self.assertEqual(
            ctx.options & getattr(ssl, "OP_NO_COMPRESSION", 0),
            getattr(ssl, "OP_NO_COMPRESSION", 0),
        )

        with open(SIGNING_CA) as f:
            cadata = f.read()
        ctx = ssl.create_default_context(cafile=SIGNING_CA, capath=CAPATH,
                                         cadata=cadata)
        self.assertEqual(ctx.protocol, ssl.PROTOCOL_SSLv23)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
        self.assertEqual(ctx.options & ssl.OP_NO_SSLv2, ssl.OP_NO_SSLv2)
        self.assertEqual(
            ctx.options & getattr(ssl, "OP_NO_COMPRESSION", 0),
            getattr(ssl, "OP_NO_COMPRESSION", 0),
        )

        ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
        self.assertEqual(ctx.protocol, ssl.PROTOCOL_SSLv23)
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)
        self.assertEqual(ctx.options & ssl.OP_NO_SSLv2, ssl.OP_NO_SSLv2)
        self.assertEqual(
            ctx.options & getattr(ssl, "OP_NO_COMPRESSION", 0),
            getattr(ssl, "OP_NO_COMPRESSION", 0),
        )
        self.assertEqual(
            ctx.options & getattr(ssl, "OP_SINGLE_DH_USE", 0),
            getattr(ssl, "OP_SINGLE_DH_USE", 0),
        )
        self.assertEqual(
            ctx.options & getattr(ssl, "OP_SINGLE_ECDH_USE", 0),
            getattr(ssl, "OP_SINGLE_ECDH_USE", 0),
        )

    def test__create_stdlib_context(self):
        ctx = ssl._create_stdlib_context()
        self.assertEqual(ctx.protocol, ssl.PROTOCOL_SSLv23)
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)
        self.assertFalse(ctx.check_hostname)
        self.assertEqual(ctx.options & ssl.OP_NO_SSLv2, ssl.OP_NO_SSLv2)

        ctx = ssl._create_stdlib_context(ssl.PROTOCOL_TLSv1)
        self.assertEqual(ctx.protocol, ssl.PROTOCOL_TLSv1)
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)
        self.assertEqual(ctx.options & ssl.OP_NO_SSLv2, ssl.OP_NO_SSLv2)

        ctx = ssl._create_stdlib_context(ssl.PROTOCOL_TLSv1,
                                         cert_reqs=ssl.CERT_REQUIRED,
                                         check_hostname=True)
        self.assertEqual(ctx.protocol, ssl.PROTOCOL_TLSv1)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
        self.assertTrue(ctx.check_hostname)
        self.assertEqual(ctx.options & ssl.OP_NO_SSLv2, ssl.OP_NO_SSLv2)

        ctx = ssl._create_stdlib_context(purpose=ssl.Purpose.CLIENT_AUTH)
        self.assertEqual(ctx.protocol, ssl.PROTOCOL_SSLv23)
        self.assertEqual(ctx.verify_mode, ssl.CERT_NONE)
        self.assertEqual(ctx.options & ssl.OP_NO_SSLv2, ssl.OP_NO_SSLv2)

    def test_check_hostname(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        self.assertFalse(ctx.check_hostname)

        # Requires CERT_REQUIRED or CERT_OPTIONAL
        with self.assertRaises(ValueError):
            ctx.check_hostname = True
        ctx.verify_mode = ssl.CERT_REQUIRED
        self.assertFalse(ctx.check_hostname)
        ctx.check_hostname = True
        self.assertTrue(ctx.check_hostname)

        ctx.verify_mode = ssl.CERT_OPTIONAL
        ctx.check_hostname = True
        self.assertTrue(ctx.check_hostname)

        # Cannot set CERT_NONE with check_hostname enabled
        with self.assertRaises(ValueError):
            ctx.verify_mode = ssl.CERT_NONE
        ctx.check_hostname = False
        self.assertFalse(ctx.check_hostname)


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
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        with self.assertRaises(ssl.SSLError) as cm:
            ctx.load_dh_params(CERTFILE)
        self.assertEqual(cm.exception.library, 'PEM')
        self.assertEqual(cm.exception.reason, 'NO_START_LINE')
        s = str(cm.exception)
        self.assertTrue(s.startswith("[PEM: NO_START_LINE] no start line"), s)

    def test_subclass(self):
        # Check that the appropriate SSLError subclass is raised
        # (this only tests one of them)
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        with socket.socket() as s:
            s.bind(("127.0.0.1", 0))
            s.listen(5)
            c = socket.socket()
            c.connect(s.getsockname())
            c.setblocking(False)
            with ctx.wrap_socket(c, False, do_handshake_on_connect=False) as c:
                with self.assertRaises(ssl.SSLWantReadError) as cm:
                    c.do_handshake()
                s = str(cm.exception)
                self.assertTrue(s.startswith("The operation did not complete (read)"), s)
                # For compatibility
                self.assertEqual(cm.exception.errno, ssl.SSL_ERROR_WANT_READ)


class NetworkedTests(unittest.TestCase):

    def test_connect(self):
        with support.transient_internet("svn.python.org"):
            s = ssl.wrap_socket(socket.socket(socket.AF_INET),
                                cert_reqs=ssl.CERT_NONE)
            try:
                s.connect(("svn.python.org", 443))
                self.assertEqual({}, s.getpeercert())
            finally:
                s.close()

            # this should fail because we have no verification certs
            s = ssl.wrap_socket(socket.socket(socket.AF_INET),
                                cert_reqs=ssl.CERT_REQUIRED)
            self.assertRaisesRegex(ssl.SSLError, "certificate verify failed",
                                   s.connect, ("svn.python.org", 443))
            s.close()

            # this should succeed because we specify the root cert
            s = ssl.wrap_socket(socket.socket(socket.AF_INET),
                                cert_reqs=ssl.CERT_REQUIRED,
                                ca_certs=SVN_PYTHON_ORG_ROOT_CERT)
            try:
                s.connect(("svn.python.org", 443))
                self.assertTrue(s.getpeercert())
            finally:
                s.close()

    def test_connect_ex(self):
        # Issue #11326: check connect_ex() implementation
        with support.transient_internet("svn.python.org"):
            s = ssl.wrap_socket(socket.socket(socket.AF_INET),
                                cert_reqs=ssl.CERT_REQUIRED,
                                ca_certs=SVN_PYTHON_ORG_ROOT_CERT)
            try:
                self.assertEqual(0, s.connect_ex(("svn.python.org", 443)))
                self.assertTrue(s.getpeercert())
            finally:
                s.close()

    def test_non_blocking_connect_ex(self):
        # Issue #11326: non-blocking connect_ex() should allow handshake
        # to proceed after the socket gets ready.
        with support.transient_internet("svn.python.org"):
            s = ssl.wrap_socket(socket.socket(socket.AF_INET),
                                cert_reqs=ssl.CERT_REQUIRED,
                                ca_certs=SVN_PYTHON_ORG_ROOT_CERT,
                                do_handshake_on_connect=False)
            try:
                s.setblocking(False)
                rc = s.connect_ex(('svn.python.org', 443))
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
            finally:
                s.close()

    def test_timeout_connect_ex(self):
        # Issue #12065: on a timeout, connect_ex() should return the original
        # errno (mimicking the behaviour of non-SSL sockets).
        with support.transient_internet("svn.python.org"):
            s = ssl.wrap_socket(socket.socket(socket.AF_INET),
                                cert_reqs=ssl.CERT_REQUIRED,
                                ca_certs=SVN_PYTHON_ORG_ROOT_CERT,
                                do_handshake_on_connect=False)
            try:
                s.settimeout(0.0000001)
                rc = s.connect_ex(('svn.python.org', 443))
                if rc == 0:
                    self.skipTest("svn.python.org responded too quickly")
                self.assertIn(rc, (errno.EAGAIN, errno.EWOULDBLOCK))
            finally:
                s.close()

    def test_connect_ex_error(self):
        with support.transient_internet("svn.python.org"):
            s = ssl.wrap_socket(socket.socket(socket.AF_INET),
                                cert_reqs=ssl.CERT_REQUIRED,
                                ca_certs=SVN_PYTHON_ORG_ROOT_CERT)
            try:
                rc = s.connect_ex(("svn.python.org", 444))
                # Issue #19919: Windows machines or VMs hosted on Windows
                # machines sometimes return EWOULDBLOCK.
                self.assertIn(rc, (errno.ECONNREFUSED, errno.EWOULDBLOCK))
            finally:
                s.close()

    def test_connect_with_context(self):
        with support.transient_internet("svn.python.org"):
            # Same as test_connect, but with a separately created context
            ctx = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            s = ctx.wrap_socket(socket.socket(socket.AF_INET))
            s.connect(("svn.python.org", 443))
            try:
                self.assertEqual({}, s.getpeercert())
            finally:
                s.close()
            # Same with a server hostname
            s = ctx.wrap_socket(socket.socket(socket.AF_INET),
                                server_hostname="svn.python.org")
            if ssl.HAS_SNI:
                s.connect(("svn.python.org", 443))
                s.close()
            else:
                self.assertRaises(ValueError, s.connect, ("svn.python.org", 443))
            # This should fail because we have no verification certs
            ctx.verify_mode = ssl.CERT_REQUIRED
            s = ctx.wrap_socket(socket.socket(socket.AF_INET))
            self.assertRaisesRegex(ssl.SSLError, "certificate verify failed",
                                    s.connect, ("svn.python.org", 443))
            s.close()
            # This should succeed because we specify the root cert
            ctx.load_verify_locations(SVN_PYTHON_ORG_ROOT_CERT)
            s = ctx.wrap_socket(socket.socket(socket.AF_INET))
            s.connect(("svn.python.org", 443))
            try:
                cert = s.getpeercert()
                self.assertTrue(cert)
            finally:
                s.close()

    def test_connect_capath(self):
        # Verify server certificates using the `capath` argument
        # NOTE: the subject hashing algorithm has been changed between
        # OpenSSL 0.9.8n and 1.0.0, as a result the capath directory must
        # contain both versions of each certificate (same content, different
        # filename) for this test to be portable across OpenSSL releases.
        with support.transient_internet("svn.python.org"):
            ctx = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            ctx.verify_mode = ssl.CERT_REQUIRED
            ctx.load_verify_locations(capath=CAPATH)
            s = ctx.wrap_socket(socket.socket(socket.AF_INET))
            s.connect(("svn.python.org", 443))
            try:
                cert = s.getpeercert()
                self.assertTrue(cert)
            finally:
                s.close()
            # Same with a bytes `capath` argument
            ctx = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            ctx.verify_mode = ssl.CERT_REQUIRED
            ctx.load_verify_locations(capath=BYTES_CAPATH)
            s = ctx.wrap_socket(socket.socket(socket.AF_INET))
            s.connect(("svn.python.org", 443))
            try:
                cert = s.getpeercert()
                self.assertTrue(cert)
            finally:
                s.close()

    def test_connect_cadata(self):
        with open(CAFILE_CACERT) as f:
            pem = f.read()
        der = ssl.PEM_cert_to_DER_cert(pem)
        with support.transient_internet("svn.python.org"):
            ctx = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            ctx.verify_mode = ssl.CERT_REQUIRED
            ctx.load_verify_locations(cadata=pem)
            with ctx.wrap_socket(socket.socket(socket.AF_INET)) as s:
                s.connect(("svn.python.org", 443))
                cert = s.getpeercert()
                self.assertTrue(cert)

            # same with DER
            ctx = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            ctx.verify_mode = ssl.CERT_REQUIRED
            ctx.load_verify_locations(cadata=der)
            with ctx.wrap_socket(socket.socket(socket.AF_INET)) as s:
                s.connect(("svn.python.org", 443))
                cert = s.getpeercert()
                self.assertTrue(cert)

    @unittest.skipIf(os.name == "nt", "Can't use a socket as a file under Windows")
    def test_makefile_close(self):
        # Issue #5238: creating a file-like object with makefile() shouldn't
        # delay closing the underlying "real socket" (here tested with its
        # file descriptor, hence skipping the test under Windows).
        with support.transient_internet("svn.python.org"):
            ss = ssl.wrap_socket(socket.socket(socket.AF_INET))
            ss.connect(("svn.python.org", 443))
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
        with support.transient_internet("svn.python.org"):
            s = socket.socket(socket.AF_INET)
            s.connect(("svn.python.org", 443))
            s.setblocking(False)
            s = ssl.wrap_socket(s,
                                cert_reqs=ssl.CERT_NONE,
                                do_handshake_on_connect=False)
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
            s.close()
            if support.verbose:
                sys.stdout.write("\nNeeded %d calls to do_handshake() to establish session.\n" % count)

    def test_get_server_certificate(self):
        def _test_get_server_certificate(host, port, cert=None):
            with support.transient_internet(host):
                pem = ssl.get_server_certificate((host, port),
                                                 ssl.PROTOCOL_SSLv23)
                if not pem:
                    self.fail("No server certificate on %s:%s!" % (host, port))

                try:
                    pem = ssl.get_server_certificate((host, port),
                                                     ssl.PROTOCOL_SSLv23,
                                                     ca_certs=CERTFILE)
                except ssl.SSLError as x:
                    #should fail
                    if support.verbose:
                        sys.stdout.write("%s\n" % x)
                else:
                    self.fail("Got server certificate %s for %s:%s!" % (pem, host, port))

                pem = ssl.get_server_certificate((host, port),
                                                 ssl.PROTOCOL_SSLv23,
                                                 ca_certs=cert)
                if not pem:
                    self.fail("No server certificate on %s:%s!" % (host, port))
                if support.verbose:
                    sys.stdout.write("\nVerified certificate for %s:%s is\n%s\n" % (host, port ,pem))

        _test_get_server_certificate('svn.python.org', 443, SVN_PYTHON_ORG_ROOT_CERT)
        if support.IPV6_ENABLED:
            _test_get_server_certificate('ipv6.google.com', 443)

    def test_ciphers(self):
        remote = ("svn.python.org", 443)
        with support.transient_internet(remote[0]):
            with ssl.wrap_socket(socket.socket(socket.AF_INET),
                                 cert_reqs=ssl.CERT_NONE, ciphers="ALL") as s:
                s.connect(remote)
            with ssl.wrap_socket(socket.socket(socket.AF_INET),
                                 cert_reqs=ssl.CERT_NONE, ciphers="DEFAULT") as s:
                s.connect(remote)
            # Error checking can happen at instantiation or when connecting
            with self.assertRaisesRegex(ssl.SSLError, "No cipher can be selected"):
                with socket.socket(socket.AF_INET) as sock:
                    s = ssl.wrap_socket(sock,
                                        cert_reqs=ssl.CERT_NONE, ciphers="^$:,;?*'dorothyx")
                    s.connect(remote)

    def test_algorithms(self):
        # Issue #8484: all algorithms should be available when verifying a
        # certificate.
        # SHA256 was added in OpenSSL 0.9.8
        if ssl.OPENSSL_VERSION_INFO < (0, 9, 8, 0, 15):
            self.skipTest("SHA256 not available on %r" % ssl.OPENSSL_VERSION)
        # sha256.tbs-internet.com needs SNI to use the correct certificate
        if not ssl.HAS_SNI:
            self.skipTest("SNI needed for this test")
        # https://sha2.hboeck.de/ was used until 2011-01-08 (no route to host)
        remote = ("sha256.tbs-internet.com", 443)
        sha256_cert = os.path.join(os.path.dirname(__file__), "sha256.pem")
        with support.transient_internet("sha256.tbs-internet.com"):
            ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            ctx.verify_mode = ssl.CERT_REQUIRED
            ctx.load_verify_locations(sha256_cert)
            s = ctx.wrap_socket(socket.socket(socket.AF_INET),
                                server_hostname="sha256.tbs-internet.com")
            try:
                s.connect(remote)
                if support.verbose:
                    sys.stdout.write("\nCipher with %r is %r\n" %
                                     (remote, s.cipher()))
                    sys.stdout.write("Certificate is:\n%s\n" %
                                     pprint.pformat(s.getpeercert()))
            finally:
                s.close()

    def test_get_ca_certs_capath(self):
        # capath certs are loaded on request
        with support.transient_internet("svn.python.org"):
            ctx = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            ctx.verify_mode = ssl.CERT_REQUIRED
            ctx.load_verify_locations(capath=CAPATH)
            self.assertEqual(ctx.get_ca_certs(), [])
            s = ctx.wrap_socket(socket.socket(socket.AF_INET))
            s.connect(("svn.python.org", 443))
            try:
                cert = s.getpeercert()
                self.assertTrue(cert)
            finally:
                s.close()
            self.assertEqual(len(ctx.get_ca_certs()), 1)

    @needs_sni
    def test_context_setget(self):
        # Check that the context of a connected socket can be replaced.
        with support.transient_internet("svn.python.org"):
            ctx1 = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            ctx2 = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            s = socket.socket(socket.AF_INET)
            with ctx1.wrap_socket(s) as ss:
                ss.connect(("svn.python.org", 443))
                self.assertIs(ss.context, ctx1)
                self.assertIs(ss._sslobj.context, ctx1)
                ss.context = ctx2
                self.assertIs(ss.context, ctx2)
                self.assertIs(ss._sslobj.context, ctx2)

try:
    import threading
except ImportError:
    _have_threads = False
else:
    _have_threads = True

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
                self.sock.setblocking(1)
                self.sslconn = None
                threading.Thread.__init__(self)
                self.daemon = True

            def wrap_conn(self):
                try:
                    self.sslconn = self.server.context.wrap_socket(
                        self.sock, server_side=True)
                    self.server.selected_protocols.append(self.sslconn.selected_npn_protocol())
                except (ssl.SSLError, ConnectionResetError) as e:
                    # We treat ConnectionResetError as though it were an
                    # SSLError - OpenSSL on Ubuntu abruptly closes the
                    # connection when asked to use an unsupported protocol.
                    #
                    # XXX Various errors can have happened here, for example
                    # a mismatching protocol version, an invalid certificate,
                    # or a low-level bug. This should be made more discriminating.
                    self.server.conn_errors.append(e)
                    if self.server.chatty:
                        handle_error("\n server:  bad connection attempt from " + repr(self.addr) + ":\n")
                    self.running = False
                    self.server.stop()
                    self.close()
                    return False
                else:
                    if self.server.context.verify_mode == ssl.CERT_REQUIRED:
                        cert = self.sslconn.getpeercert()
                        if support.verbose and self.server.chatty:
                            sys.stdout.write(" client cert is " + pprint.pformat(cert) + "\n")
                        cert_binary = self.sslconn.getpeercert(True)
                        if support.verbose and self.server.chatty:
                            sys.stdout.write(" cert binary is " + str(len(cert_binary)) + " bytes\n")
                    cipher = self.sslconn.cipher()
                    if support.verbose and self.server.chatty:
                        sys.stdout.write(" server: connection cipher is now " + str(cipher) + "\n")
                        sys.stdout.write(" server: selected protocol is now "
                                + str(self.sslconn.selected_npn_protocol()) + "\n")
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
                        else:
                            if (support.verbose and
                                self.server.connectionchatty):
                                ctype = (self.sslconn and "encrypted") or "unencrypted"
                                sys.stdout.write(" server: read %r (%s), sending back %r (%s)...\n"
                                                 % (msg, ctype, msg.lower(), ctype))
                            self.write(msg.lower())
                    except OSError:
                        if self.server.chatty:
                            handle_error("Test server failure:\n")
                        self.close()
                        self.running = False
                        # normally, we'd just stop here, but for the test
                        # harness, we want to stop the server
                        self.server.stop()

        def __init__(self, certificate=None, ssl_version=None,
                     certreqs=None, cacerts=None,
                     chatty=True, connectionchatty=False, starttls_server=False,
                     npn_protocols=None, ciphers=None, context=None):
            if context:
                self.context = context
            else:
                self.context = ssl.SSLContext(ssl_version
                                              if ssl_version is not None
                                              else ssl.PROTOCOL_TLSv1)
                self.context.verify_mode = (certreqs if certreqs is not None
                                            else ssl.CERT_NONE)
                if cacerts:
                    self.context.load_verify_locations(cacerts)
                if certificate:
                    self.context.load_cert_chain(certificate)
                if npn_protocols:
                    self.context.set_npn_protocols(npn_protocols)
                if ciphers:
                    self.context.set_ciphers(ciphers)
            self.chatty = chatty
            self.connectionchatty = connectionchatty
            self.starttls_server = starttls_server
            self.sock = socket.socket()
            self.port = support.bind_port(self.sock)
            self.flag = None
            self.active = False
            self.selected_protocols = []
            self.conn_errors = []
            threading.Thread.__init__(self)
            self.daemon = True

        def __enter__(self):
            self.start(threading.Event())
            self.flag.wait()
            return self

        def __exit__(self, *args):
            self.stop()
            self.join()

        def start(self, flag=None):
            self.flag = flag
            threading.Thread.start(self)

        def run(self):
            self.sock.settimeout(0.05)
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
                except socket.timeout:
                    pass
                except KeyboardInterrupt:
                    self.stop()
            self.sock.close()

        def stop(self):
            self.active = False

    class AsyncoreEchoServer(threading.Thread):

        # this one's based on asyncore.dispatcher

        class EchoServer (asyncore.dispatcher):

            class ConnectionHandler (asyncore.dispatcher_with_send):

                def __init__(self, conn, certfile):
                    self.socket = ssl.wrap_socket(conn, server_side=True,
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
                self.port = support.bind_port(sock, '')
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

    def bad_cert_test(certfile):
        """
        Launch a server with CERT_REQUIRED, and check that trying to
        connect to it with the given client certificate fails.
        """
        server = ThreadedEchoServer(CERTFILE,
                                    certreqs=ssl.CERT_REQUIRED,
                                    cacerts=CERTFILE, chatty=False,
                                    connectionchatty=False)
        with server:
            try:
                with socket.socket() as sock:
                    s = ssl.wrap_socket(sock,
                                        certfile=certfile,
                                        ssl_version=ssl.PROTOCOL_TLSv1)
                    s.connect((HOST, server.port))
            except ssl.SSLError as x:
                if support.verbose:
                    sys.stdout.write("\nSSLError is %s\n" % x.args[1])
            except OSError as x:
                if support.verbose:
                    sys.stdout.write("\nOSError is %s\n" % x.args[1])
            except OSError as x:
                if x.errno != errno.ENOENT:
                    raise
                if support.verbose:
                    sys.stdout.write("\OSError is %s\n" % str(x))
            else:
                raise AssertionError("Use of invalid cert should have failed!")

    def server_params_test(client_context, server_context, indata=b"FOO\n",
                           chatty=True, connectionchatty=False, sni_name=None):
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
                    server_hostname=sni_name) as s:
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
                    'client_npn_protocol': s.selected_npn_protocol()
                })
                s.close()
            stats['server_npn_protocols'] = server.selected_protocols
        return stats

    def try_protocol_combo(server_protocol, client_protocol, expect_success,
                           certsreqs=None, server_options=0, client_options=0):
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
        client_context = ssl.SSLContext(client_protocol)
        client_context.options |= client_options
        server_context = ssl.SSLContext(server_protocol)
        server_context.options |= server_options

        # NOTE: we must enable "ALL" ciphers on the client, otherwise an
        # SSLv23 client will send an SSLv3 hello (rather than SSLv2)
        # starting from OpenSSL 1.0.0 (see issue #8322).
        if client_context.protocol == ssl.PROTOCOL_SSLv23:
            client_context.set_ciphers("ALL")

        for ctx in (client_context, server_context):
            ctx.verify_mode = certsreqs
            ctx.load_cert_chain(CERTFILE)
            ctx.load_verify_locations(CERTFILE)
        try:
            server_params_test(client_context, server_context,
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


    class ThreadedTests(unittest.TestCase):

        @skip_if_broken_ubuntu_ssl
        def test_echo(self):
            """Basic test of an SSL client connecting to a server"""
            if support.verbose:
                sys.stdout.write("\n")
            for protocol in PROTOCOLS:
                with self.subTest(protocol=ssl._PROTOCOL_NAMES[protocol]):
                    context = ssl.SSLContext(protocol)
                    context.load_cert_chain(CERTFILE)
                    server_params_test(context, context,
                                       chatty=True, connectionchatty=True)

        def test_getpeercert(self):
            if support.verbose:
                sys.stdout.write("\n")
            context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            context.verify_mode = ssl.CERT_REQUIRED
            context.load_verify_locations(CERTFILE)
            context.load_cert_chain(CERTFILE)
            server = ThreadedEchoServer(context=context, chatty=False)
            with server:
                s = context.wrap_socket(socket.socket(),
                                        do_handshake_on_connect=False)
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
                s.close()

        @unittest.skipUnless(have_verify_flags(),
                            "verify_flags need OpenSSL > 0.9.8")
        def test_crl_check(self):
            if support.verbose:
                sys.stdout.write("\n")

            server_context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            server_context.load_cert_chain(SIGNED_CERTFILE)

            context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            context.verify_mode = ssl.CERT_REQUIRED
            context.load_verify_locations(SIGNING_CA)
            self.assertEqual(context.verify_flags, ssl.VERIFY_DEFAULT)

            # VERIFY_DEFAULT should pass
            server = ThreadedEchoServer(context=server_context, chatty=True)
            with server:
                with context.wrap_socket(socket.socket()) as s:
                    s.connect((HOST, server.port))
                    cert = s.getpeercert()
                    self.assertTrue(cert, "Can't get peer certificate.")

            # VERIFY_CRL_CHECK_LEAF without a loaded CRL file fails
            context.verify_flags |= ssl.VERIFY_CRL_CHECK_LEAF

            server = ThreadedEchoServer(context=server_context, chatty=True)
            with server:
                with context.wrap_socket(socket.socket()) as s:
                    with self.assertRaisesRegex(ssl.SSLError,
                                                "certificate verify failed"):
                        s.connect((HOST, server.port))

            # now load a CRL file. The CRL file is signed by the CA.
            context.load_verify_locations(CRLFILE)

            server = ThreadedEchoServer(context=server_context, chatty=True)
            with server:
                with context.wrap_socket(socket.socket()) as s:
                    s.connect((HOST, server.port))
                    cert = s.getpeercert()
                    self.assertTrue(cert, "Can't get peer certificate.")

        @needs_sni
        def test_check_hostname(self):
            if support.verbose:
                sys.stdout.write("\n")

            server_context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            server_context.load_cert_chain(SIGNED_CERTFILE)

            context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            context.verify_mode = ssl.CERT_REQUIRED
            context.check_hostname = True
            context.load_verify_locations(SIGNING_CA)

            # correct hostname should verify
            server = ThreadedEchoServer(context=server_context, chatty=True)
            with server:
                with context.wrap_socket(socket.socket(),
                                         server_hostname="localhost") as s:
                    s.connect((HOST, server.port))
                    cert = s.getpeercert()
                    self.assertTrue(cert, "Can't get peer certificate.")

            # incorrect hostname should raise an exception
            server = ThreadedEchoServer(context=server_context, chatty=True)
            with server:
                with context.wrap_socket(socket.socket(),
                                         server_hostname="invalid") as s:
                    with self.assertRaisesRegex(ssl.CertificateError,
                                                "hostname 'invalid' doesn't match 'localhost'"):
                        s.connect((HOST, server.port))

            # missing server_hostname arg should cause an exception, too
            server = ThreadedEchoServer(context=server_context, chatty=True)
            with server:
                with socket.socket() as s:
                    with self.assertRaisesRegex(ValueError,
                                                "check_hostname requires server_hostname"):
                        context.wrap_socket(s)

        def test_empty_cert(self):
            """Connecting with an empty cert file"""
            bad_cert_test(os.path.join(os.path.dirname(__file__) or os.curdir,
                                      "nullcert.pem"))
        def test_malformed_cert(self):
            """Connecting with a badly formatted certificate (syntax error)"""
            bad_cert_test(os.path.join(os.path.dirname(__file__) or os.curdir,
                                       "badcert.pem"))
        def test_nonexisting_cert(self):
            """Connecting with a non-existing cert file"""
            bad_cert_test(os.path.join(os.path.dirname(__file__) or os.curdir,
                                       "wrongcert.pem"))
        def test_malformed_key(self):
            """Connecting with a badly formatted key (syntax error)"""
            bad_cert_test(os.path.join(os.path.dirname(__file__) or os.curdir,
                                       "badkey.pem"))

        def test_rude_shutdown(self):
            """A brutal shutdown of an SSL server should raise an OSError
            in the client when attempting handshake.
            """
            listener_ready = threading.Event()
            listener_gone = threading.Event()

            s = socket.socket()
            port = support.bind_port(s, HOST)

            # `listener` runs in a thread.  It sits in an accept() until
            # the main thread connects.  Then it rudely closes the socket,
            # and sets Event `listener_gone` to let the main thread know
            # the socket is gone.
            def listener():
                s.listen(5)
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
                        ssl_sock = ssl.wrap_socket(c)
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

        @skip_if_broken_ubuntu_ssl
        @unittest.skipUnless(hasattr(ssl, 'PROTOCOL_SSLv2'),
                             "OpenSSL is compiled without SSLv2 support")
        def test_protocol_sslv2(self):
            """Connecting to an SSLv2 server with various client options"""
            if support.verbose:
                sys.stdout.write("\n")
            try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv2, True)
            try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv2, True, ssl.CERT_OPTIONAL)
            try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv2, True, ssl.CERT_REQUIRED)
            try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv23, False)
            try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv3, False)
            try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_TLSv1, False)
            # SSLv23 client with specific SSL options
            if no_sslv2_implies_sslv3_hello():
                # No SSLv2 => client will use an SSLv3 hello on recent OpenSSLs
                try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv23, False,
                                   client_options=ssl.OP_NO_SSLv2)
            try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv23, False,
                               client_options=ssl.OP_NO_SSLv3)
            try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv23, False,
                               client_options=ssl.OP_NO_TLSv1)

        @skip_if_broken_ubuntu_ssl
        def test_protocol_sslv23(self):
            """Connecting to an SSLv23 server with various client options"""
            if support.verbose:
                sys.stdout.write("\n")
            if hasattr(ssl, 'PROTOCOL_SSLv2'):
                try:
                    try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv2, True)
                except OSError as x:
                    # this fails on some older versions of OpenSSL (0.9.7l, for instance)
                    if support.verbose:
                        sys.stdout.write(
                            " SSL2 client to SSL23 server test unexpectedly failed:\n %s\n"
                            % str(x))
            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv3, True)
            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv23, True)
            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_TLSv1, True)

            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv3, True, ssl.CERT_OPTIONAL)
            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv23, True, ssl.CERT_OPTIONAL)
            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_TLSv1, True, ssl.CERT_OPTIONAL)

            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv3, True, ssl.CERT_REQUIRED)
            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv23, True, ssl.CERT_REQUIRED)
            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_TLSv1, True, ssl.CERT_REQUIRED)

            # Server with specific SSL options
            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv3, False,
                               server_options=ssl.OP_NO_SSLv3)
            # Will choose TLSv1
            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv23, True,
                               server_options=ssl.OP_NO_SSLv2 | ssl.OP_NO_SSLv3)
            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_TLSv1, False,
                               server_options=ssl.OP_NO_TLSv1)


        @skip_if_broken_ubuntu_ssl
        def test_protocol_sslv3(self):
            """Connecting to an SSLv3 server with various client options"""
            if support.verbose:
                sys.stdout.write("\n")
            try_protocol_combo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv3, True)
            try_protocol_combo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv3, True, ssl.CERT_OPTIONAL)
            try_protocol_combo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv3, True, ssl.CERT_REQUIRED)
            if hasattr(ssl, 'PROTOCOL_SSLv2'):
                try_protocol_combo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv2, False)
            try_protocol_combo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv23, False,
                               client_options=ssl.OP_NO_SSLv3)
            try_protocol_combo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_TLSv1, False)
            if no_sslv2_implies_sslv3_hello():
                # No SSLv2 => client will use an SSLv3 hello on recent OpenSSLs
                try_protocol_combo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv23, True,
                                   client_options=ssl.OP_NO_SSLv2)

        @skip_if_broken_ubuntu_ssl
        def test_protocol_tlsv1(self):
            """Connecting to a TLSv1 server with various client options"""
            if support.verbose:
                sys.stdout.write("\n")
            try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_TLSv1, True)
            try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_TLSv1, True, ssl.CERT_OPTIONAL)
            try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_TLSv1, True, ssl.CERT_REQUIRED)
            if hasattr(ssl, 'PROTOCOL_SSLv2'):
                try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_SSLv2, False)
            try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_SSLv3, False)
            try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_SSLv23, False,
                               client_options=ssl.OP_NO_TLSv1)

        @skip_if_broken_ubuntu_ssl
        @unittest.skipUnless(hasattr(ssl, "PROTOCOL_TLSv1_1"),
                             "TLS version 1.1 not supported.")
        def test_protocol_tlsv1_1(self):
            """Connecting to a TLSv1.1 server with various client options.
               Testing against older TLS versions."""
            if support.verbose:
                sys.stdout.write("\n")
            try_protocol_combo(ssl.PROTOCOL_TLSv1_1, ssl.PROTOCOL_TLSv1_1, True)
            if hasattr(ssl, 'PROTOCOL_SSLv2'):
                try_protocol_combo(ssl.PROTOCOL_TLSv1_1, ssl.PROTOCOL_SSLv2, False)
            try_protocol_combo(ssl.PROTOCOL_TLSv1_1, ssl.PROTOCOL_SSLv3, False)
            try_protocol_combo(ssl.PROTOCOL_TLSv1_1, ssl.PROTOCOL_SSLv23, False,
                               client_options=ssl.OP_NO_TLSv1_1)

            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_TLSv1_1, True)
            try_protocol_combo(ssl.PROTOCOL_TLSv1_1, ssl.PROTOCOL_TLSv1, False)
            try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_TLSv1_1, False)


        @skip_if_broken_ubuntu_ssl
        @unittest.skipUnless(hasattr(ssl, "PROTOCOL_TLSv1_2"),
                             "TLS version 1.2 not supported.")
        def test_protocol_tlsv1_2(self):
            """Connecting to a TLSv1.2 server with various client options.
               Testing against older TLS versions."""
            if support.verbose:
                sys.stdout.write("\n")
            try_protocol_combo(ssl.PROTOCOL_TLSv1_2, ssl.PROTOCOL_TLSv1_2, True,
                               server_options=ssl.OP_NO_SSLv3|ssl.OP_NO_SSLv2,
                               client_options=ssl.OP_NO_SSLv3|ssl.OP_NO_SSLv2,)
            if hasattr(ssl, 'PROTOCOL_SSLv2'):
                try_protocol_combo(ssl.PROTOCOL_TLSv1_2, ssl.PROTOCOL_SSLv2, False)
            try_protocol_combo(ssl.PROTOCOL_TLSv1_2, ssl.PROTOCOL_SSLv3, False)
            try_protocol_combo(ssl.PROTOCOL_TLSv1_2, ssl.PROTOCOL_SSLv23, False,
                               client_options=ssl.OP_NO_TLSv1_2)

            try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_TLSv1_2, True)
            try_protocol_combo(ssl.PROTOCOL_TLSv1_2, ssl.PROTOCOL_TLSv1, False)
            try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_TLSv1_2, False)
            try_protocol_combo(ssl.PROTOCOL_TLSv1_2, ssl.PROTOCOL_TLSv1_1, False)
            try_protocol_combo(ssl.PROTOCOL_TLSv1_1, ssl.PROTOCOL_TLSv1_2, False)

        def test_starttls(self):
            """Switching from clear text to encrypted and back again."""
            msgs = (b"msg 1", b"MSG 2", b"STARTTLS", b"MSG 3", b"msg 4", b"ENDTLS", b"msg 5", b"msg 6")

            server = ThreadedEchoServer(CERTFILE,
                                        ssl_version=ssl.PROTOCOL_TLSv1,
                                        starttls_server=True,
                                        chatty=True,
                                        connectionchatty=True)
            wrapped = False
            with server:
                s = socket.socket()
                s.setblocking(1)
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
                        conn = ssl.wrap_socket(s, ssl_version=ssl.PROTOCOL_TLSv1)
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
            """Using a SocketServer to create and manage SSL connections."""
            server = make_https_server(self, certfile=CERTFILE)
            # try to connect
            if support.verbose:
                sys.stdout.write('\n')
            with open(CERTFILE, 'rb') as f:
                d1 = f.read()
            d2 = ''
            # now fetch the same data from the HTTPS server
            url = 'https://%s:%d/%s' % (
                HOST, server.port, os.path.split(CERTFILE)[1])
            f = urllib.request.urlopen(url)
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
            indata = "TEST MESSAGE of mixed case\n"

            if support.verbose:
                sys.stdout.write("\n")

            indata = b"FOO\n"
            server = AsyncoreEchoServer(CERTFILE)
            with server:
                s = ssl.wrap_socket(socket.socket())
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
                                        ssl_version=ssl.PROTOCOL_TLSv1,
                                        cacerts=CERTFILE,
                                        chatty=True,
                                        connectionchatty=False)
            with server:
                s = ssl.wrap_socket(socket.socket(),
                                    server_side=False,
                                    certfile=CERTFILE,
                                    ca_certs=CERTFILE,
                                    cert_reqs=ssl.CERT_NONE,
                                    ssl_version=ssl.PROTOCOL_TLSv1)
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

                # (name, method, whether to expect success, *args)
                send_methods = [
                    ('send', s.send, True, []),
                    ('sendto', s.sendto, False, ["some.address"]),
                    ('sendall', s.sendall, True, []),
                ]
                recv_methods = [
                    ('recv', s.recv, True, []),
                    ('recvfrom', s.recvfrom, False, ["some.address"]),
                    ('recv_into', _recv_into, True, []),
                    ('recvfrom_into', _recvfrom_into, False, []),
                ]
                data_prefix = "PREFIX_"

                for meth_name, send_meth, expect_success, args in send_methods:
                    indata = (data_prefix + meth_name).encode('ascii')
                    try:
                        send_meth(indata, *args)
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

                # Make sure sendmsg et al are disallowed to avoid
                # inadvertent disclosure of data and/or corruption
                # of the encrypted data stream
                self.assertRaises(NotImplementedError, s.sendmsg, [b"data"])
                self.assertRaises(NotImplementedError, s.recvmsg, 100)
                self.assertRaises(NotImplementedError,
                                  s.recvmsg_into, bytearray(100))

                s.write(b"over\n")
                s.close()

        def test_handshake_timeout(self):
            # Issue #5103: SSL handshake must respect the socket timeout
            server = socket.socket(socket.AF_INET)
            host = "127.0.0.1"
            port = support.bind_port(server)
            started = threading.Event()
            finish = False

            def serve():
                server.listen(5)
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
                    self.assertRaisesRegex(socket.timeout, "timed out",
                                           ssl.wrap_socket, c)
                finally:
                    c.close()
                try:
                    c = socket.socket(socket.AF_INET)
                    c = ssl.wrap_socket(c)
                    c.settimeout(0.2)
                    # Will attempt handshake and time out
                    self.assertRaisesRegex(socket.timeout, "timed out",
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
            context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            context.verify_mode = ssl.CERT_REQUIRED
            context.load_verify_locations(CERTFILE)
            context.load_cert_chain(CERTFILE)
            server = socket.socket(socket.AF_INET)
            host = "127.0.0.1"
            port = support.bind_port(server)
            server = context.wrap_socket(server, server_side=True)

            evt = threading.Event()
            remote = None
            peer = None
            def serve():
                nonlocal remote, peer
                server.listen(5)
                # Block on the accept and wait on the connection to close.
                evt.set()
                remote, peer = server.accept()
                remote.recv(1)

            t = threading.Thread(target=serve)
            t.start()
            # Client wait until server setup and perform a connect.
            evt.wait()
            client = context.wrap_socket(socket.socket())
            client.connect((host, port))
            client_addr = client.getsockname()
            client.close()
            t.join()
            remote.close()
            server.close()
            # Sanity checks.
            self.assertIsInstance(remote, ssl.SSLSocket)
            self.assertEqual(peer, client_addr)

        def test_getpeercert_enotconn(self):
            context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            with context.wrap_socket(socket.socket()) as sock:
                with self.assertRaises(OSError) as cm:
                    sock.getpeercert()
                self.assertEqual(cm.exception.errno, errno.ENOTCONN)

        def test_do_handshake_enotconn(self):
            context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            with context.wrap_socket(socket.socket()) as sock:
                with self.assertRaises(OSError) as cm:
                    sock.do_handshake()
                self.assertEqual(cm.exception.errno, errno.ENOTCONN)

        def test_default_ciphers(self):
            context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            try:
                # Force a set of weak ciphers on our client context
                context.set_ciphers("DES")
            except ssl.SSLError:
                self.skipTest("no DES cipher available")
            with ThreadedEchoServer(CERTFILE,
                                    ssl_version=ssl.PROTOCOL_SSLv23,
                                    chatty=False) as server:
                with context.wrap_socket(socket.socket()) as s:
                    with self.assertRaises(OSError):
                        s.connect((HOST, server.port))
            self.assertIn("no shared cipher", str(server.conn_errors[0]))

        @unittest.skipUnless(ssl.HAS_ECDH, "test requires ECDH-enabled OpenSSL")
        def test_default_ecdh_curve(self):
            # Issue #21015: elliptic curve-based Diffie Hellman key exchange
            # should be enabled by default on SSL contexts.
            context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            context.load_cert_chain(CERTFILE)
            # Prior to OpenSSL 1.0.0, ECDH ciphers have to be enabled
            # explicitly using the 'ECCdraft' cipher alias.  Otherwise,
            # our default cipher list should prefer ECDH-based ciphers
            # automatically.
            if ssl.OPENSSL_VERSION_INFO < (1, 0, 0):
                context.set_ciphers("ECCdraft:ECDH")
            with ThreadedEchoServer(context=context) as server:
                with context.wrap_socket(socket.socket()) as s:
                    s.connect((HOST, server.port))
                    self.assertIn("ECDH", s.cipher()[0])

        @unittest.skipUnless("tls-unique" in ssl.CHANNEL_BINDING_TYPES,
                             "'tls-unique' channel binding not available")
        def test_tls_unique_channel_binding(self):
            """Test tls-unique channel binding."""
            if support.verbose:
                sys.stdout.write("\n")

            server = ThreadedEchoServer(CERTFILE,
                                        certreqs=ssl.CERT_NONE,
                                        ssl_version=ssl.PROTOCOL_TLSv1,
                                        cacerts=CERTFILE,
                                        chatty=True,
                                        connectionchatty=False)
            with server:
                s = ssl.wrap_socket(socket.socket(),
                                    server_side=False,
                                    certfile=CERTFILE,
                                    ca_certs=CERTFILE,
                                    cert_reqs=ssl.CERT_NONE,
                                    ssl_version=ssl.PROTOCOL_TLSv1)
                s.connect((HOST, server.port))
                # get the data
                cb_data = s.get_channel_binding("tls-unique")
                if support.verbose:
                    sys.stdout.write(" got channel binding data: {0!r}\n"
                                     .format(cb_data))

                # check if it is sane
                self.assertIsNotNone(cb_data)
                self.assertEqual(len(cb_data), 12) # True for TLSv1

                # and compare with the peers version
                s.write(b"CB tls-unique\n")
                peer_data_repr = s.read().strip()
                self.assertEqual(peer_data_repr,
                                 repr(cb_data).encode("us-ascii"))
                s.close()

                # now, again
                s = ssl.wrap_socket(socket.socket(),
                                    server_side=False,
                                    certfile=CERTFILE,
                                    ca_certs=CERTFILE,
                                    cert_reqs=ssl.CERT_NONE,
                                    ssl_version=ssl.PROTOCOL_TLSv1)
                s.connect((HOST, server.port))
                new_cb_data = s.get_channel_binding("tls-unique")
                if support.verbose:
                    sys.stdout.write(" got another channel binding data: {0!r}\n"
                                     .format(new_cb_data))
                # is it really unique
                self.assertNotEqual(cb_data, new_cb_data)
                self.assertIsNotNone(cb_data)
                self.assertEqual(len(cb_data), 12) # True for TLSv1
                s.write(b"CB tls-unique\n")
                peer_data_repr = s.read().strip()
                self.assertEqual(peer_data_repr,
                                 repr(new_cb_data).encode("us-ascii"))
                s.close()

        def test_compression(self):
            context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            context.load_cert_chain(CERTFILE)
            stats = server_params_test(context, context,
                                       chatty=True, connectionchatty=True)
            if support.verbose:
                sys.stdout.write(" got compression: {!r}\n".format(stats['compression']))
            self.assertIn(stats['compression'], { None, 'ZLIB', 'RLE' })

        @unittest.skipUnless(hasattr(ssl, 'OP_NO_COMPRESSION'),
                             "ssl.OP_NO_COMPRESSION needed for this test")
        def test_compression_disabled(self):
            context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            context.load_cert_chain(CERTFILE)
            context.options |= ssl.OP_NO_COMPRESSION
            stats = server_params_test(context, context,
                                       chatty=True, connectionchatty=True)
            self.assertIs(stats['compression'], None)

        def test_dh_params(self):
            # Check we can get a connection with ephemeral Diffie-Hellman
            context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            context.load_cert_chain(CERTFILE)
            context.load_dh_params(DHFILE)
            context.set_ciphers("kEDH")
            stats = server_params_test(context, context,
                                       chatty=True, connectionchatty=True)
            cipher = stats["cipher"][0]
            parts = cipher.split("-")
            if "ADH" not in parts and "EDH" not in parts and "DHE" not in parts:
                self.fail("Non-DH cipher: " + cipher[0])

        def test_selected_npn_protocol(self):
            # selected_npn_protocol() is None unless NPN is used
            context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            context.load_cert_chain(CERTFILE)
            stats = server_params_test(context, context,
                                       chatty=True, connectionchatty=True)
            self.assertIs(stats['client_npn_protocol'], None)

        @unittest.skipUnless(ssl.HAS_NPN, "NPN support needed for this test")
        def test_npn_protocols(self):
            server_protocols = ['http/1.1', 'spdy/2']
            protocol_tests = [
                (['http/1.1', 'spdy/2'], 'http/1.1'),
                (['spdy/2', 'http/1.1'], 'http/1.1'),
                (['spdy/2', 'test'], 'spdy/2'),
                (['abc', 'def'], 'abc')
            ]
            for client_protocols, expected in protocol_tests:
                server_context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
                server_context.load_cert_chain(CERTFILE)
                server_context.set_npn_protocols(server_protocols)
                client_context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
                client_context.load_cert_chain(CERTFILE)
                client_context.set_npn_protocols(client_protocols)
                stats = server_params_test(client_context, server_context,
                                           chatty=True, connectionchatty=True)

                msg = "failed trying %s (s) and %s (c).\n" \
                      "was expecting %s, but got %%s from the %%s" \
                          % (str(server_protocols), str(client_protocols),
                             str(expected))
                client_result = stats['client_npn_protocol']
                self.assertEqual(client_result, expected, msg % (client_result, "client"))
                server_result = stats['server_npn_protocols'][-1] \
                    if len(stats['server_npn_protocols']) else 'nothing'
                self.assertEqual(server_result, expected, msg % (server_result, "server"))

        def sni_contexts(self):
            server_context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            server_context.load_cert_chain(SIGNED_CERTFILE)
            other_context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            other_context.load_cert_chain(SIGNED_CERTFILE2)
            client_context = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
            client_context.verify_mode = ssl.CERT_REQUIRED
            client_context.load_verify_locations(SIGNING_CA)
            return server_context, other_context, client_context

        def check_common_name(self, stats, name):
            cert = stats['peercert']
            self.assertIn((('commonName', name),), cert['subject'])

        @needs_sni
        def test_sni_callback(self):
            calls = []
            server_context, other_context, client_context = self.sni_contexts()

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
            self.check_common_name(stats, 'localhost')

            # Check disabling the callback
            calls = []
            server_context.set_servername_callback(None)

            stats = server_params_test(client_context, server_context,
                                       chatty=True,
                                       sni_name='notfunny')
            # Certificate didn't change
            self.check_common_name(stats, 'localhost')
            self.assertEqual(calls, [])

        @needs_sni
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

        @needs_sni
        def test_sni_callback_raising(self):
            # Raising fails the connection with a TLS handshake failure alert.
            server_context, other_context, client_context = self.sni_contexts()

            def cb_raising(ssl_sock, server_name, initial_context):
                1/0
            server_context.set_servername_callback(cb_raising)

            with self.assertRaises(ssl.SSLError) as cm, \
                 support.captured_stderr() as stderr:
                stats = server_params_test(client_context, server_context,
                                           chatty=False,
                                           sni_name='supermessage')
            self.assertEqual(cm.exception.reason, 'SSLV3_ALERT_HANDSHAKE_FAILURE')
            self.assertIn("ZeroDivisionError", stderr.getvalue())

        @needs_sni
        def test_sni_callback_wrong_return_type(self):
            # Returning the wrong return type terminates the TLS connection
            # with an internal error alert.
            server_context, other_context, client_context = self.sni_contexts()

            def cb_wrong_return_type(ssl_sock, server_name, initial_context):
                return "foo"
            server_context.set_servername_callback(cb_wrong_return_type)

            with self.assertRaises(ssl.SSLError) as cm, \
                 support.captured_stderr() as stderr:
                stats = server_params_test(client_context, server_context,
                                           chatty=False,
                                           sni_name='supermessage')
            self.assertEqual(cm.exception.reason, 'TLSV1_ALERT_INTERNAL_ERROR')
            self.assertIn("TypeError", stderr.getvalue())

        def test_read_write_after_close_raises_valuerror(self):
            context = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
            context.verify_mode = ssl.CERT_REQUIRED
            context.load_verify_locations(CERTFILE)
            context.load_cert_chain(CERTFILE)
            server = ThreadedEchoServer(context=context, chatty=False)

            with server:
                s = context.wrap_socket(socket.socket())
                s.connect((HOST, server.port))
                s.close()

                self.assertRaises(ValueError, s.read, 1024)
                self.assertRaises(ValueError, s.write, b'hello')


def test_main(verbose=False):
    if support.verbose:
        plats = {
            'Linux': platform.linux_distribution,
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
        CERTFILE, SVN_PYTHON_ORG_ROOT_CERT, BYTES_CERTFILE,
        ONLYCERT, ONLYKEY, BYTES_ONLYCERT, BYTES_ONLYKEY,
        SIGNED_CERTFILE, SIGNED_CERTFILE2, SIGNING_CA,
        BADCERT, BADKEY, EMPTYCERT]:
        if not os.path.exists(filename):
            raise support.TestFailed("Can't read certificate file %r" % filename)

    tests = [ContextTests, BasicSocketTests, SSLErrorTests]

    if support.is_resource_enabled('network'):
        tests.append(NetworkedTests)

    if _have_threads:
        thread_info = support.threading_setup()
        if thread_info:
            tests.append(ThreadedTests)

    try:
        support.run_unittest(*tests)
    finally:
        if _have_threads:
            support.threading_cleanup(*thread_info)

if __name__ == "__main__":
    test_main()
