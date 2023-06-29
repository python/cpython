"""Unittests for the various HTTPServer modules.

Written by Cody A.W. Somerville <cody-somerville@ubuntu.com>,
Josip Dzolonga, and Michael Otteneder for the 2007/08 GHOP contest.
"""
from collections import OrderedDict
from http.server import BaseHTTPRequestHandler, HTTPServer, \
     SimpleHTTPRequestHandler, CGIHTTPRequestHandler
from http import server, HTTPStatus

import os
import socket
import sys
import re
import base64
import ntpath
import pathlib
import shutil
import email.message
import email.utils
import html
import http, http.client
import urllib.parse
import tempfile
import time
import datetime
import threading
from unittest import mock
from io import BytesIO, StringIO

import unittest
from test import support
from test.support import os_helper
from test.support import threading_helper

support.requires_working_socket(module=True)

class NoLogRequestHandler:
    def log_message(self, *args):
        # don't write log messages to stderr
        pass

    def read(self, n=None):
        return ''


class TestServerThread(threading.Thread):
    def __init__(self, test_object, request_handler):
        threading.Thread.__init__(self)
        self.request_handler = request_handler
        self.test_object = test_object

    def run(self):
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
    def setUp(self):
        self._threads = threading_helper.threading_setup()
        os.environ = os_helper.EnvironmentVarGuard()
        self.server_started = threading.Event()
        self.thread = TestServerThread(self, self.request_handler)
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

        self.assertTrue(
            err.getvalue().endswith('"GET / HTTP/1.1" 200 -\n'))

    def test_err(self):
        self.con = http.client.HTTPConnection(self.HOST, self.PORT)
        self.con.connect()

        with support.captured_stderr() as err:
            self.con.request('ERROR', '/')
            self.con.getresponse()

        lines = err.getvalue().split('\n')
        self.assertTrue(lines[0].endswith('code 404, message File not found'))
        self.assertTrue(lines[1].endswith('"ERROR / HTTP/1.1" 404 -'))


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

    @unittest.skipIf(sys.platform == 'darwin',
                     'undecodable name cannot always be decoded on macOS')
    @unittest.skipIf(sys.platform == 'win32',
                     'undecodable name cannot be decoded on win32')
    @unittest.skipUnless(os_helper.TESTFN_UNDECODABLE,
                         'need os_helper.TESTFN_UNDECODABLE')
    def test_undecodable_filename(self):
        enc = sys.getfilesystemencoding()
        filename = os.fsdecode(os_helper.TESTFN_UNDECODABLE) + '.txt'
        with open(os.path.join(self.tempdir, filename), 'wb') as f:
            f.write(os_helper.TESTFN_UNDECODABLE)
        response = self.request(self.base_url + '/')
        if sys.platform == 'darwin':
            # On Mac OS the HFS+ filesystem replaces bytes that aren't valid
            # UTF-8 into a percent-encoded value.
            for name in os.listdir(self.tempdir):
                if name != 'test': # Ignore a filename created in setUp().
                    filename = name
                    break
        body = self.check_status_and_reason(response, HTTPStatus.OK)
        quotedname = urllib.parse.quote(filename, errors='surrogatepass')
        self.assertIn(('href="%s"' % quotedname)
                      .encode(enc, 'surrogateescape'), body)
        self.assertIn(('>%s<' % html.escape(filename, quote=False))
                      .encode(enc, 'surrogateescape'), body)
        response = self.request(self.base_url + '/' + quotedname)
        self.check_status_and_reason(response, HTTPStatus.OK,
                                     data=os_helper.TESTFN_UNDECODABLE)

    def test_undecodable_parameter(self):
        # sanity check using a valid parameter
        response = self.request(self.base_url + '/?x=123').read()
        self.assertRegex(response, rf'listing for {self.base_url}/\?x=123'.encode('latin1'))
        # now the bogus encoding
        response = self.request(self.base_url + '/?x=%bb').read()
        self.assertRegex(response, rf'listing for {self.base_url}/\?x=\xef\xbf\xbd'.encode('latin1'))

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
        self.assertFalse(location.startswith('//'), msg=location)
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
        self.assertTrue(location.startswith('https://pypi.org/'), msg=location)

    def test_get(self):
        #constructs the path relative to the root directory of the HTTPServer
        response = self.request(self.base_url + '/test')
        self.check_status_and_reason(response, HTTPStatus.OK, data=self.data)
        # check for trailing "/" which should return 404. See Issue17324
        response = self.request(self.base_url + '/test/')
        self.check_status_and_reason(response, HTTPStatus.NOT_FOUND)
        response = self.request(self.base_url + '/')
        self.check_status_and_reason(response, HTTPStatus.OK)
        response = self.request(self.base_url)
        self.check_status_and_reason(response, HTTPStatus.MOVED_PERMANENTLY)
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
        response = self.request(self.tempdir_name + '/?hi=2')
        self.check_status_and_reason(response, HTTPStatus.OK)
        response = self.request(self.tempdir_name + '?hi=1')
        self.check_status_and_reason(response, HTTPStatus.MOVED_PERMANENTLY)
        self.assertEqual(response.getheader("Location"),
                         self.tempdir_name + "/?hi=1")

    def test_html_escape_filename(self):
        filename = '<test&>.txt'
        fullpath = os.path.join(self.tempdir, filename)

        try:
            open(fullpath, 'wb').close()
        except OSError:
            raise unittest.SkipTest('Can not create file %s on current file '
                                    'system' % filename)

        try:
            response = self.request(self.base_url + '/')
            body = self.check_status_and_reason(response, HTTPStatus.OK)
            enc = response.headers.get_content_charset()
        finally:
            os.unlink(fullpath)  # avoid affecting test_undecodable_filename

        self.assertIsNotNone(enc)
        html_text = '>%s<' % html.escape(filename, quote=False)
        self.assertIn(html_text.encode(enc), body)


