"""IMAP4 client.

Based on RFC 2060.

Public class:           IMAP4
Public variable:        Debug
Public functions:       Internaldate2tuple
                        Int2AP
                        ParseFlags
                        Time2Internaldate
"""

# Author: Piers Lauder <piers@cs.su.oz.au> December 1997.
#
# Authentication code contributed by Donn Cave <donn@u.washington.edu> June 1998.
# String method conversion by ESR, February 2001.

__version__ = "2.40"

import binascii, re, socket, time, random, sys

__all__ = ["IMAP4", "Internaldate2tuple",
           "Int2AP", "ParseFlags", "Time2Internaldate"]

#       Globals

CRLF = '\r\n'
Debug = 0
IMAP4_PORT = 143
AllowedVersions = ('IMAP4REV1', 'IMAP4')        # Most recent first

#       Commands

Commands = {
        # name            valid states
        'APPEND':       ('AUTH', 'SELECTED'),
        'AUTHENTICATE': ('NONAUTH',),
        'CAPABILITY':   ('NONAUTH', 'AUTH', 'SELECTED', 'LOGOUT'),
        'CHECK':        ('SELECTED',),
        'CLOSE':        ('SELECTED',),
        'COPY':         ('SELECTED',),
        'CREATE':       ('AUTH', 'SELECTED'),
        'DELETE':       ('AUTH', 'SELECTED'),
        'EXAMINE':      ('AUTH', 'SELECTED'),
        'EXPUNGE':      ('SELECTED',),
        'FETCH':        ('SELECTED',),
        'LIST':         ('AUTH', 'SELECTED'),
        'LOGIN':        ('NONAUTH',),
        'LOGOUT':       ('NONAUTH', 'AUTH', 'SELECTED', 'LOGOUT'),
        'LSUB':         ('AUTH', 'SELECTED'),
        'NOOP':         ('NONAUTH', 'AUTH', 'SELECTED', 'LOGOUT'),
        'PARTIAL':      ('SELECTED',),
        'RENAME':       ('AUTH', 'SELECTED'),
        'SEARCH':       ('SELECTED',),
        'SELECT':       ('AUTH', 'SELECTED'),
        'STATUS':       ('AUTH', 'SELECTED'),
        'STORE':        ('SELECTED',),
        'SUBSCRIBE':    ('AUTH', 'SELECTED'),
        'UID':          ('SELECTED',),
        'UNSUBSCRIBE':  ('AUTH', 'SELECTED'),
        }

#       Patterns to match server responses

Continuation = re.compile(r'\+( (?P<data>.*))?')
Flags = re.compile(r'.*FLAGS \((?P<flags>[^\)]*)\)')
InternalDate = re.compile(r'.*INTERNALDATE "'
        r'(?P<day>[ 123][0-9])-(?P<mon>[A-Z][a-z][a-z])-(?P<year>[0-9][0-9][0-9][0-9])'
        r' (?P<hour>[0-9][0-9]):(?P<min>[0-9][0-9]):(?P<sec>[0-9][0-9])'
        r' (?P<zonen>[-+])(?P<zoneh>[0-9][0-9])(?P<zonem>[0-9][0-9])'
        r'"')
Literal = re.compile(r'.*{(?P<size>\d+)}$')
Response_code = re.compile(r'\[(?P<type>[A-Z-]+)( (?P<data>[^\]]*))?\]')
Untagged_response = re.compile(r'\* (?P<type>[A-Z-]+)( (?P<data>.*))?')
Untagged_status = re.compile(r'\* (?P<data>\d+) (?P<type>[A-Z-]+)( (?P<data2>.*))?')



