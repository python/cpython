#! /usr/bin/env python
# Simulate the artwork in the hall.
# Jack Jansen, Feb 91.

from gl import *
from GL import *
from math import *
from DEVICE import *
import sys
import __main__
main_dict = __main__.__dict__

SPOTDIRECTION = 103
SPOTLIGHT = 104

#
# Make a cylinder paralel with the Z axis with center (X,Y,0)
# and radius 1
def mkcyl(nslice, nparts, docircle):
        cyl = []
        step = 2.0 / float(nslice)
        z = -1.0
        for i in range(nslice):
            cyl.append(mkslice(z, z+step, nparts, docircle))
            z = z + step
        return drawcylinder(cyl)
#
# Make one part of a cylinder
#
def mkslice(z1, z2, nparts, docircle):
        if docircle:
            w1 = z1
            w2 = z2
            w1 = sqrt(1.0-w1*w1)
            w2 = sqrt(1.0-w2*w2)
            normalz = 1.0
        else:
            w1 = 1.0
            w2 = 1.0
            normalz = 0.0
        slice = []
        step = (2.0*pi)/float(nparts)
        angle = 0.0
        for i in range(nparts+1):
            vx = cos(angle)
            vy = sin(angle)
            slice.append( ((vx*w1,vy*w1,z1), (vx*w1, vy*w1, z1*normalz)) )
            slice.append( ((vx*w2,vy*w2,z2), (vx*w2, vy*w2, z2*normalz)) )
            angle = angle + step
        return slice
#
# Drawcylinder : draw the cylinder
#
class struct: pass
curobj = struct()
curobj.curobj = 1
def drawcylinder(cyl):
        obj = curobj.curobj
        curobj.curobj = curobj.curobj+1
        makeobj(obj)
        for slice in cyl:
            bgntmesh()
            vnarray(slice)
            endtmesh()
        closeobj()
        return obj
#
def drawnormals(cyl):
        for slice in cyl:
            for triang in slice:
                bgnline()
                v3f(triang[0])
                v3f(triang[0][0] + triang[1][0], triang[0][1] + triang[1][1], triang[0][2] + triang[1][2])
                endline()
def drawfloors():
        obj = curobj.curobj
        curobj.curobj = curobj.curobj+1
        makeobj(obj)
        bgnpolygon()
        v3i(4,6,-6)
        v3i(-6,6,-6)
        v3i(-6,-6,-6)
        v3i(4,-6,-6)
        endpolygon()
        for floor in range(3):
            pos = -1 + 5*floor
            bgnpolygon()
            v3i(4,4,pos)
            v3i(-6,4,pos)
            v3i(-6,6,pos)
            v3i(4,6,pos)
            endpolygon()
            bgnpolygon()
            v3i(-4,4,pos)
            v3i(-4,-4,pos)
            v3i(-6,-4,pos)
            v3i(-6,4,pos)
            endpolygon()
            bgnpolygon()
            v3i(-6,-4,pos)
            v3i(-6,-6,pos)
            v3i(4,-6,pos)
            v3i(4,-4,pos)
            endpolygon()
        closeobj()
        return obj
def drawdoors():
        obj = curobj.curobj
        curobj.curobj = curobj.curobj+1
        makeobj(obj)
        for floor in range(3):
            pos = -1+5*floor
            bgnpolygon()
            v3i(-2,6,pos)
            v3i(-2,6,pos+3)
            v3i(0,6,pos+3)
            v3i(0,6,pos)
            endpolygon()
        closeobj()
        return obj
def drawrailing():
        obj = curobj.curobj
        curobj.curobj = curobj.curobj+1
        makeobj(obj)
        for floor in range(3):
            pos = -1 + 5*floor
            bgnpolygon()
            v3i(4,4,pos)
            v3i(4,4,pos-1)
            v3i(-4,4,pos-1)
            v3i(-4,4,pos)
            endpolygon()
            bgnpolygon()
            v3i(-4,4,pos)
            v3i(-4,4,pos-1)
            v3i(-4,-4,pos-1)
            v3i(-4,-4,pos)
            endpolygon()
            bgnpolygon()
            v3i(-4,-4,pos)
            v3i(-4,-4,pos-1)
            v3i(4,-4,pos-1)
            v3i(4,-4,pos)
            endpolygon()
        closeobj()
        return obj
def drawwalls():
        obj = curobj.curobj
        curobj.curobj = curobj.curobj+1
        makeobj(obj)
        bgnpolygon()
        v3i(4,6,-6)
        v3i(4,6,18)
        v3i(-6,6,18)
        v3i(-6,6,-6)
        endpolygon()
        bgnpolygon()
        v3i(-6,6,-6)
        v3i(-6,6,18)
        v3i(-6,-6,18)
        v3i(-6,-6,-6)
        endpolygon()
        bgnpolygon()
        v3i(-6,-6,-6)
        v3i(-6,-6,18)
        v3i(4,-6,18)
        v3i(4,-6,-6)
        endpolygon()
        bgnpolygon()
        v3i(4,-6,-6)
        v3i(4,-6,18)
        v3i(4,4,18)
        v3i(4,4,-6)
        endpolygon()
        closeobj()
        return obj
