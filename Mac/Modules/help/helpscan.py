# Scan an Apple header file, generating a Python file of generator calls.

import sys
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner

LONG = "MacHelp"
SHORT = "help"
OBJECT = "NOTUSED"

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
                ]

    def makeblacklisttypes(self):
        return [
##                      "TipFunctionUPP",
##                      "HMMessageRecord",
##                      "HMMessageRecord_ptr",
                "HMWindowContentUPP",
                "HMMenuTitleContentUPP",
                "HMControlContentUPP",
                "HMMenuItemContentUPP",
                # For the moment
                "HMHelpContentRec",
                "HMHelpContentRec_ptr",
                ]

    def makerepairinstructions(self):
        return [
##                      ([("WindowPtr", "*", "OutMode")],
##                       [("ExistingWindowPtr", "*", "*")]),
                ]

if __name__ == "__main__":
    main()