cgi_file1 = """\
#!%s

print("Content-type: text/html")
print()
print("Hello World")
"""

cgi_file2 = """\
#!%s
import os
import sys
import urllib.parse

print("Content-type: text/html")
print()

content_length = int(os.environ["CONTENT_LENGTH"])
query_string = sys.stdin.buffer.read(content_length)
params = {key.decode("utf-8"): val.decode("utf-8")
            for key, val in urllib.parse.parse_qsl(query_string)}

print("%%s, %%s, %%s" %% (params["spam"], params["eggs"], params["bacon"]))
"""

cgi_file4 = """\
#!%s
import os

print("Content-type: text/html")
print()

print(os.environ["%s"])
"""

cgi_file6 = """\
#!%s
import os

print("X-ambv: was here")
print("Content-type: text/html")
print()
print("<pre>")
for k, v in os.environ.items():
    try:
        k.encode('ascii')
        v.encode('ascii')
    except UnicodeEncodeError:
        continue  # see: BPO-44647
    print(f"{k}={v}")
print("</pre>")
"""


@unittest.skipIf(hasattr(os, 'geteuid') and os.geteuid() == 0,
        "This test can't be run reliably as root (issue #13308).")
class CGIHTTPServerTestCase(BaseTestCase):
    class request_handler(NoLogRequestHandler, CGIHTTPRequestHandler):
        pass

    linesep = os.linesep.encode('ascii')

    def setUp(self):
        BaseTestCase.setUp(self)
        self.cwd = os.getcwd()
        self.parent_dir = tempfile.mkdtemp()
        self.cgi_dir = os.path.join(self.parent_dir, 'cgi-bin')
        self.cgi_child_dir = os.path.join(self.cgi_dir, 'child-dir')
        self.sub_dir_1 = os.path.join(self.parent_dir, 'sub')
        self.sub_dir_2 = os.path.join(self.sub_dir_1, 'dir')
        self.cgi_dir_in_sub_dir = os.path.join(self.sub_dir_2, 'cgi-bin')
        os.mkdir(self.cgi_dir)
        os.mkdir(self.cgi_child_dir)
        os.mkdir(self.sub_dir_1)
        os.mkdir(self.sub_dir_2)
        os.mkdir(self.cgi_dir_in_sub_dir)
        self.nocgi_path = None
        self.file1_path = None
        self.file2_path = None
        self.file3_path = None
        self.file4_path = None
        self.file5_path = None

        # The shebang line should be pure ASCII: use symlink if possible.
        # See issue #7668.
        self._pythonexe_symlink = None
        if os_helper.can_symlink():
            self.pythonexe = os.path.join(self.parent_dir, 'python')
            self._pythonexe_symlink = support.PythonSymlink(self.pythonexe).__enter__()
        else:
            self.pythonexe = sys.executable

        try:
            # The python executable path is written as the first line of the
            # CGI Python script. The encoding cookie cannot be used, and so the
            # path should be encodable to the default script encoding (utf-8)
            self.pythonexe.encode('utf-8')
        except UnicodeEncodeError:
            self.tearDown()
            self.skipTest("Python executable path is not encodable to utf-8")

        self.nocgi_path = os.path.join(self.parent_dir, 'nocgi.py')
        with open(self.nocgi_path, 'w', encoding='utf-8') as fp:
            fp.write(cgi_file1 % self.pythonexe)
        os.chmod(self.nocgi_path, 0o777)

        self.file1_path = os.path.join(self.cgi_dir, 'file1.py')
        with open(self.file1_path, 'w', encoding='utf-8') as file1:
            file1.write(cgi_file1 % self.pythonexe)
        os.chmod(self.file1_path, 0o777)

        self.file2_path = os.path.join(self.cgi_dir, 'file2.py')
        with open(self.file2_path, 'w', encoding='utf-8') as file2:
            file2.write(cgi_file2 % self.pythonexe)
        os.chmod(self.file2_path, 0o777)

        self.file3_path = os.path.join(self.cgi_child_dir, 'file3.py')
        with open(self.file3_path, 'w', encoding='utf-8') as file3:
            file3.write(cgi_file1 % self.pythonexe)
        os.chmod(self.file3_path, 0o777)

        self.file4_path = os.path.join(self.cgi_dir, 'file4.py')
        with open(self.file4_path, 'w', encoding='utf-8') as file4:
            file4.write(cgi_file4 % (self.pythonexe, 'QUERY_STRING'))
        os.chmod(self.file4_path, 0o777)

        self.file5_path = os.path.join(self.cgi_dir_in_sub_dir, 'file5.py')
        with open(self.file5_path, 'w', encoding='utf-8') as file5:
            file5.write(cgi_file1 % self.pythonexe)
        os.chmod(self.file5_path, 0o777)

        self.file6_path = os.path.join(self.cgi_dir, 'file6.py')
        with open(self.file6_path, 'w', encoding='utf-8') as file6:
            file6.write(cgi_file6 % self.pythonexe)
        os.chmod(self.file6_path, 0o777)

        os.chdir(self.parent_dir)

    def tearDown(self):
        try:
            os.chdir(self.cwd)
            if self._pythonexe_symlink:
                self._pythonexe_symlink.__exit__(None, None, None)
            if self.nocgi_path:
                os.remove(self.nocgi_path)
            if self.file1_path:
                os.remove(self.file1_path)
            if self.file2_path:
                os.remove(self.file2_path)
            if self.file3_path:
                os.remove(self.file3_path)
            if self.file4_path:
                os.remove(self.file4_path)
            if self.file5_path:
                os.remove(self.file5_path)
            if self.file6_path:
                os.remove(self.file6_path)
            os.rmdir(self.cgi_child_dir)
            os.rmdir(self.cgi_dir)
            os.rmdir(self.cgi_dir_in_sub_dir)
            os.rmdir(self.sub_dir_2)
            os.rmdir(self.sub_dir_1)
            os.rmdir(self.parent_dir)
        finally:
            BaseTestCase.tearDown(self)

    def test_url_collapse_path(self):
        # verify tail is the last portion and head is the rest on proper urls
        test_vectors = {
            '': '//',
            '..': IndexError,
            '/.//..': IndexError,
            '/': '//',
            '//': '//',
            '/\\': '//\\',
            '/.//': '//',
            'cgi-bin/file1.py': '/cgi-bin/file1.py',
            '/cgi-bin/file1.py': '/cgi-bin/file1.py',
            'a': '//a',
            '/a': '//a',
            '//a': '//a',
            './a': '//a',
            './C:/': '/C:/',
            '/a/b': '/a/b',
            '/a/b/': '/a/b/',
            '/a/b/.': '/a/b/',
            '/a/b/c/..': '/a/b/',
            '/a/b/c/../d': '/a/b/d',
            '/a/b/c/../d/e/../f': '/a/b/d/f',
            '/a/b/c/../d/e/../../f': '/a/b/f',
            '/a/b/c/../d/e/.././././..//f': '/a/b/f',
            '../a/b/c/../d/e/.././././..//f': IndexError,
            '/a/b/c/../d/e/../../../f': '/a/f',
            '/a/b/c/../d/e/../../../../f': '//f',
            '/a/b/c/../d/e/../../../../../f': IndexError,
            '/a/b/c/../d/e/../../../../f/..': '//',
            '/a/b/c/../d/e/../../../../f/../.': '//',
        }
        for path, expected in test_vectors.items():
            if isinstance(expected, type) and issubclass(expected, Exception):
                self.assertRaises(expected,
                                  server._url_collapse_path, path)
            else:
                actual = server._url_collapse_path(path)
                self.assertEqual(expected, actual,
                                 msg='path = %r\nGot:    %r\nWanted: %r' %
                                 (path, actual, expected))

    def test_headers_and_content(self):
        res = self.request('/cgi-bin/file1.py')
        self.assertEqual(
            (res.read(), res.getheader('Content-type'), res.status),
            (b'Hello World' + self.linesep, 'text/html', HTTPStatus.OK))

    def test_issue19435(self):
        res = self.request('///////////nocgi.py/../cgi-bin/nothere.sh')
        self.assertEqual(res.status, HTTPStatus.NOT_FOUND)

    def test_post(self):
        params = urllib.parse.urlencode(
            {'spam' : 1, 'eggs' : 'python', 'bacon' : 123456})
        headers = {'Content-type' : 'application/x-www-form-urlencoded'}
        res = self.request('/cgi-bin/file2.py', 'POST', params, headers)

        self.assertEqual(res.read(), b'1, python, 123456' + self.linesep)

    def test_invaliduri(self):
        res = self.request('/cgi-bin/invalid')
        res.read()
        self.assertEqual(res.status, HTTPStatus.NOT_FOUND)

    def test_authorization(self):
        headers = {b'Authorization' : b'Basic ' +
                   base64.b64encode(b'username:pass')}
        res = self.request('/cgi-bin/file1.py', 'GET', headers=headers)
        self.assertEqual(
            (b'Hello World' + self.linesep, 'text/html', HTTPStatus.OK),
            (res.read(), res.getheader('Content-type'), res.status))

    def test_no_leading_slash(self):
        # http://bugs.python.org/issue2254
        res = self.request('cgi-bin/file1.py')
        self.assertEqual(
            (b'Hello World' + self.linesep, 'text/html', HTTPStatus.OK),
            (res.read(), res.getheader('Content-type'), res.status))

    def test_os_environ_is_not_altered(self):
        signature = "Test CGI Server"
        os.environ['SERVER_SOFTWARE'] = signature
        res = self.request('/cgi-bin/file1.py')
        self.assertEqual(
            (b'Hello World' + self.linesep, 'text/html', HTTPStatus.OK),
            (res.read(), res.getheader('Content-type'), res.status))
        self.assertEqual(os.environ['SERVER_SOFTWARE'], signature)

    def test_urlquote_decoding_in_cgi_check(self):
        res = self.request('/cgi-bin%2ffile1.py')
        self.assertEqual(
            (b'Hello World' + self.linesep, 'text/html', HTTPStatus.OK),
            (res.read(), res.getheader('Content-type'), res.status))

    def test_nested_cgi_path_issue21323(self):
        res = self.request('/cgi-bin/child-dir/file3.py')
        self.assertEqual(
            (b'Hello World' + self.linesep, 'text/html', HTTPStatus.OK),
            (res.read(), res.getheader('Content-type'), res.status))

    def test_query_with_multiple_question_mark(self):
        res = self.request('/cgi-bin/file4.py?a=b?c=d')
        self.assertEqual(
            (b'a=b?c=d' + self.linesep, 'text/html', HTTPStatus.OK),
            (res.read(), res.getheader('Content-type'), res.status))

    def test_query_with_continuous_slashes(self):
        res = self.request('/cgi-bin/file4.py?k=aa%2F%2Fbb&//q//p//=//a//b//')
        self.assertEqual(
            (b'k=aa%2F%2Fbb&//q//p//=//a//b//' + self.linesep,
             'text/html', HTTPStatus.OK),
            (res.read(), res.getheader('Content-type'), res.status))

    def test_cgi_path_in_sub_directories(self):
        try:
            CGIHTTPRequestHandler.cgi_directories.append('/sub/dir/cgi-bin')
            res = self.request('/sub/dir/cgi-bin/file5.py')
            self.assertEqual(
                (b'Hello World' + self.linesep, 'text/html', HTTPStatus.OK),
                (res.read(), res.getheader('Content-type'), res.status))
        finally:
            CGIHTTPRequestHandler.cgi_directories.remove('/sub/dir/cgi-bin')

    def test_accept(self):
        browser_accept = \
                    'text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8'
        tests = (
            ((('Accept', browser_accept),), browser_accept),
            ((), ''),
            # Hack case to get two values for the one header
            ((('Accept', 'text/html'), ('ACCEPT', 'text/plain')),
               'text/html,text/plain'),
        )
        for headers, expected in tests:
            headers = OrderedDict(headers)
            with self.subTest(headers):
                res = self.request('/cgi-bin/file6.py', 'GET', headers=headers)
                self.assertEqual(http.HTTPStatus.OK, res.status)
                expected = f"HTTP_ACCEPT={expected}".encode('ascii')
                self.assertIn(expected, res.read())


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
        self.assertTrue(result[0].startswith(b'HTTP/1.1 400 '))
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
        self.assertEqual(result[0], b'HTTP/1.1 414 Request-URI Too Long\r\n')
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


def setUpModule():
    unittest.addModuleCleanup(os.chdir, os.getcwd())


if __name__ == '__main__':
    unittest.main()
