from gl import *
from GL import *
from DEVICE import *
import time
import sys
import getopt
import socket
import posix
import vtime

# Preallocation parameter
PREALLOC = 4 # Megabyte

# Sync audio parameters
SYNCPORT = 10000
CTLPORT = 10001

from vpregs import *

class Struct(): pass
epoch = Struct()

def getvideosize():
    w = getvideo(VP_WIDTH)
    h = getvideo(VP_HEIGHT)
    print 'WIDTH,HEIGHT:', w, h
    print 'GB{X,Y}ORG:', getvideo(VP_GBXORG), getvideo(VP_GBYORG)
    print 'FB{X,Y}ORG:',  getvideo(VP_FBXORG), getvideo(VP_FBYORG)
    x = 0
    y = 0
    return x,y,w,h

framelist = []

def prealloc(w, h):
	nbytes = w*h*4
	limit = PREALLOC*1024*1024
	total = 0
	list = []
	print 'Prealloc to', PREALLOC, 'Megabytes...'
	while total+nbytes <=  limit:
		list.append('x'*nbytes)
		total = total + nbytes
	print 'Done.'

def grabframe(f,x,y,w,h,pf):
    readsource(SRC_FRONT)
    if pf:
    	w = w/pf*pf
    	h = h/pf*pf
    data = lrectread(x,y,x+w-1,y+h-1)
    t = time.millitimer()-epoch.epoch
    framelist.append(data, t)
    readsource(SRC_FRAMEGRABBER)

def saveframes(f, w, h, pf):
	for data, t in framelist:
		if pf:
		    	w = w/pf*pf
		    	h = h/pf*pf
			data = packrect(w,h,pf,data)
		f.write(`t` + ',' + `len(data)` + '\n')
		f.write(data)
	framelist[:] = []

def saveframe(f,x,y,w,h,pf, notime):
    readsource(SRC_FRONT)
    if pf:
    	w = w/pf*pf
    	h = h/pf*pf
    data = lrectread(x,y,x+w-1,y+h-1)
    if pf: data = packrect(w,h,pf,data)
    if notime: t = 0
    else: t = time.millitimer()-epoch.epoch
    f.write(`t` + ',' + `len(data)` + '\n')
    f.write(data)
    readsource(SRC_FRAMEGRABBER)

def drawframe(x,y,w,h,col):
    drawmode(OVERDRAW)
    color(col)
    bgnline()
    v2i(x-1,y-1) ; v2i(x+w,y-1); v2i(x+w,y+h); v2i(x-1,y+h); v2i(x-1,y-1)
    endline()
    drawmode(NORMALDRAW)

def usage():
    sys.stderr.write('Usage: camcorder ' + \
	'[-c] [-p packfactor] [-a audiomachine [-s]] [outputfile]\n')
    sys.exit(2)

def wrheader(f, w, h, pf):
    	f.write('CMIF video 1.0\n')
	f.write(`w,h,pf` + '\n')
	print 'width,height,pf:', w, h, pf,
	if pf == 0: pf = 4
	print '(i.e.,', w*h*pf, 'bytes/frame)'

