# The oldest AppleEvent test program.
# Its function has been overtaken by echo.py and tell.py.

import AE
from AppleEvents import *
import Evt
from Events import *
import struct
import aetools
import macfs
import sys
import MacOS

MacOS.SchedParams(1, 0)

def aehandler(request, reply):
	tosend = []
	print 'request:', aetools.unpackevent(request)
	param = request.AEGetParamDesc(keyDirectObject, typeWildCard)
	if param.type == typeAEList:
		n = param.AECountItems()
		print 'List has', n, 'items'
		for i in range(1, 1+n):
			type, item = param.AEGetNthDesc(i, typeFSS)
			data = item.data
			print 'item', i, ':', type, item.type, len(data), 'bytes'
			vol, dir, fnlen = struct.unpack('hlb', data[:7])
			filename = data[7:7+fnlen]
			print 'vol:', vol, '; dir:', dir, '; filename:', `filename`
			print 'full path:', macfs.FSSpec((vol,dir,filename)).as_pathname()
			tosend.append(item)
	else:
		pass
		print 'param:', (param.type, param.data[:20]), param.data[20:] and '...'
	if tosend:
		passtothink(tosend)


def passtothink(list):
	target = AE.AECreateDesc(typeApplSignature, 'KAHL')
	event = AE.AECreateAppleEvent(kCoreEventClass,
	                              kAEOpenDocuments,
	                              target,
	                              kAutoGenerateReturnID,
	                              kAnyTransactionID)
	aetools.packevent(event, {keyDirectObject: list})
	reply = event.AESend(kAENoReply | kAEAlwaysInteract | kAECanSwitchLayer,
	                     kAENormalPriority,
	                     kAEDefaultTimeout)
	#print "Reply:", aetools.unpackevent(reply)
	return
	event = AE.AECreateAppleEvent(kCoreEventClass,
	                              kAEOpenApplication,
	                              target,
	                              kAutoGenerateReturnID,
	                              kAnyTransactionID)
	reply = event.AESend(kAENoReply | kAEAlwaysInteract | kAECanSwitchLayer,
	                     kAENormalPriority,
	                     kAEDefaultTimeout)

def unihandler(req, rep):
	print 'unihandler'
	aehandler(req, rep)

quit = 0
def quithandler(req, rep):
	global quit
	quit = 1

def corehandler(req, rep):
	print 'core event!'
	parameters, attributes = aetools.unpackevent(req)
	print "event class =", attributes['evcl']
	print "event id =", attributes['evid']
	print 'parameters:', parameters
	# echo the arguments, to see how Script Editor formats them
	aetools.packevent(rep, parameters)

def wildhandler(req, rep):
	print 'wildcard event!'
	parameters, attributes = aetools.unpackevent(req)
	print "event class =", attributes['evcl']
	print "event id =", attributes['evid']
	print 'parameters:', parameters

AE.AEInstallEventHandler(typeAppleEvent, kAEOpenApplication, aehandler)
AE.AEInstallEventHandler(typeAppleEvent, kAEOpenDocuments, aehandler)
AE.AEInstallEventHandler(typeAppleEvent, kAEPrintDocuments, aehandler)
AE.AEInstallEventHandler(typeAppleEvent, kAEQuitApplication, quithandler)
AE.AEInstallEventHandler(typeAppleEvent, typeWildCard, unihandler)
AE.AEInstallEventHandler('core', typeWildCard, corehandler)
#AE.AEInstallEventHandler(typeWildCard, typeWildCard, wildhandler)


def main():
	global quit
	quit = 0
	while not quit:
		ok, e = Evt.WaitNextEvent(-1, 60)
		if ok:
			print 'Event:', e
			if e[0] == 23: # kHighLevelEvent
				AE.AEProcessAppleEvent(e)
			elif e[0] == keyDown and chr(e[1]&0xff) == '.' and e[4]&cmdKey:
				raise KeyboardInterrupt, "Command-Period"
			else:
				MacOS.HandleEvent(e)

if __name__ == '__main__':
	main()

print "This module is obsolete -- use echo.py or tell.py ..."
