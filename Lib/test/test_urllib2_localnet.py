#!/usr/bin/env python

import mimetools
import threading
import urlparse
import urllib2
import BaseHTTPServer
import unittest
import hashlib
from test import test_support

# Loopback http server infrastructure

class LoopbackHttpServer(BaseHTTPServer.HTTPServer):
    """HTTP server w/ a few modifications that make it useful for
    loopback testing purposes.
    """

    def __init__(self, server_address, RequestHandlerClass):
        BaseHTTPServer.HTTPServer.__init__(self,
                                           server_address,
                                           RequestHandlerClass)

        # Set the timeout of our listening socket really low so
        # that we can stop the server easily.
        self.socket.settimeout(1.0)

    def get_request(self):
        """BaseHTTPServer method, overridden."""

        request, client_address = self.socket.accept()

        # It's a loopback connection, so setting the timeout
        # really low shouldn't affect anything, but should make
        # deadlocks less likely to occur.
        request.settimeout(10.0)

        return (request, client_address)

class LoopbackHttpServerThread(threading.Thread):
    """Stoppable thread that runs a loopback http server."""

    def __init__(self, request_handler):
        threading.Thread.__init__(self)
        self._stop = False
        self.ready = threading.Event()
        request_handler.protocol_version = "HTTP/1.0"
        self.httpd = LoopbackHttpServer(('127.0.0.1', 0),
                                        request_handler)
        #print "Serving HTTP on %s port %s" % (self.httpd.server_name,
        #                                      self.httpd.server_port)
        self.port = self.httpd.server_port

    def stop(self):
        """Stops the webserver if it's currently running."""

        # Set the stop flag.
        self._stop = True

        self.join()

    def run(self):
        self.ready.set()
        while not self._stop:
            self.httpd.handle_request()

# Authentication infrastructure

class DigestAuthHandler:
    """Handler for performing digest authentication."""

    def __init__(self):
        self._request_num = 0
        self._nonces = []
        self._users = {}
        self._realm_name = "Test Realm"
        self._qop = "auth"

    def set_qop(self, qop):
        self._qop = qop

    def set_users(self, users):
        assert isinstance(users, dict)
        self._users = users

    def set_realm(self, realm):
        self._realm_name = realm

    def _generate_nonce(self):
        self._request_num += 1
        nonce = hashlib.md5(str(self._request_num)).hexdigest()
        self._nonces.append(nonce)
        return nonce

    def _create_auth_dict(self, auth_str):
        first_space_index = auth_str.find(" ")
        auth_str = auth_str[first_space_index+1:]

        parts = auth_str.split(",")

        auth_dict = {}
        for part in parts:
            name, value = part.split("=")
            name = name.strip()
            if value[0] == '"' and value[-1] == '"':
                value = value[1:-1]
            else:
                value = value.strip()
            auth_dict[name] = value
        return auth_dict

    def _validate_auth(self, auth_dict, password, method, uri):
        final_dict = {}
        final_dict.update(auth_dict)
        final_dict["password"] = password
        final_dict["method"] = method
        final_dict["uri"] = uri
        HA1_str = "%(username)s:%(realm)s:%(password)s" % final_dict
        HA1 = hashlib.md5(HA1_str).hexdigest()
        HA2_str = "%(method)s:%(uri)s" % final_dict
        HA2 = hashlib.md5(HA2_str).hexdigest()
        final_dict["HA1"] = HA1
        final_dict["HA2"] = HA2
        response_str = "%(HA1)s:%(nonce)s:%(nc)s:" \
                       "%(cnonce)s:%(qop)s:%(HA2)s" % final_dict
        response = hashlib.md5(response_str).hexdigest()

        return response == auth_dict["response"]

    def _return_auth_challenge(self, request_handler):
        request_handler.send_response(407, "Proxy Authentication Required")
        request_handler.send_header("Content-Type", "text/html")
        request_handler.send_header(
            'Proxy-Authenticate', 'Digest realm="%s", '
            'qop="%s",'
            'nonce="%s", ' % \
            (self._realm_name, self._qop, self._generate_nonce()))
        # XXX: Not sure if we're supposed to add this next header or
        # not.
        #request_handler.send_header('Connection', 'close')
        request_handler.end_headers()
        request_handler.wfile.write("Proxy Authentication Required.")
        return False

    def handle_request(self, request_handler):
        """Performs digest authentication on the given HTTP request
        handler.  Returns True if authentication was successful, False
        otherwise.

        If no users have been set, then digest auth is effectively
        disabled and this method will always return True.
        """

        if len(self._users) == 0:
            return True

        if not request_handler.headers.has_key('Proxy-Authorization'):
            return self._return_auth_challenge(request_handler)
        else:
            auth_dict = self._create_auth_dict(
                request_handler.headers['Proxy-Authorization']
                )
            if self._users.has_key(auth_dict["username"]):
                password = self._users[ auth_dict["username"] ]
            else:
                return self._return_auth_challenge(request_handler)
            if not auth_dict.get("nonce") in self._nonces:
                return self._return_auth_challenge(request_handler)
            else:
                self._nonces.remove(auth_dict["nonce"])

            auth_validated = False

            # MSIE uses short_path in its validation, but Python's
            # urllib2 uses the full path, so we're going to see if
            # either of them works here.

            for path in [request_handler.path, request_handler.short_path]:
                if self._validate_auth(auth_dict,
                                       password,
                                       request_handler.command,
                                       path):
                    auth_validated = True

            if not auth_validated:
                return self._return_auth_challenge(request_handler)
            return True

