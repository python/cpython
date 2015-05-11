import unittest
import textwrap
from test import support, mock_socket
import socket
import io
import smtpd
import asyncore


class DummyServer(smtpd.SMTPServer):
    def __init__(self, *args, **kwargs):
        smtpd.SMTPServer.__init__(self, *args, **kwargs)
        self.messages = []
        if self._decode_data:
            self.return_status = 'return status'
        else:
            self.return_status = b'return status'

    def process_message(self, peer, mailfrom, rcpttos, data, **kw):
        self.messages.append((peer, mailfrom, rcpttos, data))
        if data == self.return_status:
            return '250 Okish'
        if 'mail_options' in kw and 'SMTPUTF8' in kw['mail_options']:
            return '250 SMTPUTF8 message okish'


class DummyDispatcherBroken(Exception):
    pass


class BrokenDummyServer(DummyServer):
    def listen(self, num):
        raise DummyDispatcherBroken()


class SMTPDServerTest(unittest.TestCase):
    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket

    def test_process_message_unimplemented(self):
        server = smtpd.SMTPServer((support.HOST, 0), ('b', 0),
                                  decode_data=True)
        conn, addr = server.accept()
        channel = smtpd.SMTPChannel(server, conn, addr, decode_data=True)

        def write_line(line):
            channel.socket.queue_recv(line)
            channel.handle_read()

        write_line(b'HELO example')
        write_line(b'MAIL From:eggs@example')
        write_line(b'RCPT To:spam@example')
        write_line(b'DATA')
        self.assertRaises(NotImplementedError, write_line, b'spam\r\n.\r\n')

    def test_decode_data_default_warns(self):
        with self.assertWarns(DeprecationWarning):
            smtpd.SMTPServer((support.HOST, 0), ('b', 0))

    def test_decode_data_and_enable_SMTPUTF8_raises(self):
        self.assertRaises(
            ValueError,
            smtpd.SMTPServer,
            (support.HOST, 0),
            ('b', 0),
            enable_SMTPUTF8=True,
            decode_data=True)

    def tearDown(self):
        asyncore.close_all()
        asyncore.socket = smtpd.socket = socket


class DebuggingServerTest(unittest.TestCase):

    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket

    def send_data(self, channel, data, enable_SMTPUTF8=False):
        def write_line(line):
            channel.socket.queue_recv(line)
            channel.handle_read()
        write_line(b'EHLO example')
        if enable_SMTPUTF8:
            write_line(b'MAIL From:eggs@example BODY=8BITMIME SMTPUTF8')
        else:
            write_line(b'MAIL From:eggs@example')
        write_line(b'RCPT To:spam@example')
        write_line(b'DATA')
        write_line(data)
        write_line(b'.')

    def test_process_message_with_decode_data_true(self):
        server = smtpd.DebuggingServer((support.HOST, 0), ('b', 0),
                                       decode_data=True)
        conn, addr = server.accept()
        channel = smtpd.SMTPChannel(server, conn, addr, decode_data=True)
        with support.captured_stdout() as s:
            self.send_data(channel, b'From: test\n\nhello\n')
        stdout = s.getvalue()
        self.assertEqual(stdout, textwrap.dedent("""\
             ---------- MESSAGE FOLLOWS ----------
             From: test
             X-Peer: peer-address

             hello
             ------------ END MESSAGE ------------
             """))

    def test_process_message_with_decode_data_false(self):
        server = smtpd.DebuggingServer((support.HOST, 0), ('b', 0),
                                       decode_data=False)
        conn, addr = server.accept()
        channel = smtpd.SMTPChannel(server, conn, addr, decode_data=False)
        with support.captured_stdout() as s:
            self.send_data(channel, b'From: test\n\nh\xc3\xa9llo\xff\n')
        stdout = s.getvalue()
        self.assertEqual(stdout, textwrap.dedent("""\
             ---------- MESSAGE FOLLOWS ----------
             b'From: test'
             b'X-Peer: peer-address'
             b''
             b'h\\xc3\\xa9llo\\xff'
             ------------ END MESSAGE ------------
             """))

    def test_process_message_with_enable_SMTPUTF8_true(self):
        server = smtpd.DebuggingServer((support.HOST, 0), ('b', 0),
                                       enable_SMTPUTF8=True)
        conn, addr = server.accept()
        channel = smtpd.SMTPChannel(server, conn, addr, enable_SMTPUTF8=True)
        with support.captured_stdout() as s:
            self.send_data(channel, b'From: test\n\nh\xc3\xa9llo\xff\n')
        stdout = s.getvalue()
        self.assertEqual(stdout, textwrap.dedent("""\
             ---------- MESSAGE FOLLOWS ----------
             b'From: test'
             b'X-Peer: peer-address'
             b''
             b'h\\xc3\\xa9llo\\xff'
             ------------ END MESSAGE ------------
             """))

    def test_process_SMTPUTF8_message_with_enable_SMTPUTF8_true(self):
        server = smtpd.DebuggingServer((support.HOST, 0), ('b', 0),
                                       enable_SMTPUTF8=True)
        conn, addr = server.accept()
        channel = smtpd.SMTPChannel(server, conn, addr, enable_SMTPUTF8=True)
        with support.captured_stdout() as s:
            self.send_data(channel, b'From: test\n\nh\xc3\xa9llo\xff\n',
                           enable_SMTPUTF8=True)
        stdout = s.getvalue()
        self.assertEqual(stdout, textwrap.dedent("""\
             ---------- MESSAGE FOLLOWS ----------
             mail options: ['BODY=8BITMIME', 'SMTPUTF8']
             b'From: test'
             b'X-Peer: peer-address'
             b''
             b'h\\xc3\\xa9llo\\xff'
             ------------ END MESSAGE ------------
             """))

    def tearDown(self):
        asyncore.close_all()
        asyncore.socket = smtpd.socket = socket


