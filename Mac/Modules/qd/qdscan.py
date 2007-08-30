# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)

from scantools import Scanner

def main():
    input = "QuickDraw.h"
    output = "qdgen.py"
    defsoutput = TOOLBOXDIR + "QuickDraw.py"
    scanner = MyScanner(input, output, defsoutput)
    scanner.scan()
    scanner.close()

    # Grmpf. Universal Headers have Text-stuff in a different include file...
    input = "QuickDrawText.h"
    output = "@qdgentext.py"
    defsoutput = "@QuickDrawText.py"
    have_extra = 0
    try:
        scanner = MyScanner(input, output, defsoutput)
        scanner.scan()
        scanner.close()
        have_extra = 1
    except IOError:
        pass
    if have_extra:
        print("=== Copying QuickDrawText stuff into main files... ===")
        ifp = open("@qdgentext.py")
        ofp = open("qdgen.py", "a")
        ofp.write(ifp.read())
        ifp.close()
        ofp.close()
        ifp = open("@QuickDrawText.py")
        ofp = open(TOOLBOXDIR + "QuickDraw.py", "a")
        ofp.write(ifp.read())
        ifp.close()
        ofp.close()

    print("=== Testing definitions output code ===")
    exec(open(defsoutput).read(), {}, {})
    print("=== Done scanning and generating, now importing the generated code... ===")
    import qdsupport
    print("=== Done.  It's up to you to compile it now! ===")

class MyScanner(Scanner):

    def destination(self, type, name, arglist):
        classname = "Function"
        listname = "functions"
        if arglist:
            t, n, m = arglist[0]
            if t in ('GrafPtr', 'CGrafPtr') and m == 'InMode':
                classname = "Method"
                listname = "gr_methods"
            elif t == 'BitMapPtr' and m == 'InMode':
                classname = "Method"
                listname = "bm_methods"
##                      elif t == "PolyHandle" and m == "InMode":
##                              classname = "Method"
##                              listname = "p_methods"
##                      elif t == "RgnHandle" and m == "InMode":
##                              classname = "Method"
##                              listname = "r_methods"
        return classname, listname


    def writeinitialdefs(self):
        self.defsfile.write("""
def FOUR_CHAR_CODE(x): return x
normal                                          = 0
bold                                            = 1
italic                                          = 2
underline                                       = 4
outline                                         = 8
shadow                                          = 0x10
condense                                        = 0x20
extend                                          = 0x40
""")

    def makeblacklistnames(self):
        return [
                'InitGraf',
                'StuffHex',
                'StdLine',
                'StdComment',
                'StdGetPic',
                'OpenPort',
                'InitPort',
                'ClosePort',
                'OpenCPort',
                'InitCPort',
                'CloseCPort',
                'BitMapToRegionGlue',
                'StdOpcode',    # XXXX Missing from library...
                # The following are for non-macos use:
                'LockPortBits',
                'UnlockPortBits',
                'UpdatePort',
                'GetPortNativeWindow',
                'GetNativeWindowPort',
                'NativeRegionToMacRegion',
                'MacRegionToNativeRegion',
                'GetPortHWND',
                'GetHWNDPort',
                'GetPICTFromDIB',

                'HandleToRgn', # Funny signature

                # Need Cm, which we don't want to drag in just yet
                'OpenCursorComponent',
                'CloseCursorComponent',
                'SetCursorComponent',
                'CursorComponentChanged',
                'CursorComponentSetData',
                ]

    def makeblacklisttypes(self):
        return [
                "QDRegionBitsRef", # Should do this, but too lazy now.
                'CIconHandle', # Obsolete
                'CQDProcs',
                'CQDProcsPtr',
                'CSpecArray',
                'ColorComplementProcPtr',
                'ColorComplementUPP',
                'ColorSearchProcPtr',
                'ColorSearchUPP',
                'ConstPatternParam',
                'DeviceLoopDrawingProcPtr',
                'DeviceLoopFlags',
                'GrafVerb',
                'OpenCPicParams_ptr',
                'Ptr',
                'QDProcs',
                'ReqListRec',
                'void_ptr',
                'CustomXFerProcPtr',
                ]

    def makerepairinstructions(self):
        return [
                ([('void_ptr', 'textBuf', 'InMode'),
                  ('short', 'firstByte', 'InMode'),
                  ('short', 'byteCount', 'InMode')],
                 [('TextThingie', '*', '*'), ('*', '*', '*'), ('*', '*', '*')]),

                # GetPen and SetPt use a point-pointer as output-only:
                ('GetPen', [('Point', '*', 'OutMode')], [('*', '*', 'OutMode')]),
                ('SetPt', [('Point', '*', 'OutMode')], [('*', '*', 'OutMode')]),

                # All others use it as input/output:
                ([('Point', '*', 'OutMode')],
                 [('*', '*', 'InOutMode')]),

                 # InsetRect, OffsetRect
                 ([('Rect', 'r', 'OutMode'),
                        ('short', 'dh', 'InMode'),
                        ('short', 'dv', 'InMode')],
                  [('Rect', 'r', 'InOutMode'),
                        ('short', 'dh', 'InMode'),
                        ('short', 'dv', 'InMode')]),

                 # MapRect
                 ([('Rect', 'r', 'OutMode'),
                        ('Rect_ptr', 'srcRect', 'InMode'),
                        ('Rect_ptr', 'dstRect', 'InMode')],
                  [('Rect', 'r', 'InOutMode'),
                        ('Rect_ptr', 'srcRect', 'InMode'),
                        ('Rect_ptr', 'dstRect', 'InMode')]),

                 # CopyBits and friends
                 ([('RgnHandle', 'maskRgn', 'InMode')],
                  [('OptRgnHandle', 'maskRgn', 'InMode')]),

                 ('QDFlushPortBuffer',
                  [('RgnHandle', '*', 'InMode')],
                  [('OptRgnHandle', '*', 'InMode')]),

                 # Accessors with reference argument also returned.
                 ([('Rect_ptr', 'GetPortBounds', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('RGBColor_ptr', 'GetPortForeColor', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('RGBColor_ptr', 'GetPortBackColor', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('RGBColor_ptr', 'GetPortOpColor', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('RGBColor_ptr', 'GetPortHiliteColor', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('Point_ptr', 'GetPortPenSize', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('Point_ptr', 'GetPortPenLocation', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('Rect_ptr', 'GetPixBounds', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('BitMap_ptr', 'GetQDGlobalsScreenBits', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('Cursor_ptr', 'GetQDGlobalsArrow', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('Rect_ptr', 'GetRegionBounds', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('Pattern_ptr', '*', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('Point_ptr', 'QDLocalToGlobalPoint', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('Rect_ptr', 'QDLocalToGlobalRect', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('Point_ptr', 'QDGlobalToLocalPoint', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                 ([('Rect_ptr', 'QDGlobalToLocalRect', 'ReturnMode')],
                  [('void', '*', 'ReturnMode')]),

                ]

if __name__ == "__main__":
    main()