# Proxy test infrastructure

class FakeProxyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    """This is a 'fake proxy' that makes it look like the entire
    internet has gone down due to a sudden zombie invasion.  It main
    utility is in providing us with authentication support for
    testing.
    """

    digest_auth_handler = DigestAuthHandler()

    def log_message(self, format, *args):
        # Uncomment the next line for debugging.
        #sys.stderr.write(format % args)
        pass

    def do_GET(self):
        (scm, netloc, path, params, query, fragment) = urlparse.urlparse(
            self.path, 'http')
        self.short_path = path
        if self.digest_auth_handler.handle_request(self):
            self.send_response(200, "OK")
            self.send_header("Content-Type", "text/html")
            self.end_headers()
            self.wfile.write("You've reached %s!<BR>" % self.path)
            self.wfile.write("Our apologies, but our server is down due to "
                              "a sudden zombie invasion.")

# Test cases

class ProxyAuthTests(unittest.TestCase):
    URL = "http://localhost"

    USER = "tester"
    PASSWD = "test123"
    REALM = "TestRealm"

    def setUp(self):
        FakeProxyHandler.digest_auth_handler.set_users({
            self.USER : self.PASSWD
            })
        FakeProxyHandler.digest_auth_handler.set_realm(self.REALM)

        self.server = LoopbackHttpServerThread(FakeProxyHandler)
        self.server.start()
        self.server.ready.wait()
        proxy_url = "http://127.0.0.1:%d" % self.server.port
        handler = urllib2.ProxyHandler({"http" : proxy_url})
        self._digest_auth_handler = urllib2.ProxyDigestAuthHandler()
        self.opener = urllib2.build_opener(handler, self._digest_auth_handler)

    def tearDown(self):
        self.server.stop()

    def test_proxy_with_bad_password_raises_httperror(self):
        self._digest_auth_handler.add_password(self.REALM, self.URL,
                                               self.USER, self.PASSWD+"bad")
        FakeProxyHandler.digest_auth_handler.set_qop("auth")
        self.assertRaises(urllib2.HTTPError,
                          self.opener.open,
                          self.URL)

    def test_proxy_with_no_password_raises_httperror(self):
        FakeProxyHandler.digest_auth_handler.set_qop("auth")
        self.assertRaises(urllib2.HTTPError,
                          self.opener.open,
                          self.URL)

    def test_proxy_qop_auth_works(self):
        self._digest_auth_handler.add_password(self.REALM, self.URL,
                                               self.USER, self.PASSWD)
        FakeProxyHandler.digest_auth_handler.set_qop("auth")
        result = self.opener.open(self.URL)
        while result.read():
            pass
        result.close()

    def test_proxy_qop_auth_int_works_or_throws_urlerror(self):
        self._digest_auth_handler.add_password(self.REALM, self.URL,
                                               self.USER, self.PASSWD)
        FakeProxyHandler.digest_auth_handler.set_qop("auth-int")
        try:
            result = self.opener.open(self.URL)
        except urllib2.URLError:
            # It's okay if we don't support auth-int, but we certainly
            # shouldn't receive any kind of exception here other than
            # a URLError.
            result = None
        if result:
            while result.read():
                pass
            result.close()


def GetRequestHandler(responses):

    class FakeHTTPRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):

        server_version = "TestHTTP/"
        requests = []
        headers_received = []
        port = 80

        def do_GET(self):
            body = self.send_head()
            if body:
                self.wfile.write(body)

        def do_POST(self):
            content_length = self.headers['Content-Length']
            post_data = self.rfile.read(int(content_length))
            self.do_GET()
            self.requests.append(post_data)

        def send_head(self):
            FakeHTTPRequestHandler.headers_received = self.headers
            self.requests.append(self.path)
            response_code, headers, body = responses.pop(0)

            self.send_response(response_code)

            for (header, value) in headers:
                self.send_header(header, value % self.port)
            if body:
                self.send_header('Content-type', 'text/plain')
                self.end_headers()
                return body
            self.end_headers()

        def log_message(self, *args):
            pass


    return FakeHTTPRequestHandler


