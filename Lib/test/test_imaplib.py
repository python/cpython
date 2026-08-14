from test import support
from test.support import socket_helper

from contextlib import contextmanager
from email.message import EmailMessage
import imaplib
import os.path
import socketserver
import time
import calendar
import threading
import re
import select
import socket

from test.support import verbose, run_with_tz, run_with_locale, cpython_only
from test.support import hashlib_helper, threading_helper
import unittest
from unittest import mock
from datetime import datetime, timezone, timedelta
try:
    import ssl
except ImportError:
    ssl = None

support.requires_working_socket(module=True)

CERTFILE = os.path.join(os.path.dirname(__file__) or os.curdir, "certdata", "keycert3.pem")
CAFILE = os.path.join(os.path.dirname(__file__) or os.curdir, "certdata", "pycacert.pem")

_quoted_string = re.compile(r'"(?:[^"\\]|\\.)*"')


def splitargs(line):
    # Split a command line into IMAP arguments: quoted strings, balanced
    # (possibly nested) parenthesized lists, and bare atoms.
    args = []
    i, n = 0, len(line)
    while i < n:
        if line[i] == ' ':
            i += 1
            continue
        start = i
        depth = 0
        while i < n and (depth or line[i] != ' '):
            c = line[i]
            if c == '"':
                i = _quoted_string.match(line, i).end()
                continue
            elif c == '(':
                depth += 1
            elif c == ')':
                depth -= 1
            i += 1
        args.append(line[start:i])
    return args


def parse_seq_number(s, maxmsg):
    if s == '*':
        return maxmsg
    return int(s)


def parse_sequence_set(arg, maxmsg):
    for s in arg.split(','):
        if ':' in s:
            lo, hi = s.split(':')
            lo = parse_seq_number(lo, maxmsg)
            hi = parse_seq_number(hi, maxmsg)
            yield from range(min(lo, hi), max(lo, hi) + 1)
        else:
            yield parse_seq_number(s, maxmsg)


def make_simple_handler(command, untagged_response=(),
                        completed=None):
    if completed is None:
        completed = f'{command} completed'
    def cmd(self, tag, args):
        for msg in untagged_response:
            self._send_textline(msg)
        self._send_tagged(tag, 'OK', completed)
    cmd.__name__ = 'cmd_' + command
    class Handler(SimpleIMAPHandler):
        pass
    Handler.__name__ = command.title() + 'Handler'
    setattr(Handler, cmd.__name__, cmd)
    Handler.__qualname__ = Handler.__name__
    cmd.__qualname__ = Handler.__qualname__ + '.' + cmd.__name__
    return Handler


def _read_literal(handler, marker):
    # Read one literal, a raw octet sequence, by its count from the marker
    # ('{N}', or '(~{N}' in UTF8 mode).
    size = int(re.search(r'\{(\d+)\}', marker).group(1))
    # The client must wait for the continuation, so nothing should be readable.
    if select.select([handler.connection], [], [], 0)[0]:
        raise AssertionError('client sent the literal before the '
                             'continuation request')
    handler._send_textline('+')
    return handler.rfile.read(size)


class TestImaplib(unittest.TestCase):

    def test_Internaldate2tuple(self):
        t0 = calendar.timegm((2000, 1, 1, 0, 0, 0, -1, -1, -1))
        tt = imaplib.Internaldate2tuple(
            b'25 (INTERNALDATE "01-Jan-2000 00:00:00 +0000")')
        self.assertEqual(time.mktime(tt), t0)
        tt = imaplib.Internaldate2tuple(
            b'25 (INTERNALDATE "01-Jan-2000 11:30:00 +1130")')
        self.assertEqual(time.mktime(tt), t0)
        tt = imaplib.Internaldate2tuple(
            b'25 (INTERNALDATE "31-Dec-1999 12:30:00 -1130")')
        self.assertEqual(time.mktime(tt), t0)

    @run_with_tz('MST+07MDT,M4.1.0,M10.5.0')
    def test_Internaldate2tuple_issue10941(self):
        self.assertNotEqual(imaplib.Internaldate2tuple(
            b'25 (INTERNALDATE "02-Apr-2000 02:30:00 +0000")'),
            imaplib.Internaldate2tuple(
                b'25 (INTERNALDATE "02-Apr-2000 03:30:00 +0000")'))

    def timevalues(self):
        return [2000000000, 2000000000.0, time.localtime(2000000000),
                (2033, 5, 18, 5, 33, 20, -1, -1, -1),
                (2033, 5, 18, 5, 33, 20, -1, -1, 1),
                datetime.fromtimestamp(2000000000,
                                       timezone(timedelta(0, 2 * 60 * 60))),
                '"18-May-2033 05:33:20 +0200"']

    @run_with_locale('LC_ALL', 'de_DE', 'fr_FR', '')
    # DST rules included to work around quirk where the Gnu C library may not
    # otherwise restore the previous time zone
    @run_with_tz('STD-1DST,M3.2.0,M11.1.0')
    def test_Time2Internaldate(self):
        expected = '"18-May-2033 05:33:20 +0200"'

        for t in self.timevalues():
            internal = imaplib.Time2Internaldate(t)
            self.assertEqual(internal, expected)

    @run_with_tz('STD-1DST,M3.2.0,M11.1.0')
    def test_Time2Internaldate_datetime_timetuple(self):
        date_time = datetime.fromtimestamp(2000000000).timetuple()
        self.assertIsNone(date_time.tm_gmtoff)
        self.assertEqual(
            imaplib.Time2Internaldate(date_time),
            '"18-May-2033 05:33:20 +0200"',
        )

    def test_that_Time2Internaldate_returns_a_result(self):
        # Without tzset, we can check only that it successfully
        # produces a result, not the correctness of the result itself,
        # since the result depends on the timezone the machine is in.
        for t in self.timevalues():
            imaplib.Time2Internaldate(t)

    @socket_helper.skip_if_tcp_blackhole
    def test_imap4_host_default_value(self):
        # Check whether the IMAP4_PORT is truly unavailable.
        with socket.socket() as s:
            try:
                s.connect(('', imaplib.IMAP4_PORT))
                self.skipTest(
                    "Cannot run the test with local IMAP server running.")
            except socket.error:
                pass

        # This is the exception that should be raised.
        expected_errnos = socket_helper.get_socket_conn_refused_errs()
        with self.assertRaises(OSError) as cm:
            imaplib.IMAP4()
        self.assertIn(cm.exception.errno, expected_errnos)

    def test_astring(self):
        m = imaplib.IMAP4.__new__(imaplib.IMAP4)
        m._encoding = 'ascii'
        # Plain atoms are left unquoted.
        self.assertEqual(m._astring('INBOX'), b'INBOX')
        self.assertEqual(m._astring(b'INBOX'), b'INBOX')
        # Names with protocol-sensitive characters are quoted.
        self.assertEqual(m._astring('New folder'), b'"New folder"')
        self.assertEqual(m._astring('a"b'), rb'"a\"b"')
        self.assertEqual(m._astring(r'a\b'), rb'"a\\b"')
        self.assertEqual(m._astring(''), b'""')
        self.assertEqual(m._astring('*'), b'"*"')
        # A well-formed quoted string is passed through unchanged.
        self.assertEqual(m._astring('"New folder"'), b'"New folder"')
        self.assertEqual(m._astring('""'), b'""')
        # Including a lenient (non-RFC) backslash escape, which the server
        # may accept.
        self.assertEqual(m._astring(r'"a\b"'), rb'"a\b"')
        # A string that only looks quoted but is not a single token is
        # quoted as data, closing the argument injection vector.
        self.assertEqual(m._astring('"a" SELECT evil "'),
                         rb'"\"a\" SELECT evil \""')
        self.assertEqual(m._astring('"'), rb'"\""')
        # Non-ASCII names are only allowed in a quoted string or a
        # literal, never in an atom (RFC 6855).
        m._encoding = 'utf-8'
        self.assertEqual(m._astring('Entwürfe'), '"Entwürfe"'.encode())
        self.assertEqual(m._astring(b'Entw\xc3\xbcrfe'), b'"Entw\xc3\xbcrfe"')

    def test_encode_criteria(self):
        m = imaplib.IMAP4.__new__(imaplib.IMAP4)
        enc = m._encode_criteria
        # No charset: criteria are returned unchanged.
        self.assertEqual(enc(None, ('TEXT', 'x')), ('TEXT', 'x'))
        # str criteria are encoded to the charset; ASCII is charset-independent.
        self.assertEqual(enc('UTF-8', ('TEXT', 'XXXXXX')), (b'TEXT', b'XXXXXX'))
        # Non-ASCII text is encoded to the declared charset, including charsets
        # other than ASCII, Latin-1 and UTF-8.
        self.assertEqual(enc('UTF-8', ('"café"',)), ('"café"'.encode('utf-8'),))
        self.assertEqual(enc('ISO-8859-1', ('"café"',)),
                         ('"café"'.encode('latin-1'),))
        self.assertEqual(enc('KOI8-U', ('"Київ"',)),
                         ('"Київ"'.encode('koi8-u'),))
        self.assertEqual(enc('SHIFT_JIS', ('"日本"',)),
                         ('"日本"'.encode('shift_jis'),))
        # bytes criteria are already encoded and pass through unchanged.
        self.assertEqual(enc('SHIFT_JIS', (b'"already"',)), (b'"already"',))
        # The charset name may itself be bytes.
        self.assertEqual(enc(b'UTF-8', ('"café"',)), ('"café"'.encode('utf-8'),))
        # A charset with no codec at all (not even via the iconv codec) cannot
        # encode str criteria; bytes criteria must be used with such
        # server-only charsets.
        self.assertRaises(LookupError, enc, 'no-such-charset', ('TEXT',))
        self.assertEqual(enc('no-such-charset', (b'TEXT',)), (b'TEXT',))

    def test_astring_idempotent(self):
        # Quoting an already quoted argument should not change it, so that
        # quoting twice gives the same result as quoting once.
        m = imaplib.IMAP4.__new__(imaplib.IMAP4)
        m._encoding = 'ascii'
        for arg in ['INBOX', 'New folder', 'a"b', 'a\\b', '', '*', '%',
                    '"New folder"', '""', '"a\\b"', '"a" SELECT evil "',
                    '"', 'a\tb', 'a\rb', '\x7f', '(a)', b'Entw\xc3\xbcrfe']:
            with self.subTest(arg=arg):
                once = m._astring(arg)
                self.assertEqual(m._astring(once), once)
                twice = m._list_mailbox(arg)
                self.assertEqual(m._list_mailbox(twice), twice)

    def test_list_mailbox(self):
        m = imaplib.IMAP4.__new__(imaplib.IMAP4)
        m._encoding = 'ascii'
        # Wildcards are not quoted in a list pattern.
        self.assertEqual(m._list_mailbox('*'), b'*')
        self.assertEqual(m._list_mailbox('%'), b'%')
        self.assertEqual(m._list_mailbox('foo/%'), b'foo/%')
        # But spaces still require quoting.
        self.assertEqual(m._list_mailbox('New folder'), b'"New folder"')
        self.assertEqual(m._list_mailbox('"New folder"'), b'"New folder"')
        # A non-ASCII pattern is encoded as modified UTF-7; the wildcards keep
        # their meaning (they are printable ASCII, passed through directly).
        self.assertEqual(m._list_mailbox('Entwürfe/%'), b'Entw&APw-rfe/%')
        # Under UTF8=ACCEPT the name is sent as UTF-8 in a quoted string.
        m._encoding = 'utf-8'
        self.assertEqual(m._list_mailbox('Entwürfe/%'),
                         '"Entwürfe/%"'.encode())

    def test_mailbox(self):
        m = imaplib.IMAP4.__new__(imaplib.IMAP4)
        m._encoding = 'ascii'
        # A plain atom is left unquoted; bytes are sent verbatim.
        self.assertEqual(m._mailbox('INBOX'), b'INBOX')
        self.assertEqual(m._mailbox(b'Entw&APw-rfe'), b'Entw&APw-rfe')
        # A non-ASCII name is encoded as modified UTF-7 (RFC 3501 5.1.3).
        self.assertEqual(m._mailbox('Entwürfe'), b'Entw&APw-rfe')
        # A str that is already valid modified UTF-7 is passed through, so a
        # name obtained from LIST (raw bytes, decoded as ASCII) round-trips.
        self.assertEqual(m._mailbox('Entw&APw-rfe'), b'Entw&APw-rfe')
        self.assertEqual(m._mailbox('"Entw&APw-rfe"'), b'"Entw&APw-rfe"')
        # A literal '&' (not a valid shift on its own) is encoded as '&-'.
        self.assertEqual(m._mailbox('A&B'), b'A&-B')
        # Names needing quoting are still quoted.
        self.assertEqual(m._mailbox('New folder'), b'"New folder"')
        self.assertEqual(m._mailbox(''), b'""')
        # Control characters are Base64-encoded, never sent as raw bytes.
        self.assertEqual(m._mailbox('a\x01b'), b'a&AAE-b')
        # Under UTF8=ACCEPT a non-ASCII name is sent as UTF-8, quoted.
        m._encoding = 'utf-8'
        self.assertEqual(m._mailbox('Entwürfe'), '"Entwürfe"'.encode())
        self.assertEqual(m._mailbox('Entw&APw-rfe'), b'Entw&APw-rfe')

    def test_sequence_set(self):
        m = imaplib.IMAP4.__new__(imaplib.IMAP4)
        # A scalar is passed through as a string.
        self.assertEqual(m._sequence_set(5), '5')
        self.assertEqual(m._sequence_set('1:3,7'), '1:3,7')
        # A sequence of numbers and ranges is formatted as a sequence set.
        self.assertEqual(m._sequence_set([1, 2, 5]), '1,2,5')
        self.assertEqual(m._sequence_set([1, (3, 5), (8, '*')]), '1,3:5,8:*')
        self.assertEqual(m._sequence_set([(5, None)]), '5:*')
        # A range is inclusive; a non-unit step falls back to explicit numbers.
        self.assertEqual(m._sequence_set([range(1, 4), 7]), '1:3,7')
        self.assertEqual(m._sequence_set([range(1, 10, 2)]), '1,3,5,7,9')
        # Message numbers must be integers: a string is not coerced (the
        # string form is the whole preformatted set), nor is a float.
        self.assertRaises(TypeError, m._sequence_set, ['7'])
        self.assertRaises(TypeError, m._sequence_set, [2.9])
        self.assertRaises(TypeError, m._sequence_set, [(1, 2.9)])

    def test_set_quote(self):
        m = imaplib.IMAP4.__new__(imaplib.IMAP4)
        # A string is parenthesized unless it already is.
        self.assertEqual(m._set_quote(r'\Seen'), r'(\Seen)')
        self.assertEqual(m._set_quote(r'(\Seen)'), r'(\Seen)')
        # A sequence of atoms is joined and parenthesized.
        self.assertEqual(m._set_quote([r'\Seen', r'\Answered']),
                         r'(\Seen \Answered)')
        self.assertEqual(m._set_quote(['MESSAGES', 'UNSEEN']),
                         '(MESSAGES UNSEEN)')

    def test_substitute(self):
        sub = imaplib._substitute
        # '?' quotes an astring; an atom-safe value is left unquoted.
        self.assertEqual(sub('FROM ?', ['me@host']), 'FROM me@host')
        self.assertEqual(sub('SUBJECT ?', ['hello world']),
                         'SUBJECT "hello world"')
        self.assertEqual(sub('SUBJECT ?', ['a"b']), r'SUBJECT "a\"b"')
        # An integer becomes a number, a list a parenthesized list.
        self.assertEqual(sub('LARGER ?', [1000]), 'LARGER 1000')
        self.assertEqual(sub('HEADER.FIELDS ?', [['DATE', 'FROM']]),
                         'HEADER.FIELDS (DATE FROM)')
        # '?f' emits flags verbatim, never quoted.
        self.assertEqual(sub('?f', [r'\Seen']), r'\Seen')
        self.assertEqual(sub('?f', [[r'\Seen', r'\Answered']]),
                         r'(\Seen \Answered)')
        # '?s' formats a message sequence set.
        self.assertEqual(sub('?s', [[1, (3, 5), (8, '*')]]), '1,3:5,8:*')
        # '??' is a literal '?'.
        self.assertEqual(sub('a?? b', []), 'a? b')

    def test_substitute_errors(self):
        sub = imaplib._substitute
        self.assertRaises(TypeError, sub, '? ?', ['x'])    # too few parameters
        self.assertRaises(TypeError, sub, '?', ['x', 'y']) # too many parameters
        self.assertRaises(ValueError, sub, '?f', ['a b'])  # not a valid flag
        self.assertRaises(ValueError, sub, '?', ['a\r\nb'])  # CR/LF not inline
        self.assertRaises(TypeError, sub, '?', [True])     # bool is not a string
        self.assertRaises(TypeError, sub, '?', [1.5])      # float is not a string
        self.assertRaises(TypeError, sub, '?s', [['a']])   # not a message number
        self.assertRaises(TypeError, sub, '?s', [[1.5]])   # not a message number
        self.assertRaises(TypeError, sub, '?s', [[(1, 'a')]])  # not a message number
        self.assertRaises(ValueError, sub, '?s', [[(1,)]])       # not a range pair
        self.assertRaises(ValueError, sub, '?s', [[(1, 2, 3)]])  # not a range pair


