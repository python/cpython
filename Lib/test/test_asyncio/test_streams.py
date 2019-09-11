"""Tests for streams.py."""

import contextlib
import gc
import io
import os
import queue
import pickle
import socket
import sys
import threading
import unittest
from unittest import mock
from test import support
try:
    import ssl
except ImportError:
    ssl = None

import asyncio
from asyncio.streams import _StreamProtocol, _ensure_can_read, _ensure_can_write
from test.test_asyncio import utils as test_utils


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class StreamModeTests(unittest.TestCase):
    def test__ensure_can_read_ok(self):
        self.assertIsNone(_ensure_can_read(asyncio.StreamMode.READ))
        self.assertIsNone(_ensure_can_read(asyncio.StreamMode.READWRITE))

    def test__ensure_can_read_fail(self):
        with self.assertRaisesRegex(RuntimeError, "The stream is write-only"):
            _ensure_can_read(asyncio.StreamMode.WRITE)

    def test__ensure_can_write_ok(self):
        self.assertIsNone(_ensure_can_write(asyncio.StreamMode.WRITE))
        self.assertIsNone(_ensure_can_write(asyncio.StreamMode.READWRITE))

    def test__ensure_can_write_fail(self):
        with self.assertRaisesRegex(RuntimeError, "The stream is read-only"):
            _ensure_can_write(asyncio.StreamMode.READ)


