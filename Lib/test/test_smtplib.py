import asyncore
import email.mime.text
import email.utils
import socket
import smtpd
import smtplib
import io
import re
import sys
import time
import select
import errno

import unittest
from test import support, mock_socket

try:
    import threading
except ImportError:
    threading = None

HOST = support.HOST

if sys.platform == 'darwin':
    # select.poll returns a select.POLLHUP at the end of the tests
    # on darwin, so just ignore it
    def handle_expt(self):
        pass
    smtpd.SMTPChannel.handle_expt = handle_expt


def server(evt, buf, serv):
    serv.listen(5)
    evt.set()
    try:
        conn, addr = serv.accept()
    except socket.timeout:
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

class GeneralTests(unittest.TestCase):

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
        smtp = smtplib.SMTP(HOST, self.port)
        smtp.close()

    def testSourceAddress(self):
        mock_socket.reply_with(b"220 Hola mundo")
        # connects
        smtp = smtplib.SMTP(HOST, self.port,
                source_address=('127.0.0.1',19876))
        self.assertEqual(smtp.source_address, ('127.0.0.1', 19876))
        smtp.close()

    def testBasic2(self):
        mock_socket.reply_with(b"220 Hola mundo")
        # connects, include port in host name
        smtp = smtplib.SMTP("%s:%s" % (HOST, self.port))
        smtp.close()

    def testLocalHostName(self):
        mock_socket.reply_with(b"220 Hola mundo")
        # check that supplied local_hostname is used
        smtp = smtplib.SMTP(HOST, self.port, local_hostname="testhost")
        self.assertEqual(smtp.local_hostname, "testhost")
        smtp.close()

    def testTimeoutDefault(self):
        mock_socket.reply_with(b"220 Hola mundo")
        self.assertIsNone(mock_socket.getdefaulttimeout())
        mock_socket.setdefaulttimeout(30)
        self.assertEqual(mock_socket.getdefaulttimeout(), 30)
        try:
            smtp = smtplib.SMTP(HOST, self.port)
        finally:
            mock_socket.setdefaulttimeout(None)
        self.assertEqual(smtp.sock.gettimeout(), 30)
        smtp.close()

    def testTimeoutNone(self):
        mock_socket.reply_with(b"220 Hola mundo")
        self.assertIsNone(socket.getdefaulttimeout())
        socket.setdefaulttimeout(30)
        try:
            smtp = smtplib.SMTP(HOST, self.port, timeout=None)
        finally:
            socket.setdefaulttimeout(None)
        self.assertIsNone(smtp.sock.gettimeout())
        smtp.close()

    def testTimeoutValue(self):
        mock_socket.reply_with(b"220 Hola mundo")
        smtp = smtplib.SMTP(HOST, self.port, timeout=30)
        self.assertEqual(smtp.sock.gettimeout(), 30)
        smtp.close()


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

    except socket.timeout:
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
@unittest.skipUnless(threading, 'Threading required for this test.')
class DebuggingServerTests(unittest.TestCase):

    maxDiff = None

    def setUp(self):
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
        self.serv = smtpd.DebuggingServer((HOST, 0), ('nowhere', -1))
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
        self.thread.join()
        # restore sys.stdout
        sys.stdout = self.old_stdout
        # restore DEBUGSTREAM
        smtpd.DEBUGSTREAM.close()
        smtpd.DEBUGSTREAM = self.old_DEBUGSTREAM

    def testBasic(self):
        # connect
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
        smtp.quit()

    def testSourceAddress(self):
        # connect
        port = support.find_unused_port()
        try:
            smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost',
                    timeout=3, source_address=('127.0.0.1', port))
            self.assertEqual(smtp.source_address, ('127.0.0.1', port))
            self.assertEqual(smtp.local_hostname, 'localhost')
            smtp.quit()
        except OSError as e:
            if e.errno == errno.EADDRINUSE:
                self.skipTest("couldn't bind to port %d" % port)
            raise

    def testNOOP(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
        expected = (250, b'OK')
        self.assertEqual(smtp.noop(), expected)
        smtp.quit()

    def testRSET(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
        expected = (250, b'OK')
        self.assertEqual(smtp.rset(), expected)
        smtp.quit()

    def testELHO(self):
        # EHLO isn't implemented in DebuggingServer
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
        expected = (250, b'\nSIZE 33554432\nHELP')
        self.assertEqual(smtp.ehlo(), expected)
        smtp.quit()

    def testEXPNNotImplemented(self):
        # EXPN isn't implemented in DebuggingServer
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
        expected = (502, b'EXPN not implemented')
        smtp.putcmd('EXPN')
        self.assertEqual(smtp.getreply(), expected)
        smtp.quit()

    def testVRFY(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
        expected = (252, b'Cannot VRFY user, but will accept message ' + \
                         b'and attempt delivery')
        self.assertEqual(smtp.vrfy('nobody@nowhere.com'), expected)
        self.assertEqual(smtp.verify('nobody@nowhere.com'), expected)
        smtp.quit()

    def testSecondHELO(self):
        # check that a second HELO returns a message that it's a duplicate
        # (this behavior is specific to smtpd.SMTPChannel)
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
        smtp.helo()
        expected = (503, b'Duplicate HELO/EHLO')
        self.assertEqual(smtp.helo(), expected)
        smtp.quit()

    def testHELP(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
        self.assertEqual(smtp.help(), b'Supported commands: EHLO HELO MAIL ' + \
                                      b'RCPT DATA RSET NOOP QUIT VRFY')
        smtp.quit()

    def testSend(self):
        # connect and send mail
        m = 'A test message'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
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
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
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
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
        smtp.sendmail('John', 'Sally', m)
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m, MSG_END)
        self.assertEqual(self.output.getvalue(), mexpect)

    def testSendNullSender(self):
        m = 'A test message'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
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
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
        smtp.send_message(m, from_addr='John', to_addrs='Sally')
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        # Add the X-Peer header that DebuggingServer adds
        m['X-Peer'] = socket.gethostbyname('localhost')
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m.as_string(), MSG_END)
        self.assertEqual(self.output.getvalue(), mexpect)

    def testSendMessageWithAddresses(self):
        m = email.mime.text.MIMEText('A test message')
        m['From'] = 'foo@bar.com'
        m['To'] = 'John'
        m['CC'] = 'Sally, Fred'
        m['Bcc'] = 'John Root <root@localhost>, "Dinsdale" <warped@silly.walks.com>'
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
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
        # Add the X-Peer header that DebuggingServer adds
        m['X-Peer'] = socket.gethostbyname('localhost')
        # The Bcc header should not be transmitted.
        del m['Bcc']
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m.as_string(), MSG_END)
        self.assertEqual(self.output.getvalue(), mexpect)
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
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
        smtp.send_message(m)
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        # Add the X-Peer header that DebuggingServer adds
        m['X-Peer'] = socket.gethostbyname('localhost')
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m.as_string(), MSG_END)
        self.assertEqual(self.output.getvalue(), mexpect)
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
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
        smtp.send_message(m, from_addr='joe@example.com', to_addrs='foo@example.net')
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        # Add the X-Peer header that DebuggingServer adds
        m['X-Peer'] = socket.gethostbyname('localhost')
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m.as_string(), MSG_END)
        self.assertEqual(self.output.getvalue(), mexpect)
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
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
        smtp.send_message(m)
        # XXX (see comment in testSend)
        time.sleep(0.01)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        # Add the X-Peer header that DebuggingServer adds
        m['X-Peer'] = socket.gethostbyname('localhost')
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m.as_string(), MSG_END)
        self.assertEqual(self.output.getvalue(), mexpect)
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
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
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
        # Add the X-Peer header that DebuggingServer adds
        m['X-Peer'] = socket.gethostbyname('localhost')
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m.as_string(), MSG_END)
        self.assertEqual(self.output.getvalue(), mexpect)
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
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=3)
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


