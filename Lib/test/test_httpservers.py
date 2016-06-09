"""Unittests for the various HTTPServer modules.

Written by Cody A.W. Somerville <cody-somerville@ubuntu.com>,
Josip Dzolonga, and Michael Otteneder for the 2007/08 GHOP contest.
"""

import os
import sys
import re
import base64
import ntpath
import shutil
import urllib
import httplib
import tempfile
import unittest
import CGIHTTPServer


from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler
from CGIHTTPServer import CGIHTTPRequestHandler
from StringIO import StringIO
from test import test_support


threading = test_support.import_module('threading')


class NoLogRequestHandler:
    def log_message(self, *args):
        # don't write log messages to stderr
        pass

class SocketlessRequestHandler(SimpleHTTPRequestHandler):
    def __init__(self):
        self.get_called = False
        self.protocol_version = "HTTP/1.1"

    def do_GET(self):
        self.get_called = True
        self.send_response(200)
        self.send_header('Content-Type', 'text/html')
        self.end_headers()
        self.wfile.write(b'<html><body>Data</body></html>\r\n')

    def log_message(self, fmt, *args):
        pass


class TestServerThread(threading.Thread):
    def __init__(self, test_object, request_handler):
        threading.Thread.__init__(self)
        self.request_handler = request_handler
        self.test_object = test_object

    def run(self):
        self.server = HTTPServer(('', 0), self.request_handler)
        self.test_object.PORT = self.server.socket.getsockname()[1]
        self.test_object.server_started.set()
        self.test_object = None
        try:
            self.server.serve_forever(0.05)
        finally:
            self.server.server_close()

    def stop(self):
        self.server.shutdown()


class BaseTestCase(unittest.TestCase):
    def setUp(self):
        self._threads = test_support.threading_setup()
        os.environ = test_support.EnvironmentVarGuard()
        self.server_started = threading.Event()
        self.thread = TestServerThread(self, self.request_handler)
        self.thread.start()
        self.server_started.wait()

    def tearDown(self):
        self.thread.stop()
        os.environ.__exit__()
        test_support.threading_cleanup(*self._threads)

    def request(self, uri, method='GET', body=None, headers={}):
        self.connection = httplib.HTTPConnection('localhost', self.PORT)
        self.connection.request(method, uri, body, headers)
        return self.connection.getresponse()

class BaseHTTPRequestHandlerTestCase(unittest.TestCase):
    """Test the functionality of the BaseHTTPServer focussing on
    BaseHTTPRequestHandler.
    """

    HTTPResponseMatch = re.compile('HTTP/1.[0-9]+ 200 OK')

    def setUp (self):
        self.handler = SocketlessRequestHandler()

    def send_typical_request(self, message):
        input_msg = StringIO(message)
        output = StringIO()
        self.handler.rfile = input_msg
        self.handler.wfile = output
        self.handler.handle_one_request()
        output.seek(0)
        return output.readlines()

    def verify_get_called(self):
        self.assertTrue(self.handler.get_called)

    def verify_expected_headers(self, headers):
        for fieldName in 'Server: ', 'Date: ', 'Content-Type: ':
            self.assertEqual(sum(h.startswith(fieldName) for h in headers), 1)

    def verify_http_server_response(self, response):
        match = self.HTTPResponseMatch.search(response)
        self.assertIsNotNone(match)

    def test_http_1_1(self):
        result = self.send_typical_request('GET / HTTP/1.1\r\n\r\n')
        self.verify_http_server_response(result[0])
        self.verify_expected_headers(result[1:-1])
        self.verify_get_called()
        self.assertEqual(result[-1], '<html><body>Data</body></html>\r\n')

    def test_http_1_0(self):
        result = self.send_typical_request('GET / HTTP/1.0\r\n\r\n')
        self.verify_http_server_response(result[0])
        self.verify_expected_headers(result[1:-1])
        self.verify_get_called()
        self.assertEqual(result[-1], '<html><body>Data</body></html>\r\n')

    def test_http_0_9(self):
        result = self.send_typical_request('GET / HTTP/0.9\r\n\r\n')
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0], '<html><body>Data</body></html>\r\n')
        self.verify_get_called()

    def test_with_continue_1_0(self):
        result = self.send_typical_request('GET / HTTP/1.0\r\nExpect: 100-continue\r\n\r\n')
        self.verify_http_server_response(result[0])
        self.verify_expected_headers(result[1:-1])
        self.verify_get_called()
        self.assertEqual(result[-1], '<html><body>Data</body></html>\r\n')

    def test_request_length(self):
        # Issue #10714: huge request lines are discarded, to avoid Denial
        # of Service attacks.
        result = self.send_typical_request(b'GET ' + b'x' * 65537)
        self.assertEqual(result[0], b'HTTP/1.1 414 Request-URI Too Long\r\n')
        self.assertFalse(self.handler.get_called)


