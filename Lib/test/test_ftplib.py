"""Test script for ftplib module."""

# Modified by Giampaolo Rodola' to test FTP class and IPv6 environment

import ftplib
import threading
import asyncore
import asynchat
import socket
import StringIO

from unittest import TestCase
from test import test_support
from test.test_support import HOST


# the dummy data returned by server over the data channel when
# RETR, LIST and NLST commands are issued
RETR_DATA = 'abcde12345\r\n' * 1000
LIST_DATA = 'foo\r\nbar\r\n'
NLST_DATA = 'foo\r\nbar\r\n'


class DummyDTPHandler(asynchat.async_chat):

    def __init__(self, conn, baseclass):
        asynchat.async_chat.__init__(self, conn)
        self.baseclass = baseclass
        self.baseclass.last_received_data = ''

    def handle_read(self):
        self.baseclass.last_received_data += self.recv(1024)

    def handle_close(self):
        self.baseclass.push('226 transfer complete')
        self.close()


class DummyFTPHandler(asynchat.async_chat):

    def __init__(self, conn):
        asynchat.async_chat.__init__(self, conn)
        self.set_terminator("\r\n")
        self.in_buffer = []
        self.dtp = None
        self.last_received_cmd = None
        self.last_received_data = ''
        self.next_response = ''
        self.push('220 welcome')

    def collect_incoming_data(self, data):
        self.in_buffer.append(data)

    def found_terminator(self):
        line = ''.join(self.in_buffer)
        self.in_buffer = []
        if self.next_response:
            self.push(self.next_response)
            self.next_response = ''
        cmd = line.split(' ')[0].lower()
        self.last_received_cmd = cmd
        space = line.find(' ')
        if space != -1:
            arg = line[space + 1:]
        else:
            arg = ""
        if hasattr(self, 'cmd_' + cmd):
            method = getattr(self, 'cmd_' + cmd)
            method(arg)
        else:
            self.push('550 command "%s" not understood.' %cmd)

    def handle_error(self):
        raise

    def push(self, data):
        asynchat.async_chat.push(self, data + '\r\n')

    def cmd_port(self, arg):
        addr = map(int, arg.split(','))
        ip = '%d.%d.%d.%d' %tuple(addr[:4])
        port = (addr[4] * 256) + addr[5]
        s = socket.create_connection((ip, port), timeout=2)
        self.dtp = DummyDTPHandler(s, baseclass=self)
        self.push('200 active data connection established')

    def cmd_pasv(self, arg):
        sock = socket.socket()
        sock.bind((self.socket.getsockname()[0], 0))
        sock.listen(5)
        sock.settimeout(2)
        ip, port = sock.getsockname()[:2]
        ip = ip.replace('.', ','); p1 = port / 256; p2 = port % 256
        self.push('227 entering passive mode (%s,%d,%d)' %(ip, p1, p2))
        conn, addr = sock.accept()
        self.dtp = DummyDTPHandler(conn, baseclass=self)

    def cmd_eprt(self, arg):
        af, ip, port = arg.split(arg[0])[1:-1]
        port = int(port)
        s = socket.create_connection((ip, port), timeout=2)
        self.dtp = DummyDTPHandler(s, baseclass=self)
        self.push('200 active data connection established')

    def cmd_epsv(self, arg):
        sock = socket.socket(socket.AF_INET6)
        sock.bind((self.socket.getsockname()[0], 0))
        sock.listen(5)
        sock.settimeout(2)
        port = sock.getsockname()[1]
        self.push('229 entering extended passive mode (|||%d|)' %port)
        conn, addr = sock.accept()
        self.dtp = DummyDTPHandler(conn, baseclass=self)

    def cmd_echo(self, arg):
        # sends back the received string (used by the test suite)
        self.push(arg)

    def cmd_user(self, arg):
        self.push('331 username ok')

    def cmd_pass(self, arg):
        self.push('230 password ok')

    def cmd_acct(self, arg):
        self.push('230 acct ok')

    def cmd_rnfr(self, arg):
        self.push('350 rnfr ok')

    def cmd_rnto(self, arg):
        self.push('250 rnto ok')

    def cmd_dele(self, arg):
        self.push('250 dele ok')

    def cmd_cwd(self, arg):
        self.push('250 cwd ok')

    def cmd_size(self, arg):
        self.push('250 1000')

    def cmd_mkd(self, arg):
        self.push('257 "%s"' %arg)

    def cmd_rmd(self, arg):
        self.push('250 rmd ok')

    def cmd_pwd(self, arg):
        self.push('257 "pwd ok"')

    def cmd_type(self, arg):
        self.push('200 type ok')

    def cmd_quit(self, arg):
        self.push('221 quit ok')
        self.close()

    def cmd_stor(self, arg):
        self.push('125 stor ok')

    def cmd_retr(self, arg):
        self.push('125 retr ok')
        self.dtp.push(RETR_DATA)
        self.dtp.close_when_done()

    def cmd_list(self, arg):
        self.push('125 list ok')
        self.dtp.push(LIST_DATA)
        self.dtp.close_when_done()

    def cmd_nlst(self, arg):
        self.push('125 nlst ok')
        self.dtp.push(NLST_DATA)
        self.dtp.close_when_done()


