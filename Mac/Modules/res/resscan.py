# Scan Resources.h header file, generate resgen.py and Resources.py files.
# Then run ressupport to generate Resmodule.c.
# (Should learn how to tell the compiler to compile it as well.)

import sys
import MacOS

from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)

from scantools import Scanner

def main():
    input = "Resources.h"
    output = "resgen.py"
    defsoutput = TOOLBOXDIR + "Resources.py"
    scanner = ResourcesScanner(input, output, defsoutput)
    scanner.scan()
    scanner.close()
    print "=== Testing definitions output code ==="
    execfile(defsoutput, {}, {})
    print "=== Done scanning and generating, now doing 'import ressupport' ==="
    import ressupport
    print "=== Done 'import ressupport'.  It's up to you to compile Resmodule.c ==="

class ResourcesScanner(Scanner):

    def destination(self, type, name, arglist):
        classname = "ResFunction"
        listname = "functions"
        if arglist:
            t, n, m = arglist[0]
            if t == "Handle" and m == "InMode":
                classname = "ResMethod"
                listname = "resmethods"
        return classname, listname

    def makeblacklistnames(self):
        return [
                "ReadPartialResource",
                "WritePartialResource",
                "TempInsertROMMap",
##                      "RmveResource",         # RemoveResource
##                      "SizeResource",         # GetResourceSizeOnDisk
##                      "MaxSizeRsrc",          # GetMaxResourceSize
                # OS8 only
                'RGetResource',
                'OpenResFile',
                'CreateResFile',
                'RsrcZoneInit',
                'InitResources',
                'RsrcMapEntry',
                ]

    def makeblacklisttypes(self):
        return [
                ]

    def makerepairinstructions(self):
        return [
                ([("Str255", "*", "InMode")],
                 [("*", "*", "OutMode")]),

                ([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
                 [("InBuffer", "*", "*")]),

                ([("void", "*", "OutMode"), ("long", "*", "InMode")],
                 [("InOutBuffer", "*", "*")]),

                ([("void", "*", "OutMode"), ("long", "*", "InMode"),
                                            ("long", "*", "OutMode")],
                 [("OutBuffer", "*", "InOutMode")]),

                ([("SInt8", "*", "*")],
                 [("SignedByte", "*", "*")]),


                ([("UniCharCount", "*", "InMode"), ("UniChar_ptr", "*", "InMode")],
                 [("UnicodeReverseInBuffer", "*", "*")]),
                ]

if __name__ == "__main__":
    main()
