#! /usr/bin/env python

#   zrgb  (Requires Z buffer.)
#
# This program demostrates zbuffering 3 intersecting RGB polygons while
# in doublebuffer mode where, movement of the mouse with the LEFTMOUSE 
# button depressed will, rotate the 3 polygons. This is done by compound
# rotations allowing continuous screen-oriented rotations. 
#
#    Press the "Esc" key to exit.  

from gl import *
from GL import *
from DEVICE import *


idmat=[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]

def main() :
	#
	# old and new mouse position
	#
	#
	mode = 0
	omx = 0
	mx = 0
	omy = 0
	my = 0
	#
	objmat=[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]
	#
	initialize ()
	#
	draw_scene (objmat)
	#
	while (1) :
		#
		dev, val = qread()
		#
		if dev == ESCKEY :
			if val :
				break	
			# exit when key is going up, not down
			# this avoids the scenario where a window 
			# underneath this program's window
			# would otherwise "eat up" the up-
			# event of the Esc key being released
			return        
			#
		elif dev == REDRAW :
			reshapeviewport()
			draw_scene(objmat)
			#
		elif dev == LEFTMOUSE:
			omx = mx
			omy = my
			if val :
				mode = 1
			else :
				mode = 0
		elif dev == MOUSEX :
			omx = mx
			mx = val
			#print omx, mx
			objmat = update_scene(objmat,mx,my,omx,omy,mode)
			#
		elif dev == MOUSEY :
			omy = my
			my = val
			#print omy, my
			objmat = update_scene(objmat,mx,my,omx,omy,mode)
			#


def initialize () :
	#
	foreground ()
	keepaspect(5, 4)
	w = winopen('Zbuffered RGB')
	#
	doublebuffer()
	RGBmode()
	gconfig()
	zbuffer(1)
	lsetdepth(0x0, 0x7FFFFF)
	#
	qdevice(ESCKEY)
	qdevice(LEFTMOUSE)
	qdevice(MOUSEX)
	qdevice(MOUSEY)

def update_scene (mat, mx, my, omx, omy, mode) :
	#
	if mode == 1 :
		mat = orient(mat, mx, my, omx, omy)
		draw_scene(mat)
	return mat

def orient (mat, mx, my, omx, omy) :
	#
	#
	pushmatrix()
	loadmatrix(idmat)
	#
	if mx - omx : rot (float (mx - omx), 'y')
	if omy - my : rot (float (omy - my), 'x')
	#
	multmatrix(mat)
	mat = getmatrix()
	#
	popmatrix()
	#
	return mat

def draw_scene (mat) :
	RGBcolor(40, 100, 200)
	clear()
	zclear()
	#
	perspective(400, 1.25, 30.0, 60.0)
	translate(0.0, 0.0, -40.0)
	multmatrix(mat)
	#
	# skews original view to show all polygons
	#
	rotate(-580, 'y')
	draw_polys()
	#
	swapbuffers()

polygon1 = [(-10.0,-10.0,0.0),(10.0,-10.0,0.0),(-10.0,10.0,0.0)]

polygon2 = [(0.0,-10.0,-10.0),(0.0,-10.0,10.0),(0.0,5.0,-10.0)]

polygon3 = [(-10.0,6.0,4.0),(-10.0,3.0,4.0),(4.0,-9.0,-10.0),(4.0,-6.0,-10.0)]

def draw_polys():
	bgnpolygon()
	cpack(0x0)
	v3f(polygon1[0])
	cpack(0x007F7F7F)
	v3f(polygon1[1])
	cpack(0x00FFFFFF)
	v3f(polygon1[2])
	endpolygon()
	#
	bgnpolygon()
	cpack(0x0000FFFF)
	v3f(polygon2[0])
	cpack(0x007FFF00)
	v3f(polygon2[1])
	cpack(0x00FF0000)
	v3f(polygon2[2])
	endpolygon()
	#
	bgnpolygon()
	cpack(0x0000FFFF)
	v3f(polygon3[0])
	cpack(0x00FF00FF)
	v3f(polygon3[1])
	cpack(0x00FF0000)
	v3f(polygon3[2])
	cpack(0x00FF00FF)
	v3f(polygon3[3])
	endpolygon()


main ()
