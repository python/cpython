"""Suite Eudora Suite: Terms specific to Eudora
Level 1, version 1

Generated from flap:Programma's:Eudora Light
AETE/AEUT resource version 2/16, language 0, script 0
"""

import aetools
import MacOS

_code = 'CSOm'

class Eudora_Suite:

	_argmap_connect = {
		'sending' : 'eSen',
		'checking' : 'eChk',
		'waiting' : 'eIdl',
	}

	def connect(self, _no_object=None, _attributes={}, **_arguments):
		"""connect: Connect to the mail server and transfer mail
		Keyword argument sending: true to make eudora send queued messages
		Keyword argument checking: true to make eudora check for mail
		Keyword argument waiting: true to make eudora wait for idle time before checking
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'CSOm'
		_subcode = 'eCon'

		aetools.keysubst(_arguments, self._argmap_connect)
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_reply = {
		'quoting' : 'eQTx',
		'everyone' : 'eRAl',
		'self' : 'eSlf',
	}

	def reply(self, _object, _attributes={}, **_arguments):
		"""reply: Reply to a message
		Required argument: the message to reply to
		Keyword argument quoting: true if you want to quote the original text in the reply
		Keyword argument everyone: true if you want the reply to go to everyone who got the original
		Keyword argument self: true if you want the reply to go to yourself, too
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: to the reply message
		"""
		_code = 'CSOm'
		_subcode = 'eRep'

		aetools.keysubst(_arguments, self._argmap_reply)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def forward(self, _object, _attributes={}, **_arguments):
		"""forward: Forward a message
		Required argument: the message to forward
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: to the forwarded message
		"""
		_code = 'CSOm'
		_subcode = 'eFwd'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def redirect(self, _object, _attributes={}, **_arguments):
		"""redirect: Redirect a message
		Required argument: the message to redirect
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: to the redirected message
		"""
		_code = 'CSOm'
		_subcode = 'eRdr'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def send_again(self, _object, _attributes={}, **_arguments):
		"""send again: Send a message again
		Required argument: the message to send again
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: to the message sent again
		"""
		_code = 'CSOm'
		_subcode = 'eSav'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_queue = {
		'_for' : 'eWhn',
	}

	def queue(self, _object, _attributes={}, **_arguments):
		"""queue: Queue a message to be sent
		Required argument: the message to queue
		Keyword argument _for: date to send the message, in seconds since 1904, UTC
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'CSOm'
		_subcode = 'eQue'

		aetools.keysubst(_arguments, self._argmap_queue)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def unqueue(self, _object, _attributes={}, **_arguments):
		"""unqueue: Remove a message from the queue, so it won¹t be sent
		Required argument: the message to unqueue
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'CSOm'
		_subcode = 'eUnQ'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_attach_to = {
		'documents' : 'eDcl',
	}

	def attach_to(self, _object, _attributes={}, **_arguments):
		"""attach to: Attach documents to a message
		Required argument: the message to attach the documents to
		Keyword argument documents: list of documents to attach
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'CSOm'
		_subcode = 'eAtc'

		aetools.keysubst(_arguments, self._argmap_attach_to)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_start_notifying = {
		'when' : 'eWHp',
	}

	def start_notifying(self, _object, _attributes={}, **_arguments):
		"""start notifying: Notify an application of things that happen
		Required argument: an application to notify
		Keyword argument when: what to notify the application of
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'CSOm'
		_subcode = 'nIns'

		aetools.keysubst(_arguments, self._argmap_start_notifying)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_stop_notifying = {
		'when' : 'eWHp',
	}

	def stop_notifying(self, _object, _attributes={}, **_arguments):
		"""stop notifying: Stop notifying applications of things that are happening
		Required argument: an application currently being notified
		Keyword argument when: the things no longer to notify it of
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'CSOm'
		_subcode = 'nRem'

		aetools.keysubst(_arguments, self._argmap_stop_notifying)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_notice = {
		'occurrence' : 'eWHp',
		'messages' : 'eMLs',
	}

	def notice(self, _no_object=None, _attributes={}, **_arguments):
		"""notice: Eudora sends this event to notify an application that something happened
		Keyword argument occurrence: what happened
		Keyword argument messages: of the messages involved
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'CSOm'
		_subcode = 'eNot'

		aetools.keysubst(_arguments, self._argmap_notice)
		if _no_object != None: raise TypeError, 'No direct arg expected'

		aetools.enumsubst(_arguments, 'eWHp', _Enum_eNot)

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


class mail_folder(aetools.ComponentItem):
	"""mail folder - A folder containing mailboxes or other mail folders."""
	want = 'euMF'
class name(aetools.NProperty):
	"""name - the name"""
	which = 'pnam'
	want = 'itxt'
#        element 'euMB' as ['indx', 'name']
#        element 'euMF' as ['indx', 'name']

class mailbox(aetools.ComponentItem):
	"""mailbox - A mailbox."""
	want = 'euMB'
# repeated property name the name of the mail folder
class space_wasted(aetools.NProperty):
	"""space wasted - the amount of waste space in the mailbox"""
	which = 'euWS'
	want = 'long'
class space_required(aetools.NProperty):
	"""space required - the minimum amount of space required to hold the mailbox"""
	which = 'euNS'
	want = 'long'
class location(aetools.NProperty):
	"""location - the file the mailbox is stored in"""
	which = 'euFS'
	want = 'fss '
class toc_location(aetools.NProperty):
	"""toc location - the file the table of contents is stored in"""
	which = 'eTFS'
	want = 'fss '
#        element 'euMS' as ['indx']

class message(aetools.ComponentItem):
	"""message - A message"""
	want = 'euMS'
class body(aetools.NProperty):
	"""body - the body of the message"""
	which = 'eBod'
	want = 'TEXT'
class priority(aetools.NProperty):
	"""priority - the priority"""
	which = 'euPY'
	want = 'long'
class label(aetools.NProperty):
	"""label - the index of the label"""
	which = 'eLbl'
	want = 'long'
class status(aetools.NProperty):
	"""status - the message status"""
	which = 'euST'
	want = 'eSta'
class sender(aetools.NProperty):
	"""sender - the sender as appearing in the message summary"""
	which = 'euSe'
	want = 'itxt'
class date(aetools.NProperty):
	"""date - the date as appearing in the message summary"""
	which = 'euDa'
	want = 'itxt'
class subject(aetools.NProperty):
	"""subject - the subject as appearing in the message summary"""
	which = 'euSu'
	want = 'itxt'
class size(aetools.NProperty):
	"""size - the size of the message"""
	which = 'euSi'
	want = 'long'
class outgoing(aetools.NProperty):
	"""outgoing - is the message is outgoing?"""
	which = 'euOu'
	want = 'bool'
class signature(aetools.NProperty):
	"""signature - which signature the message should have"""
	which = 'eSig'
	want = 'eSig'
class QP(aetools.NProperty):
	"""QP - is Eudora allowed to encode text?"""
	which = 'eMQP'
	want = 'bool'
class return_receipt(aetools.NProperty):
	"""return receipt - is a return receipt is requested?"""
	which = 'eRRR'
	want = 'bool'
class wrap(aetools.NProperty):
	"""wrap - should the text be wrapped when sent?"""
	which = 'eWrp'
	want = 'bool'
class tab_expansion(aetools.NProperty):
	"""tab expansion - should tabs get expanded to spaces?"""
	which = 'eTab'
	want = 'bool'
class keep_copy(aetools.NProperty):
	"""keep copy - should a copy should be kept after message is sent?"""
	which = 'eCpy'
	want = 'bool'
class preserve_macintosh_info(aetools.NProperty):
	"""preserve macintosh info - should Macintosh information always be sent with attachments?"""
	which = 'eXTX'
	want = 'bool'
class attachment_encoding(aetools.NProperty):
	"""attachment encoding - the type of encoding to use for attachments"""
	which = 'eATy'
	want = 'eAty'
class show_all_headers(aetools.NProperty):
	"""show all headers - should all headers be visible?"""
	which = 'eBla'
	want = 'bool'
class transliteration_table(aetools.NProperty):
	"""transliteration table - the resource id of the transliteration table"""
	which = 'eTbl'
	want = 'long'
class will_be_fetched(aetools.NProperty):
	"""will be fetched - will the message be [re]fetched on next check?"""
	which = 'eWFh'
	want = 'bool'
class will_be_deleted(aetools.NProperty):
	"""will be deleted - will the message be deleted from server on next check?"""
	which = 'eWDl'
	want = 'bool'
#        element 'euFd' as ['name']

class field(aetools.ComponentItem):
	"""field - An RFC 822 header field in a message (field named "" is the body)"""
	want = 'euFd'

class setting(aetools.ComponentItem):
	"""setting - Eudora's settings"""
	want = 'ePrf'
