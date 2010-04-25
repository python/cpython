"""Unittests for the various HTTPServer modules.

Written by Cody A.W. Somerville <cody-somerville@ubuntu.com>,
Josip Dzolonga, and Michael Otteneder for the 2007/08 GHOP contest.
"""

from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from SimpleHTTPServer import SimpleHTTPRequestHandler
from CGIHTTPServer import CGIHTTPRequestHandler

import os
import sys
import base64
import shutil
import urllib
import httplib
import tempfile
import threading

import unittest
from test import test_support


class NoLogRequestHandler:
    def log_message(self, *args):
        # don't write log messages to stderr
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
        self.server_started = threading.Event()
        self.thread = TestServerThread(self, self.request_handler)
        self.thread.start()
        self.server_started.wait()

    def tearDown(self):
        self.thread.stop()

    def request(self, uri, method='GET', body=None, headers={}):
        self.connection = httplib.HTTPConnection('localhost', self.PORT)
        self.connection.request(method, uri, body, headers)
        return self.connection.getresponse()


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

    def setUp(self):
        BaseTestCase.setUp(self)
        self.con = httplib.HTTPConnection('localhost', self.PORT)
        self.con.connect()

    def test_command(self):
        self.con.request('GET', '/')
        res = self.con.getresponse()
        self.assertEquals(res.status, 501)

    def test_request_line_trimming(self):
        self.con._http_vsn_str = 'HTTP/1.1\n'
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEquals(res.status, 501)

    def test_version_bogus(self):
        self.con._http_vsn_str = 'FUBAR'
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEquals(res.status, 400)

    def test_version_digits(self):
        self.con._http_vsn_str = 'HTTP/9.9.9'
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEquals(res.status, 400)

    def test_version_none_get(self):
        self.con._http_vsn_str = ''
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEquals(res.status, 501)

    def test_version_none(self):
        self.con._http_vsn_str = ''
        self.con.putrequest('PUT', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEquals(res.status, 400)

    def test_version_invalid(self):
        self.con._http_vsn = 99
        self.con._http_vsn_str = 'HTTP/9.9'
        self.con.putrequest('GET', '/')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEquals(res.status, 505)

    def test_send_blank(self):
        self.con._http_vsn_str = ''
        self.con.putrequest('', '')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEquals(res.status, 400)

    def test_header_close(self):
        self.con.putrequest('GET', '/')
        self.con.putheader('Connection', 'close')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEquals(res.status, 501)

    def test_head_keep_alive(self):
        self.con._http_vsn_str = 'HTTP/1.1'
        self.con.putrequest('GET', '/')
        self.con.putheader('Connection', 'keep-alive')
        self.con.endheaders()
        res = self.con.getresponse()
        self.assertEquals(res.status, 501)

    def test_handler(self):
        self.con.request('TEST', '/')
        res = self.con.getresponse()
        self.assertEquals(res.status, 204)

    def test_return_header_keep_alive(self):
        self.con.request('KEEP', '/')
        res = self.con.getresponse()
        self.assertEquals(res.getheader('Connection'), 'keep-alive')
        self.con.request('TEST', '/')

    def test_internal_key_error(self):
        self.con.request('KEYERROR', '/')
        res = self.con.getresponse()
        self.assertEquals(res.status, 999)

    def test_return_custom_status(self):
        self.con.request('CUSTOM', '/')
        res = self.con.getresponse()
        self.assertEquals(res.status, 999)


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
        temp = open(os.path.join(self.tempdir, 'test'), 'wb')
        temp.write(self.data)
        temp.close()

    def tearDown(self):
        try:
            os.chdir(self.cwd)
            try:
                shutil.rmtree(self.tempdir)
            except:
                pass
        finally:
            BaseTestCase.tearDown(self)

    def check_status_and_reason(self, response, status, data=None):
        body = response.read()
        self.assert_(response)
        self.assertEquals(response.status, status)
        self.assert_(response.reason != None)
        if data:
            self.assertEqual(data, body)

    def test_get(self):
        #constructs the path relative to the root directory of the HTTPServer
        response = self.request(self.tempdir_name + '/test')
        self.check_status_and_reason(response, 200, data=self.data)
        response = self.request(self.tempdir_name + '/')
        self.check_status_and_reason(response, 200)
        response = self.request(self.tempdir_name)
        self.check_status_and_reason(response, 301)
        response = self.request('/ThisDoesNotExist')
        self.check_status_and_reason(response, 404)
        response = self.request('/' + 'ThisDoesNotExist' + '/')
        self.check_status_and_reason(response, 404)
        f = open(os.path.join(self.tempdir_name, 'index.html'), 'w')
        response = self.request('/' + self.tempdir_name + '/')
        self.check_status_and_reason(response, 200)
        if os.name == 'posix':
            # chmod won't work as expected on Windows platforms
            os.chmod(self.tempdir, 0)
            response = self.request(self.tempdir_name + '/')
            self.check_status_and_reason(response, 404)
            os.chmod(self.tempdir, 0755)

    def test_head(self):
        response = self.request(
            self.tempdir_name + '/test', method='HEAD')
        self.check_status_and_reason(response, 200)
        self.assertEqual(response.getheader('content-length'),
                         str(len(self.data)))
        self.assertEqual(response.getheader('content-type'),
                         'application/octet-stream')

    def test_invalid_requests(self):
        response = self.request('/', method='FOO')
        self.check_status_and_reason(response, 501)
        # requests must be case sensitive,so this should fail too
        response = self.request('/', method='get')
        self.check_status_and_reason(response, 501)
        response = self.request('/', method='GETs')
        self.check_status_and_reason(response, 501)


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
print "%%s, %%s, %%s" %% (form.getfirst("spam"), form.getfirst("eggs"),\
              form.getfirst("bacon"))
"""

class CGIHTTPServerTestCase(BaseTestCase):
    class request_handler(NoLogRequestHandler, CGIHTTPRequestHandler):
        pass

    def setUp(self):
        BaseTestCase.setUp(self)
        self.parent_dir = tempfile.mkdtemp()
        self.cgi_dir = os.path.join(self.parent_dir, 'cgi-bin')
        os.mkdir(self.cgi_dir)

        # The shebang line should be pure ASCII: use symlink if possible.
        # See issue #7668.
        if hasattr(os, 'symlink'):
            self.pythonexe = os.path.join(self.parent_dir, 'python')
            os.symlink(sys.executable, self.pythonexe)
        else:
            self.pythonexe = sys.executable

        self.file1_path = os.path.join(self.cgi_dir, 'file1.py')
        with open(self.file1_path, 'w') as file1:
            file1.write(cgi_file1 % self.pythonexe)
        os.chmod(self.file1_path, 0777)

        self.file2_path = os.path.join(self.cgi_dir, 'file2.py')
        with open(self.file2_path, 'w') as file2:
            file2.write(cgi_file2 % self.pythonexe)
        os.chmod(self.file2_path, 0777)

        self.cwd = os.getcwd()
        os.chdir(self.parent_dir)

    def tearDown(self):
        try:
            os.chdir(self.cwd)
            if self.pythonexe != sys.executable:
                os.remove(self.pythonexe)
            os.remove(self.file1_path)
            os.remove(self.file2_path)
            os.rmdir(self.cgi_dir)
            os.rmdir(self.parent_dir)
        finally:
            BaseTestCase.tearDown(self)

    def test_headers_and_content(self):
        res = self.request('/cgi-bin/file1.py')
        self.assertEquals(('Hello World\n', 'text/html', 200), \
             (res.read(), res.getheader('Content-type'), res.status))

    def test_post(self):
        params = urllib.urlencode({'spam' : 1, 'eggs' : 'python', 'bacon' : 123456})
        headers = {'Content-type' : 'application/x-www-form-urlencoded'}
        res = self.request('/cgi-bin/file2.py', 'POST', params, headers)

        self.assertEquals(res.read(), '1, python, 123456\n')

    def test_invaliduri(self):
        res = self.request('/cgi-bin/invalid')
        res.read()
        self.assertEquals(res.status, 404)

    def test_authorization(self):
        headers = {'Authorization' : 'Basic %s' % \
                base64.b64encode('username:pass')}
        res = self.request('/cgi-bin/file1.py', 'GET', headers=headers)
        self.assertEquals(('Hello World\n', 'text/html', 200), \
             (res.read(), res.getheader('Content-type'), res.status))


def test_main(verbose=None):
    cwd = os.getcwd()
    env = os.environ.copy()
    try:
        test_support.run_unittest(BaseHTTPServerTestCase,
                                  SimpleHTTPServerTestCase,
                                  CGIHTTPServerTestCase
                                  )
    finally:
        test_support.reap_children()
        os.environ.clear()
        os.environ.update(env)
        os.chdir(cwd)

if __name__ == '__main__':
    test_main()
