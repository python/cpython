# test asynchat

import asyncore, asynchat, socket, time
import unittest
import sys
from test import test_support
try:
    import threading
except ImportError:
    threading = None

HOST = test_support.HOST
SERVER_QUIT = 'QUIT\n'

if threading:
    class echo_server(threading.Thread):
        # parameter to determine the number of bytes passed back to the
        # client each send
        chunk_size = 1

        def __init__(self, event):
            threading.Thread.__init__(self)
            self.event = event
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.port = test_support.bind_port(self.sock)
            # This will be set if the client wants us to wait before echoing data
            # back.
            self.start_resend_event = None

        def run(self):
            self.sock.listen(1)
            self.event.set()
            conn, client = self.sock.accept()
            self.buffer = ""
            # collect data until quit message is seen
            while SERVER_QUIT not in self.buffer:
                data = conn.recv(1)
                if not data:
                    break
                self.buffer = self.buffer + data

            # remove the SERVER_QUIT message
            self.buffer = self.buffer.replace(SERVER_QUIT, '')

            if self.start_resend_event:
                self.start_resend_event.wait()

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
            self.sock.close()

    class echo_client(asynchat.async_chat):

        def __init__(self, terminator, server_port):
            asynchat.async_chat.__init__(self)
            self.contents = []
            self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
            self.connect((HOST, server_port))
            self.set_terminator(terminator)
            self.buffer = ''

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
            self.buffer = ""


    def start_echo_server():
        event = threading.Event()
        s = echo_server(event)
        s.start()
        event.wait()
        event.clear()
        time.sleep(0.01) # Give server time to start accepting.
        return s, event


@unittest.skipUnless(threading, 'Threading required for this test.')
class TestAsynchat(unittest.TestCase):
    usepoll = False

    def setUp (self):
        self._threads = test_support.threading_setup()

    def tearDown (self):
        test_support.threading_cleanup(*self._threads)

    def line_terminator_check(self, term, server_chunk):
        event = threading.Event()
        s = echo_server(event)
        s.chunk_size = server_chunk
        s.start()
        event.wait()
        event.clear()
        time.sleep(0.01) # Give server time to start accepting.
        c = echo_client(term, s.port)
        c.push("hello ")
        c.push("world%s" % term)
        c.push("I'm not dead yet!%s" % term)
        c.push(SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        s.join()

        self.assertEqual(c.contents, ["hello world", "I'm not dead yet!"])

    # the line terminator tests below check receiving variously-sized
    # chunks back from the server in order to exercise all branches of
    # async_chat.handle_read

    def test_line_terminator1(self):
        # test one-character terminator
        for l in (1,2,3):
            self.line_terminator_check('\n', l)

    def test_line_terminator2(self):
        # test two-character terminator
        for l in (1,2,3):
            self.line_terminator_check('\r\n', l)

    def test_line_terminator3(self):
        # test three-character terminator
        for l in (1,2,3):
            self.line_terminator_check('qqq', l)

    def numeric_terminator_check(self, termlen):
        # Try reading a fixed number of bytes
        s, event = start_echo_server()
        c = echo_client(termlen, s.port)
        data = "hello world, I'm not dead yet!\n"
        c.push(data)
        c.push(SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        s.join()

        self.assertEqual(c.contents, [data[:termlen]])

    def test_numeric_terminator1(self):
        # check that ints & longs both work (since type is
        # explicitly checked in async_chat.handle_read)
        self.numeric_terminator_check(1)
        self.numeric_terminator_check(1L)

    def test_numeric_terminator2(self):
        self.numeric_terminator_check(6L)

    def test_none_terminator(self):
        # Try reading a fixed number of bytes
        s, event = start_echo_server()
        c = echo_client(None, s.port)
        data = "hello world, I'm not dead yet!\n"
        c.push(data)
        c.push(SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        s.join()

        self.assertEqual(c.contents, [])
        self.assertEqual(c.buffer, data)

    def test_simple_producer(self):
        s, event = start_echo_server()
        c = echo_client('\n', s.port)
        data = "hello world\nI'm not dead yet!\n"
        p = asynchat.simple_producer(data+SERVER_QUIT, buffer_size=8)
        c.push_with_producer(p)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        s.join()

        self.assertEqual(c.contents, ["hello world", "I'm not dead yet!"])

    def test_string_producer(self):
        s, event = start_echo_server()
        c = echo_client('\n', s.port)
        data = "hello world\nI'm not dead yet!\n"
        c.push_with_producer(data+SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        s.join()

        self.assertEqual(c.contents, ["hello world", "I'm not dead yet!"])

    def test_empty_line(self):
        # checks that empty lines are handled correctly
        s, event = start_echo_server()
        c = echo_client('\n', s.port)
        c.push("hello world\n\nI'm not dead yet!\n")
        c.push(SERVER_QUIT)
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)
        s.join()

        self.assertEqual(c.contents, ["hello world", "", "I'm not dead yet!"])

    def test_close_when_done(self):
        s, event = start_echo_server()
        s.start_resend_event = threading.Event()
        c = echo_client('\n', s.port)
        c.push("hello world\nI'm not dead yet!\n")
        c.push(SERVER_QUIT)
        c.close_when_done()
        asyncore.loop(use_poll=self.usepoll, count=300, timeout=.01)

        # Only allow the server to start echoing data back to the client after
        # the client has closed its connection.  This prevents a race condition
        # where the server echoes all of its data before we can check that it
        # got any down below.
        s.start_resend_event.set()
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
        f.push('a')
        self.assertEqual(len(f), 2)
        self.assertEqual(f.first(), 7)
        self.assertEqual(f.pop(), (1, 7))
        self.assertEqual(len(f), 1)
        self.assertEqual(f.first(), 'a')
        self.assertEqual(f.is_empty(), False)
        self.assertEqual(f.pop(), (1, 'a'))
        self.assertEqual(len(f), 0)
        self.assertEqual(f.is_empty(), True)
        self.assertEqual(f.pop(), (0, None))

    def test_given_list(self):
        f = asynchat.fifo(['x', 17, 3])
        self.assertEqual(len(f), 3)
        self.assertEqual(f.pop(), (1, 'x'))
        self.assertEqual(f.pop(), (1, 17))
        self.assertEqual(f.pop(), (1, 3))
        self.assertEqual(f.pop(), (0, None))


def test_main(verbose=None):
    test_support.run_unittest(TestAsynchat, TestAsynchat_WithPoll,
                              TestHelperFunctions, TestFifo)

if __name__ == "__main__":
    test_main(verbose=True)
