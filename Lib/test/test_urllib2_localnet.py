import base64
import os
import email
import urllib.parse
import urllib.request
import http.server
import unittest
import hashlib

from test import support

threading = support.import_module('threading')

try:
    import ssl
except ImportError:
    ssl = None

here = os.path.dirname(__file__)
# Self-signed cert file for 'localhost'
CERT_localhost = os.path.join(here, 'keycert.pem')
# Self-signed cert file for 'fakehostname'
CERT_fakehostname = os.path.join(here, 'keycert2.pem')


# Loopback http server infrastructure

class LoopbackHttpServer(http.server.HTTPServer):
    """HTTP server w/ a few modifications that make it useful for
    loopback testing purposes.
    """

    def __init__(self, server_address, RequestHandlerClass):
        http.server.HTTPServer.__init__(self,
                                        server_address,
                                        RequestHandlerClass)

        # Set the timeout of our listening socket really low so
        # that we can stop the server easily.
        self.socket.settimeout(0.1)

    def get_request(self):
        """HTTPServer method, overridden."""

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
        self._stop_server = False
        self.ready = threading.Event()
        request_handler.protocol_version = "HTTP/1.0"
        self.httpd = LoopbackHttpServer(("127.0.0.1", 0),
                                        request_handler)
        self.port = self.httpd.server_port

    def stop(self):
        """Stops the webserver if it's currently running."""

        self._stop_server = True

        self.join()
        self.httpd.server_close()

    def run(self):
        self.ready.set()
        while not self._stop_server:
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
        nonce = hashlib.md5(str(self._request_num).encode("ascii")).hexdigest()
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
        HA1 = hashlib.md5(HA1_str.encode("ascii")).hexdigest()
        HA2_str = "%(method)s:%(uri)s" % final_dict
        HA2 = hashlib.md5(HA2_str.encode("ascii")).hexdigest()
        final_dict["HA1"] = HA1
        final_dict["HA2"] = HA2
        response_str = "%(HA1)s:%(nonce)s:%(nc)s:" \
                       "%(cnonce)s:%(qop)s:%(HA2)s" % final_dict
        response = hashlib.md5(response_str.encode("ascii")).hexdigest()

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
        request_handler.wfile.write(b"Proxy Authentication Required.")
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

        if "Proxy-Authorization" not in request_handler.headers:
            return self._return_auth_challenge(request_handler)
        else:
            auth_dict = self._create_auth_dict(
                request_handler.headers["Proxy-Authorization"]
                )
            if auth_dict["username"] in self._users:
                password = self._users[ auth_dict["username"] ]
            else:
                return self._return_auth_challenge(request_handler)
            if not auth_dict.get("nonce") in self._nonces:
                return self._return_auth_challenge(request_handler)
            else:
                self._nonces.remove(auth_dict["nonce"])

            auth_validated = False

            # MSIE uses short_path in its validation, but Python's
            # urllib.request uses the full path, so we're going to see if
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


