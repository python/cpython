"""'echo' -- an AppleEvent handler which handles all events the same.

It replies to each event by echoing the parameter back to the client.
This is a good way to find out how the Script Editor formats AppleEvents,
especially to figure out all the different forms an object specifier
can have (without having to rely on Apple's implementation).
"""

import addpack
addpack.addpack('Demo')
addpack.addpack('bgen')
addpack.addpack('ae')
addpack.addpack('evt')

import sys
sys.stdout = sys.stderr
import traceback
import MacOS
import AE
from AppleEvents import *
import Evt
from Events import *
import aetools

kHighLevelEvent = 23				# Not defined anywhere for Python yet?


def main():
	echo = EchoServer()
	MacOS.EnableAppswitch(0)		# Disable Python's own "event handling"
	try:
		echo.mainloop()
	finally:
		MacOS.EnableAppswitch(1)	# Let Python have a go at events
		echo.close()


class EchoServer:
	
	suites = ['aevt', 'core']
	
	def __init__(self):
		self.active = 0
		for suite in self.suites:
			AE.AEInstallEventHandler(suite, typeWildCard, self.aehandler)
		self.active = 1
	
	def __del__(self):
		self.close()
	
	def close(self):
		if self.active:
			self.active = 0
			for suite in self.suites:
				AE.AERemoveEventHandler(suite, typeWildCard)
	
	def mainloop(self, mask = everyEvent, timeout = 60*60):
		while 1:
			self.dooneevent(mask, timeout)
	
	def dooneevent(self, mask = everyEvent, timeout = 60*60):
			got, event = Evt.WaitNextEvent(mask, timeout)
			if got:
				self.lowlevelhandler(event)
	
	def lowlevelhandler(self, event):
		what, message, when, (h, v), modifiers = event
		if what == kHighLevelEvent:
			print "High Level Event:", `code(message)`, `code(h | (v<<16))`
			try:
				AE.AEProcessAppleEvent(event)
			except AE.Error, msg:
				print "AEProcessAppleEvent error:"
				traceback.print_exc()
		elif what == keyDown:
			c = chr(message & charCodeMask)
			if c == '.' and modifiers & cmdKey:
				raise KeyboardInterrupt, "Command-period"
			MacOS.HandleEvent(event)
		elif what <> autoKey:
			print "Event:", (eventname(what), message, when, (h, v), modifiers)
			MacOS.HandleEvent(event)
	
	def aehandler(self, request, reply):
		print "Apple Event",
		parameters, attributes = aetools.unpackevent(request)
		print "class =", `attributes['evcl'].type`,
		print "id =", `attributes['evid'].type`
		print "Parameters:"
		keys = parameters.keys()
		keys.sort()
		for key in keys:
			print "%s: %.150s" % (`key`, `parameters[key]`)
			print "      :", str(parameters[key])
		print "Attributes:"
		keys = attributes.keys()
		keys.sort()
		for key in keys:
			print "%s: %.150s" % (`key`, `attributes[key]`)
		aetools.packevent(reply, parameters)


_eventnames = {
	keyDown: 'keyDown',
	autoKey: 'autoKey',
	mouseDown: 'mouseDown',
	mouseUp: 'mouseUp',
	updateEvt: 'updateEvt',
	diskEvt: 'diskEvt',
	activateEvt: 'activateEvt',
	osEvt: 'osEvt',
}

def eventname(what):
	if _eventnames.has_key(what): return _eventnames[what]
	else: return `what`

def code(x):
	"Convert a long int to the 4-character code it really is"
	s = ''
	for i in range(4):
		x, c = divmod(x, 256)
		s = chr(c) + s
	return s


if __name__ == '__main__':
	main()