class DummyFTPServer(asyncore.dispatcher, threading.Thread):

    handler = DummyFTPHandler

    def __init__(self, address, af=socket.AF_INET):
        threading.Thread.__init__(self)
        asyncore.dispatcher.__init__(self)
        self.create_socket(af, socket.SOCK_STREAM)
        self.bind(address)
        self.listen(5)
        self.active = False
        self.active_lock = threading.Lock()
        self.host, self.port = self.socket.getsockname()[:2]

    def start(self):
        assert not self.active
        self.__flag = threading.Event()
        threading.Thread.start(self)
        self.__flag.wait()

    def run(self):
        self.active = True
        self.__flag.set()
        while self.active and asyncore.socket_map:
            self.active_lock.acquire()
            asyncore.loop(timeout=0.1, count=1)
            self.active_lock.release()
        asyncore.close_all(ignore_all=True)

    def stop(self):
        assert self.active
        self.active = False
        self.join()

    def handle_accept(self):
        conn, addr = self.accept()
        self.handler = self.handler(conn)
        self.close()

    def handle_connect(self):
        self.close()
    handle_read = handle_connect

    def writable(self):
        return 0

    def handle_error(self):
        raise


class TestFTPClass(TestCase):

    def setUp(self):
        self.server = DummyFTPServer((HOST, 0))
        self.server.start()
        self.client = ftplib.FTP(timeout=2)
        self.client.connect(self.server.host, self.server.port)

    def tearDown(self):
        self.client.close()
        self.server.stop()

    def test_getwelcome(self):
        self.assertEqual(self.client.getwelcome(), '220 welcome')

    def test_sanitize(self):
        self.assertEqual(self.client.sanitize('foo'), repr('foo'))
        self.assertEqual(self.client.sanitize('pass 12345'), repr('pass *****'))
        self.assertEqual(self.client.sanitize('PASS 12345'), repr('PASS *****'))

    def test_exceptions(self):
        self.assertRaises(ftplib.error_temp, self.client.sendcmd, 'echo 400')
        self.assertRaises(ftplib.error_temp, self.client.sendcmd, 'echo 499')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'echo 500')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'echo 599')
        self.assertRaises(ftplib.error_proto, self.client.sendcmd, 'echo 999')

    def test_all_errors(self):
        exceptions = (ftplib.error_reply, ftplib.error_temp, ftplib.error_perm,
                      ftplib.error_proto, ftplib.Error, IOError, EOFError)
        for x in exceptions:
            try:
                raise x('exception not included in all_errors set')
            except ftplib.all_errors:
                pass

    def test_set_pasv(self):
        # passive mode is supposed to be enabled by default
        self.assertTrue(self.client.passiveserver)
        self.client.set_pasv(True)
        self.assertTrue(self.client.passiveserver)
        self.client.set_pasv(False)
        self.assertFalse(self.client.passiveserver)

    def test_voidcmd(self):
        self.client.voidcmd('echo 200')
        self.client.voidcmd('echo 299')
        self.assertRaises(ftplib.error_reply, self.client.voidcmd, 'echo 199')
        self.assertRaises(ftplib.error_reply, self.client.voidcmd, 'echo 300')

    def test_login(self):
        self.client.login()

    def test_acct(self):
        self.client.acct('passwd')

    def test_rename(self):
        self.client.rename('a', 'b')
        self.server.handler.next_response = '200'
        self.assertRaises(ftplib.error_reply, self.client.rename, 'a', 'b')

    def test_delete(self):
        self.client.delete('foo')
        self.server.handler.next_response = '199'
        self.assertRaises(ftplib.error_reply, self.client.delete, 'foo')

    def test_size(self):
        self.client.size('foo')

    def test_mkd(self):
        dir = self.client.mkd('/foo')
        self.assertEqual(dir, '/foo')

    def test_rmd(self):
        self.client.rmd('foo')

    def test_pwd(self):
        dir = self.client.pwd()
        self.assertEqual(dir, 'pwd ok')

    def test_quit(self):
        self.assertEqual(self.client.quit(), '221 quit ok')
        # Ensure the connection gets closed; sock attribute should be None
        self.assertEqual(self.client.sock, None)

    def test_retrbinary(self):
        received = []
        self.client.retrbinary('retr', received.append)
        self.assertEqual(''.join(received), RETR_DATA)

    def test_retrlines(self):
        received = []
        self.client.retrlines('retr', received.append)
        self.assertEqual(''.join(received), RETR_DATA.replace('\r\n', ''))

    def test_storbinary(self):
        f = StringIO.StringIO(RETR_DATA)
        self.client.storbinary('stor', f)
        self.assertEqual(self.server.handler.last_received_data, RETR_DATA)
        # test new callback arg
        flag = []
        f.seek(0)
        self.client.storbinary('stor', f, callback=lambda x: flag.append(None))
        self.assertTrue(flag)

    def test_storlines(self):
        f = StringIO.StringIO(RETR_DATA.replace('\r\n', '\n'))
        self.client.storlines('stor', f)
        self.assertEqual(self.server.handler.last_received_data, RETR_DATA)
        # test new callback arg
        flag = []
        f.seek(0)
        self.client.storlines('stor foo', f, callback=lambda x: flag.append(None))
        self.assertTrue(flag)

    def test_nlst(self):
        self.client.nlst()
        self.assertEqual(self.client.nlst(), NLST_DATA.split('\r\n')[:-1])

    def test_dir(self):
        l = []
        self.client.dir(lambda x: l.append(x))
        self.assertEqual(''.join(l), LIST_DATA.replace('\r\n', ''))

    def test_makeport(self):
        self.client.makeport()
        # IPv4 is in use, just make sure send_eprt has not been used
        self.assertEqual(self.server.handler.last_received_cmd, 'port')

    def test_makepasv(self):
        host, port = self.client.makepasv()
        conn = socket.create_connection((host, port), 2)
        conn.close()
        # IPv4 is in use, just make sure send_epsv has not been used
        self.assertEqual(self.server.handler.last_received_cmd, 'pasv')


