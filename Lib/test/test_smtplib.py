import base64
import email.mime.text
from email.message import EmailMessage
from email.base64mime import body_encode as encode_base64
import email.utils
import hashlib
import hmac
import socket
import smtplib
import io
import re
import sys
import time
import select
import errno
import textwrap
import threading

import unittest
from test import support, mock_socket
from test.support import _asyncore as asyncore
from test.support import _smtpd as smtpd
from test.support import hashlib_helper
from test.support import socket_helper
from test.support import threading_helper
from unittest.mock import Mock

HOST = socket_helper.HOST

if sys.platform == 'darwin':
    # select.poll returns a select.POLLHUP at the end of the tests
    # on darwin, so just ignore it
    def handle_expt(self):
        pass
    smtpd.SMTPChannel.handle_expt = handle_expt


def server(evt, buf, serv):
    serv.listen()
    evt.set()
    try:
        conn, addr = serv.accept()
    except TimeoutError:
        pass
    else:
        n = 500
        while buf and n > 0:
            r, w, e = select.select([], [conn], [])
            if w:
                sent = conn.send(buf)
                buf = buf[sent:]

            n -= 1

        conn.close()
    finally:
        serv.close()
        evt.set()

class GeneralTests:

    def setUp(self):
        smtplib.socket = mock_socket
        self.port = 25

    def tearDown(self):
        smtplib.socket = socket

    # This method is no longer used but is retained for backward compatibility,
    # so test to make sure it still works.
    def testQuoteData(self):
        teststr  = "abc\n.jkl\rfoo\r\n..blue"
        expected = "abc\r\n..jkl\r\nfoo\r\n...blue"
        self.assertEqual(expected, smtplib.quotedata(teststr))

    def testBasic1(self):
        mock_socket.reply_with(b"220 Hola mundo")
        # connects
        client = self.client(HOST, self.port)
        client.close()

    def testSourceAddress(self):
        mock_socket.reply_with(b"220 Hola mundo")
        # connects
        client = self.client(HOST, self.port,
                             source_address=('127.0.0.1',19876))
        self.assertEqual(client.source_address, ('127.0.0.1', 19876))
        client.close()

    def testBasic2(self):
        mock_socket.reply_with(b"220 Hola mundo")
        # connects, include port in host name
        client = self.client("%s:%s" % (HOST, self.port))
        client.close()

    def testLocalHostName(self):
        mock_socket.reply_with(b"220 Hola mundo")
        # check that supplied local_hostname is used
        client = self.client(HOST, self.port, local_hostname="testhost")
        self.assertEqual(client.local_hostname, "testhost")
        client.close()

    def testTimeoutDefault(self):
        mock_socket.reply_with(b"220 Hola mundo")
        self.assertIsNone(mock_socket.getdefaulttimeout())
        mock_socket.setdefaulttimeout(30)
        self.assertEqual(mock_socket.getdefaulttimeout(), 30)
        try:
            client = self.client(HOST, self.port)
        finally:
            mock_socket.setdefaulttimeout(None)
        self.assertEqual(client.sock.gettimeout(), 30)
        client.close()

    def testTimeoutNone(self):
        mock_socket.reply_with(b"220 Hola mundo")
        self.assertIsNone(socket.getdefaulttimeout())
        socket.setdefaulttimeout(30)
        try:
            client = self.client(HOST, self.port, timeout=None)
        finally:
            socket.setdefaulttimeout(None)
        self.assertIsNone(client.sock.gettimeout())
        client.close()

    def testTimeoutZero(self):
        mock_socket.reply_with(b"220 Hola mundo")
        with self.assertRaises(ValueError):
            self.client(HOST, self.port, timeout=0)

    def testTimeoutValue(self):
        mock_socket.reply_with(b"220 Hola mundo")
        client = self.client(HOST, self.port, timeout=30)
        self.assertEqual(client.sock.gettimeout(), 30)
        client.close()

    def test_debuglevel(self):
        mock_socket.reply_with(b"220 Hello world")
        client = self.client()
        client.set_debuglevel(1)
        with support.captured_stderr() as stderr:
            client.connect(HOST, self.port)
        client.close()
        expected = re.compile(r"^connect:", re.MULTILINE)
        self.assertRegex(stderr.getvalue(), expected)

    def test_debuglevel_2(self):
        mock_socket.reply_with(b"220 Hello world")
        client = self.client()
        client.set_debuglevel(2)
        with support.captured_stderr() as stderr:
            client.connect(HOST, self.port)
        client.close()
        expected = re.compile(r"^\d{2}:\d{2}:\d{2}\.\d{6} connect: ",
                              re.MULTILINE)
        self.assertRegex(stderr.getvalue(), expected)


class SMTPGeneralTests(GeneralTests, unittest.TestCase):

    client = smtplib.SMTP


class LMTPGeneralTests(GeneralTests, unittest.TestCase):

    client = smtplib.LMTP

    @unittest.skipUnless(hasattr(socket, 'AF_UNIX'), "test requires Unix domain socket")
    def testUnixDomainSocketTimeoutDefault(self):
        local_host = '/some/local/lmtp/delivery/program'
        mock_socket.reply_with(b"220 Hello world")
        try:
            client = self.client(local_host, self.port)
        finally:
            mock_socket.setdefaulttimeout(None)
        self.assertIsNone(client.sock.gettimeout())
        client.close()

    def testTimeoutZero(self):
        super().testTimeoutZero()
        local_host = '/some/local/lmtp/delivery/program'
        with self.assertRaises(ValueError):
            self.client(local_host, timeout=0)

# Test server thread using the specified SMTP server class
def debugging_server(serv, serv_evt, client_evt):
    serv_evt.set()

    try:
        if hasattr(select, 'poll'):
            poll_fun = asyncore.poll2
        else:
            poll_fun = asyncore.poll

        n = 1000
        while asyncore.socket_map and n > 0:
            poll_fun(0.01, asyncore.socket_map)

            # when the client conversation is finished, it will
            # set client_evt, and it's then ok to kill the server
            if client_evt.is_set():
                serv.close()
                break

            n -= 1

    except TimeoutError:
        pass
    finally:
        if not client_evt.is_set():
            # allow some time for the client to read the result
            time.sleep(0.5)
            serv.close()
        asyncore.close_all()
        serv_evt.set()

MSG_BEGIN = '---------- MESSAGE FOLLOWS ----------\n'
MSG_END = '------------ END MESSAGE ------------\n'

# NOTE: Some SMTP objects in the tests below are created with a non-default
# local_hostname argument to the constructor, since (on some systems) the FQDN
# lookup caused by the default local_hostname sometimes takes so long that the
# test server times out, causing the test to fail.