class IMAP4:

    """IMAP4 client class.

    Instantiate with: IMAP4([host[, port]])

            host - host's name (default: localhost);
            port - port number (default: standard IMAP4 port).

    All IMAP4rev1 commands are supported by methods of the same
    name (in lower-case).

    All arguments to commands are converted to strings, except for
    AUTHENTICATE, and the last argument to APPEND which is passed as
    an IMAP4 literal.  If necessary (the string contains any
    non-printing characters or white-space and isn't enclosed with
    either parentheses or double quotes) each string is quoted.
    However, the 'password' argument to the LOGIN command is always
    quoted.  If you want to avoid having an argument string quoted
    (eg: the 'flags' argument to STORE) then enclose the string in
    parentheses (eg: "(\Deleted)").

    Each command returns a tuple: (type, [data, ...]) where 'type'
    is usually 'OK' or 'NO', and 'data' is either the text from the
    tagged response, or untagged results from command.

    Errors raise the exception class <instance>.error("<reason>").
    IMAP4 server errors raise <instance>.abort("<reason>"),
    which is a sub-class of 'error'. Mailbox status changes
    from READ-WRITE to READ-ONLY raise the exception class
    <instance>.readonly("<reason>"), which is a sub-class of 'abort'.

    "error" exceptions imply a program error.
    "abort" exceptions imply the connection should be reset, and
            the command re-tried.
    "readonly" exceptions imply the command should be re-tried.

    Note: to use this module, you must read the RFCs pertaining
    to the IMAP4 protocol, as the semantics of the arguments to
    each IMAP4 command are left to the invoker, not to mention
    the results.
    """

    class error(Exception): pass    # Logical errors - debug required
    class abort(error): pass        # Service errors - close and retry
    class readonly(abort): pass     # Mailbox status changed to READ-ONLY

    mustquote = re.compile(r"[^\w!#$%&'*+,.:;<=>?^`|~-]")

    def __init__(self, host = '', port = IMAP4_PORT):
        self.host = host
        self.port = port
        self.debug = Debug
        self.state = 'LOGOUT'
        self.literal = None             # A literal argument to a command
        self.tagged_commands = {}       # Tagged commands awaiting response
        self.untagged_responses = {}    # {typ: [data, ...], ...}
        self.continuation_response = '' # Last continuation response
        self.is_readonly = None         # READ-ONLY desired state
        self.tagnum = 0

        # Open socket to server.

        self.open(host, port)

        # Create unique tag for this session,
        # and compile tagged response matcher.

        self.tagpre = Int2AP(random.randint(0, 31999))
        self.tagre = re.compile(r'(?P<tag>'
                        + self.tagpre
                        + r'\d+) (?P<type>[A-Z]+) (?P<data>.*)')

        # Get server welcome message,
        # request and store CAPABILITY response.

        if __debug__:
            if self.debug >= 1:
                _mesg('new IMAP4 connection, tag=%s' % self.tagpre)

        self.welcome = self._get_response()
        if self.untagged_responses.has_key('PREAUTH'):
            self.state = 'AUTH'
        elif self.untagged_responses.has_key('OK'):
            self.state = 'NONAUTH'
        else:
            raise self.error(self.welcome)

        cap = 'CAPABILITY'
        self._simple_command(cap)
        if not self.untagged_responses.has_key(cap):
            raise self.error('no CAPABILITY response from server')
        self.capabilities = tuple(self.untagged_responses[cap][-1].upper().split())

        if __debug__:
            if self.debug >= 3:
                _mesg('CAPABILITIES: %s' % `self.capabilities`)

        for version in AllowedVersions:
            if not version in self.capabilities:
                continue
            self.PROTOCOL_VERSION = version
            return

        raise self.error('server not IMAP4 compliant')


    def __getattr__(self, attr):
        #       Allow UPPERCASE variants of IMAP4 command methods.
        if Commands.has_key(attr):
            return eval("self.%s" % attr.lower())
        raise AttributeError("Unknown IMAP4 command: '%s'" % attr)



    #       Public methods


    def open(self, host, port):
        """Setup 'self.sock' and 'self.file'."""
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))
        self.file = self.sock.makefile('r')


    def recent(self):
        """Return most recent 'RECENT' responses if any exist,
        else prompt server for an update using the 'NOOP' command.

        (typ, [data]) = <instance>.recent()

        'data' is None if no new messages,
        else list of RECENT responses, most recent last.
        """
        name = 'RECENT'
        typ, dat = self._untagged_response('OK', [None], name)
        if dat[-1]:
            return typ, dat
        typ, dat = self.noop()  # Prod server for response
        return self._untagged_response(typ, dat, name)


    def response(self, code):
        """Return data for response 'code' if received, or None.

        Old value for response 'code' is cleared.

        (code, [data]) = <instance>.response(code)
        """
        return self._untagged_response(code, [None], code.upper())


    def socket(self):
        """Return socket instance used to connect to IMAP4 server.

        socket = <instance>.socket()
        """
        return self.sock



    #       IMAP4 commands


    def append(self, mailbox, flags, date_time, message):
        """Append message to named mailbox.

        (typ, [data]) = <instance>.append(mailbox, flags, date_time, message)

                All args except `message' can be None.
        """
        name = 'APPEND'
        if not mailbox:
            mailbox = 'INBOX'
        if flags:
            if (flags[0],flags[-1]) != ('(',')'):
                flags = '(%s)' % flags
        else:
            flags = None
        if date_time:
            date_time = Time2Internaldate(date_time)
        else:
            date_time = None
        self.literal = message
        return self._simple_command(name, mailbox, flags, date_time)


    def authenticate(self, mechanism, authobject):
        """Authenticate command - requires response processing.

        'mechanism' specifies which authentication mechanism is to
        be used - it must appear in <instance>.capabilities in the
        form AUTH=<mechanism>.

        'authobject' must be a callable object:

                data = authobject(response)

        It will be called to process server continuation responses.
        It should return data that will be encoded and sent to server.
        It should return None if the client abort response '*' should
        be sent instead.
        """
        mech = mechanism.upper()
        cap = 'AUTH=%s' % mech
        if not cap in self.capabilities:
            raise self.error("Server doesn't allow %s authentication." % mech)
        self.literal = _Authenticator(authobject).process
        typ, dat = self._simple_command('AUTHENTICATE', mech)
        if typ != 'OK':
            raise self.error(dat[-1])
        self.state = 'AUTH'
        return typ, dat


    def check(self):
        """Checkpoint mailbox on server.

        (typ, [data]) = <instance>.check()
        """
        return self._simple_command('CHECK')


    def close(self):
        """Close currently selected mailbox.

        Deleted messages are removed from writable mailbox.
        This is the recommended command before 'LOGOUT'.

        (typ, [data]) = <instance>.close()
        """
        try:
            typ, dat = self._simple_command('CLOSE')
        finally:
            self.state = 'AUTH'
        return typ, dat


    def copy(self, message_set, new_mailbox):
        """Copy 'message_set' messages onto end of 'new_mailbox'.

        (typ, [data]) = <instance>.copy(message_set, new_mailbox)
        """
        return self._simple_command('COPY', message_set, new_mailbox)


    def create(self, mailbox):
        """Create new mailbox.

        (typ, [data]) = <instance>.create(mailbox)
        """
        return self._simple_command('CREATE', mailbox)


    def delete(self, mailbox):
        """Delete old mailbox.

        (typ, [data]) = <instance>.delete(mailbox)
        """
        return self._simple_command('DELETE', mailbox)


    def expunge(self):
        """Permanently remove deleted items from selected mailbox.

        Generates 'EXPUNGE' response for each deleted message.

        (typ, [data]) = <instance>.expunge()

        'data' is list of 'EXPUNGE'd message numbers in order received.
        """
        name = 'EXPUNGE'
        typ, dat = self._simple_command(name)
        return self._untagged_response(typ, dat, name)


    def fetch(self, message_set, message_parts):
        """Fetch (parts of) messages.

        (typ, [data, ...]) = <instance>.fetch(message_set, message_parts)

        'message_parts' should be a string of selected parts
        enclosed in parentheses, eg: "(UID BODY[TEXT])".

        'data' are tuples of message part envelope and data.
        """
        name = 'FETCH'
        typ, dat = self._simple_command(name, message_set, message_parts)
        return self._untagged_response(typ, dat, name)


    def list(self, directory='""', pattern='*'):
        """List mailbox names in directory matching pattern.

        (typ, [data]) = <instance>.list(directory='""', pattern='*')

        'data' is list of LIST responses.
        """
        name = 'LIST'
        typ, dat = self._simple_command(name, directory, pattern)
        return self._untagged_response(typ, dat, name)


    def login(self, user, password):
        """Identify client using plaintext password.

        (typ, [data]) = <instance>.login(user, password)

        NB: 'password' will be quoted.
        """
        #if not 'AUTH=LOGIN' in self.capabilities:
        #       raise self.error("Server doesn't allow LOGIN authentication." % mech)
        typ, dat = self._simple_command('LOGIN', user, self._quote(password))
        if typ != 'OK':
            raise self.error(dat[-1])
        self.state = 'AUTH'
        return typ, dat


    def logout(self):
        """Shutdown connection to server.

        (typ, [data]) = <instance>.logout()

        Returns server 'BYE' response.
        """
        self.state = 'LOGOUT'
        try: typ, dat = self._simple_command('LOGOUT')
        except: typ, dat = 'NO', ['%s: %s' % sys.exc_info()[:2]]
        self.file.close()
        self.sock.close()
        if self.untagged_responses.has_key('BYE'):
            return 'BYE', self.untagged_responses['BYE']
        return typ, dat


    def lsub(self, directory='""', pattern='*'):
        """List 'subscribed' mailbox names in directory matching pattern.

        (typ, [data, ...]) = <instance>.lsub(directory='""', pattern='*')

        'data' are tuples of message part envelope and data.
        """
        name = 'LSUB'
        typ, dat = self._simple_command(name, directory, pattern)
        return self._untagged_response(typ, dat, name)


    def noop(self):
        """Send NOOP command.

        (typ, data) = <instance>.noop()
        """
        if __debug__:
            if self.debug >= 3:
                _dump_ur(self.untagged_responses)
        return self._simple_command('NOOP')


    def partial(self, message_num, message_part, start, length):
        """Fetch truncated part of a message.

        (typ, [data, ...]) = <instance>.partial(message_num, message_part, start, length)

        'data' is tuple of message part envelope and data.
        """
        name = 'PARTIAL'
        typ, dat = self._simple_command(name, message_num, message_part, start, length)
        return self._untagged_response(typ, dat, 'FETCH')


    def rename(self, oldmailbox, newmailbox):
        """Rename old mailbox name to new.

        (typ, data) = <instance>.rename(oldmailbox, newmailbox)
        """
        return self._simple_command('RENAME', oldmailbox, newmailbox)


    def search(self, charset, *criteria):
        """Search mailbox for matching messages.

        (typ, [data]) = <instance>.search(charset, criterium, ...)

        'data' is space separated list of matching message numbers.
        """
        name = 'SEARCH'
        if charset:
            charset = 'CHARSET ' + charset
        typ, dat = apply(self._simple_command, (name, charset) + criteria)
        return self._untagged_response(typ, dat, name)


    def select(self, mailbox='INBOX', readonly=None):
        """Select a mailbox.

        Flush all untagged responses.

        (typ, [data]) = <instance>.select(mailbox='INBOX', readonly=None)

        'data' is count of messages in mailbox ('EXISTS' response).
        """
        # Mandated responses are ('FLAGS', 'EXISTS', 'RECENT', 'UIDVALIDITY')
        self.untagged_responses = {}    # Flush old responses.
        self.is_readonly = readonly
        if readonly:
            name = 'EXAMINE'
        else:
            name = 'SELECT'
        typ, dat = self._simple_command(name, mailbox)
        if typ != 'OK':
            self.state = 'AUTH'     # Might have been 'SELECTED'
            return typ, dat
        self.state = 'SELECTED'
        if self.untagged_responses.has_key('READ-ONLY') \
                and not readonly:
            if __debug__:
                if self.debug >= 1:
                    _dump_ur(self.untagged_responses)
            raise self.readonly('%s is not writable' % mailbox)
        return typ, self.untagged_responses.get('EXISTS', [None])


    def status(self, mailbox, names):
        """Request named status conditions for mailbox.

        (typ, [data]) = <instance>.status(mailbox, names)
        """
        name = 'STATUS'
        if self.PROTOCOL_VERSION == 'IMAP4':
            raise self.error('%s unimplemented in IMAP4 (obtain IMAP4rev1 server, or re-code)' % name)
        typ, dat = self._simple_command(name, mailbox, names)
        return self._untagged_response(typ, dat, name)


    def store(self, message_set, command, flags):
        """Alters flag dispositions for messages in mailbox.

        (typ, [data]) = <instance>.store(message_set, command, flags)
        """
        if (flags[0],flags[-1]) != ('(',')'):
            flags = '(%s)' % flags  # Avoid quoting the flags
        typ, dat = self._simple_command('STORE', message_set, command, flags)
        return self._untagged_response(typ, dat, 'FETCH')


    def subscribe(self, mailbox):
        """Subscribe to new mailbox.

        (typ, [data]) = <instance>.subscribe(mailbox)
        """
        return self._simple_command('SUBSCRIBE', mailbox)


    def uid(self, command, *args):
        """Execute "command arg ..." with messages identified by UID,
                rather than message number.

        (typ, [data]) = <instance>.uid(command, arg1, arg2, ...)

        Returns response appropriate to 'command'.
        """
        command = command.upper()
        if not Commands.has_key(command):
            raise self.error("Unknown IMAP4 UID command: %s" % command)
        if self.state not in Commands[command]:
            raise self.error('command %s illegal in state %s'
                                    % (command, self.state))
        name = 'UID'
        typ, dat = apply(self._simple_command, (name, command) + args)
        if command == 'SEARCH':
            name = 'SEARCH'
        else:
            name = 'FETCH'
        return self._untagged_response(typ, dat, name)


    def unsubscribe(self, mailbox):
        """Unsubscribe from old mailbox.

        (typ, [data]) = <instance>.unsubscribe(mailbox)
        """
        return self._simple_command('UNSUBSCRIBE', mailbox)


    def xatom(self, name, *args):
        """Allow simple extension commands
                notified by server in CAPABILITY response.

        (typ, [data]) = <instance>.xatom(name, arg, ...)
        """
        if name[0] != 'X' or not name in self.capabilities:
            raise self.error('unknown extension command: %s' % name)
        return apply(self._simple_command, (name,) + args)



    #       Private methods


    def _append_untagged(self, typ, dat):

        if dat is None: dat = ''
        ur = self.untagged_responses
        if __debug__:
            if self.debug >= 5:
                _mesg('untagged_responses[%s] %s += ["%s"]' %
                        (typ, len(ur.get(typ,'')), dat))
        if ur.has_key(typ):
            ur[typ].append(dat)
        else:
            ur[typ] = [dat]


    def _check_bye(self):
        bye = self.untagged_responses.get('BYE')
        if bye:
            raise self.abort(bye[-1])


    def _command(self, name, *args):

        if self.state not in Commands[name]:
            self.literal = None
            raise self.error(
            'command %s illegal in state %s' % (name, self.state))

        for typ in ('OK', 'NO', 'BAD'):
            if self.untagged_responses.has_key(typ):
                del self.untagged_responses[typ]

        if self.untagged_responses.has_key('READ-ONLY') \
        and not self.is_readonly:
            raise self.readonly('mailbox status changed to READ-ONLY')

        tag = self._new_tag()
        data = '%s %s' % (tag, name)
        for arg in args:
            if arg is None: continue
            data = '%s %s' % (data, self._checkquote(arg))

        literal = self.literal
        if literal is not None:
            self.literal = None
            if type(literal) is type(self._command):
                literator = literal
            else:
                literator = None
                data = '%s {%s}' % (data, len(literal))

        if __debug__:
            if self.debug >= 4:
                _mesg('> %s' % data)
            else:
                _log('> %s' % data)

        try:
            self.sock.send('%s%s' % (data, CRLF))
        except socket.error, val:
            raise self.abort('socket error: %s' % val)

        if literal is None:
            return tag

        while 1:
            # Wait for continuation response

            while self._get_response():
                if self.tagged_commands[tag]:   # BAD/NO?
                    return tag

            # Send literal

            if literator:
                literal = literator(self.continuation_response)

            if __debug__:
                if self.debug >= 4:
                    _mesg('write literal size %s' % len(literal))

            try:
                self.sock.send(literal)
                self.sock.send(CRLF)
            except socket.error, val:
                raise self.abort('socket error: %s' % val)

            if not literator:
                break

        return tag


    def _command_complete(self, name, tag):
        self._check_bye()
        try:
            typ, data = self._get_tagged_response(tag)
        except self.abort, val:
            raise self.abort('command: %s => %s' % (name, val))
        except self.error, val:
            raise self.error('command: %s => %s' % (name, val))
        self._check_bye()
        if typ == 'BAD':
            raise self.error('%s command error: %s %s' % (name, typ, data))
        return typ, data


    def _get_response(self):

        # Read response and store.
        #
        # Returns None for continuation responses,
        # otherwise first response line received.

        resp = self._get_line()

        # Command completion response?

        if self._match(self.tagre, resp):
            tag = self.mo.group('tag')
            if not self.tagged_commands.has_key(tag):
                raise self.abort('unexpected tagged response: %s' % resp)

            typ = self.mo.group('type')
            dat = self.mo.group('data')
            self.tagged_commands[tag] = (typ, [dat])
        else:
            dat2 = None

            # '*' (untagged) responses?

            if not self._match(Untagged_response, resp):
                if self._match(Untagged_status, resp):
                    dat2 = self.mo.group('data2')

            if self.mo is None:
                # Only other possibility is '+' (continuation) response...

                if self._match(Continuation, resp):
                    self.continuation_response = self.mo.group('data')
                    return None     # NB: indicates continuation

                raise self.abort("unexpected response: '%s'" % resp)

            typ = self.mo.group('type')
            dat = self.mo.group('data')
            if dat is None: dat = ''        # Null untagged response
            if dat2: dat = dat + ' ' + dat2

            # Is there a literal to come?

            while self._match(Literal, dat):

                # Read literal direct from connection.

                size = int(self.mo.group('size'))
                if __debug__:
                    if self.debug >= 4:
                        _mesg('read literal size %s' % size)
                data = self.file.read(size)

                # Store response with literal as tuple

                self._append_untagged(typ, (dat, data))

                # Read trailer - possibly containing another literal

                dat = self._get_line()

            self._append_untagged(typ, dat)

        # Bracketed response information?

        if typ in ('OK', 'NO', 'BAD') and self._match(Response_code, dat):
            self._append_untagged(self.mo.group('type'), self.mo.group('data'))

        if __debug__:
            if self.debug >= 1 and typ in ('NO', 'BAD', 'BYE'):
                _mesg('%s response: %s' % (typ, dat))

        return resp


    def _get_tagged_response(self, tag):

        while 1:
            result = self.tagged_commands[tag]
            if result is not None:
                del self.tagged_commands[tag]
                return result

            # Some have reported "unexpected response" exceptions.
            # Note that ignoring them here causes loops.
            # Instead, send me details of the unexpected response and
            # I'll update the code in `_get_response()'.

            try:
                self._get_response()
            except self.abort, val:
                if __debug__:
                    if self.debug >= 1:
                        print_log()
                raise


    def _get_line(self):

        line = self.file.readline()
        if not line:
            raise self.abort('socket error: EOF')

        # Protocol mandates all lines terminated by CRLF

        line = line[:-2]
        if __debug__:
            if self.debug >= 4:
                _mesg('< %s' % line)
            else:
                _log('< %s' % line)
        return line


    def _match(self, cre, s):

        # Run compiled regular expression match method on 's'.
        # Save result, return success.

        self.mo = cre.match(s)
        if __debug__:
            if self.mo is not None and self.debug >= 5:
                _mesg("\tmatched r'%s' => %s" % (cre.pattern, `self.mo.groups()`))
        return self.mo is not None


    def _new_tag(self):

        tag = '%s%s' % (self.tagpre, self.tagnum)
        self.tagnum = self.tagnum + 1
        self.tagged_commands[tag] = None
        return tag


    def _checkquote(self, arg):

        # Must quote command args if non-alphanumeric chars present,
        # and not already quoted.

        if type(arg) is not type(''):
            return arg
        if (arg[0],arg[-1]) in (('(',')'),('"','"')):
            return arg
        if self.mustquote.search(arg) is None:
            return arg
        return self._quote(arg)


    def _quote(self, arg):

        arg = arg.replace('\\', '\\\\')
        arg = arg.replace('"', '\\"')

        return '"%s"' % arg


    def _simple_command(self, name, *args):

        return self._command_complete(name, apply(self._command, (name,) + args))


    def _untagged_response(self, typ, dat, name):

        if typ == 'NO':
            return typ, dat
        if not self.untagged_responses.has_key(name):
            return typ, [None]
        data = self.untagged_responses[name]
        if __debug__:
            if self.debug >= 5:
                _mesg('untagged_responses[%s] => %s' % (name, data))
        del self.untagged_responses[name]
        return typ, data



