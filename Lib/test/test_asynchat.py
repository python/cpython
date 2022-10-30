# test asynchat

from test import support
from test.support import socket_helper
from test.support import threading_helper
from test.support import warnings_helper

import errno
import socket
import sys
import threading
import time
import unittest
import unittest.mock


asynchat = warnings_helper.import_deprecated('asynchat')
asyncore = warnings_helper.import_deprecated('asyncore')

support.requires_working_socket(module=True)

HOST = socket_helper.HOST
SERVER_QUIT = b'QUIT\n'


class echo_server(threading.Thread):
    # parameter to determine the number of bytes passed back to the
    # client each send
    chunk_size = 1

    def __init__(self, event):
        threading.Thread.__init__(self)
        self.event = event
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.port = socket_helper.bind_port(self.sock)
        # This will be set if the client wants us to wait before echoing
        # data back.
        self.start_resend_event = None

    def run(self):
        self.sock.listen()
        self.event.set()
        conn, client = self.sock.accept()
        self.buffer = b""
        # collect data until quit message is seen
        while SERVER_QUIT not in self.buffer:
            data = conn.recv(1)
            if not data:
                break
            self.buffer = self.buffer + data

        # remove the SERVER_QUIT message
        self.buffer = self.buffer.replace(SERVER_QUIT, b'')

        if self.start_resend_event:
            self.start_resend_event.wait()

        # re-send entire set of collected data
        try:
            # this may fail on some tests, such as test_close_when_done,
            # since the client closes the channel when it's done sending
            while self.buffer:
                n = conn.send(self.buffer[:self.chunk_size])
                time.sleep(0.001)
                self.buffer = self.buffer[n:]
        except:
            pass

        conn.close()
        self.sock.close()

class echo_client(asynchat.async_chat):

    def __init__(self, terminator, server_port):
        asynchat.async_chat.__init__(self)
        self.contents = []
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connect((HOST, server_port))
        self.set_terminator(terminator)
        self.buffer = b""

    def handle_connect(self):
        pass

    if sys.platform == 'darwin':
        # select.poll returns a select.POLLHUP at the end of the tests
        # on darwin, so just ignore it
        def handle_expt(self):
            pass

    def collect_incoming_data(self, data):
        self.buffer += data

    def found_terminator(self):
        self.contents.append(self.buffer)
        self.buffer = b""

def start_echo_server():
    event = threading.Event()
    s = echo_server(event)
    s.start()
    event.wait()
    event.clear()
    time.sleep(0.01)   # Give server time to start accepting.
    return s, event