# Test behavior of smtpd.DebuggingServer
class DebuggingServerTests(unittest.TestCase):

    maxDiff = None

    def setUp(self):
        self.thread_key = threading_helper.threading_setup()
        self.real_getfqdn = socket.getfqdn
        socket.getfqdn = mock_socket.getfqdn
        # temporarily replace sys.stdout to capture DebuggingServer output
        self.old_stdout = sys.stdout
        self.output = io.StringIO()
        sys.stdout = self.output

        self.serv_evt = threading.Event()
        self.client_evt = threading.Event()
        # Capture SMTPChannel debug output
        self.old_DEBUGSTREAM = smtpd.DEBUGSTREAM
        smtpd.DEBUGSTREAM = io.StringIO()
        # Pick a random unused port by passing 0 for the port number
        self.serv = smtpd.DebuggingServer((HOST, 0), ('nowhere', -1),
                                          decode_data=True)
        # Keep a note of what server host and port were assigned
        self.host, self.port = self.serv.socket.getsockname()[:2]
        serv_args = (self.serv, self.serv_evt, self.client_evt)
        self.thread = threading.Thread(target=debugging_server, args=serv_args)
        self.thread.start()

        # wait until server thread has assigned a port number
        self.serv_evt.wait()
        self.serv_evt.clear()

    def tearDown(self):
        socket.getfqdn = self.real_getfqdn
        # indicate that the client is finished
        self.client_evt.set()
        # wait for the server thread to terminate
        self.serv_evt.wait()
        threading_helper.join_thread(self.thread)
        # restore sys.stdout
        sys.stdout = self.old_stdout
        # restore DEBUGSTREAM
        smtpd.DEBUGSTREAM.close()
        smtpd.DEBUGSTREAM = self.old_DEBUGSTREAM
        del self.thread
        self.doCleanups()
        threading_helper.threading_cleanup(*self.thread_key)

    def get_output_without_xpeer(self):
        test_output = self.output.getvalue()
        return re.sub(r'(.*?)^X-Peer:\s*\S+\n(.*)', r'\1\2',
                      test_output, flags=re.MULTILINE|re.DOTALL)

    def testBasic(self):
        # connect
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        smtp.quit()

    def testSourceAddress(self):
        # connect
        src_port = socket_helper.find_unused_port()
        try:
            smtp = smtplib.SMTP(self.host, self.port, local_hostname='localhost',
                                timeout=support.LOOPBACK_TIMEOUT,
                                source_address=(self.host, src_port))
            self.addCleanup(smtp.close)
            self.assertEqual(smtp.source_address, (self.host, src_port))
            self.assertEqual(smtp.local_hostname, 'localhost')
            smtp.quit()
        except OSError as e:
            if e.errno == errno.EADDRINUSE:
                self.skipTest("couldn't bind to source port %d" % src_port)
            raise

    def testNOOP(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        expected = (250, b'OK')
        self.assertEqual(smtp.noop(), expected)
        smtp.quit()

    def testRSET(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        expected = (250, b'OK')
        self.assertEqual(smtp.rset(), expected)
        smtp.quit()

    def testELHO(self):
        # EHLO isn't implemented in DebuggingServer
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        expected = (250, b'\nSIZE 33554432\nHELP')
        self.assertEqual(smtp.ehlo(), expected)
        smtp.quit()

    def testEXPNNotImplemented(self):
        # EXPN isn't implemented in DebuggingServer
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        expected = (502, b'EXPN not implemented')
        smtp.putcmd('EXPN')
        self.assertEqual(smtp.getreply(), expected)
        smtp.quit()

    def test_issue43124_putcmd_escapes_newline(self):
        # see: https://bugs.python.org/issue43124
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        with self.assertRaises(ValueError) as exc:
            smtp.putcmd('helo\nX-INJECTED')
        self.assertIn("prohibited newline characters", str(exc.exception))
        smtp.quit()

    def testVRFY(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        expected = (252, b'Cannot VRFY user, but will accept message ' + \
                         b'and attempt delivery')
        self.assertEqual(smtp.vrfy('nobody@nowhere.com'), expected)
        self.assertEqual(smtp.verify('nobody@nowhere.com'), expected)
        smtp.quit()

    def testSecondHELO(self):
        # check that a second HELO returns a message that it's a duplicate
        # (this behavior is specific to smtpd.SMTPChannel)
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.helo()
        expected = (503, b'Duplicate HELO/EHLO')
        self.assertEqual(smtp.helo(), expected)
        smtp.quit()

    def testHELP(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        self.assertEqual(smtp.help(), b'Supported commands: EHLO HELO MAIL ' + \
                                      b'RCPT DATA RSET NOOP QUIT VRFY')
        smtp.quit()

    def testSend(self):
        # connect and send mail
        m = 'A test message'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.sendmail('John', 'Sally', m)
        # XXX(nnorwitz): this test is flaky and dies with a bad file descriptor
        # in asyncore.  This sleep might help, but should really be fixed
        # properly by using an Event variable.
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m, MSG_END)
        self.assertEqual(self.output.getvalue(), mexpect)

    def testSendBinary(self):
        m = b'A test message'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.sendmail('John', 'Sally', m)
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m.decode('ascii'), MSG_END)
        self.assertEqual(self.output.getvalue(), mexpect)

    def testSendNeedingDotQuote(self):
        # Issue 12283
        m = '.A test\n.mes.sage.'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.sendmail('John', 'Sally', m)
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m, MSG_END)
        self.assertEqual(self.output.getvalue(), mexpect)

    def test_issue43124_escape_localhostname(self):
        # see: https://bugs.python.org/issue43124
        # connect and send mail
        m = 'wazzuuup\nlinetwo'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='hi\nX-INJECTED',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        with self.assertRaises(ValueError) as exc:
            smtp.sendmail("hi@me.com", "you@me.com", m)
        self.assertIn(
            "prohibited newline characters: ehlo hi\\nX-INJECTED",
            str(exc.exception),
        )
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        debugout = smtpd.DEBUGSTREAM.getvalue()
        self.assertNotIn("X-INJECTED", debugout)

    def test_issue43124_escape_options(self):
        # see: https://bugs.python.org/issue43124
        # connect and send mail
        m = 'wazzuuup\nlinetwo'
        smtp = smtplib.SMTP(
            HOST, self.port, local_hostname='localhost',
            timeout=support.LOOPBACK_TIMEOUT)

        self.addCleanup(smtp.close)
        smtp.sendmail("hi@me.com", "you@me.com", m)
        with self.assertRaises(ValueError) as exc:
            smtp.mail("hi@me.com", ["X-OPTION\nX-INJECTED-1", "X-OPTION2\nX-INJECTED-2"])
        msg = str(exc.exception)
        self.assertIn("prohibited newline characters", msg)
        self.assertIn("X-OPTION\\nX-INJECTED-1 X-OPTION2\\nX-INJECTED-2", msg)
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        debugout = smtpd.DEBUGSTREAM.getvalue()
        self.assertNotIn("X-OPTION", debugout)
        self.assertNotIn("X-OPTION2", debugout)
        self.assertNotIn("X-INJECTED-1", debugout)
        self.assertNotIn("X-INJECTED-2", debugout)

    def testSendNullSender(self):
        m = 'A test message'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.sendmail('<>', 'Sally', m)
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m, MSG_END)
        self.assertEqual(self.output.getvalue(), mexpect)
        debugout = smtpd.DEBUGSTREAM.getvalue()
        sender = re.compile("^sender: <>$", re.MULTILINE)
        self.assertRegex(debugout, sender)

    def testSendMessage(self):
        m = email.mime.text.MIMEText('A test message')
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.send_message(m, from_addr='John', to_addrs='Sally')
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        # Remove the X-Peer header that DebuggingServer adds as figuring out
        # exactly what IP address format is put there is not easy (and
        # irrelevant to our test).  Typically 127.0.0.1 or ::1, but it is
        # not always the same as socket.gethostbyname(HOST). :(
        test_output = self.get_output_without_xpeer()
        del m['X-Peer']
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m.as_string(), MSG_END)
        self.assertEqual(test_output, mexpect)

    def testSendMessageWithAddresses(self):
        m = email.mime.text.MIMEText('A test message')
        m['From'] = 'foo@bar.com'
        m['To'] = 'John'
        m['CC'] = 'Sally, Fred'
        m['Bcc'] = 'John Root <root@localhost>, "Dinsdale" <warped@silly.walks.com>'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.send_message(m)
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()
        # make sure the Bcc header is still in the message.
        self.assertEqual(m['Bcc'], 'John Root <root@localhost>, "Dinsdale" '
                                    '<warped@silly.walks.com>')

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        # Remove the X-Peer header that DebuggingServer adds.
        test_output = self.get_output_without_xpeer()
        del m['X-Peer']
        # The Bcc header should not be transmitted.
        del m['Bcc']
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m.as_string(), MSG_END)
        self.assertEqual(test_output, mexpect)
        debugout = smtpd.DEBUGSTREAM.getvalue()
        sender = re.compile("^sender: foo@bar.com$", re.MULTILINE)
        self.assertRegex(debugout, sender)
        for addr in ('John', 'Sally', 'Fred', 'root@localhost',
                     'warped@silly.walks.com'):
            to_addr = re.compile(r"^recips: .*'{}'.*$".format(addr),
                                 re.MULTILINE)
            self.assertRegex(debugout, to_addr)

    def testSendMessageWithSomeAddresses(self):
        # Make sure nothing breaks if not all of the three 'to' headers exist
        m = email.mime.text.MIMEText('A test message')
        m['From'] = 'foo@bar.com'
        m['To'] = 'John, Dinsdale'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.send_message(m)
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        # Remove the X-Peer header that DebuggingServer adds.
        test_output = self.get_output_without_xpeer()
        del m['X-Peer']
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m.as_string(), MSG_END)
        self.assertEqual(test_output, mexpect)
        debugout = smtpd.DEBUGSTREAM.getvalue()
        sender = re.compile("^sender: foo@bar.com$", re.MULTILINE)
        self.assertRegex(debugout, sender)
        for addr in ('John', 'Dinsdale'):
            to_addr = re.compile(r"^recips: .*'{}'.*$".format(addr),
                                 re.MULTILINE)
            self.assertRegex(debugout, to_addr)

    def testSendMessageWithSpecifiedAddresses(self):
        # Make sure addresses specified in call override those in message.
        m = email.mime.text.MIMEText('A test message')
        m['From'] = 'foo@bar.com'
        m['To'] = 'John, Dinsdale'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.send_message(m, from_addr='joe@example.com', to_addrs='foo@example.net')
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        # Remove the X-Peer header that DebuggingServer adds.
        test_output = self.get_output_without_xpeer()
        del m['X-Peer']
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m.as_string(), MSG_END)
        self.assertEqual(test_output, mexpect)
        debugout = smtpd.DEBUGSTREAM.getvalue()
        sender = re.compile("^sender: joe@example.com$", re.MULTILINE)
        self.assertRegex(debugout, sender)
        for addr in ('John', 'Dinsdale'):
            to_addr = re.compile(r"^recips: .*'{}'.*$".format(addr),
                                 re.MULTILINE)
            self.assertNotRegex(debugout, to_addr)
        recip = re.compile(r"^recips: .*'foo@example.net'.*$", re.MULTILINE)
        self.assertRegex(debugout, recip)

    def testSendMessageWithMultipleFrom(self):
        # Sender overrides To
        m = email.mime.text.MIMEText('A test message')
        m['From'] = 'Bernard, Bianca'
        m['Sender'] = 'the_rescuers@Rescue-Aid-Society.com'
        m['To'] = 'John, Dinsdale'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.send_message(m)
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        # Remove the X-Peer header that DebuggingServer adds.
        test_output = self.get_output_without_xpeer()
        del m['X-Peer']
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m.as_string(), MSG_END)
        self.assertEqual(test_output, mexpect)
        debugout = smtpd.DEBUGSTREAM.getvalue()
        sender = re.compile("^sender: the_rescuers@Rescue-Aid-Society.com$", re.MULTILINE)
        self.assertRegex(debugout, sender)
        for addr in ('John', 'Dinsdale'):
            to_addr = re.compile(r"^recips: .*'{}'.*$".format(addr),
                                 re.MULTILINE)
            self.assertRegex(debugout, to_addr)

    def testSendMessageResent(self):
        m = email.mime.text.MIMEText('A test message')
        m['From'] = 'foo@bar.com'
        m['To'] = 'John'
        m['CC'] = 'Sally, Fred'
        m['Bcc'] = 'John Root <root@localhost>, "Dinsdale" <warped@silly.walks.com>'
        m['Resent-Date'] = 'Thu, 1 Jan 1970 17:42:00 +0000'
        m['Resent-From'] = 'holy@grail.net'
        m['Resent-To'] = 'Martha <my_mom@great.cooker.com>, Jeff'
        m['Resent-Bcc'] = 'doe@losthope.net'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.send_message(m)
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        # The Resent-Bcc headers are deleted before serialization.
        del m['Bcc']
        del m['Resent-Bcc']
        # Remove the X-Peer header that DebuggingServer adds.
        test_output = self.get_output_without_xpeer()
        del m['X-Peer']
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m.as_string(), MSG_END)
        self.assertEqual(test_output, mexpect)
        debugout = smtpd.DEBUGSTREAM.getvalue()
        sender = re.compile("^sender: holy@grail.net$", re.MULTILINE)
        self.assertRegex(debugout, sender)
        for addr in ('my_mom@great.cooker.com', 'Jeff', 'doe@losthope.net'):
            to_addr = re.compile(r"^recips: .*'{}'.*$".format(addr),
                                 re.MULTILINE)
            self.assertRegex(debugout, to_addr)

    def testSendMessageMultipleResentRaises(self):
        m = email.mime.text.MIMEText('A test message')
        m['From'] = 'foo@bar.com'
        m['To'] = 'John'
        m['CC'] = 'Sally, Fred'
        m['Bcc'] = 'John Root <root@localhost>, "Dinsdale" <warped@silly.walks.com>'
        m['Resent-Date'] = 'Thu, 1 Jan 1970 17:42:00 +0000'
        m['Resent-From'] = 'holy@grail.net'
        m['Resent-To'] = 'Martha <my_mom@great.cooker.com>, Jeff'
        m['Resent-Bcc'] = 'doe@losthope.net'
        m['Resent-Date'] = 'Thu, 2 Jan 1970 17:42:00 +0000'
        m['Resent-To'] = 'holy@grail.net'
        m['Resent-From'] = 'Martha <my_mom@great.cooker.com>, Jeff'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        with self.assertRaises(ValueError):
            smtp.send_message(m)
        smtp.close()

