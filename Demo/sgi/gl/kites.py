#! /usr/bin/env python

# *** This only works correctly on a 24 bit-plane machine. ***
#
# A simple Python program that tests the some parts of the
# GL library. It shows the speed that can be obtained when
# doing simple graphics.
#
# The bottleneck in this program is NOT Python but the graphics
# engine; i.e Python can feed the graphics pipeline fast enough
# on the 4D/25G.
#
# This program show 3 kites flying around the screen. It uses
#
# 	* bgnpolygon, endpolygon
# 	* v3, n3
# 	* lmdef, lmbind
#
# Usage :
# 
# 	ESC 	-> exit program
# 	MOUSE3 	-> freeze toggle
# 	MOUSE2 	-> one step (use this in freeze state)

from GL import *
from gl import *
import DEVICE
from math import *

#
# viewobj : sets the rotation, translation and scaling
# set appropiate material, call drawobject()
#
def viewobj (r, s, t, mat) :
	pushmatrix()
	rot (r * 10.0, 'X')
	rot (r * 10.0, 'Y')
	rot (r * 10.0, 'Z')
	scale (s[0], s[1], s[2])
	translate (t[0], t[1], t[2])
	lmbind(MATERIAL, mat)
	drawobject()
	popmatrix()

#
# makeobj : the constructor of the object
#
def mkobj () :
	v0 = (-5.0 ,0.0, 0.0)
	v1 = (0.0 ,5.0, 0.0)
	v2 = (5.0 ,0.0, 0.0)
	v3 = (0.0 ,2.0, 0.0)
	n0 = (sqrt(2.0)/2.0, sqrt(2.0)/2.0, 0.0)
	vn = ((v0, n0), (v1, n0), (v2, n0), (v3, n0))
	#
	return vn

#
# the object itself as an array of vertices and normals
#
kite = mkobj ()

#
# drawobject : draw a triangle. with bgnpolygon
#
def drawobject () :
	#
	bgnpolygon()
	vnarray (kite)
	endpolygon()

#
# identity matrix
#
idmat=[1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0]

#
# the rgb-value of light-blue 
#
LightBlue = (43,169,255)

#
# the different materials.
#
m1=[SPECULAR,0.0,0.0,0.6,DIFFUSE,0.0,0.0,0.8,SHININESS,20.0,LMNULL]
m2=[SPECULAR,0.8,0.0,0.1,DIFFUSE,0.8,0.0,0.3,SHININESS,120.0,LMNULL]
m3=[SPECULAR,0.0,1.0,0.0,DIFFUSE,0.0,0.6,0.0,SHININESS,120.0,LMNULL]

#
# lightsources
#
light1 = [LCOLOR,1.0,1.0,1.0,POSITION,15.0,15.0,0.0,1.0,LMNULL]
light2 = [LCOLOR,1.0,1.0,1.0,POSITION,-15.0,15.0,0.0,1.0,LMNULL]

#
# the lightmodel
#
model = [AMBIENT,0.2,0.2,0.2,LMNULL]

#
# initgl : opens the window, configures the pipeline to 2buf and zbuf,
# sets the viewing, defines and binds the materials
#
def initgl () :
	#
	# open window
	#
	foreground ()
	keepaspect (1, 1)
	prefposition (100, 500, 100, 500)
	w = winopen ('PYTHON lights')
	keepaspect (1, 1)
	winconstraints()
	#
	# configure pipeline (zbuf, 2buf, GOURAUD and RGBmode)
	#
	zbuffer (1)
	doublebuffer ()
	shademodel (GOURAUD)
	RGBmode ()
	gconfig ()
	#
	# define and bind materials (set perspective BEFORE loadmat !)
	#
	mmode(MVIEWING)
	perspective (900, 1.0, 1.0, 20.0)
	loadmatrix(idmat)
	lmdef(DEFMATERIAL, 1, m1)
	lmdef(DEFMATERIAL, 2, m2)
	lmdef(DEFMATERIAL, 3, m3)
	lmdef(DEFLIGHT, 1, light1)
	lmdef(DEFLIGHT, 2, light2)
	lmdef(DEFLMODEL, 1, model)
	lmbind(LIGHT0,1)
	lmbind(LIGHT1,2)
	lmbind(LMODEL,1)
	#
	# set viewing
	#
	lookat (0.0, 0.0, 10.0, 0.0, 0.0, 0.0, 0)
	#
	# ask for the REDRAW and ESCKEY events
	#
	qdevice(DEVICE.MOUSE3)
	qdevice(DEVICE.MOUSE2)
	qdevice(DEVICE.REDRAW)
	qdevice(DEVICE.ESCKEY)

#
# GoForIT : use 2buf to redraw the object 2n times. index i is used as 
# the (smoothly changing) rotation angle
#
def GoForIt(i) :
	freeze = 1
	while 1 :
		if freeze <> 0 :
			i = i + 1
		#
		# clear z-buffer and clear background to light-blue
		#
		zclear()
		c3i (LightBlue)
		clear()
		#
		# draw the 3 traiangles scaled above each other.
		#
		viewobj(float(i),[1.0,1.0,1.0],[1.0,1.0,1.0],1)
		viewobj(float(i),[0.75,0.75,0.75],[0.0,2.0,2.0],2)
		viewobj(float(i),[0.5,0.5,0.5],[0.0,4.0,4.0],3)
		#
		swapbuffers()
		#
		if qtest() <> 0 :
			dev, val = qread()
			if dev == DEVICE.ESCKEY :
				break
			elif dev == DEVICE.REDRAW :
				reshapeviewport ()
			elif dev == DEVICE.MOUSE3 and val <> 0 :
				freeze = 1 - freeze
			elif dev == DEVICE.MOUSE2 and val <> 0 :
				i = i + 1


# the main program
#
def main () :
	initgl ()
	GoForIt (0)

#
# exec main
#
main  ()