class _Authenticator:

    """Private class to provide en/decoding
            for base64-based authentication conversation.
    """

    def __init__(self, mechinst):
        self.mech = mechinst    # Callable object to provide/process data

    def process(self, data):
        ret = self.mech(self.decode(data))
        if ret is None:
            return '*'      # Abort conversation
        return self.encode(ret)

    def encode(self, inp):
        #
        #  Invoke binascii.b2a_base64 iteratively with
        #  short even length buffers, strip the trailing
        #  line feed from the result and append.  "Even"
        #  means a number that factors to both 6 and 8,
        #  so when it gets to the end of the 8-bit input
        #  there's no partial 6-bit output.
        #
        oup = ''
        while inp:
            if len(inp) > 48:
                t = inp[:48]
                inp = inp[48:]
            else:
                t = inp
                inp = ''
            e = binascii.b2a_base64(t)
            if e:
                oup = oup + e[:-1]
        return oup

    def decode(self, inp):
        if not inp:
            return ''
        return binascii.a2b_base64(inp)



Mon2num = {'Jan': 1, 'Feb': 2, 'Mar': 3, 'Apr': 4, 'May': 5, 'Jun': 6,
        'Jul': 7, 'Aug': 8, 'Sep': 9, 'Oct': 10, 'Nov': 11, 'Dec': 12}

