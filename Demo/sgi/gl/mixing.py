#! /usr/bin/env python

# Use Gouraud shading to mix colors.  Requires Z-buffer.
# It changes the color assignments so fast that you see white.
# Left button pauses, middle rotates the square.  ESC to quit.
# Experiment with a larger window (too slow) or smaller window (really white).

from GL import *
from gl import *
import DEVICE
from math import *

#
#  tekenvlak : draw a square. with bgnpolygon
#
def tekenvlak (vc) :
	bgnpolygon()
	#vcarray (vc)
	for i in vc :
		c3f (i[1])
		v3f (i[0])
	endpolygon()

#
# tekendoos : draw a box
#
def tekendoos (col) :
	v = [(-5.0,0.0,0.0),(0.0,5.0,0.0),(5.0,0.0,0.0),(0.0,-5.0,0.0)]
	vc = [(v[0],col[0]),(v[1],col[1]),(v[2],col[2]),(v[3],col[1])]
	tekenvlak (vc)

#
# initialize gl
#
def initgl () :
	#
	# open window
	#
	foreground ()
	keepaspect (1, 1)
	prefposition (100, 500, 100, 500)
	w = winopen ('PYTHON RGB')
	keepaspect (1, 1)
	winconstraints()
	#
	# configure pipeline (2buf, GOURAUD and RGBmode)
	#
	doublebuffer ()
	zbuffer (1)
	shademodel (GOURAUD)
	RGBmode ()
	gconfig ()
	#
	# set viewing
	#
	perspective (900, 1, 1.0, 10.0)
	polarview (10.0, 0, 0, 0)
	#
	# ask for the REDRAW and ESCKEY events
	#
	qdevice(DEVICE.MOUSE2)
	qdevice(DEVICE.MOUSE3)
	qdevice(DEVICE.REDRAW)
	qdevice(DEVICE.ESCKEY)


#
# the color black
#
black = 0
#
# GoForIT : use 2buf to redraw the object 2n times. index i is used as 
# the (smoothly changing) rotation angle
#
def GoForIt(i) :
	col = [(255.0,0.0,0.0), (0.0,255.0,0.0), (0.0,0.0,255.0)]
	twist = 0
	freeze = 1
	while 1 :
		if freeze <> 0 :
			col[0],col[1],col[2] = col[1],col[2],col[0]
		#
		# clear z-buffer and clear background to light-blue
		#
		zclear()
		cpack (black)
		clear()
		#
		tekendoos (col)
		#
		swapbuffers()
		#
		if qtest() <> 0 :
			dev, val = qread()
			if dev == DEVICE.ESCKEY :
				break
			elif dev == DEVICE.REDRAW :
				reshapeviewport ()
			elif dev == DEVICE.MOUSE2 and val <> 0 :
				twist = twist + 30
				perspective (900, 1, 1.0, 10.0)
				polarview (10.0, 0, 0, twist)
			elif dev == DEVICE.MOUSE3 and val <> 0 :
				freeze = 1 - freeze


# the main program
#
def main () :
	initgl ()
	GoForIt (0)

#
# exec main
#
main  ()