class TestFamilyDetection(unittest.TestCase):
    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket

    def tearDown(self):
        asyncore.close_all()
        asyncore.socket = smtpd.socket = socket

    @unittest.skipUnless(support.IPV6_ENABLED, "IPv6 not enabled")
    def test_socket_uses_IPv6(self):
        server = smtpd.SMTPServer((support.HOSTv6, 0), (support.HOST, 0),
                                  decode_data=False)
        self.assertEqual(server.socket.family, socket.AF_INET6)

    def test_socket_uses_IPv4(self):
        server = smtpd.SMTPServer((support.HOST, 0), (support.HOSTv6, 0),
                                  decode_data=False)
        self.assertEqual(server.socket.family, socket.AF_INET)


class TestRcptOptionParsing(unittest.TestCase):
    error_response = (b'555 RCPT TO parameters not recognized or not '
                      b'implemented\r\n')

    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket
        self.old_debugstream = smtpd.DEBUGSTREAM
        self.debug = smtpd.DEBUGSTREAM = io.StringIO()

    def tearDown(self):
        asyncore.close_all()
        asyncore.socket = smtpd.socket = socket
        smtpd.DEBUGSTREAM = self.old_debugstream

    def write_line(self, channel, line):
        channel.socket.queue_recv(line)
        channel.handle_read()

    def test_params_rejected(self):
        server = DummyServer((support.HOST, 0), ('b', 0), decode_data=False)
        conn, addr = server.accept()
        channel = smtpd.SMTPChannel(server, conn, addr, decode_data=False)
        self.write_line(channel, b'EHLO example')
        self.write_line(channel, b'MAIL from: <foo@example.com> size=20')
        self.write_line(channel, b'RCPT to: <foo@example.com> foo=bar')
        self.assertEqual(channel.socket.last, self.error_response)

    def test_nothing_accepted(self):
        server = DummyServer((support.HOST, 0), ('b', 0), decode_data=False)
        conn, addr = server.accept()
        channel = smtpd.SMTPChannel(server, conn, addr, decode_data=False)
        self.write_line(channel, b'EHLO example')
        self.write_line(channel, b'MAIL from: <foo@example.com> size=20')
        self.write_line(channel, b'RCPT to: <foo@example.com>')
        self.assertEqual(channel.socket.last, b'250 OK\r\n')