if ssl:
    class SecureTCPServer(socketserver.TCPServer):

        def get_request(self):
            newsocket, fromaddr = self.socket.accept()
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
            context.load_cert_chain(CERTFILE)
            connstream = context.wrap_socket(newsocket, server_side=True)
            return connstream, fromaddr

    IMAP4_SSL = imaplib.IMAP4_SSL

else:

    class SecureTCPServer:
        pass

    IMAP4_SSL = None


class SimpleIMAPHandler(socketserver.StreamRequestHandler):
    timeout = support.LOOPBACK_TIMEOUT
    continuation = None
    capabilities = ''

    def setup(self):
        super().setup()
        self.server.is_selected = None
        self.server.logged = None

    def _send(self, message):
        if verbose:
            print("SENT: %r" % message.strip())
        self.wfile.write(message)

    def _send_line(self, message):
        self._send(message + b'\r\n')

    def _send_textline(self, message):
        self._send_line(message.encode('ASCII'))

    def _send_tagged(self, tag, code, message):
        self._send_textline(' '.join((tag, code, message)))

    welcome = '* OK IMAP4rev1'

    def handle(self):
        # Send a welcome message.
        self._send_textline(self.welcome)
        while 1:
            # Gather up input until we receive a line terminator or we timeout.
            # Accumulate read(1) because it's simpler to handle the differences
            # between naked sockets and SSL sockets.
            line = b''
            while 1:
                try:
                    part = self.rfile.read(1)
                    if part == b'':
                        # Naked sockets return empty strings..
                        return
                    line += part
                except OSError:
                    # ..but SSLSockets raise exceptions.
                    return
                if line.endswith(b'\r\n'):
                    break

            if verbose:
                print('GOT: %r' % line.strip())
            if self.continuation:
                try:
                    self.continuation.send(line)
                except StopIteration:
                    self.continuation = None
                continue
            self.server.line = line
            # surrogateescape so a criterion encoded in a non-UTF-8 charset
            # does not crash the handler; tests inspect server.line for bytes.
            splitline = splitargs(line.decode('utf-8', 'surrogateescape')
                                  .removesuffix('\r\n'))
            tag = splitline[0]
            cmd = splitline[1]
            args = splitline[2:]
            self.server.args = args

            if hasattr(self, 'cmd_' + cmd):
                continuation = getattr(self, 'cmd_' + cmd)(tag, args)
                if continuation:
                    self.continuation = continuation
                    next(continuation)
            else:
                self._send_tagged(tag, 'BAD', cmd + ' unknown')

    def cmd_CAPABILITY(self, tag, args):
        caps = ('IMAP4rev1 ' + self.capabilities
                if self.capabilities
                else 'IMAP4rev1')
        self._send_textline('* CAPABILITY ' + caps)
        self._send_tagged(tag, 'OK', 'CAPABILITY completed')

    def cmd_LOGOUT(self, tag, args):
        self.server.logged = None
        self._send_textline('* BYE IMAP4ref1 Server logging out')
        self._send_tagged(tag, 'OK', 'LOGOUT completed')

    def cmd_LOGIN(self, tag, args):
        self.server.logged = args
        self._send_tagged(tag, 'OK', 'LOGIN completed')

    def cmd_SELECT(self, tag, args):
        self.server.is_selected = args
        self._send_line(b'* 2 EXISTS')
        self._send_tagged(tag, 'OK', '[READ-WRITE] SELECT completed.')

    def cmd_EXAMINE(self, tag, args):
        self.server.is_selected = args
        self._send_line(b'* 2 EXISTS')
        self._send_tagged(tag, 'OK', '[READ-ONLY] EXAMINE completed.')

    def cmd_UNSELECT(self, tag, args):
        if self.server.is_selected is not None:
            self.server.is_selected = None
            self._send_tagged(tag, 'OK', 'Returned to authenticated state. (Success)')
        else:
            self._send_tagged(tag, 'BAD', 'No mailbox selected')


class IdleCmdDenyHandler(SimpleIMAPHandler):
    capabilities = 'IDLE'
    def cmd_IDLE(self, tag, args):
        self._send_tagged(tag, 'NO', 'IDLE is not allowed at this time')


class IdleCmdHandler(SimpleIMAPHandler):
    capabilities = 'IDLE'
    def cmd_IDLE(self, tag, args):
        # pre-idle-continuation response
        self._send_line(b'* 0 EXISTS')
        self._send_textline('+ idling')
        # simple response
        self._send_line(b'* 2 EXISTS')
        # complex response: fragmented data due to literal string
        self._send_line(b'* 1 FETCH (BODY[HEADER.FIELDS (DATE)] {41}')
        self._send(b'Date: Fri, 06 Dec 2024 06:00:00 +0000\r\n\r\n')
        self._send_line(b')')
        # simple response following a fragmented one
        self._send_line(b'* 3 EXISTS')
        # response arriving later
        time.sleep(1)
        self._send_line(b'* 1 RECENT')
        r = yield
        if r == b'DONE\r\n':
            self._send_line(b'* 9 RECENT')
            self._send_tagged(tag, 'OK', 'Idle completed')
        else:
            self._send_tagged(tag, 'BAD', 'Expected DONE')


class IdleCmdDelayedPacketHandler(SimpleIMAPHandler):
    capabilities = 'IDLE'
    def cmd_IDLE(self, tag, args):
        self._send_textline('+ idling')
        # response line spanning multiple packets, the last one delayed
        self._send(b'* 1 EX')
        time.sleep(0.2)
        self._send(b'IS')
        time.sleep(1)
        self._send(b'TS\r\n')
        r = yield
        if r == b'DONE\r\n':
            self._send_tagged(tag, 'OK', 'Idle completed')
        else:
            self._send_tagged(tag, 'BAD', 'Expected DONE')


class AuthHandler_CRAM_MD5(SimpleIMAPHandler):
    capabilities = 'LOGINDISABLED AUTH=CRAM-MD5'
    def cmd_AUTHENTICATE(self, tag, args):
        self._send_textline('+ PDE4OTYuNjk3MTcwOTUyQHBvc3RvZmZpY2Uucm'
                            'VzdG9uLm1jaS5uZXQ=')
        r = yield
        if (r == b'dGltIGYxY2E2YmU0NjRiOWVmYT'
                 b'FjY2E2ZmZkNmNmMmQ5ZjMy\r\n'):
            self._send_tagged(tag, 'OK', 'CRAM-MD5 successful')
        else:
            self._send_tagged(tag, 'NO', 'No access')


class AuthHandler_PLAIN(SimpleIMAPHandler):
    capabilities = 'LOGINDISABLED AUTH=PLAIN'
    def cmd_AUTHENTICATE(self, tag, args):
        self.server.auth_args = args
        self._send_textline('+')
        self.server.response = yield
        self._send_tagged(tag, 'OK', 'Logged in.')