# test response of client to a non-successful HELO message
@unittest.skipUnless(threading, 'Threading required for this test.')
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


@unittest.skipUnless(threading, 'Threading required for this test.')
class TooLongLineTests(unittest.TestCase):
    respdata = b'250 OK' + (b'.' * smtplib._MAXLINE * 2) + b'\n'

    def setUp(self):
        self.old_stdout = sys.stdout
        self.output = io.StringIO()
        sys.stdout = self.output

        self.evt = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(15)
        self.port = support.bind_port(self.sock)
        servargs = (self.evt, self.respdata, self.sock)
        threading.Thread(target=server, args=servargs).start()
        self.evt.wait()
        self.evt.clear()

    def tearDown(self):
        self.evt.wait()
        sys.stdout = self.old_stdout

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
sim_auth_credentials = {
    'login': 'TXIuQUBzb21ld2hlcmUuY29t',
    'plain': 'AE1yLkFAc29tZXdoZXJlLmNvbQBzb21lcGFzc3dvcmQ=',
    'cram-md5': ('TXIUQUBZB21LD2HLCMUUY29TIDG4OWQ0MJ'
                 'KWZGQ4ODNMNDA4NTGXMDRLZWMYZJDMODG1'),
    }
sim_auth_login_password = 'C29TZXBHC3N3B3JK'

