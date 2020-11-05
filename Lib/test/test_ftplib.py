"""Test script for ftplib module."""

# Modified by Giampaolo Rodola' to test FTP class, IPv6 and TLS
# environment

import ftplib
import asyncore
import asynchat
import socket
import io
import errno
import os
import threading
import time
try:
    import ssl
except ImportError:
    ssl = None

from unittest import TestCase, skipUnless
from test import support
from test.support import threading_helper
from test.support import socket_helper
from test.support import warnings_helper
from test.support.socket_helper import HOST, HOSTv6

TIMEOUT = support.LOOPBACK_TIMEOUT
DEFAULT_ENCODING = 'utf-8'
# the dummy data returned by server over the data channel when
# RETR, LIST, NLST, MLSD commands are issued
RETR_DATA = 'abcde12345\r\n' * 1000 + 'non-ascii char \xAE\r\n'
LIST_DATA = 'foo\r\nbar\r\n non-ascii char \xAE\r\n'
NLST_DATA = 'foo\r\nbar\r\n non-ascii char \xAE\r\n'
MLSD_DATA = ("type=cdir;perm=el;unique==keVO1+ZF4; test\r\n"
             "type=pdir;perm=e;unique==keVO1+d?3; ..\r\n"
             "type=OS.unix=slink:/foobar;perm=;unique==keVO1+4G4; foobar\r\n"
             "type=OS.unix=chr-13/29;perm=;unique==keVO1+5G4; device\r\n"
             "type=OS.unix=blk-11/108;perm=;unique==keVO1+6G4; block\r\n"
             "type=file;perm=awr;unique==keVO1+8G4; writable\r\n"
             "type=dir;perm=cpmel;unique==keVO1+7G4; promiscuous\r\n"
             "type=dir;perm=;unique==keVO1+1t2; no-exec\r\n"
             "type=file;perm=r;unique==keVO1+EG4; two words\r\n"
             "type=file;perm=r;unique==keVO1+IH4;  leading space\r\n"
             "type=file;perm=r;unique==keVO1+1G4; file1\r\n"
             "type=dir;perm=cpmel;unique==keVO1+7G4; incoming\r\n"
             "type=file;perm=r;unique==keVO1+1G4; file2\r\n"
             "type=file;perm=r;unique==keVO1+1G4; file3\r\n"
             "type=file;perm=r;unique==keVO1+1G4; file4\r\n"
             "type=dir;perm=cpmel;unique==SGP1; dir \xAE non-ascii char\r\n"
             "type=file;perm=r;unique==SGP2; file \xAE non-ascii char\r\n")


class DummyDTPHandler(asynchat.async_chat):
    dtp_conn_closed = False

    def __init__(self, conn, baseclass):
        asynchat.async_chat.__init__(self, conn)
        self.baseclass = baseclass
        self.baseclass.last_received_data = ''
        self.encoding = baseclass.encoding

    def handle_read(self):
        new_data = self.recv(1024).decode(self.encoding, 'replace')
        self.baseclass.last_received_data += new_data

    def handle_close(self):
        # XXX: this method can be called many times in a row for a single
        # connection, including in clear-text (non-TLS) mode.
        # (behaviour witnessed with test_data_connection)
        if not self.dtp_conn_closed:
            self.baseclass.push('226 transfer complete')
            self.close()
            self.dtp_conn_closed = True

    def push(self, what):
        if self.baseclass.next_data is not None:
            what = self.baseclass.next_data
            self.baseclass.next_data = None
        if not what:
            return self.close_when_done()
        super(DummyDTPHandler, self).push(what.encode(self.encoding))

    def handle_error(self):
        raise Exception


