#
# records an AIFF sample and plays it
# infinity number of times.
#

import time
import al

def recordit () :
	p = al.openport('hello', 'r')
	print 'recording...'
	buf = p.readsamps(500000)
	print 'done.'
	p.closeport()
	
	return buf

def playit (buf) :
	p = al.openport('hello', 'w')
	print 'playing...'
	p.writesamps(buf)
	while p.getfilled() > 0:
		time.sleep(0.01)
	print 'done.'
	p.closeport()

while 1 :
	playit (recordit ())
