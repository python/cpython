"""IMAP4 client.

Based on RFC 2060.

Author: Piers Lauder <piers@cs.su.oz.au> December 1997.

Public class:		IMAP4
Public variable:	Debug
Public functions:	Internaldate2tuple
			Int2AP
			ParseFlags
			Time2Internaldate
"""

import re, socket, string, time, whrandom

#	Globals

CRLF = '\r\n'
Debug = 0
IMAP4_PORT = 143
AllowedVersions = ('IMAP4REV1', 'IMAP4')	# Most recent first

#	Commands

Commands = {
	# name		  valid states
	'APPEND':	('AUTH', 'SELECTED'),
	'AUTHENTICATE':	('NONAUTH',),
	'CAPABILITY':	('NONAUTH', 'AUTH', 'SELECTED', 'LOGOUT'),
	'CHECK':	('SELECTED',),
	'CLOSE':	('SELECTED',),
	'COPY':		('SELECTED',),
	'CREATE':	('AUTH', 'SELECTED'),
	'DELETE':	('AUTH', 'SELECTED'),
	'EXAMINE':	('AUTH', 'SELECTED'),
	'EXPUNGE':	('SELECTED',),
	'FETCH':	('SELECTED',),
	'LIST':		('AUTH', 'SELECTED'),
	'LOGIN':	('NONAUTH',),
	'LOGOUT':	('NONAUTH', 'AUTH', 'SELECTED', 'LOGOUT'),
	'LSUB':		('AUTH', 'SELECTED'),
	'NOOP':		('NONAUTH', 'AUTH', 'SELECTED', 'LOGOUT'),
	'RENAME':	('AUTH', 'SELECTED'),
	'SEARCH':	('SELECTED',),
	'SELECT':	('AUTH', 'SELECTED'),
	'STATUS':	('AUTH', 'SELECTED'),
	'STORE':	('SELECTED',),
	'SUBSCRIBE':	('AUTH', 'SELECTED'),
	'UID':		('SELECTED',),
	'UNSUBSCRIBE':	('AUTH', 'SELECTED'),
	}

#	Patterns to match server responses

Continuation = re.compile(r'\+ (?P<data>.*)')
Flags = re.compile(r'.*FLAGS \((?P<flags>[^\)]*)\)')
InternalDate = re.compile(r'.*INTERNALDATE "'
	r'(?P<day>[ 123][0-9])-(?P<mon>[A-Z][a-z][a-z])-(?P<year>[0-9][0-9][0-9][0-9])'
	r' (?P<hour>[0-9][0-9]):(?P<min>[0-9][0-9]):(?P<sec>[0-9][0-9])'
	r' (?P<zonen>[-+])(?P<zoneh>[0-9][0-9])(?P<zonem>[0-9][0-9])'
	r'"')
Literal = re.compile(r'(?P<data>.*) {(?P<size>\d+)}$')
Response_code = re.compile(r'\[(?P<type>[A-Z-]+)( (?P<data>[^\]]*))?\]')
Untagged_response = re.compile(r'\* (?P<type>[A-Z-]+) (?P<data>.*)')
Untagged_status = re.compile(r'\* (?P<data>\d+) (?P<type>[A-Z-]+)( (?P<data2>.*))?')