class NonConnectingTests(unittest.TestCase):

    def testNotConnected(self):
        # Test various operations on an unconnected SMTP object that
        # should raise exceptions (at present the attempt in SMTP.send
        # to reference the nonexistent 'sock' attribute of the SMTP object
        # causes an AttributeError)
        smtp = smtplib.SMTP()
        self.assertRaises(smtplib.SMTPServerDisconnected, smtp.ehlo)
        self.assertRaises(smtplib.SMTPServerDisconnected,
                          smtp.send, 'test msg')

    def testNonnumericPort(self):
        # check that non-numeric port raises OSError
        self.assertRaises(OSError, smtplib.SMTP,
                          "localhost", "bogus")
        self.assertRaises(OSError, smtplib.SMTP,
                          "localhost:bogus")

    def testSockAttributeExists(self):
        # check that sock attribute is present outside of a connect() call
        # (regression test, the previous behavior raised an
        #  AttributeError: 'SMTP' object has no attribute 'sock')
        with smtplib.SMTP() as smtp:
            self.assertIsNone(smtp.sock)


class DefaultArgumentsTests(unittest.TestCase):

    def setUp(self):
        self.msg = EmailMessage()
        self.msg['From'] = 'Páolo <főo@bar.com>'
        self.smtp = smtplib.SMTP()
        self.smtp.ehlo = Mock(return_value=(200, 'OK'))
        self.smtp.has_extn, self.smtp.sendmail = Mock(), Mock()

    def testSendMessage(self):
        expected_mail_options = ('SMTPUTF8', 'BODY=8BITMIME')
        self.smtp.send_message(self.msg)
        self.smtp.send_message(self.msg)
        self.assertEqual(self.smtp.sendmail.call_args_list[0][0][3],
                         expected_mail_options)
        self.assertEqual(self.smtp.sendmail.call_args_list[1][0][3],
                         expected_mail_options)

    def testSendMessageWithMailOptions(self):
        mail_options = ['STARTTLS']
        expected_mail_options = ('STARTTLS', 'SMTPUTF8', 'BODY=8BITMIME')
        self.smtp.send_message(self.msg, None, None, mail_options)
        self.assertEqual(mail_options, ['STARTTLS'])
        self.assertEqual(self.smtp.sendmail.call_args_list[0][0][3],
                         expected_mail_options)