mail_folder._propdict = {
	'name' : name,
}
mail_folder._elemdict = {
	'mailbox' : mailbox,
	'mail_folder' : mail_folder,
}
mailbox._propdict = {
	'name' : name,
	'space_wasted' : space_wasted,
	'space_required' : space_required,
	'location' : location,
	'toc_location' : toc_location,
}
mailbox._elemdict = {
	'message' : message,
}
message._propdict = {
	'body' : body,
	'priority' : priority,
	'label' : label,
	'status' : status,
	'sender' : sender,
	'date' : date,
	'subject' : subject,
	'size' : size,
	'outgoing' : outgoing,
	'signature' : signature,
	'QP' : QP,
	'return_receipt' : return_receipt,
	'wrap' : wrap,
	'tab_expansion' : tab_expansion,
	'keep_copy' : keep_copy,
	'preserve_macintosh_info' : preserve_macintosh_info,
	'attachment_encoding' : attachment_encoding,
	'show_all_headers' : show_all_headers,
	'transliteration_table' : transliteration_table,
	'will_be_fetched' : will_be_fetched,
	'will_be_deleted' : will_be_deleted,
}
message._elemdict = {
	'field' : field,
}
field._propdict = {
}
field._elemdict = {
}
setting._propdict = {
}
setting._elemdict = {
}
_Enum_eSta = {
	'unread' : 'euS\001',	# has not been read
	'already_read' : 'euS\002',	# has been read
	'replied' : 'euS\003',	# has been replied to
	'forwarded' : 'euS\010',	# has been forwarded
	'redirected' : 'euS\004',	# has been redirected
	'not_sendable' : 'euS\005',	# cannot be sent
	'sendable' : 'euS\006',	# can be sent
	'queued' : 'euS\007',	# queued for delivery
	'sent' : 'euS\011',	# has been sent
	'never_sent' : 'euS\012',	# never was sent
}

