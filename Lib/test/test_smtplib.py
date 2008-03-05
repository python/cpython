import asyncore
import email.utils
import socket
import threading
import smtpd
import smtplib
import StringIO
import sys
import time
import select

from unittest import TestCase
from test import test_support

# PORT is used to communicate the port number assigned to the server
# to the test client
HOST = "localhost"
PORT = None

def server(evt, buf):
    serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv.settimeout(15)
    serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    serv.bind(("", 0))
    global PORT
    PORT = serv.getsockname()[1]
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
        PORT = None
        evt.set()

class GeneralTests(TestCase):

    def setUp(self):
        self.evt = threading.Event()
        servargs = (self.evt, "220 Hola mundo\n")
        threading.Thread(target=server, args=servargs).start()
        self.evt.wait()
        self.evt.clear()

    def tearDown(self):
        self.evt.wait()

    def testBasic1(self):
        # connects
        smtp = smtplib.SMTP(HOST, PORT)
        smtp.sock.close()

    def testBasic2(self):
        # connects, include port in host name
        smtp = smtplib.SMTP("%s:%s" % (HOST, PORT))
        smtp.sock.close()

    def testLocalHostName(self):
        # check that supplied local_hostname is used
        smtp = smtplib.SMTP(HOST, PORT, local_hostname="testhost")
        self.assertEqual(smtp.local_hostname, "testhost")
        smtp.sock.close()

    def testTimeoutDefault(self):
        # default
        smtp = smtplib.SMTP(HOST, PORT)
        self.assertTrue(smtp.sock.gettimeout() is None)
        smtp.sock.close()

    def testTimeoutValue(self):
        # a value
        smtp = smtplib.SMTP(HOST, PORT, timeout=30)
        self.assertEqual(smtp.sock.gettimeout(), 30)
        smtp.sock.close()

    def testTimeoutNone(self):
        # None, having other default
        previous = socket.getdefaulttimeout()
        socket.setdefaulttimeout(30)
        try:
            smtp = smtplib.SMTP(HOST, PORT, timeout=None)
        finally:
            socket.setdefaulttimeout(previous)
        self.assertEqual(smtp.sock.gettimeout(), 30)
        smtp.sock.close()


# Test server thread using the specified SMTP server class
def debugging_server(server_class, serv_evt, client_evt):
    serv = server_class(("", 0), ('nowhere', -1))
    global PORT
    PORT = serv.getsockname()[1]
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
            if client_evt.isSet():
                serv.close()
                break

            n -= 1

    except socket.timeout:
        pass
    finally:
        if not client_evt.isSet():
            # allow some time for the client to read the result
            time.sleep(0.5)
            serv.close()
        asyncore.close_all()
        PORT = None
        serv_evt.set()

MSG_BEGIN = '---------- MESSAGE FOLLOWS ----------\n'
MSG_END = '------------ END MESSAGE ------------\n'

# NOTE: Some SMTP objects in the tests below are created with a non-default
# local_hostname argument to the constructor, since (on some systems) the FQDN
# lookup caused by the default local_hostname sometimes takes so long that the
# test server times out, causing the test to fail.

# Test behavior of smtpd.DebuggingServer
class DebuggingServerTests(TestCase):

    def setUp(self):
        # temporarily replace sys.stdout to capture DebuggingServer output
        self.old_stdout = sys.stdout
        self.output = StringIO.StringIO()
        sys.stdout = self.output

        self.serv_evt = threading.Event()
        self.client_evt = threading.Event()
        serv_args = (smtpd.DebuggingServer, self.serv_evt, self.client_evt)
        threading.Thread(target=debugging_server, args=serv_args).start()

        # wait until server thread has assigned a port number
        self.serv_evt.wait()
        self.serv_evt.clear()

    def tearDown(self):
        # indicate that the client is finished
        self.client_evt.set()
        # wait for the server thread to terminate
        self.serv_evt.wait()
        # restore sys.stdout
        sys.stdout = self.old_stdout

    def testBasic(self):
        # connect
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)
        smtp.quit()

    def testNOOP(self):
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)
        expected = (250, 'Ok')
        self.assertEqual(smtp.noop(), expected)
        smtp.quit()

    def testRSET(self):
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)
        expected = (250, 'Ok')
        self.assertEqual(smtp.rset(), expected)
        smtp.quit()

    def testNotImplemented(self):
        # EHLO isn't implemented in DebuggingServer
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)
        expected = (502, 'Error: command "EHLO" not implemented')
        self.assertEqual(smtp.ehlo(), expected)
        smtp.quit()

    def testVRFY(self):
        # VRFY isn't implemented in DebuggingServer
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)
        expected = (502, 'Error: command "VRFY" not implemented')
        self.assertEqual(smtp.vrfy('nobody@nowhere.com'), expected)
        self.assertEqual(smtp.verify('nobody@nowhere.com'), expected)
        smtp.quit()

    def testSecondHELO(self):
        # check that a second HELO returns a message that it's a duplicate
        # (this behavior is specific to smtpd.SMTPChannel)
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)
        smtp.helo()
        expected = (503, 'Duplicate HELO/EHLO')
        self.assertEqual(smtp.helo(), expected)
        smtp.quit()

    def testHELP(self):
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)
        self.assertEqual(smtp.help(), 'Error: command "HELP" not implemented')
        smtp.quit()

    def testSend(self):
        # connect and send mail
        m = 'A test message'
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)
        smtp.sendmail('John', 'Sally', m)
        smtp.quit()

        self.client_evt.set()
        self.serv_evt.wait()
        self.output.flush()
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m, MSG_END)
        self.assertEqual(self.output.getvalue(), mexpect)