# test response of client to a non-successful HELO message
class BadHELOServerTests(unittest.TestCase):

    def setUp(self):
        smtplib.socket = mock_socket
        mock_socket.reply_with(b"199 no hello for you!")
        self.old_stdout = sys.stdout
        self.output = io.StringIO()
        sys.stdout = self.output
        self.port = 25

    def tearDown(self):
        smtplib.socket = socket
        sys.stdout = self.old_stdout

    def testFailingHELO(self):
        self.assertRaises(smtplib.SMTPConnectError, smtplib.SMTP,
                            HOST, self.port, 'localhost', 3)


class TooLongLineTests(unittest.TestCase):
    respdata = b'250 OK' + (b'.' * smtplib._MAXLINE * 2) + b'\n'

    def setUp(self):
        self.thread_key = threading_helper.threading_setup()
        self.old_stdout = sys.stdout
        self.output = io.StringIO()
        sys.stdout = self.output

        self.evt = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(15)
        self.port = socket_helper.bind_port(self.sock)
        servargs = (self.evt, self.respdata, self.sock)
        self.thread = threading.Thread(target=server, args=servargs)
        self.thread.start()
        self.evt.wait()
        self.evt.clear()

    def tearDown(self):
        self.evt.wait()
        sys.stdout = self.old_stdout
        threading_helper.join_thread(self.thread)
        del self.thread
        self.doCleanups()
        threading_helper.threading_cleanup(*self.thread_key)

    def testLineTooLong(self):
        self.assertRaises(smtplib.SMTPResponseException, smtplib.SMTP,
                          HOST, self.port, 'localhost', 3)


sim_users = {'Mr.A@somewhere.com':'John A',
             'Ms.B@xn--fo-fka.com':'Sally B',
             'Mrs.C@somewhereesle.com':'Ruth C',
            }

sim_auth = ('Mr.A@somewhere.com', 'somepassword')
sim_cram_md5_challenge = ('PENCeUxFREJoU0NnbmhNWitOMjNGNn'
                          'dAZWx3b29kLmlubm9zb2Z0LmNvbT4=')
sim_lists = {'list-1':['Mr.A@somewhere.com','Mrs.C@somewhereesle.com'],
             'list-2':['Ms.B@xn--fo-fka.com',],
            }

