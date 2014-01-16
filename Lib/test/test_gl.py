"""Very simple test script for the SGI gl library extension module
    taken mostly from the documentation.
    Roger E. Masse
"""
import unittest
from test.test_support import verbose, import_module
import time
gl = import_module('gl')
GL = import_module('GL')

glattrs = ['RGBcolor', 'RGBcursor', 'RGBmode', 'RGBrange', 'RGBwritemask',
'__doc__', '__name__', 'addtopup', 'altgetmatrix', 'arc', 'arcf',
'arcfi', 'arcfs', 'arci', 'arcs', 'attachcursor', 'backbuffer',
'backface', 'bbox2', 'bbox2i', 'bbox2s', 'bgnclosedline', 'bgnline',
'bgnpoint', 'bgnpolygon', 'bgnsurface', 'bgntmesh', 'bgntrim',
'blankscreen', 'blanktime', 'blendfunction', 'blink', 'c3f', 'c3i',
'c3s', 'c4f', 'c4i', 'c4s', 'callobj', 'charstr', 'chunksize', 'circ',
'circf', 'circfi', 'circfs', 'circi', 'circs', 'clear',
'clearhitcode', 'clkoff', 'clkon', 'closeobj', 'cmode', 'cmov',
'cmov2', 'cmov2i', 'cmov2s', 'cmovi', 'cmovs', 'color', 'colorf',
'compactify', 'concave', 'cpack', 'crv', 'crvn', 'curorigin',
'cursoff', 'curson', 'curstype', 'curvebasis', 'curveit',
'curveprecision', 'cyclemap', 'czclear', 'defbasis', 'defcursor',
'deflinestyle', 'delobj', 'deltag', 'depthcue', 'devport', 'dglclose',
'dglopen', 'dither', 'dopup', 'doublebuffer', 'draw', 'draw2',
'draw2i', 'draw2s', 'drawi', 'drawmode', 'draws', 'editobj',
'endclosedline', 'endfullscrn', 'endline', 'endpick', 'endpoint',
'endpolygon', 'endpupmode', 'endselect', 'endsurface', 'endtmesh',
'endtrim', 'finish', 'font', 'foreground', 'freepup', 'frontbuffer',
'fudge', 'fullscrn', 'gRGBcolor', 'gRGBmask', 'gammaramp', 'gbegin',
'gconfig', 'genobj', 'gentag', 'getbackface', 'getbuffer',
'getbutton', 'getcmmode', 'getcolor', 'getcpos', 'getcursor',
'getdcm', 'getdepth', 'getdescender', 'getdisplaymode', 'getdrawmode',
'getfont', 'getgdesc', 'getgpos', 'getheight', 'gethitcode',
'getlsbackup', 'getlsrepeat', 'getlstyle', 'getlwidth', 'getmap',
'getmatrix', 'getmcolor', 'getmmode', 'getmonitor',
'getnurbsproperty', 'getopenobj', 'getorigin', 'getothermonitor',
'getpattern', 'getplanes', 'getport', 'getresetls', 'getscrmask',
'getshade', 'getsize', 'getsm', 'gettp', 'getvaluator', 'getvideo',
'getviewport', 'getwritemask', 'getzbuffer', 'gewrite', 'gflush',
'ginit', 'glcompat', 'greset', 'gselect', 'gsync', 'gversion',
'iconsize', 'icontitle', 'imakebackground', 'initnames', 'ismex',
'isobj', 'isqueued', 'istag', 'keepaspect', 'lRGBrange', 'lampoff',
'lampon', 'linesmooth', 'linewidth', 'lmbind', 'lmcolor', 'lmdef',
'loadmatrix', 'loadname', 'logicop', 'lookat', 'lrectread',
'lrectwrite', 'lsbackup', 'lsetdepth', 'lshaderange', 'lsrepeat',
'makeobj', 'maketag', 'mapcolor', 'mapw', 'mapw2', 'maxsize',
'minsize', 'mmode', 'move', 'move2', 'move2i', 'move2s', 'movei',
'moves', 'multimap', 'multmatrix', 'n3f', 'newpup', 'newtag',
'noborder', 'noise', 'noport', 'normal', 'nurbscurve', 'nurbssurface',
'nvarray', 'objdelete', 'objinsert', 'objreplace', 'onemap', 'ortho',
'ortho2', 'overlay', 'packrect', 'pagecolor', 'pagewritemask',
'passthrough', 'patch', 'patchbasis', 'patchcurves', 'patchprecision',
'pclos', 'pdr', 'pdr2', 'pdr2i', 'pdr2s', 'pdri', 'pdrs',
'perspective', 'pick', 'picksize', 'pixmode', 'pmv', 'pmv2', 'pmv2i',
'pmv2s', 'pmvi', 'pmvs', 'pnt', 'pnt2', 'pnt2i', 'pnt2s', 'pnti',
'pnts', 'pntsmooth', 'polarview', 'polf', 'polf2', 'polf2i', 'polf2s',
'polfi', 'polfs', 'poly', 'poly2', 'poly2i', 'poly2s', 'polyi',
'polys', 'popattributes', 'popmatrix', 'popname', 'popviewport',
'prefposition', 'prefsize', 'pupmode', 'pushattributes', 'pushmatrix',
'pushname', 'pushviewport', 'pwlcurve', 'qdevice', 'qenter', 'qgetfd',
'qread', 'qreset', 'qtest', 'rcrv', 'rcrvn', 'rdr', 'rdr2', 'rdr2i',
'rdr2s', 'rdri', 'rdrs', 'readdisplay', 'readsource', 'rect',
'rectcopy', 'rectf', 'rectfi', 'rectfs', 'recti', 'rects', 'rectzoom',
'resetls', 'reshapeviewport', 'ringbell', 'rmv', 'rmv2', 'rmv2i',
'rmv2s', 'rmvi', 'rmvs', 'rot', 'rotate', 'rpatch', 'rpdr', 'rpdr2',
'rpdr2i', 'rpdr2s', 'rpdri', 'rpdrs', 'rpmv', 'rpmv2', 'rpmv2i',
'rpmv2s', 'rpmvi', 'rpmvs', 'sbox', 'sboxf', 'sboxfi', 'sboxfs',
'sboxi', 'sboxs', 'scale', 'screenspace', 'scrmask', 'setbell',
'setcursor', 'setdepth', 'setlinestyle', 'setmap', 'setmonitor',
'setnurbsproperty', 'setpattern', 'setpup', 'setshade', 'setvaluator',
'setvideo', 'shademodel', 'shaderange', 'singlebuffer', 'smoothline',
'spclos', 'splf', 'splf2', 'splf2i', 'splf2s', 'splfi', 'splfs',
'stepunit', 'strwidth', 'subpixel', 'swapbuffers', 'swapinterval',
'swaptmesh', 'swinopen', 'textcolor', 'textinit', 'textport',
'textwritemask', 'tie', 'tpoff', 'tpon', 'translate', 'underlay',
'unpackrect', 'unqdevice', 'v2d', 'v2f', 'v2i', 'v2s', 'v3d', 'v3f',
'v3i', 'v3s', 'v4d', 'v4f', 'v4i', 'v4s', 'varray', 'videocmd',
'viewport', 'vnarray', 'winattach', 'winclose', 'winconstraints',
'windepth', 'window', 'winget', 'winmove', 'winopen', 'winpop',
'winposition', 'winpush', 'winset', 'wintitle', 'wmpack', 'writemask',
'writepixels', 'xfpt', 'xfpt2', 'xfpt2i', 'xfpt2s', 'xfpt4', 'xfpt4i',
'xfpt4s', 'xfpti', 'xfpts', 'zbuffer', 'zclear', 'zdraw', 'zfunction',
'zsource', 'zwritemask']

