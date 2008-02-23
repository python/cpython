# Scan an Apple header file, generating a Python file of generator calls.

import sys
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner

LONG = "OSAconst"
SHORT = "osa"

def main():
    input = "OSA.h"
    output = SHORT + "gen.py"
    defsoutput = TOOLBOXDIR + LONG + ".py"
    scanner = MyScanner(input, output, defsoutput)
    scanner.scan()
    scanner.close()
    scanner.gentypetest(SHORT+"typetest.py")
    print "=== Testing definitions output code ==="
    execfile(defsoutput, {}, {})
    print "=== Done scanning and generating, now importing the generated code... ==="
    exec "import " + SHORT + "support"
    print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner):

    def destination(self, type, name, arglist):
        classname = "Function"
        listname = "functions"
        if arglist:
            t, n, m = arglist[0]
            if t == "ComponentInstance" and m == "InMode":
                classname = "Method"
                listname = "methods"
        return classname, listname

    def writeinitialdefs(self):
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
        self.defsfile.write("from Carbon.AppleEvents import *\n")
        self.defsfile.write("kAEUseStandardDispatch = -1\n")

    def makeblacklistnames(self):
        return [
                "OSACopyScript",
                ]

    def makeblacklisttypes(self):
        return [
                "OSALocalOrGlobal",
                "OSACreateAppleEventUPP",
                "OSAActiveUPP",
                "AEEventHandlerUPP",
                "OSASendUPP",
                ]

    def makerepairinstructions(self):
        return [
                ]

if __name__ == "__main__":
    main()