# Simulated SMTP channel & server
class ResponseException(Exception): pass
class SimSMTPChannel(smtpd.SMTPChannel):

    quit_response = None
    mail_response = None
    rcpt_response = None
    data_response = None
    rcpt_count = 0
    rset_count = 0
    disconnect = 0
    AUTH = 99    # Add protocol state to enable auth testing.
    authenticated_user = None

    def __init__(self, extra_features, *args, **kw):
        self._extrafeatures = ''.join(
            [ "250-{0}\r\n".format(x) for x in extra_features ])
        super(SimSMTPChannel, self).__init__(*args, **kw)

    # AUTH related stuff.  It would be nice if support for this were in smtpd.
    def found_terminator(self):
        if self.smtp_state == self.AUTH:
            line = self._emptystring.join(self.received_lines)
            print('Data:', repr(line), file=smtpd.DEBUGSTREAM)
            self.received_lines = []
            try:
                self.auth_object(line)
            except ResponseException as e:
                self.smtp_state = self.COMMAND
                self.push('%s %s' % (e.smtp_code, e.smtp_error))
            return
        super().found_terminator()


    def smtp_AUTH(self, arg):
        if not self.seen_greeting:
            self.push('503 Error: send EHLO first')
            return
        if not self.extended_smtp or 'AUTH' not in self._extrafeatures:
            self.push('500 Error: command "AUTH" not recognized')
            return
        if self.authenticated_user is not None:
            self.push(
                '503 Bad sequence of commands: already authenticated')
            return
        args = arg.split()
        if len(args) not in [1, 2]:
            self.push('501 Syntax: AUTH <mechanism> [initial-response]')
            return
        auth_object_name = '_auth_%s' % args[0].lower().replace('-', '_')
        try:
            self.auth_object = getattr(self, auth_object_name)
        except AttributeError:
            self.push('504 Command parameter not implemented: unsupported '
                      ' authentication mechanism {!r}'.format(auth_object_name))
            return
        self.smtp_state = self.AUTH
        self.auth_object(args[1] if len(args) == 2 else None)

    def _authenticated(self, user, valid):
        if valid:
            self.authenticated_user = user
            self.push('235 Authentication Succeeded')
        else:
            self.push('535 Authentication credentials invalid')
        self.smtp_state = self.COMMAND

    def _decode_base64(self, string):
        return base64.decodebytes(string.encode('ascii')).decode('utf-8')

    def _auth_plain(self, arg=None):
        if arg is None:
            self.push('334 ')
        else:
            logpass = self._decode_base64(arg)
            try:
                *_, user, password = logpass.split('\0')
            except ValueError as e:
                self.push('535 Splitting response {!r} into user and password'
                          ' failed: {}'.format(logpass, e))
                return
            self._authenticated(user, password == sim_auth[1])

    def _auth_login(self, arg=None):
        if arg is None:
            # base64 encoded 'Username:'
            self.push('334 VXNlcm5hbWU6')
        elif not hasattr(self, '_auth_login_user'):
            self._auth_login_user = self._decode_base64(arg)
            # base64 encoded 'Password:'
            self.push('334 UGFzc3dvcmQ6')
        else:
            password = self._decode_base64(arg)
            self._authenticated(self._auth_login_user, password == sim_auth[1])
            del self._auth_login_user

    def _auth_buggy(self, arg=None):
        # This AUTH mechanism will 'trap' client in a neverending 334
        # base64 encoded 'BuGgYbUgGy'
        self.push('334 QnVHZ1liVWdHeQ==')

    def _auth_cram_md5(self, arg=None):
        if arg is None:
            self.push('334 {}'.format(sim_cram_md5_challenge))
        else:
            logpass = self._decode_base64(arg)
            try:
                user, hashed_pass = logpass.split()
            except ValueError as e:
                self.push('535 Splitting response {!r} into user and password '
                          'failed: {}'.format(logpass, e))
                return False
            valid_hashed_pass = hmac.HMAC(
                sim_auth[1].encode('ascii'),
                self._decode_base64(sim_cram_md5_challenge).encode('ascii'),
                'md5').hexdigest()
            self._authenticated(user, hashed_pass == valid_hashed_pass)
    # end AUTH related stuff.

    def smtp_EHLO(self, arg):
        resp = ('250-testhost\r\n'
                '250-EXPN\r\n'
                '250-SIZE 20000000\r\n'
                '250-STARTTLS\r\n'
                '250-DELIVERBY\r\n')
        resp = resp + self._extrafeatures + '250 HELP'
        self.push(resp)
        self.seen_greeting = arg
        self.extended_smtp = True

    def smtp_VRFY(self, arg):
        # For max compatibility smtplib should be sending the raw address.
        if arg in sim_users:
            self.push('250 %s %s' % (sim_users[arg], smtplib.quoteaddr(arg)))
        else:
            self.push('550 No such user: %s' % arg)

    def smtp_EXPN(self, arg):
        list_name = arg.lower()
        if list_name in sim_lists:
            user_list = sim_lists[list_name]
            for n, user_email in enumerate(user_list):
                quoted_addr = smtplib.quoteaddr(user_email)
                if n < len(user_list) - 1:
                    self.push('250-%s %s' % (sim_users[user_email], quoted_addr))
                else:
                    self.push('250 %s %s' % (sim_users[user_email], quoted_addr))
        else:
            self.push('550 No access for you!')

    def smtp_QUIT(self, arg):
        if self.quit_response is None:
            super(SimSMTPChannel, self).smtp_QUIT(arg)
        else:
            self.push(self.quit_response)
            self.close_when_done()

    def smtp_MAIL(self, arg):
        if self.mail_response is None:
            super().smtp_MAIL(arg)
        else:
            self.push(self.mail_response)
            if self.disconnect:
                self.close_when_done()

    def smtp_RCPT(self, arg):
        if self.rcpt_response is None:
            super().smtp_RCPT(arg)
            return
        self.rcpt_count += 1
        self.push(self.rcpt_response[self.rcpt_count-1])

    def smtp_RSET(self, arg):
        self.rset_count += 1
        super().smtp_RSET(arg)

    def smtp_DATA(self, arg):
        if self.data_response is None:
            super().smtp_DATA(arg)
        else:
            self.push(self.data_response)

    def handle_error(self):
        raise


class SimSMTPServer(smtpd.SMTPServer):

    channel_class = SimSMTPChannel

    def __init__(self, *args, **kw):
        self._extra_features = []
        self._addresses = {}
        smtpd.SMTPServer.__init__(self, *args, **kw)

    def handle_accepted(self, conn, addr):
        self._SMTPchannel = self.channel_class(
            self._extra_features, self, conn, addr,
            decode_data=self._decode_data)

    def process_message(self, peer, mailfrom, rcpttos, data):
        self._addresses['from'] = mailfrom
        self._addresses['tos'] = rcpttos

    def add_feature(self, feature):
        self._extra_features.append(feature)

    def handle_error(self):
        raise


