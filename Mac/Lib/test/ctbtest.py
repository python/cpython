#
# Simple test program for ctb module: emulate a terminal.
# To simplify matters use the python console window for output.
#
import ctb
import Evt
import Events
import MacOS
import sys

def cb(err):
	print 'Done, err=', err

def main():
	if not ctb.available():
		print 'Communications Toolbox not available'
		sys.exit(1)
	# Disable Python's event processing (we do that)
	MacOS.SchedParams(1, 0)
	print 'Minimal terminal emulator V1.0'
	print '(type command-Q to exit)'
	print
	
	l = ctb.CMNew('Serial Tool', None)
	l.Open(10)
	l.SetConfig(l.GetConfig() + ' baud 4800')
	
	while 1:
		l.Idle()	# Give time to ctb
		
		ok, evt = Evt.WaitNextEvent(0xffff, 0)
		if ok:
			what, message, when, where, modifiers = evt
			
			if what == Events.keyDown:
				# It is ours. Check for command-. to terminate
				ch = chr(message & Events.charCodeMask)
				if ch == 'q' and (modifiers & Events.cmdKey):
					break
				l.Write(ch, ctb.cmData, -1, 0)
		d, dummy = l.Read(1000, ctb.cmData, 1)
		if d:
			for ch in d:
				if ch != '\r':
					sys.stdout.write(ch)
			sys.stdout.flush()
	l.Close(-1, 1)
	del l
			
main()