def Internaldate2tuple(resp):
    """Convert IMAP4 INTERNALDATE to UT.

    Returns Python time module tuple.
    """

    mo = InternalDate.match(resp)
    if not mo:
        return None

    mon = Mon2num[mo.group('mon')]
    zonen = mo.group('zonen')

    day = int(mo.group('day'))
    year = int(mo.group('year'))
    hour = int(mo.group('hour'))
    min = int(mo.group('min'))
    sec = int(mo.group('sec'))
    zoneh = int(mo.group('zoneh'))
    zonem = int(mo.group('zonem'))

    # INTERNALDATE timezone must be subtracted to get UT

    zone = (zoneh*60 + zonem)*60
    if zonen == '-':
        zone = -zone

    tt = (year, mon, day, hour, min, sec, -1, -1, -1)

    utc = time.mktime(tt)

    # Following is necessary because the time module has no 'mkgmtime'.
    # 'mktime' assumes arg in local timezone, so adds timezone/altzone.

    lt = time.localtime(utc)
    if time.daylight and lt[-1]:
        zone = zone + time.altzone
    else:
        zone = zone + time.timezone

    return time.localtime(utc - zone)



def Int2AP(num):

    """Convert integer to A-P string representation."""

    val = ''; AP = 'ABCDEFGHIJKLMNOP'
    num = int(abs(num))
    while num:
        num, mod = divmod(num, 16)
        val = AP[mod] + val
    return val



