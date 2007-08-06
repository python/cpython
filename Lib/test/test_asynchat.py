# test asynchat -- requires threading

import thread # If this fails, we can't test this module
import asyncore, asynchat, socket, threading, time
import unittest
import sys
from test import test_support

HOST = "127.0.0.1"
PORT = 54322
SERVER_QUIT = b'QUIT\n'

class echo_server(threading.Thread):
    # parameter to determine the number of bytes passed back to the
    # client each send
    chunk_size = 1

    def run(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        global PORT
        PORT = test_support.bind_port(sock, HOST, PORT)
        sock.listen(1)
        conn, client = sock.accept()
        self.buffer = b""
        # collect data until quit message is seen
        while SERVER_QUIT not in self.buffer:
            data = conn.recv(1)
            if not data:
                break
            self.buffer = self.buffer + data

        # remove the SERVER_QUIT message
        self.buffer = self.buffer.replace(SERVER_QUIT, b'')

        # re-send entire set of collected data
        try:
            # this may fail on some tests, such as test_close_when_done, since
            # the client closes the channel when it's done sending
            while self.buffer:
                n = conn.send(self.buffer[:self.chunk_size])
                time.sleep(0.001)
                self.buffer = self.buffer[n:]
        except:
            pass

        conn.close()
        sock.close()

class echo_client(asynchat.async_chat):

    def __init__(self, terminator):
        asynchat.async_chat.__init__(self)
        self.contents = []
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connect((HOST, PORT))
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


class TestAsynchat(unittest.TestCase):
    usepoll = False

    def setUp (self):
        pass

    def tearDown (self):
        pass

    def line_terminator_check(self, term, server_chunk):
        s = echo_server()
        s.chunk_size = server_chunk
        s.start()
        time.sleep(0.5) # Give server time to initialize
        c = echo_client(term)
        c.push(b"hello ")
        c.push(bytes("world%s" % term))
        c.push(bytes("I'm not dead yet!%s" % term))
        c.push(SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        s.join()

        self.assertEqual(c.contents, [b"hello world", b"I'm not dead yet!"])

    # the line terminator tests below check receiving variously-sized
    # chunks back from the server in order to exercise all branches of
    # async_chat.handle_read

    def test_line_terminator1(self):
        # test one-character terminator
        for l in (1,2,3):
            self.line_terminator_check(b'\n', l)

    def test_line_terminator2(self):
        # test two-character terminator
        for l in (1,2,3):
            self.line_terminator_check(b'\r\n', l)

    def test_line_terminator3(self):
        # test three-character terminator
        for l in (1,2,3):
            self.line_terminator_check(b'qqq', l)

    def numeric_terminator_check(self, termlen):
        # Try reading a fixed number of bytes
        s = echo_server()
        s.start()
        time.sleep(0.5) # Give server time to initialize
        c = echo_client(termlen)
        data = b"hello world, I'm not dead yet!\n"
        c.push(data)
        c.push(SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        s.join()

        self.assertEqual(c.contents, [data[:termlen]])

    def test_numeric_terminator1(self):
        # check that ints & longs both work (since type is
        # explicitly checked in async_chat.handle_read)
        self.numeric_terminator_check(1)

    def test_numeric_terminator2(self):
        self.numeric_terminator_check(6)

    def test_none_terminator(self):
        # Try reading a fixed number of bytes
        s = echo_server()
        s.start()
        time.sleep(0.5) # Give server time to initialize
        c = echo_client(None)
        data = b"hello world, I'm not dead yet!\n"
        c.push(data)
        c.push(SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        s.join()

        self.assertEqual(c.contents, [])
        self.assertEqual(c.buffer, data)

    def test_simple_producer(self):
        s = echo_server()
        s.start()
        time.sleep(0.5) # Give server time to initialize
        c = echo_client(b'\n')
        data = b"hello world\nI'm not dead yet!\n"
        p = asynchat.simple_producer(data+SERVER_QUIT, buffer_size=8)
        c.push_with_producer(p)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        s.join()

        self.assertEqual(c.contents, [b"hello world", b"I'm not dead yet!"])

    def test_string_producer(self):
        s = echo_server()
        s.start()
        time.sleep(0.5) # Give server time to initialize
        c = echo_client(b'\n')
        data = b"hello world\nI'm not dead yet!\n"
        c.push_with_producer(data+SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        s.join()

        self.assertEqual(c.contents, [b"hello world", b"I'm not dead yet!"])

    def test_empty_line(self):
        # checks that empty lines are handled correctly
        s = echo_server()
        s.start()
        time.sleep(0.5) # Give server time to initialize
        c = echo_client(b'\n')
        c.push("hello world\n\nI'm not dead yet!\n")
        c.push(SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        s.join()

        self.assertEqual(c.contents,
                         [b"hello world", b"", b"I'm not dead yet!"])

    def test_close_when_done(self):
        s = echo_server()
        s.start()
        time.sleep(0.5) # Give server time to initialize
        c = echo_client(b'\n')
        c.push("hello world\nI'm not dead yet!\n")
        c.push(SERVER_QUIT)
        c.close_when_done()
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        s.join()

        self.assertEqual(c.contents, [])
        # the server might have been able to send a byte or two back, but this
        # at least checks that it received something and didn't just fail
        # (which could still result in the client not having received anything)
        self.assertTrue(len(s.buffer) > 0)


class TestAsynchat_WithPoll(TestAsynchat):
    usepoll = True

class TestHelperFunctions(unittest.TestCase):
    def test_find_prefix_at_end(self):
        self.assertEqual(asynchat.find_prefix_at_end("qwerty\r", "\r\n"), 1)
        self.assertEqual(asynchat.find_prefix_at_end("qwertydkjf", "\r\n"), 0)

class TestFifo(unittest.TestCase):
    def test_basic(self):
        f = asynchat.fifo()
        f.push(7)
        f.push(b'a')
        self.assertEqual(len(f), 2)
        self.assertEqual(f.first(), 7)
        self.assertEqual(f.pop(), (1, 7))
        self.assertEqual(len(f), 1)
        self.assertEqual(f.first(), b'a')
        self.assertEqual(f.is_empty(), False)
        self.assertEqual(f.pop(), (1, b'a'))
        self.assertEqual(len(f), 0)
        self.assertEqual(f.is_empty(), True)
        self.assertEqual(f.pop(), (0, None))

    def test_given_list(self):
        f = asynchat.fifo([b'x', 17, 3])
        self.assertEqual(len(f), 3)
        self.assertEqual(f.pop(), (1, b'x'))
        self.assertEqual(f.pop(), (1, 17))
        self.assertEqual(f.pop(), (1, 3))
        self.assertEqual(f.pop(), (0, None))


def test_main(verbose=None):
    test_support.run_unittest(TestAsynchat, TestAsynchat_WithPoll,
                              TestHelperFunctions, TestFifo)

if __name__ == "__main__":
    test_main(verbose=True)
