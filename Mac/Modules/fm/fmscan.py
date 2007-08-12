# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner

LONG = "Fonts"
SHORT = "fm"

def main():
    input = "Fonts.h"
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
        return classname, listname

    def makeblacklistnames(self):
        return [
                "OutlineMetrics",       # Too complicated
                "AntiTextIsAntiAliased",        # XXXX Missing from library...
                "AntiTextGetEnabled",
                "AntiTextSetEnabled",
                "AntiTextGetApplicationAware",
                "AntiTextSetApplicationAware",
                # These are tricky: they're not Carbon dependent or anything, but they
                # exist only on 8.6 or later (both in Carbon and Classic).
                # Disabling them is the easiest path.
                'SetAntiAliasedTextEnabled',
                'IsAntiAliasedTextEnabled',
                # OS8-only
                'InitFonts',
                'SetFontLock',
                'FlushFonts',
                ]

    def makeblacklisttypes(self):
        return [
                "FMInput_ptr",  # Not needed for now
                "FMOutPtr",             # Ditto
##                      "void_ptr",             # Don't know how to do this right now
                "FontInfo",             # Ditto
                ]

    def makerepairinstructions(self):
        return [
                ([('Str255', '*', 'InMode')], [('Str255', '*', 'OutMode')]),
                ([('FMetricRecPtr', 'theMetrics', 'InMode')], [('FMetricRecPtr', 'theMetrics', 'OutMode')]),
                ([('short', 'byteCount', 'InMode'), ('void_ptr', 'textAddr', 'InMode'),],
                 [('TextBuffer', 'inText', 'InMode')]),
                ]

    def writeinitialdefs(self):
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
        self.defsfile.write("kNilOptions = 0\n")

if __name__ == "__main__":
    main()