class IMAP4:

	"""IMAP4 client class.

	Instantiate with: IMAP4([host[, port]])

		host - host's name (default: localhost);
		port - port number (default: standard IMAP4 port).

	All IMAP4rev1 commands are supported by methods of the same
	name (in lower-case). Each command returns a tuple: (type, [data, ...])
	where 'type' is usually 'OK' or 'NO', and 'data' is either the
	text from the tagged response, or untagged results from command.

	Errors raise the exception class <instance>.error("<reason>").
	IMAP4 server errors raise <instance>.abort("<reason>"),
	which is a sub-class of 'error'.
	"""

	class error(Exception): pass	# Logical errors - debug required
	class abort(error): pass	# Service errors - close and retry


	def __init__(self, host = '', port = IMAP4_PORT):
		self.host = host
		self.port = port
		self.debug = Debug
		self.state = 'LOGOUT'
		self.tagged_commands = {}	# Tagged commands awaiting response
		self.untagged_responses = {}	# {typ: [data, ...], ...}
		self.continuation_response = ''	# Last continuation response
		self.tagnum = 0

		# Open socket to server.

		self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.sock.connect(self.host, self.port)
		self.file = self.sock.makefile('r')

		# Create unique tag for this session,
		# and compile tagged response matcher.

		self.tagpre = Int2AP(whrandom.random()*32000)
		self.tagre = re.compile(r'(?P<tag>'
				+ self.tagpre
				+ r'\d+) (?P<type>[A-Z]+) (?P<data>.*)')

		# Get server welcome message,
		# request and store CAPABILITY response.

		if __debug__ and self.debug >= 1:
			print '\tnew IMAP4 connection, tag=%s' % self.tagpre

		self.welcome = self._get_response()
		if self.untagged_responses.has_key('PREAUTH'):
			self.state = 'AUTH'
		elif self.untagged_responses.has_key('OK'):
			self.state = 'NONAUTH'
