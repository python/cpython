"""Mock PyPI Server implementation, to use in tests.

This module also provides a simple test case to extend if you need to use
the PyPIServer all along your test case. Be sure to read the documentation
before any use.

XXX TODO:

The mock server can handle simple HTTP request (to simulate a simple index) or
XMLRPC requests, over HTTP. Both does not have the same intergface to deal
with, and I think it's a pain.

A good idea could be to re-think a bit the way dstributions are handled in the
mock server. As it should return malformed HTML pages, we need to keep the
static behavior.

I think of something like that:

    >>> server = PyPIMockServer()
    >>> server.startHTTP()
    >>> server.startXMLRPC()

Then, the server must have only one port to rely on, eg.

    >>> server.fulladress()
    "http://ip:port/"

It could be simple to have one HTTP server, relaying the requests to the two
implementations (static HTTP and XMLRPC over HTTP).
"""

import os
import queue
import select
import socket
import threading
import socketserver
from functools import wraps
from http.server import HTTPServer, SimpleHTTPRequestHandler
from xmlrpc.server import SimpleXMLRPCServer

from packaging.tests import unittest

PYPI_DEFAULT_STATIC_PATH = os.path.join(
    os.path.dirname(os.path.abspath(__file__)), 'pypiserver')


def use_xmlrpc_server(*server_args, **server_kwargs):
    server_kwargs['serve_xmlrpc'] = True
    return use_pypi_server(*server_args, **server_kwargs)


def use_http_server(*server_args, **server_kwargs):
    server_kwargs['serve_xmlrpc'] = False
    return use_pypi_server(*server_args, **server_kwargs)


def use_pypi_server(*server_args, **server_kwargs):
    """Decorator to make use of the PyPIServer for test methods,
    just when needed, and not for the entire duration of the testcase.
    """
    def wrapper(func):
        @wraps(func)
        def wrapped(*args, **kwargs):
            server = PyPIServer(*server_args, **server_kwargs)
            server.start()
            try:
                func(server=server, *args, **kwargs)
            finally:
                server.stop()
        return wrapped
    return wrapper


class PyPIServerTestCase(unittest.TestCase):

    def setUp(self):
        super(PyPIServerTestCase, self).setUp()
        self.pypi = PyPIServer()
        self.pypi.start()
        self.addCleanup(self.pypi.stop)


class PyPIServer(threading.Thread):
    """PyPI Mocked server.
    Provides a mocked version of the PyPI API's, to ease tests.

    Support serving static content and serving previously given text.
    """

    def __init__(self, test_static_path=None,
                 static_filesystem_paths=["default"],
                 static_uri_paths=["simple", "packages"], serve_xmlrpc=False):
        """Initialize the server.

        Default behavior is to start the HTTP server. You can either start the
        xmlrpc server by setting xmlrpc to True. Caution: Only one server will
        be started.

        static_uri_paths and static_base_path are parameters used to provides
        respectively the http_paths to serve statically, and where to find the
        matching files on the filesystem.
        """
        # we want to launch the server in a new dedicated thread, to not freeze
        # tests.
        threading.Thread.__init__(self)
        self._run = True
        self._serve_xmlrpc = serve_xmlrpc

        #TODO allow to serve XMLRPC and HTTP static files at the same time.
        if not self._serve_xmlrpc:
            self.server = HTTPServer(('127.0.0.1', 0), PyPIRequestHandler)
            self.server.RequestHandlerClass.pypi_server = self

            self.request_queue = queue.Queue()
            self._requests = []
            self.default_response_status = 404
            self.default_response_headers = [('Content-type', 'text/plain')]
            self.default_response_data = "The page does not exists"

            # initialize static paths / filesystems
            self.static_uri_paths = static_uri_paths

            # append the static paths defined locally
            if test_static_path is not None:
                static_filesystem_paths.append(test_static_path)
            self.static_filesystem_paths = [
                PYPI_DEFAULT_STATIC_PATH + "/" + path
                for path in static_filesystem_paths]
        else:
            # XMLRPC server
            self.server = PyPIXMLRPCServer(('127.0.0.1', 0))
            self.xmlrpc = XMLRPCMockIndex()
            # register the xmlrpc methods
            self.server.register_introspection_functions()
            self.server.register_instance(self.xmlrpc)

        self.address = (self.server.server_name, self.server.server_port)
        # to not have unwanted outputs.
        self.server.RequestHandlerClass.log_request = lambda *_: None

    def run(self):
        # loop because we can't stop it otherwise, for python < 2.6
        while self._run:
            r, w, e = select.select([self.server], [], [], 0.5)
            if r:
                self.server.handle_request()

    def stop(self):
        """self shutdown is not supported for python < 2.6"""
        self._run = False
        if self.is_alive():
            self.join()
        self.server.server_close()

    def get_next_response(self):
        return (self.default_response_status,
                self.default_response_headers,
                self.default_response_data)

    @property
    def requests(self):
        """Use this property to get all requests that have been made
        to the server
        """
        while True:
            try:
                self._requests.append(self.request_queue.get_nowait())
            except queue.Empty:
                break
        return self._requests

    @property
    def full_address(self):
        return "http://%s:%s" % self.address


