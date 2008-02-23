# Scan an Apple header file, generating a Python file of generator calls.

import sys
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner

LONG = "TextEdit"
SHORT = "te"
OBJECT = "TEHandle"

def main():
    input = LONG + ".h"
    output = SHORT + "gen.py"
    defsoutput = TOOLBOXDIR + LONG + ".py"
    scanner = MyScanner(input, output, defsoutput)
    scanner.scan()
    scanner.close()
    print("=== Testing definitions output code ===")
    exec(open(defsoutput).read(), {}, {})
    print("=== Done scanning and generating, now importing the generated code... ===")
    exec "import " + SHORT + "support"
    print("=== Done.  It's up to you to compile it now! ===")

class MyScanner(Scanner):

    def destination(self, type, name, arglist):
        classname = "Function"
        listname = "functions"
        if arglist:
            t, n, m = arglist[-1]
            # This is non-functional today
            if t == OBJECT and m == "InMode":
                classname = "Method"
                listname = "methods"
        return classname, listname

    def makeblacklistnames(self):
        return [
                "TEDispose",
                "TEInit",
##                      "TEGetHiliteRgn",
                ]

    def makeblacklisttypes(self):
        return [
                "TEClickLoopUPP",
                "UniversalProcPtr",
                "WordBreakUPP",
                "TEDoTextUPP",
                "TERecalcUPP",
                "TEFindWordUPP",
                ]

    def makerepairinstructions(self):
        return [
                ([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
                 [("InBuffer", "*", "*")]),

                # TEContinuousStyle
                ([("short", "mode", "OutMode"), ("TextStyle", "aStyle", "OutMode")],
                 [("short", "mode", "InOutMode"), ("TextStyle", "aStyle", "InOutMode")])
                ]

if __name__ == "__main__":
    main()