def axis():
        bgnline()
        cpack(0xff0000)
        v3i(-1,0,0)
        v3i(1,0,0)
        v3f(1.0, 0.1, 0.1)
        endline()
        bgnline()
        cpack(0xff00)
        v3i(0,-1,0)
        v3i(0,1,0)
        v3f(0.1, 1.0, 0.1)
        endline()
        bgnline()
        cpack(0xff)
        v3i(0,0,-1)
        v3i(0,0,1)
        v3f(0.1,0.1,1.0)
        endline()
#
green_velvet = [ DIFFUSE, 0.05, 0.4, 0.05, LMNULL]
silver = [ DIFFUSE, 0.3, 0.3, 0.3, SPECULAR, 0.9, 0.9, 0.95, \
        SHININESS, 40.0, LMNULL]
floormat = [ AMBIENT, 0.5, 0.25, 0.15, DIFFUSE, 0.5, 0.25, 0.15, SPECULAR, 0.6, 0.3, 0.2, SHININESS, 20.0, LMNULL]
wallmat = [ DIFFUSE, 0.4, 0.2, 0.1, AMBIENT, 0.4, 0.20, 0.10, SPECULAR, 0.0, 0.0, 0.0, SHININESS, 20.0, LMNULL]
offwhite = [ DIFFUSE, 0.8, 0.8, 0.6, AMBIENT, 0.8, 0.8, 0.6, SPECULAR, 0.9, 0.9, 0.9, SHININESS, 30.0, LMNULL]
doormat = [ DIFFUSE, 0.1, 0.2, 0.5, AMBIENT, 0.2, 0.4, 1.0, SPECULAR, 0.2, 0.4, 1.0, SHININESS, 60.0, LMNULL]

toplight = [ LCOLOR, 1.0, 1.0, 0.5, \
        POSITION, 0.0, 0.0, 11.0, 1.0, LMNULL]
floor1light = [ LCOLOR, 1.0, 1.0, 1.0, POSITION, 3.9, -3.9, 0.0, 1.0, \
        SPOTDIRECTION, 1.0, 1.0, 0.0, SPOTLIGHT, 10.0, 90.0, LMNULL]

lmodel = [ AMBIENT, 0.92, 0.8, 0.5, LOCALVIEWER, 1.0, LMNULL]
#
def lighting():
        lmdef(DEFMATERIAL, 1, green_velvet)
        lmdef(DEFMATERIAL, 2, silver)
        lmdef(DEFMATERIAL, 3, floormat)
        lmdef(DEFMATERIAL, 4, wallmat)
        lmdef(DEFMATERIAL, 5, offwhite)
        lmdef(DEFMATERIAL, 6, doormat)
        lmdef(DEFLIGHT, 1, toplight)
        lmdef(DEFLIGHT, 2, floor1light)
        lmdef(DEFLMODEL, 1, lmodel)
        lmbind(MATERIAL, 1)
        lmbind(LIGHT0, 1)
        lmbind(LIGHT1, 2)
        lmbind(LMODEL, 1)
IdMat=[1.0,0.0,0.0,0.0, 0.0,1.0,0.0,0.0, 0.0,0.0,1.0,0.0, 0.0,0.0,0.0,1.0]
#
def defun(axis):
        done = 0
        while not done:
            print 'F'+axis+'(t) = ',
            s = sys.stdin.readline(100)
            print
            try:
                s = 'def f'+axis+'(t): return '+s
                exec(s, main_dict)
                done = 1
            except RuntimeError:
                print 'Sorry, there is a syntax error in your expression'
def getfunctions():
        print 'Welcome to the CWI art simulator. You can now enter X, Y and Z'
        print 'coordinates as a function of t.'
        print 'Normal trig functions are available. Please use floating point'
        print 'values only (so 0.0 for 0). Comments to jack@cwi.nl'
        defun('x')
        defun('y')
        defun('z')
        print 'Ok, here you go. Use mouse+right button to move up/down,'
        print 'mouse+middle to speed up/slow down time. type ESC to quit simulation'
