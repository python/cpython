# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner_OSX

LONG = "CoreGraphics"
SHORT = "cg"
OBJECTS = ("CGContextRef",
                )
# ADD object typenames here

def main():
    input = [
            "CGContext.h",
    ]
    output = SHORT + "gen.py"
    defsoutput = TOOLBOXDIR + LONG + ".py"
    scanner = MyScanner(input, output, defsoutput)
    scanner.scan()
    scanner.gentypetest(SHORT+"typetest.py")
    scanner.close()
    print "=== Testing definitions output code ==="
    execfile(defsoutput, {}, {})
    print "=== Done scanning and generating, now importing the generated code... ==="
    exec "import " + SHORT + "support"
    print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner_OSX):

    def destination(self, type, name, arglist):
        classname = "Function"
        listname = "functions"
        if arglist:
            t, n, m = arglist[0]
            if t in OBJECTS and m == "InMode":
                classname = "Method"
                listname = t + "_methods"
            # Special case for the silly first AllocatorRef argument
            if t == 'CFAllocatorRef' and m == 'InMode' and len(arglist) > 1:
                t, n, m = arglist[1]
                if t in OBJECTS and m == "InMode":
                    classname = "MethodSkipArg1"
                    listname = t + "_methods"
        return classname, listname

    def writeinitialdefs(self):
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")

    def makeblacklistnames(self):
        return [
                "CGContextRetain",
                "CGContextRelease",
                ]

    def makegreylist(self):
        return []

    def makeblacklisttypes(self):
        return [
                "float_ptr",
                "CGRect_ptr",
                "CGPoint_ptr",
                "CGColorSpaceRef",
                "CGColorRenderingIntent",
                "CGFontRef",
#                       "char_ptr",
                "CGGlyph_ptr",
                "CGImageRef",
                "CGPDFDocumentRef",
                ]

    def makerepairinstructions(self):
        return [
                ([("char_ptr", "cstring", "InMode"), ("size_t", "length", "InMode")],
                 [("InBuffer", "*", "*")]),
#                       ([("char_ptr", "name", "InMode"),],
#                        [("CCCCC", "*", "*")]),
                ]

if __name__ == "__main__":
    main()
