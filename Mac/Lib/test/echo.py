"""'echo' -- an AppleEvent handler which handles all events the same.

It replies to each event by echoing the parameter back to the client.
This is a good way to find out how the Script Editor formats AppleEvents,
especially to figure out all the different forms an object specifier
can have (without having to rely on Apple's implementation).
"""

import sys
sys.stdout = sys.stderr
import traceback
import MacOS
import AE
from AppleEvents import *
import Evt
from Events import *
import Menu
import Dlg
import Win
from Windows import *
import Qd

import aetools
import EasyDialogs

kHighLevelEvent = 23				# Not defined anywhere for Python yet?

def mymessage(str):
	err = AE.AEInteractWithUser(kAEDefaultTimeout)
	if err:
		print str
	EasyDialogs.Message(str)

def main():
	echo = EchoServer()
	saveparams = MacOS.SchedParams(0, 0)	# Disable Python's own "event handling"
	try:
		echo.mainloop(everyEvent, 0)
	finally:
		apply(MacOS.SchedParams, saveparams)	# Let Python have a go at events
		echo.close()


class EchoServer:
	
	#suites = ['aevt', 'core', 'reqd']
	suites = ['****']
	
	def __init__(self):
		self.active = 0
		for suite in self.suites:
			AE.AEInstallEventHandler(suite, typeWildCard, self.aehandler)
			print (suite, typeWildCard, self.aehandler)
		self.active = 1
		self.appleid = 1
		Menu.ClearMenuBar()
		self.applemenu = applemenu = Menu.NewMenu(self.appleid, "\024")
		applemenu.AppendMenu("All about echo...;(-")
		applemenu.InsertMenu(0)
		Menu.DrawMenuBar()
	
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
		what, message, when, where, modifiers = event
		h, v = where
		if what == kHighLevelEvent:
			msg = "High Level Event: %s %s" % \
				(`code(message)`, `code(h | (v<<16))`)
			try:
				AE.AEProcessAppleEvent(event)
			except AE.Error, err:
				mymessage(msg + "\015AEProcessAppleEvent error: %s" % str(err))
				traceback.print_exc()
			else:
				mymessage(msg + "\015OK!")
		elif what == keyDown:
			c = chr(message & charCodeMask)
			if c == '.' and modifiers & cmdKey:
				raise KeyboardInterrupt, "Command-period"
			MacOS.HandleEvent(event)
		elif what == mouseDown:
			partcode, window = Win.FindWindow(where)
			if partcode == inMenuBar:
				result = Menu.MenuSelect(where)
				id = (result>>16) & 0xffff	# Hi word
				item = result & 0xffff		# Lo word
				if id == self.appleid:
					if item == 1:
						mymessage("Echo -- echo AppleEvents")
		elif what <> autoKey:
			print "Event:", (eventname(what), message, when, (h, v), modifiers)
##			MacOS.HandleEvent(event)
	
	def aehandler(self, request, reply):
		print "Apple Event!"
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