class TestAsynchat(unittest.TestCase):
    usepoll = False

    def setUp(self):
        self._threads = threading_helper.threading_setup()

    def tearDown(self):
        threading_helper.threading_cleanup(*self._threads)

    def line_terminator_check(self, term, server_chunk):
        event = threading.Event()
        s = echo_server(event)
        s.chunk_size = server_chunk
        s.start()
        event.wait()
        event.clear()
        time.sleep(0.01)   # Give server time to start accepting.
        c = echo_client(term, s.port)
        c.push(b"hello ")
        c.push(b"world" + term)
        c.push(b"I'm not dead yet!" + term)
        c.push(SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        threading_helper.join_thread(s)

        self.assertEqual(c.contents, [b"hello world", b"I'm not dead yet!"])

    # the line terminator tests below check receiving variously-sized
    # chunks back from the server in order to exercise all branches of
    # async_chat.handle_read

    def test_line_terminator1(self):
        # test one-character terminator
        for l in (1, 2, 3):
            self.line_terminator_check(b'\n', l)

    def test_line_terminator2(self):
        # test two-character terminator
        for l in (1, 2, 3):
            self.line_terminator_check(b'\r\n', l)

    def test_line_terminator3(self):
        # test three-character terminator
        for l in (1, 2, 3):
            self.line_terminator_check(b'qqq', l)

    def numeric_terminator_check(self, termlen):
        # Try reading a fixed number of bytes
        s, event = start_echo_server()
        c = echo_client(termlen, s.port)
        data = b"hello world, I'm not dead yet!\n"
        c.push(data)
        c.push(SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        threading_helper.join_thread(s)

        self.assertEqual(c.contents, [data[:termlen]])

    def test_numeric_terminator1(self):
        # check that ints & longs both work (since type is
        # explicitly checked in async_chat.handle_read)
        self.numeric_terminator_check(1)

    def test_numeric_terminator2(self):
        self.numeric_terminator_check(6)

    def test_none_terminator(self):
        # Try reading a fixed number of bytes
        s, event = start_echo_server()
        c = echo_client(None, s.port)
        data = b"hello world, I'm not dead yet!\n"
        c.push(data)
        c.push(SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        threading_helper.join_thread(s)

        self.assertEqual(c.contents, [])
        self.assertEqual(c.buffer, data)

    def test_simple_producer(self):
        s, event = start_echo_server()
        c = echo_client(b'\n', s.port)
        data = b"hello world\nI'm not dead yet!\n"
        p = asynchat.simple_producer(data+SERVER_QUIT, buffer_size=8)
        c.push_with_producer(p)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        threading_helper.join_thread(s)

        self.assertEqual(c.contents, [b"hello world", b"I'm not dead yet!"])

    def test_string_producer(self):
        s, event = start_echo_server()
        c = echo_client(b'\n', s.port)
        data = b"hello world\nI'm not dead yet!\n"
        c.push_with_producer(data+SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        threading_helper.join_thread(s)

        self.assertEqual(c.contents, [b"hello world", b"I'm not dead yet!"])

    def test_empty_line(self):
        # checks that empty lines are handled correctly
        s, event = start_echo_server()
        c = echo_client(b'\n', s.port)
        c.push(b"hello world\n\nI'm not dead yet!\n")
        c.push(SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        threading_helper.join_thread(s)

        self.assertEqual(c.contents,
                         [b"hello world", b"", b"I'm not dead yet!"])

    def test_close_when_done(self):
        s, event = start_echo_server()
        s.start_resend_event = threading.Event()
        c = echo_client(b'\n', s.port)
        c.push(b"hello world\nI'm not dead yet!\n")
        c.push(SERVER_QUIT)
        c.close_when_done()
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)

        # Only allow the server to start echoing data back to the client after
        # the client has closed its connection.  This prevents a race condition
        # where the server echoes all of its data before we can check that it
        # got any down below.
        s.start_resend_event.set()
        threading_helper.join_thread(s)

        self.assertEqual(c.contents, [])
        # the server might have been able to send a byte or two back, but this
        # at least checks that it received something and didn't just fail
        # (which could still result in the client not having received anything)
        self.assertGreater(len(s.buffer), 0)

    def test_push(self):
        # Issue #12523: push() should raise a TypeError if it doesn't get
        # a bytes string
        s, event = start_echo_server()
        c = echo_client(b'\n', s.port)
        data = b'bytes\n'
        c.push(data)
        c.push(bytearray(data))
        c.push(memoryview(data))
        self.assertRaises(TypeError, c.push, 10)
        self.assertRaises(TypeError, c.push, 'unicode')
        c.push(SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        threading_helper.join_thread(s)
        self.assertEqual(c.contents, [b'bytes', b'bytes', b'bytes'])


class TestAsynchat_WithPoll(TestAsynchat):
    usepoll = True


class TestAsynchatMocked(unittest.TestCase):
    def test_blockingioerror(self):
        # Issue #16133: handle_read() must ignore BlockingIOError
        sock = unittest.mock.Mock()
        sock.recv.side_effect = BlockingIOError(errno.EAGAIN)

        dispatcher = asynchat.async_chat()
        dispatcher.set_socket(sock)
        self.addCleanup(dispatcher.del_channel)

        with unittest.mock.patch.object(dispatcher, 'handle_error') as error:
            dispatcher.handle_read()
        self.assertFalse(error.called)


class TestHelperFunctions(unittest.TestCase):
    def test_find_prefix_at_end(self):
        self.assertEqual(asynchat.find_prefix_at_end("qwerty\r", "\r\n"), 1)
        self.assertEqual(asynchat.find_prefix_at_end("qwertydkjf", "\r\n"), 0)


class TestNotConnected(unittest.TestCase):
    def test_disallow_negative_terminator(self):
        # Issue #11259
        client = asynchat.async_chat()
        self.assertRaises(ValueError, client.set_terminator, -1)



if __name__ == "__main__":
    unittest.main()