class BaseHTTPServerTestCase(BaseTestCase):
    class request_handler(NoLogRequestHandler, BaseHTTPRequestHandler):
        protocol_version = 'HTTP/1.1'
        default_request_version = 'HTTP/1.1'

        def do_TEST(self):
            self.send_response(204)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Connection', 'close')
            self.end_headers()

        def do_KEEP(self):
            self.send_response(204)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Connection', 'keep-alive')
            self.end_headers()

        def do_KEYERROR(self):
            self.send_error(999)

        def do_CUSTOM(self):
            self.send_response(999)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Connection', 'close')
            self.end_headers()

        def do_SEND_ERROR(self):
            self.send_error(int(self.path[1:]))

        def do_HEAD(self):
            self.send_error(int(self.path[1:]))

    def setUp(self):
        BaseTestCase.setUp(self)
        self.con = httplib.HTTPConnection('localhost', self.PORT)
        self.con.connect()

    def test_command(self):
        self.con.request('GET', '/')
        res = self.con.getresponse()
        self.assertEqual(res.status, 501)

    def test_request_line_trimming(self):
        self.con._http_vsn_str = 'HTTP/1.1\n'
        self.con.putrequest('XYZBOGUS', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, 501)

    def test_version_bogus(self):
        self.con._http_vsn_str = 'FUBAR'
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, 400)

    def test_version_digits(self):
        self.con._http_vsn_str = 'HTTP/9.9.9'
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, 400)

    def test_version_none_get(self):
        self.con._http_vsn_str = ''
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, 501)

    def test_version_none(self):
        # Test that a valid method is rejected when not HTTP/1.x
        self.con._http_vsn_str = ''
        self.con.putrequest('CUSTOM', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, 400)

    def test_version_invalid(self):
        self.con._http_vsn = 99
        self.con._http_vsn_str = 'HTTP/9.9'
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, 505)

    def test_send_blank(self):
        self.con._http_vsn_str = ''
        self.con.putrequest('', '')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, 400)

    def test_header_close(self):
        self.con.putrequest('GET', '/')
        self.con.putheader('Connection', 'close')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, 501)

    def test_head_keep_alive(self):
        self.con._http_vsn_str = 'HTTP/1.1'
        self.con.putrequest('GET', '/')
        self.con.putheader('Connection', 'keep-alive')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEqual(res.status, 501)

    def test_handler(self):
        self.con.request('TEST', '/')
        res = self.con.getresponse()
        self.assertEqual(res.status, 204)

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

    def test_send_error(self):
        allow_transfer_encoding_codes = (205, 304)
        for code in (101, 102, 204, 205, 304):
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
        allow_transfer_encoding_codes = (205, 304)
        for code in (101, 200, 204, 205, 304):
            self.con.request('HEAD', '/{}'.format(code))
            res = self.con.getresponse()
            self.assertEqual(code, res.status)
            if code == 200:
                self.assertEqual(None, res.getheader('Content-Length'))
                self.assertIn('text/html', res.getheader('Content-Type'))
            else:
                self.assertEqual(None, res.getheader('Content-Length'))
                self.assertEqual(None, res.getheader('Content-Type'))
            if code not in allow_transfer_encoding_codes:
                self.assertEqual(None, res.getheader('Transfer-Encoding'))

            data = res.read()
            self.assertEqual(b'', data)