class PyPIRequestHandler(SimpleHTTPRequestHandler):
    # we need to access the pypi server while serving the content
    pypi_server = None

    def serve_request(self):
        """Serve the content.

        Also record the requests to be accessed later. If trying to access an
        url matching a static uri, serve static content, otherwise serve
        what is provided by the `get_next_response` method.

        If nothing is defined there, return a 404 header.
        """
        # record the request. Read the input only on PUT or POST requests
        if self.command in ("PUT", "POST"):
            if 'content-length' in self.headers:
                request_data = self.rfile.read(
                    int(self.headers['content-length']))
            else:
                request_data = self.rfile.read()

        elif self.command in ("GET", "DELETE"):
            request_data = ''

        self.pypi_server.request_queue.put((self, request_data))

        # serve the content from local disc if we request an URL beginning
        # by a pattern defined in `static_paths`
        url_parts = self.path.split("/")
        if (len(url_parts) > 1 and
                url_parts[1] in self.pypi_server.static_uri_paths):
            data = None
            # always take the last first.
            fs_paths = []
            fs_paths.extend(self.pypi_server.static_filesystem_paths)
            fs_paths.reverse()
            relative_path = self.path
            for fs_path in fs_paths:
                try:
                    if self.path.endswith("/"):
                        relative_path += "index.html"

                    if relative_path.endswith('.tar.gz'):
                        with open(fs_path + relative_path, 'br') as file:
                            data = file.read()
                        headers = [('Content-type', 'application/x-gtar')]
                    else:
                        with open(fs_path + relative_path) as file:
                            data = file.read().encode()
                        headers = [('Content-type', 'text/html')]

                    self.make_response(data, headers=headers)

                except IOError:
                    pass

            if data is None:
                self.make_response("Not found", 404)

        # otherwise serve the content from get_next_response
        else:
            # send back a response
            status, headers, data = self.pypi_server.get_next_response()
            self.make_response(data, status, headers)

    do_POST = do_GET = do_DELETE = do_PUT = serve_request

    def make_response(self, data, status=200,
                      headers=[('Content-type', 'text/html')]):
        """Send the response to the HTTP client"""
        if not isinstance(status, int):
            try:
                status = int(status)
            except ValueError:
                # we probably got something like YYY Codename.
                # Just get the first 3 digits
                status = int(status[:3])

        self.send_response(status)
        for header, value in headers:
            self.send_header(header, value)
        self.end_headers()

        if type(data) is str:
            data = data.encode()

        self.wfile.write(data)


class PyPIXMLRPCServer(SimpleXMLRPCServer):
    def server_bind(self):
        """Override server_bind to store the server name."""
        socketserver.TCPServer.server_bind(self)
        host, port = self.socket.getsockname()[:2]
        self.server_name = socket.getfqdn(host)
        self.server_port = port