#		elif self.untagged_responses.has_key('BYE'):
		else:
			raise self.error(self.welcome)

		cap = 'CAPABILITY'
		self._simple_command(cap)
		if not self.untagged_responses.has_key(cap):
			raise self.error('no CAPABILITY response from server')
		self.capabilities = tuple(string.split(self.untagged_responses[cap][-1]))

		if __debug__ and self.debug >= 3:
			print '\tCAPABILITIES: %s' % `self.capabilities`

		self.PROTOCOL_VERSION = None
		for version in AllowedVersions:
			if not version in self.capabilities:
				continue
			self.PROTOCOL_VERSION = version
			break
		if not self.PROTOCOL_VERSION:
			raise self.error('server not IMAP4 compliant')


	def __getattr__(self, attr):
		"""Allow UPPERCASE variants of all following IMAP4 commands."""
		if Commands.has_key(attr):
			return eval("self.%s" % string.lower(attr))
		raise AttributeError("Unknown IMAP4 command: '%s'" % attr)


	#	Public methods


	def append(self, mailbox, flags, date_time, message):
		"""Append message to named mailbox.

		(typ, [data]) = <instance>.append(mailbox, flags, date_time, message)
		"""
		name = 'APPEND'
		if flags:
			flags = '(%s)' % flags
		else:
			flags = None
		if date_time:
			date_time = Time2Internaldate(date_time)
		else:
			date_time = None
		tag = self._command(name, mailbox, flags, date_time, message)
		return self._command_complete(name, tag)


	def authenticate(self, func):
		"""Authenticate command - requires response processing.

		UNIMPLEMENTED
		"""
		raise self.error('UNIMPLEMENTED')


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
			try: typ, dat = self._simple_command('CLOSE')
			except EOFError: typ, dat = None, [None]
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
		return self._untagged_response(typ, name)


	def fetch(self, message_set, message_parts):
		"""Fetch (parts of) messages.

		(typ, [data, ...]) = <instance>.fetch(message_set, message_parts)

		'data' are tuples of message part envelope and data.
		"""
		name = 'FETCH'
		typ, dat = self._simple_command(name, message_set, message_parts)
		return self._untagged_response(typ, name)


	def list(self, directory='""', pattern='*'):
		"""List mailbox names in directory matching pattern.

		(typ, [data]) = <instance>.list(directory='""', pattern='*')

		'data' is list of LIST responses.
		"""
		name = 'LIST'
		typ, dat = self._simple_command(name, directory, pattern)
		return self._untagged_response(typ, name)


	def login(self, user, password):
		"""Identify client using plaintext password.

		(typ, [data]) = <instance>.list(user, password)
		"""
		if not 'AUTH-LOGIN' in self.capabilities:
			raise self.error("server doesn't allow LOGIN authorisation")
		typ, dat = self._simple_command('LOGIN', user, password)
		if typ != 'OK':
			raise self.error(dat)
		self.state = 'AUTH'
		return typ, dat


	def logout(self):
		"""Shutdown connection to server.

		(typ, [data]) = <instance>.logout()

		Returns server 'BYE' response.
		"""
		self.state = 'LOGOUT'
		try: typ, dat = self._simple_command('LOGOUT')
		except EOFError: typ, dat = None, [None]
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
		return self._untagged_response(typ, name)


	def recent(self):
		"""Prompt server for an update.

		(typ, [data]) = <instance>.recent()

		'data' is None if no new messages,
		else value of RECENT response.
		"""
		name = 'RECENT'
		typ, dat = self._untagged_response('OK', name)
		if dat[-1]:
			return typ, dat
		typ, dat = self._simple_command('NOOP')
		return self._untagged_response(typ, name)


	def rename(self, oldmailbox, newmailbox):
		"""Rename old mailbox name to new.

		(typ, data) = <instance>.rename(oldmailbox, newmailbox)
		"""
		return self._simple_command('RENAME', oldmailbox, newmailbox)


	def response(self, code):
		"""Return data for response 'code' if received, or None.

		(code, [data]) = <instance>.response(code)
		"""
		return code, self.untagged_responses.get(code, [None])


	def search(self, charset, criteria):
		"""Search mailbox for matching messages.

		(typ, [data]) = <instance>.search(charset, criteria)

		'data' is space separated list of matching message numbers.
		"""
		name = 'SEARCH'
		if charset:
			charset = 'CHARSET ' + charset
		typ, dat = self._simple_command(name, charset, criteria)
		return self._untagged_response(typ, name)


	def select(self, mailbox='INBOX', readonly=None):
		"""Select a mailbox.

		(typ, [data]) = <instance>.select(mailbox='INBOX', readonly=None)

		'data' is count of messages in mailbox ('EXISTS' response).
		"""
		# Mandated responses are ('FLAGS', 'EXISTS', 'RECENT', 'UIDVALIDITY')
		# Remove immediately interesting responses
		for r in ('EXISTS', 'READ-WRITE'):
			if self.untagged_responses.has_key(r):
				del self.untagged_responses[r]
		if readonly:
			name = 'EXAMINE'
		else:
			name = 'SELECT'
		typ, dat = self._simple_command(name, mailbox)
		if typ == 'OK':
			self.state = 'SELECTED'
		elif typ == 'NO':
			self.state = 'AUTH'
		if not readonly and not self.untagged_responses.has_key('READ-WRITE'):
			raise self.error('%s is not writable' % mailbox)
		return typ, self.untagged_responses.get('EXISTS', [None])


	def status(self, mailbox, names):
		"""Request named status conditions for mailbox.

		(typ, [data]) = <instance>.status(mailbox, names)
		"""
		name = 'STATUS'
		typ, dat = self._simple_command(name, mailbox, names)
		return self._untagged_response(typ, name)


	def store(self, message_set, command, flag_list):
		"""Alters flag dispositions for messages in mailbox.

		(typ, [data]) = <instance>.store(message_set, command, flag_list)
		"""
		command = '%s %s' % (command, flag_list)
		typ, dat = self._simple_command('STORE', message_set, command)
		return self._untagged_response(typ, 'FETCH')


	def subscribe(self, mailbox):
		"""Subscribe to new mailbox.

		(typ, [data]) = <instance>.subscribe(mailbox)
		"""
		return self._simple_command('SUBSCRIBE', mailbox)


	def uid(self, command, args):
		"""Execute "command args" with messages identified by UID,
			rather than message number.

		(typ, [data]) = <instance>.uid(command, args)

		Returns response appropriate to 'command'.
		"""
		name = 'UID'
		typ, dat = self._simple_command('UID', command, args)
		if command == 'SEARCH':
			name = 'SEARCH'
		else:
			name = 'FETCH'
		typ, dat2 = self._untagged_response(typ, name)
		if dat2[-1]: dat = dat2
		return typ, dat


	def unsubscribe(self, mailbox):
		"""Unsubscribe from old mailbox.

		(typ, [data]) = <instance>.unsubscribe(mailbox)
		"""
		return self._simple_command('UNSUBSCRIBE', mailbox)


	def xatom(self, name, arg1=None, arg2=None):
		"""Allow simple extension commands
			notified by server in CAPABILITY response.

		(typ, [data]) = <instance>.xatom(name, arg1=None, arg2=None)
		"""
		if name[0] != 'X' or not name in self.capabilities:
			raise self.error('unknown extension command: %s' % name)
		return self._simple_command(name, arg1, arg2)



	#	Private methods


	def _append_untagged(self, typ, dat):

		if self.untagged_responses.has_key(typ):
			self.untagged_responses[typ].append(dat)
		else:
			self.untagged_responses[typ] = [dat]

		if __debug__ and self.debug >= 5:
			print '\tuntagged_responses[%s] += %.20s..' % (typ, `dat`)


	def _command(self, name, dat1=None, dat2=None, dat3=None, literal=None):

		if self.state not in Commands[name]:
			raise self.error(
			'command %s illegal in state %s' % (name, self.state))

		tag = self._new_tag()
		data = '%s %s' % (tag, name)
		for d in (dat1, dat2, dat3):
			if d is not None: data = '%s %s' % (data, d)
		if literal is not None:
			data = '%s {%s}' % (data, len(literal))

		try:
			self.sock.send('%s%s' % (data, CRLF))
		except socket.error, val:
			raise self.abort('socket error: %s' % val)

		if __debug__ and self.debug >= 4:
			print '\t> %s' % data

		if literal is None:
			return tag

		# Wait for continuation response

		while self._get_response():
			if self.tagged_commands[tag]:	# BAD/NO?
				return tag

		# Send literal

		if __debug__ and self.debug >= 4:
			print '\twrite literal size %s' % len(literal)

		try:
			self.sock.send(literal)
			self.sock.send(CRLF)
		except socket.error, val:
			raise self.abort('socket error: %s' % val)

		return tag


	def _command_complete(self, name, tag):
		try:
			typ, data = self._get_tagged_response(tag)
		except self.abort, val:
			raise self.abort('command: %s => %s' % (name, val))
		except self.error, val:
			raise self.error('command: %s => %s' % (name, val))
		if self.untagged_responses.has_key('BYE') and name != 'LOGOUT':
			raise self.abort(self.untagged_responses['BYE'][-1])
		if typ == 'BAD':
			raise self.error('%s command error: %s %s' % (name, typ, data))
		return typ, data


	def _get_response(self):

		# Read response and store.
		#
		# Returns None for continuation responses,
		# otherwise first response line received

		# Protocol mandates all lines terminated by CRLF.

		resp = self._get_line()[:-2]

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
				# Only other possibility is '+' (continuation) rsponse...

				if self._match(Continuation, resp):
					self.continuation_response = self.mo.group('data')
					return None	# NB: indicates continuation

				raise self.abort('unexpected response: %s' % resp)

			typ = self.mo.group('type')
			dat = self.mo.group('data')
			if dat2: dat = dat + ' ' + dat2

			# Is there a literal to come?

			while self._match(Literal, dat):

				# Read literal direct from connection.

				size = string.atoi(self.mo.group('size'))
				if __debug__ and self.debug >= 4:
					print '\tread literal size %s' % size
				data = self.file.read(size)

				# Store response with literal as tuple

				self._append_untagged(typ, (dat, data))

				# Read trailer - possibly containing another literal

				dat = self._get_line()[:-2]

			self._append_untagged(typ, dat)

		# Bracketed response information?

		if typ in ('OK', 'NO', 'BAD') and self._match(Response_code, dat):
			self._append_untagged(self.mo.group('type'), self.mo.group('data'))

		return resp


	def _get_tagged_response(self, tag):

		while 1:
			result = self.tagged_commands[tag]
			if result is not None:
				del self.tagged_commands[tag]
				return result
			self._get_response()


	def _get_line(self):

		line = self.file.readline()
		if not line:
			raise EOFError

		# Protocol mandates all lines terminated by CRLF

		if __debug__ and self.debug >= 4:
			print '\t< %s' % line[:-2]
		return line


	def _match(self, cre, s):

		# Run compiled regular expression match method on 's'.
		# Save result, return success.

		self.mo = cre.match(s)
		if __debug__ and self.mo is not None and self.debug >= 5:
			print "\tmatched r'%s' => %s" % (cre.pattern, `self.mo.groups()`)
		return self.mo is not None


	def _new_tag(self):

		tag = '%s%s' % (self.tagpre, self.tagnum)
		self.tagnum = self.tagnum + 1
		self.tagged_commands[tag] = None
		return tag


	def _simple_command(self, name, dat1=None, dat2=None):

		return self._command_complete(name, self._command(name, dat1, dat2))


	def _untagged_response(self, typ, name):

		if not self.untagged_responses.has_key(name):
			return typ, [None]
		data = self.untagged_responses[name]
		del self.untagged_responses[name]
		return typ, data



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

	for name in ('day', 'year', 'hour', 'min', 'sec', 'zoneh', 'zonem'):
		exec "%s = string.atoi(mo.group('%s'))" % (name, name)

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

	return tuple(string.split(mo.group('flags')))


