# Scan <Drag.h>, generating draggen.py.
import sys
from bgenlocations import TOOLBOXDIR, BGENDIR, INCLUDEDIR
sys.path.append(BGENDIR)

from scantools import Scanner

MISSING_DEFINES="""
kDragHasLeftSenderWindow        = (1 << 0)
kDragInsideSenderApplication = (1 << 1)
kDragInsideSenderWindow         = (1 << 2)
kDragRegionAndImage                     = (1 << 4)
flavorSenderOnly                        = (1 << 0)
flavorSenderTranslated          = (1 << 1)
flavorNotSaved                          = (1 << 2)
flavorSystemTranslated          = (1 << 8)
"""


def main():
    input = INCLUDEDIR + "Drag.h"
    output = "draggen.py"
    defsoutput = TOOLBOXDIR + "Dragconst.py"
    scanner = MyScanner(input, output, defsoutput)
    scanner.scan()
    scanner.close()
    print("=== Testing definitions output code ===")
    exec(open(defsoutput).read(), {}, {})
    print("=== Done scanning and generating, now doing 'import dragsupport' ===")
    import dragsupport
    print("=== Done.  It's up to you to compile Dragmodule.c ===")

class MyScanner(Scanner):

    def destination(self, type, name, arglist):
        classname = "Function"
        listname = "functions"
        if arglist:
            t, n, m = arglist[0]
            if t in ('DragReference', 'DragRef') and m == "InMode":
                classname = "Method"
                listname = "methods"
        return classname, listname

    def writeinitialdefs(self):
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
        self.defsfile.write("from Carbon.TextEdit import *\n")
        self.defsfile.write("from Carbon.QuickDraw import *\n")
        self.defsfile.write("fkDragActionAll = -1\n")
        self.defsfile.write("\n")
        # Defines unparseable in Drag.h
        self.defsfile.write(MISSING_DEFINES)

    def makeblacklistnames(self):
        return [
                "kDragActionAll",
                ]

    def makeblacklisttypes(self):
        return [
                "DragTrackingHandlerUPP",
                "DragReceiveHandlerUPP",
                "DragSendDataUPP",
                "DragInputUPP",
                "DragDrawingUPP",
                ]

    def makerepairinstructions(self):
        return [
                ([("void_ptr", "*", "InMode"), ("Size", "*", "InMode")],
                 [("OptionalInBuffer", "*", "*")]),

                ([("void", "*", "OutMode"), ("Size", "*", "OutMode")],
                 [("VarOutBuffer", "*", "InOutMode")]),

                ]

if __name__ == "__main__":
    main()
