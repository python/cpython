"""Utilities shared by tests."""

import collections
import contextlib
import io
import os
import re
import socket
import socketserver
import sys
import tempfile
import threading
import time
from unittest import mock

from http.server import HTTPServer
from wsgiref.simple_server import WSGIRequestHandler, WSGIServer

try:
    import ssl
except ImportError:  # pragma: no cover
    ssl = None

from . import base_events
from . import events
from . import futures
from . import selectors
from . import tasks


if sys.platform == 'win32':  # pragma: no cover
    from .windows_utils import socketpair
else:
    from socket import socketpair  # pragma: no cover


def dummy_ssl_context():
    if ssl is None:
        return None
    else:
        return ssl.SSLContext(ssl.PROTOCOL_SSLv23)


def run_briefly(loop):
    @tasks.coroutine
    def once():
        pass
    gen = once()
    t = tasks.Task(gen, loop=loop)
    try:
        loop.run_until_complete(t)
    finally:
        gen.close()


def run_until(loop, pred, timeout=30):
    deadline = time.time() + timeout
    while not pred():
        if timeout is not None:
            timeout = deadline - time.time()
            if timeout <= 0:
                raise futures.TimeoutError()
        loop.run_until_complete(tasks.sleep(0.001, loop=loop))


def run_once(loop):
    """loop.stop() schedules _raise_stop_error()
    and run_forever() runs until _raise_stop_error() callback.
    this wont work if test waits for some IO events, because
    _raise_stop_error() runs before any of io events callbacks.
    """
    loop.stop()
    loop.run_forever()


class SilentWSGIRequestHandler(WSGIRequestHandler):

    def get_stderr(self):
        return io.StringIO()

    def log_message(self, format, *args):
        pass


class SilentWSGIServer(WSGIServer):

    def handle_error(self, request, client_address):
        pass


class SSLWSGIServerMixin:

    def finish_request(self, request, client_address):
        # The relative location of our test directory (which
        # contains the ssl key and certificate files) differs
        # between the stdlib and stand-alone asyncio.
        # Prefer our own if we can find it.
        here = os.path.join(os.path.dirname(__file__), '..', 'tests')
        if not os.path.isdir(here):
            here = os.path.join(os.path.dirname(os.__file__),
                                'test', 'test_asyncio')
        keyfile = os.path.join(here, 'ssl_key.pem')
        certfile = os.path.join(here, 'ssl_cert.pem')
        ssock = ssl.wrap_socket(request,
                                keyfile=keyfile,
                                certfile=certfile,
                                server_side=True)
        try:
            self.RequestHandlerClass(ssock, client_address, self)
            ssock.close()
        except OSError:
            # maybe socket has been closed by peer
            pass


class SSLWSGIServer(SSLWSGIServerMixin, SilentWSGIServer):
    pass


def _run_test_server(*, address, use_ssl=False, server_cls, server_ssl_cls):

    def app(environ, start_response):
        status = '200 OK'
        headers = [('Content-type', 'text/plain')]
        start_response(status, headers)
        return [b'Test message']

    # Run the test WSGI server in a separate thread in order not to
    # interfere with event handling in the main thread
    server_class = server_ssl_cls if use_ssl else server_cls
    httpd = server_class(address, SilentWSGIRequestHandler)
    httpd.set_app(app)
    httpd.address = httpd.server_address
    server_thread = threading.Thread(target=httpd.serve_forever)
    server_thread.start()
    try:
        yield httpd
    finally:
        httpd.shutdown()
        httpd.server_close()
        server_thread.join()


if hasattr(socket, 'AF_UNIX'):

    class UnixHTTPServer(socketserver.UnixStreamServer, HTTPServer):

        def server_bind(self):
            socketserver.UnixStreamServer.server_bind(self)
            self.server_name = '127.0.0.1'
            self.server_port = 80


    class UnixWSGIServer(UnixHTTPServer, WSGIServer):

        def server_bind(self):
            UnixHTTPServer.server_bind(self)
            self.setup_environ()

        def get_request(self):
            request, client_addr = super().get_request()
            # Code in the stdlib expects that get_request
            # will return a socket and a tuple (host, port).
            # However, this isn't true for UNIX sockets,
            # as the second return value will be a path;
            # hence we return some fake data sufficient
            # to get the tests going
            return request, ('127.0.0.1', '')


    class SilentUnixWSGIServer(UnixWSGIServer):

        def handle_error(self, request, client_address):
            pass


    class UnixSSLWSGIServer(SSLWSGIServerMixin, SilentUnixWSGIServer):
        pass


    def gen_unix_socket_path():
        with tempfile.NamedTemporaryFile() as file:
            return file.name


    @contextlib.contextmanager
    def unix_socket_path():
        path = gen_unix_socket_path()
        try:
            yield path
        finally:
            try:
                os.unlink(path)
            except OSError:
                pass


    @contextlib.contextmanager
    def run_test_unix_server(*, use_ssl=False):
        with unix_socket_path() as path:
            yield from _run_test_server(address=path, use_ssl=use_ssl,
                                        server_cls=SilentUnixWSGIServer,
                                        server_ssl_cls=UnixSSLWSGIServer)