sim_lists = {'list-1':['Mr.A@somewhere.com','Mrs.C@somewhereesle.com'],
             'list-2':['Ms.B@xn--fo-fka.com',],
            }

# Simulated SMTP channel & server
class SimSMTPChannel(smtpd.SMTPChannel):

    quit_response = None
    mail_response = None
    rcpt_response = None
    data_response = None
    rcpt_count = 0
    rset_count = 0
    disconnect = 0

    def __init__(self, extra_features, *args, **kw):
        self._extrafeatures = ''.join(
            [ "250-{0}\r\n".format(x) for x in extra_features ])
        super(SimSMTPChannel, self).__init__(*args, **kw)

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

    def smtp_AUTH(self, arg):
        if arg.strip().lower()=='cram-md5':
            self.push('334 {}'.format(sim_cram_md5_challenge))
            return
        mech, auth = arg.split()
        mech = mech.lower()
        if mech not in sim_auth_credentials:
            self.push('504 auth type unimplemented')
            return
        if mech == 'plain' and auth==sim_auth_credentials['plain']:
            self.push('235 plain auth ok')
        elif mech=='login' and auth==sim_auth_credentials['login']:
            self.push('334 Password:')
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
        smtpd.SMTPServer.__init__(self, *args, **kw)

    def handle_accepted(self, conn, addr):
        self._SMTPchannel = self.channel_class(
            self._extra_features, self, conn, addr)

    def process_message(self, peer, mailfrom, rcpttos, data):
        pass

    def add_feature(self, feature):
        self._extra_features.append(feature)

    def handle_error(self):
        raise