_Enum_eSig = {
	'none' : 'sig\000',	# no signature
	'standard' : 'sig\001',	# standard signature file
	'alternate' : 'sig\002',	# alternate signature file
}

_Enum_eAty = {
	'AppleDouble' : 'atc\000',	# AppleDouble format
	'AppleSingle' : 'atc\001',	# AppleSingle format
	'BinHex' : 'atc\002',	# BinHex format
	'uuencode' : 'atc\003',	# uuencode format
}

_Enum_eNot = {
	'mail_arrives' : 'wArv',	# mail arrival
	'mail_sent' : 'wSnt',	# mail has been sent
	'will_connect' : 'wWCn',	# eudora is about to connect to a mail server
	'has_connected' : 'wHCn',	# eudora has finished talking to a mail server
	'has_manually_filtered' : 'mFil',	# eudora has finished manually filtering messages
	'opens_filters' : 'wFil',	# user has requested Eudora open the filter window
}


#
# Indices of types declared in this module
#
_classdeclarations = {
	'euMB' : mailbox,
	'euMS' : message,
	'euMF' : mail_folder,
	'ePrf' : setting,
	'euFd' : field,
}

_propdeclarations = {
	'eWFh' : will_be_fetched,
	'euDa' : date,
	'euSi' : size,
	'eRRR' : return_receipt,
	'pnam' : name,
	'euSe' : sender,
	'eWrp' : wrap,
	'eSig' : signature,
	'euOu' : outgoing,
	'eMQP' : QP,
	'eTFS' : toc_location,
	'eWDl' : will_be_deleted,
	'eLbl' : label,
	'eATy' : attachment_encoding,
	'euSu' : subject,
	'eBla' : show_all_headers,
	'eCpy' : keep_copy,
	'euWS' : space_wasted,
	'eBod' : body,
	'euNS' : space_required,
	'eTab' : tab_expansion,
	'eTbl' : transliteration_table,
	'eXTX' : preserve_macintosh_info,
	'euFS' : location,
	'euST' : status,
	'euPY' : priority,
}

_compdeclarations = {
}

_enumdeclarations = {
	'eAty' : _Enum_eAty,
	'eNot' : _Enum_eNot,
	'eSta' : _Enum_eSta,
	'eSig' : _Enum_eSig,
}
