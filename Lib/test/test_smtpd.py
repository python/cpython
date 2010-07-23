import asynchat
from unittest import TestCase
import socket
from test import support
import asyncore
import io
import smtpd

# mock-ish socket to sit underneath asyncore
class DummySocket:
    def __init__(self):
        self.output = []
        self.queue = []
        self.conn = None
    def queue_recv(self, line):
        self.queue.append(line)
    def recv(self, *args):
        data = self.queue.pop(0) + b'\r\n'
        return data
    def fileno(self):
        return 0
    def setsockopt(self, *args):
        pass
    def getsockopt(self, *args):
        return 0
    def bind(self, *args):
        pass
    def accept(self):
        self.conn = DummySocket()
        return self.conn, 'c'
    def listen(self, *args):
        pass
    def setblocking(self, *args):
        pass
    def send(self, data):
        self.last = data
        self.output.append(data)
        return len(data)
    def getpeername(self):
        return 'peer'
    def close(self):
        pass

class DummyServer(smtpd.SMTPServer):
    messages = []
    def create_socket(self, family, type):
        self.family_and_type = (socket.AF_INET, socket.SOCK_STREAM)
        self.set_socket(DummySocket())
    def process_message(self, peer, mailfrom, rcpttos, data):
        self.messages.append((peer, mailfrom, rcpttos, data))
        if data == 'return status':
            return '250 Okish'

class DummyDispatcherBroken(Exception):
    pass

class BrokenDummyServer(DummyServer):
    def listen(self, num):
        raise DummyDispatcherBroken()

class SMTPDChannelTest(TestCase):
    def setUp(self):
        self.debug = smtpd.DEBUGSTREAM = io.StringIO()
        self.server = DummyServer('a', 'b')
        conn, addr = self.server.accept()
        self.channel = smtpd.SMTPChannel(self.server, conn, addr)

    def write_line(self, line):
        self.channel.socket.queue_recv(line)
        self.channel.handle_read()

    def test_broken_connect(self):
        self.assertRaises(DummyDispatcherBroken, BrokenDummyServer, 'a', 'b')

    def test_server_accept(self):
        self.server.handle_accept()

    def test_missing_data(self):
        self.write_line(b'')
        self.assertEqual(self.channel.last,
                         b'500 Error: bad syntax\r\n')

    def test_EHLO_not_implemented(self):
        self.write_line(b'EHLO test.example')
        self.assertEqual(self.channel.last,
                         b'502 Error: command "EHLO" not implemented\r\n')

    def test_HELO(self):
        name = socket.getfqdn()
        self.write_line(b'HELO test.example')
        self.assertEqual(self.channel.last,
                         '250 {}\r\n'.format(name).encode('ascii'))

    def test_HELO_bad_syntax(self):
        self.write_line(b'HELO')
        self.assertEqual(self.channel.last,
                         b'501 Syntax: HELO hostname\r\n')

    def test_HELO_duplicate(self):
        self.write_line(b'HELO test.example')
        self.write_line(b'HELO test.example')
        self.assertEqual(self.channel.last,
                         b'503 Duplicate HELO/EHLO\r\n')

    def test_NOOP(self):
        self.write_line(b'NOOP')
        self.assertEqual(self.channel.last, b'250 Ok\r\n')

    def test_NOOP_bad_syntax(self):
        self.write_line(b'NOOP hi')
        self.assertEqual(self.channel.last,
                         b'501 Syntax: NOOP\r\n')

    def test_QUIT(self):
        self.write_line(b'QUIT')
        self.assertEqual(self.channel.last, b'221 Bye\r\n')

    def test_QUIT_arg_ignored(self):
        self.write_line(b'QUIT bye bye')
        self.assertEqual(self.channel.last, b'221 Bye\r\n')

    def test_bad_state(self):
        self.channel._SMTPChannel__state = 'BAD STATE'
        self.write_line(b'HELO')
        self.assertEqual(self.channel.last, b'451 Internal confusion\r\n')

    def test_need_MAIL(self):
        self.write_line(b'RCPT to:spam@example')
        self.assertEqual(self.channel.last,
            b'503 Error: need MAIL command\r\n')

    def test_MAIL_syntax(self):
        self.write_line(b'MAIL from eggs@example')
        self.assertEqual(self.channel.last,
            b'501 Syntax: MAIL FROM:<address>\r\n')

    def test_MAIL_missing_from(self):
        self.write_line(b'MAIL from:')
        self.assertEqual(self.channel.last,
            b'501 Syntax: MAIL FROM:<address>\r\n')

    def test_MAIL_chevrons(self):
        self.write_line(b'MAIL from:<eggs@example>')
        self.assertEqual(self.channel.last, b'250 Ok\r\n')

    def test_nested_MAIL(self):
        self.write_line(b'MAIL from:eggs@example')
        self.write_line(b'MAIL from:spam@example')
        self.assertEqual(self.channel.last,
            b'503 Error: nested MAIL command\r\n')

    def test_need_RCPT(self):
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'DATA')
        self.assertEqual(self.channel.last,
            b'503 Error: need RCPT command\r\n')

    def test_RCPT_syntax(self):
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT to eggs@example')
        self.assertEqual(self.channel.last,
            b'501 Syntax: RCPT TO: <address>\r\n')

    def test_data_dialog(self):
        self.write_line(b'MAIL From:eggs@example')
        self.assertEqual(self.channel.last, b'250 Ok\r\n')
        self.write_line(b'RCPT To:spam@example')
        self.assertEqual(self.channel.last, b'250 Ok\r\n')

        self.write_line(b'DATA')
        self.assertEqual(self.channel.last,
            b'354 End data with <CR><LF>.<CR><LF>\r\n')
        self.write_line(b'data\r\nmore\r\n.')
        self.assertEqual(self.channel.last, b'250 Ok\r\n')
        self.assertEqual(self.server.messages[-1],
            ('peer', 'eggs@example', ['spam@example'], 'data\nmore'))

    def test_DATA_syntax(self):
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'DATA spam')
        self.assertEqual(self.channel.last, b'501 Syntax: DATA\r\n')

    def test_multiple_RCPT(self):
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'RCPT To:ham@example')
        self.write_line(b'DATA')
        self.write_line(b'data\r\n.')
        self.assertEqual(self.server.messages[-1],
            ('peer', 'eggs@example', ['spam@example','ham@example'], 'data'))

    def test_manual_status(self):
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'DATA')
        self.write_line(b'return status\r\n.')
        self.assertEqual(self.channel.last, b'250 Okish\r\n')

    def test_RSET(self):
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'RSET')
        self.assertEqual(self.channel.last, b'250 Ok\r\n')
        self.write_line(b'MAIL From:foo@example')
        self.write_line(b'RCPT To:eggs@example')
        self.write_line(b'DATA')
        self.write_line(b'data\r\n.')
        self.assertEqual(self.server.messages[0],
            ('peer', 'foo@example', ['eggs@example'], 'data'))

    def test_RSET_syntax(self):
        self.write_line(b'RSET hi')
        self.assertEqual(self.channel.last, b'501 Syntax: RSET\r\n')


def test_main():
    support.run_unittest(SMTPDChannelTest)

if __name__ == "__main__":
    test_main()