class TestMailOptionParsing(unittest.TestCase):
    error_response = (b'555 MAIL FROM parameters not recognized or not '
                      b'implemented\r\n')

    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket
        self.old_debugstream = smtpd.DEBUGSTREAM
        self.debug = smtpd.DEBUGSTREAM = io.StringIO()

    def tearDown(self):
        asyncore.close_all()
        asyncore.socket = smtpd.socket = socket
        smtpd.DEBUGSTREAM = self.old_debugstream

    def write_line(self, channel, line):
        channel.socket.queue_recv(line)
        channel.handle_read()

    def test_with_decode_data_true(self):
        server = DummyServer((support.HOST, 0), ('b', 0), decode_data=True)
        conn, addr = server.accept()
        channel = smtpd.SMTPChannel(server, conn, addr, decode_data=True)
        self.write_line(channel, b'EHLO example')
        for line in [
            b'MAIL from: <foo@example.com> size=20 SMTPUTF8',
            b'MAIL from: <foo@example.com> size=20 SMTPUTF8 BODY=8BITMIME',
            b'MAIL from: <foo@example.com> size=20 BODY=UNKNOWN',
            b'MAIL from: <foo@example.com> size=20 body=8bitmime',
        ]:
            self.write_line(channel, line)
            self.assertEqual(channel.socket.last, self.error_response)
        self.write_line(channel, b'MAIL from: <foo@example.com> size=20')
        self.assertEqual(channel.socket.last, b'250 OK\r\n')

    def test_with_decode_data_false(self):
        server = DummyServer((support.HOST, 0), ('b', 0), decode_data=False)
        conn, addr = server.accept()
        channel = smtpd.SMTPChannel(server, conn, addr, decode_data=False)
        self.write_line(channel, b'EHLO example')
        for line in [
            b'MAIL from: <foo@example.com> size=20 SMTPUTF8',
            b'MAIL from: <foo@example.com> size=20 SMTPUTF8 BODY=8BITMIME',
        ]:
            self.write_line(channel, line)
            self.assertEqual(channel.socket.last, self.error_response)
        self.write_line(
            channel,
            b'MAIL from: <foo@example.com> size=20 SMTPUTF8 BODY=UNKNOWN')
        self.assertEqual(
            channel.socket.last,
            b'501 Error: BODY can only be one of 7BIT, 8BITMIME\r\n')
        self.write_line(
            channel, b'MAIL from: <foo@example.com> size=20 body=8bitmime')
        self.assertEqual(channel.socket.last, b'250 OK\r\n')

    def test_with_enable_smtputf8_true(self):
        server = DummyServer((support.HOST, 0), ('b', 0), enable_SMTPUTF8=True)
        conn, addr = server.accept()
        channel = smtpd.SMTPChannel(server, conn, addr, enable_SMTPUTF8=True)
        self.write_line(channel, b'EHLO example')
        self.write_line(
            channel,
            b'MAIL from: <foo@example.com> size=20 body=8bitmime smtputf8')
        self.assertEqual(channel.socket.last, b'250 OK\r\n')


