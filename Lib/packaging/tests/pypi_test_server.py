"""Test PyPI Server implementation at testpypi.python.org, to use in tests.

This is a drop-in replacement for the mock pypi server for testing against a
real pypi server hosted by python.org especially for testing against.
"""

import unittest

PYPI_DEFAULT_STATIC_PATH = None


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
        def wrapped(*args, **kwargs):
            server = PyPIServer(*server_args, **server_kwargs)
            func(server=server, *args, **kwargs)
        return wrapped
    return wrapper


class PyPIServerTestCase(unittest.TestCase):

    def setUp(self):
        super(PyPIServerTestCase, self).setUp()
        self.pypi = PyPIServer()
        self.pypi.start()
        self.addCleanup(self.pypi.stop)


class PyPIServer:
    """Shim to access testpypi.python.org, for testing a real server."""

    def __init__(self, test_static_path=None,
                 static_filesystem_paths=["default"],
                 static_uri_paths=["simple"], serve_xmlrpc=False):
        self.address = ('testpypi.python.org', '80')

    def start(self):
        pass

    def stop(self):
        pass

    @property
    def full_address(self):
        return "http://%s:%s" % self.address
