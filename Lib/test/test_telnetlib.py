import socket
import threading
import telnetlib
import time
import Queue

from unittest import TestCase
from test import test_support

HOST = test_support.HOST
EOF_sigil = object()

def server(evt, serv, dataq=None):
    """ Open a tcp server in three steps
        1) set evt to true to let the parent know we are ready
        2) [optional] write all the data in dataq to the socket
           terminate when dataq.get() returns EOF_sigil
        3) set evt to true to let the parent know we're done
    """
    serv.listen(5)
    evt.set()
    try:
        conn, addr = serv.accept()
        if dataq:
            data = ''
            new_data = dataq.get(True, 0.5)
            while new_data is not EOF_sigil:
                if type(new_data) == str:
                    data += new_data
                elif type(new_data) in [int, float]:
                    time.sleep(new_data)
                written = conn.send(data)
                data = data[written:]
                new_data = dataq.get(True, 0.5)
    except socket.timeout:
        pass
    finally:
        serv.close()
        evt.set()

def wibble_float(num):
    ''' return a (low, high) tuple that are 1% more and 1% less of num '''
    return num * 0.99, num * 1.01

class GeneralTests(TestCase):

    def setUp(self):
        self.evt = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(3)
        self.port = test_support.bind_port(self.sock)
        self.thread = threading.Thread(target=server, args=(self.evt,self.sock))
        self.thread.start()
        self.evt.wait()
        self.evt.clear()
        time.sleep(.1)

    def tearDown(self):
        self.evt.wait()
        self.thread.join()

    def testBasic(self):
        # connects
        telnet = telnetlib.Telnet(HOST, self.port)
        telnet.sock.close()

    def testTimeoutDefault(self):
        self.assertTrue(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(30)
        try:
            telnet = telnetlib.Telnet("localhost", self.port)
        finally:
            socket.setdefaulttimeout(None)
        self.assertEqual(telnet.sock.gettimeout(), 30)
        telnet.sock.close()

    def testTimeoutNone(self):
        # None, having other default
        self.assertTrue(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(30)
        try:
            telnet = telnetlib.Telnet(HOST, self.port, timeout=None)
        finally:
            socket.setdefaulttimeout(None)
        self.assertTrue(telnet.sock.gettimeout() is None)
        telnet.sock.close()

    def testTimeoutValue(self):
        telnet = telnetlib.Telnet("localhost", self.port, timeout=30)
        self.assertEqual(telnet.sock.gettimeout(), 30)
        telnet.sock.close()

    def testTimeoutOpen(self):
        telnet = telnetlib.Telnet()
        telnet.open("localhost", self.port, timeout=30)
        self.assertEqual(telnet.sock.gettimeout(), 30)
        telnet.sock.close()

def _read_setUp(self):
    # the blocking constant should be tuned!
    self.blocking_timeout = 0.0
    self.evt = threading.Event()
    self.dataq = Queue.Queue()
    self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.sock.settimeout(3)
    self.port = test_support.bind_port(self.sock)
    self.thread = threading.Thread(target=server, args=(self.evt,self.sock, self.dataq))
    self.thread.start()
    self.evt.wait()
    self.evt.clear()
    time.sleep(.1)

def _read_tearDown(self):
    self.evt.wait()
    self.thread.join()


class ReadTests(TestCase):
    setUp = _read_setUp
    tearDown = _read_tearDown

    def _test_blocking(self, func):
        start = time.time()
        self.dataq.put(self.blocking_timeout)
        self.dataq.put(EOF_sigil)
        data = func()
        low, high = wibble_float(self.blocking_timeout)
        self.assertTrue(time.time() - start >= low)

    def test_read_until_A(self):
        """
        read_until(expected, [timeout])
          Read until the expected string has been seen, or a timeout is
          hit (default is no timeout); may block.
        """
        want = ['x' * 10, 'match', 'y' * 10, EOF_sigil]
        for item in want:
            self.dataq.put(item)
        telnet = telnetlib.Telnet(HOST, self.port)
        data = telnet.read_until('match')
        self.assertEqual(data, ''.join(want[:-2]))

    def test_read_until_B(self):
        # test the timeout - it does NOT raise socket.timeout
        want = ['hello', self.blocking_timeout, EOF_sigil]
        for item in want:
            self.dataq.put(item)
        telnet = telnetlib.Telnet(HOST, self.port)
        start = time.time()
        timeout = self.blocking_timeout / 2
        data = telnet.read_until('not seen', timeout)
        low, high = wibble_float(timeout)
        self.assertTrue(low <= time.time() - high)
        self.assertEqual(data, want[0])

    def test_read_all_A(self):
        """
        read_all()
          Read all data until EOF; may block.
        """
        want = ['x' * 500, 'y' * 500, 'z' * 500, EOF_sigil]
        for item in want:
            self.dataq.put(item)
        telnet = telnetlib.Telnet(HOST, self.port)
        data = telnet.read_all()
        self.assertEqual(data, ''.join(want[:-1]))
        return

    def test_read_all_B(self):
        self._test_blocking(telnetlib.Telnet(HOST, self.port).read_all)

    def test_read_some_A(self):
        """
        read_some()
          Read at least one byte or EOF; may block.
        """
        # test 'at least one byte'
        want = ['x' * 500, EOF_sigil]
        for item in want:
            self.dataq.put(item)
        telnet = telnetlib.Telnet(HOST, self.port)
        data = telnet.read_all()
        self.assertTrue(len(data) >= 1)

    def test_read_some_B(self):
        # test EOF
        self.dataq.put(EOF_sigil)
        telnet = telnetlib.Telnet(HOST, self.port)
        self.assertEqual('', telnet.read_some())

    def test_read_all_C(self):
        self._test_blocking(telnetlib.Telnet(HOST, self.port).read_some)

    def _test_read_any_eager_A(self, func_name):
        """
        read_very_eager()
          Read all data available already queued or on the socket,
          without blocking.
        """
        # this never blocks so it should return eat part in turn
        want = ['x' * 100, self.blocking_timeout/2, 'y' * 100, EOF_sigil]
        expects = want[0] + want[2]
        for item in want:
            self.dataq.put(item)
        telnet = telnetlib.Telnet(HOST, self.port)
        func = getattr(telnet, func_name)
        time.sleep(self.blocking_timeout/10)
        data = ''
        while True:
            try:
                data += func()
                self.assertTrue(expects.startswith(data))
                time.sleep(self.blocking_timeout)
            except EOFError:
                break
        self.assertEqual(expects, data)

    def _test_read_any_eager_B(self, func_name):
        # test EOF
        self.dataq.put(EOF_sigil)
        time.sleep(self.blocking_timeout / 10)
        telnet = telnetlib.Telnet(HOST, self.port)
        func = getattr(telnet, func_name)
        self.assertRaises(EOFError, func)

    # read_eager and read_very_eager make the same gaurantees
    # (they behave differently but we only test the gaurantees)
    def test_read_very_eager_A(self):
        self._test_read_any_eager_A('read_very_eager')
    def test_read_very_eager_B(self):
        self._test_read_any_eager_B('read_very_eager')
    def test_read_eager_A(self):
        self._test_read_any_eager_A('read_eager')
    def test_read_eager_B(self):
        self._test_read_any_eager_B('read_eager')
    # NB -- we need to test the IAC block which is mentioned in the docstring
    # but not in the module docs

    def _test_read_any_lazy_A(self, func_name):
        want = [self.blocking_timeout/2, 'x' * 100, EOF_sigil]
        for item in want:
            self.dataq.put(item)
        telnet = telnetlib.Telnet(HOST, self.port)
        func = getattr(telnet, func_name)
        self.assertEqual('', func())
        data = ''
        while True:
            time.sleep(self.blocking_timeout)
            try:
                telnet.fill_rawq()
                data += func()
                if not data:
                    break
            except EOFError:
                break
            self.assertTrue(want[1].startswith(data))
        return data, want[1]

    def _test_read_any_lazy_B(self, func_name):
        self.dataq.put(EOF_sigil)
        telnet = telnetlib.Telnet(HOST, self.port)
        func = getattr(telnet, func_name)
        time.sleep(self.blocking_timeout/10)
        telnet.fill_rawq()
        self.assertRaises(EOFError, func)

    # read_lazy and read_very_lazy make the samish gaurantees
    def test_read_very_lazy_A(self):
        data, want = self._test_read_any_lazy_A('read_very_lazy')
        self.assertEqual(data, '')
    def test_read_lazy(self):
        data, want = self._test_read_any_lazy_A('read_lazy')
        self.assertEqual(data, want)
    def test_read_very_lazy_B(self):
        self._test_read_any_lazy_B('read_very_lazy')
    def test_read_lazy_B(self):
        self._test_read_any_lazy_B('read_lazy')

class nego_collector(object):
    def __init__(self, sb_getter=None):
        self.seen = ''
        self.sb_getter = sb_getter
        self.sb_seen = ''

    def do_nego(self, sock, cmd, opt):
        self.seen += cmd + opt
        if cmd == tl.SE and self.sb_getter:
            sb_data = self.sb_getter()
            self.sb_seen += sb_data

tl = telnetlib
class OptionTests(TestCase):
    setUp = _read_setUp
    tearDown = _read_tearDown
    # RFC 854 commands
    cmds = [tl.AO, tl.AYT, tl.BRK, tl.EC, tl.EL, tl.GA, tl.IP, tl.NOP]

    def _test_command(self, data):
        """ helper for testing IAC + cmd """
        self.setUp()
        for item in data:
            self.dataq.put(item)
        telnet = telnetlib.Telnet(HOST, self.port)
        nego = nego_collector()
        telnet.set_option_negotiation_callback(nego.do_nego)
        time.sleep(self.blocking_timeout/10)
        txt = telnet.read_all()
        cmd = nego.seen
        self.assertTrue(len(cmd) > 0) # we expect at least one command
        self.assertTrue(cmd[0] in self.cmds)
        self.assertEqual(cmd[1], tl.NOOPT)
        self.assertEqual(len(''.join(data[:-1])), len(txt + cmd))
        self.tearDown()

    def test_IAC_commands(self):
        # reset our setup
        self.dataq.put(EOF_sigil)
        telnet = telnetlib.Telnet(HOST, self.port)
        self.tearDown()

        for cmd in self.cmds:
            self._test_command(['x' * 100, tl.IAC + cmd, 'y'*100, EOF_sigil])
            self._test_command(['x' * 10, tl.IAC + cmd, 'y'*10, EOF_sigil])
            self._test_command([tl.IAC + cmd, EOF_sigil])
        # all at once
        self._test_command([tl.IAC + cmd for (cmd) in self.cmds] + [EOF_sigil])

    def test_SB_commands(self):
        # RFC 855, subnegotiations portion
        send = [tl.IAC + tl.SB + tl.IAC + tl.SE,
                tl.IAC + tl.SB + tl.IAC + tl.IAC + tl.IAC + tl.SE,
                tl.IAC + tl.SB + tl.IAC + tl.IAC + 'aa' + tl.IAC + tl.SE,
                tl.IAC + tl.SB + 'bb' + tl.IAC + tl.IAC + tl.IAC + tl.SE,
                tl.IAC + tl.SB + 'cc' + tl.IAC + tl.IAC + 'dd' + tl.IAC + tl.SE,
                EOF_sigil,
               ]
        for item in send:
            self.dataq.put(item)
        telnet = telnetlib.Telnet(HOST, self.port)
        nego = nego_collector(telnet.read_sb_data)
        telnet.set_option_negotiation_callback(nego.do_nego)
        time.sleep(self.blocking_timeout/10)
        txt = telnet.read_all()
        self.assertEqual(txt, '')
        want_sb_data = tl.IAC + tl.IAC + 'aabb' + tl.IAC + 'cc' + tl.IAC + 'dd'
        self.assertEqual(nego.sb_seen, want_sb_data)

def test_main(verbose=None):
    test_support.run_unittest(GeneralTests, ReadTests, OptionTests)

if __name__ == '__main__':
    test_main()