class SMTPDChannelTest(unittest.TestCase):
    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket
        self.old_debugstream = smtpd.DEBUGSTREAM
        self.debug = smtpd.DEBUGSTREAM = io.StringIO()
        self.server = DummyServer((support.HOST, 0), ('b', 0),
                                  decode_data=True)
        conn, addr = self.server.accept()
        self.channel = smtpd.SMTPChannel(self.server, conn, addr,
                                         decode_data=True)

    def tearDown(self):
        asyncore.close_all()
        asyncore.socket = smtpd.socket = socket
        smtpd.DEBUGSTREAM = self.old_debugstream

    def write_line(self, line):
        self.channel.socket.queue_recv(line)
        self.channel.handle_read()

    def test_broken_connect(self):
        self.assertRaises(
            DummyDispatcherBroken, BrokenDummyServer,
            (support.HOST, 0), ('b', 0), decode_data=True)

    def test_server_accept(self):
        self.server.handle_accept()

    def test_missing_data(self):
        self.write_line(b'')
        self.assertEqual(self.channel.socket.last,
                         b'500 Error: bad syntax\r\n')

    def test_EHLO(self):
        self.write_line(b'EHLO example')
        self.assertEqual(self.channel.socket.last, b'250 HELP\r\n')

    def test_EHLO_bad_syntax(self):
        self.write_line(b'EHLO')
        self.assertEqual(self.channel.socket.last,
                         b'501 Syntax: EHLO hostname\r\n')

    def test_EHLO_duplicate(self):
        self.write_line(b'EHLO example')
        self.write_line(b'EHLO example')
        self.assertEqual(self.channel.socket.last,
                         b'503 Duplicate HELO/EHLO\r\n')

    def test_EHLO_HELO_duplicate(self):
        self.write_line(b'EHLO example')
        self.write_line(b'HELO example')
        self.assertEqual(self.channel.socket.last,
                         b'503 Duplicate HELO/EHLO\r\n')

    def test_HELO(self):
        name = smtpd.socket.getfqdn()
        self.write_line(b'HELO example')
        self.assertEqual(self.channel.socket.last,
                         '250 {}\r\n'.format(name).encode('ascii'))

    def test_HELO_EHLO_duplicate(self):
        self.write_line(b'HELO example')
        self.write_line(b'EHLO example')
        self.assertEqual(self.channel.socket.last,
                         b'503 Duplicate HELO/EHLO\r\n')

    def test_HELP(self):
        self.write_line(b'HELP')
        self.assertEqual(self.channel.socket.last,
                         b'250 Supported commands: EHLO HELO MAIL RCPT ' + \
                         b'DATA RSET NOOP QUIT VRFY\r\n')

    def test_HELP_command(self):
        self.write_line(b'HELP MAIL')
        self.assertEqual(self.channel.socket.last,
                         b'250 Syntax: MAIL FROM: <address>\r\n')

    def test_HELP_command_unknown(self):
        self.write_line(b'HELP SPAM')
        self.assertEqual(self.channel.socket.last,
                         b'501 Supported commands: EHLO HELO MAIL RCPT ' + \
                         b'DATA RSET NOOP QUIT VRFY\r\n')

    def test_HELO_bad_syntax(self):
        self.write_line(b'HELO')
        self.assertEqual(self.channel.socket.last,
                         b'501 Syntax: HELO hostname\r\n')

    def test_HELO_duplicate(self):
        self.write_line(b'HELO example')
        self.write_line(b'HELO example')
        self.assertEqual(self.channel.socket.last,
                         b'503 Duplicate HELO/EHLO\r\n')

    def test_HELO_parameter_rejected_when_extensions_not_enabled(self):
        self.extended_smtp = False
        self.write_line(b'HELO example')
        self.write_line(b'MAIL from:<foo@example.com> SIZE=1234')
        self.assertEqual(self.channel.socket.last,
                         b'501 Syntax: MAIL FROM: <address>\r\n')

    def test_MAIL_allows_space_after_colon(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL from:   <foo@example.com>')
        self.assertEqual(self.channel.socket.last,
                         b'250 OK\r\n')

    def test_extended_MAIL_allows_space_after_colon(self):
        self.write_line(b'EHLO example')
        self.write_line(b'MAIL from:   <foo@example.com> size=20')
        self.assertEqual(self.channel.socket.last,
                         b'250 OK\r\n')

    def test_NOOP(self):
        self.write_line(b'NOOP')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')

    def test_HELO_NOOP(self):
        self.write_line(b'HELO example')
        self.write_line(b'NOOP')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')

    def test_NOOP_bad_syntax(self):
        self.write_line(b'NOOP hi')
        self.assertEqual(self.channel.socket.last,
                         b'501 Syntax: NOOP\r\n')

    def test_QUIT(self):
        self.write_line(b'QUIT')
        self.assertEqual(self.channel.socket.last, b'221 Bye\r\n')

    def test_HELO_QUIT(self):
        self.write_line(b'HELO example')
        self.write_line(b'QUIT')
        self.assertEqual(self.channel.socket.last, b'221 Bye\r\n')

    def test_QUIT_arg_ignored(self):
        self.write_line(b'QUIT bye bye')
        self.assertEqual(self.channel.socket.last, b'221 Bye\r\n')

    def test_bad_state(self):
        self.channel.smtp_state = 'BAD STATE'
        self.write_line(b'HELO example')
        self.assertEqual(self.channel.socket.last,
                         b'451 Internal confusion\r\n')

    def test_command_too_long(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL from: ' +
                        b'a' * self.channel.command_size_limit +
                        b'@example')
        self.assertEqual(self.channel.socket.last,
                         b'500 Error: line too long\r\n')

    def test_MAIL_command_limit_extended_with_SIZE(self):
        self.write_line(b'EHLO example')
        fill_len = self.channel.command_size_limit - len('MAIL from:<@example>')
        self.write_line(b'MAIL from:<' +
                        b'a' * fill_len +
                        b'@example> SIZE=1234')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')

        self.write_line(b'MAIL from:<' +
                        b'a' * (fill_len + 26) +
                        b'@example> SIZE=1234')
        self.assertEqual(self.channel.socket.last,
                         b'500 Error: line too long\r\n')

    def test_MAIL_command_rejects_SMTPUTF8_by_default(self):
        self.write_line(b'EHLO example')
        self.write_line(
            b'MAIL from: <naive@example.com> BODY=8BITMIME SMTPUTF8')
        self.assertEqual(self.channel.socket.last[0:1], b'5')

    def test_data_longer_than_default_data_size_limit(self):
        # Hack the default so we don't have to generate so much data.
        self.channel.data_size_limit = 1048
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'DATA')
        self.write_line(b'A' * self.channel.data_size_limit +
                        b'A\r\n.')
        self.assertEqual(self.channel.socket.last,
                         b'552 Error: Too much mail data\r\n')

    def test_MAIL_size_parameter(self):
        self.write_line(b'EHLO example')
        self.write_line(b'MAIL FROM:<eggs@example> SIZE=512')
        self.assertEqual(self.channel.socket.last,
                         b'250 OK\r\n')

    def test_MAIL_invalid_size_parameter(self):
        self.write_line(b'EHLO example')
        self.write_line(b'MAIL FROM:<eggs@example> SIZE=invalid')
        self.assertEqual(self.channel.socket.last,
            b'501 Syntax: MAIL FROM: <address> [SP <mail-parameters>]\r\n')

    def test_MAIL_RCPT_unknown_parameters(self):
        self.write_line(b'EHLO example')
        self.write_line(b'MAIL FROM:<eggs@example> ham=green')
        self.assertEqual(self.channel.socket.last,
            b'555 MAIL FROM parameters not recognized or not implemented\r\n')

        self.write_line(b'MAIL FROM:<eggs@example>')
        self.write_line(b'RCPT TO:<eggs@example> ham=green')
        self.assertEqual(self.channel.socket.last,
            b'555 RCPT TO parameters not recognized or not implemented\r\n')

    def test_MAIL_size_parameter_larger_than_default_data_size_limit(self):
        self.channel.data_size_limit = 1048
        self.write_line(b'EHLO example')
        self.write_line(b'MAIL FROM:<eggs@example> SIZE=2096')
        self.assertEqual(self.channel.socket.last,
            b'552 Error: message size exceeds fixed maximum message size\r\n')

    def test_need_MAIL(self):
        self.write_line(b'HELO example')
        self.write_line(b'RCPT to:spam@example')
        self.assertEqual(self.channel.socket.last,
            b'503 Error: need MAIL command\r\n')

    def test_MAIL_syntax_HELO(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL from eggs@example')
        self.assertEqual(self.channel.socket.last,
            b'501 Syntax: MAIL FROM: <address>\r\n')

    def test_MAIL_syntax_EHLO(self):
        self.write_line(b'EHLO example')
        self.write_line(b'MAIL from eggs@example')
        self.assertEqual(self.channel.socket.last,
            b'501 Syntax: MAIL FROM: <address> [SP <mail-parameters>]\r\n')

    def test_MAIL_missing_address(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL from:')
        self.assertEqual(self.channel.socket.last,
            b'501 Syntax: MAIL FROM: <address>\r\n')

    def test_MAIL_chevrons(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL from:<eggs@example>')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')

    def test_MAIL_empty_chevrons(self):
        self.write_line(b'EHLO example')
        self.write_line(b'MAIL from:<>')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')

    def test_MAIL_quoted_localpart(self):
        self.write_line(b'EHLO example')
        self.write_line(b'MAIL from: <"Fred Blogs"@example.com>')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')
        self.assertEqual(self.channel.mailfrom, '"Fred Blogs"@example.com')

    def test_MAIL_quoted_localpart_no_angles(self):
        self.write_line(b'EHLO example')
        self.write_line(b'MAIL from: "Fred Blogs"@example.com')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')
        self.assertEqual(self.channel.mailfrom, '"Fred Blogs"@example.com')

    def test_MAIL_quoted_localpart_with_size(self):
        self.write_line(b'EHLO example')
        self.write_line(b'MAIL from: <"Fred Blogs"@example.com> SIZE=1000')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')
        self.assertEqual(self.channel.mailfrom, '"Fred Blogs"@example.com')

    def test_MAIL_quoted_localpart_with_size_no_angles(self):
        self.write_line(b'EHLO example')
        self.write_line(b'MAIL from: "Fred Blogs"@example.com SIZE=1000')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')
        self.assertEqual(self.channel.mailfrom, '"Fred Blogs"@example.com')

    def test_nested_MAIL(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL from:eggs@example')
        self.write_line(b'MAIL from:spam@example')
        self.assertEqual(self.channel.socket.last,
            b'503 Error: nested MAIL command\r\n')

    def test_VRFY(self):
        self.write_line(b'VRFY eggs@example')
        self.assertEqual(self.channel.socket.last,
            b'252 Cannot VRFY user, but will accept message and attempt ' + \
            b'delivery\r\n')

    def test_VRFY_syntax(self):
        self.write_line(b'VRFY')
        self.assertEqual(self.channel.socket.last,
            b'501 Syntax: VRFY <address>\r\n')

    def test_EXPN_not_implemented(self):
        self.write_line(b'EXPN')
        self.assertEqual(self.channel.socket.last,
            b'502 EXPN not implemented\r\n')

    def test_no_HELO_MAIL(self):
        self.write_line(b'MAIL from:<foo@example.com>')
        self.assertEqual(self.channel.socket.last,
                         b'503 Error: send HELO first\r\n')

    def test_need_RCPT(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'DATA')
        self.assertEqual(self.channel.socket.last,
            b'503 Error: need RCPT command\r\n')

    def test_RCPT_syntax_HELO(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From: eggs@example')
        self.write_line(b'RCPT to eggs@example')
        self.assertEqual(self.channel.socket.last,
            b'501 Syntax: RCPT TO: <address>\r\n')

    def test_RCPT_syntax_EHLO(self):
        self.write_line(b'EHLO example')
        self.write_line(b'MAIL From: eggs@example')
        self.write_line(b'RCPT to eggs@example')
        self.assertEqual(self.channel.socket.last,
            b'501 Syntax: RCPT TO: <address> [SP <mail-parameters>]\r\n')

    def test_RCPT_lowercase_to_OK(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From: eggs@example')
        self.write_line(b'RCPT to: <eggs@example>')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')

    def test_no_HELO_RCPT(self):
        self.write_line(b'RCPT to eggs@example')
        self.assertEqual(self.channel.socket.last,
                         b'503 Error: send HELO first\r\n')

    def test_data_dialog(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')
        self.write_line(b'RCPT To:spam@example')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')

        self.write_line(b'DATA')
        self.assertEqual(self.channel.socket.last,
            b'354 End data with <CR><LF>.<CR><LF>\r\n')
        self.write_line(b'data\r\nmore\r\n.')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')
        self.assertEqual(self.server.messages,
            [(('peer-address', 'peer-port'),
              'eggs@example',
              ['spam@example'],
              'data\nmore')])

    def test_DATA_syntax(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'DATA spam')
        self.assertEqual(self.channel.socket.last, b'501 Syntax: DATA\r\n')

    def test_no_HELO_DATA(self):
        self.write_line(b'DATA spam')
        self.assertEqual(self.channel.socket.last,
                         b'503 Error: send HELO first\r\n')

    def test_data_transparency_section_4_5_2(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'DATA')
        self.write_line(b'..\r\n.\r\n')
        self.assertEqual(self.channel.received_data, '.')

    def test_multiple_RCPT(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'RCPT To:ham@example')
        self.write_line(b'DATA')
        self.write_line(b'data\r\n.')
        self.assertEqual(self.server.messages,
            [(('peer-address', 'peer-port'),
              'eggs@example',
              ['spam@example','ham@example'],
              'data')])

    def test_manual_status(self):
        # checks that the Channel is able to return a custom status message
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'DATA')
        self.write_line(b'return status\r\n.')
        self.assertEqual(self.channel.socket.last, b'250 Okish\r\n')

    def test_RSET(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'RSET')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')
        self.write_line(b'MAIL From:foo@example')
        self.write_line(b'RCPT To:eggs@example')
        self.write_line(b'DATA')
        self.write_line(b'data\r\n.')
        self.assertEqual(self.server.messages,
            [(('peer-address', 'peer-port'),
               'foo@example',
               ['eggs@example'],
               'data')])

    def test_HELO_RSET(self):
        self.write_line(b'HELO example')
        self.write_line(b'RSET')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')

    def test_RSET_syntax(self):
        self.write_line(b'RSET hi')
        self.assertEqual(self.channel.socket.last, b'501 Syntax: RSET\r\n')

    def test_unknown_command(self):
        self.write_line(b'UNKNOWN_CMD')
        self.assertEqual(self.channel.socket.last,
                         b'500 Error: command "UNKNOWN_CMD" not ' + \
                         b'recognized\r\n')

    def test_attribute_deprecations(self):
        with support.check_warnings(('', DeprecationWarning)):
            spam = self.channel._SMTPChannel__server
        with support.check_warnings(('', DeprecationWarning)):
            self.channel._SMTPChannel__server = 'spam'
        with support.check_warnings(('', DeprecationWarning)):
            spam = self.channel._SMTPChannel__line
        with support.check_warnings(('', DeprecationWarning)):
            self.channel._SMTPChannel__line = 'spam'
        with support.check_warnings(('', DeprecationWarning)):
            spam = self.channel._SMTPChannel__state
        with support.check_warnings(('', DeprecationWarning)):
            self.channel._SMTPChannel__state = 'spam'
        with support.check_warnings(('', DeprecationWarning)):
            spam = self.channel._SMTPChannel__greeting
        with support.check_warnings(('', DeprecationWarning)):
            self.channel._SMTPChannel__greeting = 'spam'
        with support.check_warnings(('', DeprecationWarning)):
            spam = self.channel._SMTPChannel__mailfrom
        with support.check_warnings(('', DeprecationWarning)):
            self.channel._SMTPChannel__mailfrom = 'spam'
        with support.check_warnings(('', DeprecationWarning)):
            spam = self.channel._SMTPChannel__rcpttos
        with support.check_warnings(('', DeprecationWarning)):
            self.channel._SMTPChannel__rcpttos = 'spam'
        with support.check_warnings(('', DeprecationWarning)):
            spam = self.channel._SMTPChannel__data
        with support.check_warnings(('', DeprecationWarning)):
            self.channel._SMTPChannel__data = 'spam'
        with support.check_warnings(('', DeprecationWarning)):
            spam = self.channel._SMTPChannel__fqdn
        with support.check_warnings(('', DeprecationWarning)):
            self.channel._SMTPChannel__fqdn = 'spam'
        with support.check_warnings(('', DeprecationWarning)):
            spam = self.channel._SMTPChannel__peer
        with support.check_warnings(('', DeprecationWarning)):
            self.channel._SMTPChannel__peer = 'spam'
        with support.check_warnings(('', DeprecationWarning)):
            spam = self.channel._SMTPChannel__conn
        with support.check_warnings(('', DeprecationWarning)):
            self.channel._SMTPChannel__conn = 'spam'
        with support.check_warnings(('', DeprecationWarning)):
            spam = self.channel._SMTPChannel__addr
        with support.check_warnings(('', DeprecationWarning)):
            self.channel._SMTPChannel__addr = 'spam'

    def test_decode_data_default_warning(self):
        with self.assertWarns(DeprecationWarning):
            server = DummyServer((support.HOST, 0), ('b', 0))
        conn, addr = self.server.accept()
        with self.assertWarns(DeprecationWarning):
            smtpd.SMTPChannel(server, conn, addr)

@unittest.skipUnless(support.IPV6_ENABLED, "IPv6 not enabled")
class SMTPDChannelIPv6Test(SMTPDChannelTest):
    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket
        self.old_debugstream = smtpd.DEBUGSTREAM
        self.debug = smtpd.DEBUGSTREAM = io.StringIO()
        self.server = DummyServer((support.HOSTv6, 0), ('b', 0),
                                  decode_data=True)
        conn, addr = self.server.accept()
        self.channel = smtpd.SMTPChannel(self.server, conn, addr,
                                         decode_data=True)

class SMTPDChannelWithDataSizeLimitTest(unittest.TestCase):

    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket
        self.old_debugstream = smtpd.DEBUGSTREAM
        self.debug = smtpd.DEBUGSTREAM = io.StringIO()
        self.server = DummyServer((support.HOST, 0), ('b', 0),
                                  decode_data=True)
        conn, addr = self.server.accept()
        # Set DATA size limit to 32 bytes for easy testing
        self.channel = smtpd.SMTPChannel(self.server, conn, addr, 32,
                                         decode_data=True)

    def tearDown(self):
        asyncore.close_all()
        asyncore.socket = smtpd.socket = socket
        smtpd.DEBUGSTREAM = self.old_debugstream

    def write_line(self, line):
        self.channel.socket.queue_recv(line)
        self.channel.handle_read()

    def test_data_limit_dialog(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')
        self.write_line(b'RCPT To:spam@example')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')

        self.write_line(b'DATA')
        self.assertEqual(self.channel.socket.last,
            b'354 End data with <CR><LF>.<CR><LF>\r\n')
        self.write_line(b'data\r\nmore\r\n.')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')
        self.assertEqual(self.server.messages,
            [(('peer-address', 'peer-port'),
              'eggs@example',
              ['spam@example'],
              'data\nmore')])

    def test_data_limit_dialog_too_much_data(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')
        self.write_line(b'RCPT To:spam@example')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')

        self.write_line(b'DATA')
        self.assertEqual(self.channel.socket.last,
            b'354 End data with <CR><LF>.<CR><LF>\r\n')
        self.write_line(b'This message is longer than 32 bytes\r\n.')
        self.assertEqual(self.channel.socket.last,
                         b'552 Error: Too much mail data\r\n')


class SMTPDChannelWithDecodeDataFalse(unittest.TestCase):

    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket
        self.old_debugstream = smtpd.DEBUGSTREAM
        self.debug = smtpd.DEBUGSTREAM = io.StringIO()
        self.server = DummyServer((support.HOST, 0), ('b', 0),
                                  decode_data=False)
        conn, addr = self.server.accept()
        # Set decode_data to False
        self.channel = smtpd.SMTPChannel(self.server, conn, addr,
                decode_data=False)

    def tearDown(self):
        asyncore.close_all()
        asyncore.socket = smtpd.socket = socket
        smtpd.DEBUGSTREAM = self.old_debugstream

    def write_line(self, line):
        self.channel.socket.queue_recv(line)
        self.channel.handle_read()

    def test_ascii_data(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'DATA')
        self.write_line(b'plain ascii text')
        self.write_line(b'.')
        self.assertEqual(self.channel.received_data, b'plain ascii text')

    def test_utf8_data(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'DATA')
        self.write_line(b'utf8 enriched text: \xc5\xbc\xc5\xba\xc4\x87')
        self.write_line(b'and some plain ascii')
        self.write_line(b'.')
        self.assertEqual(
            self.channel.received_data,
            b'utf8 enriched text: \xc5\xbc\xc5\xba\xc4\x87\n'
                b'and some plain ascii')


class SMTPDChannelWithDecodeDataTrue(unittest.TestCase):

    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket
        self.old_debugstream = smtpd.DEBUGSTREAM
        self.debug = smtpd.DEBUGSTREAM = io.StringIO()
        self.server = DummyServer((support.HOST, 0), ('b', 0),
                                  decode_data=True)
        conn, addr = self.server.accept()
        # Set decode_data to True
        self.channel = smtpd.SMTPChannel(self.server, conn, addr,
                decode_data=True)

    def tearDown(self):
        asyncore.close_all()
        asyncore.socket = smtpd.socket = socket
        smtpd.DEBUGSTREAM = self.old_debugstream

    def write_line(self, line):
        self.channel.socket.queue_recv(line)
        self.channel.handle_read()

    def test_ascii_data(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'DATA')
        self.write_line(b'plain ascii text')
        self.write_line(b'.')
        self.assertEqual(self.channel.received_data, 'plain ascii text')

    def test_utf8_data(self):
        self.write_line(b'HELO example')
        self.write_line(b'MAIL From:eggs@example')
        self.write_line(b'RCPT To:spam@example')
        self.write_line(b'DATA')
        self.write_line(b'utf8 enriched text: \xc5\xbc\xc5\xba\xc4\x87')
        self.write_line(b'and some plain ascii')
        self.write_line(b'.')
        self.assertEqual(
            self.channel.received_data,
            'utf8 enriched text: żźć\nand some plain ascii')


class SMTPDChannelTestWithEnableSMTPUTF8True(unittest.TestCase):
    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket
        self.old_debugstream = smtpd.DEBUGSTREAM
        self.debug = smtpd.DEBUGSTREAM = io.StringIO()
        self.server = DummyServer((support.HOST, 0), ('b', 0),
                                  enable_SMTPUTF8=True)
        conn, addr = self.server.accept()
        self.channel = smtpd.SMTPChannel(self.server, conn, addr,
                                         enable_SMTPUTF8=True)

    def tearDown(self):
        asyncore.close_all()
        asyncore.socket = smtpd.socket = socket
        smtpd.DEBUGSTREAM = self.old_debugstream

    def write_line(self, line):
        self.channel.socket.queue_recv(line)
        self.channel.handle_read()

    def test_MAIL_command_accepts_SMTPUTF8_when_announced(self):
        self.write_line(b'EHLO example')
        self.write_line(
            'MAIL from: <naïve@example.com> BODY=8BITMIME SMTPUTF8'.encode(
                'utf-8')
        )
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')

    def test_process_smtputf8_message(self):
        self.write_line(b'EHLO example')
        for mail_parameters in [b'', b'BODY=8BITMIME SMTPUTF8']:
            self.write_line(b'MAIL from: <a@example> ' + mail_parameters)
            self.assertEqual(self.channel.socket.last[0:3], b'250')
            self.write_line(b'rcpt to:<b@example.com>')
            self.assertEqual(self.channel.socket.last[0:3], b'250')
            self.write_line(b'data')
            self.assertEqual(self.channel.socket.last[0:3], b'354')
            self.write_line(b'c\r\n.')
            if mail_parameters == b'':
                self.assertEqual(self.channel.socket.last, b'250 OK\r\n')
            else:
                self.assertEqual(self.channel.socket.last,
                                 b'250 SMTPUTF8 message okish\r\n')

    def test_utf8_data(self):
        self.write_line(b'EHLO example')
        self.write_line(
            'MAIL From: naïve@examplé BODY=8BITMIME SMTPUTF8'.encode('utf-8'))
        self.assertEqual(self.channel.socket.last[0:3], b'250')
        self.write_line('RCPT To:späm@examplé'.encode('utf-8'))
        self.assertEqual(self.channel.socket.last[0:3], b'250')
        self.write_line(b'DATA')
        self.assertEqual(self.channel.socket.last[0:3], b'354')
        self.write_line(b'utf8 enriched text: \xc5\xbc\xc5\xba\xc4\x87')
        self.write_line(b'.')
        self.assertEqual(
            self.channel.received_data,
            b'utf8 enriched text: \xc5\xbc\xc5\xba\xc4\x87')

    def test_MAIL_command_limit_extended_with_SIZE_and_SMTPUTF8(self):
        self.write_line(b'ehlo example')
        fill_len = (512 + 26 + 10) - len('mail from:<@example>')
        self.write_line(b'MAIL from:<' +
                        b'a' * (fill_len + 1) +
                        b'@example>')
        self.assertEqual(self.channel.socket.last,
                         b'500 Error: line too long\r\n')
        self.write_line(b'MAIL from:<' +
                        b'a' * fill_len +
                        b'@example>')
        self.assertEqual(self.channel.socket.last, b'250 OK\r\n')

    def test_multiple_emails_with_extended_command_length(self):
        self.write_line(b'ehlo example')
        fill_len = (512 + 26 + 10) - len('mail from:<@example>')
        for char in [b'a', b'b', b'c']:
            self.write_line(b'MAIL from:<' + char * fill_len + b'a@example>')
            self.assertEqual(self.channel.socket.last[0:3], b'500')
            self.write_line(b'MAIL from:<' + char * fill_len + b'@example>')
            self.assertEqual(self.channel.socket.last[0:3], b'250')
            self.write_line(b'rcpt to:<hans@example.com>')
            self.assertEqual(self.channel.socket.last[0:3], b'250')
            self.write_line(b'data')
            self.assertEqual(self.channel.socket.last[0:3], b'354')
            self.write_line(b'test\r\n.')
            self.assertEqual(self.channel.socket.last[0:3], b'250')

if __name__ == "__main__":
    unittest.main()