class NewIMAPTestsMixin:
    client = None

    def _setup(self, imap_handler, connect=True):
        """
        Sets up imap_handler for tests. imap_handler should inherit from either:
        - SimpleIMAPHandler - for testing IMAP commands,
        - socketserver.StreamRequestHandler - if raw access to stream is needed.
        Returns (client, server).
        """
        class TestTCPServer(self.server_class):
            def handle_error(self, request, client_address):
                """
                End request and raise the error if one occurs.
                """
                self.close_request(request)
                self.server_close()
                raise

        self.addCleanup(self._cleanup)
        self.server = self.server_class((socket_helper.HOST, 0), imap_handler)
        self.thread = threading.Thread(
            name=self._testMethodName+'-server',
            target=self.server.serve_forever,
            # Short poll interval to make the test finish quickly.
            # Time between requests is short enough that we won't wake
            # up spuriously too many times.
            kwargs={'poll_interval': 0.01})
        self.thread.daemon = True  # In case this function raises.
        self.thread.start()

        if connect:
            self.client = self.imap_class(*self.server.server_address)

        return self.client, self.server

    def _cleanup(self):
        """
        Cleans up the test server. This method should not be called manually,
        it is added to the cleanup queue in the _setup method already.
        """
        # if logout was called already we'd raise an exception trying to
        # shutdown the client once again
        if self.client is not None and self.client.state != 'LOGOUT':
            self.client.shutdown()
        # cleanup the server
        self.server.shutdown()
        self.server.server_close()
        threading_helper.join_thread(self.thread)
        # Explicitly clear the attribute to prevent dangling thread
        self.thread = None

    def test_EOF_without_complete_welcome_message(self):
        # http://bugs.python.org/issue5949
        class EOFHandler(socketserver.StreamRequestHandler):
            def handle(self):
                self.wfile.write(b'* OK')
        _, server = self._setup(EOFHandler, connect=False)
        self.assertRaises(imaplib.IMAP4.abort, self.imap_class,
                          *server.server_address)

    def test_invalid_greeting(self):
        # An invalid greeting, e.g. from a POP3 server on the IMAP port,
        # must not fail with "error: None" but report the server's line
        # (gh-108280).
        class Pop3Handler(socketserver.StreamRequestHandler):
            def handle(self):
                self.wfile.write(b'+OK POP3 server ready\r\n')
        _, server = self._setup(Pop3Handler, connect=False)
        with self.assertRaisesRegex(imaplib.IMAP4.error,
                                    r'invalid greeting: \+OK POP3 server ready'):
            self.imap_class(*server.server_address)

    def test_invalid_greeting_untagged(self):
        # An untagged greeting that is neither OK nor PREAUTH (e.g. BYE)
        # is reported as is (gh-108280).
        class ByeHandler(socketserver.StreamRequestHandler):
            def handle(self):
                self.wfile.write(b'* BYE Server unavailable\r\n')
        _, server = self._setup(ByeHandler, connect=False)
        with self.assertRaisesRegex(imaplib.IMAP4.error,
                                    r'invalid greeting: \* BYE Server unavailable'):
            self.imap_class(*server.server_address)

    def test_invalid_greeting_bare_continuation(self):
        # A bare continuation greeting is still reported (gh-108280).
        class BareHandler(socketserver.StreamRequestHandler):
            def handle(self):
                self.wfile.write(b'+\r\n')
        _, server = self._setup(BareHandler, connect=False)
        with self.assertRaisesRegex(imaplib.IMAP4.error,
                                    r'invalid greeting: \+'):
            self.imap_class(*server.server_address)

    def test_line_termination(self):
        class BadNewlineHandler(SimpleIMAPHandler):
            def cmd_CAPABILITY(self, tag, args):
                self._send(b'* CAPABILITY IMAP4rev1 AUTH\n')
                self._send_tagged(tag, 'OK', 'CAPABILITY completed')
        _, server = self._setup(BadNewlineHandler, connect=False)
        self.assertRaises(imaplib.IMAP4.abort, self.imap_class,
                          *server.server_address)

    def test_enable_raises_error_if_not_AUTH(self):
        class EnableHandler(SimpleIMAPHandler):
            capabilities = 'AUTH ENABLE UTF8=ACCEPT'
        client, _ = self._setup(EnableHandler)
        self.assertFalse(client.utf8_enabled)
        with self.assertRaisesRegex(imaplib.IMAP4.error, 'ENABLE.*NONAUTH'):
            client.enable('foo')
        self.assertFalse(client.utf8_enabled)

    def test_enable_raises_error_if_no_capability(self):
        client, _ = self._setup(SimpleIMAPHandler)
        with self.assertRaisesRegex(imaplib.IMAP4.error,
                'does not support ENABLE'):
            client.enable('foo')

    def test_enable_UTF8_raises_error_if_not_supported(self):
        client, _ = self._setup(SimpleIMAPHandler)
        typ, data = client.login('user', 'pass')
        self.assertEqual(typ, 'OK')
        with self.assertRaisesRegex(imaplib.IMAP4.error,
                'does not support ENABLE'):
            client.enable('UTF8=ACCEPT')

    def test_enable_UTF8_True_append(self):
        class UTF8AppendServer(SimpleIMAPHandler):
            capabilities = 'ENABLE UTF8=ACCEPT'
            def cmd_ENABLE(self, tag, args):
                self._send_tagged(tag, 'OK', 'ENABLE successful')
            def cmd_AUTHENTICATE(self, tag, args):
                self._send_textline('+')
                self.server.response = yield
                self._send_tagged(tag, 'OK', 'FAKEAUTH successful')
            def cmd_APPEND(self, tag, args):
                self.server.response = args
                self.server.response.append(_read_literal(self, args[-1]))
                literal = yield
                self.server.response.append(literal)
                self._send_tagged(tag, 'OK', 'okay')
        client, server = self._setup(UTF8AppendServer)
        self.assertEqual(client._encoding, 'ascii')
        code, _ = client.authenticate('MYAUTH', lambda x: b'fake')
        self.assertEqual(code, 'OK')
        self.assertEqual(server.response, b'ZmFrZQ==\r\n')  # b64 encoded 'fake'
        code, _ = client.enable('UTF8=ACCEPT')
        self.assertEqual(code, 'OK')
        self.assertEqual(client._encoding, 'utf-8')
        self.assertEqual(server.args, ['UTF8=ACCEPT'])
        msg_string = 'Subject: üñí©öðé'
        typ, data = client.append(
            None, None, None, (msg_string + '\n').encode('utf-8'))
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.response,
            ['INBOX', 'UTF8',
             '(~{25}', ('%s\r\n' % msg_string).encode('utf-8'),
             b')\r\n' ])

    def test_search_disallows_charset_in_utf8_mode(self):
        class UTF8Server(SimpleIMAPHandler):
            capabilities = 'AUTH ENABLE UTF8=ACCEPT'
            def cmd_ENABLE(self, tag, args):
                self._send_tagged(tag, 'OK', 'ENABLE successful')
            def cmd_AUTHENTICATE(self, tag, args):
                self._send_textline('+')
                self.server.response = yield
                self._send_tagged(tag, 'OK', 'FAKEAUTH successful')
        client, _ = self._setup(UTF8Server)
        typ, _ = client.authenticate('MYAUTH', lambda x: b'fake')
        self.assertEqual(typ, 'OK')
        typ, _ = client.enable('UTF8=ACCEPT')
        self.assertEqual(typ, 'OK')
        self.assertTrue(client.utf8_enabled)
        with self.assertRaisesRegex(imaplib.IMAP4.error, 'charset.*UTF8'):
            client.search('foo', 'bar')

    def test_utf8_mailbox_name(self):
        class UTF8Server(SimpleIMAPHandler):
            capabilities = 'AUTH ENABLE UTF8=ACCEPT'
            def cmd_ENABLE(self, tag, args):
                self._send_tagged(tag, 'OK', 'ENABLE successful')
            def cmd_AUTHENTICATE(self, tag, args):
                self._send_textline('+')
                self.server.response = yield
                self._send_tagged(tag, 'OK', 'FAKEAUTH successful')
        client, server = self._setup(UTF8Server)
        typ, _ = client.authenticate('MYAUTH', lambda x: b'fake')
        self.assertEqual(typ, 'OK')
        typ, _ = client.enable('UTF8=ACCEPT')
        self.assertEqual(typ, 'OK')
        # A non-ASCII mailbox name is only allowed in a quoted string
        # or a literal, never in an atom (RFC 6855).
        typ, _ = client.select('Entwürfe')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.is_selected, ['"Entwürfe"'])

    def test_bad_auth_name(self):
        class MyServer(SimpleIMAPHandler):
            def cmd_AUTHENTICATE(self, tag, args):
                self._send_tagged(tag, 'NO',
                    'unrecognized authentication type {}'.format(args[0]))
        client, _ = self._setup(MyServer)
        with self.assertRaisesRegex(imaplib.IMAP4.error,
                'unrecognized authentication type METHOD'):
            client.authenticate('METHOD', lambda: 1)

    def test_invalid_authentication(self):
        class MyServer(SimpleIMAPHandler):
            def cmd_AUTHENTICATE(self, tag, args):
                self._send_textline('+')
                self.response = yield
                self._send_tagged(tag, 'NO', '[AUTHENTICATIONFAILED] invalid')
        client, _ = self._setup(MyServer)
        with self.assertRaisesRegex(imaplib.IMAP4.error,
                r'\[AUTHENTICATIONFAILED\] invalid'):
            client.authenticate('MYAUTH', lambda x: b'fake')

    def test_invalid_login(self):
        class MyServer(SimpleIMAPHandler):
            def cmd_LOGIN(self, tag, args):
                self.server.logged = args[0]
                self._send_tagged(tag, 'NO', '[LOGIN] failed')
        client, _ = self._setup(MyServer)
        with self.assertRaisesRegex(imaplib.IMAP4.error,
                r'\[LOGIN\] failed'):
            client.login('user', 'wrongpass')

    def test_valid_authentication_bytes(self):
        class MyServer(SimpleIMAPHandler):
            def cmd_AUTHENTICATE(self, tag, args):
                self._send_textline('+')
                self.server.response = yield
                self._send_tagged(tag, 'OK', 'FAKEAUTH successful')
        client, server = self._setup(MyServer)
        code, _ = client.authenticate('MYAUTH', lambda x: b'fake')
        self.assertEqual(code, 'OK')
        self.assertEqual(server.response, b'ZmFrZQ==\r\n')  # b64 encoded 'fake'

    def test_valid_authentication_plain_text(self):
        class MyServer(SimpleIMAPHandler):
            def cmd_AUTHENTICATE(self, tag, args):
                self._send_textline('+')
                self.server.response = yield
                self._send_tagged(tag, 'OK', 'FAKEAUTH successful')
        client, server = self._setup(MyServer)
        code, _ = client.authenticate('MYAUTH', lambda x: 'fake')
        self.assertEqual(code, 'OK')
        self.assertEqual(server.response, b'ZmFrZQ==\r\n')  # b64 encoded 'fake'

    @hashlib_helper.requires_hashdigest('md5', openssl=True)
    def test_login_cram_md5_bytes(self):
        client, _ = self._setup(AuthHandler_CRAM_MD5)
        self.assertIn('AUTH=CRAM-MD5', client.capabilities)
        ret, _ = client.login_cram_md5("tim", b"tanstaaftanstaaf")
        self.assertEqual(ret, "OK")

    @hashlib_helper.requires_hashdigest('md5', openssl=True)
    def test_login_cram_md5_plain_text(self):
        client, _ = self._setup(AuthHandler_CRAM_MD5)
        self.assertIn('AUTH=CRAM-MD5', client.capabilities)
        ret, _ = client.login_cram_md5("tim", "tanstaaftanstaaf")
        self.assertEqual(ret, "OK")

    @hashlib_helper.block_algorithm("md5")
    def test_login_cram_md5_blocked(self):
        client, _ = self._setup(AuthHandler_CRAM_MD5)
        self.assertIn('AUTH=CRAM-MD5', client.capabilities)
        msg = re.escape("CRAM-MD5 authentication is not supported")
        with self.assertRaisesRegex(imaplib.IMAP4.error, msg):
            client.login_cram_md5("tim", b"tanstaaftanstaaf")

    def test_login_plain_ascii(self):
        client, server = self._setup(AuthHandler_PLAIN)
        self.assertIn('AUTH=PLAIN', client.capabilities)
        ret, _ = client.login_plain("prem", "pass")
        self.assertEqual(ret, "OK")
        self.assertEqual(server.auth_args, ['PLAIN'])
        self.assertEqual(server.response, b'AHByZW0AcGFzcw==\r\n')

    def test_login_plain_utf8(self):
        client, server = self._setup(AuthHandler_PLAIN)
        self.assertIn('AUTH=PLAIN', client.capabilities)
        ret, _ = client.login_plain("pręm", "żółć")
        self.assertEqual(ret, "OK")
        self.assertEqual(server.response, b'AHByxJltAMW8w7PFgsSH\r\n')

    def test_login_plain_bytes(self):
        # bytes are accepted and sent verbatim (not re-encoded).
        client, server = self._setup(AuthHandler_PLAIN)
        ret, _ = client.login_plain(b'pr\xc4\x99m', b'\xc5\xbc\xc3\xb3\xc5\x82\xc4\x87')
        self.assertEqual(ret, "OK")
        self.assertEqual(server.response, b'AHByxJltAMW8w7PFgsSH\r\n')

    def test_login_plain_nul(self):
        client, _ = self._setup(AuthHandler_PLAIN)
        with self.assertRaises(ValueError):
            client.login_plain('user\0', 'pass')
        with self.assertRaises(ValueError):
            client.login_plain('user', b'pa\0ss')

    def test_aborted_authentication(self):
        class MyServer(SimpleIMAPHandler):
            def cmd_AUTHENTICATE(self, tag, args):
                self._send_textline('+')
                self.response = yield
                if self.response == b'*\r\n':
                    self._send_tagged(
                        tag,
                        'NO',
                        '[AUTHENTICATIONFAILED] aborted')
                else:
                    self._send_tagged(tag, 'OK', 'MYAUTH successful')
        client, _ = self._setup(MyServer)
        with self.assertRaisesRegex(imaplib.IMAP4.error,
                r'\[AUTHENTICATIONFAILED\] aborted'):
            client.authenticate('MYAUTH', lambda x: None)

    @mock.patch('imaplib._MAXLINE', 10)
    def test_linetoolong(self):
        class TooLongHandler(SimpleIMAPHandler):
            def handle(self):
                # send response line longer than the limit set in the next line
                self.wfile.write(b'* OK ' + 11 * b'x' + b'\r\n')
        _, server = self._setup(TooLongHandler, connect=False)
        with self.assertRaisesRegex(imaplib.IMAP4.error,
                'got more than 10 bytes'):
            self.imap_class(*server.server_address)

    def test_simple_with_statement(self):
        _, server = self._setup(SimpleIMAPHandler, connect=False)
        with self.imap_class(*server.server_address):
            pass

    def test_imaplib_timeout_test(self):
        _, server = self._setup(SimpleIMAPHandler, connect=False)
        with self.imap_class(*server.server_address, timeout=None) as client:
            self.assertEqual(client.sock.timeout, None)
        with self.imap_class(*server.server_address, timeout=support.LOOPBACK_TIMEOUT) as client:
            self.assertEqual(client.sock.timeout, support.LOOPBACK_TIMEOUT)
        with self.assertRaises(ValueError):
            self.imap_class(*server.server_address, timeout=0)

    def test_imaplib_timeout_functionality_test(self):
        class TimeoutHandler(SimpleIMAPHandler):
            def handle(self):
                time.sleep(1)
                SimpleIMAPHandler.handle(self)

        _, server = self._setup(TimeoutHandler)
        addr = server.server_address[1]
        with self.assertRaises(TimeoutError):
            client = self.imap_class("localhost", addr, timeout=0.001)

    def test_with_statement(self):
        _, server = self._setup(SimpleIMAPHandler, connect=False)
        with self.imap_class(*server.server_address) as imap:
            imap.login('user', 'pass')
            self.assertEqual(server.logged, ['user', '"pass"'])
        self.assertIsNone(server.logged)

    def test_with_statement_logout(self):
        # It is legal to log out explicitly inside the with block
        _, server = self._setup(SimpleIMAPHandler, connect=False)
        with self.imap_class(*server.server_address) as imap:
            imap.login('user', 'pass')
            self.assertEqual(server.logged, ['user', '"pass"'])
            imap.logout()
            self.assertIsNone(server.logged)
        self.assertIsNone(server.logged)

    # command tests

    def test_idle_capability(self):
        client, _ = self._setup(SimpleIMAPHandler)
        with self.assertRaisesRegex(imaplib.IMAP4.error,
                'does not support IMAP4 IDLE'):
            with client.idle():
                pass

    def test_idle_denied(self):
        client, _ = self._setup(IdleCmdDenyHandler)
        client.login('user', 'pass')
        with self.assertRaises(imaplib.IMAP4.error):
            with client.idle() as idler:
                pass

    def test_idle_iter(self):
        client, _ = self._setup(IdleCmdHandler)
        client.login('user', 'pass')
        with client.idle() as idler:
            # iteration should include response between 'IDLE' & '+ idling'
            response = next(idler)
            self.assertEqual(response, ('EXISTS', [b'0']))
            # iteration should produce responses
            response = next(idler)
            self.assertEqual(response, ('EXISTS', [b'2']))
            # fragmented response (with literal string) should arrive whole
            expected_fetch_data = [
                (b'1 (BODY[HEADER.FIELDS (DATE)] {41}',
                    b'Date: Fri, 06 Dec 2024 06:00:00 +0000\r\n\r\n'),
                b')']
            typ, data = next(idler)
            self.assertEqual(typ, 'FETCH')
            self.assertEqual(data, expected_fetch_data)
            # response after a fragmented one should arrive separately
            response = next(idler)
            self.assertEqual(response, ('EXISTS', [b'3']))
        # iteration should have consumed untagged responses
        _, data = client.response('EXISTS')
        self.assertEqual(data, [None])
        # responses not iterated should be available after idle
        _, data = client.response('RECENT')
        self.assertEqual(data[0], b'1')
        # responses received after 'DONE' should be available after idle
        self.assertEqual(data[1], b'9')

    def test_idle_burst(self):
        client, _ = self._setup(IdleCmdHandler)
        client.login('user', 'pass')
        # burst() should yield immediately available responses
        with client.idle() as idler:
            batch = list(idler.burst())
            self.assertEqual(len(batch), 4)
        # burst() should not have consumed later responses
        _, data = client.response('RECENT')
        self.assertEqual(data, [b'1', b'9'])

    def test_idle_delayed_packet(self):
        client, _ = self._setup(IdleCmdDelayedPacketHandler)
        client.login('user', 'pass')
        # If our readline() implementation fails to preserve line fragments
        # when idle timeouts trigger, a response spanning delayed packets
        # can be corrupted, leaving the protocol stream in a bad state.
        try:
            with client.idle(0.5) as idler:
                self.assertRaises(StopIteration, next, idler)
        except client.abort as err:
            self.fail('multi-packet response was corrupted by idle timeout')

    def test_login(self):
        client, server = self._setup(SimpleIMAPHandler)
        typ, data = client.login('user', 'pass')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data[0], b'LOGIN completed')
        self.assertEqual(client.state, 'AUTH')
        # The user name is quoted only when necessary, but the password
        # is always quoted.
        self.assertEqual(server.logged, ['user', '"pass"'])
        self.assertRaises(imaplib.IMAP4.error, client.login, 'user', 'pass')

    def test_login_quoted(self):
        client, server = self._setup(SimpleIMAPHandler)
        typ, data = client.login('us*r', 'p%ss')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data[0], b'LOGIN completed')
        self.assertEqual(client.state, 'AUTH')
        self.assertEqual(server.logged, ['"us*r"', '"p%ss"'])

    def test_login_quoted2(self):
        # An already quoted user name is passed through unchanged, rather
        # than being quoted a second time; the password is always quoted.
        client, server = self._setup(SimpleIMAPHandler)
        typ, data = client.login('"user"', '"pass"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data[0], b'LOGIN completed')
        self.assertEqual(client.state, 'AUTH')
        self.assertEqual(server.logged, ['"user"', r'"\"pass\""'])

    def test_append_translate_line_endings(self):
        # By default line endings are normalized to CRLF; False sends the
        # literal exactly (gh-49680).
        class AppendHandler(SimpleIMAPHandler):
            def cmd_APPEND(self, tag, args):
                self.server.response = _read_literal(self, args[-1])
                yield  # read the trailer line
                self._send_tagged(tag, 'OK', 'APPEND completed')
        client, server = self._setup(AppendHandler)
        client.login('user', 'pass')
        message = b'a\rb\nc\r\nd'
        client.append('INBOX', None, None, message)
        self.assertEqual(server.response, b'a\r\nb\r\nc\r\nd')
        client.append('INBOX', None, None, message,
                      translate_line_endings=False)
        self.assertEqual(server.response, message)

        # An email message uses bare LF by default; False sends it verbatim.
        message = EmailMessage()
        message['Subject'] = 'line endings'
        message.set_content('body line\n')
        message = message.as_bytes()
        self.assertNotIn(b'\r\n', message)
        client.append('INBOX', None, None, message,
                      translate_line_endings=False)
        self.assertEqual(server.response, message)

        # The mailbox is quoted and the flags are wrapped in parentheses
        # when necessary.
        client.append('New folder', r'\Seen', None, b'data')
        self.assertEqual(server.args, ['"New folder"', r'(\Seen)', '{4}'])

    def test_login_capabilities(self):
        # A server may advertise new capabilities after login (as an
        # untagged CAPABILITY response); imaplib must refresh its cached
        # capability list (gh-63121, gh-103451).
        class CapabilityLoginHandler(SimpleIMAPHandler):
            def cmd_LOGIN(self, tag, args):
                self.server.logged = args[0]
                self._send_textline('* CAPABILITY IMAP4rev1 ENABLE UTF8=ACCEPT')
                self._send_tagged(tag, 'OK', 'LOGIN completed')
            def cmd_ENABLE(self, tag, args):
                self._send_tagged(tag, 'OK', 'ENABLE completed')

        client, _ = self._setup(CapabilityLoginHandler)
        self.assertNotIn('ENABLE', client.capabilities)
        client.login('user', 'pass')
        self.assertIn('ENABLE', client.capabilities)
        self.assertIn('UTF8=ACCEPT', client.capabilities)
        typ, _ = client.enable('UTF8=ACCEPT')
        self.assertEqual(typ, 'OK')

    def test_authenticate_capabilities(self):
        # Capabilities are also refreshed after AUTHENTICATE, here from a
        # CAPABILITY response code in the tagged OK response.
        class CapabilityAuthHandler(SimpleIMAPHandler):
            def cmd_AUTHENTICATE(self, tag, args):
                self._send_textline('+')
                self.server.response = yield
                self._send_tagged(
                    tag, 'OK',
                    '[CAPABILITY IMAP4rev1 ENABLE] AUTHENTICATE completed')

        client, _ = self._setup(CapabilityAuthHandler)
        self.assertNotIn('ENABLE', client.capabilities)
        client.authenticate('MYAUTH', lambda x: b'fake')
        self.assertIn('ENABLE', client.capabilities)

    def test_greeting_capabilities(self):
        # Capabilities advertised in the greeting are used directly,
        # without sending a separate CAPABILITY command.
        class GreetingHandler(SimpleIMAPHandler):
            welcome = '* OK [CAPABILITY IMAP4rev1 ENABLE] Server ready'
            def cmd_CAPABILITY(self, tag, args):
                self.server.capability_queried = True
                super().cmd_CAPABILITY(tag, args)

        client, server = self._setup(GreetingHandler)
        self.assertEqual(client.capabilities, ('IMAP4REV1', 'ENABLE'))
        self.assertFalse(getattr(server, 'capability_queried', False))

    def test_login_requery_capabilities(self):
        # If the server does not advertise capabilities after login,
        # imaplib re-queries them (as it does after STARTTLS), so a
        # capability that becomes available only after authentication is
        # still recognized (gh-63121).
        class RequeryHandler(SimpleIMAPHandler):
            def cmd_CAPABILITY(self, tag, args):
                caps = 'IMAP4rev1 ENABLE' if self.server.logged else 'IMAP4rev1'
                self._send_textline('* CAPABILITY ' + caps)
                self._send_tagged(tag, 'OK', 'CAPABILITY completed')

        client, _ = self._setup(RequeryHandler)
        self.assertNotIn('ENABLE', client.capabilities)
        client.login('user', 'pass')
        self.assertIn('ENABLE', client.capabilities)

    def test_readonly_error_reports_mailbox(self):
        # The read-only error reports the mailbox via repr(), which also
        # avoids BytesWarning for a bytes mailbox under -bb.
        class ReadOnlyHandler(SimpleIMAPHandler):
            def cmd_SELECT(self, tag, args):
                self._send_line(b'* 2 EXISTS')
                self._send_tagged(tag, 'OK', '[READ-ONLY] SELECT completed.')
        client, _ = self._setup(ReadOnlyHandler)
        client.login('user', 'pass')
        for mailbox, expected in [('INBOX', "'INBOX'"), (b'INBOX', r"b'INBOX'")]:
            with self.subTest(mailbox=mailbox):
                with self.assertRaisesRegex(imaplib.IMAP4.readonly,
                                            r"%s is not writable" % expected):
                    client.select(mailbox)

    def test_uid_unknown_command_reports_command(self):
        # The unknown-UID-command error reports the command via repr(), which
        # also avoids BytesWarning for a bytes command under -bb.
        client, _ = self._setup(SimpleIMAPHandler)
        for command, expected in [('BOGUS', "'BOGUS'"), (b'BOGUS', r"b'BOGUS'")]:
            with self.subTest(command=command):
                with self.assertRaisesRegex(
                        imaplib.IMAP4.error,
                        r"Unknown IMAP4 UID command: %s" % expected):
                    client.uid(command, '1')

    def test_logout(self):
        client, _ = self._setup(SimpleIMAPHandler)
        typ, data = client.login('user', 'pass')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data[0], b'LOGIN completed')
        typ, data = client.logout()
        self.assertEqual(typ, 'BYE', (typ, data))
        self.assertEqual(data[0], b'IMAP4ref1 Server logging out', (typ, data))
        self.assertEqual(client.state, 'LOGOUT')

    def test_lsub(self):
        client, server = self._setup(make_simple_handler('LSUB',
            ['* LSUB () "." directoryA', '* LSUB () "." directoryB']))
        client.login('user', 'pass')
        typ, data = client.lsub()
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'() "." directoryA', b'() "." directoryB'])
        self.assertEqual(server.args, ['""', '*'])

        typ, data = client.lsub('~/Mail/', '%')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['~/Mail/', '%'])

        # The directory is quoted when necessary; wildcards in the pattern
        # are preserved.
        typ, data = client.lsub('New folder', '%')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"New folder"', '%'])

    def test_extra_blank_line_after_literal(self):
        # Some buggy servers send an extra blank line after the counted
        # literal data.  imaplib should skip it instead of failing.
        class BlankLineHandler(SimpleIMAPHandler):
            def cmd_FETCH(self, tag, args):
                self._send(b'* 1 FETCH (BODY[HEADER] {13}\r\n')
                self._send(b'Subject: test')       # 13-byte literal
                self._send(b'\r\n)\r\n')            # stray blank line, then ')'
                self._send_tagged(tag, 'OK', 'FETCH completed')
        client, _ = self._setup(BlankLineHandler)
        client.login('user', 'pass')
        client.select()
        typ, data = client.fetch('1', '(BODY[HEADER])')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [(b'1 (BODY[HEADER] {13}', b'Subject: test'),
                                b')'])

    def test_literal_terminating_response(self):
        # A literal ending a response (a LIST mailbox name sent as a literal)
        # has an empty trailer that must not be swallowed.  Conforming case:
        # no spurious blank lines.
        names = [b'My (box)"', b'Another', b'Third']
        class Handler(SimpleIMAPHandler):
            def cmd_LIST(self, tag, args):
                for name in names:
                    self._send(b'* LIST (\\HasNoChildren) "/" {%d}\r\n'
                               % len(name))
                    self._send(name)
                    self._send(b'\r\n')             # ends the response, no blank
                self._send_tagged(tag, 'OK', 'LIST completed')
        client, _ = self._setup(Handler)
        client.login('user', 'pass')
        typ, data = client.list()
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [
            (b'(\\HasNoChildren) "/" {9}', b'My (box)"'), b'',
            (b'(\\HasNoChildren) "/" {7}', b'Another'), b'',
            (b'(\\HasNoChildren) "/" {5}', b'Third'), b'',
        ])

    def test_spurious_blank_lines_between_responses(self):
        # A spurious blank line after each terminating literal falls between the
        # untagged responses and must be skipped, even several in a row.
        names = [b'My (box)"', b'Another', b'Third']
        class Handler(SimpleIMAPHandler):
            def cmd_LIST(self, tag, args):
                for name in names:
                    self._send(b'* LIST (\\HasNoChildren) "/" {%d}\r\n'
                               % len(name))
                    self._send(name)
                    self._send(b'\r\n\r\n')     # ends the response, then a blank
                self._send_tagged(tag, 'OK', 'LIST completed')
        client, _ = self._setup(Handler)
        client.login('user', 'pass')
        typ, data = client.list()
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [
            (b'(\\HasNoChildren) "/" {9}', b'My (box)"'), b'',
            (b'(\\HasNoChildren) "/" {7}', b'Another'), b'',
            (b'(\\HasNoChildren) "/" {5}', b'Third'), b'',
        ])

    def test_unselect(self):
        client, server = self._setup(SimpleIMAPHandler)
        client.login('user', 'pass')
        typ, data = client.select()
        self.assertEqual(typ, 'OK')
        self.assertEqual(data[0], b'2')

        typ, data = client.unselect()
        self.assertEqual(typ, 'OK')
        self.assertEqual(data[0], b'Returned to authenticated state. (Success)')
        self.assertEqual(client.state, 'AUTH')
        self.assertIsNone(server.is_selected)

    def test_enable(self):
        class EnableHandler(SimpleIMAPHandler):
            capabilities = 'IMAP4rev1 ID LITERAL+ ENABLE X-GOOD-IDEA'
            def cmd_ENABLE(self, tag, args):
                capabilities = self.capabilities.split()
                for arg in args:
                    if arg in capabilities:
                        self._send_textline('* ENABLED ' + arg)
                self._send_tagged(tag, 'OK', 'foo')

        client, server = self._setup(EnableHandler)
        client.login('user', 'pass')
        code, data = client.enable('CONDSTORE X-GOOD-IDEA')
        self.assertEqual(code, 'OK')
        self.assertEqual(data, [b'foo'])
        self.assertEqual(server.args, ['CONDSTORE', 'X-GOOD-IDEA'])

    def test_select(self):
        client, server = self._setup(SimpleIMAPHandler)
        client.login('user', 'pass')
        typ, data = client.select()
        self.assertEqual(typ, 'OK')
        self.assertEqual(data[0], b'2')
        self.assertEqual(server.is_selected, ['INBOX'])

        typ, data = client.select('Archive')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.is_selected, ['Archive'])

        # readonly=True issues EXAMINE instead of SELECT
        # (there is no separate examine() method).
        typ, data = client.select(readonly=True)
        self.assertEqual(typ, 'OK')
        self.assertEqual(data[0], b'2')
        self.assertEqual(server.is_selected, ['INBOX'])
        self.assertTrue(client.is_readonly)

        typ, data = client.select('New folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.is_selected, ['"New folder"'])

    def test_expunge(self):
        client, server = self._setup(make_simple_handler('EXPUNGE',
            ['* 3 EXPUNGE', '* 3 EXPUNGE', '* 5 EXPUNGE', '* 8 EXPUNGE']))
        client.login('user', 'pass')
        client.select()
        typ, data = client.expunge()
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'3', b'3', b'5', b'8'])

        # A message set is only accepted with uid=True (UID EXPUNGE).
        with self.assertRaises(imaplib.IMAP4.error):
            client.expunge('3:8')

    def test_uid_expunge(self):
        client, server = self._setup(make_simple_handler('UID',
            ['* 3 EXPUNGE', '* 3 EXPUNGE', '* 5 EXPUNGE', '* 8 EXPUNGE'],
            'UID EXPUNGE completed'))
        client.login('user', 'pass')
        client.select()
        typ, data = client.expunge('3:8', uid=True)
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'3', b'3', b'5', b'8'])
        self.assertEqual(server.args, ['EXPUNGE', '3:8'])

        # UID EXPUNGE requires a message set.
        with self.assertRaises(imaplib.IMAP4.error):
            client.expunge(uid=True)

    def test_close(self):
        client, server = self._setup(make_simple_handler('CLOSE'))
        client.login('user', 'pass')
        client.select()
        typ, data = client.close()
        self.assertEqual(typ, 'OK')
        self.assertEqual(client.state, 'AUTH')

    def test_check(self):
        client, server = self._setup(make_simple_handler('CHECK'))
        client.login('user', 'pass')
        client.select()
        typ, data = client.check()
        self.assertEqual(typ, 'OK')

    def test_noop(self):
        client, server = self._setup(make_simple_handler('NOOP',
            ['* 4 EXISTS']))
        client.login('user', 'pass')
        typ, data = client.noop()
        self.assertEqual(typ, 'OK')
        # NOOP is used to pick up server-pushed untagged responses.
        self.assertEqual(client.untagged_responses['EXISTS'], [b'4'])

    def test_namespace(self):
        client, server = self._setup(make_simple_handler('NAMESPACE',
            ['* NAMESPACE (("" "/")) NIL NIL']))
        client.login('user', 'pass')
        typ, data = client.namespace()
        self.assertEqual(typ, 'OK')
        self.assertEqual(data[0], b'(("" "/")) NIL NIL')

    def test_capability(self):
        client, server = self._setup(make_simple_handler('CAPABILITY',
            ['* CAPABILITY IMAP4rev1 IDLE NAMESPACE']))
        client.login('user', 'pass')
        typ, data = client.capability()
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'IMAP4rev1 IDLE NAMESPACE'])

    def test_recent(self):
        # recent() prods the server with NOOP if no RECENT was seen yet.
        client, server = self._setup(make_simple_handler('NOOP',
            ['* 5 RECENT']))
        client.login('user', 'pass')
        typ, data = client.recent()
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'5'])

    def test_response(self):
        client, server = self._setup(SimpleIMAPHandler)
        client.login('user', 'pass')
        client.select()  # the handler answers with '* 2 EXISTS'
        typ, data = client.response('EXISTS')
        self.assertEqual(typ, 'EXISTS')
        self.assertEqual(data, [b'2'])
        # The value is cleared once read.
        typ, data = client.response('EXISTS')
        self.assertEqual(data, [None])

    def test_create(self):
        client, server = self._setup(make_simple_handler('CREATE'))
        client.login('user', 'pass')
        typ, data = client.create('owatagusiam/')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'CREATE completed'])
        self.assertEqual(server.args, ['owatagusiam/'])

        typ, data = client.create('owatagusiam/blurdybloop')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'CREATE completed'])
        self.assertEqual(server.args, ['owatagusiam/blurdybloop'])

        typ, data = client.create('New folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'CREATE completed'])
        self.assertEqual(server.args, ['"New folder"'])

    def test_copy(self):
        client, server = self._setup(make_simple_handler('COPY'))
        client.login('user', 'pass')
        client.select()
        typ, data = client.copy('2:4', 'MEETING')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'COPY completed'])
        self.assertEqual(server.args, ['2:4', 'MEETING'])

        typ, data = client.copy('2:4', 'New folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'COPY completed'])
        self.assertEqual(server.args, ['2:4', '"New folder"'])

        # A structured message set is formatted into a sequence set.
        typ, data = client.copy([2, (3, 5)], 'MEETING')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['2,3:5', 'MEETING'])

    def test_uid_copy(self):
        client, server = self._setup(make_simple_handler('UID',
            completed='UID COPY completed'))
        client.login('user', 'pass')
        client.select()
        typ, data = client.uid('copy', '4827313:4828442', 'MEETING')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [None])
        self.assertEqual(server.args, ['COPY', '4827313:4828442', 'MEETING'])

        typ, data = client.uid('copy', '4827313:4828442', 'New folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [None])
        self.assertEqual(server.args, ['COPY', '4827313:4828442', '"New folder"'])

        # The uid=True keyword is a shorthand for uid('COPY', ...).
        typ, data = client.copy('4827313:4828442', 'MEETING', uid=True)
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'UID COPY completed'])
        self.assertEqual(server.args, ['COPY', '4827313:4828442', 'MEETING'])

        # A structured message set is formatted into a sequence set.
        typ, data = client.uid('copy', [1, (3, 5)], 'MEETING')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['COPY', '1,3:5', 'MEETING'])

    def test_move(self):
        client, server = self._setup(make_simple_handler('MOVE'))
        client.login('user', 'pass')
        client.select()
        typ, data = client.move('2:4', 'MEETING')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'MOVE completed'])
        self.assertEqual(server.args, ['2:4', 'MEETING'])

        typ, data = client.move('2:4', 'New folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'MOVE completed'])
        self.assertEqual(server.args, ['2:4', '"New folder"'])

        # A structured message set is formatted into a sequence set.
        typ, data = client.move([2, (3, 5)], 'MEETING')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['2,3:5', 'MEETING'])

    def test_uid_move(self):
        client, server = self._setup(make_simple_handler('UID',
            completed='UID MOVE completed'))
        client.login('user', 'pass')
        client.select()
        typ, data = client.uid('move', '4827313:4828442', 'MEETING')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [None])
        self.assertEqual(server.args, ['MOVE', '4827313:4828442', 'MEETING'])

        typ, data = client.uid('move', '4827313:4828442', 'New folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [None])
        self.assertEqual(server.args, ['MOVE', '4827313:4828442', '"New folder"'])

        # The uid=True keyword is a shorthand for uid('MOVE', ...).
        typ, data = client.move('4827313:4828442', 'MEETING', uid=True)
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'UID MOVE completed'])
        self.assertEqual(server.args, ['MOVE', '4827313:4828442', 'MEETING'])

        # A structured message set is formatted into a sequence set.
        typ, data = client.uid('move', [1, (3, 5)], 'MEETING')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['MOVE', '1,3:5', 'MEETING'])

    def test_store(self):
        client, server = self._setup(make_simple_handler('STORE', [
            r'* 2 FETCH (FLAGS (\Deleted \Seen))',
            r'* 3 FETCH (FLAGS (\Deleted))',
            r'* 4 FETCH (FLAGS (\Deleted \Flagged \Seen))',
        ]))
        client.login('user', 'pass')
        client.select()
        typ, data = client.store('2:4', '+FLAGS', r'(\Deleted)')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [
            br'2 (FLAGS (\Deleted \Seen))',
            br'3 (FLAGS (\Deleted))',
            br'4 (FLAGS (\Deleted \Flagged \Seen))',
        ])
        self.assertEqual(server.args, ['2:4', '+FLAGS', r'(\Deleted)'])

        typ, data = client.store('2:4', '+FLAGS', r'\Deleted')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['2:4', '+FLAGS', r'(\Deleted)'])

        # The flags may be a sequence, and the message set may be structured.
        typ, data = client.store([2, (3, 4)], '+FLAGS',
                                 [r'\Deleted', r'\Seen'])
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['2,3:4', '+FLAGS', r'(\Deleted \Seen)'])

    def test_uid_store(self):
        client, server = self._setup(make_simple_handler('UID', [
            r'* 23 FETCH (FLAGS (\Deleted \Seen) UID 4827313)',
            r'* 24 FETCH (FLAGS (\Deleted) UID 4827943)',
            r'* 25 FETCH (FLAGS (\Deleted \Flagged \Seen) UID 4828442)',
        ], 'UID STORE completed'))
        client.login('user', 'pass')
        client.select()
        typ, data = client.uid('store', '4827313:4828442', '+FLAGS', r'(\Deleted)')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [
            br'23 (FLAGS (\Deleted \Seen) UID 4827313)',
            br'24 (FLAGS (\Deleted) UID 4827943)',
            br'25 (FLAGS (\Deleted \Flagged \Seen) UID 4828442)',
        ])
        self.assertEqual(server.args, ['STORE', '4827313:4828442', '+FLAGS', r'(\Deleted)'])

        typ, data = client.uid('store', '4827313:4828442', '+FLAGS', r'\Deleted')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['STORE', '4827313:4828442', '+FLAGS', r'(\Deleted)'])

        # The uid=True keyword is a shorthand for uid('STORE', ...).
        typ, data = client.store('4827313:4828442', '+FLAGS', r'(\Deleted)',
                                 uid=True)
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [
            br'23 (FLAGS (\Deleted \Seen) UID 4827313)',
            br'24 (FLAGS (\Deleted) UID 4827943)',
            br'25 (FLAGS (\Deleted \Flagged \Seen) UID 4828442)',
        ])
        self.assertEqual(server.args, ['STORE', '4827313:4828442', '+FLAGS', r'(\Deleted)'])

        # The flags may be a sequence.
        typ, data = client.uid('store', '4827313:4828442', '+FLAGS',
                               [r'\Deleted', r'\Seen'])
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args,
                         ['STORE', '4827313:4828442', '+FLAGS', r'(\Deleted \Seen)'])

    def test_fetch(self):
        # The handler expands the requested sequence set and answers for
        # exactly those messages, so the test exercises the round trip of
        # the message set, not just a canned reply.
        class FetchHandler(SimpleIMAPHandler):
            messages = 4
            def cmd_SELECT(self, tag, args):
                self.server.is_selected = args
                self._send_line(b'* %d EXISTS' % self.messages)
                self._send_tagged(tag, 'OK', '[READ-WRITE] SELECT completed.')
            def cmd_FETCH(self, tag, args):
                for n in parse_sequence_set(args[0], self.messages):
                    self._send_textline(r'* %d FETCH (FLAGS (\Seen))' % n)
                self._send_tagged(tag, 'OK', 'FETCH completed')

        client, server = self._setup(FetchHandler)
        client.login('user', 'pass')
        client.select()
        typ, data = client.fetch('2:4', '(FLAGS)')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [
            br'2 (FLAGS (\Seen))',
            br'3 (FLAGS (\Seen))',
            br'4 (FLAGS (\Seen))',
        ])
        self.assertEqual(server.args, ['2:4', '(FLAGS)'])

        # A comma-separated set with an open range up to '*'.
        typ, data = client.fetch('1,3:*', '(FLAGS)')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [
            br'1 (FLAGS (\Seen))',
            br'3 (FLAGS (\Seen))',
            br'4 (FLAGS (\Seen))',
        ])
        self.assertEqual(server.args, ['1,3:*', '(FLAGS)'])

        # An item with nested parentheses is sent (and parsed) as a
        # single argument.
        typ, data = client.fetch('1', '(BODY[HEADER.FIELDS (DATE FROM)])')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [br'1 (FLAGS (\Seen))'])
        self.assertEqual(server.args, ['1', '(BODY[HEADER.FIELDS (DATE FROM)])'])

        # message_parts is wrapped in parentheses if it is not already.
        typ, data = client.fetch('2:4', 'FLAGS')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['2:4', '(FLAGS)'])

        # But the macros are not, as they are not data item names.
        typ, data = client.fetch('2:4', 'ALL')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['2:4', 'ALL'])
        typ, data = client.fetch('2:4', 'fast')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['2:4', 'fast'])

        # A structured message set is formatted into a sequence set.
        typ, data = client.fetch([2, (3, 4)], '(FLAGS)')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['2,3:4', '(FLAGS)'])

        # message_parts may be a sequence of items.
        typ, data = client.fetch('1', ['UID', 'FLAGS'])
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['1', '(UID FLAGS)'])

        # 'params' substitutes and quotes '?' placeholders.
        typ, data = client.fetch('1', 'FLAGS BODY[HEADER.FIELDS ?]',
                                 params=[['DATE', 'FROM']])
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args,
                         ['1', '(FLAGS BODY[HEADER.FIELDS (DATE FROM)])'])

    def test_uid_fetch(self):
        client, server = self._setup(make_simple_handler('UID', [
            r'* 23 FETCH (FLAGS (\Seen) UID 4827313)',
            r'* 24 FETCH (FLAGS (\Seen) UID 4827943)',
            r'* 25 FETCH (FLAGS (\Seen) UID 4828442)',
        ], 'UID FETCH completed'))
        client.login('user', 'pass')
        client.select()
        typ, data = client.uid('fetch', '4827313:4828442', '(FLAGS)')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [
            br'23 (FLAGS (\Seen) UID 4827313)',
            br'24 (FLAGS (\Seen) UID 4827943)',
            br'25 (FLAGS (\Seen) UID 4828442)',
        ])
        self.assertEqual(server.args, ['FETCH', '4827313:4828442', '(FLAGS)'])

        typ, data = client.uid('fetch', '4827313:4828442', 'FLAGS')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['FETCH', '4827313:4828442', '(FLAGS)'])

        typ, data = client.uid('fetch', '4827313:4828442', 'ALL')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['FETCH', '4827313:4828442', 'ALL'])

        # The uid=True keyword is a shorthand for uid('FETCH', ...).
        typ, data = client.fetch('4827313:4828442', '(FLAGS)', uid=True)
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [
            br'23 (FLAGS (\Seen) UID 4827313)',
            br'24 (FLAGS (\Seen) UID 4827943)',
            br'25 (FLAGS (\Seen) UID 4828442)',
        ])
        self.assertEqual(server.args, ['FETCH', '4827313:4828442', '(FLAGS)'])

        # message_parts may be a sequence, and 'params' substitutes '?'.
        typ, data = client.uid('fetch', '4827313:4828442', ['UID', 'FLAGS'])
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['FETCH', '4827313:4828442', '(UID FLAGS)'])

        typ, data = client.uid('fetch', '4827313:4828442',
                               'BODY[HEADER.FIELDS ?]', params=[['DATE', 'FROM']])
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args,
                         ['FETCH', '4827313:4828442',
                          '(BODY[HEADER.FIELDS (DATE FROM)])'])

    def test_partial(self):
        client, server = self._setup(make_simple_handler('PARTIAL',
            ['* 1 FETCH (RFC822.TEXT<0.10> "0123456789")']))
        client.login('user', 'pass')
        client.select()
        typ, data = client.partial('1', 'RFC822.TEXT', '0', '10')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'1 (RFC822.TEXT<0.10> "0123456789")'])
        self.assertEqual(server.args, ['1', 'RFC822.TEXT', '0', '10'])

    def test_search(self):
        response = []
        client, server = self._setup(make_simple_handler('SEARCH', response))
        client.login('user', 'pass')
        client.select()
        response[:] = ['* SEARCH 2 84 882']
        typ, data = client.search(None, 'FLAGGED', 'SINCE', '1-Feb-1994', 'NOT', 'FROM', '"Smith"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'2 84 882'])
        self.assertEqual(server.args, ['FLAGGED', 'SINCE', '1-Feb-1994', 'NOT', 'FROM', '"Smith"'])

        response[:] = ['* SEARCH']
        typ, data = client.search(None, 'TEXT', '"string not in mailbox"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b''])
        self.assertEqual(server.args, ['TEXT', '"string not in mailbox"'])

        response[:] = ['* SEARCH 43']
        typ, data = client.search('UTF-8', 'TEXT', 'XXXXXX')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'43'])
        self.assertEqual(server.args, ['CHARSET', 'UTF-8', 'TEXT', 'XXXXXX'])

        # A non-ASCII str criterion is encoded to the declared charset (KOI8-U
        # here, which is not UTF-8, so check the encoded bytes on the wire).
        response[:] = ['* SEARCH 43']
        typ, data = client.search('KOI8-U', 'SUBJECT', '"Київ"')
        self.assertEqual(typ, 'OK')
        self.assertIn(b'CHARSET KOI8-U ', server.line)
        self.assertIn('"Київ"'.encode('koi8-u'), server.line)

        # bytes criteria keep this focused on charset-name quoting (the
        # parentheses force the name to be quoted) without criteria encoding.
        typ, data = client.search('NF_Z_62-010_(1973)', b'TEXT', b'XXXXXX')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['CHARSET', '"NF_Z_62-010_(1973)"', 'TEXT', 'XXXXXX'])

        # 'params' substitutes and quotes '?' placeholders.
        response[:] = ['* SEARCH 1']
        typ, data = client.search(None, 'FROM ? SUBJECT ?',
                                  params=['me@host', 'trip report'])
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args,
                         ['FROM', 'me@host', 'SUBJECT', '"trip report"'])

        # Without 'params', a literal '?' is sent unchanged.
        typ, data = client.search(None, 'SUBJECT', '"what?"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['SUBJECT', '"what?"'])

    def test_uid_search(self):
        response = []
        client, server = self._setup(make_simple_handler('UID', response,
            'UID SEARCH completed'))
        client.login('user', 'pass')
        client.select()
        response[:] = ['* SEARCH 2 84 882']
        typ, data = client.uid('SEARCH', 'FLAGGED', 'SINCE', '1-Feb-1994', 'NOT', 'FROM', '"Smith"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'2 84 882'])
        self.assertEqual(server.args, ['SEARCH', 'FLAGGED', 'SINCE', '1-Feb-1994', 'NOT', 'FROM', '"Smith"'])

        response[:] = ['* SEARCH']
        typ, data = client.uid('SEARCH', 'TEXT', '"string not in mailbox"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b''])
        self.assertEqual(server.args, ['SEARCH', 'TEXT', '"string not in mailbox"'])

        response[:] = ['* SEARCH 43']
        typ, data = client.uid('SEARCH', 'CHARSET', 'UTF-8', 'TEXT', 'XXXXXX')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'43'])
        self.assertEqual(server.args, ['SEARCH', 'CHARSET', 'UTF-8', 'TEXT', 'XXXXXX'])

        typ, data = client.uid('SEARCH', 'CHARSET', '"NF_Z_62-010_(1973)"', 'TEXT', 'XXXXXX')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['SEARCH', 'CHARSET', '"NF_Z_62-010_(1973)"', 'TEXT', 'XXXXXX'])

        # The uid=True keyword is a shorthand for uid('SEARCH', ...).
        response[:] = ['* SEARCH 2 84 882']
        typ, data = client.search(None, 'FLAGGED', 'SINCE', '1-Feb-1994',
                                  'NOT', 'FROM', '"Smith"', uid=True)
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'2 84 882'])
        self.assertEqual(server.args,
                ['SEARCH', 'FLAGGED', 'SINCE', '1-Feb-1994', 'NOT', 'FROM', '"Smith"'])

        response[:] = ['* SEARCH 43']
        typ, data = client.search('UTF-8', 'TEXT', 'XXXXXX', uid=True)
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'43'])
        self.assertEqual(server.args, ['SEARCH', 'CHARSET', 'UTF-8', 'TEXT', 'XXXXXX'])

        # 'params' substitutes and quotes '?' placeholders.
        response[:] = ['* SEARCH 1']
        typ, data = client.uid('SEARCH', 'FROM ? SUBJECT ?',
                               params=['me@host', 'trip report'])
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args,
                         ['SEARCH', 'FROM', 'me@host', 'SUBJECT', '"trip report"'])

    def test_sort(self):
        response = []
        client, server = self._setup(make_simple_handler('SORT', response))
        client.login('user', 'pass')
        client.select()
        response[:] = ['* SORT 2 84 882']
        typ, data = client.sort('(SUBJECT)', 'UTF-8', 'SINCE', '1-Feb-1994')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [br'2 84 882'])
        self.assertEqual(server.args, ['(SUBJECT)', 'UTF-8', 'SINCE', '1-Feb-1994'])

        response[:] = ['* SORT 5 3 4 1 2']
        typ, data = client.sort('(SUBJECT REVERSE DATE)', 'UTF-8', 'ALL')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [br'5 3 4 1 2'])
        self.assertEqual(server.args, ['(SUBJECT REVERSE DATE)', 'UTF-8', 'ALL'])

        response[:] = ['* SORT']
        typ, data = client.sort('(SUBJECT)', 'US-ASCII', 'TEXT', '"not in mailbox"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [br''])
        self.assertEqual(server.args, ['(SUBJECT)', 'US-ASCII', 'TEXT', '"not in mailbox"'])

        typ, data = client.sort('SUBJECT', 'NF_Z_62-010_(1973)', b'TEXT', b'"not in mailbox"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['(SUBJECT)', '"NF_Z_62-010_(1973)"', 'TEXT', '"not in mailbox"'])

        # A non-ASCII str criterion is encoded to the declared charset.
        response[:] = ['* SORT']
        typ, data = client.sort('(SUBJECT)', 'KOI8-U', 'TEXT', '"Київ"')
        self.assertEqual(typ, 'OK')
        self.assertIn('"Київ"'.encode('koi8-u'), server.line)

        # sort_criteria may be a sequence, and 'params' substitutes and
        # quotes '?' (a value with a space becomes a quoted string).
        response[:] = ['* SORT 1']
        typ, data = client.sort(['REVERSE', 'DATE'], 'UTF-8', 'SUBJECT ?',
                                params=['trip report'])
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args,
                         ['(REVERSE DATE)', 'UTF-8', 'SUBJECT', '"trip report"'])

    def test_uid_sort(self):
        response = []
        client, server = self._setup(make_simple_handler('UID', response,
            'UID SORT completed'))
        client.login('user', 'pass')
        client.select()
        response[:] = ['* SORT 2 84 882']
        typ, data = client.uid('sort', '(SUBJECT)', 'UTF-8', 'SINCE', '1-Feb-1994')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [br'2 84 882'])
        self.assertEqual(server.args, ['SORT', '(SUBJECT)', 'UTF-8', 'SINCE', '1-Feb-1994'])

        response[:] = ['* SORT 5 3 4 1 2']
        typ, data = client.uid('sort', '(SUBJECT REVERSE DATE)', 'UTF-8', 'ALL')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [br'5 3 4 1 2'])
        self.assertEqual(server.args, ['SORT', '(SUBJECT REVERSE DATE)', 'UTF-8', 'ALL'])

        response[:] = ['* SORT']
        typ, data = client.uid('sort', '(SUBJECT)', 'US-ASCII', 'TEXT', '"not in mailbox"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [br''])
        self.assertEqual(server.args, ['SORT', '(SUBJECT)', 'US-ASCII', 'TEXT', '"not in mailbox"'])

        typ, data = client.uid('sort', 'SUBJECT', 'NF_Z_62-010_(1973)', b'TEXT', b'"not in mailbox"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['SORT', '(SUBJECT)', '"NF_Z_62-010_(1973)"', 'TEXT', '"not in mailbox"'])

        # A non-ASCII str criterion is encoded to the declared charset.
        response[:] = ['* SORT']
        typ, data = client.uid('sort', '(SUBJECT)', 'KOI8-U', 'TEXT', '"Київ"')
        self.assertEqual(typ, 'OK')
        self.assertIn('"Київ"'.encode('koi8-u'), server.line)

        # The uid=True keyword is a shorthand for uid('SORT', ...).
        response[:] = ['* SORT 2 84 882']
        typ, data = client.sort('(SUBJECT)', 'UTF-8', 'SINCE', '1-Feb-1994', uid=True)
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [br'2 84 882'])
        self.assertEqual(server.args, ['SORT', '(SUBJECT)', 'UTF-8', 'SINCE', '1-Feb-1994'])

        # sort_criteria may be a sequence, and 'params' substitutes and
        # quotes '?' (a value with a space becomes a quoted string).
        response[:] = ['* SORT 1']
        typ, data = client.uid('sort', ['REVERSE', 'DATE'], 'UTF-8', 'SUBJECT ?',
                               params=['trip report'])
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args,
                         ['SORT', '(REVERSE DATE)', 'UTF-8', 'SUBJECT', '"trip report"'])

    def test_thread(self):
        response = []
        client, server = self._setup(make_simple_handler('THREAD', response))
        client.login('user', 'pass')
        client.select()
        response[:] = [
            '* THREAD (166)(167)(168)(169)(172)(170)(171)'
            '(173)(174 (175)(176)(178)(181)(180))(179)(177 '
            '(183)(182)(188)(184)(185)(186)(187)(189))(190)'
            '(191)(192)(193)(194 195)(196 (197)(198))(199)'
            '(200 202)(201)(203)(204)(205)(206 207)(208)']
        typ, data = client.thread('ORDEREDSUBJECT', 'UTF-8', 'SINCE', '5-MAR-2000')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [
            b'(166)(167)(168)(169)(172)(170)(171)'
            b'(173)(174 (175)(176)(178)(181)(180))(179)(177 '
            b'(183)(182)(188)(184)(185)(186)(187)(189))(190)'
            b'(191)(192)(193)(194 195)(196 (197)(198))(199)'
            b'(200 202)(201)(203)(204)(205)(206 207)(208)'])
        self.assertEqual(server.args, ['ORDEREDSUBJECT', 'UTF-8', 'SINCE', '5-MAR-2000'])

        response[:] = [
            '* THREAD (166)(167)(168)(169)(172)((170)(179))'
            '(171)(173)((174)(175)(176)(178)(181)(180))'
            '((177)(183)(182)(188 (184)(189))(185 186)(187))'
            '(190)(191)(192)(193)((194)(195 196))(197 198)'
            '(199)(200 202)(201)(203)(204)(205 206 207)(208)']
        typ, data = client.thread('ORDEREDSUBJECT', 'US-ASCII', 'TEXT', '"gewp"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [
            b'(166)(167)(168)(169)(172)((170)(179))'
            b'(171)(173)((174)(175)(176)(178)(181)(180))'
            b'((177)(183)(182)(188 (184)(189))(185 186)(187))'
            b'(190)(191)(192)(193)((194)(195 196))(197 198)'
            b'(199)(200 202)(201)(203)(204)(205 206 207)(208)'])
        self.assertEqual(server.args, ['ORDEREDSUBJECT', 'US-ASCII', 'TEXT', '"gewp"'])

        typ, data = client.thread('ORDEREDSUBJECT', 'NF_Z_62-010_(1973)', b'TEXT', b'"gewp"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['ORDEREDSUBJECT', '"NF_Z_62-010_(1973)"', 'TEXT', '"gewp"'])

        # A non-ASCII str criterion is encoded to the declared charset.
        response[:] = ['* THREAD (1)']
        typ, data = client.thread('ORDEREDSUBJECT', 'KOI8-U', 'TEXT', '"Київ"')
        self.assertEqual(typ, 'OK')
        self.assertIn('"Київ"'.encode('koi8-u'), server.line)

        # 'params' substitutes and quotes '?' (a value with a space becomes
        # a quoted string).
        response[:] = ['* THREAD (1)']
        typ, data = client.thread('REFERENCES', 'UTF-8', 'SUBJECT ?',
                                  params=['trip report'])
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args,
                         ['REFERENCES', 'UTF-8', 'SUBJECT', '"trip report"'])

    def test_uid_thread(self):
        response = []
        client, server = self._setup(make_simple_handler('UID', response,
            'UID THREAD completed'))
        client.login('user', 'pass')
        client.select()
        response[:] = [
            '* THREAD (166)(167)(168)(169)(172)(170)(171)'
            '(173)(174 (175)(176)(178)(181)(180))(179)(177 '
            '(183)(182)(188)(184)(185)(186)(187)(189))(190)'
            '(191)(192)(193)(194 195)(196 (197)(198))(199)'
            '(200 202)(201)(203)(204)(205)(206 207)(208)']
        typ, data = client.uid('THREAD', 'ORDEREDSUBJECT', 'UTF-8', 'SINCE', '5-MAR-2000')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [
            b'(166)(167)(168)(169)(172)(170)(171)'
            b'(173)(174 (175)(176)(178)(181)(180))(179)(177 '
            b'(183)(182)(188)(184)(185)(186)(187)(189))(190)'
            b'(191)(192)(193)(194 195)(196 (197)(198))(199)'
            b'(200 202)(201)(203)(204)(205)(206 207)(208)'])
        self.assertEqual(server.args, ['THREAD', 'ORDEREDSUBJECT', 'UTF-8', 'SINCE', '5-MAR-2000'])

        response[:] = [
            '* THREAD (166)(167)(168)(169)(172)((170)(179))'
            '(171)(173)((174)(175)(176)(178)(181)(180))'
            '((177)(183)(182)(188 (184)(189))(185 186)(187))'
            '(190)(191)(192)(193)((194)(195 196))(197 198)'
            '(199)(200 202)(201)(203)(204)(205 206 207)(208)']
        typ, data = client.uid('THREAD', 'ORDEREDSUBJECT', 'US-ASCII', 'TEXT', '"gewp"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [
            b'(166)(167)(168)(169)(172)((170)(179))'
            b'(171)(173)((174)(175)(176)(178)(181)(180))'
            b'((177)(183)(182)(188 (184)(189))(185 186)(187))'
            b'(190)(191)(192)(193)((194)(195 196))(197 198)'
            b'(199)(200 202)(201)(203)(204)(205 206 207)(208)'])
        self.assertEqual(server.args, ['THREAD', 'ORDEREDSUBJECT', 'US-ASCII', 'TEXT', '"gewp"'])

        typ, data = client.uid('THREAD', 'ORDEREDSUBJECT', 'NF_Z_62-010_(1973)', b'TEXT', b'"gewp"')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['THREAD', 'ORDEREDSUBJECT', '"NF_Z_62-010_(1973)"', 'TEXT', '"gewp"'])

        # A non-ASCII str criterion is encoded to the declared charset.
        response[:] = ['* THREAD (1)']
        typ, data = client.uid('THREAD', 'ORDEREDSUBJECT', 'KOI8-U', 'TEXT', '"Київ"')
        self.assertEqual(typ, 'OK')
        self.assertIn('"Київ"'.encode('koi8-u'), server.line)

        # The uid=True keyword is a shorthand for uid('THREAD', ...).
        response[:] = ['* THREAD (166)(167)(168)']
        typ, data = client.thread('ORDEREDSUBJECT', 'UTF-8', 'SINCE', '5-MAR-2000',
                                  uid=True)
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'(166)(167)(168)'])
        self.assertEqual(server.args, ['THREAD', 'ORDEREDSUBJECT', 'UTF-8', 'SINCE', '5-MAR-2000'])

        # 'params' substitutes and quotes '?' (a value with a space becomes
        # a quoted string).
        response[:] = ['* THREAD (1)']
        typ, data = client.uid('THREAD', 'REFERENCES', 'UTF-8', 'SUBJECT ?',
                               params=['trip report'])
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args,
                         ['THREAD', 'REFERENCES', 'UTF-8', 'SUBJECT', '"trip report"'])

    def test_delete(self):
        client, server = self._setup(make_simple_handler('DELETE'))
        client.login('user', 'pass')
        typ, data = client.delete('blurdybloop')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'DELETE completed'])
        self.assertEqual(server.args, ['blurdybloop'])

        typ, data = client.delete('foo/bar')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['foo/bar'])

        typ, data = client.delete('New folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"New folder"'])

    def test_rename(self):
        client, server = self._setup(make_simple_handler('RENAME'))
        client.login('user', 'pass')
        typ, data = client.rename('blurdybloop', 'sarasoop')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'RENAME completed'])
        self.assertEqual(server.args, ['blurdybloop', 'sarasoop'])

        typ, data = client.rename('Old folder', 'New folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"Old folder"', '"New folder"'])

    def test_subscribe(self):
        client, server = self._setup(make_simple_handler('SUBSCRIBE'))
        client.login('user', 'pass')
        typ, data = client.subscribe('#news.comp.mail.mime')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'SUBSCRIBE completed'])
        self.assertEqual(server.args, ['#news.comp.mail.mime'])

        typ, data = client.subscribe('New folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"New folder"'])

    def test_unsubscribe(self):
        client, server = self._setup(make_simple_handler('UNSUBSCRIBE'))
        client.login('user', 'pass')
        typ, data = client.unsubscribe('#news.comp.mail.mime')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'UNSUBSCRIBE completed'])
        self.assertEqual(server.args, ['#news.comp.mail.mime'])

        typ, data = client.unsubscribe('New folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"New folder"'])

    def test_list(self):
        client, server = self._setup(make_simple_handler('LIST',
            [r'* LIST (\Noselect) "/" ""',
             r'* LIST (\Unmarked) "/" "~/Mail/foo"']))
        client.login('user', 'pass')
        typ, data = client.list()
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [br'(\Noselect) "/" ""',
                                br'(\Unmarked) "/" "~/Mail/foo"'])
        self.assertEqual(server.args, ['""', '*'])

        typ, data = client.list('~/Mail/', '%')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['~/Mail/', '%'])

        typ, data = client.list('New folder', '*')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"New folder"', '*'])

        # A pattern without wildcards is quoted when necessary.
        typ, data = client.list('~/Mail/', 'My Folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['~/Mail/', '"My Folder"'])

    def test_status(self):
        client, server = self._setup(make_simple_handler('STATUS',
            ['* STATUS blurdybloop (MESSAGES 231 UIDNEXT 44292)']))
        client.login('user', 'pass')
        typ, data = client.status('blurdybloop', '(UIDNEXT MESSAGES)')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'blurdybloop (MESSAGES 231 UIDNEXT 44292)'])
        self.assertEqual(server.args, ['blurdybloop', '(UIDNEXT MESSAGES)'])

        # The names argument is wrapped in parentheses if it is not already.
        typ, data = client.status('New folder', 'UIDNEXT MESSAGES')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"New folder"', '(UIDNEXT MESSAGES)'])

        # The names argument may be a sequence of item names.
        typ, data = client.status('blurdybloop', ['UIDNEXT', 'MESSAGES'])
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['blurdybloop', '(UIDNEXT MESSAGES)'])

    def test_getacl(self):
        client, server = self._setup(make_simple_handler('GETACL',
            ['* ACL INBOX Fred rwipslxetad']))
        client.login('user', 'pass')
        typ, data = client.getacl('INBOX')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'INBOX Fred rwipslxetad'])
        self.assertEqual(server.args, ['INBOX'])

        typ, data = client.getacl('New folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"New folder"'])

    def test_setacl(self):
        client, server = self._setup(make_simple_handler('SETACL'))
        client.login('user', 'pass')
        typ, data = client.setacl('INBOX', 'Fred', 'rwipslxetad')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'SETACL completed'])
        self.assertEqual(server.args, ['INBOX', 'Fred', 'rwipslxetad'])

        typ, data = client.setacl('New folder', 'Fred', '+lr')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"New folder"', 'Fred', '+lr'])

        # The identifier and the rights are quoted when necessary too.
        typ, data = client.setacl('INBOX', 'John Doe', 'a b')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['INBOX', '"John Doe"', '"a b"'])

    def test_deleteacl(self):
        client, server = self._setup(make_simple_handler('DELETEACL'))
        client.login('user', 'pass')
        typ, data = client.deleteacl('INBOX', 'Fred')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'DELETEACL completed'])
        self.assertEqual(server.args, ['INBOX', 'Fred'])

        # The identifier is quoted when necessary too.
        typ, data = client.deleteacl('New folder', 'John Doe')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"New folder"', '"John Doe"'])

    def test_myrights(self):
        client, server = self._setup(make_simple_handler('MYRIGHTS',
            ['* MYRIGHTS INBOX rwiptsldaex']))
        client.login('user', 'pass')
        typ, data = client.myrights('INBOX')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'INBOX rwiptsldaex'])
        self.assertEqual(server.args, ['INBOX'])

        typ, data = client.myrights('New folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"New folder"'])

    def test_getquota(self):
        client, server = self._setup(make_simple_handler('GETQUOTA',
            ['* QUOTA "" (STORAGE 10 512)']))
        client.login('user', 'pass')
        typ, data = client.getquota('#news')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'"" (STORAGE 10 512)'])
        self.assertEqual(server.args, ['#news'])

        typ, data = client.getquota('')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['""'])

    def test_getquotaroot(self):
        client, server = self._setup(make_simple_handler('GETQUOTAROOT',
            ['* QUOTAROOT INBOX ""', '* QUOTA "" (STORAGE 10 512)']))
        client.login('user', 'pass')
        typ, data = client.getquotaroot('INBOX')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [[b'INBOX ""'], [b'"" (STORAGE 10 512)']])
        self.assertEqual(server.args, ['INBOX'])

        typ, data = client.getquotaroot('New folder')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"New folder"'])

    def test_id(self):
        client, server = self._setup(make_simple_handler('ID',
            ['* ID ("name" "Cyrus" "version" "1.5")']))
        typ, data = client.id({'name': 'imaplib', 'version': '3.16'})
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'("name" "Cyrus" "version" "1.5")'])
        self.assertEqual(server.args, ['("name" "imaplib" "version" "3.16")'])

        typ, data = client.id()
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['NIL'])

        # Fields and values are quoted strings; a None value is sent
        # as NIL.
        typ, data = client.id({'name': 'my "client"', 'os': None})
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, [r'("name" "my \"client\"" "os" NIL)'])

    def test_setquota(self):
        client, server = self._setup(make_simple_handler('SETQUOTA',
            ['* QUOTA "" (STORAGE 512)']))
        client.login('user', 'pass')
        typ, data = client.setquota('#news', '(STORAGE 512)')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'"" (STORAGE 512)'])
        self.assertEqual(server.args, ['#news', '(STORAGE 512)'])

        typ, data = client.setquota('', '(STORAGE 512)')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['""', '(STORAGE 512)'])

        # The limits argument is wrapped in parentheses if it is not already.
        typ, data = client.setquota('', 'STORAGE 512')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['""', '(STORAGE 512)'])

    def test_getannotation(self):
        client, server = self._setup(make_simple_handler('GETANNOTATION',
            ['* ANNOTATION INBOX "/comment" ("value.shared" "Hello")']))
        client.login('user', 'pass')
        typ, data = client.getannotation('INBOX', '/comment', 'value.shared')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'INBOX "/comment" ("value.shared" "Hello")'])
        self.assertEqual(server.args, ['INBOX', '/comment', 'value.shared'])

        typ, data = client.getannotation('New folder', '/comment', 'value.shared')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"New folder"', '/comment', 'value.shared'])

    def test_setannotation(self):
        client, server = self._setup(make_simple_handler('SETANNOTATION'))
        client.login('user', 'pass')
        typ, data = client.setannotation('INBOX', '/comment',
                                         '("value.shared" "My comment")')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args,
                         ['INBOX', '/comment', '("value.shared" "My comment")'])

        typ, data = client.setannotation('New folder', '/comment',
                                         '("value.shared" "My comment")')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args,
            ['"New folder"', '/comment', '("value.shared" "My comment")'])

    def test_proxyauth(self):
        client, server = self._setup(make_simple_handler('PROXYAUTH'))
        client.login('user', 'pass')
        typ, data = client.proxyauth('user')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'PROXYAUTH completed'])
        self.assertEqual(server.args, ['user'])

        typ, data = client.proxyauth('us er')
        self.assertEqual(typ, 'OK')
        self.assertEqual(server.args, ['"us er"'])

    def test_xatom(self):
        client, server = self._setup(make_simple_handler('MYCOMMAND',
            completed='MYCOMMAND completed'))
        client.login('user', 'pass')
        self.addCleanup(imaplib.Commands.pop, 'MYCOMMAND', None)
        typ, data = client.xatom('MYCOMMAND', 'arg1', 'arg2')
        self.assertEqual(typ, 'OK')
        self.assertEqual(data, [b'MYCOMMAND completed'])
        self.assertEqual(server.args, ['arg1', 'arg2'])

    def test_uppercase_command_names(self):
        client, server = self._setup(SimpleIMAPHandler)
        client.login('user', 'pass')
        self.assertEqual(client.CAPABILITY, client.capability)
        self.assertEqual(client.SELECT, client.select)
        typ, data = client.CAPABILITY()
        self.assertEqual(typ, 'OK')
        with self.assertRaises(AttributeError):
            client.NONEXISTENT

    def test_control_characters(self):
        client, server = self._setup(SimpleIMAPHandler)
        client.login('user', 'pass')
        # In the default (ASCII) mode a mailbox name is sent as modified UTF-7,
        # which Base64-encodes every non-printable character (NUL, CR and LF
        # included).  So a control character is never sent as a raw byte -- it
        # cannot corrupt the command -- and the name round-trips.
        for c in support.control_characters_c0():
            with self.subTest(c=c):
                typ, _ = client.select(f'a{c}b')
                self.assertEqual(typ, 'OK')
                [name] = server.is_selected
                self.assertNotIn(c, name)
                self.assertEqual(name.encode('ascii').decode('utf-7-imap'),
                                 f'a{c}b')

    def test_mutf7_mailbox_name(self):
        # Without UTF8=ACCEPT a non-ASCII mailbox name is sent as modified
        # UTF-7 (RFC 3501, 5.1.3).  A name that is already modified UTF-7 (e.g.
        # obtained from a raw LIST response) is sent unchanged, whether as str
        # or bytes, so it round-trips without being encoded again.
        client, server = self._setup(SimpleIMAPHandler)
        client.login('user', 'pass')
        for name in ['Entwürfe', 'Entw&APw-rfe', b'Entw&APw-rfe']:
            with self.subTest(name=name):
                server.is_selected = None
                typ, _ = client.select(name)
                self.assertEqual(typ, 'OK')
                self.assertEqual(server.is_selected, ['Entw&APw-rfe'])

    # property tests

    def test_file_property_getter(self):
        client, _ = self._setup(SimpleIMAPHandler)
        with self.assertWarns(DeprecationWarning):
            self.assertIsInstance(client.file.raw, socket.SocketIO)

    def test_file_property_setter(self):
        client, _ = self._setup(SimpleIMAPHandler)
        with self.assertWarns(DeprecationWarning):
            # ensure that the caller closes the existing file
            client.file.close()
        for new_file in [mock.Mock(), None]:
            with self.assertWarns(DeprecationWarning):
                client.file = new_file
            with self.assertWarns(DeprecationWarning):
                self.assertIs(client.file, new_file)

    def test_file_property_setter_should_not_close_previous_file(self):
        client, _ = self._setup(SimpleIMAPHandler)
        with mock.patch.object(client, "_imaplib_file", mock.Mock()) as f:
            f.close.assert_not_called()
            with self.assertWarns(DeprecationWarning):
                self.assertIs(client.file, f)
            with self.assertWarns(DeprecationWarning):
                client.file = None
            with self.assertWarns(DeprecationWarning):
                self.assertIsNone(client.file)
            f.close.assert_not_called()