def ParseFlags(resp):

    """Convert IMAP4 flags response to python tuple."""

    mo = Flags.match(resp)
    if not mo:
        return ()

    return tuple(mo.group('flags').split())


def Time2Internaldate(date_time):

    """Convert 'date_time' to IMAP4 INTERNALDATE representation.

    Return string in form: '"DD-Mmm-YYYY HH:MM:SS +HHMM"'
    """

    dttype = type(date_time)
    if dttype is type(1) or dttype is type(1.1):
        tt = time.localtime(date_time)
    elif dttype is type(()):
        tt = date_time
    elif dttype is type(""):
        return date_time        # Assume in correct format
    else: raise ValueError

    dt = time.strftime("%d-%b-%Y %H:%M:%S", tt)
    if dt[0] == '0':
        dt = ' ' + dt[1:]
    if time.daylight and tt[-1]:
        zone = -time.altzone
    else:
        zone = -time.timezone
    return '"' + dt + " %+02d%02d" % divmod(zone/60, 60) + '"'



if __debug__:

    def _mesg(s, secs=None):
        if secs is None:
            secs = time.time()
        tm = time.strftime('%M:%S', time.localtime(secs))
        sys.stderr.write('  %s.%02d %s\n' % (tm, (secs*100)%100, s))
        sys.stderr.flush()

    def _dump_ur(dict):
        # Dump untagged responses (in `dict').
        l = dict.items()
        if not l: return
        t = '\n\t\t'
        l = map(lambda x:'%s: "%s"' % (x[0], x[1][0] and '" "'.join(x[1]) or ''), l)
        _mesg('untagged responses dump:%s%s' % (t, t.join(l)))

    _cmd_log = []           # Last `_cmd_log_len' interactions
    _cmd_log_len = 10

    def _log(line):
        # Keep log of last `_cmd_log_len' interactions for debugging.
        if len(_cmd_log) == _cmd_log_len:
            del _cmd_log[0]
        _cmd_log.append((time.time(), line))

    def print_log():
        _mesg('last %d IMAP4 interactions:' % len(_cmd_log))
        for secs,line in _cmd_log:
            _mesg(line, secs)