# Test various SMTP & ESMTP commands/behaviors that require a simulated server
# (i.e., something with more features than DebuggingServer)
@unittest.skipUnless(threading, 'Threading required for this test.')
class SMTPSimTests(unittest.TestCase):

    def setUp(self):
        self.real_getfqdn = socket.getfqdn
        socket.getfqdn = mock_socket.getfqdn
        self.serv_evt = threading.Event()
        self.client_evt = threading.Event()
        # Pick a random unused port by passing 0 for the port number
        self.serv = SimSMTPServer((HOST, 0), ('nowhere', -1))
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
        self.thread.join()

    def testBasic(self):
        # smoke test
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=15)
        smtp.quit()

    def testEHLO(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=15)

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
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=15)

        for email, name in sim_users.items():
            expected_known = (250, bytes('%s %s' %
                                         (name, smtplib.quoteaddr(email)),
                                         "ascii"))
            self.assertEqual(smtp.vrfy(email), expected_known)

        u = 'nobody@nowhere.com'
        expected_unknown = (550, ('No such user: %s' % u).encode('ascii'))
        self.assertEqual(smtp.vrfy(u), expected_unknown)
        smtp.quit()

    def testEXPN(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=15)

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
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=15)

        expected_auth_ok = (235, b'plain auth ok')
        self.assertEqual(smtp.login(sim_auth[0], sim_auth[1]), expected_auth_ok)
        smtp.close()

    # SimSMTPChannel doesn't fully support LOGIN or CRAM-MD5 auth because they
    # require a synchronous read to obtain the credentials...so instead smtpd
    # sees the credential sent by smtplib's login method as an unknown command,
    # which results in smtplib raising an auth error.  Fortunately the error
    # message contains the encoded credential, so we can partially check that it
    # was generated correctly (partially, because the 'word' is uppercased in
    # the error message).

    def testAUTH_LOGIN(self):
        self.serv.add_feature("AUTH LOGIN")
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=15)
        try: smtp.login(sim_auth[0], sim_auth[1])
        except smtplib.SMTPAuthenticationError as err:
            self.assertIn(sim_auth_login_password, str(err))
        smtp.close()

    def testAUTH_CRAM_MD5(self):
        self.serv.add_feature("AUTH CRAM-MD5")
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=15)

        try: smtp.login(sim_auth[0], sim_auth[1])
        except smtplib.SMTPAuthenticationError as err:
            self.assertIn(sim_auth_credentials['cram-md5'], str(err))
        smtp.close()

    def testAUTH_multiple(self):
        # Test that multiple authentication methods are tried.
        self.serv.add_feature("AUTH BOGUS PLAIN LOGIN CRAM-MD5")
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=15)
        try: smtp.login(sim_auth[0], sim_auth[1])
        except smtplib.SMTPAuthenticationError as err:
            self.assertIn(sim_auth_login_password, str(err))
        smtp.close()

    def test_quit_resets_greeting(self):
        smtp = smtplib.SMTP(HOST, self.port,
                            local_hostname='localhost',
                            timeout=15)
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
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=15)
        smtp.noop()
        self.serv._SMTPchannel.mail_response = '451 Requested action aborted'
        self.serv._SMTPchannel.disconnect = True
        with self.assertRaises(smtplib.SMTPSenderRefused):
            smtp.sendmail('John', 'Sally', 'test message')
        self.assertIsNone(smtp.sock)

    # Issue 5713: make sure close, not rset, is called if we get a 421 error
    def test_421_from_mail_cmd(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=15)
        smtp.noop()
        self.serv._SMTPchannel.mail_response = '421 closing connection'
        with self.assertRaises(smtplib.SMTPSenderRefused):
            smtp.sendmail('John', 'Sally', 'test message')
        self.assertIsNone(smtp.sock)
        self.assertEqual(self.serv._SMTPchannel.rset_count, 0)

    def test_421_from_rcpt_cmd(self):
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=15)
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
        smtp = smtplib.SMTP(HOST, self.port, local_hostname='localhost', timeout=15)
        smtp.noop()
        with self.assertRaises(smtplib.SMTPDataError):
            smtp.sendmail('John@foo.org', ['Sally@foo.org'], 'test message')
        self.assertIsNone(smtp.sock)
        self.assertEqual(self.serv._SMTPchannel.rcpt_count, 0)


@support.reap_threads
def test_main(verbose=None):
    support.run_unittest(GeneralTests, DebuggingServerTests,
                              NonConnectingTests,
                              BadHELOServerTests, SMTPSimTests,
                              TooLongLineTests)

if __name__ == '__main__':
    test_main()
