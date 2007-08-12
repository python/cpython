# Scan an Apple header file, generating a Python file of generator calls.
#
# Note that the scrap-manager include file is so weird that this
# generates a boilerplate to be edited by hand.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner

LONG = "Scrap"
SHORT = "scrap"

def main():
    input = "Scrap.h"
    output = SHORT + "gen.py"
    defsoutput = "@Scrap.py"
    scanner = MyScanner(input, output, defsoutput)
    scanner.scan()
    scanner.close()
##      print "=== Testing definitions output code ==="
##      exec(open(defsoutput).read(), {}, {})
    print "=== Done scanning and generating, now importing the generated code... ==="
    exec "import " + SHORT + "support"
    print "=== Done.  It's up to you to compile it now! ==="

class MyScanner(Scanner):

    def destination(self, type, name, arglist):
        classname = "Function"
        listname = "functions"
        if arglist:
            t, n, m = arglist[0]
            if t == 'ScrapRef' and m == "InMode":
                classname = "Method"
                listname = "methods"
        return classname, listname

    def makeblacklistnames(self):
        return [
                "GetScrapFlavorInfoList",
                'InfoScrap',
                'GetScrap',
                'ZeroScrap',
                'PutScrap',
                ]

    def makeblacklisttypes(self):
        return [
                'ScrapPromiseKeeperUPP',
                ]

    def makerepairinstructions(self):
        return [
                ([('void', '*', 'OutMode')], [('putscrapbuffer', '*', 'InMode')]),
                ([('void_ptr', '*', 'InMode')], [('putscrapbuffer', '*', 'InMode')]),
                ]

if __name__ == "__main__":
    main()