class MockDist:
    """Fake distribution, used in the Mock PyPI Server"""

    def __init__(self, name, version="1.0", hidden=False, url="http://url/",
             type="sdist", filename="", size=10000,
             digest="123456", downloads=7, has_sig=False,
             python_version="source", comment="comment",
             author="John Doe", author_email="john@doe.name",
             maintainer="Main Tayner", maintainer_email="maintainer_mail",
             project_url="http://project_url/", homepage="http://homepage/",
             keywords="", platform="UNKNOWN", classifiers=[], licence="",
             description="Description", summary="Summary", stable_version="",
             ordering="", documentation_id="", code_kwalitee_id="",
             installability_id="", obsoletes=[], obsoletes_dist=[],
             provides=[], provides_dist=[], requires=[], requires_dist=[],
             requires_external=[], requires_python=""):

        # basic fields
        self.name = name
        self.version = version
        self.hidden = hidden

        # URL infos
        self.url = url
        self.digest = digest
        self.downloads = downloads
        self.has_sig = has_sig
        self.python_version = python_version
        self.comment = comment
        self.type = type

        # metadata
        self.author = author
        self.author_email = author_email
        self.maintainer = maintainer
        self.maintainer_email = maintainer_email
        self.project_url = project_url
        self.homepage = homepage
        self.keywords = keywords
        self.platform = platform
        self.classifiers = classifiers
        self.licence = licence
        self.description = description
        self.summary = summary
        self.stable_version = stable_version
        self.ordering = ordering
        self.cheesecake_documentation_id = documentation_id
        self.cheesecake_code_kwalitee_id = code_kwalitee_id
        self.cheesecake_installability_id = installability_id

        self.obsoletes = obsoletes
        self.obsoletes_dist = obsoletes_dist
        self.provides = provides
        self.provides_dist = provides_dist
        self.requires = requires
        self.requires_dist = requires_dist
        self.requires_external = requires_external
        self.requires_python = requires_python

    def url_infos(self):
        return {
            'url': self.url,
            'packagetype': self.type,
            'filename': 'filename.tar.gz',
            'size': '6000',
            'md5_digest': self.digest,
            'downloads': self.downloads,
            'has_sig': self.has_sig,
            'python_version': self.python_version,
            'comment_text': self.comment,
        }

    def metadata(self):
        return {
            'maintainer': self.maintainer,
            'project_url': [self.project_url],
            'maintainer_email': self.maintainer_email,
            'cheesecake_code_kwalitee_id': self.cheesecake_code_kwalitee_id,
            'keywords': self.keywords,
            'obsoletes_dist': self.obsoletes_dist,
            'requires_external': self.requires_external,
            'author': self.author,
            'author_email': self.author_email,
            'download_url': self.url,
            'platform': self.platform,
            'version': self.version,
            'obsoletes': self.obsoletes,
            'provides': self.provides,
            'cheesecake_documentation_id': self.cheesecake_documentation_id,
            '_pypi_hidden': self.hidden,
            'description': self.description,
            '_pypi_ordering': 19,
            'requires_dist': self.requires_dist,
            'requires_python': self.requires_python,
            'classifiers': [],
            'name': self.name,
            'licence': self.licence,
            'summary': self.summary,
            'home_page': self.homepage,
            'stable_version': self.stable_version,
            'provides_dist': self.provides_dist or "%s (%s)" % (self.name,
                                                              self.version),
            'requires': self.requires,
            'cheesecake_installability_id': self.cheesecake_installability_id,
        }

    def search_result(self):
        return {
            '_pypi_ordering': 0,
            'version': self.version,
            'name': self.name,
            'summary': self.summary,
        }


class XMLRPCMockIndex:
    """Mock XMLRPC server"""

    def __init__(self, dists=[]):
        self._dists = dists
        self._search_result = []

    def add_distributions(self, dists):
        for dist in dists:
            self._dists.append(MockDist(**dist))

    def set_distributions(self, dists):
        self._dists = []
        self.add_distributions(dists)

    def set_search_result(self, result):
        """set a predefined search result"""
        self._search_result = result

    def _get_search_results(self):
        results = []
        for name in self._search_result:
            found_dist = [d for d in self._dists if d.name == name]
            if found_dist:
                results.append(found_dist[0])
            else:
                dist = MockDist(name)
                results.append(dist)
                self._dists.append(dist)
        return [r.search_result() for r in results]

    def list_packages(self):
        return [d.name for d in self._dists]

    def package_releases(self, package_name, show_hidden=False):
        if show_hidden:
            # return all
            return [d.version for d in self._dists if d.name == package_name]
        else:
            # return only un-hidden
            return [d.version for d in self._dists if d.name == package_name
                    and not d.hidden]

    def release_urls(self, package_name, version):
        return [d.url_infos() for d in self._dists
                if d.name == package_name and d.version == version]

    def release_data(self, package_name, version):
        release = [d for d in self._dists
                   if d.name == package_name and d.version == version]
        if release:
            return release[0].metadata()
        else:
            return {}

    def search(self, spec, operator="and"):
        return self._get_search_results()
