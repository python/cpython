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
import AE
import AppleEvents
import MacOS

from aetypes import *
from aepack import pack, unpack, coerce, AEDescType

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
			if msg[0] != -1701:
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
	errarg = (errn, MacOS.GetErrorString(errn))
	if arguments.has_key('errs'):
		errarg = errarg + (arguments['errs'],)
	if arguments.has_key('erob'):
		errarg = errarg + (arguments['erob'],)
	return errarg

class TalkTo:
	"""An AE connection to an application"""
	
	def __init__(self, signature):
		"""Create a communication channel with a particular application.
		
		Addressing the application is done by specifying either a
		4-byte signature, an AEDesc or an object that will __aepack__
		to an AEDesc.
		"""
		if type(signature) == AEDescType:
			self.target = signature
		elif type(signature) == InstanceType and hasattr(signature, '__aepack__'):
			self.target = signature.__aepack__()
		elif type(signature) == StringType and len(signature) == 4:
			self.target = AE.AECreateDesc(AppleEvents.typeApplSignature, signature)
		else:
			raise TypeError, "signature should be 4-char string or AEDesc"
		self.send_flags = AppleEvents.kAEWaitReply
		self.send_priority = AppleEvents.kAENormalPriority
		self.send_timeout = AppleEvents.kAEDefaultTimeout
	
	def newevent(self, code, subcode, parameters = {}, attributes = {}):
		"""Create a complete structure for an apple event"""
		
		event = AE.AECreateAppleEvent(code, subcode, self.target,
		      	  AppleEvents.kAutoGenerateReturnID, AppleEvents.kAnyTransactionID)
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
	
	def activate(self):
		"""Send 'activate' command"""
		self.send('misc', 'actv')
	
	
# Test program
# XXXX Should test more, really...

def test():
	target = AE.AECreateDesc('sign', 'KAHL')
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