class TestUrlopen(unittest.TestCase):
    """Tests urllib2.urlopen using the network.

    These tests are not exhaustive.  Assuming that testing using files does a
    good job overall of some of the basic interface features.  There are no
    tests exercising the optional 'data' and 'proxies' arguments.  No tests
    for transparent redirection have been written.
    """

    def start_server(self, responses):
        handler = GetRequestHandler(responses)

        self.server = LoopbackHttpServerThread(handler)
        self.server.start()
        self.server.ready.wait()
        port = self.server.port
        handler.port = port
        return handler


    def test_redirection(self):
        expected_response = 'We got here...'
        responses = [
            (302, [('Location', 'http://localhost:%s/somewhere_else')], ''),
            (200, [], expected_response)
        ]

        handler = self.start_server(responses)

        try:
            f = urllib2.urlopen('http://localhost:%s/' % handler.port)
            data = f.read()
            f.close()

            self.assertEquals(data, expected_response)
            self.assertEquals(handler.requests, ['/', '/somewhere_else'])
        finally:
            self.server.stop()


    def test_404(self):
        expected_response = 'Bad bad bad...'
        handler = self.start_server([(404, [], expected_response)])

        try:
            try:
                urllib2.urlopen('http://localhost:%s/weeble' % handler.port)
            except urllib2.URLError, f:
                pass
            else:
                self.fail('404 should raise URLError')

            data = f.read()
            f.close()

            self.assertEquals(data, expected_response)
            self.assertEquals(handler.requests, ['/weeble'])
        finally:
            self.server.stop()


    def test_200(self):
        expected_response = 'pycon 2008...'
        handler = self.start_server([(200, [], expected_response)])

        try:
            f = urllib2.urlopen('http://localhost:%s/bizarre' % handler.port)
            data = f.read()
            f.close()

            self.assertEquals(data, expected_response)
            self.assertEquals(handler.requests, ['/bizarre'])
        finally:
            self.server.stop()

    def test_200_with_parameters(self):
        expected_response = 'pycon 2008...'
        handler = self.start_server([(200, [], expected_response)])

        try:
            f = urllib2.urlopen('http://localhost:%s/bizarre' % handler.port, 'get=with_feeling')
            data = f.read()
            f.close()

            self.assertEquals(data, expected_response)
            self.assertEquals(handler.requests, ['/bizarre', 'get=with_feeling'])
        finally:
            self.server.stop()


    def test_sending_headers(self):
        handler = self.start_server([(200, [], "we don't care")])

        try:
            req = urllib2.Request("http://localhost:%s/" % handler.port,
                                  headers={'Range': 'bytes=20-39'})
            urllib2.urlopen(req)
            self.assertEqual(handler.headers_received['Range'], 'bytes=20-39')
        finally:
            self.server.stop()

    def test_basic(self):
        handler = self.start_server([(200, [], "we don't care")])

        try:
            open_url = urllib2.urlopen("http://localhost:%s" % handler.port)
            for attr in ("read", "close", "info", "geturl"):
                self.assert_(hasattr(open_url, attr), "object returned from "
                             "urlopen lacks the %s attribute" % attr)
            try:
                self.assert_(open_url.read(), "calling 'read' failed")
            finally:
                open_url.close()
        finally:
            self.server.stop()

    def test_info(self):
        handler = self.start_server([(200, [], "we don't care")])

        try:
            open_url = urllib2.urlopen("http://localhost:%s" % handler.port)
            info_obj = open_url.info()
            self.assert_(isinstance(info_obj, mimetools.Message),
                         "object returned by 'info' is not an instance of "
                         "mimetools.Message")
            self.assertEqual(info_obj.getsubtype(), "plain")
        finally:
            self.server.stop()

    def test_geturl(self):
        # Make sure same URL as opened is returned by geturl.
        handler = self.start_server([(200, [], "we don't care")])

        try:
            open_url = urllib2.urlopen("http://localhost:%s" % handler.port)
            url = open_url.geturl()
            self.assertEqual(url, "http://localhost:%s" % handler.port)
        finally:
            self.server.stop()


    def test_bad_address(self):
        # Make sure proper exception is raised when connecting to a bogus
        # address.
        self.assertRaises(IOError,
                          # SF patch 809915:  In Sep 2003, VeriSign started
                          # highjacking invalid .com and .net addresses to
                          # boost traffic to their own site.  This test
                          # started failing then.  One hopes the .invalid
                          # domain will be spared to serve its defined
                          # purpose.
                          # urllib2.urlopen, "http://www.sadflkjsasadf.com/")
                          urllib2.urlopen, "http://www.python.invalid./")


def test_main():
    # We will NOT depend on the network resource flag
    # (Lib/test/regrtest.py -u network) since all tests here are only
    # localhost.  However, if this is a bad rationale, then uncomment
    # the next line.
    #test_support.requires("network")

    test_support.run_unittest(ProxyAuthTests)
    test_support.run_unittest(TestUrlopen)

if __name__ == "__main__":
    test_main()