class DummyFTPHandler(asynchat.async_chat):

    dtp_handler = DummyDTPHandler

    def __init__(self, conn, encoding=DEFAULT_ENCODING):
        asynchat.async_chat.__init__(self, conn)
        # tells the socket to handle urgent data inline (ABOR command)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_OOBINLINE, 1)
        self.set_terminator(b"\r\n")
        self.in_buffer = []
        self.dtp = None
        self.last_received_cmd = None
        self.last_received_data = ''
        self.next_response = ''
        self.next_data = None
        self.rest = None
        self.next_retr_data = RETR_DATA
        self.push('220 welcome')
        self.encoding = encoding

    def collect_incoming_data(self, data):
        self.in_buffer.append(data)

    def found_terminator(self):
        line = b''.join(self.in_buffer).decode(self.encoding)
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
        raise Exception

    def push(self, data):
        asynchat.async_chat.push(self, data.encode(self.encoding) + b'\r\n')

    def cmd_port(self, arg):
        addr = list(map(int, arg.split(',')))
        ip = '%d.%d.%d.%d' %tuple(addr[:4])
        port = (addr[4] * 256) + addr[5]
        s = socket.create_connection((ip, port), timeout=TIMEOUT)
        self.dtp = self.dtp_handler(s, baseclass=self)
        self.push('200 active data connection established')

    def cmd_pasv(self, arg):
        with socket.create_server((self.socket.getsockname()[0], 0)) as sock:
            sock.settimeout(TIMEOUT)
            ip, port = sock.getsockname()[:2]
            ip = ip.replace('.', ','); p1 = port / 256; p2 = port % 256
            self.push('227 entering passive mode (%s,%d,%d)' %(ip, p1, p2))
            conn, addr = sock.accept()
            self.dtp = self.dtp_handler(conn, baseclass=self)

    def cmd_eprt(self, arg):
        af, ip, port = arg.split(arg[0])[1:-1]
        port = int(port)
        s = socket.create_connection((ip, port), timeout=TIMEOUT)
        self.dtp = self.dtp_handler(s, baseclass=self)
        self.push('200 active data connection established')

    def cmd_epsv(self, arg):
        with socket.create_server((self.socket.getsockname()[0], 0),
                                  family=socket.AF_INET6) as sock:
            sock.settimeout(TIMEOUT)
            port = sock.getsockname()[1]
            self.push('229 entering extended passive mode (|||%d|)' %port)
            conn, addr = sock.accept()
            self.dtp = self.dtp_handler(conn, baseclass=self)

    def cmd_echo(self, arg):
        # sends back the received string (used by the test suite)
        self.push(arg)

    def cmd_noop(self, arg):
        self.push('200 noop ok')

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

    def cmd_abor(self, arg):
        self.push('226 abor ok')

    def cmd_stor(self, arg):
        self.push('125 stor ok')

    def cmd_rest(self, arg):
        self.rest = arg
        self.push('350 rest ok')

    def cmd_retr(self, arg):
        self.push('125 retr ok')
        if self.rest is not None:
            offset = int(self.rest)
        else:
            offset = 0
        self.dtp.push(self.next_retr_data[offset:])
        self.dtp.close_when_done()
        self.rest = None

    def cmd_list(self, arg):
        self.push('125 list ok')
        self.dtp.push(LIST_DATA)
        self.dtp.close_when_done()

    def cmd_nlst(self, arg):
        self.push('125 nlst ok')
        self.dtp.push(NLST_DATA)
        self.dtp.close_when_done()

    def cmd_opts(self, arg):
        self.push('200 opts ok')

    def cmd_mlsd(self, arg):
        self.push('125 mlsd ok')
        self.dtp.push(MLSD_DATA)
        self.dtp.close_when_done()

    def cmd_setlongretr(self, arg):
        # For testing. Next RETR will return long line.
        self.next_retr_data = 'x' * int(arg)
        self.push('125 setlongretr ok')


class DummyFTPServer(asyncore.dispatcher, threading.Thread):

    handler = DummyFTPHandler

    def __init__(self, address, af=socket.AF_INET, encoding=DEFAULT_ENCODING):
        threading.Thread.__init__(self)
        asyncore.dispatcher.__init__(self)
        self.daemon = True
        self.create_socket(af, socket.SOCK_STREAM)
        self.bind(address)
        self.listen(5)
        self.active = False
        self.active_lock = threading.Lock()
        self.host, self.port = self.socket.getsockname()[:2]
        self.handler_instance = None
        self.encoding = encoding

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

    def handle_accepted(self, conn, addr):
        self.handler_instance = self.handler(conn, encoding=self.encoding)

    def handle_connect(self):
        self.close()
    handle_read = handle_connect

    def writable(self):
        return 0

    def handle_error(self):
        raise Exception