class NonConnectingTests(TestCase):

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
        # check that non-numeric port raises socket.error
        self.assertRaises(socket.error, smtplib.SMTP,
                          "localhost", "bogus")
        self.assertRaises(socket.error, smtplib.SMTP,
                          "localhost:bogus")


# test response of client to a non-successful HELO message
class BadHELOServerTests(TestCase):

    def setUp(self):
        self.old_stdout = sys.stdout
        self.output = StringIO.StringIO()
        sys.stdout = self.output

        self.evt = threading.Event()
        servargs = (self.evt, "199 no hello for you!\n")
        threading.Thread(target=server, args=servargs).start()
        self.evt.wait()
        self.evt.clear()

    def tearDown(self):
        self.evt.wait()
        sys.stdout = self.old_stdout

    def testFailingHELO(self):
        self.assertRaises(smtplib.SMTPConnectError, smtplib.SMTP,
                            HOST, PORT, 'localhost', 3)


sim_users = {'Mr.A@somewhere.com':'John A',
             'Ms.B@somewhere.com':'Sally B',
             'Mrs.C@somewhereesle.com':'Ruth C',
            }

sim_lists = {'list-1':['Mr.A@somewhere.com','Mrs.C@somewhereesle.com'],
             'list-2':['Ms.B@somewhere.com',],
            }

# Simulated SMTP channel & server
class SimSMTPChannel(smtpd.SMTPChannel):
    def smtp_EHLO(self, arg):
        resp = '250-testhost\r\n' \
               '250-EXPN\r\n' \
               '250-SIZE 20000000\r\n' \
               '250-STARTTLS\r\n' \
               '250-DELIVERBY\r\n' \
               '250 HELP'
        self.push(resp)

    def smtp_VRFY(self, arg):
#        print '\nsmtp_VRFY(%r)\n' % arg

        raw_addr = email.utils.parseaddr(arg)[1]
        quoted_addr = smtplib.quoteaddr(arg)
        if raw_addr in sim_users:
            self.push('250 %s %s' % (sim_users[raw_addr], quoted_addr))
        else:
            self.push('550 No such user: %s' % arg)

    def smtp_EXPN(self, arg):
#        print '\nsmtp_EXPN(%r)\n' % arg

        list_name = email.utils.parseaddr(arg)[1].lower()
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


class SimSMTPServer(smtpd.SMTPServer):
    def handle_accept(self):
        conn, addr = self.accept()
        channel = SimSMTPChannel(self, conn, addr)

    def process_message(self, peer, mailfrom, rcpttos, data):
        pass


# Test various SMTP & ESMTP commands/behaviors that require a simulated server
# (i.e., something with more features than DebuggingServer)
class SMTPSimTests(TestCase):

    def setUp(self):
        self.serv_evt = threading.Event()
        self.client_evt = threading.Event()
        serv_args = (SimSMTPServer, self.serv_evt, self.client_evt)
        threading.Thread(target=debugging_server, args=serv_args).start()

        # wait until server thread has assigned a port number
        self.serv_evt.wait()
        self.serv_evt.clear()

    def tearDown(self):
        # indicate that the client is finished
        self.client_evt.set()
        # wait for the server thread to terminate
        self.serv_evt.wait()

    def testBasic(self):
        # smoke test
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)
        smtp.quit()

    def testEHLO(self):
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)

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
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)

        for email, name in sim_users.items():
            expected_known = (250, '%s %s' % (name, smtplib.quoteaddr(email)))
            self.assertEqual(smtp.vrfy(email), expected_known)

        u = 'nobody@nowhere.com'
        expected_unknown = (550, 'No such user: %s' % smtplib.quoteaddr(u))
        self.assertEqual(smtp.vrfy(u), expected_unknown)
        smtp.quit()

    def testEXPN(self):
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)

        for listname, members in sim_lists.items():
            users = []
            for m in members:
                users.append('%s %s' % (sim_users[m], smtplib.quoteaddr(m)))
            expected_known = (250, '\n'.join(users))
            self.assertEqual(smtp.expn(listname), expected_known)

        u = 'PSU-Members-List'
        expected_unknown = (550, 'No access for you!')
        self.assertEqual(smtp.expn(u), expected_unknown)
        smtp.quit()



def test_main(verbose=None):
    test_support.run_unittest(GeneralTests, DebuggingServerTests,
                              NonConnectingTests,
                              BadHELOServerTests, SMTPSimTests)

if __name__ == '__main__':
    test_main()
