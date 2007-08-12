# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner

LONG = "Appearance"
SHORT = "app"
OBJECT = "ThemeDrawingState"

def main():
    input = LONG + ".h"
    output = SHORT + "gen.py"
    defsoutput = TOOLBOXDIR + LONG + ".py"
    scanner = MyScanner(input, output, defsoutput)
    scanner.scan()
    scanner.close()
    print "=== Testing definitions output code ==="
    exec(open(defsoutput).read(), {}, {})
    print "=== Done scanning and generating, now importing the generated code... ==="
    exec "import " + SHORT + "support"
    print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner):

    def destination(self, type, name, arglist):
        classname = "Function"
        listname = "functions"
        if arglist:
            t, n, m = arglist[0]
            # This is non-functional today
            if t == OBJECT and m == "InMode":
                classname = "Method"
                listname = "methods"
        return classname, listname

    def writeinitialdefs(self):
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")

    def makeblacklistnames(self):
        return [
                "GetThemeFont",         # Funny stringbuffer in/out parameter, I think...
                # Constants with funny definitions
                "appearanceBadBrushIndexErr",
                "appearanceProcessRegisteredErr",
                "appearanceProcessNotRegisteredErr",
                "appearanceBadTextColorIndexErr",
                "appearanceThemeHasNoAccents",
                "appearanceBadCursorIndexErr",
                ]

    def makeblacklisttypes(self):
        return [
                "MenuTitleDrawingUPP",
                "MenuItemDrawingUPP",
                "ThemeIteratorUPP",
                "ThemeTabTitleDrawUPP",
#                       "ThemeEraseUPP",
#                       "ThemeButtonDrawUPP",
                "WindowTitleDrawingUPP",
                "ProcessSerialNumber_ptr",              # Too much work for now.
                "ThemeTrackDrawInfo_ptr",       # Too much work
#                       "ThemeButtonDrawInfo_ptr",      # ditto
                "ThemeWindowMetrics_ptr",       # ditto
#                       "ThemeDrawingState",    # This is an opaque pointer, so it should be simple. Later.
                "Collection",           # No interface to collection mgr yet.
                "BytePtr",              # Not yet.
                ]

    def makerepairinstructions(self):
        return [
                ([("void", 'inContext', "OutMode")],
                 [("NULL", 'inContext', "InMode")]),
                ([("Point", 'ioBounds', "OutMode")],
                 [("Point", 'ioBounds', "InOutMode")]),
                ]

if __name__ == "__main__":
    main()
