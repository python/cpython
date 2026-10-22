from xmlrpc.server import DocXMLRPCServer
import http.client
import re
import sys
import threading
import unittest
from test import support

support.requires_working_socket(module=True)

def make_request_and_skipIf(condition, reason):
    # If we skip the test, we have to make a request because
    # the server created in setUp blocks expecting one to come in.
    if not condition:
        return lambda func: func
    def decorator(func):
        def make_request_and_skip(self):
            self.client.request("GET", "/")
            self.client.getresponse()
            raise unittest.SkipTest(reason)
        return make_request_and_skip
    return decorator


def make_server():
    serv = DocXMLRPCServer(("localhost", 0), logRequests=False)

    try:
        # Add some documentation
        serv.set_server_title("DocXMLRPCServer Test Documentation")
        serv.set_server_name("DocXMLRPCServer Test Docs")
        serv.set_server_documentation(
            "This is an XML-RPC server's documentation, but the server "
            "can be used by POSTing to /RPC2. Try self.add, too.")

        # Create and register classes and functions
        class TestClass(object):
            def test_method(self, arg):
                """Test method's docs. This method truly does very little."""
                self.arg = arg

        serv.register_introspection_functions()
        serv.register_instance(TestClass())

        def add(x, y):
            """Add two instances together. This follows PEP008, but has nothing
            to do with RFC1952. Case should matter: pEp008 and rFC1952.  Things
            that start with http and ftp should be auto-linked, too:
            http://google.com.
            """
            return x + y

        def annotation(x: int):
            """ Use function annotations. """
            return x

        class ClassWithAnnotation:
            def method_annotation(self, x: bytes):
                return x.decode()

        serv.register_function(add)
        serv.register_function(lambda x, y: x-y)
        serv.register_function(annotation)
        serv.register_instance(ClassWithAnnotation())
        return serv
    except:
        serv.server_close()
        raise

