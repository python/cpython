#! /usr/bin/env python

#   backface
#
#   draw a cube that can run with backface() turned on or off.
#   cube is moved when LEFTMOUSE is pressed and mouse itself is moved.

from gl import *
from DEVICE import *
from GL import *

CUBE_SIZE = 200.0
CUBE_OBJ = 1

def main () :
	#
	x = 0
	y = 0
	moveit = 0
	#
	initialize()
	#
	while (1) :
		#
		while (qtest()) :
			dev, val = qread()
			#
			if dev == ESCKEY :
				backface(0)
				return
				#
			elif dev == REDRAW :
				reshapeviewport()
				drawcube(x,y)
				#
			elif dev == LEFTMOUSE :
				#
				# LEFTMOUSE down
				moveit = val
				#
			elif dev == BKEY :
				backface(1)
				drawcube(x,y)
				#
			elif dev == FKEY :
				backface(0)
				drawcube(x,y)
				#
		if moveit :
			x = getvaluator(MOUSEX)
			y = getvaluator(MOUSEY)
			drawcube(x,y)


def initialize () :
	foreground ()
	keepaspect (1, 1)
	gid = winopen('backface')
	winset(gid)
	winconstraints()
	#
	doublebuffer()
	gconfig()
	shademodel(FLAT)
	#
	ortho(-1024.0, 1024.0, -1024.0, 1024.0, -1024.0, 1024.0)
	#
	qdevice(ESCKEY)
	qdevice(REDRAW)
	qdevice(LEFTMOUSE)
	qdevice(BKEY)
	qdevice(FKEY)
	qenter(REDRAW,gid)
	#
	backface(1)

#
# define a cube
def cube () :
	#
	# front face
	pushmatrix()
	translate(0.0,0.0,CUBE_SIZE)
	color(RED)
	rectf(-CUBE_SIZE,-CUBE_SIZE,CUBE_SIZE,CUBE_SIZE)
	popmatrix()
	#
	# right face
	pushmatrix()
	translate(CUBE_SIZE, 0.0, 0.0)
	rotate(900, 'y')
	color(GREEN)
	rectf(-CUBE_SIZE,-CUBE_SIZE,CUBE_SIZE,CUBE_SIZE)
	popmatrix()
	#
	# back face
	pushmatrix()
	translate(0.0, 0.0, -CUBE_SIZE)
	rotate(1800, 'y')
	color(BLUE)
	rectf(-CUBE_SIZE,-CUBE_SIZE,CUBE_SIZE,CUBE_SIZE)
	popmatrix()
	#
	# left face
	pushmatrix()
	translate(-CUBE_SIZE, 0.0, 0.0)
	rotate(-900, 'y')
	color(CYAN)
	rectf(-CUBE_SIZE,-CUBE_SIZE,CUBE_SIZE,CUBE_SIZE)
	popmatrix()
	#
	# top face
	pushmatrix()
	translate(0.0, CUBE_SIZE, 0.0)
	rotate(-900, 'x')
	color(MAGENTA)
	rectf(-CUBE_SIZE,-CUBE_SIZE,CUBE_SIZE,CUBE_SIZE)
	popmatrix()
	#
	# bottom face
	pushmatrix()
	translate(0.0, -CUBE_SIZE, 0.0)
	rotate(900, 'x')
	color(YELLOW)
	rectf(-CUBE_SIZE,-CUBE_SIZE,CUBE_SIZE,CUBE_SIZE)
	popmatrix()

def drawcube(x,y) :
	#
	pushmatrix()
	rotate(2*x, 'x')
	rotate(2*y, 'y')
	color(BLACK)
	clear()
	cube()        
	popmatrix()
	swapbuffers()


main ()