if ssl is not None:

    CERTFILE = os.path.join(os.path.dirname(__file__), "keycert3.pem")
    CAFILE = os.path.join(os.path.dirname(__file__), "pycacert.pem")

    class SSLConnection(asyncore.dispatcher):
        """An asyncore.dispatcher subclass supporting TLS/SSL."""

        _ssl_accepting = False
        _ssl_closing = False

        def secure_connection(self):
            context = ssl.SSLContext()
            context.load_cert_chain(CERTFILE)
            socket = context.wrap_socket(self.socket,
                                         suppress_ragged_eofs=False,
                                         server_side=True,
                                         do_handshake_on_connect=False)
            self.del_channel()
            self.set_socket(socket)
            self._ssl_accepting = True

        def _do_ssl_handshake(self):
            try:
                self.socket.do_handshake()
            except ssl.SSLError as err:
                if err.args[0] in (ssl.SSL_ERROR_WANT_READ,
                                   ssl.SSL_ERROR_WANT_WRITE):
                    return
                elif err.args[0] == ssl.SSL_ERROR_EOF:
                    return self.handle_close()
                # TODO: SSLError does not expose alert information
                elif "SSLV3_ALERT_BAD_CERTIFICATE" in err.args[1]:
                    return self.handle_close()
                raise
            except OSError as err:
                if err.args[0] == errno.ECONNABORTED:
                    return self.handle_close()
            else:
                self._ssl_accepting = False

        def _do_ssl_shutdown(self):
            self._ssl_closing = True
            try:
                self.socket = self.socket.unwrap()
            except ssl.SSLError as err:
                if err.args[0] in (ssl.SSL_ERROR_WANT_READ,
                                   ssl.SSL_ERROR_WANT_WRITE):
                    return
            except OSError:
                # Any "socket error" corresponds to a SSL_ERROR_SYSCALL return
                # from OpenSSL's SSL_shutdown(), corresponding to a
                # closed socket condition. See also:
                # http://www.mail-archive.com/openssl-users@openssl.org/msg60710.html
                pass
            self._ssl_closing = False
            if getattr(self, '_ccc', False) is False:
                super(SSLConnection, self).close()
            else:
                pass

        def handle_read_event(self):
            if self._ssl_accepting:
                self._do_ssl_handshake()
            elif self._ssl_closing:
                self._do_ssl_shutdown()
            else:
                super(SSLConnection, self).handle_read_event()

        def handle_write_event(self):
            if self._ssl_accepting:
                self._do_ssl_handshake()
            elif self._ssl_closing:
                self._do_ssl_shutdown()
            else:
                super(SSLConnection, self).handle_write_event()

        def send(self, data):
            try:
                return super(SSLConnection, self).send(data)
            except ssl.SSLError as err:
                if err.args[0] in (ssl.SSL_ERROR_EOF, ssl.SSL_ERROR_ZERO_RETURN,
                                   ssl.SSL_ERROR_WANT_READ,
                                   ssl.SSL_ERROR_WANT_WRITE):
                    return 0
                raise

        def recv(self, buffer_size):
            try:
                return super(SSLConnection, self).recv(buffer_size)
            except ssl.SSLError as err:
                if err.args[0] in (ssl.SSL_ERROR_WANT_READ,
                                   ssl.SSL_ERROR_WANT_WRITE):
                    return b''
                if err.args[0] in (ssl.SSL_ERROR_EOF, ssl.SSL_ERROR_ZERO_RETURN):
                    self.handle_close()
                    return b''
                raise

        def handle_error(self):
            raise Exception

        def close(self):
            if (isinstance(self.socket, ssl.SSLSocket) and
                    self.socket._sslobj is not None):
                self._do_ssl_shutdown()
            else:
                super(SSLConnection, self).close()


    class DummyTLS_DTPHandler(SSLConnection, DummyDTPHandler):
        """A DummyDTPHandler subclass supporting TLS/SSL."""

        def __init__(self, conn, baseclass):
            DummyDTPHandler.__init__(self, conn, baseclass)
            if self.baseclass.secure_data_channel:
                self.secure_connection()


    class DummyTLS_FTPHandler(SSLConnection, DummyFTPHandler):
        """A DummyFTPHandler subclass supporting TLS/SSL."""

        dtp_handler = DummyTLS_DTPHandler

        def __init__(self, conn, encoding=DEFAULT_ENCODING):
            DummyFTPHandler.__init__(self, conn, encoding=encoding)
            self.secure_data_channel = False
            self._ccc = False

        def cmd_auth(self, line):
            """Set up secure control channel."""
            self.push('234 AUTH TLS successful')
            self.secure_connection()

        def cmd_ccc(self, line):
            self.push('220 Reverting back to clear-text')
            self._ccc = True
            self._do_ssl_shutdown()

        def cmd_pbsz(self, line):
            """Negotiate size of buffer for secure data transfer.
            For TLS/SSL the only valid value for the parameter is '0'.
            Any other value is accepted but ignored.
            """
            self.push('200 PBSZ=0 successful.')

        def cmd_prot(self, line):
            """Setup un/secure data channel."""
            arg = line.upper()
            if arg == 'C':
                self.push('200 Protection set to Clear')
                self.secure_data_channel = False
            elif arg == 'P':
                self.push('200 Protection set to Private')
                self.secure_data_channel = True
            else:
                self.push("502 Unrecognized PROT type (use C or P).")


    class DummyTLS_FTPServer(DummyFTPServer):
        handler = DummyTLS_FTPHandler


