import asyncore
import socket
import threading
import smtpd
import smtplib
import io
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
    try:
        serv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        serv.settimeout(3)
        serv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        serv.bind(("", 0))
        global PORT
        PORT = serv.getsockname()[1]
        serv.listen(5)
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
            time.sleep(0.01)

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

        # wait until server thread has assigned a port number
        n = 500
        while PORT is None and n > 0:
            time.sleep(0.01)
            n -= 1

        # wait a little longer (sometimes connections are refused
        # on slow machines without this additional wait)
        time.sleep(0.5)

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
        self.assertRaises(socket.error, smtplib.SMTP,
                          "localhost", "bogus")

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
def debugging_server(serv_evt, client_evt):
    serv = smtpd.DebuggingServer(("", 0), ('nowhere', -1))
    global PORT
    PORT = serv.getsockname()[1]

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
        # allow some time for the client to read the result
        time.sleep(0.5)
        serv.close()
        asyncore.close_all()
        PORT = None
        time.sleep(0.5)
        serv_evt.set()

MSG_BEGIN = '---------- MESSAGE FOLLOWS ----------\n'
MSG_END = '------------ END MESSAGE ------------\n'

# Test behavior of smtpd.DebuggingServer
# NOTE: the SMTP objects are created with a non-default local_hostname
# argument to the constructor, since (on some systems) the FQDN lookup
# caused by the default local_hostname sometimes takes so long that the
# test server times out, causing the test to fail.
class DebuggingServerTests(TestCase):

    def setUp(self):
        # temporarily replace sys.stdout to capture DebuggingServer output
        self.old_stdout = sys.stdout
        self.output = io.StringIO()
        sys.stdout = self.output

        self.serv_evt = threading.Event()
        self.client_evt = threading.Event()
        serv_args = (self.serv_evt, self.client_evt)
        threading.Thread(target=debugging_server, args=serv_args).start()

        # wait until server thread has assigned a port number
        n = 500
        while PORT is None and n > 0:
            time.sleep(0.01)
            n -= 1

        # wait a little longer (sometimes connections are refused
        # on slow machines without this additional wait)
        time.sleep(0.5)

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

    def testEHLO(self):
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)
        expected = (502, b'Error: command "EHLO" not implemented')
        self.assertEqual(smtp.ehlo(), expected)
        smtp.quit()

    def testHELP(self):
        smtp = smtplib.SMTP(HOST, PORT, local_hostname='localhost', timeout=3)
        self.assertEqual(smtp.help(), b'Error: command "HELP" not implemented')
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


class BadHELOServerTests(TestCase):

    def setUp(self):
        self.old_stdout = sys.stdout
        self.output = io.StringIO()
        sys.stdout = self.output

        self.evt = threading.Event()
        servargs = (self.evt, b"199 no hello for you!\n")
        threading.Thread(target=server, args=servargs).start()

        # wait until server thread has assigned a port number
        n = 500
        while PORT is None and n > 0:
            time.sleep(0.01)
            n -= 1

        # wait a little longer (sometimes connections are refused
        # on slow machines without this additional wait)
        time.sleep(0.5)

    def tearDown(self):
        self.evt.wait()
        sys.stdout = self.old_stdout

    def testFailingHELO(self):
        self.assertRaises(smtplib.SMTPConnectError, smtplib.SMTP,
                            HOST, PORT, 'localhost', 3)

def test_main(verbose=None):
    test_support.run_unittest(GeneralTests, DebuggingServerTests,
                              BadHELOServerTests)

if __name__ == '__main__':
    test_main()