class NewIMAPTests(NewIMAPTestsMixin, unittest.TestCase):
    imap_class = imaplib.IMAP4
    server_class = socketserver.TCPServer


@unittest.skipUnless(ssl, "SSL not available")
class NewIMAPSSLTests(NewIMAPTestsMixin, unittest.TestCase):
    imap_class = IMAP4_SSL
    server_class = SecureTCPServer

    def test_ssl_raises(self):
        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        self.assertEqual(ssl_context.verify_mode, ssl.CERT_REQUIRED)
        self.assertEqual(ssl_context.check_hostname, True)
        ssl_context.load_verify_locations(CAFILE)

        # Allow for flexible libssl error messages.
        regex = re.compile(r"""(
            IP address mismatch, certificate is not valid for '127.0.0.1'   # OpenSSL
            |
            CERTIFICATE_VERIFY_FAILED                                       # AWS-LC
        )""", re.X)
        with self.assertRaisesRegex(ssl.CertificateError, regex):
            _, server = self._setup(SimpleIMAPHandler, connect=False)
            client = self.imap_class(*server.server_address,
                                     ssl_context=ssl_context)
            client.shutdown()

    def test_ssl_verified(self):
        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ssl_context.load_verify_locations(CAFILE)

        _, server = self._setup(SimpleIMAPHandler, connect=False)
        client = self.imap_class("localhost", server.server_address[1],
                                 ssl_context=ssl_context)
        client.shutdown()

