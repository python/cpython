"""Unittests for the various HTTPServer modules.

Written by Cody A.W. Somerville <cody-somerville@ubuntu.com>,
Josip Dzolonga, and Michael Otteneder for the 2007/08 GHOP contest.
"""

from http.server import BaseHTTPRequestHandler, HTTPServer, HTTPSServer, \
     SimpleHTTPRequestHandler
from http import server, HTTPStatus

import contextlib
import os
import socket
import sys
import re
import ntpath
import pathlib
import shutil
import email.message
import email.utils
import html
import http, http.client
import urllib.parse
import urllib.request
import tempfile
import time
import datetime
import threading
from unittest import mock
from io import BytesIO, StringIO

import unittest
from test import support
from test.support import (
    is_apple, import_helper, os_helper, threading_helper
)
from test.support.script_helper import kill_python, spawn_python
from test.support.socket_helper import find_unused_port

try:
    import ssl
except ImportError:
    ssl = None

support.requires_working_socket(module=True)

class NoLogRequestHandler:
    def log_message(self, *args):
        # don't write log messages to stderr
        pass

    def read(self, n=None):
        return ''


class DummyRequestHandler(NoLogRequestHandler, SimpleHTTPRequestHandler):
    pass


def create_https_server(
    certfile,
    keyfile=None,
    password=None,
    *,
    address=('localhost', 0),
    request_handler=DummyRequestHandler,
):
    return HTTPSServer(
        address, request_handler,
        certfile=certfile, keyfile=keyfile, password=password
    )


class TestSSLDisabled(unittest.TestCase):
    def test_https_server_raises_runtime_error(self):
        with import_helper.isolated_modules():
            sys.modules['ssl'] = None
            certfile = certdata_file("keycert.pem")
            with self.assertRaises(RuntimeError):
                create_https_server(certfile)


class TestServerThread(threading.Thread):
    def __init__(self, test_object, request_handler, tls=None):
        threading.Thread.__init__(self)
        self.request_handler = request_handler
        self.test_object = test_object
        self.tls = tls

    def run(self):
        if self.tls:
            certfile, keyfile, password = self.tls
            self.server = create_https_server(
                certfile, keyfile, password,
                request_handler=self.request_handler,
            )
        else:
            self.server = HTTPServer(('localhost', 0), self.request_handler)
        self.test_object.HOST, self.test_object.PORT = self.server.socket.getsockname()
        self.test_object.server_started.set()
        self.test_object = None
        try:
            self.server.serve_forever(0.05)
        finally:
            self.server.server_close()

    def stop(self):
        self.server.shutdown()
        self.join()


class BaseTestCase(unittest.TestCase):

    # Optional tuple (certfile, keyfile, password) to use for HTTPS servers.
    tls = None

    def setUp(self):
        self._threads = threading_helper.threading_setup()
        os.environ = os_helper.EnvironmentVarGuard()
        self.server_started = threading.Event()
        self.thread = TestServerThread(self, self.request_handler, self.tls)
        self.thread.start()
        self.server_started.wait()

    def tearDown(self):
        self.thread.stop()
        self.thread = None
        os.environ.__exit__()
        threading_helper.threading_cleanup(*self._threads)

    def request(self, uri, method='GET', body=None, headers={}):
        self.connection = http.client.HTTPConnection(self.HOST, self.PORT)
        self.connection.request(method, uri, body, headers)
        return self.connection.getresponse()


class BaseHTTPServerTestCase(BaseTestCase):
    class request_handler(NoLogRequestHandler, BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.1'
        default_request_version = 'HTTP/1.1'

        def do_TEST(self):
            self.send_response(HTTPStatus.NO_CONTENT)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Connection', 'close')
            self.end_headers()

        def do_KEEP(self):
            self.send_response(HTTPStatus.NO_CONTENT)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Connection', 'keep-alive')
            self.end_headers()

        def do_KEYERROR(self):
            self.send_error(999)

        def do_NOTFOUND(self):
            self.send_error(HTTPStatus.NOT_FOUND)

        def do_EXPLAINERROR(self):
            self.send_error(999, "Short Message",
                            "This is a long \n explanation")

        def do_CUSTOM(self):
            self.send_response(999)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Connection', 'close')
            self.end_headers()

        def do_LATINONEHEADER(self):
            self.send_response(999)
            self.send_header('X-Special', 'Dängerous Mind')
            self.send_header('Connection', 'close')
            self.end_headers()
            body = self.headers['x-special-incoming'].encode('utf-8')
            self.wfile.write(body)

        def do_SEND_ERROR(self):
            self.send_error(int(self.path[1:]))

        def do_HEAD(self):
            self.send_error(int(self.path[1:]))

    def setUp(self):
        BaseTestCase.setUp(self)
        self.con = http.client.HTTPConnection(self.HOST, self.PORT)
        self.con.connect()

    def test_command(self):
        self.con.request('GET', '/')
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.NOT_IMPLEMENTED)

    def test_request_line_trimming(self):
        self.con._http_vsn_str = 'HTTP/1.1\n'
        self.con.putrequest('XYZBOGUS', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.NOT_IMPLEMENTED)

    def test_version_bogus(self):
        self.con._http_vsn_str = 'FUBAR'
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.BAD_REQUEST)

    def test_version_digits(self):
        self.con._http_vsn_str = 'HTTP/9.9.9'
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.BAD_REQUEST)

    def test_version_signs_and_underscores(self):
        self.con._http_vsn_str = 'HTTP/-9_9_9.+9_9_9'
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.BAD_REQUEST)

    def test_major_version_number_too_long(self):
        self.con._http_vsn_str = 'HTTP/909876543210.0'
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.BAD_REQUEST)

    def test_minor_version_number_too_long(self):
        self.con._http_vsn_str = 'HTTP/1.909876543210'
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.BAD_REQUEST)

    def test_version_none_get(self):
        self.con._http_vsn_str = ''
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.NOT_IMPLEMENTED)

    def test_version_none(self):
        # Test that a valid method is rejected when not HTTP/1.x
        self.con._http_vsn_str = ''
        self.con.putrequest('CUSTOM', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.BAD_REQUEST)

    def test_version_invalid(self):
        self.con._http_vsn = 99
        self.con._http_vsn_str = 'HTTP/9.9'
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.HTTP_VERSION_NOT_SUPPORTED)

    def test_send_blank(self):
        self.con._http_vsn_str = ''
        self.con.putrequest('', '')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.BAD_REQUEST)

    def test_header_close(self):
        self.con.putrequest('GET', '/')
        self.con.putheader('Connection', 'close')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.NOT_IMPLEMENTED)

    def test_header_keep_alive(self):
        self.con._http_vsn_str = 'HTTP/1.1'
        self.con.putrequest('GET', '/')
        self.con.putheader('Connection', 'keep-alive')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.NOT_IMPLEMENTED)

    def test_handler(self):
        self.con.request('TEST', '/')
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.NO_CONTENT)

    def test_return_header_keep_alive(self):
        self.con.request('KEEP', '/')
        res = self.con.getresponse()
        self.assertEqual(res.getheader('Connection'), 'keep-alive')
        self.con.request('TEST', '/')
        self.addCleanup(self.con.close)

    def test_internal_key_error(self):
        self.con.request('KEYERROR', '/')
        res = self.con.getresponse()
        self.assertEqual(res.status, 999)

    def test_return_custom_status(self):
        self.con.request('CUSTOM', '/')
        res = self.con.getresponse()
        self.assertEqual(res.status, 999)

    def test_return_explain_error(self):
        self.con.request('EXPLAINERROR', '/')
        res = self.con.getresponse()
        self.assertEqual(res.status, 999)
        self.assertTrue(int(res.getheader('Content-Length')))

    def test_latin1_header(self):
        self.con.request('LATINONEHEADER', '/', headers={
            'X-Special-Incoming':       'Ärger mit Unicode'
        })
        res = self.con.getresponse()
        self.assertEqual(res.getheader('X-Special'), 'Dängerous Mind')
        self.assertEqual(res.read(), 'Ärger mit Unicode'.encode('utf-8'))

    def test_error_content_length(self):
        # Issue #16088: standard error responses should have a content-length
        self.con.request('NOTFOUND', '/')
        res = self.con.getresponse()
        self.assertEqual(res.status, HTTPStatus.NOT_FOUND)

        data = res.read()
        self.assertEqual(int(res.getheader('Content-Length')), len(data))

    def test_send_error(self):
        allow_transfer_encoding_codes = (HTTPStatus.NOT_MODIFIED,
                                         HTTPStatus.RESET_CONTENT)
        for code in (HTTPStatus.NO_CONTENT, HTTPStatus.NOT_MODIFIED,
                     HTTPStatus.PROCESSING, HTTPStatus.RESET_CONTENT,
                     HTTPStatus.SWITCHING_PROTOCOLS):
            self.con.request('SEND_ERROR', '/{}'.format(code))
            res = self.con.getresponse()
            self.assertEqual(code, res.status)
            self.assertEqual(None, res.getheader('Content-Length'))
            self.assertEqual(None, res.getheader('Content-Type'))
            if code not in allow_transfer_encoding_codes:
                self.assertEqual(None, res.getheader('Transfer-Encoding'))

            data = res.read()
            self.assertEqual(b'', data)

    def test_head_via_send_error(self):
        allow_transfer_encoding_codes = (HTTPStatus.NOT_MODIFIED,
                                         HTTPStatus.RESET_CONTENT)
        for code in (HTTPStatus.OK, HTTPStatus.NO_CONTENT,
                     HTTPStatus.NOT_MODIFIED, HTTPStatus.RESET_CONTENT,
                     HTTPStatus.SWITCHING_PROTOCOLS):
            self.con.request('HEAD', '/{}'.format(code))
            res = self.con.getresponse()
            self.assertEqual(code, res.status)
            if code == HTTPStatus.OK:
                self.assertTrue(int(res.getheader('Content-Length')) > 0)
                self.assertIn('text/html', res.getheader('Content-Type'))
            else:
                self.assertEqual(None, res.getheader('Content-Length'))
                self.assertEqual(None, res.getheader('Content-Type'))
            if code not in allow_transfer_encoding_codes:
                self.assertEqual(None, res.getheader('Transfer-Encoding'))

            data = res.read()
            self.assertEqual(b'', data)


