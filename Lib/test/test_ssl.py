# Test the support for SSL and sockets

import sys
import unittest
from test import support
import socket
import select
import time
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

ssl = support.import_module("ssl")

PROTOCOLS = [
    ssl.PROTOCOL_SSLv3,
    ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_TLSv1
]
if hasattr(ssl, 'PROTOCOL_SSLv2'):
    PROTOCOLS.append(ssl.PROTOCOL_SSLv2)

HOST = support.HOST

data_file = lambda name: os.path.join(os.path.dirname(__file__), name)

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
CAPATH = data_file("capath")
BYTES_CAPATH = os.fsencode(CAPATH)

SVN_PYTHON_ORG_ROOT_CERT = data_file("https_svn_python_org_root.pem")

EMPTYCERT = data_file("nullcert.pem")
BADCERT = data_file("badcert.pem")
WRONGCERT = data_file("XXXnonexisting.pem")
BADKEY = data_file("badkey.pem")


def handle_error(prefix):
    exc_format = ' '.join(traceback.format_exception(*sys.exc_info()))
    if support.verbose:
        sys.stdout.write(prefix + exc_format)

def can_clear_options():
    # 0.9.8m or higher
    return ssl.OPENSSL_VERSION_INFO >= (0, 9, 8, 13, 15)

def no_sslv2_implies_sslv3_hello():
    # 0.9.7h or higher
    return ssl.OPENSSL_VERSION_INFO >= (0, 9, 7, 8, 15)


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


class BasicSocketTests(unittest.TestCase):

    def test_constants(self):
        #ssl.PROTOCOL_SSLv2
        ssl.PROTOCOL_SSLv23
        ssl.PROTOCOL_SSLv3
        ssl.PROTOCOL_TLSv1
        ssl.CERT_NONE
        ssl.CERT_OPTIONAL
        ssl.CERT_REQUIRED
        self.assertIn(ssl.HAS_SNI, {True, False})

    def test_random(self):
        v = ssl.RAND_status()
        if support.verbose:
            sys.stdout.write("\n RAND_status is %d (%s)\n"
                             % (v, (v and "sufficient randomness") or
                                "insufficient randomness"))
        try:
            ssl.RAND_egd(1)
        except TypeError:
            pass
        else:
            print("didn't raise TypeError")
        ssl.RAND_add("this is a random string", 75.0)

    def test_parse_cert(self):
        # note that this uses an 'unofficial' function in _ssl.c,
        # provided solely for this test, to exercise the certificate
        # parsing code
        p = ssl._ssl._test_decode_cert(CERTFILE)
        if support.verbose:
            sys.stdout.write("\n" + pprint.pformat(p) + "\n")

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
        del ss
        self.assertEqual(wr(), None)

    def test_wrapped_unconnected(self):
        # Methods on an unconnected SSLSocket propagate the original
        # socket.error raise by the underlying socket object.
        s = socket.socket(socket.AF_INET)
        ss = ssl.wrap_socket(s)
        self.assertRaises(socket.error, ss.recv, 1)
        self.assertRaises(socket.error, ss.recv_into, bytearray(b'x'))
        self.assertRaises(socket.error, ss.recvfrom, 1)
        self.assertRaises(socket.error, ss.recvfrom_into, bytearray(b'x'), 1)
        self.assertRaises(socket.error, ss.send, b'x')
        self.assertRaises(socket.error, ss.sendto, b'x', ('0.0.0.0', 0))

    def test_timeout(self):
        # Issue #8524: when creating an SSL socket, the timeout of the
        # original socket should be retained.
        for timeout in (None, 0.0, 5.0):
            s = socket.socket(socket.AF_INET)
            s.settimeout(timeout)
            ss = ssl.wrap_socket(s)
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
        s = ssl.wrap_socket(sock, server_side=True, certfile=CERTFILE)
        self.assertRaisesRegex(ValueError, "can't connect in server-side mode",
                                s.connect, (HOST, 8080))
        with self.assertRaises(IOError) as cm:
            with socket.socket() as sock:
                ssl.wrap_socket(sock, certfile=WRONGCERT)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        with self.assertRaises(IOError) as cm:
            with socket.socket() as sock:
                ssl.wrap_socket(sock, certfile=CERTFILE, keyfile=WRONGCERT)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        with self.assertRaises(IOError) as cm:
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

        cert = {'subject': ((('commonName', 'a.*.com'),),)}
        ok(cert, 'a.foo.com')
        fail(cert, 'a..com')
        fail(cert, 'a.com')

        cert = {'subject': ((('commonName', 'f*.com'),),)}
        ok(cert, 'foo.com')
        ok(cert, 'f.com')
        fail(cert, 'bar.com')
        fail(cert, 'foo.a.com')
        fail(cert, 'bar.foo.com')

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

    def test_server_side(self):
        # server_hostname doesn't work for server sockets
        ctx = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
        with socket.socket() as sock:
            self.assertRaises(ValueError, ctx.wrap_socket, sock, True,
                              server_hostname="some.hostname")