class SimpleHTTPServerTestCase(BaseTestCase):
    class request_handler(NoLogRequestHandler, SimpleHTTPRequestHandler):
        pass

    def setUp(self):
        BaseTestCase.setUp(self)
        self.cwd = os.getcwd()
        basetempdir = tempfile.gettempdir()
        os.chdir(basetempdir)
        self.data = 'We are the knights who say Ni!'
        self.tempdir = tempfile.mkdtemp(dir=basetempdir)
        self.tempdir_name = os.path.basename(self.tempdir)
        self.base_url = '/' + self.tempdir_name
        temp = open(os.path.join(self.tempdir, 'test'), 'wb')
        temp.write(self.data)
        temp.close()

    def tearDown(self):
        try:
            os.chdir(self.cwd)
            try:
                shutil.rmtree(self.tempdir)
            except OSError:
                pass
        finally:
            BaseTestCase.tearDown(self)

    def check_status_and_reason(self, response, status, data=None):
        body = response.read()
        self.assertTrue(response)
        self.assertEqual(response.status, status)
        self.assertIsNotNone(response.reason)
        if data:
            self.assertEqual(data, body)

    def test_get(self):
        #constructs the path relative to the root directory of the HTTPServer
        response = self.request(self.base_url + '/test')
        self.check_status_and_reason(response, 200, data=self.data)
        # check for trailing "/" which should return 404. See Issue17324
        response = self.request(self.base_url + '/test/')
        self.check_status_and_reason(response, 404)
        response = self.request(self.base_url + '/')
        self.check_status_and_reason(response, 200)
        response = self.request(self.base_url)
        self.check_status_and_reason(response, 301)
        response = self.request(self.base_url + '/?hi=2')
        self.check_status_and_reason(response, 200)
        response = self.request(self.base_url + '?hi=1')
        self.check_status_and_reason(response, 301)
        self.assertEqual(response.getheader("Location"),
                         self.base_url + "/?hi=1")
        response = self.request('/ThisDoesNotExist')
        self.check_status_and_reason(response, 404)
        response = self.request('/' + 'ThisDoesNotExist' + '/')
        self.check_status_and_reason(response, 404)
        with open(os.path.join(self.tempdir_name, 'index.html'), 'w') as fp:
            response = self.request(self.base_url + '/')
            self.check_status_and_reason(response, 200)
            # chmod() doesn't work as expected on Windows, and filesystem
            # permissions are ignored by root on Unix.
            if os.name == 'posix' and os.geteuid() != 0:
                os.chmod(self.tempdir, 0)
                response = self.request(self.base_url + '/')
                self.check_status_and_reason(response, 404)
                os.chmod(self.tempdir, 0755)

    def test_head(self):
        response = self.request(
            self.base_url + '/test', method='HEAD')
        self.check_status_and_reason(response, 200)
        self.assertEqual(response.getheader('content-length'),
                         str(len(self.data)))
        self.assertEqual(response.getheader('content-type'),
                         'application/octet-stream')

    def test_invalid_requests(self):
        response = self.request('/', method='FOO')
        self.check_status_and_reason(response, 501)
        # requests must be case sensitive,so this should fail too
        response = self.request('/', method='custom')
        self.check_status_and_reason(response, 501)
        response = self.request('/', method='GETs')
        self.check_status_and_reason(response, 501)

    def test_path_without_leading_slash(self):
        response = self.request(self.tempdir_name + '/test')
        self.check_status_and_reason(response, 200, data=self.data)
        response = self.request(self.tempdir_name + '/test/')
        self.check_status_and_reason(response, 404)
        response = self.request(self.tempdir_name + '/')
        self.check_status_and_reason(response, 200)
        response = self.request(self.tempdir_name)
        self.check_status_and_reason(response, 301)
        response = self.request(self.tempdir_name + '/?hi=2')
        self.check_status_and_reason(response, 200)
        response = self.request(self.tempdir_name + '?hi=1')
        self.check_status_and_reason(response, 301)
        self.assertEqual(response.getheader("Location"),
                         self.tempdir_name + "/?hi=1")


cgi_file1 = """\
#!%s

print "Content-type: text/html"
print
print "Hello World"
"""