class TestFTPClass(TestCase):

    def setUp(self, encoding=DEFAULT_ENCODING):
        self.server = DummyFTPServer((HOST, 0), encoding=encoding)
        self.server.start()
        self.client = ftplib.FTP(timeout=TIMEOUT, encoding=encoding)
        self.client.connect(self.server.host, self.server.port)

    def tearDown(self):
        self.client.close()
        self.server.stop()
        # Explicitly clear the attribute to prevent dangling thread
        self.server = None
        asyncore.close_all(ignore_all=True)

    def check_data(self, received, expected):
        self.assertEqual(len(received), len(expected))
        self.assertEqual(received, expected)

    def test_getwelcome(self):
        self.assertEqual(self.client.getwelcome(), '220 welcome')

    def test_sanitize(self):
        self.assertEqual(self.client.sanitize('foo'), repr('foo'))
        self.assertEqual(self.client.sanitize('pass 12345'), repr('pass *****'))
        self.assertEqual(self.client.sanitize('PASS 12345'), repr('PASS *****'))

    def test_exceptions(self):
        self.assertRaises(ValueError, self.client.sendcmd, 'echo 40\r\n0')
        self.assertRaises(ValueError, self.client.sendcmd, 'echo 40\n0')
        self.assertRaises(ValueError, self.client.sendcmd, 'echo 40\r0')
        self.assertRaises(ftplib.error_temp, self.client.sendcmd, 'echo 400')
        self.assertRaises(ftplib.error_temp, self.client.sendcmd, 'echo 499')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'echo 500')
        self.assertRaises(ftplib.error_perm, self.client.sendcmd, 'echo 599')
        self.assertRaises(ftplib.error_proto, self.client.sendcmd, 'echo 999')

    def test_all_errors(self):
        exceptions = (ftplib.error_reply, ftplib.error_temp, ftplib.error_perm,
                      ftplib.error_proto, ftplib.Error, OSError,
                      EOFError)
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
        self.server.handler_instance.next_response = '200'
        self.assertRaises(ftplib.error_reply, self.client.rename, 'a', 'b')

    def test_delete(self):
        self.client.delete('foo')
        self.server.handler_instance.next_response = '199'
        self.assertRaises(ftplib.error_reply, self.client.delete, 'foo')

    def test_size(self):
        self.client.size('foo')

    def test_mkd(self):
        dir = self.client.mkd('/foo')
        self.assertEqual(dir, '/foo')

    def test_rmd(self):
        self.client.rmd('foo')

    def test_cwd(self):
        dir = self.client.cwd('/foo')
        self.assertEqual(dir, '250 cwd ok')

    def test_pwd(self):
        dir = self.client.pwd()
        self.assertEqual(dir, 'pwd ok')

    def test_quit(self):
        self.assertEqual(self.client.quit(), '221 quit ok')
        # Ensure the connection gets closed; sock attribute should be None
        self.assertEqual(self.client.sock, None)

    def test_abort(self):
        self.client.abort()

    def test_retrbinary(self):
        def callback(data):
            received.append(data.decode(self.client.encoding))
        received = []
        self.client.retrbinary('retr', callback)
        self.check_data(''.join(received), RETR_DATA)

    def test_retrbinary_rest(self):
        def callback(data):
            received.append(data.decode(self.client.encoding))
        for rest in (0, 10, 20):
            received = []
            self.client.retrbinary('retr', callback, rest=rest)
            self.check_data(''.join(received), RETR_DATA[rest:])

    def test_retrlines(self):
        received = []
        self.client.retrlines('retr', received.append)
        self.check_data(''.join(received), RETR_DATA.replace('\r\n', ''))

    def test_storbinary(self):
        f = io.BytesIO(RETR_DATA.encode(self.client.encoding))
        self.client.storbinary('stor', f)
        self.check_data(self.server.handler_instance.last_received_data, RETR_DATA)
        # test new callback arg
        flag = []
        f.seek(0)
        self.client.storbinary('stor', f, callback=lambda x: flag.append(None))
        self.assertTrue(flag)

    def test_storbinary_rest(self):
        data = RETR_DATA.replace('\r\n', '\n').encode(self.client.encoding)
        f = io.BytesIO(data)
        for r in (30, '30'):
            f.seek(0)
            self.client.storbinary('stor', f, rest=r)
            self.assertEqual(self.server.handler_instance.rest, str(r))

    def test_storlines(self):
        data = RETR_DATA.replace('\r\n', '\n').encode(self.client.encoding)
        f = io.BytesIO(data)
        self.client.storlines('stor', f)
        self.check_data(self.server.handler_instance.last_received_data, RETR_DATA)
        # test new callback arg
        flag = []
        f.seek(0)
        self.client.storlines('stor foo', f, callback=lambda x: flag.append(None))
        self.assertTrue(flag)

        f = io.StringIO(RETR_DATA.replace('\r\n', '\n'))
        # storlines() expects a binary file, not a text file
        with warnings_helper.check_warnings(('', BytesWarning), quiet=True):
            self.assertRaises(TypeError, self.client.storlines, 'stor foo', f)

    def test_nlst(self):
        self.client.nlst()
        self.assertEqual(self.client.nlst(), NLST_DATA.split('\r\n')[:-1])

    def test_dir(self):
        l = []
        self.client.dir(lambda x: l.append(x))
        self.assertEqual(''.join(l), LIST_DATA.replace('\r\n', ''))

    def test_mlsd(self):
        list(self.client.mlsd())
        list(self.client.mlsd(path='/'))
        list(self.client.mlsd(path='/', facts=['size', 'type']))

        ls = list(self.client.mlsd())
        for name, facts in ls:
            self.assertIsInstance(name, str)
            self.assertIsInstance(facts, dict)
            self.assertTrue(name)
            self.assertIn('type', facts)
            self.assertIn('perm', facts)
            self.assertIn('unique', facts)

        def set_data(data):
            self.server.handler_instance.next_data = data

        def test_entry(line, type=None, perm=None, unique=None, name=None):
            type = 'type' if type is None else type
            perm = 'perm' if perm is None else perm
            unique = 'unique' if unique is None else unique
            name = 'name' if name is None else name
            set_data(line)
            _name, facts = next(self.client.mlsd())
            self.assertEqual(_name, name)
            self.assertEqual(facts['type'], type)
            self.assertEqual(facts['perm'], perm)
            self.assertEqual(facts['unique'], unique)

        # plain
        test_entry('type=type;perm=perm;unique=unique; name\r\n')
        # "=" in fact value
        test_entry('type=ty=pe;perm=perm;unique=unique; name\r\n', type="ty=pe")
        test_entry('type==type;perm=perm;unique=unique; name\r\n', type="=type")
        test_entry('type=t=y=pe;perm=perm;unique=unique; name\r\n', type="t=y=pe")
        test_entry('type=====;perm=perm;unique=unique; name\r\n', type="====")
        # spaces in name
        test_entry('type=type;perm=perm;unique=unique; na me\r\n', name="na me")
        test_entry('type=type;perm=perm;unique=unique; name \r\n', name="name ")
        test_entry('type=type;perm=perm;unique=unique;  name\r\n', name=" name")
        test_entry('type=type;perm=perm;unique=unique; n am  e\r\n', name="n am  e")
        # ";" in name
        test_entry('type=type;perm=perm;unique=unique; na;me\r\n', name="na;me")
        test_entry('type=type;perm=perm;unique=unique; ;name\r\n', name=";name")
        test_entry('type=type;perm=perm;unique=unique; ;name;\r\n', name=";name;")
        test_entry('type=type;perm=perm;unique=unique; ;;;;\r\n', name=";;;;")
        # case sensitiveness
        set_data('Type=type;TyPe=perm;UNIQUE=unique; name\r\n')
        _name, facts = next(self.client.mlsd())
        for x in facts:
            self.assertTrue(x.islower())
        # no data (directory empty)
        set_data('')
        self.assertRaises(StopIteration, next, self.client.mlsd())
        set_data('')
        for x in self.client.mlsd():
            self.fail("unexpected data %s" % x)

    def test_makeport(self):
        with self.client.makeport():
            # IPv4 is in use, just make sure send_eprt has not been used
            self.assertEqual(self.server.handler_instance.last_received_cmd,
                                'port')

    def test_makepasv(self):
        host, port = self.client.makepasv()
        conn = socket.create_connection((host, port), timeout=TIMEOUT)
        conn.close()
        # IPv4 is in use, just make sure send_epsv has not been used
        self.assertEqual(self.server.handler_instance.last_received_cmd, 'pasv')

    def test_with_statement(self):
        self.client.quit()

        def is_client_connected():
            if self.client.sock is None:
                return False
            try:
                self.client.sendcmd('noop')
            except (OSError, EOFError):
                return False
            return True

        # base test
        with ftplib.FTP(timeout=TIMEOUT) as self.client:
            self.client.connect(self.server.host, self.server.port)
            self.client.sendcmd('noop')
            self.assertTrue(is_client_connected())
        self.assertEqual(self.server.handler_instance.last_received_cmd, 'quit')
        self.assertFalse(is_client_connected())

        # QUIT sent inside the with block
        with ftplib.FTP(timeout=TIMEOUT) as self.client:
            self.client.connect(self.server.host, self.server.port)
            self.client.sendcmd('noop')
            self.client.quit()
        self.assertEqual(self.server.handler_instance.last_received_cmd, 'quit')
        self.assertFalse(is_client_connected())

        # force a wrong response code to be sent on QUIT: error_perm
        # is expected and the connection is supposed to be closed
        try:
            with ftplib.FTP(timeout=TIMEOUT) as self.client:
                self.client.connect(self.server.host, self.server.port)
                self.client.sendcmd('noop')
                self.server.handler_instance.next_response = '550 error on quit'
        except ftplib.error_perm as err:
            self.assertEqual(str(err), '550 error on quit')
        else:
            self.fail('Exception not raised')
        # needed to give the threaded server some time to set the attribute
        # which otherwise would still be == 'noop'
        time.sleep(0.1)
        self.assertEqual(self.server.handler_instance.last_received_cmd, 'quit')
        self.assertFalse(is_client_connected())

    def test_source_address(self):
        self.client.quit()
        port = socket_helper.find_unused_port()
        try:
            self.client.connect(self.server.host, self.server.port,
                                source_address=(HOST, port))
            self.assertEqual(self.client.sock.getsockname()[1], port)
            self.client.quit()
        except OSError as e:
            if e.errno == errno.EADDRINUSE:
                self.skipTest("couldn't bind to port %d" % port)
            raise

    def test_source_address_passive_connection(self):
        port = socket_helper.find_unused_port()
        self.client.source_address = (HOST, port)
        try:
            with self.client.transfercmd('list') as sock:
                self.assertEqual(sock.getsockname()[1], port)
        except OSError as e:
            if e.errno == errno.EADDRINUSE:
                self.skipTest("couldn't bind to port %d" % port)
            raise

    def test_parse257(self):
        self.assertEqual(ftplib.parse257('257 "/foo/bar"'), '/foo/bar')
        self.assertEqual(ftplib.parse257('257 "/foo/bar" created'), '/foo/bar')
        self.assertEqual(ftplib.parse257('257 ""'), '')
        self.assertEqual(ftplib.parse257('257 "" created'), '')
        self.assertRaises(ftplib.error_reply, ftplib.parse257, '250 "/foo/bar"')
        # The 257 response is supposed to include the directory
        # name and in case it contains embedded double-quotes
        # they must be doubled (see RFC-959, chapter 7, appendix 2).
        self.assertEqual(ftplib.parse257('257 "/foo/b""ar"'), '/foo/b"ar')
        self.assertEqual(ftplib.parse257('257 "/foo/b""ar" created'), '/foo/b"ar')

    def test_line_too_long(self):
        self.assertRaises(ftplib.Error, self.client.sendcmd,
                          'x' * self.client.maxline * 2)

    def test_retrlines_too_long(self):
        self.client.sendcmd('SETLONGRETR %d' % (self.client.maxline * 2))
        received = []
        self.assertRaises(ftplib.Error,
                          self.client.retrlines, 'retr', received.append)

    def test_storlines_too_long(self):
        f = io.BytesIO(b'x' * self.client.maxline * 2)
        self.assertRaises(ftplib.Error, self.client.storlines, 'stor', f)

    def test_encoding_param(self):
        encodings = ['latin-1', 'utf-8']
        for encoding in encodings:
            with self.subTest(encoding=encoding):
                self.tearDown()
                self.setUp(encoding=encoding)
                self.assertEqual(encoding, self.client.encoding)
                self.test_retrbinary()
                self.test_storbinary()
                self.test_retrlines()
                new_dir = self.client.mkd('/non-ascii dir \xAE')
                self.check_data(new_dir, '/non-ascii dir \xAE')
        # Check default encoding
        client = ftplib.FTP(timeout=TIMEOUT)
        self.assertEqual(DEFAULT_ENCODING, client.encoding)