class TestIPv6Environment(TestCase):

    def setUp(self):
        self.server = DummyFTPServer((HOST, 0), af=socket.AF_INET6)
        self.server.start()
        self.client = ftplib.FTP()
        self.client.connect(self.server.host, self.server.port)

    def tearDown(self):
        self.client.close()
        self.server.stop()

    def test_af(self):
        self.assertEqual(self.client.af, socket.AF_INET6)

    def test_makeport(self):
        self.client.makeport()
        self.assertEqual(self.server.handler.last_received_cmd, 'eprt')

    def test_makepasv(self):
        host, port = self.client.makepasv()
        conn = socket.create_connection((host, port), 2)
        conn.close()
        self.assertEqual(self.server.handler.last_received_cmd, 'epsv')

    def test_transfer(self):
        def retr():
            received = []
            self.client.retrbinary('retr', received.append)
            self.assertEqual(''.join(received), RETR_DATA)
        self.client.set_pasv(True)
        retr()
        self.client.set_pasv(False)
        retr()


class TestTimeouts(TestCase):

    def setUp(self):
        self.evt = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(3)
        self.port = test_support.bind_port(self.sock)
        threading.Thread(target=self.server, args=(self.evt,self.sock)).start()
        # Wait for the server to be ready.
        self.evt.wait()
        self.evt.clear()
        ftplib.FTP.port = self.port

    def tearDown(self):
        self.evt.wait()

    def server(self, evt, serv):
        # This method sets the evt 3 times:
        #  1) when the connection is ready to be accepted.
        #  2) when it is safe for the caller to close the connection
        #  3) when we have closed the socket
        serv.listen(5)
        # (1) Signal the caller that we are ready to accept the connection.
        evt.set()
        try:
            conn, addr = serv.accept()
        except socket.timeout:
            pass
        else:
            conn.send("1 Hola mundo\n")
            # (2) Signal the caller that it is safe to close the socket.
            evt.set()
            conn.close()
        finally:
            serv.close()
            # (3) Signal the caller that we are done.
            evt.set()

    def testTimeoutDefault(self):
        # default -- use global socket timeout
        self.assert_(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(30)
        try:
            ftp = ftplib.FTP("localhost")
        finally:
            socket.setdefaulttimeout(None)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.close()

    def testTimeoutNone(self):
        # no timeout -- do not use global socket timeout
        self.assert_(socket.getdefaulttimeout() is None)
        socket.setdefaulttimeout(30)
        try:
            ftp = ftplib.FTP("localhost", timeout=None)
        finally:
            socket.setdefaulttimeout(None)
        self.assertTrue(ftp.sock.gettimeout() is None)
        self.evt.wait()
        ftp.close()

    def testTimeoutValue(self):
        # a value
        ftp = ftplib.FTP(HOST, timeout=30)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.close()

    def testTimeoutConnect(self):
        ftp = ftplib.FTP()
        ftp.connect(HOST, timeout=30)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.close()

    def testTimeoutDifferentOrder(self):
        ftp = ftplib.FTP(timeout=30)
        ftp.connect(HOST)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.close()

    def testTimeoutDirectAccess(self):
        ftp = ftplib.FTP()
        ftp.timeout = 30
        ftp.connect(HOST)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.close()


def test_main():
    tests = [TestFTPClass, TestTimeouts]
    if socket.has_ipv6:
        try:
            DummyFTPServer((HOST, 0), af=socket.AF_INET6)
        except socket.error:
            pass
        else:
            tests.append(TestIPv6Environment)
    thread_info = test_support.threading_setup()
    try:
        test_support.run_unittest(*tests)
    finally:
        test_support.threading_cleanup(*thread_info)


if __name__ == '__main__':
    test_main()