def main():
        getfunctions()
        foreground()
        prefposition(100,600,100,600)
        void = winopen('cyl')
        qdevice(ESCKEY)
        qdevice(MOUSE1)
        qdevice(MOUSE2)
        qdevice(PKEY)
        RGBmode()
        doublebuffer()
        gconfig()
        zbuffer(1)
        mmode(MVIEWING)
        perspective(400, 1.0, 1.0, 20.0)
        loadmatrix(IdMat)
        vx = 0.0
        vy = -6.0
        vz = 0.0
        lookat(0.0, -6.0, 0.0, 0.0, 0.0, 0.0, 0)
        lighting()
        t = -1.0
        step = 1.0
        bol = mkcyl(12,24, 1)
        cable = mkcyl(1, 6, 0)
        floors = drawfloors()
        walls = drawwalls()
        pillar = mkcyl(1,4,0)
        railing = drawrailing()
        doors = drawdoors()
        shademodel(GOURAUD)
        mousing = -1
        pausing = 0
        while 1:
            #
            # Check for some user input
            #
            if qtest():
                dev, value = qread()
                if dev == PKEY and value == 1:
                        pausing = 1
                if dev == ESCKEY:
                    break
                elif (dev==MOUSE1 or dev==MOUSE2) and value == 1:
                    if mousing > 0:
                        vx = 0.0
                        vy = -6.0
                        vz = 0.0
                    mousing = dev
                    oldx = getvaluator(MOUSEX)
                    oldy = getvaluator(MOUSEY)
                elif (dev==MOUSE1 or dev==MOUSE2):
                    mousing = -1
            if mousing >= 0:
                newx = getvaluator(MOUSEX)
                newy = getvaluator(MOUSEY)
                if newy <> oldy and mousing==MOUSE1:
                    vz = vz + float(newy - oldy)/100.0
                    dist = sqrt(vx*vx + vy*vy + vz*vz)
                    perspective(400, 1.0, 1.0, dist+16.0)
                    loadmatrix(IdMat)
                    if vz < 0.0:
                        lookat(vx, vy, vz, 0.0, 0.0, 0.0, 1800)
                    else:
                        lookat(vx, vy, vz, 0.0, 0.0, 0.0, 0)
                if newy <> oldy and mousing==MOUSE2:
                    step = step * exp(float(newy-oldy)/400.0)
            if getbutton(CTRLKEY) == 0:
                t = t + step
            else:
                t = t - step
            if getbutton(LEFTSHIFTKEY) == 0:
                shademodel(GOURAUD)
            else:
                shademodel(FLAT)
            #
            # Draw background and axis
            cpack(0x105090)
            clear()
            zclear()
            cpack(0x905010)
            axis()
            #
            # Draw object
            #
            bolx = fx(t)
            boly = fy(t)
            bolz = fz(t)
            err = ''
            if bolx < -4.0 or bolx > 4.0:
                err = 'X('+`bolx`+') out of range [-4,4]'
            if boly < -4.0 or boly > 4.0:
                err = 'Y('+`boly`+') out of range [-4,4]'
            if bolz < -4.0 or bolz > 8.0:
                err = 'Z('+`bolz`+') out of range [-4,8]'
            if not err:
                pushmatrix()
                translate(bolx, boly, bolz)
                scale(0.3, 0.3, 0.3)
                lmbind(MATERIAL, 2)
                callobj(bol)
                popmatrix()
                #
                # Draw the cables
                #
                bolz = bolz + 0.3
                pushmatrix()
                linesmooth(SML_ON)
                bgnline()
                v3i(-4,-4,9)
                v3f(bolx, boly, bolz)
                endline()
                bgnline()
                v3i(-4,4,9)
                v3f(bolx, boly, bolz)
                endline()
                bgnline()
                v3i(4,-4,9)
                v3f(bolx, boly, bolz)
                endline()
                bgnline()
                v3i(4,4,9)
                v3f(bolx, boly, bolz)
                endline()
                popmatrix()
            #
            # draw the floors
            #
            lmbind(MATERIAL, 3)
            callobj(floors)
            lmbind(MATERIAL, 4)
            callobj(walls)
            lmbind(MATERIAL, 5)
            pushmatrix()
            translate(-4.5,4.5,3.0)
            scale(0.2,0.2,9.0)
            rotate(450,'z')
            callobj(pillar)
            popmatrix()
            callobj(railing)
            lmbind(MATERIAL, 6)
            pushmatrix()
            translate(0.0, -0.01, 0.0)
            callobj(doors)
            popmatrix()
            if mousing == MOUSE2 or err:
                cpack(0xff0000)
                cmov(0.0, 0.0, 0.4)
                charstr('t='+`t`)
            if mousing == MOUSE2:
                cpack(0xff0000)
                cmov(0.0, 0.0, 0.2)
                charstr('delta-t='+`step`)
            if err:
                cpack(0xff00)
                cmov(0.0, 0.0, 0.2)
                print err
                charstr(err)
                pausing = 1
            if pausing:
                cpack(0xff00)
                cmov(0.0, 0.0, 0.0)
                charstr('Pausing, type P to continue')
            swapbuffers()
            if pausing:
                while 1:
                    dv=qread()
                    if dv==(PKEY,1):
                        break
                    if dv==(ESCKEY,1):
                        sys.exit(0)
                pausing = 0
#
try:
    main()
except KeyboardInterrupt:
    sys.exit(1)
