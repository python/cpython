# Scan an Apple header file, generating a Python file of generator calls.

import sys
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner_OSX

LONG = "Folders"
SHORT = "folder"
OBJECT = "NOTUSED"

def main():
    input = LONG + ".h"
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

class MyScanner(Scanner_OSX):

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

    def makeblacklistnames(self):
        return [
                "FindFolderExtended", # Has funny void* argument
                "FSFindFolderExtended", # ditto
                "FolderManagerRegisterCallNotificationProcs", # ditto

                "FindFolderEx", # Non-MacOS routine
                ]

    def makeblacklisttypes(self):
        return [
                "FolderManagerNotificationProcPtr",
                "FolderManagerNotificationUPP",
                "FolderRouting", # To be done, not difficult
                "FolderDesc", # To be done, not difficult

                ]

    def makerepairinstructions(self):
        return [
                ]

    def writeinitialdefs(self):
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
        self.defsfile.write("true = True\n")
        self.defsfile.write("false = False\n")

if __name__ == "__main__":
    main()