def main():
    foreground()
    pf = 2
    ausync = 0
    austart = 0
    optlist, args = getopt.getopt(sys.argv[1:],'ca:sp:')
    for opt, arg in optlist:
	if opt == '-c':
	    pf = 0
	elif opt == '-a':
	    ausync = 1
	    aumachine = arg
	elif opt == '-s':
	    austart = 1
	elif opt == '-p':
	    pf = int(eval(arg))
	else:
	    usage()
    if args:
    	if len(args) > 1:
    	    print 'Too many arguments'
    	    usage()
    	filename = args[0]
    else:
    	filename = 'film.video'
    if austart:
	if not ausync:
	    print 'Cannot use -s without -a'
	    usage()
	print 'Starting audio recorder...'
	posix.system('rsh '+aumachine+' syncrecord '+socket.gethostname()+' &')
    if ausync:
	print 'Syncing to audio recorder...'
	globtime = vtime.VTime().init(1,aumachine,SYNCPORT)
	ctl = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
	ctl.bind((socket.gethostname(),CTLPORT))
	aua = (socket.gethostbyname(aumachine), CTLPORT)
	print 'Done.'
    vidx, vidy, w, h = getvideosize()
    #prefsize(w,h)
    winx, winy = 1280-w-10, 1024-h-30
    prefposition(winx,winx+w-1,winy,winy+h-1)
    win = winopen(filename)
    f = open(filename, 'w')
    w, h = getsize()
    realw, realh = w, h
    ####doublebuffer()
    RGBmode()
    gconfig()
    qdevice(LEFTMOUSE)
    qdevice(RKEY)
    qdevice(SKEY)
    qdevice(CKEY)
    qdevice(PKEY)
    qdevice(ESCKEY)
    qdevice(WINQUIT)
    qdevice(WINSHUT)
    inrunning = 1
    outrunning = 0
    stop = 'stop'
    readsource(SRC_FRAMEGRABBER)
    mousing = 0
    epoch.epoch = time.millitimer()
    stoptime = epoch.epoch
    sizewritten = 0
    x, y = realw/4, realh/4
    w, h = w/2, h/2
    prealloc(w, h)
    try:
	drawframe(x,y,w,h,1)
	nframe = 0
	num = 0
	while 1:
	    insingle = 0
	    outsingle = 0
	    if mousing:
		drawframe(x,y,w,h,0)
		ox, oy = getorigin()
		if sizewritten:
		    x = getvaluator(MOUSEX)-ox
		    y = getvaluator(MOUSEY)-oy
		else:
		    w = getvaluator(MOUSEX)-x-ox
		    h = getvaluator(MOUSEY)-y-oy
		drawframe(x,y,w,h,1)
	    if qtest() or \
	    not (mousing or inrunning or insingle or outrunning or outsingle):
		ev, val = qread()
		if ev == LEFTMOUSE and val == 1:
		    drawframe(x,y,w,h,0)
		    mousing = 1
		    ox, oy = getorigin()
		    x = getvaluator(MOUSEX)-ox
		    y = getvaluator(MOUSEY)-oy
		elif ev == LEFTMOUSE and val == 0:
		    if h < 0:
		    	y, h = y+h, -h
		    if w < 0:
		    	x, w = x+w, -w
		    mousing = 0
		    if not sizewritten:
		    	wrheader(f, w, h, pf)
			sizewritten = 1
			prealloc(w, h)
		elif ev == RKEY and val == 1:
		    if not inrunning:
			ringbell()
		    else:
			outrunning = 1
			wasstopped = time.millitimer() - stoptime
			epoch.epoch = epoch.epoch + wasstopped
			nframe = 0
			starttime = time.millitimer()
			if ausync:
			    ctl.sendto(`(1,starttime)`, aua)
		elif ev == PKEY and val == 1 and outrunning:
		    outrunning = 0
		    stoptime = time.millitimer()
		    if ausync:
			ctl.sendto(`(0,stoptime)`, aua)
		    fps =  nframe * 1000.0 / (time.millitimer()-starttime)
		    print 'Recorded', nframe,
		    print 'frames at', 0.1*int(fps*10),'frames/sec'
		    print 'Saving...'
		    saveframes(f, w, h, pf)
		    print 'Done.'
		elif ev == PKEY and val == 1 and not outrunning:
			outsingle = 1
		elif ev == CKEY and val == 1:
			inrunning = 1
		elif ev == SKEY and val == 1:
			if outrunning:
			    ringbell()
			elif inrunning:
			    inrunning = 0
			else:
			    insingle = 1
		elif ev in (ESCKEY, WINQUIT, WINSHUT):
		    if ausync:
			ctl.sendto(`(2,time.millitimer())`, aua)
		    raise stop
		elif ev == REDRAW:
			drawframe(x,y,w,h,0)
			reshapeviewport()
			drawframe(x,y,w,h,1)
	    if inrunning or insingle:
	    	if outrunning:
			rectcopy(vidx+x,vidy+y,vidx+x+w-1,vidy+y+h-1,x,y)
	    	else:
			rectcopy(vidx,vidy,vidx+realw-1,vidx+realh-1,0,0)
	        ####swapbuffers()
	    if outrunning or outsingle:
		nframe = nframe + 1
		if not sizewritten:
		    wrheader(f, w, h, pf)
		    sizewritten = 1
		if outrunning:
			grabframe(f, x, y, w, h, pf)
		else:
			saveframe(f, x, y, w, h, pf, outsingle)
    except stop:
	pass
    finally:
        drawmode(OVERDRAW)
        color(0)
        clear()

main()
