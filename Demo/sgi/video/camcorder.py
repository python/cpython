#!/ufs/guido/bin/sgi/python3.3
from gl import *
from GL import *
from DEVICE import *
import time
import sys
import getopt
import socket
import posix
import vtime

SYNCPORT = 10000
CTLPORT = 10001

VP_GBXORG = 0x1000001
VP_GBYORG = 0x1000002
VP_FBXORG = 0x1000003
VP_FBYORG = 0x1000004
VP_WIDTH =  0x1000005
VP_HEIGHT = 0x1000006

class Struct(): pass
epoch = Struct()

def getvideosize():
    w = getvideo(VP_WIDTH)
    h = getvideo(VP_HEIGHT)
    print getvideo(VP_GBXORG), getvideo(VP_GBYORG)
    print getvideo(VP_FBXORG), getvideo(VP_FBYORG)
    x = 0
    y = 0
    return x,y,w,h
def saveframe(f,x,y,w,h,pf, notime):
    readsource(SRC_FRONT)
    if pf:
    	w = w/pf*pf
    	h = h/pf*pf
    data = None
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
def main():
    foreground()
    pf = 2
    ausync = 0
    austart = 0
    optlist, args = getopt.getopt(sys.argv[1:],'ca:s')
    for opt, arg in optlist:
	if opt = '-c':
	    pf = 0
	elif opt = '-a':
	    ausync = 1
	    aumachine = arg
	elif opt = '-s':
	    austart = 1
	else:
	    print 'Usage: camcorder [-c] [-a audiomachine [-s]]'
	    sys.exit(1)
    if austart:
	if not ausync:
	    print 'Cannot use -s without -a'
	    sys.exit(1)
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
    prefsize(w,h)
    win = winopen('Camcorder')
    if len(args) > 1:
        f = open(args, 'w')
    else:
        f = open('film.video', 'w')
    w, h = getsize()
    realw, realh = w, h
    doublebuffer()
    RGBmode()
    gconfig()
    qdevice(LEFTMOUSE)
    qdevice(RKEY)
    qdevice(SKEY)
    qdevice(CKEY)
    qdevice(PKEY)
    qdevice(ESCKEY)
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
    drawframe(x,y,w,h,1)
    nframe = 0
    try:
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
	    if qtest():
		ev, val = qread()
		if ev = LEFTMOUSE and val = 1:
		    drawframe(x,y,w,h,0)
		    mousing = 1
		    ox, oy = getorigin()
		    x = getvaluator(MOUSEX)-ox
		    y = getvaluator(MOUSEY)-oy
		elif ev = LEFTMOUSE and val = 0:
		    mousing = 0
		    if not sizewritten:
		    	f.write('CMIF video 1.0\n')
			f.write(`w,h,pf` + '\n')
			sizewritten = 1
		if ev = RKEY and val = 1:
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
		elif ev = PKEY and val = 1 and outrunning:
		    outrunning = 0
		    stoptime = time.millitimer()
		    if ausync:
			ctl.sendto(`(0,stoptime)`, aua)
		    nf =  nframe * 1000.0 / (time.millitimer()-starttime)
		    drawmode(OVERDRAW)
		    color(0)
		    clear()
		    color(1)
		    cmov2i(5,5)
		    charstr('Recorded ' + `nf` + ' frames/sec')
		    drawmode(NORMALDRAW)
		elif ev = PKEY and val = 1 and not outrunning:
			outsingle = 1
		elif ev = CKEY and val = 1:
			inrunning = 1
		elif ev = SKEY and val = 1:
			if outrunning:
			    ringbell()
			elif inrunning:
			    inrunning = 0
			else:
			    insingle = 1
		elif ev = ESCKEY:
		    if ausync:
			ctl.sendto(`(2,time.millitimer())`, aua)
		    raise stop
	    if inrunning or insingle:
		rectcopy(vidx,vidy,realw,realh,0,0)
	        swapbuffers()
	    if outrunning or outsingle:
		nframe = nframe + 1
		if not sizewritten:
		    f.write('CMIF video 1.0\n')
		    f.write(`w,h,pf` + '\n')
		    sizewritten = 1
		saveframe(f, x, y, w, h, pf, outsingle)
    except stop:
	pass
    drawmode(OVERDRAW)
    color(0)
    clear()
#
main()