class StreamTests(test_utils.TestCase):

    DATA = b'line1\nline2\nline3\n'

    def setUp(self):
        super().setUp()
        self.loop = asyncio.new_event_loop()
        self.set_event_loop(self.loop)

    def tearDown(self):
        # just in case if we have transport close callbacks
        test_utils.run_briefly(self.loop)

        self.loop.close()
        gc.collect()
        super().tearDown()

    @mock.patch('asyncio.streams.events')
    def test_ctor_global_loop(self, m_events):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                _asyncio_internal=True)
        self.assertIs(stream._loop, m_events.get_event_loop.return_value)

    def _basetest_open_connection(self, open_connection_fut):
        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        with self.assertWarns(DeprecationWarning):
            reader, writer = self.loop.run_until_complete(open_connection_fut)
        writer.write(b'GET / HTTP/1.0\r\n\r\n')
        f = reader.readline()
        data = self.loop.run_until_complete(f)
        self.assertEqual(data, b'HTTP/1.0 200 OK\r\n')
        f = reader.read()
        data = self.loop.run_until_complete(f)
        self.assertTrue(data.endswith(b'\r\n\r\nTest message'))
        writer.close()
        self.assertEqual(messages, [])

    def test_open_connection(self):
        with test_utils.run_test_server() as httpd:
            conn_fut = asyncio.open_connection(*httpd.address,
                                               loop=self.loop)
            self._basetest_open_connection(conn_fut)

    @support.skip_unless_bind_unix_socket
    def test_open_unix_connection(self):
        with test_utils.run_test_unix_server() as httpd:
            conn_fut = asyncio.open_unix_connection(httpd.address,
                                                    loop=self.loop)
            self._basetest_open_connection(conn_fut)

    def _basetest_open_connection_no_loop_ssl(self, open_connection_fut):
        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        try:
            with self.assertWarns(DeprecationWarning):
                reader, writer = self.loop.run_until_complete(
                    open_connection_fut)
        finally:
            asyncio.set_event_loop(None)
        writer.write(b'GET / HTTP/1.0\r\n\r\n')
        f = reader.read()
        data = self.loop.run_until_complete(f)
        self.assertTrue(data.endswith(b'\r\n\r\nTest message'))

        writer.close()
        self.assertEqual(messages, [])

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_open_connection_no_loop_ssl(self):
        with test_utils.run_test_server(use_ssl=True) as httpd:
            conn_fut = asyncio.open_connection(
                *httpd.address,
                ssl=test_utils.dummy_ssl_context(),
                loop=self.loop)

            self._basetest_open_connection_no_loop_ssl(conn_fut)

    @support.skip_unless_bind_unix_socket
    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_open_unix_connection_no_loop_ssl(self):
        with test_utils.run_test_unix_server(use_ssl=True) as httpd:
            conn_fut = asyncio.open_unix_connection(
                httpd.address,
                ssl=test_utils.dummy_ssl_context(),
                server_hostname='',
                loop=self.loop)

            self._basetest_open_connection_no_loop_ssl(conn_fut)

    def _basetest_open_connection_error(self, open_connection_fut):
        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        with self.assertWarns(DeprecationWarning):
            reader, writer = self.loop.run_until_complete(open_connection_fut)
        writer._protocol.connection_lost(ZeroDivisionError())
        f = reader.read()
        with self.assertRaises(ZeroDivisionError):
            self.loop.run_until_complete(f)
        writer.close()
        test_utils.run_briefly(self.loop)
        self.assertEqual(messages, [])

    def test_open_connection_error(self):
        with test_utils.run_test_server() as httpd:
            conn_fut = asyncio.open_connection(*httpd.address,
                                               loop=self.loop)
            self._basetest_open_connection_error(conn_fut)

    @support.skip_unless_bind_unix_socket
    def test_open_unix_connection_error(self):
        with test_utils.run_test_unix_server() as httpd:
            conn_fut = asyncio.open_unix_connection(httpd.address,
                                                    loop=self.loop)
            self._basetest_open_connection_error(conn_fut)

    def test_feed_empty_data(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)

        stream._feed_data(b'')
        self.assertEqual(b'', stream._buffer)

    def test_feed_nonempty_data(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)

        stream._feed_data(self.DATA)
        self.assertEqual(self.DATA, stream._buffer)

    def test_read_zero(self):
        # Read zero bytes.
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(self.DATA)

        data = self.loop.run_until_complete(stream.read(0))
        self.assertEqual(b'', data)
        self.assertEqual(self.DATA, stream._buffer)

    def test_read(self):
        # Read bytes.
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        read_task = self.loop.create_task(stream.read(30))

        def cb():
            stream._feed_data(self.DATA)
        self.loop.call_soon(cb)

        data = self.loop.run_until_complete(read_task)
        self.assertEqual(self.DATA, data)
        self.assertEqual(b'', stream._buffer)

    def test_read_line_breaks(self):
        # Read bytes without line breaks.
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(b'line1')
        stream._feed_data(b'line2')

        data = self.loop.run_until_complete(stream.read(5))

        self.assertEqual(b'line1', data)
        self.assertEqual(b'line2', stream._buffer)

    def test_read_eof(self):
        # Read bytes, stop at eof.
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        read_task = self.loop.create_task(stream.read(1024))

        def cb():
            stream._feed_eof()
        self.loop.call_soon(cb)

        data = self.loop.run_until_complete(read_task)
        self.assertEqual(b'', data)
        self.assertEqual(b'', stream._buffer)

    def test_read_until_eof(self):
        # Read all bytes until eof.
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        read_task = self.loop.create_task(stream.read(-1))

        def cb():
            stream._feed_data(b'chunk1\n')
            stream._feed_data(b'chunk2')
            stream._feed_eof()
        self.loop.call_soon(cb)

        data = self.loop.run_until_complete(read_task)

        self.assertEqual(b'chunk1\nchunk2', data)
        self.assertEqual(b'', stream._buffer)

    def test_read_exception(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(b'line\n')

        data = self.loop.run_until_complete(stream.read(2))
        self.assertEqual(b'li', data)

        stream._set_exception(ValueError())
        self.assertRaises(
            ValueError, self.loop.run_until_complete, stream.read(2))

    def test_invalid_limit(self):
        with self.assertRaisesRegex(ValueError, 'imit'):
            asyncio.Stream(mode=asyncio.StreamMode.READ,
                           limit=0, loop=self.loop,
                           _asyncio_internal=True)

        with self.assertRaisesRegex(ValueError, 'imit'):
            asyncio.Stream(mode=asyncio.StreamMode.READ,
                           limit=-1, loop=self.loop,
                           _asyncio_internal=True)

    def test_read_limit(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                limit=3, loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(b'chunk')
        data = self.loop.run_until_complete(stream.read(5))
        self.assertEqual(b'chunk', data)
        self.assertEqual(b'', stream._buffer)

    def test_readline(self):
        # Read one line. 'readline' will need to wait for the data
        # to come from 'cb'
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(b'chunk1 ')
        read_task = self.loop.create_task(stream.readline())

        def cb():
            stream._feed_data(b'chunk2 ')
            stream._feed_data(b'chunk3 ')
            stream._feed_data(b'\n chunk4')
        self.loop.call_soon(cb)

        line = self.loop.run_until_complete(read_task)
        self.assertEqual(b'chunk1 chunk2 chunk3 \n', line)
        self.assertEqual(b' chunk4', stream._buffer)

    def test_readline_limit_with_existing_data(self):
        # Read one line. The data is in Stream's buffer
        # before the event loop is run.

        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                limit=3, loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(b'li')
        stream._feed_data(b'ne1\nline2\n')

        self.assertRaises(
            ValueError, self.loop.run_until_complete, stream.readline())
        # The buffer should contain the remaining data after exception
        self.assertEqual(b'line2\n', stream._buffer)

        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                limit=3, loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(b'li')
        stream._feed_data(b'ne1')
        stream._feed_data(b'li')

        self.assertRaises(
            ValueError, self.loop.run_until_complete, stream.readline())
        # No b'\n' at the end. The 'limit' is set to 3. So before
        # waiting for the new data in buffer, 'readline' will consume
        # the entire buffer, and since the length of the consumed data
        # is more than 3, it will raise a ValueError. The buffer is
        # expected to be empty now.
        self.assertEqual(b'', stream._buffer)

    def test_at_eof(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        self.assertFalse(stream.at_eof())

        stream._feed_data(b'some data\n')
        self.assertFalse(stream.at_eof())

        self.loop.run_until_complete(stream.readline())
        self.assertFalse(stream.at_eof())

        stream._feed_data(b'some data\n')
        stream._feed_eof()
        self.loop.run_until_complete(stream.readline())
        self.assertTrue(stream.at_eof())

    def test_readline_limit(self):
        # Read one line. Streams are fed with data after
        # their 'readline' methods are called.

        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                limit=7, loop=self.loop,
                                _asyncio_internal=True)
        def cb():
            stream._feed_data(b'chunk1')
            stream._feed_data(b'chunk2')
            stream._feed_data(b'chunk3\n')
            stream._feed_eof()
        self.loop.call_soon(cb)

        self.assertRaises(
            ValueError, self.loop.run_until_complete, stream.readline())
        # The buffer had just one line of data, and after raising
        # a ValueError it should be empty.
        self.assertEqual(b'', stream._buffer)

        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                limit=7, loop=self.loop,
                                _asyncio_internal=True)
        def cb():
            stream._feed_data(b'chunk1')
            stream._feed_data(b'chunk2\n')
            stream._feed_data(b'chunk3\n')
            stream._feed_eof()
        self.loop.call_soon(cb)

        self.assertRaises(
            ValueError, self.loop.run_until_complete, stream.readline())
        self.assertEqual(b'chunk3\n', stream._buffer)

        # check strictness of the limit
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                limit=7, loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(b'1234567\n')
        line = self.loop.run_until_complete(stream.readline())
        self.assertEqual(b'1234567\n', line)
        self.assertEqual(b'', stream._buffer)

        stream._feed_data(b'12345678\n')
        with self.assertRaises(ValueError) as cm:
            self.loop.run_until_complete(stream.readline())
        self.assertEqual(b'', stream._buffer)

        stream._feed_data(b'12345678')
        with self.assertRaises(ValueError) as cm:
            self.loop.run_until_complete(stream.readline())
        self.assertEqual(b'', stream._buffer)

    def test_readline_nolimit_nowait(self):
        # All needed data for the first 'readline' call will be
        # in the buffer.
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(self.DATA[:6])
        stream._feed_data(self.DATA[6:])

        line = self.loop.run_until_complete(stream.readline())

        self.assertEqual(b'line1\n', line)
        self.assertEqual(b'line2\nline3\n', stream._buffer)

    def test_readline_eof(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(b'some data')
        stream._feed_eof()

        line = self.loop.run_until_complete(stream.readline())
        self.assertEqual(b'some data', line)

    def test_readline_empty_eof(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_eof()

        line = self.loop.run_until_complete(stream.readline())
        self.assertEqual(b'', line)

    def test_readline_read_byte_count(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(self.DATA)

        self.loop.run_until_complete(stream.readline())

        data = self.loop.run_until_complete(stream.read(7))

        self.assertEqual(b'line2\nl', data)
        self.assertEqual(b'ine3\n', stream._buffer)

    def test_readline_exception(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(b'line\n')

        data = self.loop.run_until_complete(stream.readline())
        self.assertEqual(b'line\n', data)

        stream._set_exception(ValueError())
        self.assertRaises(
            ValueError, self.loop.run_until_complete, stream.readline())
        self.assertEqual(b'', stream._buffer)

    def test_readuntil_separator(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        with self.assertRaisesRegex(ValueError, 'Separator should be'):
            self.loop.run_until_complete(stream.readuntil(separator=b''))

    def test_readuntil_multi_chunks(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)

        stream._feed_data(b'lineAAA')
        data = self.loop.run_until_complete(stream.readuntil(separator=b'AAA'))
        self.assertEqual(b'lineAAA', data)
        self.assertEqual(b'', stream._buffer)

        stream._feed_data(b'lineAAA')
        data = self.loop.run_until_complete(stream.readuntil(b'AAA'))
        self.assertEqual(b'lineAAA', data)
        self.assertEqual(b'', stream._buffer)

        stream._feed_data(b'lineAAAxxx')
        data = self.loop.run_until_complete(stream.readuntil(b'AAA'))
        self.assertEqual(b'lineAAA', data)
        self.assertEqual(b'xxx', stream._buffer)

    def test_readuntil_multi_chunks_1(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)

        stream._feed_data(b'QWEaa')
        stream._feed_data(b'XYaa')
        stream._feed_data(b'a')
        data = self.loop.run_until_complete(stream.readuntil(b'aaa'))
        self.assertEqual(b'QWEaaXYaaa', data)
        self.assertEqual(b'', stream._buffer)

        stream._feed_data(b'QWEaa')
        stream._feed_data(b'XYa')
        stream._feed_data(b'aa')
        data = self.loop.run_until_complete(stream.readuntil(b'aaa'))
        self.assertEqual(b'QWEaaXYaaa', data)
        self.assertEqual(b'', stream._buffer)

        stream._feed_data(b'aaa')
        data = self.loop.run_until_complete(stream.readuntil(b'aaa'))
        self.assertEqual(b'aaa', data)
        self.assertEqual(b'', stream._buffer)

        stream._feed_data(b'Xaaa')
        data = self.loop.run_until_complete(stream.readuntil(b'aaa'))
        self.assertEqual(b'Xaaa', data)
        self.assertEqual(b'', stream._buffer)

        stream._feed_data(b'XXX')
        stream._feed_data(b'a')
        stream._feed_data(b'a')
        stream._feed_data(b'a')
        data = self.loop.run_until_complete(stream.readuntil(b'aaa'))
        self.assertEqual(b'XXXaaa', data)
        self.assertEqual(b'', stream._buffer)

    def test_readuntil_eof(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(b'some dataAA')
        stream._feed_eof()

        with self.assertRaises(asyncio.IncompleteReadError) as cm:
            self.loop.run_until_complete(stream.readuntil(b'AAA'))
        self.assertEqual(cm.exception.partial, b'some dataAA')
        self.assertIsNone(cm.exception.expected)
        self.assertEqual(b'', stream._buffer)

    def test_readuntil_limit_found_sep(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop, limit=3,
                                _asyncio_internal=True)
        stream._feed_data(b'some dataAA')

        with self.assertRaisesRegex(asyncio.LimitOverrunError,
                                    'not found') as cm:
            self.loop.run_until_complete(stream.readuntil(b'AAA'))

        self.assertEqual(b'some dataAA', stream._buffer)

        stream._feed_data(b'A')
        with self.assertRaisesRegex(asyncio.LimitOverrunError,
                                    'is found') as cm:
            self.loop.run_until_complete(stream.readuntil(b'AAA'))

        self.assertEqual(b'some dataAAA', stream._buffer)

    def test_readexactly_zero_or_less(self):
        # Read exact number of bytes (zero or less).
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(self.DATA)

        data = self.loop.run_until_complete(stream.readexactly(0))
        self.assertEqual(b'', data)
        self.assertEqual(self.DATA, stream._buffer)

        with self.assertRaisesRegex(ValueError, 'less than zero'):
            self.loop.run_until_complete(stream.readexactly(-1))
        self.assertEqual(self.DATA, stream._buffer)

    def test_readexactly(self):
        # Read exact number of bytes.
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)

        n = 2 * len(self.DATA)
        read_task = self.loop.create_task(stream.readexactly(n))

        def cb():
            stream._feed_data(self.DATA)
            stream._feed_data(self.DATA)
            stream._feed_data(self.DATA)
        self.loop.call_soon(cb)

        data = self.loop.run_until_complete(read_task)
        self.assertEqual(self.DATA + self.DATA, data)
        self.assertEqual(self.DATA, stream._buffer)

    def test_readexactly_limit(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                limit=3, loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(b'chunk')
        data = self.loop.run_until_complete(stream.readexactly(5))
        self.assertEqual(b'chunk', data)
        self.assertEqual(b'', stream._buffer)

    def test_readexactly_eof(self):
        # Read exact number of bytes (eof).
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        n = 2 * len(self.DATA)
        read_task = self.loop.create_task(stream.readexactly(n))

        def cb():
            stream._feed_data(self.DATA)
            stream._feed_eof()
        self.loop.call_soon(cb)

        with self.assertRaises(asyncio.IncompleteReadError) as cm:
            self.loop.run_until_complete(read_task)
        self.assertEqual(cm.exception.partial, self.DATA)
        self.assertEqual(cm.exception.expected, n)
        self.assertEqual(str(cm.exception),
                         '18 bytes read on a total of 36 expected bytes')
        self.assertEqual(b'', stream._buffer)

    def test_readexactly_exception(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(b'line\n')

        data = self.loop.run_until_complete(stream.readexactly(2))
        self.assertEqual(b'li', data)

        stream._set_exception(ValueError())
        self.assertRaises(
            ValueError, self.loop.run_until_complete, stream.readexactly(2))

    def test_exception(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        self.assertIsNone(stream.exception())

        exc = ValueError()
        stream._set_exception(exc)
        self.assertIs(stream.exception(), exc)

    def test_exception_waiter(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)

        async def set_err():
            stream._set_exception(ValueError())

        t1 = self.loop.create_task(stream.readline())
        t2 = self.loop.create_task(set_err())

        self.loop.run_until_complete(asyncio.wait([t1, t2]))

        self.assertRaises(ValueError, t1.result)

    def test_exception_cancel(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)

        t = self.loop.create_task(stream.readline())
        test_utils.run_briefly(self.loop)
        t.cancel()
        test_utils.run_briefly(self.loop)
        # The following line fails if set_exception() isn't careful.
        stream._set_exception(RuntimeError('message'))
        test_utils.run_briefly(self.loop)
        self.assertIs(stream._waiter, None)

    def test_start_server(self):

        class MyServer:

            def __init__(self, loop):
                self.server = None
                self.loop = loop

            async def handle_client(self, client_reader, client_writer):
                data = await client_reader.readline()
                client_writer.write(data)
                await client_writer.drain()
                client_writer.close()
                await client_writer.wait_closed()

            def start(self):
                sock = socket.create_server(('127.0.0.1', 0))
                self.server = self.loop.run_until_complete(
                    asyncio.start_server(self.handle_client,
                                         sock=sock,
                                         loop=self.loop))
                return sock.getsockname()

            def handle_client_callback(self, client_reader, client_writer):
                self.loop.create_task(self.handle_client(client_reader,
                                                         client_writer))

            def start_callback(self):
                sock = socket.create_server(('127.0.0.1', 0))
                addr = sock.getsockname()
                sock.close()
                self.server = self.loop.run_until_complete(
                    asyncio.start_server(self.handle_client_callback,
                                         host=addr[0], port=addr[1],
                                         loop=self.loop))
                return addr

            def stop(self):
                if self.server is not None:
                    self.server.close()
                    self.loop.run_until_complete(self.server.wait_closed())
                    self.server = None

        async def client(addr):
            with self.assertWarns(DeprecationWarning):
                reader, writer = await asyncio.open_connection(
                    *addr, loop=self.loop)
            # send a line
            writer.write(b"hello world!\n")
            # read it back
            msgback = await reader.readline()
            writer.close()
            await writer.wait_closed()
            return msgback

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))

        # test the server variant with a coroutine as client handler
        server = MyServer(self.loop)
        with self.assertWarns(DeprecationWarning):
            addr = server.start()
        msg = self.loop.run_until_complete(self.loop.create_task(client(addr)))
        server.stop()
        self.assertEqual(msg, b"hello world!\n")

        # test the server variant with a callback as client handler
        server = MyServer(self.loop)
        with self.assertWarns(DeprecationWarning):
            addr = server.start_callback()
        msg = self.loop.run_until_complete(self.loop.create_task(client(addr)))
        server.stop()
        self.assertEqual(msg, b"hello world!\n")

        self.assertEqual(messages, [])

    @support.skip_unless_bind_unix_socket
    def test_start_unix_server(self):

        class MyServer:

            def __init__(self, loop, path):
                self.server = None
                self.loop = loop
                self.path = path

            async def handle_client(self, client_reader, client_writer):
                data = await client_reader.readline()
                client_writer.write(data)
                await client_writer.drain()
                client_writer.close()
                await client_writer.wait_closed()

            def start(self):
                self.server = self.loop.run_until_complete(
                    asyncio.start_unix_server(self.handle_client,
                                              path=self.path,
                                              loop=self.loop))

            def handle_client_callback(self, client_reader, client_writer):
                self.loop.create_task(self.handle_client(client_reader,
                                                         client_writer))

            def start_callback(self):
                start = asyncio.start_unix_server(self.handle_client_callback,
                                                  path=self.path,
                                                  loop=self.loop)
                self.server = self.loop.run_until_complete(start)

            def stop(self):
                if self.server is not None:
                    self.server.close()
                    self.loop.run_until_complete(self.server.wait_closed())
                    self.server = None

        async def client(path):
            with self.assertWarns(DeprecationWarning):
                reader, writer = await asyncio.open_unix_connection(
                    path, loop=self.loop)
            # send a line
            writer.write(b"hello world!\n")
            # read it back
            msgback = await reader.readline()
            writer.close()
            await writer.wait_closed()
            return msgback

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))

        # test the server variant with a coroutine as client handler
        with test_utils.unix_socket_path() as path:
            server = MyServer(self.loop, path)
            with self.assertWarns(DeprecationWarning):
                server.start()
            msg = self.loop.run_until_complete(
                self.loop.create_task(client(path)))
            server.stop()
            self.assertEqual(msg, b"hello world!\n")

        # test the server variant with a callback as client handler
        with test_utils.unix_socket_path() as path:
            server = MyServer(self.loop, path)
            with self.assertWarns(DeprecationWarning):
                server.start_callback()
            msg = self.loop.run_until_complete(
                self.loop.create_task(client(path)))
            server.stop()
            self.assertEqual(msg, b"hello world!\n")

        self.assertEqual(messages, [])

    @unittest.skipIf(sys.platform == 'win32', "Don't have pipes")
    def test_read_all_from_pipe_reader(self):
        # See asyncio issue 168.  This test is derived from the example
        # subprocess_attach_read_pipe.py, but we configure the
        # Stream's limit so that twice it is less than the size
        # of the data writter.  Also we must explicitly attach a child
        # watcher to the event loop.

        code = """\
import os, sys
fd = int(sys.argv[1])
os.write(fd, b'data')
os.close(fd)
"""
        rfd, wfd = os.pipe()
        args = [sys.executable, '-c', code, str(wfd)]

        pipe = open(rfd, 'rb', 0)
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop, limit=1,
                                _asyncio_internal=True)
        protocol = _StreamProtocol(stream, loop=self.loop,
                                   _asyncio_internal=True)
        transport, _ = self.loop.run_until_complete(
            self.loop.connect_read_pipe(lambda: protocol, pipe))

        watcher = asyncio.SafeChildWatcher()
        watcher.attach_loop(self.loop)
        try:
            asyncio.set_child_watcher(watcher)
            create = asyncio.create_subprocess_exec(*args,
                                                    pass_fds={wfd},
                                                    loop=self.loop)
            proc = self.loop.run_until_complete(create)
            self.loop.run_until_complete(proc.wait())
        finally:
            asyncio.set_child_watcher(None)

        os.close(wfd)
        data = self.loop.run_until_complete(stream.read(-1))
        self.assertEqual(data, b'data')

    def test_streamreader_constructor(self):
        self.addCleanup(asyncio.set_event_loop, None)
        asyncio.set_event_loop(self.loop)

        # asyncio issue #184: Ensure that _StreamProtocol constructor
        # retrieves the current loop if the loop parameter is not set
        reader = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                _asyncio_internal=True)
        self.assertIs(reader._loop, self.loop)

    def test_streamreaderprotocol_constructor(self):
        self.addCleanup(asyncio.set_event_loop, None)
        asyncio.set_event_loop(self.loop)

        # asyncio issue #184: Ensure that _StreamProtocol constructor
        # retrieves the current loop if the loop parameter is not set
        stream = mock.Mock()
        protocol = _StreamProtocol(stream, _asyncio_internal=True)
        self.assertIs(protocol._loop, self.loop)

    def test_drain_raises_deprecated(self):
        # See http://bugs.python.org/issue25441

        # This test should not use asyncio for the mock server; the
        # whole point of the test is to test for a bug in drain()
        # where it never gives up the event loop but the socket is
        # closed on the  server side.

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        q = queue.Queue()

        def server():
            # Runs in a separate thread.
            with socket.create_server(('127.0.0.1', 0)) as sock:
                addr = sock.getsockname()
                q.put(addr)
                clt, _ = sock.accept()
                clt.close()

        async def client(host, port):
            with self.assertWarns(DeprecationWarning):
                reader, writer = await asyncio.open_connection(
                    host, port, loop=self.loop)

            while True:
                writer.write(b"foo\n")
                await writer.drain()

        # Start the server thread and wait for it to be listening.
        thread = threading.Thread(target=server)
        thread.setDaemon(True)
        thread.start()
        addr = q.get()

        # Should not be stuck in an infinite loop.
        with self.assertRaises((ConnectionResetError, ConnectionAbortedError,
                                BrokenPipeError)):
            self.loop.run_until_complete(client(*addr))

        # Clean up the thread.  (Only on success; on failure, it may
        # be stuck in accept().)
        thread.join()
        self.assertEqual([], messages)

    def test_drain_raises(self):
        # See http://bugs.python.org/issue25441

        # This test should not use asyncio for the mock server; the
        # whole point of the test is to test for a bug in drain()
        # where it never gives up the event loop but the socket is
        # closed on the  server side.

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        q = queue.Queue()

        def server():
            # Runs in a separate thread.
            with socket.create_server(('localhost', 0)) as sock:
                addr = sock.getsockname()
                q.put(addr)
                clt, _ = sock.accept()
                clt.close()

        async def client(host, port):
            stream = await asyncio.connect(host, port)

            while True:
                stream.write(b"foo\n")
                await stream.drain()

        # Start the server thread and wait for it to be listening.
        thread = threading.Thread(target=server)
        thread.setDaemon(True)
        thread.start()
        addr = q.get()

        # Should not be stuck in an infinite loop.
        with self.assertRaises((ConnectionResetError, ConnectionAbortedError,
                                BrokenPipeError)):
            self.loop.run_until_complete(client(*addr))

        # Clean up the thread.  (Only on success; on failure, it may
        # be stuck in accept().)
        thread.join()
        self.assertEqual([], messages)

    def test___repr__(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        self.assertEqual("<Stream mode=StreamMode.READ>", repr(stream))

    def test___repr__nondefault_limit(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop, limit=123,
                                _asyncio_internal=True)
        self.assertEqual("<Stream mode=StreamMode.READ limit=123>", repr(stream))

    def test___repr__eof(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_eof()
        self.assertEqual("<Stream mode=StreamMode.READ eof>", repr(stream))

    def test___repr__data(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._feed_data(b'data')
        self.assertEqual("<Stream mode=StreamMode.READ 4 bytes>", repr(stream))

    def test___repr__exception(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        exc = RuntimeError()
        stream._set_exception(exc)
        self.assertEqual("<Stream mode=StreamMode.READ exception=RuntimeError()>",
                         repr(stream))

    def test___repr__waiter(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._waiter = self.loop.create_future()
        self.assertRegex(
            repr(stream),
            r"<Stream .+ waiter=<Future pending[\S ]*>>")
        stream._waiter.set_result(None)
        self.loop.run_until_complete(stream._waiter)
        stream._waiter = None
        self.assertEqual("<Stream mode=StreamMode.READ>", repr(stream))

    def test___repr__transport(self):
        stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                loop=self.loop,
                                _asyncio_internal=True)
        stream._transport = mock.Mock()
        stream._transport.__repr__ = mock.Mock()
        stream._transport.__repr__.return_value = "<Transport>"
        self.assertEqual("<Stream mode=StreamMode.READ transport=<Transport>>",
                         repr(stream))

    def test_IncompleteReadError_pickleable(self):
        e = asyncio.IncompleteReadError(b'abc', 10)
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(pickle_protocol=proto):
                e2 = pickle.loads(pickle.dumps(e, protocol=proto))
                self.assertEqual(str(e), str(e2))
                self.assertEqual(e.partial, e2.partial)
                self.assertEqual(e.expected, e2.expected)

    def test_LimitOverrunError_pickleable(self):
        e = asyncio.LimitOverrunError('message', 10)
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(pickle_protocol=proto):
                e2 = pickle.loads(pickle.dumps(e, protocol=proto))
                self.assertEqual(str(e), str(e2))
                self.assertEqual(e.consumed, e2.consumed)

    def test_wait_closed_on_close_deprecated(self):
        with test_utils.run_test_server() as httpd:
            with self.assertWarns(DeprecationWarning):
                rd, wr = self.loop.run_until_complete(
                    asyncio.open_connection(*httpd.address, loop=self.loop))

            wr.write(b'GET / HTTP/1.0\r\n\r\n')
            f = rd.readline()
            data = self.loop.run_until_complete(f)
            self.assertEqual(data, b'HTTP/1.0 200 OK\r\n')
            f = rd.read()
            data = self.loop.run_until_complete(f)
            self.assertTrue(data.endswith(b'\r\n\r\nTest message'))
            self.assertFalse(wr.is_closing())
            wr.close()
            self.assertTrue(wr.is_closing())
            self.loop.run_until_complete(wr.wait_closed())

    def test_wait_closed_on_close(self):
        with test_utils.run_test_server() as httpd:
            stream = self.loop.run_until_complete(
                asyncio.connect(*httpd.address))

            stream.write(b'GET / HTTP/1.0\r\n\r\n')
            f = stream.readline()
            data = self.loop.run_until_complete(f)
            self.assertEqual(data, b'HTTP/1.0 200 OK\r\n')
            f = stream.read()
            data = self.loop.run_until_complete(f)
            self.assertTrue(data.endswith(b'\r\n\r\nTest message'))
            self.assertFalse(stream.is_closing())
            stream.close()
            self.assertTrue(stream.is_closing())
            self.loop.run_until_complete(stream.wait_closed())

    def test_wait_closed_on_close_with_unread_data_deprecated(self):
        with test_utils.run_test_server() as httpd:
            with self.assertWarns(DeprecationWarning):
                rd, wr = self.loop.run_until_complete(
                    asyncio.open_connection(*httpd.address, loop=self.loop))

            wr.write(b'GET / HTTP/1.0\r\n\r\n')
            f = rd.readline()
            data = self.loop.run_until_complete(f)
            self.assertEqual(data, b'HTTP/1.0 200 OK\r\n')
            wr.close()
            self.loop.run_until_complete(wr.wait_closed())

    def test_wait_closed_on_close_with_unread_data(self):
        with test_utils.run_test_server() as httpd:
            stream = self.loop.run_until_complete(
                asyncio.connect(*httpd.address))

            stream.write(b'GET / HTTP/1.0\r\n\r\n')
            f = stream.readline()
            data = self.loop.run_until_complete(f)
            self.assertEqual(data, b'HTTP/1.0 200 OK\r\n')
            stream.close()
            self.loop.run_until_complete(stream.wait_closed())

    def test_del_stream_before_sock_closing(self):
        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))

        async def test():

            with test_utils.run_test_server() as httpd:
                stream = await asyncio.connect(*httpd.address)
                sock = stream.get_extra_info('socket')
                self.assertNotEqual(sock.fileno(), -1)

                await stream.write(b'GET / HTTP/1.0\r\n\r\n')
                data = await stream.readline()
                self.assertEqual(data, b'HTTP/1.0 200 OK\r\n')

                # drop refs to reader/writer
                del stream
                gc.collect()
                # make a chance to close the socket
                await asyncio.sleep(0)

                self.assertEqual(1, len(messages), messages)
                self.assertEqual(sock.fileno(), -1)

        self.loop.run_until_complete(test())
        self.assertEqual(1, len(messages), messages)
        self.assertEqual('An open stream object is being garbage '
                         'collected; call "stream.close()" explicitly.',
                         messages[0]['message'])

    def test_del_stream_before_connection_made(self):
        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))

        with test_utils.run_test_server() as httpd:
            stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                    loop=self.loop,
                                    _asyncio_internal=True)
            pr = _StreamProtocol(stream, loop=self.loop,
                                 _asyncio_internal=True)
            del stream
            gc.collect()
            tr, _ = self.loop.run_until_complete(
                self.loop.create_connection(
                    lambda: pr, *httpd.address))

            sock = tr.get_extra_info('socket')
            self.assertEqual(sock.fileno(), -1)

        self.assertEqual(1, len(messages))
        self.assertEqual('An open stream was garbage collected prior to '
                         'establishing network connection; '
                         'call "stream.close()" explicitly.',
                         messages[0]['message'])

    def test_async_writer_api(self):
        async def inner(httpd):
            stream = await asyncio.connect(*httpd.address)

            await stream.write(b'GET / HTTP/1.0\r\n\r\n')
            data = await stream.readline()
            self.assertEqual(data, b'HTTP/1.0 200 OK\r\n')
            data = await stream.read()
            self.assertTrue(data.endswith(b'\r\n\r\nTest message'))
            await stream.close()

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))

        with test_utils.run_test_server() as httpd:
            self.loop.run_until_complete(inner(httpd))

        self.assertEqual(messages, [])

    def test_async_writer_api_exception_after_close(self):
        async def inner(httpd):
            stream = await asyncio.connect(*httpd.address)

            await stream.write(b'GET / HTTP/1.0\r\n\r\n')
            data = await stream.readline()
            self.assertEqual(data, b'HTTP/1.0 200 OK\r\n')
            data = await stream.read()
            self.assertTrue(data.endswith(b'\r\n\r\nTest message'))
            stream.close()
            with self.assertRaises(ConnectionResetError):
                await stream.write(b'data')

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))

        with test_utils.run_test_server() as httpd:
            self.loop.run_until_complete(inner(httpd))

        self.assertEqual(messages, [])

    def test_eof_feed_when_closing_writer(self):
        # See http://bugs.python.org/issue35065
        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))

        with test_utils.run_test_server() as httpd:
            with self.assertWarns(DeprecationWarning):
                rd, wr = self.loop.run_until_complete(
                    asyncio.open_connection(*httpd.address,
                                            loop=self.loop))

            wr.close()
            f = wr.wait_closed()
            self.loop.run_until_complete(f)
            assert rd.at_eof()
            f = rd.read()
            data = self.loop.run_until_complete(f)
            assert data == b''

        self.assertEqual(messages, [])

    def test_stream_reader_create_warning(self):
        with contextlib.suppress(AttributeError):
            del asyncio.StreamReader
        with self.assertWarns(DeprecationWarning):
            asyncio.StreamReader

    def test_stream_writer_create_warning(self):
        with contextlib.suppress(AttributeError):
            del asyncio.StreamWriter
        with self.assertWarns(DeprecationWarning):
            asyncio.StreamWriter

    def test_stream_reader_forbidden_ops(self):
        async def inner():
            stream = asyncio.Stream(mode=asyncio.StreamMode.READ,
                                    _asyncio_internal=True)
            with self.assertRaisesRegex(RuntimeError, "The stream is read-only"):
                await stream.write(b'data')
            with self.assertRaisesRegex(RuntimeError, "The stream is read-only"):
                await stream.writelines([b'data', b'other'])
            with self.assertRaisesRegex(RuntimeError, "The stream is read-only"):
                stream.write_eof()
            with self.assertRaisesRegex(RuntimeError, "The stream is read-only"):
                await stream.drain()

        self.loop.run_until_complete(inner())

    def test_stream_writer_forbidden_ops(self):
        async def inner():
            stream = asyncio.Stream(mode=asyncio.StreamMode.WRITE,
                                    _asyncio_internal=True)
            with self.assertRaisesRegex(RuntimeError, "The stream is write-only"):
                stream._feed_data(b'data')
            with self.assertRaisesRegex(RuntimeError, "The stream is write-only"):
                await stream.readline()
            with self.assertRaisesRegex(RuntimeError, "The stream is write-only"):
                await stream.readuntil()
            with self.assertRaisesRegex(RuntimeError, "The stream is write-only"):
                await stream.read()
            with self.assertRaisesRegex(RuntimeError, "The stream is write-only"):
                await stream.readexactly(10)
            with self.assertRaisesRegex(RuntimeError, "The stream is write-only"):
                async for chunk in stream:
                    pass

        self.loop.run_until_complete(inner())

    def _basetest_connect(self, stream):
        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))

        stream.write(b'GET / HTTP/1.0\r\n\r\n')
        f = stream.readline()
        data = self.loop.run_until_complete(f)
        self.assertEqual(data, b'HTTP/1.0 200 OK\r\n')
        f = stream.read()
        data = self.loop.run_until_complete(f)
        self.assertTrue(data.endswith(b'\r\n\r\nTest message'))
        stream.close()
        self.loop.run_until_complete(stream.wait_closed())

        self.assertEqual([], messages)

    def test_connect(self):
        with test_utils.run_test_server() as httpd:
            stream = self.loop.run_until_complete(
                asyncio.connect(*httpd.address))
            self.assertFalse(stream.is_server_side())
            self._basetest_connect(stream)

    @support.skip_unless_bind_unix_socket
    def test_connect_unix(self):
        with test_utils.run_test_unix_server() as httpd:
            stream = self.loop.run_until_complete(
                asyncio.connect_unix(httpd.address))
            self._basetest_connect(stream)

    def test_stream_async_context_manager(self):
        async def test(httpd):
            stream = await asyncio.connect(*httpd.address)
            async with stream:
                await stream.write(b'GET / HTTP/1.0\r\n\r\n')
                data = await stream.readline()
                self.assertEqual(data, b'HTTP/1.0 200 OK\r\n')
                data = await stream.read()
                self.assertTrue(data.endswith(b'\r\n\r\nTest message'))
            self.assertTrue(stream.is_closing())

        with test_utils.run_test_server() as httpd:
            self.loop.run_until_complete(test(httpd))

    def test_connect_async_context_manager(self):
        async def test(httpd):
            async with asyncio.connect(*httpd.address) as stream:
                await stream.write(b'GET / HTTP/1.0\r\n\r\n')
                data = await stream.readline()
                self.assertEqual(data, b'HTTP/1.0 200 OK\r\n')
                data = await stream.read()
                self.assertTrue(data.endswith(b'\r\n\r\nTest message'))
            self.assertTrue(stream.is_closing())

        with test_utils.run_test_server() as httpd:
            self.loop.run_until_complete(test(httpd))

    @support.skip_unless_bind_unix_socket
    def test_connect_unix_async_context_manager(self):
        async def test(httpd):
            async with asyncio.connect_unix(httpd.address) as stream:
                await stream.write(b'GET / HTTP/1.0\r\n\r\n')
                data = await stream.readline()
                self.assertEqual(data, b'HTTP/1.0 200 OK\r\n')
                data = await stream.read()
                self.assertTrue(data.endswith(b'\r\n\r\nTest message'))
            self.assertTrue(stream.is_closing())

        with test_utils.run_test_unix_server() as httpd:
            self.loop.run_until_complete(test(httpd))

    def test_stream_server(self):

        async def handle_client(stream):
            self.assertTrue(stream.is_server_side())
            data = await stream.readline()
            await stream.write(data)
            await stream.close()

        async def client(srv):
            addr = srv.sockets[0].getsockname()
            stream = await asyncio.connect(*addr)
            # send a line
            await stream.write(b"hello world!\n")
            # read it back
            msgback = await stream.readline()
            await stream.close()
            self.assertEqual(msgback, b"hello world!\n")
            await srv.close()

        async def test():
            async with asyncio.StreamServer(handle_client, '127.0.0.1', 0) as server:
                await server.start_serving()
                task = asyncio.create_task(client(server))
                with contextlib.suppress(asyncio.CancelledError):
                    await server.serve_forever()
                await task

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        self.loop.run_until_complete(test())
        self.assertEqual(messages, [])

    @support.skip_unless_bind_unix_socket
    def test_unix_stream_server(self):

        async def handle_client(stream):
            data = await stream.readline()
            await stream.write(data)
            await stream.close()

        async def client(srv):
            addr = srv.sockets[0].getsockname()
            stream = await asyncio.connect_unix(addr)
            # send a line
            await stream.write(b"hello world!\n")
            # read it back
            msgback = await stream.readline()
            await stream.close()
            self.assertEqual(msgback, b"hello world!\n")
            await srv.close()

        async def test():
            with test_utils.unix_socket_path() as path:
                async with asyncio.UnixStreamServer(handle_client, path) as server:
                    await server.start_serving()
                    task = asyncio.create_task(client(server))
                    with contextlib.suppress(asyncio.CancelledError):
                        await server.serve_forever()
                    await task

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        self.loop.run_until_complete(test())
        self.assertEqual(messages, [])

    def test_stream_server_inheritance_forbidden(self):
        with self.assertRaises(TypeError):
            class MyServer(asyncio.StreamServer):
                pass

    @support.skip_unless_bind_unix_socket
    def test_unix_stream_server_inheritance_forbidden(self):
        with self.assertRaises(TypeError):
            class MyServer(asyncio.UnixStreamServer):
                pass

    def test_stream_server_bind(self):
        async def handle_client(stream):
            await stream.close()

        async def test():
            srv = asyncio.StreamServer(handle_client, '127.0.0.1', 0)
            self.assertFalse(srv.is_bound())
            self.assertEqual(0, len(srv.sockets))
            await srv.bind()
            self.assertTrue(srv.is_bound())
            self.assertEqual(1, len(srv.sockets))
            await srv.close()
            self.assertFalse(srv.is_bound())
            self.assertEqual(0, len(srv.sockets))

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        self.loop.run_until_complete(test())
        self.assertEqual(messages, [])

    def test_stream_server_bind_async_with(self):
        async def handle_client(stream):
            await stream.close()

        async def test():
            async with asyncio.StreamServer(handle_client, '127.0.0.1', 0) as srv:
                self.assertTrue(srv.is_bound())
                self.assertEqual(1, len(srv.sockets))

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        self.loop.run_until_complete(test())
        self.assertEqual(messages, [])

    def test_stream_server_start_serving(self):
        async def handle_client(stream):
            await stream.close()

        async def test():
            async with asyncio.StreamServer(handle_client, '127.0.0.1', 0) as srv:
                self.assertFalse(srv.is_serving())
                await srv.start_serving()
                self.assertTrue(srv.is_serving())
                await srv.close()
                self.assertFalse(srv.is_serving())

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        self.loop.run_until_complete(test())
        self.assertEqual(messages, [])

    def test_stream_server_close(self):
        server_stream_aborted = False
        fut1 = self.loop.create_future()
        fut2 = self.loop.create_future()

        async def handle_client(stream):
            data = await stream.readexactly(4)
            self.assertEqual(b'data', data)
            fut1.set_result(None)
            await fut2
            self.assertEqual(b'', await stream.readline())
            nonlocal server_stream_aborted
            server_stream_aborted = True

        async def client(srv):
            addr = srv.sockets[0].getsockname()
            stream = await asyncio.connect(*addr)
            await stream.write(b'data')
            await fut2
            self.assertEqual(b'', await stream.readline())
            await stream.close()

        async def test():
            async with asyncio.StreamServer(handle_client, '127.0.0.1', 0) as server:
                await server.start_serving()
                task = asyncio.create_task(client(server))
                await fut1
                fut2.set_result(None)
                await server.close()
                await task

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        self.loop.run_until_complete(asyncio.wait_for(test(), 60.0))
        self.assertEqual(messages, [])
        self.assertTrue(fut1.done())
        self.assertTrue(fut2.done())
        self.assertTrue(server_stream_aborted)

    def test_stream_server_abort(self):
        server_stream_aborted = False
        fut1 = self.loop.create_future()
        fut2 = self.loop.create_future()

        async def handle_client(stream):
            data = await stream.readexactly(4)
            self.assertEqual(b'data', data)
            fut1.set_result(None)
            await fut2
            self.assertEqual(b'', await stream.readline())
            nonlocal server_stream_aborted
            server_stream_aborted = True

        async def client(srv):
            addr = srv.sockets[0].getsockname()
            stream = await asyncio.connect(*addr)
            await stream.write(b'data')
            await fut2
            self.assertEqual(b'', await stream.readline())
            await stream.close()

        async def test():
            async with asyncio.StreamServer(handle_client, '127.0.0.1', 0) as server:
                await server.start_serving()
                task = asyncio.create_task(client(server))
                await fut1
                fut2.set_result(None)
                await server.abort()
                await task

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        self.loop.run_until_complete(asyncio.wait_for(test(), 60.0))
        self.assertEqual(messages, [])
        self.assertTrue(fut1.done())
        self.assertTrue(fut2.done())
        self.assertTrue(server_stream_aborted)

    def test_stream_shutdown_hung_task(self):
        fut1 = self.loop.create_future()
        fut2 = self.loop.create_future()
        cancelled = self.loop.create_future()

        async def handle_client(stream):
            data = await stream.readexactly(4)
            self.assertEqual(b'data', data)
            fut1.set_result(None)
            await fut2
            try:
                while True:
                    await asyncio.sleep(0.01)
            except asyncio.CancelledError:
                cancelled.set_result(None)
                raise

        async def client(srv):
            addr = srv.sockets[0].getsockname()
            stream = await asyncio.connect(*addr)
            await stream.write(b'data')
            await fut2
            self.assertEqual(b'', await stream.readline())
            await stream.close()

        async def test():
            async with asyncio.StreamServer(handle_client,
                                            '127.0.0.1',
                                            0,
                                            shutdown_timeout=0.3) as server:
                await server.start_serving()
                task = asyncio.create_task(client(server))
                await fut1
                fut2.set_result(None)
                await server.close()
                await task
                await cancelled

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        self.loop.run_until_complete(asyncio.wait_for(test(), 60.0))
        self.assertEqual(messages, [])
        self.assertTrue(fut1.done())
        self.assertTrue(fut2.done())
        self.assertTrue(cancelled.done())

    def test_stream_shutdown_hung_task_prevents_cancellation(self):
        fut1 = self.loop.create_future()
        fut2 = self.loop.create_future()
        cancelled = self.loop.create_future()
        do_handle_client = True

        async def handle_client(stream):
            data = await stream.readexactly(4)
            self.assertEqual(b'data', data)
            fut1.set_result(None)
            await fut2
            while do_handle_client:
                with contextlib.suppress(asyncio.CancelledError):
                    await asyncio.sleep(0.01)
            cancelled.set_result(None)

        async def client(srv):
            addr = srv.sockets[0].getsockname()
            stream = await asyncio.connect(*addr)
            await stream.write(b'data')
            await fut2
            self.assertEqual(b'', await stream.readline())
            await stream.close()

        async def test():
            async with asyncio.StreamServer(handle_client,
                                            '127.0.0.1',
                                            0,
                                            shutdown_timeout=0.3) as server:
                await server.start_serving()
                task = asyncio.create_task(client(server))
                await fut1
                fut2.set_result(None)
                await server.close()
                nonlocal do_handle_client
                do_handle_client = False
                await task
                await cancelled

        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))
        self.loop.run_until_complete(asyncio.wait_for(test(), 60.0))
        self.assertEqual(1, len(messages))
        self.assertRegex(messages[0]['message'],
                         "<Task pending .+ ignored cancellation request")
        self.assertTrue(fut1.done())
        self.assertTrue(fut2.done())
        self.assertTrue(cancelled.done())

    def test_sendfile(self):
        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))

        with open(support.TESTFN, 'wb') as fp:
            fp.write(b'data\n')
        self.addCleanup(support.unlink, support.TESTFN)

        async def serve_callback(stream):
            data = await stream.readline()
            await stream.write(b'ack-' + data)
            data = await stream.readline()
            await stream.write(b'ack-' + data)
            data = await stream.readline()
            await stream.write(b'ack-' + data)
            await stream.close()

        async def do_connect(host, port):
            stream = await asyncio.connect(host, port)
            await stream.write(b'begin\n')
            data = await stream.readline()
            self.assertEqual(b'ack-begin\n', data)
            with open(support.TESTFN, 'rb') as fp:
                await stream.sendfile(fp)
            data = await stream.readline()
            self.assertEqual(b'ack-data\n', data)
            await stream.write(b'end\n')
            data = await stream.readline()
            self.assertEqual(data, b'ack-end\n')
            await stream.close()

        async def test():
            async with asyncio.StreamServer(serve_callback, '127.0.0.1', 0) as srv:
                await srv.start_serving()
                await do_connect(*srv.sockets[0].getsockname())

        self.loop.run_until_complete(test())

        self.assertEqual([], messages)


    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_connect_start_tls(self):
        with test_utils.run_test_server(use_ssl=True) as httpd:
            # connect without SSL but upgrade to TLS just after
            # connection is established
            stream = self.loop.run_until_complete(
                asyncio.connect(*httpd.address))

            self.loop.run_until_complete(
                stream.start_tls(
                    sslcontext=test_utils.dummy_ssl_context()))
            self._basetest_connect(stream)

    def test_repr_unbound(self):
        async def serve(stream):
            pass

        async def test():
            srv = asyncio.StreamServer(serve)
            self.assertEqual('<StreamServer>', repr(srv))
            await srv.close()

        self.loop.run_until_complete(test())

    def test_repr_bound(self):
        async def serve(stream):
            pass

        async def test():
            srv = asyncio.StreamServer(serve, '127.0.0.1', 0)
            await srv.bind()
            self.assertRegex(repr(srv), r'<StreamServer sockets=\(.+\)>')
            await srv.close()

        self.loop.run_until_complete(test())

    def test_repr_serving(self):
        async def serve(stream):
            pass

        async def test():
            srv = asyncio.StreamServer(serve, '127.0.0.1', 0)
            await srv.start_serving()
            self.assertRegex(repr(srv), r'<StreamServer serving sockets=\(.+\)>')
            await srv.close()

        self.loop.run_until_complete(test())


    @unittest.skipUnless(sys.platform != 'win32',
                         "Don't support pipes for Windows")
    def test_read_pipe(self):
        async def test():
            rpipe, wpipe = os.pipe()
            pipeobj = io.open(rpipe, 'rb', 1024)

            async with asyncio.connect_read_pipe(pipeobj) as stream:
                self.assertEqual(stream.mode, asyncio.StreamMode.READ)

                os.write(wpipe, b'1')
                data = await stream.readexactly(1)
                self.assertEqual(data, b'1')

                os.write(wpipe, b'2345')
                data = await stream.readexactly(4)
                self.assertEqual(data, b'2345')
                os.close(wpipe)

        self.loop.run_until_complete(test())

    @unittest.skipUnless(sys.platform != 'win32',
                         "Don't support pipes for Windows")
    def test_write_pipe(self):
        async def test():
            rpipe, wpipe = os.pipe()
            pipeobj = io.open(wpipe, 'wb', 1024)

            async with asyncio.connect_write_pipe(pipeobj) as stream:
                self.assertEqual(stream.mode, asyncio.StreamMode.WRITE)

                await stream.write(b'1')
                data = os.read(rpipe, 1024)
                self.assertEqual(data, b'1')

                await stream.write(b'2345')
                data = os.read(rpipe, 1024)
                self.assertEqual(data, b'2345')

                os.close(rpipe)

        self.loop.run_until_complete(test())

    def test_stream_ctor_forbidden(self):
        with self.assertRaisesRegex(RuntimeError,
                                    "should be instantiated "
                                    "by asyncio internals only"):
            asyncio.Stream(asyncio.StreamMode.READWRITE)

    def test_deprecated_methods(self):
        async def f():
            return asyncio.Stream(mode=asyncio.StreamMode.READWRITE,
                                  _asyncio_internal=True)

        stream = self.loop.run_until_complete(f())

        tr = mock.Mock()

        with self.assertWarns(DeprecationWarning):
            stream.set_transport(tr)

        with self.assertWarns(DeprecationWarning):
            stream.transport is tr

        with self.assertWarns(DeprecationWarning):
            stream.feed_data(b'data')

        with self.assertWarns(DeprecationWarning):
            stream.feed_eof()

        with self.assertWarns(DeprecationWarning):
            stream.set_exception(ConnectionResetError("test"))


if __name__ == '__main__':
    unittest.main()