@contextlib.contextmanager
def run_test_server(*, host='127.0.0.1', port=0, use_ssl=False):
    yield from _run_test_server(address=(host, port), use_ssl=use_ssl,
                                server_cls=SilentWSGIServer,
                                server_ssl_cls=SSLWSGIServer)


def make_test_protocol(base):
    dct = {}
    for name in dir(base):
        if name.startswith('__') and name.endswith('__'):
            # skip magic names
            continue
        dct[name] = MockCallback(return_value=None)
    return type('TestProtocol', (base,) + base.__bases__, dct)()


class TestSelector(selectors.BaseSelector):

    def __init__(self):
        self.keys = {}

    def register(self, fileobj, events, data=None):
        key = selectors.SelectorKey(fileobj, 0, events, data)
        self.keys[fileobj] = key
        return key

    def unregister(self, fileobj):
        return self.keys.pop(fileobj)

    def select(self, timeout):
        return []

    def get_map(self):
        return self.keys


class TestLoop(base_events.BaseEventLoop):
    """Loop for unittests.

    It manages self time directly.
    If something scheduled to be executed later then
    on next loop iteration after all ready handlers done
    generator passed to __init__ is calling.

    Generator should be like this:

        def gen():
            ...
            when = yield ...
            ... = yield time_advance

    Value returned by yield is absolute time of next scheduled handler.
    Value passed to yield is time advance to move loop's time forward.
    """

    def __init__(self, gen=None):
        super().__init__()

        if gen is None:
            def gen():
                yield
            self._check_on_close = False
        else:
            self._check_on_close = True

        self._gen = gen()
        next(self._gen)
        self._time = 0
        self._clock_resolution = 1e-9
        self._timers = []
        self._selector = TestSelector()

        self.readers = {}
        self.writers = {}
        self.reset_counters()

    def time(self):
        return self._time

    def advance_time(self, advance):
        """Move test time forward."""
        if advance:
            self._time += advance

    def close(self):
        if self._check_on_close:
            try:
                self._gen.send(0)
            except StopIteration:
                pass
            else:  # pragma: no cover
                raise AssertionError("Time generator is not finished")

    def add_reader(self, fd, callback, *args):
        self.readers[fd] = events.Handle(callback, args, self)

    def remove_reader(self, fd):
        self.remove_reader_count[fd] += 1
        if fd in self.readers:
            del self.readers[fd]
            return True
        else:
            return False

    def assert_reader(self, fd, callback, *args):
        assert fd in self.readers, 'fd {} is not registered'.format(fd)
        handle = self.readers[fd]
        assert handle._callback == callback, '{!r} != {!r}'.format(
            handle._callback, callback)
        assert handle._args == args, '{!r} != {!r}'.format(
            handle._args, args)

    def add_writer(self, fd, callback, *args):
        self.writers[fd] = events.Handle(callback, args, self)

    def remove_writer(self, fd):
        self.remove_writer_count[fd] += 1
        if fd in self.writers:
            del self.writers[fd]
            return True
        else:
            return False

    def assert_writer(self, fd, callback, *args):
        assert fd in self.writers, 'fd {} is not registered'.format(fd)
        handle = self.writers[fd]
        assert handle._callback == callback, '{!r} != {!r}'.format(
            handle._callback, callback)
        assert handle._args == args, '{!r} != {!r}'.format(
            handle._args, args)

    def reset_counters(self):
        self.remove_reader_count = collections.defaultdict(int)
        self.remove_writer_count = collections.defaultdict(int)

    def _run_once(self):
        super()._run_once()
        for when in self._timers:
            advance = self._gen.send(when)
            self.advance_time(advance)
        self._timers = []

    def call_at(self, when, callback, *args):
        self._timers.append(when)
        return super().call_at(when, callback, *args)

    def _process_events(self, event_list):
        return

    def _write_to_self(self):
        pass


def MockCallback(**kwargs):
    return mock.Mock(spec=['__call__'], **kwargs)


class MockPattern(str):
    """A regex based str with a fuzzy __eq__.

    Use this helper with 'mock.assert_called_with', or anywhere
    where a regex comparison between strings is needed.

    For instance:
       mock_call.assert_called_with(MockPattern('spam.*ham'))
    """
    def __eq__(self, other):
        return bool(re.search(str(self), other, re.S))
