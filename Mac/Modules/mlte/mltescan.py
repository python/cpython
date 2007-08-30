# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner_OSX

LONG = "MacTextEditor"
SHORT = "mlte"
OBJECTS = ("TXNObject", "TXNFontMenuObject")
# ADD object typenames here

def main():
    input = "MacTextEditor.h"
    output = SHORT + "gen.py"
    defsoutput = TOOLBOXDIR + LONG + ".py"
    scanner = MyScanner(input, output, defsoutput)
    scanner.scan()
    scanner.gentypetest(SHORT+"typetest.py")
    scanner.close()
    print("=== Testing definitions output code ===")
    exec(open(defsoutput).read(), {}, {})
    print("=== Done scanning and generating, now importing the generated code... ===")
    exec "import " + SHORT + "support"
    print("=== Done.  It's up to you to compile it now! ===")

class MyScanner(Scanner_OSX):

    def destination(self, type, name, arglist):
        classname = "Function"
        listname = "functions"
        if arglist:
            t, n, m = arglist[0]
            if t in OBJECTS and m == "InMode":
                classname = "Method"
                listname = t + "_methods"
        return classname, listname

    def writeinitialdefs(self):
        self.defsfile.write("""
def FOUR_CHAR_CODE(x): return x
false = 0
true = 1
kTXNClearThisControl = 0xFFFFFFFF
kTXNClearTheseFontFeatures = 0x80000000
kTXNDontCareTypeSize = 0xFFFFFFFF
kTXNDecrementTypeSize = 0x80000000
kTXNUseCurrentSelection = 0xFFFFFFFF
kTXNStartOffset = 0
kTXNEndOffset = 0x7FFFFFFF
MovieFileType = FOUR_CHAR_CODE('moov')
kTXNUseEncodingWordRulesMask = 0x80000000
kTXNFontSizeAttributeSize = 4
normal = 0
""")

    def makeblacklistnames(self):
        return [
                "TXNGetFontDefaults", # Arg is too difficult
                "TXNSetFontDefaults", # Arg is too difficult
                "TXNInitTextension", # done manually

                # Constants with funny definitions
                "kTXNClearThisControl",
                "kTXNClearTheseFontFeatures",
                "kTXNDontCareTypeSize",
                "kTXNDecrementTypeSize",
                "kTXNUseCurrentSelection",
                "kTXNStartOffset",
                "kTXNEndOffset",
                "kTXNQDFontNameAttributeSize",
                "kTXNQDFontFamilyIDAttributeSize",
                "kTXNQDFontSizeAttributeSize",
                "kTXNQDFontStyleAttributeSize",
                "kTXNQDFontColorAttributeSize",
                "kTXNTextEncodingAttributeSize",
                "kTXNUseEncodingWordRulesMask",
                "kTXNFontSizeAttributeSize",
                "status",
                "justification",
                'TXNTSMCheck', # OS8
                ]

    def makeblacklisttypes(self):
        return [
                "TXNTab", # TBD
                "TXNMargins", # TBD
                "TXNControlData", #TBD
                "TXNATSUIFeatures", #TBD
                "TXNATSUIVariations", #TBD
                "TXNAttributeData", #TBD
                "TXNTypeAttributes", #TBD
                "TXNMatchTextRecord", #TBD
                "TXNBackground", #TBD
                "TXNFindUPP",
                "ATSUStyle", #TBD
                "TXNBackground_ptr", #TBD
                "TXNControlData_ptr", #TBD
                "TXNControlTag_ptr", #TBD
                "TXNLongRect", #TBD
                "TXNLongRect_ptr", #TBD
                "TXNTypeAttributes_ptr", #TBD

                "TXNActionKeyMapperProcPtr",
                "TXNActionKeyMapperUPP",
                "TXNTextBoxOptionsData",
                "TXNCountOptions",
                "void_ptr",
                ]

    def makerepairinstructions(self):
        return [
                # TXNNewObject has a lot of optional parameters
                ([("FSSpec_ptr", "iFileSpec", "InMode")],
                 [("OptFSSpecPtr", "*", "*")]),
                ([("Rect", "iFrame", "OutMode")],
                 [("OptRectPtr", "*", "InMode")]),

                # In UH 332 some of the "const" are missing for input parameters passed
                # by reference. We fix that up here.
                ([("EventRecord", "iEvent", "OutMode")],
                 [("EventRecord_ptr", "*", "InMode")]),
                ([("FSSpec", "iFileSpecification", "OutMode")],
                 [("FSSpec_ptr", "*", "InMode")]),
                ([("TXNMacOSPreferredFontDescription", "iFontDefaults", "OutMode")],
                 [("TXNMacOSPreferredFontDescription_ptr", "*", "InMode")]),

                # In buffers are passed as void *
                ([("void", "*", "OutMode"), ("ByteCount", "*", "InMode")],
                 [("MlteInBuffer", "*", "InMode")]),

                # The AdjustCursor region handle is optional
                ([("RgnHandle", "ioCursorRgn", "InMode")],
                 [("OptRgnHandle", "*", "*")]),

                # The GWorld for TXNDraw is optional
                ([('GWorldPtr', 'iDrawPort', 'InMode')],
                 [('OptGWorldPtr', '*', '*')]),
                ]

if __name__ == "__main__":
    main()
