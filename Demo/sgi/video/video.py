from gl import *
from GL import *
from DEVICE import *
import time
import sys
import al
import AL

BUFFERSIZE = 32000

class Struct(): pass
epoch = Struct()
EndOfFile = 'End of file'
bye = 'bye'

def openspkr():
    conf = al.newconfig()
    conf.setqueuesize(BUFFERSIZE)
    conf.setwidth(AL.SAMPLE_16)
    conf.setchannels(AL.MONO)
    return al.openport('spkr','w',conf)
def openvideo(name):
    f = open(name, 'r')
    line = f.readline()
    if not line: raise EndOfFile
    if line[:4] = 'CMIF': line = f.readline()
    x = eval(line[:-1])
    if len(x) = 3: w, h, pf = x
    else: w, h = x; pf = 2
    return f, w, h, pf
def loadframe(f,w,h,pf,af,spkr):
    line = f.readline()
    if line = '':
	raise EndOfFile
    x = eval(line[:-1])
    if type(x) = type(0) or type(x) = type(0.0):
    	tijd = x
    	if pf = 0:
    		size = w*h*4
    	else:
    		size = (w/pf) * (h/pf)
    else:
    	tijd, size = x
    data = f.read(size)
    if len(data) <> size:
	raise EndOfFile
    if pf:
    	rectzoom(pf, pf)
    	w = w/pf
    	h = h/pf
    	data = unpackrect(w, h, 1, data)
    lrectwrite(0,0,w-1,h-1,data)
    # This is ugly here, but the only way to get the two
    # channels started in sync
    #if af <> None:
    #	playsound(af,spkr)
    ct = time.millitimer() - epoch.epoch
    if tijd > 0 and ct < tijd:
    	time.millisleep(tijd-ct)
    swapbuffers()
    return tijd
def playsound(af, spkr):
    nsamp = spkr.getfillable()
    data = af.read(nsamp*2)
    spkr.writesamps(data)
def main():
    if len(sys.argv) > 1:
	f, w, h, pf = openvideo(sys.argv[1])
    else:
	f, w, h, pf = openvideo('film.video')
    af = None
    spkr = None
    if len(sys.argv) > 2:
	af = open(sys.argv[2], 'r')
	spkr = openspkr()
    if len(sys.argv) > 3:
	data = af.read(eval(sys.argv[3]))
	del data
    foreground()
    prefsize(w,h)
    win = winopen('Video player')
    RGBmode()
    doublebuffer()
    gconfig()
    qdevice(ESCKEY)
    running = 1
    epoch.epoch = time.millitimer()
    nframe = 0
    tijd = 1
    try:
	while 1:
	    if running:
		try:
		    tijd = loadframe(f, w, h, pf, af, spkr)
		    nframe = nframe + 1
		except EndOfFile:
		    running = 0
		    t = time.millitimer()
		    if tijd > 0:
			    print 'Recorded at ', nframe * 1000.0 / tijd,
			    print 'frames/second (', tijd, 'ms total)'
		    print 'Played at', nframe * 1000.0 / (t-epoch.epoch),
		    print 'frames/second'
	    if af <> None:
		playsound(af,spkr)
	    if qtest():
		if qread() = (ESCKEY,1):
		    raise bye
    except bye:
	pass

main()