class ThreadedNetworkedTests(unittest.TestCase):
    server_class = socketserver.TCPServer
    imap_class = imaplib.IMAP4

    def make_server(self, addr, hdlr):

        class MyServer(self.server_class):
            def handle_error(self, request, client_address):
                self.close_request(request)
                self.server_close()
                raise

        if verbose:
            print("creating server")
        server = MyServer(addr, hdlr)
        self.assertEqual(server.server_address, server.socket.getsockname())

        if verbose:
            print("server created")
            print("ADDR =", addr)
            print("CLASS =", self.server_class)
            print("HDLR =", server.RequestHandlerClass)

        t = threading.Thread(
            name='%s serving' % self.server_class,
            target=server.serve_forever,
            # Short poll interval to make the test finish quickly.
            # Time between requests is short enough that we won't wake
            # up spuriously too many times.
            kwargs={'poll_interval': 0.01})
        t.daemon = True  # In case this function raises.
        t.start()
        if verbose:
            print("server running")
        return server, t

    def reap_server(self, server, thread):
        if verbose:
            print("waiting for server")
        server.shutdown()
        server.server_close()
        thread.join()
        if verbose:
            print("done")

    @contextmanager
    def reaped_server(self, hdlr):
        server, thread = self.make_server((socket_helper.HOST, 0), hdlr)
        try:
            yield server
        finally:
            self.reap_server(server, thread)

    @contextmanager
    def reaped_pair(self, hdlr):
        with self.reaped_server(hdlr) as server:
            client = self.imap_class(*server.server_address)
            try:
                yield server, client
            finally:
                client.logout()

    @threading_helper.reap_threads
    def test_connect(self):
        with self.reaped_server(SimpleIMAPHandler) as server:
            client = self.imap_class(*server.server_address)
            client.shutdown()

    @threading_helper.reap_threads
    def test_bracket_flags(self):

        # This violates RFC 3501, which disallows ']' characters in flags,
        # but imaplib has allowed producing such flags forever, other programs
        # also produce them (eg: OtherInbox's Organizer app as of 20140716),
        # and Gmail, for example, accepts them and produces them.  So we
        # support them.  See issue #21815.

        class BracketFlagHandler(SimpleIMAPHandler):

            def handle(self):
                self.flags = ['Answered', 'Flagged', 'Deleted', 'Seen', 'Draft']
                super().handle()

            def cmd_AUTHENTICATE(self, tag, args):
                self._send_textline('+')
                self.server.response = yield
                self._send_tagged(tag, 'OK', 'FAKEAUTH successful')

            def cmd_SELECT(self, tag, args):
                flag_msg = ' \\'.join(self.flags)
                self._send_line(('* FLAGS (%s)' % flag_msg).encode('ascii'))
                self._send_line(b'* 2 EXISTS')
                self._send_line(b'* 0 RECENT')
                msg = ('* OK [PERMANENTFLAGS %s \\*)] Flags permitted.'
                        % flag_msg)
                self._send_line(msg.encode('ascii'))
                self._send_tagged(tag, 'OK', '[READ-WRITE] SELECT completed.')

            def cmd_STORE(self, tag, args):
                new_flags = args[2].strip('(').strip(')').split()
                self.flags.extend(new_flags)
                flags_msg = '(FLAGS (%s))' % ' \\'.join(self.flags)
                msg = '* %s FETCH %s' % (args[0], flags_msg)
                self._send_line(msg.encode('ascii'))
                self._send_tagged(tag, 'OK', 'STORE completed.')

        with self.reaped_pair(BracketFlagHandler) as (server, client):
            code, data = client.authenticate('MYAUTH', lambda x: b'fake')
            self.assertEqual(code, 'OK')
            self.assertEqual(server.response, b'ZmFrZQ==\r\n')
            client.select('test')
            typ, [data] = client.store(b'1', "+FLAGS", "[test]")
            self.assertIn(b'[test]', data)
            client.select('test')
            typ, [data] = client.response('PERMANENTFLAGS')
            self.assertIn(b'[test]', data)

    @threading_helper.reap_threads
    def test_issue5949(self):

        class EOFHandler(socketserver.StreamRequestHandler):
            def handle(self):
                # EOF without sending a complete welcome message.
                self.wfile.write(b'* OK')

        with self.reaped_server(EOFHandler) as server:
            self.assertRaises(imaplib.IMAP4.abort,
                              self.imap_class, *server.server_address)

    @threading_helper.reap_threads
    def test_line_termination(self):

        class BadNewlineHandler(SimpleIMAPHandler):

            def cmd_CAPABILITY(self, tag, args):
                self._send(b'* CAPABILITY IMAP4rev1 AUTH\n')
                self._send_tagged(tag, 'OK', 'CAPABILITY completed')

        with self.reaped_server(BadNewlineHandler) as server:
            self.assertRaises(imaplib.IMAP4.abort,
                              self.imap_class, *server.server_address)

    class UTF8Server(SimpleIMAPHandler):
        capabilities = 'AUTH ENABLE UTF8=ACCEPT'

        def cmd_ENABLE(self, tag, args):
            self._send_tagged(tag, 'OK', 'ENABLE successful')

        def cmd_AUTHENTICATE(self, tag, args):
            self._send_textline('+')
            self.server.response = yield
            self._send_tagged(tag, 'OK', 'FAKEAUTH successful')

    @threading_helper.reap_threads
    def test_enable_raises_error_if_not_AUTH(self):
        with self.reaped_pair(self.UTF8Server) as (server, client):
            self.assertFalse(client.utf8_enabled)
            self.assertRaises(imaplib.IMAP4.error, client.enable, 'foo')
            self.assertFalse(client.utf8_enabled)

    # XXX Also need a test that enable after SELECT raises an error.

    @threading_helper.reap_threads
    def test_enable_raises_error_if_no_capability(self):
        class NoEnableServer(self.UTF8Server):
            capabilities = 'AUTH'
        with self.reaped_pair(NoEnableServer) as (server, client):
            self.assertRaises(imaplib.IMAP4.error, client.enable, 'foo')

    @threading_helper.reap_threads
    def test_enable_UTF8_raises_error_if_not_supported(self):
        class NonUTF8Server(SimpleIMAPHandler):
            pass
        with self.assertRaises(imaplib.IMAP4.error):
            with self.reaped_pair(NonUTF8Server) as (server, client):
                typ, data = client.login('user', 'pass')
                self.assertEqual(typ, 'OK')
                client.enable('UTF8=ACCEPT')

    @threading_helper.reap_threads
    def test_enable_UTF8_True_append(self):

        class UTF8AppendServer(self.UTF8Server):
            def cmd_APPEND(self, tag, args):
                self.server.response = args
                self.server.response.append(_read_literal(self, args[-1]))
                literal = yield
                self.server.response.append(literal)
                self._send_tagged(tag, 'OK', 'okay')

        with self.reaped_pair(UTF8AppendServer) as (server, client):
            self.assertEqual(client._encoding, 'ascii')
            code, _ = client.authenticate('MYAUTH', lambda x: b'fake')
            self.assertEqual(code, 'OK')
            self.assertEqual(server.response,
                             b'ZmFrZQ==\r\n')  # b64 encoded 'fake'
            code, _ = client.enable('UTF8=ACCEPT')
            self.assertEqual(code, 'OK')
            self.assertEqual(client._encoding, 'utf-8')
            self.assertEqual(server.args, ['UTF8=ACCEPT'])
            msg_string = 'Subject: üñí©öðé'
            typ, data = client.append(
                None, None, None, (msg_string + '\n').encode('utf-8'))
            self.assertEqual(typ, 'OK')
            self.assertEqual(server.response,
                ['INBOX', 'UTF8',
                 '(~{25}', ('%s\r\n' % msg_string).encode('utf-8'),
                 b')\r\n' ])

    # XXX also need a test that makes sure that the Literal and Untagged_status
    # regexes uses unicode in UTF8 mode instead of the default ASCII.

    @threading_helper.reap_threads
    def test_search_disallows_charset_in_utf8_mode(self):
        with self.reaped_pair(self.UTF8Server) as (server, client):
            typ, _ = client.authenticate('MYAUTH', lambda x: b'fake')
            self.assertEqual(typ, 'OK')
            typ, _ = client.enable('UTF8=ACCEPT')
            self.assertEqual(typ, 'OK')
            self.assertTrue(client.utf8_enabled)
            self.assertRaises(imaplib.IMAP4.error, client.search, 'foo', 'bar')

    @threading_helper.reap_threads
    def test_bad_auth_name(self):

        class MyServer(SimpleIMAPHandler):

            def cmd_AUTHENTICATE(self, tag, args):
                self._send_tagged(tag, 'NO', 'unrecognized authentication '
                                  'type {}'.format(args[0]))

        with self.reaped_pair(MyServer) as (server, client):
            with self.assertRaises(imaplib.IMAP4.error):
                client.authenticate('METHOD', lambda: 1)

    @threading_helper.reap_threads
    def test_invalid_authentication(self):

        class MyServer(SimpleIMAPHandler):

            def cmd_AUTHENTICATE(self, tag, args):
                self._send_textline('+')
                self.response = yield
                self._send_tagged(tag, 'NO', '[AUTHENTICATIONFAILED] invalid')

        with self.reaped_pair(MyServer) as (server, client):
            with self.assertRaises(imaplib.IMAP4.error):
                code, data = client.authenticate('MYAUTH', lambda x: b'fake')

    @threading_helper.reap_threads
    def test_valid_authentication(self):

        class MyServer(SimpleIMAPHandler):

            def cmd_AUTHENTICATE(self, tag, args):
                self._send_textline('+')
                self.server.response = yield
                self._send_tagged(tag, 'OK', 'FAKEAUTH successful')

        with self.reaped_pair(MyServer) as (server, client):
            code, data = client.authenticate('MYAUTH', lambda x: b'fake')
            self.assertEqual(code, 'OK')
            self.assertEqual(server.response,
                             b'ZmFrZQ==\r\n')  # b64 encoded 'fake'

        with self.reaped_pair(MyServer) as (server, client):
            code, data = client.authenticate('MYAUTH', lambda x: 'fake')
            self.assertEqual(code, 'OK')
            self.assertEqual(server.response,
                             b'ZmFrZQ==\r\n')  # b64 encoded 'fake'

    @threading_helper.reap_threads
    @hashlib_helper.requires_hashdigest('md5', openssl=True)
    def test_login_cram_md5(self):

        class AuthHandler(SimpleIMAPHandler):

            capabilities = 'LOGINDISABLED AUTH=CRAM-MD5'

            def cmd_AUTHENTICATE(self, tag, args):
                self._send_textline('+ PDE4OTYuNjk3MTcwOTUyQHBvc3RvZmZpY2Uucm'
                                    'VzdG9uLm1jaS5uZXQ=')
                r = yield
                if (r == b'dGltIGYxY2E2YmU0NjRiOWVmYT'
                         b'FjY2E2ZmZkNmNmMmQ5ZjMy\r\n'):
                    self._send_tagged(tag, 'OK', 'CRAM-MD5 successful')
                else:
                    self._send_tagged(tag, 'NO', 'No access')

        with self.reaped_pair(AuthHandler) as (server, client):
            self.assertTrue('AUTH=CRAM-MD5' in client.capabilities)
            ret, data = client.login_cram_md5("tim", "tanstaaftanstaaf")
            self.assertEqual(ret, "OK")

        with self.reaped_pair(AuthHandler) as (server, client):
            self.assertTrue('AUTH=CRAM-MD5' in client.capabilities)
            ret, data = client.login_cram_md5("tim", b"tanstaaftanstaaf")
            self.assertEqual(ret, "OK")


    @threading_helper.reap_threads
    def test_aborted_authentication(self):

        class MyServer(SimpleIMAPHandler):

            def cmd_AUTHENTICATE(self, tag, args):
                self._send_textline('+')
                self.response = yield

                if self.response == b'*\r\n':
                    self._send_tagged(tag, 'NO', '[AUTHENTICATIONFAILED] aborted')
                else:
                    self._send_tagged(tag, 'OK', 'MYAUTH successful')

        with self.reaped_pair(MyServer) as (server, client):
            with self.assertRaises(imaplib.IMAP4.error):
                code, data = client.authenticate('MYAUTH', lambda x: None)


    def test_linetoolong(self):
        class TooLongHandler(SimpleIMAPHandler):
            def handle(self):
                # Send a very long response line
                self.wfile.write(b'* OK ' + imaplib._MAXLINE * b'x' + b'\r\n')

        with self.reaped_server(TooLongHandler) as server:
            self.assertRaises(imaplib.IMAP4.error,
                              self.imap_class, *server.server_address)

    def test_truncated_large_literal(self):
        size = 0
        class BadHandler(SimpleIMAPHandler):
            def handle(self):
                self._send_textline('* OK {%d}' % size)
                self._send_textline('IMAP4rev1')

        for exponent in range(15, 64):
            size = 1 << exponent
            with self.subTest(f"size=2e{size}"):
                with self.reaped_server(BadHandler) as server:
                    with self.assertRaises(imaplib.IMAP4.abort):
                        self.imap_class(*server.server_address)

    @threading_helper.reap_threads
    def test_simple_with_statement(self):
        # simplest call
        with self.reaped_server(SimpleIMAPHandler) as server:
            with self.imap_class(*server.server_address):
                pass

    @threading_helper.reap_threads
    def test_with_statement(self):
        with self.reaped_server(SimpleIMAPHandler) as server:
            with self.imap_class(*server.server_address) as imap:
                imap.login('user', 'pass')
                self.assertEqual(server.logged, ['user', '"pass"'])
            self.assertIsNone(server.logged)

    @threading_helper.reap_threads
    def test_with_statement_logout(self):
        # what happens if already logout in the block?
        with self.reaped_server(SimpleIMAPHandler) as server:
            with self.imap_class(*server.server_address) as imap:
                imap.login('user', 'pass')
                self.assertEqual(server.logged, ['user', '"pass"'])
                imap.logout()
                self.assertIsNone(server.logged)
            self.assertIsNone(server.logged)

    @threading_helper.reap_threads
    @cpython_only
    @unittest.skipUnless(__debug__, "Won't work if __debug__ is False")
    def test_dump_ur(self):
        # See: http://bugs.python.org/issue26543
        untagged_resp_dict = {'READ-WRITE': [b'']}

        with self.reaped_server(SimpleIMAPHandler) as server:
            with self.imap_class(*server.server_address) as imap:
                with mock.patch.object(imap, '_mesg') as mock_mesg:
                    imap._dump_ur(untagged_resp_dict)
                    mock_mesg.assert_called_with(
                        "untagged responses dump:READ-WRITE: [b'']"
                    )