def test_main():
    # insure that we at least have an X display before continuing.
    import os
    try:
        display = os.environ['DISPLAY']
    except:
        raise unittest.SkipTest, "No $DISPLAY -- skipping gl test"

    # touch all the attributes of gl without doing anything
    if verbose:
        print 'Touching gl module attributes...'
    for attr in glattrs:
        if verbose:
            print 'touching: ', attr
        getattr(gl, attr)

    # create a small 'Crisscross' window
    if verbose:
        print 'Creating a small "CrissCross" window...'
        print 'foreground'
    gl.foreground()
    if verbose:
        print 'prefposition'
    gl.prefposition(500, 900, 500, 900)
    if verbose:
        print 'winopen "CrissCross"'
    w = gl.winopen('CrissCross')
    if verbose:
        print 'clear'
    gl.clear()
    if verbose:
        print 'ortho2'
    gl.ortho2(0.0, 400.0, 0.0, 400.0)
    if verbose:
        print 'color WHITE'
    gl.color(GL.WHITE)
    if verbose:
        print 'color RED'
    gl.color(GL.RED)
    if verbose:
        print 'bgnline'
    gl.bgnline()
    if verbose:
        print 'v2f'
    gl.v2f(0.0, 0.0)
    gl.v2f(400.0, 400.0)
    if verbose:
        print 'endline'
    gl.endline()
    if verbose:
        print 'bgnline'
    gl.bgnline()
    if verbose:
        print 'v2i'
    gl.v2i(400, 0)
    gl.v2i(0, 400)
    if verbose:
        print 'endline'
    gl.endline()
    if verbose:
        print 'Displaying window for 2 seconds...'
    time.sleep(2)
    if verbose:
        print 'winclose'
    gl.winclose(w)


if __name__ == '__main__':
    test_main()