if __name__ == '__main__':

    import getopt, getpass, sys

    try:
        optlist, args = getopt.getopt(sys.argv[1:], 'd:')
    except getopt.error, val:
        pass

    for opt,val in optlist:
        if opt == '-d':
            Debug = int(val)

    if not args: args = ('',)

    host = args[0]

    USER = getpass.getuser()
    PASSWD = getpass.getpass("IMAP password for %s on %s:" % (USER, host or "localhost"))

    test_mesg = 'From: %s@localhost\nSubject: IMAP4 test\n\ndata...\n' % USER
    test_seq1 = (
    ('login', (USER, PASSWD)),
    ('create', ('/tmp/xxx 1',)),
    ('rename', ('/tmp/xxx 1', '/tmp/yyy')),
    ('CREATE', ('/tmp/yyz 2',)),
    ('append', ('/tmp/yyz 2', None, None, test_mesg)),
    ('list', ('/tmp', 'yy*')),
    ('select', ('/tmp/yyz 2',)),
    ('search', (None, 'SUBJECT', 'test')),
    ('partial', ('1', 'RFC822', 1, 1024)),
    ('store', ('1', 'FLAGS', '(\Deleted)')),
    ('expunge', ()),
    ('recent', ()),
    ('close', ()),
    )

    test_seq2 = (
    ('select', ()),
    ('response',('UIDVALIDITY',)),
    ('uid', ('SEARCH', 'ALL')),
    ('response', ('EXISTS',)),
    ('append', (None, None, None, test_mesg)),
    ('recent', ()),
    ('logout', ()),
    )

    def run(cmd, args):
        _mesg('%s %s' % (cmd, args))
        typ, dat = apply(eval('M.%s' % cmd), args)
        _mesg('%s => %s %s' % (cmd, typ, dat))
        return dat

    try:
        M = IMAP4(host)
        _mesg('PROTOCOL_VERSION = %s' % M.PROTOCOL_VERSION)

        for cmd,args in test_seq1:
            run(cmd, args)

        for ml in run('list', ('/tmp/', 'yy%')):
            mo = re.match(r'.*"([^"]+)"$', ml)
            if mo: path = mo.group(1)
            else: path = ml.split()[-1]
            run('delete', (path,))

        for cmd,args in test_seq2:
            dat = run(cmd, args)

            if (cmd,args) != ('uid', ('SEARCH', 'ALL')):
                continue

            uid = dat[-1].split()
            if not uid: continue
            run('uid', ('FETCH', '%s' % uid[-1],
                    '(FLAGS INTERNALDATE RFC822.SIZE RFC822.HEADER RFC822.TEXT)'))

        print '\nAll tests OK.'

    except:
        print '\nTests failed.'

        if not Debug:
            print '''
If you would like to see debugging output,
try: %s -d5
''' % sys.argv[0]

        raise
