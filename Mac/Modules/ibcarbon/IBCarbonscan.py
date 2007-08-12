# IBCarbonscan.py

import sys
import os
import string

from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)

from scantools import Scanner_OSX

def main():
    print "---Scanning IBCarbonRuntime.h---"
    input = ["IBCarbonRuntime.h"]
    output = "IBCarbongen.py"
    defsoutput = TOOLBOXDIR + "IBCarbonRuntime.py"
    scanner = IBCarbon_Scanner(input, output, defsoutput)
    scanner.scan()
    scanner.close()
    print "=== Testing definitions output code ==="
    exec(open(defsoutput).read(), {}, {})
    print "--done scanning, importing--"
    import IBCarbonsupport
    print "done"

class IBCarbon_Scanner(Scanner_OSX):

    def destination(self, type, name, arglist):
        classname = "IBCarbonFunction"
        listname = "functions"
        if arglist:
            t, n, m = arglist[0]
            if t == "IBNibRef" and m == "InMode":
                classname = "IBCarbonMethod"
                listname = "methods"
        return classname, listname

    def makeblacklistnames(self):
        return [
                "DisposeNibReference",                          # taken care of by destructor
                "CreateNibReferenceWithCFBundle",  ## need to wrap CFBundle.h properly first
                ]

    def makerepairinstructions(self):
        return []


if __name__ == "__main__":
    main()