def Time2Internaldate(date_time):

	"""Convert 'date_time' to IMAP4 INTERNALDATE representation.

	Return string in form: '"DD-Mmm-YYYY HH:MM:SS +HHMM"'
	"""

	dttype = type(date_time)
	if dttype is type(1):
		tt = time.localtime(date_time)
	elif dttype is type(()):
		tt = date_time
	elif dttype is type(""):
		return date_time	# Assume in correct format
	else: raise ValueError

	dt = time.strftime("%d-%b-%Y %H:%M:%S", tt)
	if dt[0] == '0':
		dt = ' ' + dt[1:]
	if time.daylight and tt[-1]:
		zone = -time.altzone
	else:
		zone = -time.timezone
	return '"' + dt + " %+02d%02d" % divmod(zone/60, 60) + '"'



if __debug__ and __name__ == '__main__':

	import getpass
	USER = getpass.getuser()
	PASSWD = getpass.getpass()

	test_seq1 = (
	('login', (USER, PASSWD)),
	('create', ('/tmp/xxx',)),
	('rename', ('/tmp/xxx', '/tmp/yyy')),
	('CREATE', ('/tmp/yyz',)),
	('append', ('/tmp/yyz', None, None, 'From: anon@x.y.z\n\ndata...')),
	('select', ('/tmp/yyz',)),
	('recent', ()),
	('uid', ('SEARCH', 'ALL')),
	('fetch', ('1', '(INTERNALDATE RFC822)')),
	('store', ('1', 'FLAGS', '(\Deleted)')),
	('expunge', ()),
	('close', ()),
	)

	test_seq2 = (
	('select', ()),
	('response',('UIDVALIDITY',)),
	('uid', ('SEARCH', 'ALL')),
	('recent', ()),
	('response', ('EXISTS',)),
	('logout', ()),
	)

	def run(cmd, args):
		typ, dat = apply(eval('M.%s' % cmd), args)
		print ' %s %s\n  => %s %s' % (cmd, args, typ, dat)
		return dat

	Debug = 4
	M = IMAP4("newcnri")
	print 'PROTOCOL_VERSION = %s' % M.PROTOCOL_VERSION

	for cmd,args in test_seq1:
		run(cmd, args)

	for ml in run('list', ('/tmp/', 'yy%')):
		path = string.split(ml)[-1]
		run('delete', (path,))

	for cmd,args in test_seq2:
		dat = run(cmd, args)

		if (cmd,args) != ('uid', ('SEARCH', 'ALL')):
			continue

		uid = string.split(dat[0])[-1]
		run('uid', ('FETCH',
			'%s (FLAGS INTERNALDATE RFC822.SIZE RFC822.HEADER RFC822)' % uid))