cgi_file2 = """\
#!%s
import cgi

print "Content-type: text/html"
print

form = cgi.FieldStorage()
print "%%s, %%s, %%s" %% (form.getfirst("spam"), form.getfirst("eggs"),
                          form.getfirst("bacon"))
"""

cgi_file4 = """\
#!%s
import os

print("Content-type: text/html")
print()

print(os.environ["%s"])
"""


@unittest.skipIf(hasattr(os, 'geteuid') and os.geteuid() == 0,
        "This test can't be run reliably as root (issue #13308).")
class CGIHTTPServerTestCase(BaseTestCase):
    class request_handler(NoLogRequestHandler, CGIHTTPRequestHandler):
        pass

    def setUp(self):
        BaseTestCase.setUp(self)
        self.parent_dir = tempfile.mkdtemp()
        self.cgi_dir = os.path.join(self.parent_dir, 'cgi-bin')
        self.cgi_child_dir = os.path.join(self.cgi_dir, 'child-dir')
        os.mkdir(self.cgi_dir)
        os.mkdir(self.cgi_child_dir)

        # The shebang line should be pure ASCII: use symlink if possible.
        # See issue #7668.
        if hasattr(os, 'symlink'):
            self.pythonexe = os.path.join(self.parent_dir, 'python')
            os.symlink(sys.executable, self.pythonexe)
        else:
            self.pythonexe = sys.executable

        self.nocgi_path = os.path.join(self.parent_dir, 'nocgi.py')
        with open(self.nocgi_path, 'w') as fp:
            fp.write(cgi_file1 % self.pythonexe)
        os.chmod(self.nocgi_path, 0777)

        self.file1_path = os.path.join(self.cgi_dir, 'file1.py')
        with open(self.file1_path, 'w') as file1:
            file1.write(cgi_file1 % self.pythonexe)
        os.chmod(self.file1_path, 0777)

        self.file2_path = os.path.join(self.cgi_dir, 'file2.py')
        with open(self.file2_path, 'w') as file2:
            file2.write(cgi_file2 % self.pythonexe)
        os.chmod(self.file2_path, 0777)

        self.file3_path = os.path.join(self.cgi_child_dir, 'file3.py')
        with open(self.file3_path, 'w') as file3:
            file3.write(cgi_file1 % self.pythonexe)
        os.chmod(self.file3_path, 0777)

        self.file4_path = os.path.join(self.cgi_dir, 'file4.py')
        with open(self.file4_path, 'w') as file4:
            file4.write(cgi_file4 % (self.pythonexe, 'QUERY_STRING'))
        os.chmod(self.file4_path, 0o777)

        self.cwd = os.getcwd()
        os.chdir(self.parent_dir)

    def tearDown(self):
        try:
            os.chdir(self.cwd)
            if self.pythonexe != sys.executable:
                os.remove(self.pythonexe)
            os.remove(self.nocgi_path)
            os.remove(self.file1_path)
            os.remove(self.file2_path)
            os.remove(self.file3_path)
            os.remove(self.file4_path)
            os.rmdir(self.cgi_child_dir)
            os.rmdir(self.cgi_dir)
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
        for path, expected in test_vectors.iteritems():
            if isinstance(expected, type) and issubclass(expected, Exception):
                self.assertRaises(expected,
                                  CGIHTTPServer._url_collapse_path, path)
            else:
                actual = CGIHTTPServer._url_collapse_path(path)
                self.assertEqual(expected, actual,
                                 msg='path = %r\nGot:    %r\nWanted: %r' %
                                 (path, actual, expected))

    def test_headers_and_content(self):
        res = self.request('/cgi-bin/file1.py')
        self.assertEqual(('Hello World\n', 'text/html', 200),
            (res.read(), res.getheader('Content-type'), res.status))

    def test_issue19435(self):
        res = self.request('///////////nocgi.py/../cgi-bin/nothere.sh')
        self.assertEqual(res.status, 404)

    def test_post(self):
        params = urllib.urlencode({'spam' : 1, 'eggs' : 'python', 'bacon' : 123456})
        headers = {'Content-type' : 'application/x-www-form-urlencoded'}
        res = self.request('/cgi-bin/file2.py', 'POST', params, headers)

        self.assertEqual(res.read(), '1, python, 123456\n')

    def test_invaliduri(self):
        res = self.request('/cgi-bin/invalid')
        res.read()
        self.assertEqual(res.status, 404)

    def test_authorization(self):
        headers = {'Authorization' : 'Basic %s' %
                   base64.b64encode('username:pass')}
        res = self.request('/cgi-bin/file1.py', 'GET', headers=headers)
        self.assertEqual(('Hello World\n', 'text/html', 200),
                (res.read(), res.getheader('Content-type'), res.status))

    def test_no_leading_slash(self):
        # http://bugs.python.org/issue2254
        res = self.request('cgi-bin/file1.py')
        self.assertEqual(('Hello World\n', 'text/html', 200),
             (res.read(), res.getheader('Content-type'), res.status))

    def test_os_environ_is_not_altered(self):
        signature = "Test CGI Server"
        os.environ['SERVER_SOFTWARE'] = signature
        res = self.request('/cgi-bin/file1.py')
        self.assertEqual((b'Hello World\n', 'text/html', 200),
                (res.read(), res.getheader('Content-type'), res.status))
        self.assertEqual(os.environ['SERVER_SOFTWARE'], signature)

    def test_urlquote_decoding_in_cgi_check(self):
        res = self.request('/cgi-bin%2ffile1.py')
        self.assertEqual((b'Hello World\n', 'text/html', 200),
                (res.read(), res.getheader('Content-type'), res.status))

    def test_nested_cgi_path_issue21323(self):
        res = self.request('/cgi-bin/child-dir/file3.py')
        self.assertEqual((b'Hello World\n', 'text/html', 200),
                (res.read(), res.getheader('Content-type'), res.status))

    def test_query_with_multiple_question_mark(self):
        res = self.request('/cgi-bin/file4.py?a=b?c=d')
        self.assertEqual(
            (b'a=b?c=d\n', 'text/html', 200),
            (res.read(), res.getheader('Content-type'), res.status))

    def test_query_with_continuous_slashes(self):
        res = self.request('/cgi-bin/file4.py?k=aa%2F%2Fbb&//q//p//=//a//b//')
        self.assertEqual(
            (b'k=aa%2F%2Fbb&//q//p//=//a//b//\n',
             'text/html', 200),
            (res.read(), res.getheader('Content-type'), res.status))


