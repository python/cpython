#! /usr/bin/env python

# Rotate a 3D surface created using NURBS.
#
# Press left mouse button to toggle surface trimming.
# Press ESC to quit.
#
# See the GL manual for an explanation of NURBS.

from gl import *
from GL import *
from DEVICE import *

TRUE = 1
FALSE = 0
ORDER = 4

idmat = [1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]

surfknots = [-1, -1, -1, -1, 1, 1, 1, 1]

def make_ctlpoints():
	c = []
	#
	ci = []
	ci.append((-2.5,  -3.7,  1.0))
	ci.append((-1.5,  -3.7,  3.0))
	ci.append((1.5,  -3.7, -2.5))
	ci.append((2.5,  -3.7,  -0.75))
	c.append(ci)
	#
	ci = []
	ci.append((-2.5,  -2.0,  3.0))
	ci.append((-1.5,  -2.0,  4.0))
	ci.append((1.5,  -2.0,  -3.0))
	ci.append((2.5,  -2.0,  0.0))
	c.append(ci)
	#
	ci = []
	ci.append((-2.5, 2.0,  1.0))
	ci.append((-1.5, 2.0,  0.0))
	ci.append((1.5,  2.0,  -1.0))
	ci.append((2.5,  2.0,  2.0))
	c.append(ci)
	#
	ci = []
	ci.append((-2.5,  2.7,  1.25))
	ci.append((-1.5,  2.7,  0.1))
	ci.append((1.5,  2.7,  -0.6))
	ci.append((2.5,  2.7,  0.2))
	c.append(ci)
	#
	return c

ctlpoints = make_ctlpoints()

trimknots = [0., 0., 0.,  1., 1.,  2., 2.,  3., 3.,   4., 4., 4.]

def make_trimpoints():
	c = []
	c.append((1.0, 0.0, 1.0))
	c.append((1.0, 1.0, 1.0))
	c.append((0.0, 2.0, 2.0))
	c.append((-1.0, 1.0, 1.0))
	c.append((-1.0, 0.0, 1.0))
	c.append((-1.0, -1.0, 1.0))
	c.append((0.0, -2.0, 2.0))
	c.append((1.0, -1.0, 1.0) )
	c.append((1.0, 0.0, 1.0))
	return c

trimpoints = make_trimpoints()

def main():
	init_windows()
	setup_queue()
	make_lights()
	init_view()
	#
	set_scene()
	setnurbsproperty( N_ERRORCHECKING, 1.0 )
	setnurbsproperty( N_PIXEL_TOLERANCE, 50.0 )
	trim_flag = 0
	draw_trim_surface(trim_flag)
	#
	while 1:
		while qtest():
			dev, val = qread()
			if dev == ESCKEY:
				return
			elif dev == WINQUIT:
				dglclose(-1)	# this for DGL only
				return
			elif dev == REDRAW:
				reshapeviewport()
				set_scene()
				draw_trim_surface(trim_flag)
			elif dev == LEFTMOUSE:
				if val:
					trim_flag = (not trim_flag)
		set_scene()
		draw_trim_surface(trim_flag)

def init_windows():
	foreground()
	#prefposition(0, 500, 0, 500)
	wid = winopen('nurbs')
	wintitle('NURBS Surface')
	doublebuffer()
	RGBmode()
	gconfig()
	lsetdepth(0x000, 0x7fffff)
	zbuffer( TRUE )

def setup_queue():
	qdevice(ESCKEY)
	qdevice(REDRAW)
	qdevice(RIGHTMOUSE)
	qdevice(WINQUIT)
	qdevice(LEFTMOUSE) #trimming

def init_view():
	mmode(MPROJECTION)
	ortho( -4., 4., -4., 4., -4., 4. )
	#
	mmode(MVIEWING)
	loadmatrix(idmat)
	#
	lmbind(MATERIAL, 1)

def set_scene():
	lmbind(MATERIAL, 0)
	RGBcolor(150,150,150)
	lmbind(MATERIAL, 1)
	clear()
	zclear()
	#
	rotate( 100, 'y' )
	rotate( 100, 'z' )

def draw_trim_surface(trim_flag):
	bgnsurface()
	nurbssurface(surfknots, surfknots, ctlpoints, ORDER, ORDER, N_XYZ)
	if trim_flag:
		bgntrim()
		nurbscurve(trimknots, trimpoints, ORDER-1, N_STW)
		endtrim()
	endsurface()
	swapbuffers()

def make_lights():
	lmdef(DEFLMODEL,1,[])
	lmdef(DEFLIGHT,1,[])
	#
	# define material #1
	#
	a = []
	a = a + [EMISSION, 0.0, 0.0, 0.0]
	a = a + [AMBIENT,  0.1, 0.1, 0.1]
	a = a + [DIFFUSE,  0.6, 0.3, 0.3]
	a = a + [SPECULAR,  0.0, 0.6, 0.0]
	a = a + [SHININESS, 2.0]
	a = a + [LMNULL]
	lmdef(DEFMATERIAL, 1, a)
	#
	# turn on lighting
	#
	lmbind(LIGHT0, 1)
	lmbind(LMODEL, 1)

main()