class ContextTests(unittest.TestCase):

    @skip_if_broken_ubuntu_ssl
    def test_constructor(self):
        if hasattr(ssl, 'PROTOCOL_SSLv2'):
            ctx = ssl.SSLContext(ssl.PROTOCOL_SSLv2)
        ctx = ssl.SSLContext(ssl.PROTOCOL_SSLv23)
        ctx = ssl.SSLContext(ssl.PROTOCOL_SSLv3)
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
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
        # OP_ALL is the default value
        self.assertEqual(ssl.OP_ALL, ctx.options)
        ctx.options |= ssl.OP_NO_SSLv2
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

    def test_verify(self):
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

    def test_load_cert_chain(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        # Combined key and cert in a single file
        ctx.load_cert_chain(CERTFILE)
        ctx.load_cert_chain(CERTFILE, keyfile=CERTFILE)
        self.assertRaises(TypeError, ctx.load_cert_chain, keyfile=CERTFILE)
        with self.assertRaises(IOError) as cm:
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

    def test_load_verify_locations(self):
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLSv1)
        ctx.load_verify_locations(CERTFILE)
        ctx.load_verify_locations(cafile=CERTFILE, capath=None)
        ctx.load_verify_locations(BYTES_CERTFILE)
        ctx.load_verify_locations(cafile=BYTES_CERTFILE, capath=None)
        self.assertRaises(TypeError, ctx.load_verify_locations)
        self.assertRaises(TypeError, ctx.load_verify_locations, None, None)
        with self.assertRaises(IOError) as cm:
            ctx.load_verify_locations(WRONGCERT)
        self.assertEqual(cm.exception.errno, errno.ENOENT)
        with self.assertRaisesRegex(ssl.SSLError, "PEM lib"):
            ctx.load_verify_locations(BADCERT)
        ctx.load_verify_locations(CERTFILE, CAPATH)
        ctx.load_verify_locations(CERTFILE, capath=BYTES_CAPATH)

        # Issue #10989: crash if the second argument type is invalid
        self.assertRaises(TypeError, ctx.load_verify_locations, None, True)

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
                    except ssl.SSLError as err:
                        if err.args[0] == ssl.SSL_ERROR_WANT_READ:
                            select.select([s], [], [], 5.0)
                        elif err.args[0] == ssl.SSL_ERROR_WANT_WRITE:
                            select.select([], [s], [], 5.0)
                        else:
                            raise
                # SSL established
                self.assertTrue(s.getpeercert())
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
                except ssl.SSLError as err:
                    if err.args[0] == ssl.SSL_ERROR_WANT_READ:
                        select.select([s], [], [])
                    elif err.args[0] == ssl.SSL_ERROR_WANT_WRITE:
                        select.select([], [s], [])
                    else:
                        raise
            s.close()
            if support.verbose:
                sys.stdout.write("\nNeeded %d calls to do_handshake() to establish session.\n" % count)

    def test_get_server_certificate(self):
        def _test_get_server_certificate(host, port, cert=None):
            with support.transient_internet(host):
                pem = ssl.get_server_certificate((host, port))
                if not pem:
                    self.fail("No server certificate on %s:%s!" % (host, port))

                try:
                    pem = ssl.get_server_certificate((host, port), ca_certs=CERTFILE)
                except ssl.SSLError as x:
                    #should fail
                    if support.verbose:
                        sys.stdout.write("%s\n" % x)
                else:
                    self.fail("Got server certificate %s for %s:%s!" % (pem, host, port))

                pem = ssl.get_server_certificate((host, port), ca_certs=cert)
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
            s = ssl.wrap_socket(socket.socket(socket.AF_INET),
                                cert_reqs=ssl.CERT_NONE, ciphers="ALL")
            s.connect(remote)
            s = ssl.wrap_socket(socket.socket(socket.AF_INET),
                                cert_reqs=ssl.CERT_NONE, ciphers="DEFAULT")
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
        # https://sha2.hboeck.de/ was used until 2011-01-08 (no route to host)
        remote = ("sha256.tbs-internet.com", 443)
        sha256_cert = os.path.join(os.path.dirname(__file__), "sha256.pem")
        with support.transient_internet("sha256.tbs-internet.com"):
            s = ssl.wrap_socket(socket.socket(socket.AF_INET),
                                cert_reqs=ssl.CERT_REQUIRED,
                                ca_certs=sha256_cert,)
            try:
                s.connect(remote)
                if support.verbose:
                    sys.stdout.write("\nCipher with %r is %r\n" %
                                     (remote, s.cipher()))
                    sys.stdout.write("Certificate is:\n%s\n" %
                                     pprint.pformat(s.getpeercert()))
            finally:
                s.close()


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
                except ssl.SSLError:
                    # XXX Various errors can have happened here, for example
                    # a mismatching protocol version, an invalid certificate,
                    # or a low-level bug. This should be made more discriminating.
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
                        else:
                            if (support.verbose and
                                self.server.connectionchatty):
                                ctype = (self.sslconn and "encrypted") or "unencrypted"
                                sys.stdout.write(" server: read %r (%s), sending back %r (%s)...\n"
                                                 % (msg, ctype, msg.lower(), ctype))
                            self.write(msg.lower())
                    except socket.error:
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
                     ciphers=None, context=None):
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
                if ciphers:
                    self.context.set_ciphers(ciphers)
            self.chatty = chatty
            self.connectionchatty = connectionchatty
            self.starttls_server = starttls_server
            self.sock = socket.socket()
            self.port = support.bind_port(self.sock)
            self.flag = None
            self.active = False
            threading.Thread.__init__(self)
            self.daemon = True

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
                    except ssl.SSLError as err:
                        if err.args[0] in (ssl.SSL_ERROR_WANT_READ,
                                           ssl.SSL_ERROR_WANT_WRITE):
                            return
                        elif err.args[0] == ssl.SSL_ERROR_EOF:
                            return self.handle_close()
                        raise
                    except socket.error as err:
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
        flag = threading.Event()
        server.start(flag)
        # wait for it to start
        flag.wait()
        # try to connect
        try:
            try:
                with socket.socket() as sock:
                    s = ssl.wrap_socket(sock,
                                        certfile=certfile,
                                        ssl_version=ssl.PROTOCOL_TLSv1)
                    s.connect((HOST, server.port))
            except ssl.SSLError as x:
                if support.verbose:
                    sys.stdout.write("\nSSLError is %s\n" % x.args[1])
            except socket.error as x:
                if support.verbose:
                    sys.stdout.write("\nsocket.error is %s\n" % x.args[1])
            except IOError as x:
                if x.errno != errno.ENOENT:
                    raise
                if support.verbose:
                    sys.stdout.write("\IOError is %s\n" % str(x))
            else:
                raise AssertionError("Use of invalid cert should have failed!")
        finally:
            server.stop()
            server.join()

    def server_params_test(client_context, server_context, indata=b"FOO\n",
                           chatty=True, connectionchatty=False):
        """
        Launch a server, connect a client to it and try various reads
        and writes.
        """
        server = ThreadedEchoServer(context=server_context,
                                    chatty=chatty,
                                    connectionchatty=False)
        flag = threading.Event()
        server.start(flag)
        # wait for it to start
        flag.wait()
        # try to connect
        try:
            s = client_context.wrap_socket(socket.socket())
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
            s.close()
        finally:
            server.stop()
            server.join()

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
        client_context.options = ssl.OP_ALL | client_options
        server_context = ssl.SSLContext(server_protocol)
        server_context.options = ssl.OP_ALL | server_options
        for ctx in (client_context, server_context):
            ctx.verify_mode = certsreqs
            # NOTE: we must enable "ALL" ciphers, otherwise an SSLv23 client
            # will send an SSLv3 hello (rather than SSLv2) starting from
            # OpenSSL 1.0.0 (see issue #8322).
            ctx.set_ciphers("ALL")
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
        except socket.error as e:
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
            flag = threading.Event()
            server.start(flag)
            # wait for it to start
            flag.wait()
            # try to connect
            try:
                s = context.wrap_socket(socket.socket())
                s.connect((HOST, server.port))
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
            finally:
                server.stop()
                server.join()

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
            """A brutal shutdown of an SSL server should raise an IOError
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
                    except IOError:
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
            try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv23, True)
            try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv3, False)
            try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_TLSv1, False)
            # SSLv23 client with specific SSL options
            if no_sslv2_implies_sslv3_hello():
                # No SSLv2 => client will use an SSLv3 hello on recent OpenSSLs
                try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv23, False,
                                   client_options=ssl.OP_NO_SSLv2)
            try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv23, True,
                               client_options=ssl.OP_NO_SSLv3)
            try_protocol_combo(ssl.PROTOCOL_SSLv2, ssl.PROTOCOL_SSLv23, True,
                               client_options=ssl.OP_NO_TLSv1)

        @skip_if_broken_ubuntu_ssl
        def test_protocol_sslv23(self):
            """Connecting to an SSLv23 server with various client options"""
            if support.verbose:
                sys.stdout.write("\n")
            if hasattr(ssl, 'PROTOCOL_SSLv2'):
                try:
                    try_protocol_combo(ssl.PROTOCOL_SSLv23, ssl.PROTOCOL_SSLv2, True)
                except (ssl.SSLError, socket.error) as x:
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
            try_protocol_combo(ssl.PROTOCOL_SSLv3, ssl.PROTOCOL_SSLv23, False)
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
            try_protocol_combo(ssl.PROTOCOL_TLSv1, ssl.PROTOCOL_SSLv23, False)

        def test_starttls(self):
            """Switching from clear text to encrypted and back again."""
            msgs = (b"msg 1", b"MSG 2", b"STARTTLS", b"MSG 3", b"msg 4", b"ENDTLS", b"msg 5", b"msg 6")

            server = ThreadedEchoServer(CERTFILE,
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
            finally:
                server.stop()
                server.join()

        def test_socketserver(self):
            """Using a SocketServer to create and manage SSL connections."""
            server = make_https_server(self, CERTFILE)
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
            flag = threading.Event()
            server.start(flag)
            # wait for it to start
            flag.wait()
            # try to connect
            try:
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
            finally:
                if support.verbose:
                    sys.stdout.write(" cleanup: stopping server.\n")
                server.stop()
                if support.verbose:
                    sys.stdout.write(" cleanup: joining server thread.\n")
                server.join()
                if support.verbose:
                    sys.stdout.write(" cleanup: successfully joined.\n")

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
            flag = threading.Event()
            server.start(flag)
            # wait for it to start
            flag.wait()
            # try to connect
            s = ssl.wrap_socket(socket.socket(),
                                server_side=False,
                                certfile=CERTFILE,
                                ca_certs=CERTFILE,
                                cert_reqs=ssl.CERT_NONE,
                                ssl_version=ssl.PROTOCOL_TLSv1)
            s.connect((HOST, server.port))
            try:
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

                s.write(b"over\n")
                s.close()
            finally:
                server.stop()
                server.join()

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

    for filename in [
        CERTFILE, SVN_PYTHON_ORG_ROOT_CERT, BYTES_CERTFILE,
        ONLYCERT, ONLYKEY, BYTES_ONLYCERT, BYTES_ONLYKEY,
        BADCERT, BADKEY, EMPTYCERT]:
        if not os.path.exists(filename):
            raise support.TestFailed("Can't read certificate file %r" % filename)

    tests = [ContextTests, BasicSocketTests]

    if support.is_resource_enabled('network'):
        tests.append(NetworkedTests)

    if _have_threads:
        thread_info = support.threading_setup()
        if thread_info and support.is_resource_enabled('network'):
            tests.append(ThreadedTests)

    try:
        support.run_unittest(*tests)
    finally:
        if _have_threads:
            support.threading_cleanup(*thread_info)

if __name__ == "__main__":
    test_main()