class HTTP09ServerTestCase(BaseTestCase):

    class request_handler(NoLogRequestHandler, BaseHTTPRequestHandler):
        """Request handler for HTTP/0.9 server."""

        def do_GET(self):
            self.wfile.write(f'OK: here is {self.path}\r\n'.encode())

    def setUp(self):
        super().setUp()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock = self.enterContext(self.sock)
        self.sock.connect((self.HOST, self.PORT))

    def test_simple_get(self):
        self.sock.send(b'GET /index.html\r\n')
        res = self.sock.recv(1024)
        self.assertEqual(res, b"OK: here is /index.html\r\n")

    def test_invalid_request(self):
        self.sock.send(b'POST /index.html\r\n')
        res = self.sock.recv(1024)
        self.assertIn(b"Bad HTTP/0.9 request type ('POST')", res)

    def test_single_request(self):
        self.sock.send(b'GET /foo.html\r\n')
        res = self.sock.recv(1024)
        self.assertEqual(res, b"OK: here is /foo.html\r\n")

        # Ignore errors if the connection is already closed,
        # as this is the expected behavior of HTTP/0.9.
        with contextlib.suppress(OSError):
            self.sock.send(b'GET /bar.html\r\n')
            res = self.sock.recv(1024)
            # The server should not process our request.
            self.assertEqual(res, b'')


def certdata_file(*path):
    return os.path.join(os.path.dirname(__file__), "certdata", *path)


@unittest.skipIf(ssl is None, "requires ssl")
class BaseHTTPSServerTestCase(BaseTestCase):
    CERTFILE = certdata_file("keycert.pem")
    ONLYCERT = certdata_file("ssl_cert.pem")
    ONLYKEY = certdata_file("ssl_key.pem")
    CERTFILE_PROTECTED = certdata_file("keycert.passwd.pem")
    ONLYKEY_PROTECTED = certdata_file("ssl_key.passwd.pem")
    EMPTYCERT = certdata_file("nullcert.pem")
    BADCERT = certdata_file("badcert.pem")
    KEY_PASSWORD = "somepass"
    BADPASSWORD = "badpass"

    tls = (ONLYCERT, ONLYKEY, None)  # values by default

    request_handler = DummyRequestHandler

    def test_get(self):
        response = self.request('/')
        self.assertEqual(response.status, HTTPStatus.OK)

    def request(self, uri, method='GET', body=None, headers={}):
        context = ssl._create_unverified_context()
        self.connection = http.client.HTTPSConnection(
            self.HOST, self.PORT, context=context
        )
        self.connection.request(method, uri, body, headers)
        return self.connection.getresponse()

    def test_valid_certdata(self):
        valid_certdata= [
            (self.CERTFILE, None, None),
            (self.CERTFILE, self.CERTFILE, None),
            (self.CERTFILE_PROTECTED, None, self.KEY_PASSWORD),
            (self.ONLYCERT, self.ONLYKEY_PROTECTED, self.KEY_PASSWORD),
        ]
        for certfile, keyfile, password in valid_certdata:
            with self.subTest(
                certfile=certfile, keyfile=keyfile, password=password
            ):
                server = create_https_server(certfile, keyfile, password)
                self.assertIsInstance(server, HTTPSServer)
                server.server_close()

    def test_invalid_certdata(self):
        invalid_certdata = [
            (self.BADCERT, None, None),
            (self.EMPTYCERT, None, None),
            (self.ONLYCERT, None, None),
            (self.ONLYKEY, None, None),
            (self.ONLYKEY, self.ONLYCERT, None),
            (self.CERTFILE_PROTECTED, None, self.BADPASSWORD),
            # TODO: test the next case and add same case to test_ssl (We
            # specify a cert and a password-protected file, but no password):
            # (self.CERTFILE_PROTECTED, None, None),
            # see issue #132102
        ]
        for certfile, keyfile, password in invalid_certdata:
            with self.subTest(
                certfile=certfile, keyfile=keyfile, password=password
            ):
                with self.assertRaises(ssl.SSLError):
                    create_https_server(certfile, keyfile, password)


