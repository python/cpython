import getopt
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
    try:
        f = open(name, 'r')
    except:
        sys.stderr.write(name + ': cannot open\n')
        sys.exit(1)
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
    #swapbuffers()
    return tijd

def playsound(af, spkr):
    nsamp = spkr.getfillable()
    data = af.read(nsamp*2)
    spkr.writesamps(data)

def main():
	looping = 0
	packfactor = 0
	opts, args = getopt.getopt(sys.argv[1:], 'p:l')
	for opt, arg in opts:
		if opt = '-p':
			packfactor = int(eval(arg))
		elif opt = '-l':
			looping = 1
	if args:
		filename = args[0]
	else:
		filename = 'film.video'
	f, w, h, pf = openvideo(filename)
	if 0 < packfactor <> pf:
		w = w/pf*packfactor
		h = h/pf*packfactor
		pf = packfactor
	if args[1:]:
		audiofilename = args[1]
		af = open(audiofilename, 'r')
		spkr = openspkr()
		afskip = 0
		if args[2:]:
			afskip = eval(args[2])
		af.seek(afskip)
	else:
		af, spkr = None, None
	foreground()
	prefsize(w,h)
	win = winopen(filename)
	RGBmode()
	#doublebuffer()
	gconfig()
	qdevice(ESCKEY)
	qdevice(WINSHUT)
	qdevice(WINQUIT)
	running = 1
	epoch.epoch = time.millitimer()
	nframe = 0
	tijd = 1
	if looping:
		looping = f.tell()
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
				print 'Recorded at',
				print 0.1 * int(nframe * 10000.0 / tijd),
				print 'frames/sec'
			print 'Played', nframe, 'frames at',
			print 0.1 * int(nframe * 10000.0 / (t-epoch.epoch)),
			print 'frames/sec'
			if looping:
				f.seek(looping)
				epoch.epoch = time.millitimer()
				nframe = 0
				running = 1
				if af <> None:
					af.seek(afskip)
		if af <> None:
			playsound(af,spkr)
		if not running or qtest():
		    dev, val = qread()
		    if dev in (ESCKEY, WINSHUT, WINQUIT):
			raise bye
		    elif dev = REDRAW:
		    	reshapeviewport()
	except bye:
	    pass

main()
