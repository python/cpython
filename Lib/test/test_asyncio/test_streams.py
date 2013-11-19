"""Tests for streams.py."""

import gc
import unittest
import unittest.mock
try:
    import ssl
except ImportError:
    ssl = None

from asyncio import events
from asyncio import streams
from asyncio import tasks
from asyncio import test_utils


class StreamReaderTests(unittest.TestCase):

    DATA = b'line1\nline2\nline3\n'

    def setUp(self):
        self.loop = events.new_event_loop()
        events.set_event_loop(None)

    def tearDown(self):
        # just in case if we have transport close callbacks
        test_utils.run_briefly(self.loop)

        self.loop.close()
        gc.collect()

    @unittest.mock.patch('asyncio.streams.events')
    def test_ctor_global_loop(self, m_events):
        stream = streams.StreamReader()
        self.assertIs(stream._loop, m_events.get_event_loop.return_value)

    def test_open_connection(self):
        with test_utils.run_test_server() as httpd:
            f = streams.open_connection(*httpd.address, loop=self.loop)
            reader, writer = self.loop.run_until_complete(f)
            writer.write(b'GET / HTTP/1.0\r\n\r\n')
            f = reader.readline()
            data = self.loop.run_until_complete(f)
            self.assertEqual(data, b'HTTP/1.0 200 OK\r\n')
            f = reader.read()
            data = self.loop.run_until_complete(f)
            self.assertTrue(data.endswith(b'\r\n\r\nTest message'))

            writer.close()

    @unittest.skipIf(ssl is None, 'No ssl module')
    def test_open_connection_no_loop_ssl(self):
        with test_utils.run_test_server(use_ssl=True) as httpd:
            try:
                events.set_event_loop(self.loop)
                f = streams.open_connection(*httpd.address,
                                            ssl=test_utils.dummy_ssl_context())
                reader, writer = self.loop.run_until_complete(f)
            finally:
                events.set_event_loop(None)
            writer.write(b'GET / HTTP/1.0\r\n\r\n')
            f = reader.read()
            data = self.loop.run_until_complete(f)
            self.assertTrue(data.endswith(b'\r\n\r\nTest message'))

            writer.close()

    def test_open_connection_error(self):
        with test_utils.run_test_server() as httpd:
            f = streams.open_connection(*httpd.address, loop=self.loop)
            reader, writer = self.loop.run_until_complete(f)
            writer._protocol.connection_lost(ZeroDivisionError())
            f = reader.read()
            with self.assertRaises(ZeroDivisionError):
                self.loop.run_until_complete(f)

            writer.close()
            test_utils.run_briefly(self.loop)

    def test_feed_empty_data(self):
        stream = streams.StreamReader(loop=self.loop)

        stream.feed_data(b'')
        self.assertEqual(0, stream._byte_count)

    def test_feed_data_byte_count(self):
        stream = streams.StreamReader(loop=self.loop)

        stream.feed_data(self.DATA)
        self.assertEqual(len(self.DATA), stream._byte_count)

    def test_read_zero(self):
        # Read zero bytes.
        stream = streams.StreamReader(loop=self.loop)
        stream.feed_data(self.DATA)

        data = self.loop.run_until_complete(stream.read(0))
        self.assertEqual(b'', data)
        self.assertEqual(len(self.DATA), stream._byte_count)

    def test_read(self):
        # Read bytes.
        stream = streams.StreamReader(loop=self.loop)
        read_task = tasks.Task(stream.read(30), loop=self.loop)

        def cb():
            stream.feed_data(self.DATA)
        self.loop.call_soon(cb)

        data = self.loop.run_until_complete(read_task)
        self.assertEqual(self.DATA, data)
        self.assertFalse(stream._byte_count)

    def test_read_line_breaks(self):
        # Read bytes without line breaks.
        stream = streams.StreamReader(loop=self.loop)
        stream.feed_data(b'line1')
        stream.feed_data(b'line2')

        data = self.loop.run_until_complete(stream.read(5))

        self.assertEqual(b'line1', data)
        self.assertEqual(5, stream._byte_count)

    def test_read_eof(self):
        # Read bytes, stop at eof.
        stream = streams.StreamReader(loop=self.loop)
        read_task = tasks.Task(stream.read(1024), loop=self.loop)

        def cb():
            stream.feed_eof()
        self.loop.call_soon(cb)

        data = self.loop.run_until_complete(read_task)
        self.assertEqual(b'', data)
        self.assertFalse(stream._byte_count)

    def test_read_until_eof(self):
        # Read all bytes until eof.
        stream = streams.StreamReader(loop=self.loop)
        read_task = tasks.Task(stream.read(-1), loop=self.loop)

        def cb():
            stream.feed_data(b'chunk1\n')
            stream.feed_data(b'chunk2')
            stream.feed_eof()
        self.loop.call_soon(cb)

        data = self.loop.run_until_complete(read_task)

        self.assertEqual(b'chunk1\nchunk2', data)
        self.assertFalse(stream._byte_count)

    def test_read_exception(self):
        stream = streams.StreamReader(loop=self.loop)
        stream.feed_data(b'line\n')

        data = self.loop.run_until_complete(stream.read(2))
        self.assertEqual(b'li', data)

        stream.set_exception(ValueError())
        self.assertRaises(
            ValueError, self.loop.run_until_complete, stream.read(2))

    def test_readline(self):
        # Read one line.
        stream = streams.StreamReader(loop=self.loop)
        stream.feed_data(b'chunk1 ')
        read_task = tasks.Task(stream.readline(), loop=self.loop)

        def cb():
            stream.feed_data(b'chunk2 ')
            stream.feed_data(b'chunk3 ')
            stream.feed_data(b'\n chunk4')
        self.loop.call_soon(cb)

        line = self.loop.run_until_complete(read_task)
        self.assertEqual(b'chunk1 chunk2 chunk3 \n', line)
        self.assertEqual(len(b'\n chunk4')-1, stream._byte_count)

    def test_readline_limit_with_existing_data(self):
        stream = streams.StreamReader(3, loop=self.loop)
        stream.feed_data(b'li')
        stream.feed_data(b'ne1\nline2\n')

        self.assertRaises(
            ValueError, self.loop.run_until_complete, stream.readline())
        self.assertEqual([b'line2\n'], list(stream._buffer))

        stream = streams.StreamReader(3, loop=self.loop)
        stream.feed_data(b'li')
        stream.feed_data(b'ne1')
        stream.feed_data(b'li')

        self.assertRaises(
            ValueError, self.loop.run_until_complete, stream.readline())
        self.assertEqual([b'li'], list(stream._buffer))
        self.assertEqual(2, stream._byte_count)

    def test_readline_limit(self):
        stream = streams.StreamReader(7, loop=self.loop)

        def cb():
            stream.feed_data(b'chunk1')
            stream.feed_data(b'chunk2')
            stream.feed_data(b'chunk3\n')
            stream.feed_eof()
        self.loop.call_soon(cb)

        self.assertRaises(
            ValueError, self.loop.run_until_complete, stream.readline())
        self.assertEqual([b'chunk3\n'], list(stream._buffer))
        self.assertEqual(7, stream._byte_count)

    def test_readline_line_byte_count(self):
        stream = streams.StreamReader(loop=self.loop)
        stream.feed_data(self.DATA[:6])
        stream.feed_data(self.DATA[6:])

        line = self.loop.run_until_complete(stream.readline())

        self.assertEqual(b'line1\n', line)
        self.assertEqual(len(self.DATA) - len(b'line1\n'), stream._byte_count)

    def test_readline_eof(self):
        stream = streams.StreamReader(loop=self.loop)
        stream.feed_data(b'some data')
        stream.feed_eof()

        line = self.loop.run_until_complete(stream.readline())
        self.assertEqual(b'some data', line)

    def test_readline_empty_eof(self):
        stream = streams.StreamReader(loop=self.loop)
        stream.feed_eof()

        line = self.loop.run_until_complete(stream.readline())
        self.assertEqual(b'', line)

    def test_readline_read_byte_count(self):
        stream = streams.StreamReader(loop=self.loop)
        stream.feed_data(self.DATA)

        self.loop.run_until_complete(stream.readline())

        data = self.loop.run_until_complete(stream.read(7))

        self.assertEqual(b'line2\nl', data)
        self.assertEqual(
            len(self.DATA) - len(b'line1\n') - len(b'line2\nl'),
            stream._byte_count)

    def test_readline_exception(self):
        stream = streams.StreamReader(loop=self.loop)
        stream.feed_data(b'line\n')

        data = self.loop.run_until_complete(stream.readline())
        self.assertEqual(b'line\n', data)

        stream.set_exception(ValueError())
        self.assertRaises(
            ValueError, self.loop.run_until_complete, stream.readline())

    def test_readexactly_zero_or_less(self):
        # Read exact number of bytes (zero or less).
        stream = streams.StreamReader(loop=self.loop)
        stream.feed_data(self.DATA)

        data = self.loop.run_until_complete(stream.readexactly(0))
        self.assertEqual(b'', data)
        self.assertEqual(len(self.DATA), stream._byte_count)

        data = self.loop.run_until_complete(stream.readexactly(-1))
        self.assertEqual(b'', data)
        self.assertEqual(len(self.DATA), stream._byte_count)

    def test_readexactly(self):
        # Read exact number of bytes.
        stream = streams.StreamReader(loop=self.loop)

        n = 2 * len(self.DATA)
        read_task = tasks.Task(stream.readexactly(n), loop=self.loop)

        def cb():
            stream.feed_data(self.DATA)
            stream.feed_data(self.DATA)
            stream.feed_data(self.DATA)
        self.loop.call_soon(cb)

        data = self.loop.run_until_complete(read_task)
        self.assertEqual(self.DATA + self.DATA, data)
        self.assertEqual(len(self.DATA), stream._byte_count)

    def test_readexactly_eof(self):
        # Read exact number of bytes (eof).
        stream = streams.StreamReader(loop=self.loop)
        n = 2 * len(self.DATA)
        read_task = tasks.Task(stream.readexactly(n), loop=self.loop)

        def cb():
            stream.feed_data(self.DATA)
            stream.feed_eof()
        self.loop.call_soon(cb)

        data = self.loop.run_until_complete(read_task)
        self.assertEqual(self.DATA, data)
        self.assertFalse(stream._byte_count)

    def test_readexactly_exception(self):
        stream = streams.StreamReader(loop=self.loop)
        stream.feed_data(b'line\n')

        data = self.loop.run_until_complete(stream.readexactly(2))
        self.assertEqual(b'li', data)

        stream.set_exception(ValueError())
        self.assertRaises(
            ValueError, self.loop.run_until_complete, stream.readexactly(2))

    def test_exception(self):
        stream = streams.StreamReader(loop=self.loop)
        self.assertIsNone(stream.exception())

        exc = ValueError()
        stream.set_exception(exc)
        self.assertIs(stream.exception(), exc)

    def test_exception_waiter(self):
        stream = streams.StreamReader(loop=self.loop)

        @tasks.coroutine
        def set_err():
            stream.set_exception(ValueError())

        @tasks.coroutine
        def readline():
            yield from stream.readline()

        t1 = tasks.Task(stream.readline(), loop=self.loop)
        t2 = tasks.Task(set_err(), loop=self.loop)

        self.loop.run_until_complete(tasks.wait([t1, t2], loop=self.loop))

        self.assertRaises(ValueError, t1.result)

    def test_exception_cancel(self):
        stream = streams.StreamReader(loop=self.loop)

        @tasks.coroutine
        def read_a_line():
            yield from stream.readline()

        t = tasks.Task(read_a_line(), loop=self.loop)
        test_utils.run_briefly(self.loop)
        t.cancel()
        test_utils.run_briefly(self.loop)
        # The following line fails if set_exception() isn't careful.
        stream.set_exception(RuntimeError('message'))
        test_utils.run_briefly(self.loop)
        self.assertIs(stream._waiter, None)

    def test_start_server(self):

        class MyServer:

            def __init__(self, loop):
                self.server = None
                self.loop = loop

            @tasks.coroutine
            def handle_client(self, client_reader, client_writer):
                data = yield from client_reader.readline()
                client_writer.write(data)

            def start(self):
                self.server = self.loop.run_until_complete(
                    streams.start_server(self.handle_client,
                                         '127.0.0.1', 12345,
                                         loop=self.loop))

            def handle_client_callback(self, client_reader, client_writer):
                task = tasks.Task(client_reader.readline(), loop=self.loop)

                def done(task):
                    client_writer.write(task.result())

                task.add_done_callback(done)

            def start_callback(self):
                self.server = self.loop.run_until_complete(
                    streams.start_server(self.handle_client_callback,
                                         '127.0.0.1', 12345,
                                         loop=self.loop))

            def stop(self):
                if self.server is not None:
                    self.server.close()
                    self.loop.run_until_complete(self.server.wait_closed())
                    self.server = None

        @tasks.coroutine
        def client():
            reader, writer = yield from streams.open_connection(
                '127.0.0.1', 12345, loop=self.loop)
            # send a line
            writer.write(b"hello world!\n")
            # read it back
            msgback = yield from reader.readline()
            writer.close()
            return msgback

        # test the server variant with a coroutine as client handler
        server = MyServer(self.loop)
        server.start()
        msg = self.loop.run_until_complete(tasks.Task(client(),
                                                      loop=self.loop))
        server.stop()
        self.assertEqual(msg, b"hello world!\n")

        # test the server variant with a callback as client handler
        server = MyServer(self.loop)
        server.start_callback()
        msg = self.loop.run_until_complete(tasks.Task(client(),
                                                      loop=self.loop))
        server.stop()
        self.assertEqual(msg, b"hello world!\n")


if __name__ == '__main__':
    unittest.main()
