#
# Simple test program for ctb module: emulate a terminal.
#
import ctb
import macconsole
import sys

def cb(err):
	print 'Done, err=', err

def main():
	if not ctb.available():
		print 'Communications Toolbox not available'
		sys.exit(1)
#	c = macconsole.copen('Terminal window')
	print 'Minimal terminal emulator V1.0'
	print '(type @ to exit)'
	print
	c = macconsole.fopen(sys.stdin)
	f = sys.stdin
	c.setmode(macconsole.C_RAW)
	
	l = ctb.CMNew('Serial Tool', None)
	l.Open(0)
	
	while 1:
		l.Idle()
		d = f.read(1)
		if d == '@':
			break
		if d:
			l.Write(d, ctb.cmData, -1, 0)
		l.Idle()
		d, dummy = l.Read(1000, ctb.cmData, 0)
		if d:
			f.write(d)
			f.flush()
	l.Close(-1, 1)
	del l
			
main()