@unittest.skipUnless(ssl, "SSL not available")
class ThreadedNetworkedTestsSSL(ThreadedNetworkedTests):
    server_class = SecureTCPServer
    imap_class = IMAP4_SSL

    @threading_helper.reap_threads
    def test_ssl_verified(self):
        ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
        ssl_context.load_verify_locations(CAFILE)

        # Allow for flexible libssl error messages.
        regex = re.compile(r"""(
            IP address mismatch, certificate is not valid for '127.0.0.1'   # OpenSSL
            |
            CERTIFICATE_VERIFY_FAILED                                       # AWS-LC
        )""", re.X)
        with self.assertRaisesRegex(ssl.CertificateError, regex):
            with self.reaped_server(SimpleIMAPHandler) as server:
                client = self.imap_class(*server.server_address,
                                         ssl_context=ssl_context)
                client.shutdown()

        with self.reaped_server(SimpleIMAPHandler) as server:
            client = self.imap_class("localhost", server.server_address[1],
                                     ssl_context=ssl_context)
            client.shutdown()


class TestModule(unittest.TestCase):
    def test_deprecated__version__(self):
        with self.assertWarnsRegex(
            DeprecationWarning,
            "'__version__' is deprecated and slated for removal in Python 3.20",
        ) as cm:
            getattr(imaplib, "__version__")
        self.assertEqual(cm.filename, __file__)


if __name__ == "__main__":
    unittest.main()