class SimpleHTTPRequestHandlerTestCase(unittest.TestCase):
    """ Test url parsing """
    def setUp(self):
        self.translated = os.getcwd()
        self.translated = os.path.join(self.translated, 'filename')
        self.handler = SocketlessRequestHandler()

    def test_query_arguments(self):
        path = self.handler.translate_path('/filename')
        self.assertEqual(path, self.translated)
        path = self.handler.translate_path('/filename?foo=bar')
        self.assertEqual(path, self.translated)
        path = self.handler.translate_path('/filename?a=b&spam=eggs#zot')
        self.assertEqual(path, self.translated)

    def test_start_with_double_slash(self):
        path = self.handler.translate_path('//filename')
        self.assertEqual(path, self.translated)
        path = self.handler.translate_path('//filename?foo=bar')
        self.assertEqual(path, self.translated)

    def test_windows_colon(self):
        import SimpleHTTPServer
        with test_support.swap_attr(SimpleHTTPServer.os, 'path', ntpath):
            path = self.handler.translate_path('c:c:c:foo/filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated)

            path = self.handler.translate_path('\\c:../filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated)

            path = self.handler.translate_path('c:\\c:..\\foo/filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated)

            path = self.handler.translate_path('c:c:foo\\c:c:bar/filename')
            path = path.replace(ntpath.sep, os.sep)
            self.assertEqual(path, self.translated)


def test_main(verbose=None):
    try:
        cwd = os.getcwd()
        test_support.run_unittest(BaseHTTPRequestHandlerTestCase,
                                  SimpleHTTPRequestHandlerTestCase,
                                  BaseHTTPServerTestCase,
                                  SimpleHTTPServerTestCase,
                                  CGIHTTPServerTestCase
                                 )
    finally:
        os.chdir(cwd)

if __name__ == '__main__':
    test_main()