class RequestHandlerLoggingTestCase(BaseTestCase):
    class request_handler(BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.1'
        default_request_version = 'HTTP/1.1'

        def do_GET(self):
            self.send_response(HTTPStatus.OK)
            self.end_headers()

        def do_ERROR(self):
            self.send_error(HTTPStatus.NOT_FOUND, 'File not found')

    def test_get(self):
        self.con = http.client.HTTPConnection(self.HOST, self.PORT)
        self.con.connect()

        with support.captured_stderr() as err:
            self.con.request('GET', '/')
            self.con.getresponse()

        self.assertEndsWith(err.getvalue(), '"GET / HTTP/1.1" 200 -\n')

    def test_err(self):
        self.con = http.client.HTTPConnection(self.HOST, self.PORT)
        self.con.connect()

        with support.captured_stderr() as err:
            self.con.request('ERROR', '/')
            self.con.getresponse()

        lines = err.getvalue().split('\n')
        self.assertEndsWith(lines[0], 'code 404, message File not found')
        self.assertEndsWith(lines[1], '"ERROR / HTTP/1.1" 404 -')


class SimpleHTTPServerTestCase(BaseTestCase):
    class request_handler(NoLogRequestHandler, SimpleHTTPRequestHandler):
        pass

    def setUp(self):
        super().setUp()
        self.cwd = os.getcwd()
        basetempdir = tempfile.gettempdir()
        os.chdir(basetempdir)
        self.data = b'We are the knights who say Ni!'
        self.tempdir = tempfile.mkdtemp(dir=basetempdir)
        self.tempdir_name = os.path.basename(self.tempdir)
        self.base_url = '/' + self.tempdir_name
        tempname = os.path.join(self.tempdir, 'test')
        with open(tempname, 'wb') as temp:
            temp.write(self.data)
            temp.flush()
        mtime = os.stat(tempname).st_mtime
        # compute last modification datetime for browser cache tests
        last_modif = datetime.datetime.fromtimestamp(mtime,
            datetime.timezone.utc)
        self.last_modif_datetime = last_modif.replace(microsecond=0)
        self.last_modif_header = email.utils.formatdate(
            last_modif.timestamp(), usegmt=True)

    def tearDown(self):
        try:
            os.chdir(self.cwd)
            try:
                shutil.rmtree(self.tempdir)
            except:
                pass
        finally:
            super().tearDown()

    def check_status_and_reason(self, response, status, data=None):
        def close_conn():
            """Don't close reader yet so we can check if there was leftover
            buffered input"""
            nonlocal reader
            reader = response.fp
            response.fp = None
        reader = None
        response._close_conn = close_conn

        body = response.read()
        self.assertTrue(response)
        self.assertEqual(response.status, status)
        self.assertIsNotNone(response.reason)
        if data:
            self.assertEqual(data, body)
        # Ensure the server has not set up a persistent connection, and has
        # not sent any extra data
        self.assertEqual(response.version, 10)
        self.assertEqual(response.msg.get("Connection", "close"), "close")
        self.assertEqual(reader.read(30), b'', 'Connection should be closed')

        reader.close()
        return body

    def check_list_dir_dirname(self, dirname, quotedname=None):
        fullpath = os.path.join(self.tempdir, dirname)
        try:
            os.mkdir(os.path.join(self.tempdir, dirname))
        except (OSError, UnicodeEncodeError):
            self.skipTest(f'Can not create directory {dirname!a} '
                          f'on current file system')

        if quotedname is None:
            quotedname = urllib.parse.quote(dirname, errors='surrogatepass')
        response = self.request(self.base_url + '/' + quotedname + '/')
        body = self.check_status_and_reason(response, HTTPStatus.OK)
        displaypath = html.escape(f'{self.base_url}/{dirname}/', quote=False)
        enc = sys.getfilesystemencoding()
        prefix = f'listing for {displaypath}</'.encode(enc, 'surrogateescape')
        self.assertIn(prefix + b'title>', body)
        self.assertIn(prefix + b'h1>', body)

    def check_list_dir_filename(self, filename):
        fullpath = os.path.join(self.tempdir, filename)
        content = ascii(fullpath).encode() + (os_helper.TESTFN_UNDECODABLE or b'\xff')
        try:
            with open(fullpath, 'wb') as f:
                f.write(content)
        except OSError:
            self.skipTest(f'Can not create file {filename!a} '
                          f'on current file system')

        response = self.request(self.base_url + '/')
        body = self.check_status_and_reason(response, HTTPStatus.OK)
        quotedname = urllib.parse.quote(filename, errors='surrogatepass')
        enc = response.headers.get_content_charset()
        self.assertIsNotNone(enc)
        self.assertIn((f'href="{quotedname}"').encode('ascii'), body)
        displayname = html.escape(filename, quote=False)
        self.assertIn(f'>{displayname}<'.encode(enc, 'surrogateescape'), body)

        response = self.request(self.base_url + '/' + quotedname)
        self.check_status_and_reason(response, HTTPStatus.OK, data=content)

    @unittest.skipUnless(os_helper.TESTFN_NONASCII,
                         'need os_helper.TESTFN_NONASCII')
    def test_list_dir_nonascii_dirname(self):
        dirname = os_helper.TESTFN_NONASCII + '.dir'
        self.check_list_dir_dirname(dirname)

    @unittest.skipUnless(os_helper.TESTFN_NONASCII,
                         'need os_helper.TESTFN_NONASCII')
    def test_list_dir_nonascii_filename(self):
        filename = os_helper.TESTFN_NONASCII + '.txt'
        self.check_list_dir_filename(filename)

    @unittest.skipIf(is_apple,
                     'undecodable name cannot always be decoded on Apple platforms')
    @unittest.skipIf(sys.platform == 'win32',
                     'undecodable name cannot be decoded on win32')
    @unittest.skipUnless(os_helper.TESTFN_UNDECODABLE,
                         'need os_helper.TESTFN_UNDECODABLE')
    def test_list_dir_undecodable_dirname(self):
        dirname = os.fsdecode(os_helper.TESTFN_UNDECODABLE) + '.dir'
        self.check_list_dir_dirname(dirname)

    @unittest.skipIf(is_apple,
                     'undecodable name cannot always be decoded on Apple platforms')
    @unittest.skipIf(sys.platform == 'win32',
                     'undecodable name cannot be decoded on win32')
    @unittest.skipUnless(os_helper.TESTFN_UNDECODABLE,
                         'need os_helper.TESTFN_UNDECODABLE')
    def test_list_dir_undecodable_filename(self):
        filename = os.fsdecode(os_helper.TESTFN_UNDECODABLE) + '.txt'
        self.check_list_dir_filename(filename)

    def test_list_dir_undecodable_dirname2(self):
        dirname = '\ufffd.dir'
        self.check_list_dir_dirname(dirname, quotedname='%ff.dir')

    @unittest.skipUnless(os_helper.TESTFN_UNENCODABLE,
                         'need os_helper.TESTFN_UNENCODABLE')
    def test_list_dir_unencodable_dirname(self):
        dirname = os_helper.TESTFN_UNENCODABLE + '.dir'
        self.check_list_dir_dirname(dirname)

    @unittest.skipUnless(os_helper.TESTFN_UNENCODABLE,
                         'need os_helper.TESTFN_UNENCODABLE')
    def test_list_dir_unencodable_filename(self):
        filename = os_helper.TESTFN_UNENCODABLE + '.txt'
        self.check_list_dir_filename(filename)

    def test_list_dir_escape_dirname(self):
        # Characters that need special treating in URL or HTML.
        for name in ('q?', 'f#', '&amp;', '&amp', '<i>', '"dq"', "'sq'",
                     '%A4', '%E2%82%AC'):
            with self.subTest(name=name):
                dirname = name + '.dir'
                self.check_list_dir_dirname(dirname,
                        quotedname=urllib.parse.quote(dirname, safe='&<>\'"'))

    def test_list_dir_escape_filename(self):
        # Characters that need special treating in URL or HTML.
        for name in ('q?', 'f#', '&amp;', '&amp', '<i>', '"dq"', "'sq'",
                     '%A4', '%E2%82%AC'):
            with self.subTest(name=name):
                filename = name + '.txt'
                self.check_list_dir_filename(filename)
                os_helper.unlink(os.path.join(self.tempdir, filename))

    def test_list_dir_with_query_and_fragment(self):
        prefix = f'listing for {self.base_url}/</'.encode('latin1')
        response = self.request(self.base_url + '/#123').read()
        self.assertIn(prefix + b'title>', response)
        self.assertIn(prefix + b'h1>', response)
        response = self.request(self.base_url + '/?x=123').read()
        self.assertIn(prefix + b'title>', response)
        self.assertIn(prefix + b'h1>', response)

    def test_get_dir_redirect_location_domain_injection_bug(self):
        """Ensure //evil.co/..%2f../../X does not put //evil.co/ in Location.

        //netloc/ in a Location header is a redirect to a new host.
        https://github.com/python/cpython/issues/87389

        This checks that a path resolving to a directory on our server cannot
        resolve into a redirect to another server.
        """
        os.mkdir(os.path.join(self.tempdir, 'existing_directory'))
        url = f'/python.org/..%2f..%2f..%2f..%2f..%2f../%0a%0d/../{self.tempdir_name}/existing_directory'
        expected_location = f'{url}/'  # /python.org.../ single slash single prefix, trailing slash
        # Canonicalizes to /tmp/tempdir_name/existing_directory which does
        # exist and is a dir, triggering the 301 redirect logic.
        response = self.request(url)
        self.check_status_and_reason(response, HTTPStatus.MOVED_PERMANENTLY)
        location = response.getheader('Location')
        self.assertEqual(location, expected_location, msg='non-attack failed!')

        # //python.org... multi-slash prefix, no trailing slash
        attack_url = f'/{url}'
        response = self.request(attack_url)
        self.check_status_and_reason(response, HTTPStatus.MOVED_PERMANENTLY)
        location = response.getheader('Location')
        self.assertNotStartsWith(location, '//')
        self.assertEqual(location, expected_location,
                msg='Expected Location header to start with a single / and '
                'end with a / as this is a directory redirect.')

        # ///python.org... triple-slash prefix, no trailing slash
        attack3_url = f'//{url}'
        response = self.request(attack3_url)
        self.check_status_and_reason(response, HTTPStatus.MOVED_PERMANENTLY)
        self.assertEqual(response.getheader('Location'), expected_location)

        # If the second word in the http request (Request-URI for the http
        # method) is a full URI, we don't worry about it, as that'll be parsed
        # and reassembled as a full URI within BaseHTTPRequestHandler.send_head
        # so no errant scheme-less //netloc//evil.co/ domain mixup can happen.
        attack_scheme_netloc_2slash_url = f'https://pypi.org/{url}'
        expected_scheme_netloc_location = f'{attack_scheme_netloc_2slash_url}/'
        response = self.request(attack_scheme_netloc_2slash_url)
        self.check_status_and_reason(response, HTTPStatus.MOVED_PERMANENTLY)
        location = response.getheader('Location')
        # We're just ensuring that the scheme and domain make it through, if
        # there are or aren't multiple slashes at the start of the path that
        # follows that isn't important in this Location: header.
        self.assertStartsWith(location, 'https://pypi.org/')

    def test_get(self):
        #constructs the path relative to the root directory of the HTTPServer
        response = self.request(self.base_url + '/test')
        self.check_status_and_reason(response, HTTPStatus.OK, data=self.data)
        # check for trailing "/" which should return 404. See Issue17324
        response = self.request(self.base_url + '/test/')
        self.check_status_and_reason(response, HTTPStatus.NOT_FOUND)
        response = self.request(self.base_url + '/test%2f')
        self.check_status_and_reason(response, HTTPStatus.NOT_FOUND)
        response = self.request(self.base_url + '/test%2F')
        self.check_status_and_reason(response, HTTPStatus.NOT_FOUND)
        response = self.request(self.base_url + '/')
        self.check_status_and_reason(response, HTTPStatus.OK)
        response = self.request(self.base_url + '%2f')
        self.check_status_and_reason(response, HTTPStatus.OK)
        response = self.request(self.base_url + '%2F')
        self.check_status_and_reason(response, HTTPStatus.OK)
        response = self.request(self.base_url)
        self.check_status_and_reason(response, HTTPStatus.MOVED_PERMANENTLY)
        self.assertEqual(response.getheader("Location"), self.base_url + "/")
        self.assertEqual(response.getheader("Content-Length"), "0")
        response = self.request(self.base_url + '/?hi=2')
        self.check_status_and_reason(response, HTTPStatus.OK)
        response = self.request(self.base_url + '?hi=1')
        self.check_status_and_reason(response, HTTPStatus.MOVED_PERMANENTLY)
        self.assertEqual(response.getheader("Location"),
                         self.base_url + "/?hi=1")
        response = self.request('/ThisDoesNotExist')
        self.check_status_and_reason(response, HTTPStatus.NOT_FOUND)
        response = self.request('/' + 'ThisDoesNotExist' + '/')
        self.check_status_and_reason(response, HTTPStatus.NOT_FOUND)
        os.makedirs(os.path.join(self.tempdir, 'spam', 'index.html'))
        response = self.request(self.base_url + '/spam/')
        self.check_status_and_reason(response, HTTPStatus.OK)

        data = b"Dummy index file\r\n"
        with open(os.path.join(self.tempdir_name, 'index.html'), 'wb') as f:
            f.write(data)
        response = self.request(self.base_url + '/')
        self.check_status_and_reason(response, HTTPStatus.OK, data)

        # chmod() doesn't work as expected on Windows, and filesystem
        # permissions are ignored by root on Unix.
        if os.name == 'posix' and os.geteuid() != 0:
            os.chmod(self.tempdir, 0)
            try:
                response = self.request(self.base_url + '/')
                self.check_status_and_reason(response, HTTPStatus.NOT_FOUND)
            finally:
                os.chmod(self.tempdir, 0o755)

    def test_head(self):
        response = self.request(
            self.base_url + '/test', method='HEAD')
        self.check_status_and_reason(response, HTTPStatus.OK)
        self.assertEqual(response.getheader('content-length'),
                         str(len(self.data)))
        self.assertEqual(response.getheader('content-type'),
                         'application/octet-stream')

    def test_browser_cache(self):
        """Check that when a request to /test is sent with the request header
        If-Modified-Since set to date of last modification, the server returns
        status code 304, not 200
        """
        headers = email.message.Message()
        headers['If-Modified-Since'] = self.last_modif_header
        response = self.request(self.base_url + '/test', headers=headers)
        self.check_status_and_reason(response, HTTPStatus.NOT_MODIFIED)

        # one hour after last modification : must return 304
        new_dt = self.last_modif_datetime + datetime.timedelta(hours=1)
        headers = email.message.Message()
        headers['If-Modified-Since'] = email.utils.format_datetime(new_dt,
            usegmt=True)
        response = self.request(self.base_url + '/test', headers=headers)
        self.check_status_and_reason(response, HTTPStatus.NOT_MODIFIED)

    def test_browser_cache_file_changed(self):
        # with If-Modified-Since earlier than Last-Modified, must return 200
        dt = self.last_modif_datetime
        # build datetime object : 365 days before last modification
        old_dt = dt - datetime.timedelta(days=365)
        headers = email.message.Message()
        headers['If-Modified-Since'] = email.utils.format_datetime(old_dt,
            usegmt=True)
        response = self.request(self.base_url + '/test', headers=headers)
        self.check_status_and_reason(response, HTTPStatus.OK)

    def test_browser_cache_with_If_None_Match_header(self):
        # if If-None-Match header is present, ignore If-Modified-Since

        headers = email.message.Message()
        headers['If-Modified-Since'] = self.last_modif_header
        headers['If-None-Match'] = "*"
        response = self.request(self.base_url + '/test', headers=headers)
        self.check_status_and_reason(response, HTTPStatus.OK)

    def test_invalid_requests(self):
        response = self.request('/', method='FOO')
        self.check_status_and_reason(response, HTTPStatus.NOT_IMPLEMENTED)
        # requests must be case sensitive,so this should fail too
        response = self.request('/', method='custom')
        self.check_status_and_reason(response, HTTPStatus.NOT_IMPLEMENTED)
        response = self.request('/', method='GETs')
        self.check_status_and_reason(response, HTTPStatus.NOT_IMPLEMENTED)

    def test_last_modified(self):
        """Checks that the datetime returned in Last-Modified response header
        is the actual datetime of last modification, rounded to the second
        """
        response = self.request(self.base_url + '/test')
        self.check_status_and_reason(response, HTTPStatus.OK, data=self.data)
        last_modif_header = response.headers['Last-modified']
        self.assertEqual(last_modif_header, self.last_modif_header)

    def test_path_without_leading_slash(self):
        response = self.request(self.tempdir_name + '/test')
        self.check_status_and_reason(response, HTTPStatus.OK, data=self.data)
        response = self.request(self.tempdir_name + '/test/')
        self.check_status_and_reason(response, HTTPStatus.NOT_FOUND)
        response = self.request(self.tempdir_name + '/')
        self.check_status_and_reason(response, HTTPStatus.OK)
        response = self.request(self.tempdir_name)
        self.check_status_and_reason(response, HTTPStatus.MOVED_PERMANENTLY)
        self.assertEqual(response.getheader("Location"),
                         self.tempdir_name + "/")
        response = self.request(self.tempdir_name + '/?hi=2')
        self.check_status_and_reason(response, HTTPStatus.OK)
        response = self.request(self.tempdir_name + '?hi=1')
        self.check_status_and_reason(response, HTTPStatus.MOVED_PERMANENTLY)
        self.assertEqual(response.getheader("Location"),
                         self.tempdir_name + "/?hi=1")


class SocketlessRequestHandler(SimpleHTTPRequestHandler):
    def __init__(self, directory=None):
        request = mock.Mock()
        request.makefile.return_value = BytesIO()
        super().__init__(request, None, None, directory=directory)

        self.get_called = False
        self.protocol_version = "HTTP/1.1"

    def do_GET(self):
        self.get_called = True
        self.send_response(HTTPStatus.OK)
        self.send_header('Content-Type', 'text/html')
        self.end_headers()
        self.wfile.write(b'<html><body>Data</body></html>\r\n')

    def log_message(self, format, *args):
        pass


class RejectingSocketlessRequestHandler(SocketlessRequestHandler):
    def handle_expect_100(self):
        self.send_error(HTTPStatus.EXPECTATION_FAILED)
        return False


class AuditableBytesIO:

    def __init__(self):
        self.datas = []

    def write(self, data):
        self.datas.append(data)

    def getData(self):
        return b''.join(self.datas)

    @property
    def numWrites(self):
        return len(self.datas)


class BaseHTTPRequestHandlerTestCase(unittest.TestCase):
    """Test the functionality of the BaseHTTPServer.

       Test the support for the Expect 100-continue header.
       """

    HTTPResponseMatch = re.compile(b'HTTP/1.[0-9]+ 200 OK')

    def setUp (self):
        self.handler = SocketlessRequestHandler()

    def send_typical_request(self, message):
        input = BytesIO(message)
        output = BytesIO()
        self.handler.rfile = input
        self.handler.wfile = output
        self.handler.handle_one_request()
        output.seek(0)
        return output.readlines()

    def verify_get_called(self):
        self.assertTrue(self.handler.get_called)

    def verify_expected_headers(self, headers):
        for fieldName in b'Server: ', b'Date: ', b'Content-Type: ':
            self.assertEqual(sum(h.startswith(fieldName) for h in headers), 1)

    def verify_http_server_response(self, response):
        match = self.HTTPResponseMatch.search(response)
        self.assertIsNotNone(match)

    def test_unprintable_not_logged(self):
        # We call the method from the class directly as our Socketless
        # Handler subclass overrode it... nice for everything BUT this test.
        self.handler.client_address = ('127.0.0.1', 1337)
        log_message = BaseHTTPRequestHandler.log_message
        with mock.patch.object(sys, 'stderr', StringIO()) as fake_stderr:
            log_message(self.handler, '/foo')
            log_message(self.handler, '/\033bar\000\033')
            log_message(self.handler, '/spam %s.', 'a')
            log_message(self.handler, '/spam %s.', '\033\x7f\x9f\xa0beans')
            log_message(self.handler, '"GET /foo\\b"ar\007 HTTP/1.0"')
        stderr = fake_stderr.getvalue()
        self.assertNotIn('\033', stderr)  # non-printable chars are caught.
        self.assertNotIn('\000', stderr)  # non-printable chars are caught.
        lines = stderr.splitlines()
        self.assertIn('/foo', lines[0])
        self.assertIn(r'/\x1bbar\x00\x1b', lines[1])
        self.assertIn('/spam a.', lines[2])
        self.assertIn('/spam \\x1b\\x7f\\x9f\xa0beans.', lines[3])
        self.assertIn(r'"GET /foo\\b"ar\x07 HTTP/1.0"', lines[4])

    def test_http_1_1(self):
        result = self.send_typical_request(b'GET / HTTP/1.1\r\n\r\n')
        self.verify_http_server_response(result[0])
        self.verify_expected_headers(result[1:-1])
        self.verify_get_called()
        self.assertEqual(result[-1], b'<html><body>Data</body></html>\r\n')
        self.assertEqual(self.handler.requestline, 'GET / HTTP/1.1')
        self.assertEqual(self.handler.command, 'GET')
        self.assertEqual(self.handler.path, '/')
        self.assertEqual(self.handler.request_version, 'HTTP/1.1')
        self.assertSequenceEqual(self.handler.headers.items(), ())

    def test_http_1_0(self):
        result = self.send_typical_request(b'GET / HTTP/1.0\r\n\r\n')
        self.verify_http_server_response(result[0])
        self.verify_expected_headers(result[1:-1])
        self.verify_get_called()
        self.assertEqual(result[-1], b'<html><body>Data</body></html>\r\n')
        self.assertEqual(self.handler.requestline, 'GET / HTTP/1.0')
        self.assertEqual(self.handler.command, 'GET')
        self.assertEqual(self.handler.path, '/')
        self.assertEqual(self.handler.request_version, 'HTTP/1.0')
        self.assertSequenceEqual(self.handler.headers.items(), ())

    def test_http_0_9(self):
        result = self.send_typical_request(b'GET / HTTP/0.9\r\n\r\n')
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0], b'<html><body>Data</body></html>\r\n')
        self.verify_get_called()

    def test_extra_space(self):
        result = self.send_typical_request(
            b'GET /spaced out HTTP/1.1\r\n'
            b'Host: dummy\r\n'
            b'\r\n'
        )
        self.assertStartsWith(result[0], b'HTTP/1.1 400 ')
        self.verify_expected_headers(result[1:result.index(b'\r\n')])
        self.assertFalse(self.handler.get_called)

    def test_with_continue_1_0(self):
        result = self.send_typical_request(b'GET / HTTP/1.0\r\nExpect: 100-continue\r\n\r\n')
        self.verify_http_server_response(result[0])
        self.verify_expected_headers(result[1:-1])
        self.verify_get_called()
        self.assertEqual(result[-1], b'<html><body>Data</body></html>\r\n')
        self.assertEqual(self.handler.requestline, 'GET / HTTP/1.0')
        self.assertEqual(self.handler.command, 'GET')
        self.assertEqual(self.handler.path, '/')
        self.assertEqual(self.handler.request_version, 'HTTP/1.0')
        headers = (("Expect", "100-continue"),)
        self.assertSequenceEqual(self.handler.headers.items(), headers)

    def test_with_continue_1_1(self):
        result = self.send_typical_request(b'GET / HTTP/1.1\r\nExpect: 100-continue\r\n\r\n')
        self.assertEqual(result[0], b'HTTP/1.1 100 Continue\r\n')
        self.assertEqual(result[1], b'\r\n')
        self.assertEqual(result[2], b'HTTP/1.1 200 OK\r\n')
        self.verify_expected_headers(result[2:-1])
        self.verify_get_called()
        self.assertEqual(result[-1], b'<html><body>Data</body></html>\r\n')
        self.assertEqual(self.handler.requestline, 'GET / HTTP/1.1')
        self.assertEqual(self.handler.command, 'GET')
        self.assertEqual(self.handler.path, '/')
        self.assertEqual(self.handler.request_version, 'HTTP/1.1')
        headers = (("Expect", "100-continue"),)
        self.assertSequenceEqual(self.handler.headers.items(), headers)

    def test_header_buffering_of_send_error(self):

        input = BytesIO(b'GET / HTTP/1.1\r\n\r\n')
        output = AuditableBytesIO()
        handler = SocketlessRequestHandler()
        handler.rfile = input
        handler.wfile = output
        handler.request_version = 'HTTP/1.1'
        handler.requestline = ''
        handler.command = None

        handler.send_error(418)
        self.assertEqual(output.numWrites, 2)

    def test_header_buffering_of_send_response_only(self):

        input = BytesIO(b'GET / HTTP/1.1\r\n\r\n')
        output = AuditableBytesIO()
        handler = SocketlessRequestHandler()
        handler.rfile = input
        handler.wfile = output
        handler.request_version = 'HTTP/1.1'

        handler.send_response_only(418)
        self.assertEqual(output.numWrites, 0)
        handler.end_headers()
        self.assertEqual(output.numWrites, 1)

    def test_header_buffering_of_send_header(self):

        input = BytesIO(b'GET / HTTP/1.1\r\n\r\n')
        output = AuditableBytesIO()
        handler = SocketlessRequestHandler()
        handler.rfile = input
        handler.wfile = output
        handler.request_version = 'HTTP/1.1'

        handler.send_header('Foo', 'foo')
        handler.send_header('bar', 'bar')
        self.assertEqual(output.numWrites, 0)
        handler.end_headers()
        self.assertEqual(output.getData(), b'Foo: foo\r\nbar: bar\r\n\r\n')
        self.assertEqual(output.numWrites, 1)

    def test_header_unbuffered_when_continue(self):

        def _readAndReseek(f):
            pos = f.tell()
            f.seek(0)
            data = f.read()
            f.seek(pos)
            return data

        input = BytesIO(b'GET / HTTP/1.1\r\nExpect: 100-continue\r\n\r\n')
        output = BytesIO()
        self.handler.rfile = input
        self.handler.wfile = output
        self.handler.request_version = 'HTTP/1.1'

        self.handler.handle_one_request()
        self.assertNotEqual(_readAndReseek(output), b'')
        result = _readAndReseek(output).split(b'\r\n')
        self.assertEqual(result[0], b'HTTP/1.1 100 Continue')
        self.assertEqual(result[1], b'')
        self.assertEqual(result[2], b'HTTP/1.1 200 OK')

    def test_with_continue_rejected(self):
        usual_handler = self.handler        # Save to avoid breaking any subsequent tests.
        self.handler = RejectingSocketlessRequestHandler()
        result = self.send_typical_request(b'GET / HTTP/1.1\r\nExpect: 100-continue\r\n\r\n')
        self.assertEqual(result[0], b'HTTP/1.1 417 Expectation Failed\r\n')
        self.verify_expected_headers(result[1:-1])
        # The expect handler should short circuit the usual get method by
        # returning false here, so get_called should be false
        self.assertFalse(self.handler.get_called)
        self.assertEqual(sum(r == b'Connection: close\r\n' for r in result[1:-1]), 1)
        self.handler = usual_handler        # Restore to avoid breaking any subsequent tests.

    def test_request_length(self):
        # Issue #10714: huge request lines are discarded, to avoid Denial
        # of Service attacks.
        result = self.send_typical_request(b'GET ' + b'x' * 65537)
        self.assertEqual(result[0], b'HTTP/1.1 414 URI Too Long\r\n')
        self.assertFalse(self.handler.get_called)
        self.assertIsInstance(self.handler.requestline, str)

    def test_header_length(self):
        # Issue #6791: same for headers
        result = self.send_typical_request(
            b'GET / HTTP/1.1\r\nX-Foo: bar' + b'r' * 65537 + b'\r\n\r\n')
        self.assertEqual(result[0], b'HTTP/1.1 431 Line too long\r\n')
        self.assertFalse(self.handler.get_called)
        self.assertEqual(self.handler.requestline, 'GET / HTTP/1.1')

    def test_too_many_headers(self):
        result = self.send_typical_request(
            b'GET / HTTP/1.1\r\n' + b'X-Foo: bar\r\n' * 101 + b'\r\n')
        self.assertEqual(result[0], b'HTTP/1.1 431 Too many headers\r\n')
        self.assertFalse(self.handler.get_called)
        self.assertEqual(self.handler.requestline, 'GET / HTTP/1.1')

    def test_html_escape_on_error(self):
        result = self.send_typical_request(
            b'<script>alert("hello")</script> / HTTP/1.1')
        result = b''.join(result)
        text = '<script>alert("hello")</script>'
        self.assertIn(html.escape(text, quote=False).encode('ascii'), result)

    def test_close_connection(self):
        # handle_one_request() should be repeatedly called until
        # it sets close_connection
        def handle_one_request():
            self.handler.close_connection = next(close_values)
        self.handler.handle_one_request = handle_one_request

        close_values = iter((True,))
        self.handler.handle()
        self.assertRaises(StopIteration, next, close_values)

        close_values = iter((False, False, True))
        self.handler.handle()
        self.assertRaises(StopIteration, next, close_values)

    def test_date_time_string(self):
        now = time.time()
        # this is the old code that formats the timestamp
        year, month, day, hh, mm, ss, wd, y, z = time.gmtime(now)
        expected = "%s, %02d %3s %4d %02d:%02d:%02d GMT" % (
            self.handler.weekdayname[wd],
            day,
            self.handler.monthname[month],
            year, hh, mm, ss
        )
        self.assertEqual(self.handler.date_time_string(timestamp=now), expected)