class BasicAuthHandler(http.server.BaseHTTPRequestHandler):
    """Handler for performing basic authentication."""
    # Server side values
    USER = 'testUser'
    PASSWD = 'testPass'
    REALM = 'Test'
    USER_PASSWD = "%s:%s" % (USER, PASSWD)
    ENCODED_AUTH = base64.b64encode(USER_PASSWD.encode('ascii')).decode('ascii')

    def __init__(self, *args, **kwargs):
        http.server.BaseHTTPRequestHandler.__init__(self, *args, **kwargs)

    def log_message(self, format, *args):
        # Suppress console log message
        pass

    def do_HEAD(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()

    def do_AUTHHEAD(self):
        self.send_response(401)
        self.send_header("WWW-Authenticate", "Basic realm=\"%s\"" % self.REALM)
        self.send_header("Content-type", "text/html")
        self.end_headers()

    def do_GET(self):
        if not self.headers.get("Authorization", ""):
            self.do_AUTHHEAD()
            self.wfile.write(b"No Auth header received")
        elif self.headers.get(
                "Authorization", "") == "Basic " + self.ENCODED_AUTH:
            self.send_response(200)
            self.end_headers()
            self.wfile.write(b"It works")
        else:
            # Request Unauthorized
            self.do_AUTHHEAD()



# Proxy test infrastructure

class FakeProxyHandler(http.server.BaseHTTPRequestHandler):
    """This is a 'fake proxy' that makes it look like the entire
    internet has gone down due to a sudden zombie invasion.  It main
    utility is in providing us with authentication support for
    testing.
    """

    def __init__(self, digest_auth_handler, *args, **kwargs):
        # This has to be set before calling our parent's __init__(), which will
        # try to call do_GET().
        self.digest_auth_handler = digest_auth_handler
        http.server.BaseHTTPRequestHandler.__init__(self, *args, **kwargs)

    def log_message(self, format, *args):
        # Uncomment the next line for debugging.
        # sys.stderr.write(format % args)
        pass

    def do_GET(self):
        (scm, netloc, path, params, query, fragment) = urllib.parse.urlparse(
            self.path, "http")
        self.short_path = path
        if self.digest_auth_handler.handle_request(self):
            self.send_response(200, "OK")
            self.send_header("Content-Type", "text/html")
            self.end_headers()
            self.wfile.write(bytes("You've reached %s!<BR>" % self.path,
                                   "ascii"))
            self.wfile.write(b"Our apologies, but our server is down due to "
                             b"a sudden zombie invasion.")

# Test cases

@unittest.skipUnless(threading, "Threading required for this test.")
class BasicAuthTests(unittest.TestCase):
    USER = "testUser"
    PASSWD = "testPass"
    INCORRECT_PASSWD = "Incorrect"
    REALM = "Test"

    def setUp(self):
        super(BasicAuthTests, self).setUp()
        # With Basic Authentication
        def http_server_with_basic_auth_handler(*args, **kwargs):
            return BasicAuthHandler(*args, **kwargs)
        self.server = LoopbackHttpServerThread(http_server_with_basic_auth_handler)
        self.addCleanup(self.stop_server)
        self.server_url = 'http://127.0.0.1:%s' % self.server.port
        self.server.start()
        self.server.ready.wait()

    def stop_server(self):
        self.server.stop()
        self.server = None

    def tearDown(self):
        super(BasicAuthTests, self).tearDown()

    def test_basic_auth_success(self):
        ah = urllib.request.HTTPBasicAuthHandler()
        ah.add_password(self.REALM, self.server_url, self.USER, self.PASSWD)
        urllib.request.install_opener(urllib.request.build_opener(ah))
        try:
            self.assertTrue(urllib.request.urlopen(self.server_url))
        except urllib.error.HTTPError:
            self.fail("Basic auth failed for the url: %s", self.server_url)

    def test_basic_auth_httperror(self):
        ah = urllib.request.HTTPBasicAuthHandler()
        ah.add_password(self.REALM, self.server_url, self.USER, self.INCORRECT_PASSWD)
        urllib.request.install_opener(urllib.request.build_opener(ah))
        self.assertRaises(urllib.error.HTTPError, urllib.request.urlopen, self.server_url)


@unittest.skipUnless(threading, "Threading required for this test.")
class ProxyAuthTests(unittest.TestCase):
    URL = "http://localhost"

    USER = "tester"
    PASSWD = "test123"
    REALM = "TestRealm"

    def setUp(self):
        super(ProxyAuthTests, self).setUp()
        # Ignore proxy bypass settings in the environment.
        def restore_environ(old_environ):
            os.environ.clear()
            os.environ.update(old_environ)
        self.addCleanup(restore_environ, os.environ.copy())
        os.environ['NO_PROXY'] = ''
        os.environ['no_proxy'] = ''

        self.digest_auth_handler = DigestAuthHandler()
        self.digest_auth_handler.set_users({self.USER: self.PASSWD})
        self.digest_auth_handler.set_realm(self.REALM)
        # With Digest Authentication.
        def create_fake_proxy_handler(*args, **kwargs):
            return FakeProxyHandler(self.digest_auth_handler, *args, **kwargs)

        self.server = LoopbackHttpServerThread(create_fake_proxy_handler)
        self.addCleanup(self.stop_server)
        self.server.start()
        self.server.ready.wait()
        proxy_url = "http://127.0.0.1:%d" % self.server.port
        handler = urllib.request.ProxyHandler({"http" : proxy_url})
        self.proxy_digest_handler = urllib.request.ProxyDigestAuthHandler()
        self.opener = urllib.request.build_opener(
            handler, self.proxy_digest_handler)

    def stop_server(self):
        self.server.stop()
        self.server = None

    def test_proxy_with_bad_password_raises_httperror(self):
        self.proxy_digest_handler.add_password(self.REALM, self.URL,
                                               self.USER, self.PASSWD+"bad")
        self.digest_auth_handler.set_qop("auth")
        self.assertRaises(urllib.error.HTTPError,
                          self.opener.open,
                          self.URL)

    def test_proxy_with_no_password_raises_httperror(self):
        self.digest_auth_handler.set_qop("auth")
        self.assertRaises(urllib.error.HTTPError,
                          self.opener.open,
                          self.URL)

    def test_proxy_qop_auth_works(self):
        self.proxy_digest_handler.add_password(self.REALM, self.URL,
                                               self.USER, self.PASSWD)
        self.digest_auth_handler.set_qop("auth")
        result = self.opener.open(self.URL)
        while result.read():
            pass
        result.close()

    def test_proxy_qop_auth_int_works_or_throws_urlerror(self):
        self.proxy_digest_handler.add_password(self.REALM, self.URL,
                                               self.USER, self.PASSWD)
        self.digest_auth_handler.set_qop("auth-int")
        try:
            result = self.opener.open(self.URL)
        except urllib.error.URLError:
            # It's okay if we don't support auth-int, but we certainly
            # shouldn't receive any kind of exception here other than
            # a URLError.
            result = None
        if result:
            while result.read():
                pass
            result.close()


def GetRequestHandler(responses):

    class FakeHTTPRequestHandler(http.server.BaseHTTPRequestHandler):

        server_version = "TestHTTP/"
        requests = []
        headers_received = []
        port = 80

        def do_GET(self):
            body = self.send_head()
            while body:
                done = self.wfile.write(body)
                body = body[done:]

        def do_POST(self):
            content_length = self.headers["Content-Length"]
            post_data = self.rfile.read(int(content_length))
            self.do_GET()
            self.requests.append(post_data)

        def send_head(self):
            FakeHTTPRequestHandler.headers_received = self.headers
            self.requests.append(self.path)
            response_code, headers, body = responses.pop(0)

            self.send_response(response_code)

            for (header, value) in headers:
                self.send_header(header, value % {'port':self.port})
            if body:
                self.send_header("Content-type", "text/plain")
                self.end_headers()
                return body
            self.end_headers()

        def log_message(self, *args):
            pass


    return FakeHTTPRequestHandler


@unittest.skipUnless(threading, "Threading required for this test.")
class TestUrlopen(unittest.TestCase):
    """Tests urllib.request.urlopen using the network.

    These tests are not exhaustive.  Assuming that testing using files does a
    good job overall of some of the basic interface features.  There are no
    tests exercising the optional 'data' and 'proxies' arguments.  No tests
    for transparent redirection have been written.
    """

    def setUp(self):
        super(TestUrlopen, self).setUp()

        # Ignore proxies for localhost tests.
        def restore_environ(old_environ):
            os.environ.clear()
            os.environ.update(old_environ)
        self.addCleanup(restore_environ, os.environ.copy())
        os.environ['NO_PROXY'] = '*'
        os.environ['no_proxy'] = '*'

    def urlopen(self, url, data=None, **kwargs):
        l = []
        f = urllib.request.urlopen(url, data, **kwargs)
        try:
            # Exercise various methods
            l.extend(f.readlines(200))
            l.append(f.readline())
            l.append(f.read(1024))
            l.append(f.read())
        finally:
            f.close()
        return b"".join(l)

    def stop_server(self):
        self.server.stop()
        self.server = None

    def start_server(self, responses=None):
        if responses is None:
            responses = [(200, [], b"we don't care")]
        handler = GetRequestHandler(responses)

        self.server = LoopbackHttpServerThread(handler)
        self.addCleanup(self.stop_server)
        self.server.start()
        self.server.ready.wait()
        port = self.server.port
        handler.port = port
        return handler

    def start_https_server(self, responses=None, **kwargs):
        if not hasattr(urllib.request, 'HTTPSHandler'):
            self.skipTest('ssl support required')
        from test.ssl_servers import make_https_server
        if responses is None:
            responses = [(200, [], b"we care a bit")]
        handler = GetRequestHandler(responses)
        server = make_https_server(self, handler_class=handler, **kwargs)
        handler.port = server.port
        return handler

    def test_redirection(self):
        expected_response = b"We got here..."
        responses = [
            (302, [("Location", "http://localhost:%(port)s/somewhere_else")],
             ""),
            (200, [], expected_response)
        ]

        handler = self.start_server(responses)
        data = self.urlopen("http://localhost:%s/" % handler.port)
        self.assertEqual(data, expected_response)
        self.assertEqual(handler.requests, ["/", "/somewhere_else"])

    def test_chunked(self):
        expected_response = b"hello world"
        chunked_start = (
                        b'a\r\n'
                        b'hello worl\r\n'
                        b'1\r\n'
                        b'd\r\n'
                        b'0\r\n'
                        )
        response = [(200, [("Transfer-Encoding", "chunked")], chunked_start)]
        handler = self.start_server(response)
        data = self.urlopen("http://localhost:%s/" % handler.port)
        self.assertEqual(data, expected_response)

    def test_404(self):
        expected_response = b"Bad bad bad..."
        handler = self.start_server([(404, [], expected_response)])

        try:
            self.urlopen("http://localhost:%s/weeble" % handler.port)
        except urllib.error.URLError as f:
            data = f.read()
            f.close()
        else:
            self.fail("404 should raise URLError")

        self.assertEqual(data, expected_response)
        self.assertEqual(handler.requests, ["/weeble"])

    def test_200(self):
        expected_response = b"pycon 2008..."
        handler = self.start_server([(200, [], expected_response)])
        data = self.urlopen("http://localhost:%s/bizarre" % handler.port)
        self.assertEqual(data, expected_response)
        self.assertEqual(handler.requests, ["/bizarre"])

    def test_200_with_parameters(self):
        expected_response = b"pycon 2008..."
        handler = self.start_server([(200, [], expected_response)])
        data = self.urlopen("http://localhost:%s/bizarre" % handler.port,
                             b"get=with_feeling")
        self.assertEqual(data, expected_response)
        self.assertEqual(handler.requests, ["/bizarre", b"get=with_feeling"])

    def test_https(self):
        handler = self.start_https_server()
        context = ssl.create_default_context(cafile=CERT_localhost)
        data = self.urlopen("https://localhost:%s/bizarre" % handler.port, context=context)
        self.assertEqual(data, b"we care a bit")

    def test_https_with_cafile(self):
        handler = self.start_https_server(certfile=CERT_localhost)
        with support.check_warnings(('', DeprecationWarning)):
            # Good cert
            data = self.urlopen("https://localhost:%s/bizarre" % handler.port,
                                cafile=CERT_localhost)
            self.assertEqual(data, b"we care a bit")
            # Bad cert
            with self.assertRaises(urllib.error.URLError) as cm:
                self.urlopen("https://localhost:%s/bizarre" % handler.port,
                             cafile=CERT_fakehostname)
            # Good cert, but mismatching hostname
            handler = self.start_https_server(certfile=CERT_fakehostname)
            with self.assertRaises(ssl.CertificateError) as cm:
                self.urlopen("https://localhost:%s/bizarre" % handler.port,
                             cafile=CERT_fakehostname)

    def test_https_with_cadefault(self):
        handler = self.start_https_server(certfile=CERT_localhost)
        # Self-signed cert should fail verification with system certificate store
        with support.check_warnings(('', DeprecationWarning)):
            with self.assertRaises(urllib.error.URLError) as cm:
                self.urlopen("https://localhost:%s/bizarre" % handler.port,
                             cadefault=True)

    def test_https_sni(self):
        if ssl is None:
            self.skipTest("ssl module required")
        if not ssl.HAS_SNI:
            self.skipTest("SNI support required in OpenSSL")
        sni_name = None
        def cb_sni(ssl_sock, server_name, initial_context):
            nonlocal sni_name
            sni_name = server_name
        context = ssl.SSLContext(ssl.PROTOCOL_TLS)
        context.set_servername_callback(cb_sni)
        handler = self.start_https_server(context=context, certfile=CERT_localhost)
        context = ssl.create_default_context(cafile=CERT_localhost)
        self.urlopen("https://localhost:%s" % handler.port, context=context)
        self.assertEqual(sni_name, "localhost")

    def test_sending_headers(self):
        handler = self.start_server()
        req = urllib.request.Request("http://localhost:%s/" % handler.port,
                                     headers={"Range": "bytes=20-39"})
        with urllib.request.urlopen(req):
            pass
        self.assertEqual(handler.headers_received["Range"], "bytes=20-39")

    def test_basic(self):
        handler = self.start_server()
        open_url = urllib.request.urlopen("http://localhost:%s" % handler.port)
        for attr in ("read", "close", "info", "geturl"):
            self.assertTrue(hasattr(open_url, attr), "object returned from "
                         "urlopen lacks the %s attribute" % attr)
        try:
            self.assertTrue(open_url.read(), "calling 'read' failed")
        finally:
            open_url.close()

    def test_info(self):
        handler = self.start_server()
        open_url = urllib.request.urlopen(
            "http://localhost:%s" % handler.port)
        with open_url:
            info_obj = open_url.info()
        self.assertIsInstance(info_obj, email.message.Message,
                              "object returned by 'info' is not an "
                              "instance of email.message.Message")
        self.assertEqual(info_obj.get_content_subtype(), "plain")

    def test_geturl(self):
        # Make sure same URL as opened is returned by geturl.
        handler = self.start_server()
        open_url = urllib.request.urlopen("http://localhost:%s" % handler.port)
        with open_url:
            url = open_url.geturl()
        self.assertEqual(url, "http://localhost:%s" % handler.port)

    def test_iteration(self):
        expected_response = b"pycon 2008..."
        handler = self.start_server([(200, [], expected_response)])
        data = urllib.request.urlopen("http://localhost:%s" % handler.port)
        for line in data:
            self.assertEqual(line, expected_response)

    def test_line_iteration(self):
        lines = [b"We\n", b"got\n", b"here\n", b"verylong " * 8192 + b"\n"]
        expected_response = b"".join(lines)
        handler = self.start_server([(200, [], expected_response)])
        data = urllib.request.urlopen("http://localhost:%s" % handler.port)
        for index, line in enumerate(data):
            self.assertEqual(line, lines[index],
                             "Fetched line number %s doesn't match expected:\n"
                             "    Expected length was %s, got %s" %
                             (index, len(lines[index]), len(line)))
        self.assertEqual(index + 1, len(lines))


threads_key = None

def setUpModule():
    # Store the threading_setup in a key and ensure that it is cleaned up
    # in the tearDown
    global threads_key
    threads_key = support.threading_setup()

def tearDownModule():
    if threads_key:
        support.threading_cleanup(*threads_key)

if __name__ == "__main__":
    unittest.main()
