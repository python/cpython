"""AEservertest - Test AppleEvent server interface

(adapted from Guido's 'echo' program).

Build an applet from this source, and include the aete resource that you
want to test. Use the AEservertest script to try things.
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
import macfs

import aetools
import EasyDialogs

kHighLevelEvent = 23				# Not defined anywhere for Python yet?

Quit='Quit'

def mymessage(str):
	err = AE.AEInteractWithUser(kAEDefaultTimeout)
	if err:
		print str
	EasyDialogs.Message(str)

def myaskstring(str, default=''):
	err = AE.AEInteractWithUser(kAEDefaultTimeout)
	if err:
		return default
	return EasyDialogs.AskString(str, default)

def main():
	echo = EchoServer()
	savepars = MacOS.SchedParams(0, 0)		# Disable Python's own "event handling"
	try:
		try:
			echo.mainloop(everyEvent, 0)
		except Quit:
			pass
	finally:
		apply(MacOS.SchedParams, savepars)	# Let Python have a go at events
		echo.close()


class EchoServer:
	
	suites = ['aevt', 'core', 'reqd']
	
	def __init__(self):
		self.active = 0
		#
		# Install the handlers
		#
		for suite in self.suites:
			AE.AEInstallEventHandler(suite, typeWildCard, self.aehandler)
			print (suite, typeWildCard, self.aehandler)
		self.active = 1
		#
		# Setup the apple menu and file/quit
		#
		self.appleid = 1
		self.fileid = 2
		
		Menu.ClearMenuBar()
		self.applemenu = applemenu = Menu.NewMenu(self.appleid, "\024")
		applemenu.AppendMenu("All about echo...;(-")
		applemenu.InsertMenu(0)
		
		self.filemenu = Menu.NewMenu(self.fileid, 'File')
		self.filemenu.AppendMenu("Quit/Q")
		self.filemenu.InsertMenu(0)
		
		Menu.DrawMenuBar()
		#
		# Set interaction allowed (for the return values)
		#
		AE.AESetInteractionAllowed(kAEInteractWithAll)
	
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
			self.handled_by_us = 0
			try:
				AE.AEProcessAppleEvent(event)
			except AE.Error, err:
				mymessage(msg + "\015AEProcessAppleEvent error: %s" % str(err))
				traceback.print_exc()
			else:
				if self.handled_by_us == 0:
					print msg, "Handled by AE, somehow"
				else:
					print msg, 'Handled by us.'
		elif what == keyDown:
			c = chr(message & charCodeMask)
			if modifiers & cmdKey:
				if c == '.':
					raise KeyboardInterrupt, "Command-period"
				else:
					self.menuhit(Menu.MenuKey(message&charCodeMask))
			##MacOS.HandleEvent(event)
		elif what == mouseDown:
			partcode, window = Win.FindWindow(where)
			if partcode == inMenuBar:
				result = Menu.MenuSelect(where)
				self.menuhit(result)
		elif what <> autoKey:
			print "Event:", (eventname(what), message, when, (h, v), modifiers)
##			MacOS.HandleEvent(event)

	def menuhit(self, result):
			id = (result>>16) & 0xffff	# Hi word
			item = result & 0xffff		# Lo word
			if id == self.appleid:
				if item == 1:
					mymessage("Echo -- echo AppleEvents")
			elif id == self.fileid:
				if item == 1:
					raise Quit
	
	def aehandler(self, request, reply):
		print "Apple Event!"
		self.handled_by_us = 1
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
		parameters['----'] = self.askreplyvalue()
		aetools.packevent(reply, parameters)
		
	def askreplyvalue(self):
		while 1:
			str = myaskstring('Reply value to send (python-style)', 'None')
			try:
				rv = eval(str)
				break
			except:
				pass
		return rv
		
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