class SimpleHTTPRequestHandlerTestCase(unittest.TestCase):
    """ Test url parsing """
    def setUp(self):
        self.translated_1 = os.path.join(os.getcwd(), 'filename')
        self.translated_2 = os.path.join('foo', 'filename')
        self.translated_3 = os.path.join('bar', 'filename')
        self.handler_1 = SocketlessRequestHandler()
        self.handler_2 = SocketlessRequestHandler(directory='foo')
        self.handler_3 = SocketlessRequestHandler(directory=pathlib.PurePath('bar'))

    def test_query_arguments(self):
        path = self.handler_1.translate_path('/filename')
        self.assertEqual(path, self.translated_1)
        path = self.handler_2.translate_path('/filename')
        self.assertEqual(path, self.translated_2)
        path = self.handler_3.translate_path('/filename')
        self.assertEqual(path, self.translated_3)

        path = self.handler_1.translate_path('/filename?foo=bar')
        self.assertEqual(path, self.translated_1)
        path = self.handler_2.translate_path('/filename?foo=bar')
        self.assertEqual(path, self.translated_2)
        path = self.handler_3.translate_path('/filename?foo=bar')
        self.assertEqual(path, self.translated_3)

        path = self.handler_1.translate_path('/filename?a=b&spam=eggs#zot')
        self.assertEqual(path, self.translated_1)
        path = self.handler_2.translate_path('/filename?a=b&spam=eggs#zot')
        self.assertEqual(path, self.translated_2)
        path = self.handler_3.translate_path('/filename?a=b&spam=eggs#zot')
        self.assertEqual(path, self.translated_3)

    def test_start_with_double_slash(self):
        path = self.handler_1.translate_path('//filename')
        self.assertEqual(path, self.translated_1)
        path = self.handler_2.translate_path('//filename')
        self.assertEqual(path, self.translated_2)
        path = self.handler_3.translate_path('//filename')
        self.assertEqual(path, self.translated_3)

        path = self.handler_1.translate_path('//filename?foo=bar')
        self.assertEqual(path, self.translated_1)
        path = self.handler_2.translate_path('//filename?foo=bar')
        self.assertEqual(path, self.translated_2)
        path = self.handler_3.translate_path('//filename?foo=bar')
        self.assertEqual(path, self.translated_3)

    def test_windows_colon(self):
        with support.swap_attr(server.os, 'path', ntpath):
            path = self.handler_1.translate_path('c:c:c:foo/filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated_1)
            path = self.handler_2.translate_path('c:c:c:foo/filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated_2)
            path = self.handler_3.translate_path('c:c:c:foo/filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated_3)

            path = self.handler_1.translate_path('\\c:../filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated_1)
            path = self.handler_2.translate_path('\\c:../filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated_2)
            path = self.handler_3.translate_path('\\c:../filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated_3)

            path = self.handler_1.translate_path('c:\\c:..\\foo/filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated_1)
            path = self.handler_2.translate_path('c:\\c:..\\foo/filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated_2)
            path = self.handler_3.translate_path('c:\\c:..\\foo/filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated_3)

            path = self.handler_1.translate_path('c:c:foo\\c:c:bar/filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated_1)
            path = self.handler_2.translate_path('c:c:foo\\c:c:bar/filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated_2)
            path = self.handler_3.translate_path('c:c:foo\\c:c:bar/filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated_3)


class MiscTestCase(unittest.TestCase):
    def test_all(self):
        expected = []
        denylist = {'executable', 'nobody_uid', 'test'}
        for name in dir(server):
            if name.startswith('_') or name in denylist:
                continue
            module_object = getattr(server, name)
            if getattr(module_object, '__module__', None) == 'http.server':
                expected.append(name)
        self.assertCountEqual(server.__all__, expected)


class ScriptTestCase(unittest.TestCase):

    def mock_server_class(self):
        return mock.MagicMock(
            return_value=mock.MagicMock(
                __enter__=mock.MagicMock(
                    return_value=mock.MagicMock(
                        socket=mock.MagicMock(
                            getsockname=lambda: ('', 0),
                        ),
                    ),
                ),
            ),
        )

    @mock.patch('builtins.print')
    def test_server_test_unspec(self, _):
        mock_server = self.mock_server_class()
        server.test(ServerClass=mock_server, bind=None)
        self.assertIn(
            mock_server.address_family,
            (socket.AF_INET6, socket.AF_INET),
        )

    @mock.patch('builtins.print')
    def test_server_test_localhost(self, _):
        mock_server = self.mock_server_class()
        server.test(ServerClass=mock_server, bind="localhost")
        self.assertIn(
            mock_server.address_family,
            (socket.AF_INET6, socket.AF_INET),
        )

    ipv6_addrs = (
        "::",
        "2001:0db8:85a3:0000:0000:8a2e:0370:7334",
        "::1",
    )

    ipv4_addrs = (
        "0.0.0.0",
        "8.8.8.8",
        "127.0.0.1",
    )

    @mock.patch('builtins.print')
    def test_server_test_ipv6(self, _):
        for bind in self.ipv6_addrs:
            mock_server = self.mock_server_class()
            server.test(ServerClass=mock_server, bind=bind)
            self.assertEqual(mock_server.address_family, socket.AF_INET6)

    @mock.patch('builtins.print')
    def test_server_test_ipv4(self, _):
        for bind in self.ipv4_addrs:
            mock_server = self.mock_server_class()
            server.test(ServerClass=mock_server, bind=bind)
            self.assertEqual(mock_server.address_family, socket.AF_INET)


class CommandLineTestCase(unittest.TestCase):
    default_port = 8000
    default_bind = None
    default_protocol = 'HTTP/1.0'
    default_handler = SimpleHTTPRequestHandler
    default_server = unittest.mock.ANY
    tls_cert = certdata_file('ssl_cert.pem')
    tls_key = certdata_file('ssl_key.pem')
    tls_password = 'somepass'
    tls_cert_options = ['--tls-cert']
    tls_key_options = ['--tls-key']
    tls_password_options = ['--tls-password-file']
    args = {
        'HandlerClass': default_handler,
        'ServerClass': default_server,
        'protocol': default_protocol,
        'port': default_port,
        'bind': default_bind,
        'tls_cert': None,
        'tls_key': None,
        'tls_password': None,
    }

    def setUp(self):
        super().setUp()
        self.tls_password_file = tempfile.mktemp()
        with open(self.tls_password_file, 'wb') as f:
            f.write(self.tls_password.encode())
        self.addCleanup(os_helper.unlink, self.tls_password_file)

    def invoke_httpd(self, *args, stdout=None, stderr=None):
        stdout = StringIO() if stdout is None else stdout
        stderr = StringIO() if stderr is None else stderr
        with contextlib.redirect_stdout(stdout), \
            contextlib.redirect_stderr(stderr):
            server._main(args)
        return stdout.getvalue(), stderr.getvalue()

    @mock.patch('http.server.test')
    def test_port_flag(self, mock_func):
        ports = [8000, 65535]
        for port in ports:
            with self.subTest(port=port):
                self.invoke_httpd(str(port))
                call_args = self.args | dict(port=port)
                mock_func.assert_called_once_with(**call_args)
                mock_func.reset_mock()

    @mock.patch('http.server.test')
    def test_directory_flag(self, mock_func):
        options = ['-d', '--directory']
        directories = ['.', '/foo', '\\bar', '/',
                       'C:\\', 'C:\\foo', 'C:\\bar',
                       '/home/user', './foo/foo2', 'D:\\foo\\bar']
        for flag in options:
            for directory in directories:
                with self.subTest(flag=flag, directory=directory):
                    self.invoke_httpd(flag, directory)
                    mock_func.assert_called_once_with(**self.args)
                    mock_func.reset_mock()

    @mock.patch('http.server.test')
    def test_bind_flag(self, mock_func):
        options = ['-b', '--bind']
        bind_addresses = ['localhost', '127.0.0.1', '::1',
                          '0.0.0.0', '8.8.8.8']
        for flag in options:
            for bind_address in bind_addresses:
                with self.subTest(flag=flag, bind_address=bind_address):
                    self.invoke_httpd(flag, bind_address)
                    call_args = self.args | dict(bind=bind_address)
                    mock_func.assert_called_once_with(**call_args)
                    mock_func.reset_mock()

    @mock.patch('http.server.test')
    def test_protocol_flag(self, mock_func):
        options = ['-p', '--protocol']
        protocols = ['HTTP/1.0', 'HTTP/1.1', 'HTTP/2.0', 'HTTP/3.0']
        for flag in options:
            for protocol in protocols:
                with self.subTest(flag=flag, protocol=protocol):
                    self.invoke_httpd(flag, protocol)
                    call_args = self.args | dict(protocol=protocol)
                    mock_func.assert_called_once_with(**call_args)
                    mock_func.reset_mock()

    @unittest.skipIf(ssl is None, "requires ssl")
    @mock.patch('http.server.test')
    def test_tls_cert_and_key_flags(self, mock_func):
        for tls_cert_option in self.tls_cert_options:
            for tls_key_option in self.tls_key_options:
                self.invoke_httpd(tls_cert_option, self.tls_cert,
                                  tls_key_option, self.tls_key)
                call_args = self.args | {
                    'tls_cert': self.tls_cert,
                    'tls_key': self.tls_key,
                }
                mock_func.assert_called_once_with(**call_args)
                mock_func.reset_mock()

    @unittest.skipIf(ssl is None, "requires ssl")
    @mock.patch('http.server.test')
    def test_tls_cert_and_key_and_password_flags(self, mock_func):
        for tls_cert_option in self.tls_cert_options:
            for tls_key_option in self.tls_key_options:
                for tls_password_option in self.tls_password_options:
                    self.invoke_httpd(tls_cert_option,
                                      self.tls_cert,
                                      tls_key_option,
                                      self.tls_key,
                                      tls_password_option,
                                      self.tls_password_file)
                    call_args = self.args | {
                        'tls_cert': self.tls_cert,
                        'tls_key': self.tls_key,
                        'tls_password': self.tls_password,
                    }
                    mock_func.assert_called_once_with(**call_args)
                    mock_func.reset_mock()

    @unittest.skipIf(ssl is None, "requires ssl")
    @mock.patch('http.server.test')
    def test_missing_tls_cert_flag(self, mock_func):
        for tls_key_option in self.tls_key_options:
            with self.assertRaises(SystemExit):
                self.invoke_httpd(tls_key_option, self.tls_key)
            mock_func.reset_mock()

        for tls_password_option in self.tls_password_options:
            with self.assertRaises(SystemExit):
                self.invoke_httpd(tls_password_option, self.tls_password)
            mock_func.reset_mock()

    @unittest.skipIf(ssl is None, "requires ssl")
    @mock.patch('http.server.test')
    def test_invalid_password_file(self, mock_func):
        non_existent_file = 'non_existent_file'
        for tls_password_option in self.tls_password_options:
            for tls_cert_option in self.tls_cert_options:
                with self.assertRaises(SystemExit):
                    self.invoke_httpd(tls_cert_option,
                                      self.tls_cert,
                                      tls_password_option,
                                      non_existent_file)

    @mock.patch('http.server.test')
    def test_no_arguments(self, mock_func):
        self.invoke_httpd()
        mock_func.assert_called_once_with(**self.args)
        mock_func.reset_mock()

    @mock.patch('http.server.test')
    def test_help_flag(self, _):
        options = ['-h', '--help']
        for option in options:
            stdout, stderr = StringIO(), StringIO()
            with self.assertRaises(SystemExit):
                self.invoke_httpd(option, stdout=stdout, stderr=stderr)
            self.assertIn('usage', stdout.getvalue())
            self.assertEqual(stderr.getvalue(), '')

    @mock.patch('http.server.test')
    def test_unknown_flag(self, _):
        stdout, stderr = StringIO(), StringIO()
        with self.assertRaises(SystemExit):
            self.invoke_httpd('--unknown-flag', stdout=stdout, stderr=stderr)
        self.assertEqual(stdout.getvalue(), '')
        self.assertIn('error', stderr.getvalue())


class CommandLineRunTimeTestCase(unittest.TestCase):
    served_data = os.urandom(32)
    served_filename = 'served_filename'
    tls_cert = certdata_file('ssl_cert.pem')
    tls_key = certdata_file('ssl_key.pem')
    tls_password = b'somepass'
    tls_password_file = 'ssl_key_password'

    def setUp(self):
        super().setUp()
        server_dir_context = os_helper.temp_cwd()
        server_dir = self.enterContext(server_dir_context)
        with open(self.served_filename, 'wb') as f:
            f.write(self.served_data)
        with open(self.tls_password_file, 'wb') as f:
            f.write(self.tls_password)

    def fetch_file(self, path, context=None):
        req = urllib.request.Request(path, method='GET')
        with urllib.request.urlopen(req, context=context) as res:
            return res.read()

    def parse_cli_output(self, output):
        match = re.search(r'Serving (HTTP|HTTPS) on (.+) port (\d+)', output)
        if match is None:
            return None, None, None
        return match.group(1).lower(), match.group(2), int(match.group(3))

    def wait_for_server(self, proc, protocol, bind, port):
        """Check that the server has been successfully started."""
        line = proc.stdout.readline().strip()
        if support.verbose:
            print()
            print('python -m http.server: ', line)
        return self.parse_cli_output(line) == (protocol, bind, port)

    def test_http_client(self):
        bind, port = '127.0.0.1', find_unused_port()
        proc = spawn_python('-u', '-m', 'http.server', str(port), '-b', bind,
                            bufsize=1, text=True)
        self.addCleanup(kill_python, proc)
        self.addCleanup(proc.terminate)
        self.assertTrue(self.wait_for_server(proc, 'http', bind, port))
        res = self.fetch_file(f'http://{bind}:{port}/{self.served_filename}')
        self.assertEqual(res, self.served_data)

    @unittest.skipIf(ssl is None, "requires ssl")
    def test_https_client(self):
        context = ssl.create_default_context()
        # allow self-signed certificates
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE

        bind, port = '127.0.0.1', find_unused_port()
        proc = spawn_python('-u', '-m', 'http.server', str(port), '-b', bind,
                            '--tls-cert', self.tls_cert,
                            '--tls-key', self.tls_key,
                            '--tls-password-file', self.tls_password_file,
                            bufsize=1, text=True)
        self.addCleanup(kill_python, proc)
        self.addCleanup(proc.terminate)
        self.assertTrue(self.wait_for_server(proc, 'https', bind, port))
        url = f'https://{bind}:{port}/{self.served_filename}'
        res = self.fetch_file(url, context=context)
        self.assertEqual(res, self.served_data)


def setUpModule():
    unittest.addModuleCleanup(os.chdir, os.getcwd())


if __name__ == '__main__':
    unittest.main()
