"""Suite Eudora Suite: Terms specific to Eudora
Level 1, version 1

Generated from Moes:Programma's:Eudora:Eudora Light
AETE/AEUT resource version 2/16, language 0, script 0
"""

import addpack
addpack.addpack('Tools')
addpack.addpack('bgen')
addpack.addpack('ae')

import aetools
import MacOS

_code = 'CSOm'

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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def unqueue(self, _object, _attributes={}, **_arguments):
		"""unqueue: Remove a message from the queue, so it wonÍt be sent
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


#    Class 'mail folder' ('euMF') -- 'A folder containing mailboxes or other mail folders.'
#        property 'name' ('pnam') 'itxt' -- 'the name' []
#        element 'euMB' as ['indx', 'name']
#        element 'euMF' as ['indx', 'name']

#    Class 'mailbox' ('euMB') -- 'A mailbox.'
#        property 'name' ('pnam') 'itxt' -- 'the name of the mail folder' []
#        property 'space wasted' ('euWS') 'long' -- 'the amount of waste space in the mailbox' []
#        property 'space required' ('euNS') 'long' -- 'the minimum amount of space required to hold the mailbox' []
#        property 'location' ('euFS') 'fss ' -- 'the file the mailbox is stored in' []
#        property 'toc location' ('eTFS') 'fss ' -- 'the file the table of contents is stored in' []
#        element 'euMS' as ['indx']

#    Class 'message' ('euMS') -- 'A message'
#        property 'body' ('eBod') 'TEXT' -- 'the body of the message' [mutable]
#        property 'priority' ('euPY') 'long' -- 'the priority' [mutable]
#        property 'label' ('eLbl') 'long' -- 'the index of the label' [mutable]
#        property 'status' ('euST') 'eSta' -- 'the message status' [mutable enum]
#        property 'sender' ('euSe') 'itxt' -- 'the sender as appearing in the message summary' [mutable]
#        property 'date' ('euDa') 'itxt' -- 'the date as appearing in the message summary' []
#        property 'subject' ('euSu') 'itxt' -- 'the subject as appearing in the message summary' [mutable]
#        property 'size' ('euSi') 'long' -- 'the size of the message' []
#        property 'outgoing' ('euOu') 'bool' -- 'is the message is outgoing?' []
#        property 'signature' ('eSig') 'eSig' -- 'which signature the message should have' [mutable enum]
#        property 'QP' ('eMQP') 'bool' -- 'is Eudora allowed to encode text?' [mutable]
#        property 'return receipt' ('eRRR') 'bool' -- 'is a return receipt is requested?' [mutable]
#        property 'wrap' ('eWrp') 'bool' -- 'should the text be wrapped when sent?' [mutable]
#        property 'tab expansion' ('eTab') 'bool' -- 'should tabs get expanded to spaces?' [mutable]
#        property 'keep copy' ('eCpy') 'bool' -- 'should a copy should be kept after message is sent?' [mutable]
#        property 'preserve macintosh info' ('eXTX') 'bool' -- 'should Macintosh information always be sent with attachments?' [mutable]
#        property 'attachment encoding' ('eATy') 'eAty' -- 'the type of encoding to use for attachments' [mutable enum]
#        property 'show all headers' ('eBla') 'bool' -- 'should all headers be visible?' [mutable]
#        property 'transliteration table' ('eTbl') 'long' -- 'the resource id of the transliteration table' [mutable]
#        property 'will be fetched' ('eWFh') 'bool' -- 'will the message be [re]fetched on next check?' [mutable]
#        property 'will be deleted' ('eWDl') 'bool' -- 'will the message be deleted from server on next check?' [mutable]
#        element 'euFd' as ['name']

#    Class 'field' ('euFd') -- 'An RFC 822 header field in a message (field named "" is the body)'

#    Class 'setting' ('ePrf') -- "Eudora's settings"