@skipUnless(socket_helper.IPV6_ENABLED, "IPv6 not enabled")
class TestIPv6Environment(TestCase):

    def setUp(self):
        self.server = DummyFTPServer((HOSTv6, 0),
                                     af=socket.AF_INET6,
                                     encoding=DEFAULT_ENCODING)
        self.server.start()
        self.client = ftplib.FTP(timeout=TIMEOUT, encoding=DEFAULT_ENCODING)
        self.client.connect(self.server.host, self.server.port)

    def tearDown(self):
        self.client.close()
        self.server.stop()
        # Explicitly clear the attribute to prevent dangling thread
        self.server = None
        asyncore.close_all(ignore_all=True)

    def test_af(self):
        self.assertEqual(self.client.af, socket.AF_INET6)

    def test_makeport(self):
        with self.client.makeport():
            self.assertEqual(self.server.handler_instance.last_received_cmd,
                                'eprt')

    def test_makepasv(self):
        host, port = self.client.makepasv()
        conn = socket.create_connection((host, port), timeout=TIMEOUT)
        conn.close()
        self.assertEqual(self.server.handler_instance.last_received_cmd, 'epsv')

    def test_transfer(self):
        def retr():
            def callback(data):
                received.append(data.decode(self.client.encoding))
            received = []
            self.client.retrbinary('retr', callback)
            self.assertEqual(len(''.join(received)), len(RETR_DATA))
            self.assertEqual(''.join(received), RETR_DATA)
        self.client.set_pasv(True)
        retr()
        self.client.set_pasv(False)
        retr()


