"""PythonSlave.py
An application that responds to three types of apple event: 
	'pyth'/'EXEC': 	execute direct parameter as Python
	'aevt', 'quit':	quit
	'aevt', 'odoc':	perform python scripts

Copyright © 1996, Just van Rossum, Letterror
"""

__version__ = "0.1.3"

import FrameWork
import sys
import traceback
import aetools
import string
import AE
import EasyDialogs
import os
import Qd
from Types import *
from Events import charCodeMask, cmdKey
import MacOS
import Evt

def dummyfunc(): pass

modulefilename = dummyfunc.func_code.co_filename

def Interact(timeout = 50000000):			# timeout after 10 days...
	AE.AEInteractWithUser(timeout)


class PythonSlave(FrameWork.Application):
	def __init__(self):
		FrameWork.Application.__init__(self)
		AE.AEInstallEventHandler('pyth', 'EXEC', ExecHandler)
		AE.AEInstallEventHandler('aevt', 'quit', QuitHandler)
		AE.AEInstallEventHandler('aevt', 'odoc', OpenDocumentHandler)
	
	def makeusermenus(self):
		self.filemenu = m = FrameWork.Menu(self.menubar, "File")
		self._quititem = FrameWork.MenuItem(m, "Quit", "Q", self._quit)
	
	def do_kHighLevelEvent(self, event):
		(what, message, when, where, modifiers) = event
		try:
			AE.AEProcessAppleEvent(event)
		except AE.Error, detail:
			print "Apple Event was not handled, error:", detail
	
	def do_key(self, event):
		(what, message, when, where, modifiers) = event
		c = chr(message & charCodeMask)
		if modifiers & cmdKey and c == '.':
			return
		FrameWork.Application.do_key(self, event)
	
	def idle(self, event):
		Qd.InitCursor()
	
	def quit(self, *args):
		raise self
	
	def getabouttext(self):
		return "About PythonSlaveƒ"
	
	def do_about(self, id, item, window, event):
		EasyDialogs.Message("PythonSlave " + __version__ + "\rCopyright © 1996, Letterror, JvR")
	

def ExecHandler(theAppleEvent, theReply):
	parameters, args = aetools.unpackevent(theAppleEvent)
	if parameters.has_key('----'):
		if parameters.has_key('NAME'):
			print '--- executing "' + parameters['NAME'] + '" ---'
		else:
			print '--- executing "<unknown>" ---'
		stuff = parameters['----']
		MyExec(stuff + "\n")			# execute input
		print '--- done ---'
	return 0

def MyExec(stuff):
	stuff = string.splitfields(stuff, '\r')	# convert return chars
	stuff = string.joinfields(stuff, '\n')	# to newline chars
	Interact()
	saveyield = MacOS.EnableAppswitch(1)
	try:
		exec stuff in {}
	except:
		MacOS.EnableAppswitch(saveyield)
		traceback.print_exc()
	MacOS.EnableAppswitch(saveyield)

def OpenDocumentHandler(theAppleEvent, theReply):
	parameters, args = aetools.unpackevent(theAppleEvent)
	docs = parameters['----']
	if type(docs) <> ListType:
		docs = [docs]
	for doc in docs:
		fss, a = doc.Resolve()
		path = fss.as_pathname()
		if path <> modulefilename:
			MyExecFile(path)
	return 0

def MyExecFile(path):
	saveyield = MacOS.EnableAppswitch(1)
	savewd = os.getcwd()
	os.chdir(os.path.split(path)[0])
	print '--- Executing file "' + os.path.split(path)[1] + '"'
	try:
		execfile(path, {"__name__": "__main__"})
	except:
		traceback.print_exc()
		MacOS.EnableAppswitch(saveyield)
	MacOS.EnableAppswitch(saveyield)
	os.chdir(savewd)
	print "--- done ---"

def QuitHandler(theAppleEvent, theReply):
	slave.quit()
	return 0


slave = PythonSlave()
print "PythonSlave", __version__, "ready."
slave.mainloop()