# Test various SMTP & ESMTP commands/behaviors that require a simulated server
# (i.e., something with more features than DebuggingServer)
class SMTPSimTests(unittest.TestCase):

    def setUp(self):
        self.thread_key = threading_helper.threading_setup()
        self.real_getfqdn = socket.getfqdn
        socket.getfqdn = mock_socket.getfqdn
        self.serv_evt = threading.Event()
        self.client_evt = threading.Event()
        # Pick a random unused port by passing 0 for the port number
        self.serv = SimSMTPServer((HOST, 0), ('nowhere', -1), decode_data=True)
        # Keep a note of what port was assigned
        self.port = self.serv.socket.getsockname()[1]
        serv_args = (self.serv, self.serv_evt, self.client_evt)
        self.thread = threading.Thread(target=debugging_server, args=serv_args)
        self.thread.start()

        # wait until server thread has assigned a port number
        self.serv_evt.wait()
        self.serv_evt.clear()

    def tearDown(self):
        socket.getfqdn = self.real_getfqdn
        # indicate that the client is finished
        self.client_evt.set()
        # wait for the server thread to terminate
        self.serv_evt.wait()
        threading_helper.join_thread(self.thread)
        del self.thread
        self.doCleanups()
        threading_helper.threading_cleanup(*self.thread_key)

    def testBasic(self):
        # smoke test
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        smtp.quit()

    def testEHLO(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)

        # no features should be present before the EHLO
        self.assertEqual(smtp.esmtp_features, {})

        # features expected from the test server
        expected_features = {'expn':'',
                             'size': '20000000',
                             'starttls': '',
                             'deliverby': '',
                             'help': '',
                             }

        smtp.ehlo()
        self.assertEqual(smtp.esmtp_features, expected_features)
        for k in expected_features:
            self.assertTrue(smtp.has_extn(k))
        self.assertFalse(smtp.has_extn('unsupported-feature'))
        smtp.quit()

    def testVRFY(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)

        for addr_spec, name in sim_users.items():
            expected_known = (250, bytes('%s %s' %
                                         (name, smtplib.quoteaddr(addr_spec)),
                                         "ascii"))
            self.assertEqual(smtp.vrfy(addr_spec), expected_known)

        u = 'nobody@nowhere.com'
        expected_unknown = (550, ('No such user: %s' % u).encode('ascii'))
        self.assertEqual(smtp.vrfy(u), expected_unknown)
        smtp.quit()

    def testEXPN(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)

        for listname, members in sim_lists.items():
            users = []
            for m in members:
                users.append('%s %s' % (sim_users[m], smtplib.quoteaddr(m)))
            expected_known = (250, bytes('\n'.join(users), "ascii"))
            self.assertEqual(smtp.expn(listname), expected_known)

        u = 'PSU-Members-List'
        expected_unknown = (550, b'No access for you!')
        self.assertEqual(smtp.expn(u), expected_unknown)
        smtp.quit()

    def testAUTH_PLAIN(self):
        self.serv.add_feature("AUTH PLAIN")
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        resp = smtp.login(sim_auth[0], sim_auth[1])
        self.assertEqual(resp, (235, b'Authentication Succeeded'))
        smtp.close()

    def testAUTH_LOGIN(self):
        self.serv.add_feature("AUTH LOGIN")
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        resp = smtp.login(sim_auth[0], sim_auth[1])
        self.assertEqual(resp, (235, b'Authentication Succeeded'))
        smtp.close()

    def testAUTH_LOGIN_initial_response_ok(self):
        self.serv.add_feature("AUTH LOGIN")
        with smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                          timeout=support.LOOPBACK_TIMEOUT) as smtp:
            smtp.user, smtp.password = sim_auth
            smtp.ehlo("test_auth_login")
            resp = smtp.auth("LOGIN", smtp.auth_login, initial_response_ok=True)
            self.assertEqual(resp, (235, b'Authentication Succeeded'))

    def testAUTH_LOGIN_initial_response_notok(self):
        self.serv.add_feature("AUTH LOGIN")
        with smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                          timeout=support.LOOPBACK_TIMEOUT) as smtp:
            smtp.user, smtp.password = sim_auth
            smtp.ehlo("test_auth_login")
            resp = smtp.auth("LOGIN", smtp.auth_login, initial_response_ok=False)
            self.assertEqual(resp, (235, b'Authentication Succeeded'))

    def testAUTH_BUGGY(self):
        self.serv.add_feature("AUTH BUGGY")

        def auth_buggy(challenge=None):
            self.assertEqual(b"BuGgYbUgGy", challenge)
            return "\0"

        smtp = smtplib.SMTP(
            HOST, self.port, local_hostname='localhost',
            timeout=support.LOOPBACK_TIMEOUT
        )
        try:
            smtp.user, smtp.password = sim_auth
            smtp.ehlo("test_auth_buggy")
            expect = r"^Server AUTH mechanism infinite loop.*"
            with self.assertRaisesRegex(smtplib.SMTPException, expect) as cm:
                smtp.auth("BUGGY", auth_buggy, initial_response_ok=False)
        finally:
            smtp.close()

    @hashlib_helper.requires_hashdigest('md5')
    def testAUTH_CRAM_MD5(self):
        self.serv.add_feature("AUTH CRAM-MD5")
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        resp = smtp.login(sim_auth[0], sim_auth[1])
        self.assertEqual(resp, (235, b'Authentication Succeeded'))
        smtp.close()

    @hashlib_helper.requires_hashdigest('md5')
    def testAUTH_multiple(self):
        # Test that multiple authentication methods are tried.
        self.serv.add_feature("AUTH BOGUS PLAIN LOGIN CRAM-MD5")
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        resp = smtp.login(sim_auth[0], sim_auth[1])
        self.assertEqual(resp, (235, b'Authentication Succeeded'))
        smtp.close()

    def test_auth_function(self):
        supported = {'PLAIN', 'LOGIN'}
        try:
            hashlib.md5()
        except ValueError:
            pass
        else:
            supported.add('CRAM-MD5')
        for mechanism in supported:
            self.serv.add_feature("AUTH {}".format(mechanism))
        for mechanism in supported:
            with self.subTest(mechanism=mechanism):
                smtp = smtplib.SMTP(HOST, self.port,
                                    local_hostname='localhost',
                                    timeout=support.LOOPBACK_TIMEOUT)
                smtp.ehlo('foo')
                smtp.user, smtp.password = sim_auth[0], sim_auth[1]
                method = 'auth_' + mechanism.lower().replace('-', '_')
                resp = smtp.auth(mechanism, getattr(smtp, method))
                self.assertEqual(resp, (235, b'Authentication Succeeded'))
                smtp.close()

    def test_quit_resets_greeting(self):
        smtp = smtplib.SMTP(HOST, self.port,
                            local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        code, message = smtp.ehlo()
        self.assertEqual(code, 250)
        self.assertIn('size', smtp.esmtp_features)
        smtp.quit()
        self.assertNotIn('size', smtp.esmtp_features)
        smtp.connect(HOST, self.port)
        self.assertNotIn('size', smtp.esmtp_features)
        smtp.ehlo_or_helo_if_needed()
        self.assertIn('size', smtp.esmtp_features)
        smtp.quit()

    def test_with_statement(self):
        with smtplib.SMTP(HOST, self.port) as smtp:
            code, message = smtp.noop()
            self.assertEqual(code, 250)
        self.assertRaises(smtplib.SMTPServerDisconnected, smtp.send, b'foo')
        with smtplib.SMTP(HOST, self.port) as smtp:
            smtp.close()
        self.assertRaises(smtplib.SMTPServerDisconnected, smtp.send, b'foo')

    def test_with_statement_QUIT_failure(self):
        with self.assertRaises(smtplib.SMTPResponseException) as error:
            with smtplib.SMTP(HOST, self.port) as smtp:
                smtp.noop()
                self.serv._SMTPchannel.quit_response = '421 QUIT FAILED'
        self.assertEqual(error.exception.smtp_code, 421)
        self.assertEqual(error.exception.smtp_error, b'QUIT FAILED')

    #TODO: add tests for correct AUTH method fallback now that the
    #test infrastructure can support it.

    # Issue 17498: make sure _rset does not raise SMTPServerDisconnected exception
    def test__rest_from_mail_cmd(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        smtp.noop()
        self.serv._SMTPchannel.mail_response = '451 Requested action aborted'
        self.serv._SMTPchannel.disconnect = True
        with self.assertRaises(smtplib.SMTPSenderRefused):
            smtp.sendmail('John', 'Sally', 'test message')
        self.assertIsNone(smtp.sock)

    # Issue 5713: make sure close, not rset, is called if we get a 421 error
    def test_421_from_mail_cmd(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        smtp.noop()
        self.serv._SMTPchannel.mail_response = '421 closing connection'
        with self.assertRaises(smtplib.SMTPSenderRefused):
            smtp.sendmail('John', 'Sally', 'test message')
        self.assertIsNone(smtp.sock)
        self.assertEqual(self.serv._SMTPchannel.rset_count, 0)

    def test_421_from_rcpt_cmd(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        smtp.noop()
        self.serv._SMTPchannel.rcpt_response = ['250 accepted', '421 closing']
        with self.assertRaises(smtplib.SMTPRecipientsRefused) as r:
            smtp.sendmail('John', ['Sally', 'Frank', 'George'], 'test message')
        self.assertIsNone(smtp.sock)
        self.assertEqual(self.serv._SMTPchannel.rset_count, 0)
        self.assertDictEqual(r.exception.args[0], {'Frank': (421, b'closing')})

    def test_421_from_data_cmd(self):
        class MySimSMTPChannel(SimSMTPChannel):
            def found_terminator(self):
                if self.smtp_state == self.DATA:
                    self.push('421 closing')
                else:
                    super().found_terminator()
        self.serv.channel_class = MySimSMTPChannel
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        smtp.noop()
        with self.assertRaises(smtplib.SMTPDataError):
            smtp.sendmail('John@foo.org', ['Sally@foo.org'], 'test message')
        self.assertIsNone(smtp.sock)
        self.assertEqual(self.serv._SMTPchannel.rcpt_count, 0)

    def test_smtputf8_NotSupportedError_if_no_server_support(self):
        smtp = smtplib.SMTP(
            HOST, self.port, local_hostname='localhost',
            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.ehlo()
        self.assertTrue(smtp.does_esmtp)
        self.assertFalse(smtp.has_extn('smtputf8'))
        self.assertRaises(
            smtplib.SMTPNotSupportedError,
            smtp.sendmail,
            'John', 'Sally', '', mail_options=['BODY=8BITMIME', 'SMTPUTF8'])
        self.assertRaises(
            smtplib.SMTPNotSupportedError,
            smtp.mail, 'John', options=['BODY=8BITMIME', 'SMTPUTF8'])

    def test_send_unicode_without_SMTPUTF8(self):
        smtp = smtplib.SMTP(
            HOST, self.port, local_hostname='localhost',
            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        self.assertRaises(UnicodeEncodeError, smtp.sendmail, 'Alice', 'Böb', '')
        self.assertRaises(UnicodeEncodeError, smtp.mail, 'Älice')

    def test_send_message_error_on_non_ascii_addrs_if_no_smtputf8(self):
        # This test is located here and not in the SMTPUTF8SimTests
        # class because it needs a "regular" SMTP server to work
        msg = EmailMessage()
        msg['From'] = "Páolo <főo@bar.com>"
        msg['To'] = 'Dinsdale'
        msg['Subject'] = 'Nudge nudge, wink, wink \u1F609'
        smtp = smtplib.SMTP(
            HOST, self.port, local_hostname='localhost',
            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        with self.assertRaises(smtplib.SMTPNotSupportedError):
            smtp.send_message(msg)

    def test_name_field_not_included_in_envelop_addresses(self):
        smtp = smtplib.SMTP(
            HOST, self.port, local_hostname='localhost',
            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)

        message = EmailMessage()
        message['From'] = email.utils.formataddr(('Michaël', 'michael@example.com'))
        message['To'] = email.utils.formataddr(('René', 'rene@example.com'))

        self.assertDictEqual(smtp.send_message(message), {})

        self.assertEqual(self.serv._addresses['from'], 'michael@example.com')
        self.assertEqual(self.serv._addresses['tos'], ['rene@example.com'])


class SimSMTPUTF8Server(SimSMTPServer):

    def __init__(self, *args, **kw):
        # The base SMTP server turns these on automatically, but our test
        # server is set up to munge the EHLO response, so we need to provide
        # them as well.  And yes, the call is to SMTPServer not SimSMTPServer.
        self._extra_features = ['SMTPUTF8', '8BITMIME']
        smtpd.SMTPServer.__init__(self, *args, **kw)

    def handle_accepted(self, conn, addr):
        self._SMTPchannel = self.channel_class(
            self._extra_features, self, conn, addr,
            decode_data=self._decode_data,
            enable_SMTPUTF8=self.enable_SMTPUTF8,
        )

    def process_message(self, peer, mailfrom, rcpttos, data, mail_options=None,
                                                             rcpt_options=None):
        self.last_peer = peer
        self.last_mailfrom = mailfrom
        self.last_rcpttos = rcpttos
        self.last_message = data
        self.last_mail_options = mail_options
        self.last_rcpt_options = rcpt_options


class SMTPUTF8SimTests(unittest.TestCase):

    maxDiff = None

    def setUp(self):
        self.thread_key = threading_helper.threading_setup()
        self.real_getfqdn = socket.getfqdn
        socket.getfqdn = mock_socket.getfqdn
        self.serv_evt = threading.Event()
        self.client_evt = threading.Event()
        # Pick a random unused port by passing 0 for the port number
        self.serv = SimSMTPUTF8Server((HOST, 0), ('nowhere', -1),
                                      decode_data=False,
                                      enable_SMTPUTF8=True)
        # Keep a note of what port was assigned
        self.port = self.serv.socket.getsockname()[1]
        serv_args = (self.serv, self.serv_evt, self.client_evt)
        self.thread = threading.Thread(target=debugging_server, args=serv_args)
        self.thread.start()

        # wait until server thread has assigned a port number
        self.serv_evt.wait()
        self.serv_evt.clear()

    def tearDown(self):
        socket.getfqdn = self.real_getfqdn
        # indicate that the client is finished
        self.client_evt.set()
        # wait for the server thread to terminate
        self.serv_evt.wait()
        threading_helper.join_thread(self.thread)
        del self.thread
        self.doCleanups()
        threading_helper.threading_cleanup(*self.thread_key)

    def test_test_server_supports_extensions(self):
        smtp = smtplib.SMTP(
            HOST, self.port, local_hostname='localhost',
            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.ehlo()
        self.assertTrue(smtp.does_esmtp)
        self.assertTrue(smtp.has_extn('smtputf8'))

    def test_send_unicode_with_SMTPUTF8_via_sendmail(self):
        m = '¡a test message containing unicode!'.encode('utf-8')
        smtp = smtplib.SMTP(
            HOST, self.port, local_hostname='localhost',
            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.sendmail('Jőhn', 'Sálly', m,
                      mail_options=['BODY=8BITMIME', 'SMTPUTF8'])
        self.assertEqual(self.serv.last_mailfrom, 'Jőhn')
        self.assertEqual(self.serv.last_rcpttos, ['Sálly'])
        self.assertEqual(self.serv.last_message, m)
        self.assertIn('BODY=8BITMIME', self.serv.last_mail_options)
        self.assertIn('SMTPUTF8', self.serv.last_mail_options)
        self.assertEqual(self.serv.last_rcpt_options, [])

    def test_send_unicode_with_SMTPUTF8_via_low_level_API(self):
        m = '¡a test message containing unicode!'.encode('utf-8')
        smtp = smtplib.SMTP(
            HOST, self.port, local_hostname='localhost',
            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        smtp.ehlo()
        self.assertEqual(
            smtp.mail('Jő', options=['BODY=8BITMIME', 'SMTPUTF8']),
            (250, b'OK'))
        self.assertEqual(smtp.rcpt('János'), (250, b'OK'))
        self.assertEqual(smtp.data(m), (250, b'OK'))
        self.assertEqual(self.serv.last_mailfrom, 'Jő')
        self.assertEqual(self.serv.last_rcpttos, ['János'])
        self.assertEqual(self.serv.last_message, m)
        self.assertIn('BODY=8BITMIME', self.serv.last_mail_options)
        self.assertIn('SMTPUTF8', self.serv.last_mail_options)
        self.assertEqual(self.serv.last_rcpt_options, [])

    def test_send_message_uses_smtputf8_if_addrs_non_ascii(self):
        msg = EmailMessage()
        msg['From'] = "Páolo <főo@bar.com>"
        msg['To'] = 'Dinsdale'
        msg['Subject'] = 'Nudge nudge, wink, wink \u1F609'
        # XXX I don't know why I need two \n's here, but this is an existing
        # bug (if it is one) and not a problem with the new functionality.
        msg.set_content("oh là là, know what I mean, know what I mean?\n\n")
        # XXX smtpd converts received /r/n to /n, so we can't easily test that
        # we are successfully sending /r/n :(.
        expected = textwrap.dedent("""\
            From: Páolo <főo@bar.com>
            To: Dinsdale
            Subject: Nudge nudge, wink, wink \u1F609
            Content-Type: text/plain; charset="utf-8"
            Content-Transfer-Encoding: 8bit
            MIME-Version: 1.0

            oh là là, know what I mean, know what I mean?
            """)
        smtp = smtplib.SMTP(
            HOST, self.port, local_hostname='localhost',
            timeout=support.LOOPBACK_TIMEOUT)
        self.addCleanup(smtp.close)
        self.assertEqual(smtp.send_message(msg), {})
        self.assertEqual(self.serv.last_mailfrom, 'főo@bar.com')
        self.assertEqual(self.serv.last_rcpttos, ['Dinsdale'])
        self.assertEqual(self.serv.last_message.decode(), expected)
        self.assertIn('BODY=8BITMIME', self.serv.last_mail_options)
        self.assertIn('SMTPUTF8', self.serv.last_mail_options)
        self.assertEqual(self.serv.last_rcpt_options, [])


EXPECTED_RESPONSE = encode_base64(b'\0psu\0doesnotexist', eol='')

class SimSMTPAUTHInitialResponseChannel(SimSMTPChannel):
    def smtp_AUTH(self, arg):
        # RFC 4954's AUTH command allows for an optional initial-response.
        # Not all AUTH methods support this; some require a challenge.  AUTH
        # PLAIN does those, so test that here.  See issue #15014.
        args = arg.split()
        if args[0].lower() == 'plain':
            if len(args) == 2:
                # AUTH PLAIN <initial-response> with the response base 64
                # encoded.  Hard code the expected response for the test.
                if args[1] == EXPECTED_RESPONSE:
                    self.push('235 Ok')
                    return
        self.push('571 Bad authentication')

class SimSMTPAUTHInitialResponseServer(SimSMTPServer):
    channel_class = SimSMTPAUTHInitialResponseChannel


class SMTPAUTHInitialResponseSimTests(unittest.TestCase):
    def setUp(self):
        self.thread_key = threading_helper.threading_setup()
        self.real_getfqdn = socket.getfqdn
        socket.getfqdn = mock_socket.getfqdn
        self.serv_evt = threading.Event()
        self.client_evt = threading.Event()
        # Pick a random unused port by passing 0 for the port number
        self.serv = SimSMTPAUTHInitialResponseServer(
            (HOST, 0), ('nowhere', -1), decode_data=True)
        # Keep a note of what port was assigned
        self.port = self.serv.socket.getsockname()[1]
        serv_args = (self.serv, self.serv_evt, self.client_evt)
        self.thread = threading.Thread(target=debugging_server, args=serv_args)
        self.thread.start()

        # wait until server thread has assigned a port number
        self.serv_evt.wait()
        self.serv_evt.clear()

    def tearDown(self):
        socket.getfqdn = self.real_getfqdn
        # indicate that the client is finished
        self.client_evt.set()
        # wait for the server thread to terminate
        self.serv_evt.wait()
        threading_helper.join_thread(self.thread)
        del self.thread
        self.doCleanups()
        threading_helper.threading_cleanup(*self.thread_key)

    def testAUTH_PLAIN_initial_response_login(self):
        self.serv.add_feature('AUTH PLAIN')
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        smtp.login('psu', 'doesnotexist')
        smtp.close()

    def testAUTH_PLAIN_initial_response_auth(self):
        self.serv.add_feature('AUTH PLAIN')
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                            timeout=support.LOOPBACK_TIMEOUT)
        smtp.user = 'psu'
        smtp.password = 'doesnotexist'
        code, response = smtp.auth('plain', smtp.auth_plain)
        smtp.close()
        self.assertEqual(code, 235)


if __name__ == '__main__':
    unittest.main()