@skipUnless(ssl, "SSL not available")
class TestTLS_FTPClassMixin(TestFTPClass):
    """Repeat TestFTPClass tests starting the TLS layer for both control
    and data connections first.
    """

    def setUp(self, encoding=DEFAULT_ENCODING):
        self.server = DummyTLS_FTPServer((HOST, 0), encoding=encoding)
        self.server.start()
        self.client = ftplib.FTP_TLS(timeout=TIMEOUT, encoding=encoding)
        self.client.connect(self.server.host, self.server.port)
        # enable TLS
        self.client.auth()
        self.client.prot_p()


@skipUnless(ssl, "SSL not available")
class TestTLS_FTPClass(TestCase):
    """Specific TLS_FTP class tests."""

    def setUp(self, encoding=DEFAULT_ENCODING):
        self.server = DummyTLS_FTPServer((HOST, 0), encoding=encoding)
        self.server.start()
        self.client = ftplib.FTP_TLS(timeout=TIMEOUT)
        self.client.connect(self.server.host, self.server.port)

    def tearDown(self):
        self.client.close()
        self.server.stop()
        # Explicitly clear the attribute to prevent dangling thread
        self.server = None
        asyncore.close_all(ignore_all=True)

    def test_control_connection(self):
        self.assertNotIsInstance(self.client.sock, ssl.SSLSocket)
        self.client.auth()
        self.assertIsInstance(self.client.sock, ssl.SSLSocket)

    def test_data_connection(self):
        # clear text
        with self.client.transfercmd('list') as sock:
            self.assertNotIsInstance(sock, ssl.SSLSocket)
            self.assertEqual(sock.recv(1024),
                             LIST_DATA.encode(self.client.encoding))
        self.assertEqual(self.client.voidresp(), "226 transfer complete")

        # secured, after PROT P
        self.client.prot_p()
        with self.client.transfercmd('list') as sock:
            self.assertIsInstance(sock, ssl.SSLSocket)
            # consume from SSL socket to finalize handshake and avoid
            # "SSLError [SSL] shutdown while in init"
            self.assertEqual(sock.recv(1024),
                             LIST_DATA.encode(self.client.encoding))
        self.assertEqual(self.client.voidresp(), "226 transfer complete")

        # PROT C is issued, the connection must be in cleartext again
        self.client.prot_c()
        with self.client.transfercmd('list') as sock:
            self.assertNotIsInstance(sock, ssl.SSLSocket)
            self.assertEqual(sock.recv(1024),
                             LIST_DATA.encode(self.client.encoding))
        self.assertEqual(self.client.voidresp(), "226 transfer complete")

    def test_login(self):
        # login() is supposed to implicitly secure the control connection
        self.assertNotIsInstance(self.client.sock, ssl.SSLSocket)
        self.client.login()
        self.assertIsInstance(self.client.sock, ssl.SSLSocket)
        # make sure that AUTH TLS doesn't get issued again
        self.client.login()

    def test_auth_issued_twice(self):
        self.client.auth()
        self.assertRaises(ValueError, self.client.auth)

    def test_context(self):
        self.client.quit()
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ctx.check_hostname = False
        ctx.verify_mode = ssl.CERT_NONE
        self.assertRaises(ValueError, ftplib.FTP_TLS, keyfile=CERTFILE,
                          context=ctx)
        self.assertRaises(ValueError, ftplib.FTP_TLS, certfile=CERTFILE,
                          context=ctx)
        self.assertRaises(ValueError, ftplib.FTP_TLS, certfile=CERTFILE,
                          keyfile=CERTFILE, context=ctx)

        self.client = ftplib.FTP_TLS(context=ctx, timeout=TIMEOUT)
        self.client.connect(self.server.host, self.server.port)
        self.assertNotIsInstance(self.client.sock, ssl.SSLSocket)
        self.client.auth()
        self.assertIs(self.client.sock.context, ctx)
        self.assertIsInstance(self.client.sock, ssl.SSLSocket)

        self.client.prot_p()
        with self.client.transfercmd('list') as sock:
            self.assertIs(sock.context, ctx)
            self.assertIsInstance(sock, ssl.SSLSocket)

    def test_ccc(self):
        self.assertRaises(ValueError, self.client.ccc)
        self.client.login(secure=True)
        self.assertIsInstance(self.client.sock, ssl.SSLSocket)
        self.client.ccc()
        self.assertRaises(ValueError, self.client.sock.unwrap)

    @skipUnless(False, "FIXME: bpo-32706")
    def test_check_hostname(self):
        self.client.quit()
        ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertEqual(ctx.verify_mode, ssl.CERT_REQUIRED)
        self.assertEqual(ctx.check_hostname, True)
        ctx.load_verify_locations(CAFILE)
        self.client = ftplib.FTP_TLS(context=ctx, timeout=TIMEOUT)

        # 127.0.0.1 doesn't match SAN
        self.client.connect(self.server.host, self.server.port)
        with self.assertRaises(ssl.CertificateError):
            self.client.auth()
        # exception quits connection

        self.client.connect(self.server.host, self.server.port)
        self.client.prot_p()
        with self.assertRaises(ssl.CertificateError):
            with self.client.transfercmd("list") as sock:
                pass
        self.client.quit()

        self.client.connect("localhost", self.server.port)
        self.client.auth()
        self.client.quit()

        self.client.connect("localhost", self.server.port)
        self.client.prot_p()
        with self.client.transfercmd("list") as sock:
            pass


