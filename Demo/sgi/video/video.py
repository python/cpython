import getopt
from gl import *
from GL import *
from DEVICE import *
import time
import sys
import al
import AL
import colorsys

BUFFERSIZE = 32000

class Struct(): pass
epoch = Struct()
epoch.correcttiming = 1
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
    if line[:4] = 'CMIF':
	if line[:14] = 'CMIF video 2.0':
	    line = f.readline()
	    colorinfo = eval(line[:-1])
	else:
	    colorinfo = (8,0,0,0)
	line = f.readline()
    x = eval(line[:-1])
    if len(x) = 3: w, h, pf = x
    else: w, h = x; pf = 2
    return f, w, h, pf, colorinfo

def loadframe(f,w,h,pf,af,spkr, (ybits,ibits,qbits,chrompack),mf):
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
    	w = w/pf
    	h = h/pf
    if chrompack:
	cw = (w+chrompack-1)/chrompack
	ch = (h+chrompack-1)/chrompack
	chromdata = f.read(2*cw*ch)
	rectzoom(pf*chrompack*mf,pf*chrompack*mf)
	pixmode(5,16)
	writemask(0x7ff - ((1<<ybits)-1))
	lrectwrite(0,0,cw-1,ch-1,chromdata)
	writemask((1<<ybits)-1)
	pixmode(5,8)
    if pf:
    	rectzoom(pf*mf, pf*mf)
    elif mf <> 1:
	rectzoom(mf,mf)
    lrectwrite(0,0,w-1,h-1,data)
    # This is ugly here, but the only way to get the two
    # channels started in sync
    #if af <> None:
    #	playsound(af,spkr)
    ct = time.millitimer() - epoch.epoch
    if epoch.correcttiming and tijd > 0 and ct < tijd:
    	time.millisleep(tijd-ct)
    #swapbuffers()
    return tijd

def initcmap(ybits,ibits,qbits,chrompack):
    if ybits+ibits+qbits > 11:
	raise 'Sorry, 11 bits max'
    maxy = pow(2,ybits)
    maxi = pow(2,ibits)
    maxq = pow(2,qbits)
    for i in range(2048,4096-256):
	mapcolor(i, 0, 255, 0)
    for y in range(maxy):
      yv = float(y)/float(maxy-1)
      for i in range(maxi):
	iv = (float(i)/float(maxi-1))-0.5
	for q in range(maxq):
	  qv = (float(q)/float(maxq-1))-0.5
	  index = 2048 + y + (i << ybits) + (q << (ybits+ibits))

	  rv,gv,bv = colorsys.yiq_to_rgb(yv,iv,qv)
	  r,g,b = int(rv*255.0), int(gv*255.0), int(bv*255.0)
	  if index < 4096 - 256:
	      mapcolor(index, r,g,b)

def playsound(af, spkr):
    nsamp = spkr.getfillable()
    data = af.read(nsamp*2)
    spkr.writesamps(data)

def main():
	looping = 0
	packfactor = 0
	magfactor = 1
	try:
		opts, args = getopt.getopt(sys.argv[1:], 'm:p:lF')
	except getopt.error:
		sys.stderr.write('usage: video ' + \
	'[-l] [-p pf] [-m mag] [-F] [moviefile [soundfile [skipbytes]]]\n')
		sys.exit(2)
	for opt, arg in opts:
		if opt = '-m':
			magfactor = int(eval(arg))
		elif opt = '-p':
			packfactor = int(eval(arg))
		elif opt = '-l':
			looping = 1
		elif opt = '-F':
			epoch.correcttiming = 0
	if args:
		filename = args[0]
	else:
		filename = 'film.video'
	f, w, h, pf, cinfo = openvideo(filename)
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
	prefsize(w*magfactor,h*magfactor)
	win = winopen(filename)
	if pf:
	    #doublebuffer()
	    cmode()
	else:
	    RGBmode()
	#doublebuffer()
	gconfig()
	if pf:
	    initcmap(cinfo)
	    color(2048)
	    clear()
	    writemask(2047)
	    pixmode(5,8)	# 8 bit pixels
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
			tijd = loadframe(f, w, h, pf, af, spkr, cinfo,magfactor)
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
