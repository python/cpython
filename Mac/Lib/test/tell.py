# (Slightly less) primitive operations for sending Apple Events to applications.
# This could be the basis of a Script Editor like application.

from AE import *
from AppleEvents import *
import aetools
import types

class TalkTo:
	def __init__(self, signature):
		"""Create a communication channel with a particular application.
		
		For now, the application must be given by its 4-character signature
		(because I don't know yet how to do other target types).
		"""
		if type(signature) != types.StringType or len(signature) != 4:
			raise TypeError, "signature should be 4-char string"
		self.target = AECreateDesc(typeApplSignature, signature)
		self.send_flags = kAEWaitReply
		self.send_priority = kAENormalPriority
		self.send_timeout = kAEDefaultTimeout
	def newevent(self, code, subcode, parameters = {}, attributes = {}):
		event = AECreateAppleEvent(code, subcode, self.target,
		                      kAutoGenerateReturnID, kAnyTransactionID)
		aetools.packevent(event, parameters, attributes)
		return event
	def sendevent(self, event):
		reply = event.AESend(self.send_flags, self.send_priority,
		                          self.send_timeout)
		parameters, attributes = aetools.unpackevent(reply)
		return reply, parameters, attributes
		
	def send(self, code, subcode, parameters = {}, attributes = {}):
		return self.sendevent(self.newevent(code, subcode, parameters, attributes))
	
	def activate(self):
		# Send undocumented but easily reverse engineered 'activate' command
		self.send('misc', 'actv')


# This object is equivalent to "selection" in AppleScript
# (in the core suite, if that makes a difference):
get_selection = aetools.Property('sele', None)

# Test program.  You can make it do what you want by passing parameters.
# The default gets the selection from Quill (Scriptable Text Editor).

def test(app = 'quil', suite = 'core', id = 'getd', arg = get_selection):
	t = TalkTo(app)
	t.activate()
	if arg:
		dict = {'----': arg}
	else:
		dict = {}
	reply, parameters, attributes = t.send(suite, id, dict)
	print reply, parameters
	if parameters.has_key('----'): print "returns:", str(parameters['----'])
	

test()
# So we can see it:
import sys
sys.exit(1)