class TestTimeouts(TestCase):

    def setUp(self):
        self.evt = threading.Event()
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(20)
        self.port = socket_helper.bind_port(self.sock)
        self.server_thread = threading.Thread(target=self.server)
        self.server_thread.daemon = True
        self.server_thread.start()
        # Wait for the server to be ready.
        self.evt.wait()
        self.evt.clear()
        self.old_port = ftplib.FTP.port
        ftplib.FTP.port = self.port

    def tearDown(self):
        ftplib.FTP.port = self.old_port
        self.server_thread.join()
        # Explicitly clear the attribute to prevent dangling thread
        self.server_thread = None

    def server(self):
        # This method sets the evt 3 times:
        #  1) when the connection is ready to be accepted.
        #  2) when it is safe for the caller to close the connection
        #  3) when we have closed the socket
        self.sock.listen()
        # (1) Signal the caller that we are ready to accept the connection.
        self.evt.set()
        try:
            conn, addr = self.sock.accept()
        except socket.timeout:
            pass
        else:
            conn.sendall(b"1 Hola mundo\n")
            conn.shutdown(socket.SHUT_WR)
            # (2) Signal the caller that it is safe to close the socket.
            self.evt.set()
            conn.close()
        finally:
            self.sock.close()

    def testTimeoutDefault(self):
        # default -- use global socket timeout
        self.assertIsNone(socket.getdefaulttimeout())
        socket.setdefaulttimeout(30)
        try:
            ftp = ftplib.FTP(HOST)
        finally:
            socket.setdefaulttimeout(None)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.close()

    def testTimeoutNone(self):
        # no timeout -- do not use global socket timeout
        self.assertIsNone(socket.getdefaulttimeout())
        socket.setdefaulttimeout(30)
        try:
            ftp = ftplib.FTP(HOST, timeout=None)
        finally:
            socket.setdefaulttimeout(None)
        self.assertIsNone(ftp.sock.gettimeout())
        self.evt.wait()
        ftp.close()

    def testTimeoutValue(self):
        # a value
        ftp = ftplib.FTP(HOST, timeout=30)
        self.assertEqual(ftp.sock.gettimeout(), 30)
        self.evt.wait()
        ftp.close()

        # bpo-39259
        with self.assertRaises(ValueError):
            ftplib.FTP(HOST, timeout=0)

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


class MiscTestCase(TestCase):
    def test__all__(self):
        not_exported = {
            'MSG_OOB', 'FTP_PORT', 'MAXLINE', 'CRLF', 'B_CRLF', 'Error',
            'parse150', 'parse227', 'parse229', 'parse257', 'print_line',
            'ftpcp', 'test'}
        support.check__all__(self, ftplib, not_exported=not_exported)


def test_main():
    tests = [TestFTPClass, TestTimeouts,
             TestIPv6Environment,
             TestTLS_FTPClassMixin, TestTLS_FTPClass,
             MiscTestCase]

    thread_info = threading_helper.threading_setup()
    try:
        support.run_unittest(*tests)
    finally:
        threading_helper.threading_cleanup(*thread_info)


if __name__ == '__main__':
    test_main()
