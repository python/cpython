import unittest
from test import support, mock_socket
import socket
import io
import smtpd
import asyncore


class DummyServer(smtpd.SMTPServer):
    def __init__(self, localaddr, remoteaddr):
        smtpd.SMTPServer.__init__(self, localaddr, remoteaddr)
        self.messages = []

    def process_message(self, peer, mailfrom, rcpttos, data):
        self.messages.append((peer, mailfrom, rcpttos, data))
        if data == 'return status':
            return '250 Okish'


class DummyDispatcherBroken(Exception):
    pass


class BrokenDummyServer(DummyServer):
    def listen(self, num):
        raise DummyDispatcherBroken()


class SMTPDServerTest(unittest.TestCase):
    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket

    def test_process_message_unimplemented(self):
        server = smtpd.SMTPServer('a', 'b')
        conn, addr = server.accept()
        channel = smtpd.SMTPChannel(server, conn, addr)

        def write_line(line):
            channel.socket.queue_recv(line)
            channel.handle_read()

        write_line(b'HELO example')
        write_line(b'MAIL From:eggs@example')
        write_line(b'RCPT To:spam@example')
        write_line(b'DATA')
        self.assertRaises(NotImplementedError, write_line, b'spam\r\n.\r\n')

    def tearDown(self):
        asyncore.close_all()
        asyncore.socket = smtpd.socket = socket


class SMTPDChannelTest(unittest.TestCase):
    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket
        self.old_debugstream = smtpd.DEBUGSTREAM
        self.debug = smtpd.DEBUGSTREAM = io.StringIO()
        self.server = DummyServer('a', 'b')
        conn, addr = self.server.accept()
        self.channel = smtpd.SMTPChannel(self.server, conn, addr)

    def tearDown(self):
        asyncore.close_all()
        asyncore.socket = smtpd.socket = socket
        smtpd.DEBUGSTREAM = self.old_debugstream

    def write_line(self, line):
        self.channel.socket.queue_recv(line)
        self.channel.handle_read()

    def test_broken_connect(self):
        self.assertRaises(DummyDispatcherBroken, BrokenDummyServer, 'a', 'b')

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
            [('peer', 'eggs@example', ['spam@example'], 'data\nmore')])

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
            [('peer', 'eggs@example', ['spam@example','ham@example'], 'data')])

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
            [('peer', 'foo@example', ['eggs@example'], 'data')])

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


class SMTPDChannelWithDataSizeLimitTest(unittest.TestCase):

    def setUp(self):
        smtpd.socket = asyncore.socket = mock_socket
        self.old_debugstream = smtpd.DEBUGSTREAM
        self.debug = smtpd.DEBUGSTREAM = io.StringIO()
        self.server = DummyServer('a', 'b')
        conn, addr = self.server.accept()
        # Set DATA size limit to 32 bytes for easy testing
        self.channel = smtpd.SMTPChannel(self.server, conn, addr, 32)

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
            [('peer', 'eggs@example', ['spam@example'], 'data\nmore')])

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


if __name__ == "__main__":
    unittest.main()
