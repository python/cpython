import asyncore
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

HOST = "localhost"
PORT = 54328

def server(evt, buf):
    serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serv.settimeout(3)
    serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    serv.bind(("", PORT))
    serv.listen(5)
    try:
        conn, addr = serv.accept()
    except socket.timeout:
        pass
    else:
        n = 200
        while buf and n > 0:
            r, w, e = select.select([], [conn], [])
            if w:
                sent = conn.send(buf)
                buf = buf[sent:]

            n -= 1
            time.sleep(0.01)

        conn.close()
    finally:
        serv.close()
        evt.set()

class GeneralTests(TestCase):

    def setUp(self):
        self.evt = threading.Event()
        servargs = (self.evt, "220 Hola mundo\n")
        threading.Thread(target=server, args=servargs).start()
        time.sleep(.1)

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

    def testNonnumericPort(self):
        # check that non-numeric port raises ValueError
        self.assertRaises(socket.error, smtplib.SMTP, "localhost", "bogus")

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


# Test server using smtpd.DebuggingServer
def debugging_server(evt):
    serv = smtpd.DebuggingServer(("", PORT), ('nowhere', -1))

    try:
        asyncore.loop(timeout=.01, count=300)
    except socket.timeout:
        pass
    finally:
        # allow some time for the client to read the result
        time.sleep(0.5)
        asyncore.close_all()
        evt.set()

MSG_BEGIN = '---------- MESSAGE FOLLOWS ----------\n'
MSG_END = '------------ END MESSAGE ------------\n'

# Test behavior of smtpd.DebuggingServer
class DebuggingServerTests(TestCase):

    def setUp(self):
        self.old_stdout = sys.stdout
        self.output = StringIO.StringIO()
        sys.stdout = self.output

        self.evt = threading.Event()
        threading.Thread(target=debugging_server, args=(self.evt,)).start()
        time.sleep(.5)

    def tearDown(self):
        self.evt.wait()
        sys.stdout = self.old_stdout

    def testBasic(self):
        # connect
        smtp = smtplib.SMTP(HOST, PORT)
        smtp.sock.close()

    def testEHLO(self):
        smtp = smtplib.SMTP(HOST, PORT)
        self.assertEqual(smtp.ehlo(), (502, 'Error: command "EHLO" not implemented'))
        smtp.sock.close()

    def testHELP(self):
        smtp = smtplib.SMTP(HOST, PORT)
        self.assertEqual(smtp.help(), 'Error: command "HELP" not implemented')
        smtp.sock.close()

    def testSend(self):
        # connect and send mail
        m = 'A test message'
        smtp = smtplib.SMTP(HOST, PORT)
        smtp.sendmail('John', 'Sally', m)
        smtp.sock.close()

        self.evt.wait()
        self.output.flush()
        mexpect = '%s%s\n%s' % (MSG_BEGIN, m, MSG_END)
        self.assertEqual(self.output.getvalue(), mexpect)


class BadHELOServerTests(TestCase):

    def setUp(self):
        self.old_stdout = sys.stdout
        self.output = StringIO.StringIO()
        sys.stdout = self.output

        self.evt = threading.Event()
        servargs = (self.evt, "199 no hello for you!\n")
        threading.Thread(target=server, args=servargs).start()
        time.sleep(.5)

    def tearDown(self):
        self.evt.wait()
        sys.stdout = self.old_stdout

    def testFailingHELO(self):
        self.assertRaises(smtplib.SMTPConnectError, smtplib.SMTP, HOST, PORT)

def test_main(verbose=None):
    test_support.run_unittest(GeneralTests, DebuggingServerTests, BadHELOServerTests)

if __name__ == '__main__':
    test_main()
