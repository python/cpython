"""Tools for use in AppleEvent clients and servers.

pack(x) converts a Python object to an AEDesc object
unpack(desc) does the reverse

packevent(event, parameters, attributes) sets params and attrs in an AEAppleEvent record
unpackevent(event) returns the parameters and attributes from an AEAppleEvent record

Plus...  Lots of classes and routines that help representing AE objects,
ranges, conditionals, logicals, etc., so you can write, e.g.:

	x = Character(1, Document("foobar"))

and pack(x) will create an AE object reference equivalent to AppleScript's

	character 1 of document "foobar"

Some of the stuff that appears to be exported from this module comes from other
files: the pack stuff from aepack, the objects from aetypes.

"""

from types import *
from Carbon import AE
from Carbon import AppleEvents
import MacOS
import sys

from baetypes import *
from baepack import pack, unpack, coerce, AEDescType

Error = 'baetools.Error'

# Special code to unpack an AppleEvent (which is *not* a disguised record!)
# Note by Jack: No??!? If I read the docs correctly it *is*....

aekeywords = [
	'tran',
	'rtid',
	'evcl',
	'evid',
	'addr',
	'optk',
	'timo',
	'inte',	# this attribute is read only - will be set in AESend
	'esrc',	# this attribute is read only
	'miss',	# this attribute is read only
	'from'	# new in 1.0.1
]

def missed(ae):
	try:
		desc = ae.AEGetAttributeDesc('miss', 'keyw')
	except AE.Error, msg:
		return None
	return desc.data

def unpackevent(ae):
	parameters = {}
	while 1:
		key = missed(ae)
		if not key: break
		parameters[key] = unpack(ae.AEGetParamDesc(key, '****'))
	attributes = {}
	for key in aekeywords:
		try:
			desc = ae.AEGetAttributeDesc(key, '****')
		except (AE.Error, MacOS.Error), msg:
			if msg[0] != -1701 and msg[0] != -1704:
				raise sys.exc_type, sys.exc_value
			continue
		attributes[key] = unpack(desc)
	return parameters, attributes

def packevent(ae, parameters = {}, attributes = {}):
	for key, value in parameters.items():
		ae.AEPutParamDesc(key, pack(value))
	for key, value in attributes.items():
		ae.AEPutAttributeDesc(key, pack(value))

#
# Support routine for automatically generated Suite interfaces
# These routines are also useable for the reverse function.
#
def keysubst(arguments, keydict):
	"""Replace long name keys by their 4-char counterparts, and check"""
	ok = keydict.values()
	for k in arguments.keys():
		if keydict.has_key(k):
			v = arguments[k]
			del arguments[k]
			arguments[keydict[k]] = v
		elif k != '----' and k not in ok:
			raise TypeError, 'Unknown keyword argument: %s'%k
			
def enumsubst(arguments, key, edict):
	"""Substitute a single enum keyword argument, if it occurs"""
	if not arguments.has_key(key):
		return
	v = arguments[key]
	ok = edict.values()
	if edict.has_key(v):
		arguments[key] = edict[v]
	elif not v in ok:
		raise TypeError, 'Unknown enumerator: %s'%v
		
def decodeerror(arguments):
	"""Create the 'best' argument for a raise MacOS.Error"""
	errn = arguments['errn']
	err_a1 = errn
	if arguments.has_key('errs'):
		err_a2 = arguments['errs']
	else:
		err_a2 = MacOS.GetErrorString(errn)
	if arguments.has_key('erob'):
		err_a3 = arguments['erob']
	else:
		err_a3 = None
	
	return (err_a1, err_a2, err_a3)

class TalkTo:
	"""An AE connection to an application"""
	
	def __init__(self, signature, start=0, timeout=0):
		"""Create a communication channel with a particular application.		
		Addressing the application is done by specifying either a
		4-byte signature, an AEDesc or an object that will __aepack__
		to an AEDesc.
		"""
		self.target_signature = None
		if type(signature) == AEDescType:
			self.target = signature
		elif type(signature) == InstanceType and hasattr(signature, '__aepack__'):
			self.target = signature.__aepack__()
		elif type(signature) == StringType and len(signature) == 4:
			self.target = AE.AECreateDesc(AppleEvents.typeApplSignature, signature)
			self.target_signature = signature
		else:
			raise TypeError, "signature should be 4-char string or AEDesc"
		self.send_flags = AppleEvents.kAEWaitReply
		self.send_priority = AppleEvents.kAENormalPriority
		if timeout:
			self.send_timeout = timeout
		else:
			self.send_timeout = AppleEvents.kAEDefaultTimeout
		if start:
			self.start()
		
	def start(self):
		"""Start the application, if it is not running yet"""
		self.send_flags = AppleEvents.kAENoReply
		_launch(self.target_signature)
			
	def newevent(self, code, subcode, parameters = {}, attributes = {}):
		"""Create a complete structure for an apple event"""
		event = AE.AECreateAppleEvent(code, subcode, self.target,
		      	  AppleEvents.kAutoGenerateReturnID, AppleEvents.kAnyTransactionID)
#		print parameters, attributes
		packevent(event, parameters, attributes)
		return event
	
	def sendevent(self, event):
		"""Send a pre-created appleevent, await the reply and unpack it"""
		
		reply = event.AESend(self.send_flags, self.send_priority,
		                          self.send_timeout)
		parameters, attributes = unpackevent(reply)
		return reply, parameters, attributes
		
	def send(self, code, subcode, parameters = {}, attributes = {}):
		"""Send an appleevent given code/subcode/pars/attrs and unpack the reply"""
		return self.sendevent(self.newevent(code, subcode, parameters, attributes))
	
	#
	# The following events are somehow "standard" and don't seem to appear in any
	# suite...
	#
	def activate(self):
		"""Send 'activate' command"""
		self.send('misc', 'actv')

	def _get(self, _object, as=None, _attributes={}):
		"""_get: get data from an object
		Required argument: the object
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the data
		"""
		_code = 'core'
		_subcode = 'getd'

		_arguments = {'----':_object}
		if as:
			_arguments['rtyp'] = mktype(as)

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise Error, decodeerror(_arguments)

		if _arguments.has_key('----'):
			return _arguments['----']

# Tiny Finder class, for local use only

class _miniFinder(TalkTo):
	def open(self, _object, _attributes={}, **_arguments):
		"""open: Open the specified object(s)
		Required argument: list of objects to open
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'aevt'
		_subcode = 'odoc'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']
#pass
	
_finder = _miniFinder('MACS')

def _launch(appfile):
	"""Open a file thru the finder. Specify file by name or fsspec"""
	_finder.open(_application_file(('ID  ', appfile)))


class _application_file(ComponentItem):
	"""application file - An application's file on disk"""
	want = 'appf'
	
_application_file._propdict = {
}
_application_file._elemdict = {
}
	
# Test program
# XXXX Should test more, really...

def test():
	target = AE.AECreateDesc('sign', 'quil')
	ae = AE.AECreateAppleEvent('aevt', 'oapp', target, -1, 0)
	print unpackevent(ae)
	raw_input(":")
	ae = AE.AECreateAppleEvent('core', 'getd', target, -1, 0)
	obj = Character(2, Word(1, Document(1)))
	print obj
	print repr(obj)
	packevent(ae, {'----': obj})
	params, attrs = unpackevent(ae)
	print params['----']
	raw_input(":")

if __name__ == '__main__':
	test()
	sys.exit(1)