class DocXMLRPCHTTPGETServer(unittest.TestCase):
    def setUp(self):
        # Enable server feedback
        DocXMLRPCServer._send_traceback_header = True

        self.serv = make_server()
        self.thread = threading.Thread(target=self.serv.serve_forever)
        self.thread.start()

        PORT = self.serv.server_address[1]
        self.client = http.client.HTTPConnection("localhost:%d" % PORT)

    def tearDown(self):
        self.client.close()

        # Disable server feedback
        DocXMLRPCServer._send_traceback_header = False
        self.serv.shutdown()
        self.thread.join()
        self.serv.server_close()

    def test_valid_get_response(self):
        self.client.request("GET", "/")
        response = self.client.getresponse()

        self.assertEqual(response.status, 200)
        self.assertEqual(response.getheader("Content-type"), "text/html; charset=UTF-8")

        # Server raises an exception if we don't start to read the data
        response.read()

    def test_get_css(self):
        self.client.request("GET", "/pydoc.css")
        response = self.client.getresponse()

        self.assertEqual(response.status, 200)
        self.assertEqual(response.getheader("Content-type"), "text/css; charset=UTF-8")

        # Server raises an exception if we don't start to read the data
        response.read()

    def test_invalid_get_response(self):
        self.client.request("GET", "/spam")
        response = self.client.getresponse()

        self.assertEqual(response.status, 404)
        self.assertEqual(response.getheader("Content-type"), "text/plain")

        response.read()

    def test_lambda(self):
        """Test that lambda functionality stays the same.  The output produced
        currently is, I suspect invalid because of the unencoded brackets in the
        HTML, "<lambda>".

        The subtraction lambda method is tested.
        """
        self.client.request("GET", "/")
        response = self.client.getresponse()

        self.assertIn((b'<dl class="doc"><dt><a id="-&lt;lambda&gt;"><strong>'
                       b'&lt;lambda&gt;</strong></a>(x, y)</dt></dl>'),
                      response.read())

    @make_request_and_skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def test_autolinking(self):
        """Test that the server correctly automatically wraps references to
        PEPS and RFCs with links, and that it linkifies text starting with
        http or ftp protocol prefixes.

        The documentation for the "add" method contains the test material.
        """
        self.client.request("GET", "/")
        response = self.client.getresponse().read()

        self.assertIn(
            (b'<dl class="doc"><dt><a id="-add"><strong>add</strong></a>'
             b'(x, y)</dt><dd class="docstring">'
             b'Add two instances together. This '
             b'follows <a href="https://peps.python.org/pep-0008/">'
             b'PEP008</a>, but has nothing\nto do '
             b'with <a href="https://www.rfc-editor.org/rfc/rfc1952.txt">'
             b'RFC1952</a>. Case should matter: pEp008 '
             b'and rFC1952.  Things\nthat start '
             b'with http and ftp should be '
             b'auto-linked, too:\n<a href="http://google.com">'
             b'http://google.com</a>.</dd></dl>'), response)

    @make_request_and_skipIf(sys.flags.optimize >= 2,
                     "Docstrings are omitted with -O2 and above")
    def test_system_methods(self):
        """Test the presence of three consecutive system.* methods.

        This also tests their use of parameter type recognition and the
        systems related to that process.
        """
        self.client.request("GET", "/")
        response = self.client.getresponse().read()

        self.assertIn(
            (b'<dl class="doc"><dt><a id="-system.methodHelp"><strong>'
             b'system.methodHelp</strong></a>(method_name)</dt>'
             b'<dd class="docstring"><a href="#-system.method'
             b'Help">system.methodHelp</a>(\'add\') =&gt; "Adds '
             b'two integers together"\n\nReturns a'
             b' string containing documentation for '
             b'the specified method.</dd></dl>\n<dl class="doc"><dt><a id'
             b'="-system.methodSignature"><strong>system.methodSignature'
             b'</strong></a>(method_name)</dt><dd class="docstring">'
             b'<a href="#-system.methodSignature">'
             b'system.methodSignature</a>(\'add\') =&gt; [double, '
             b'int, int]\n\nReturns a list '
             b'describing the signature of the method.'
             b' In the\nabove example, the add '
             b'method takes two integers as arguments'
             b'\nand returns a double result.\n\n'
             b'This server does NOT support system'
             b'.methodSignature.</dd></dl>'), response)

    def test_autolink_dotted_methods(self):
        """Test that selfdot values are made strong automatically in the
        documentation."""
        self.client.request("GET", "/")
        response = self.client.getresponse()

        self.assertIn(b"""Try self.<strong>add</strong>, too.""",
                      response.read())

    def test_annotations(self):
        """ Test that annotations works as expected """
        self.client.request("GET", "/")
        response = self.client.getresponse()
        docstring = (b'' if sys.flags.optimize >= 2 else
                     b'<dd class="docstring">Use function annotations.</dd>')
        self.assertIn(
            (b'<dl class="doc"><dt><a id="-annotation"><strong>annotation'
             b'</strong></a>(x: int)</dt>' + docstring + b'</dl>\n'
             b'<dl class="doc"><dt><a id="-method_annotation"><strong>'
             b'method_annotation</strong></a>(x: bytes)</dt></dl>'),
            response.read())

    def test_server_title_escape(self):
        # bpo-38243: Ensure that the server title and documentation
        # are escaped for HTML.
        self.serv.set_server_title('test_title<script>')
        self.serv.set_server_documentation('test_documentation<script>')
        self.assertEqual('test_title<script>', self.serv.server_title)
        self.assertEqual('test_documentation<script>',
                self.serv.server_documentation)

        generated = self.serv.generate_html_documentation()
        title = re.search(r'<title>(.+?)</title>', generated).group()
        documentation = re.search(r'<div class="docstring">(.+?)</div>',
                                  generated).group()
        self.assertEqual('<title>Python: test_title&lt;script&gt;</title>', title)
        self.assertEqual('<div class="docstring">test_documentation&lt;script&gt;</div>',
                documentation)


if __name__ == '__main__':
    unittest.main()
