# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner

WASTEDIR='/Users/jack/src/waste/C_C++ Headers/'

if not os.path.exists(WASTEDIR):
    raise 'Error: not found: %s', WASTEDIR

OBJECT = "TEHandle"
SHORT = "waste"
OBJECT = "WEReference"
OBJECT2 = "WEObjectReference"

def main():
    input = WASTEDIR + "WASTE.h"
    output = SHORT + "gen.py"
    defsoutput = os.path.join(os.path.split(TOOLBOXDIR)[0], "WASTEconst.py")
    scanner = MyScanner(input, output, defsoutput)
    scanner.scan()
##      scanner.gentypetest(SHORT+"typetest.py")
    scanner.close()
    print "=== Testing definitions output code ==="
    execfile(defsoutput, {}, {})
    print "=== Done scanning and generating, now importing the generated code... ==="
    exec "import " + SHORT + "support"
    print "=== Done.  It's up to you to compile it now! ==="

#class MyScanner(Scanner_PreUH3):
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
            else:
                t, n, m = arglist[0]
                if t == OBJECT2 and m == "InMode":
                    classname = "Method2"
                    listname = "methods2"
        return classname, listname

    def writeinitialdefs(self):
        self.defsfile.write("kPascalStackBased = None # workaround for header parsing\n")
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")

    def makeblacklistnames(self):
        return [
                "WEDispose",
                "WESetInfo", # Argument type unknown...
                "WEGetInfo",
                "WEVersion", # Unfortunately...
                "WEPut", # XXXX TBD: needs array of flavortypes.
                "WEGetOneAttribute", # XXXX TBD: output buffer
                # Incompatible constant definitions
                "weDoAutoScroll",
                "weDoOutlineHilite",
                "weDoReadOnly",
                "weDoUndo",
                "weDoIntCutAndPaste",
                "weDoDragAndDrop",
                "weDoInhibitRecal",
                "weDoUseTempMem",
                "weDoDrawOffscreen",
                "weDoInhibitRedraw",
                "weDoMonoStyled",
                "weDoMultipleUndo",
                "weDoNoKeyboardSync",
                "weDoInhibitICSupport",
                "weDoInhibitColor",
                ]

    def makeblacklisttypes(self):
        return [
                "DragReference",        # For now...
                "UniversalProcPtr",
                "WEFontIDToNameUPP",
                "WEFontNameToIDUPP",
                "WEClickLoopUPP",
                "WEScrollUPP",
                "WETSMPreUpdateUPP",
                "WETSMPostUpdateUPP",
                "WEPreTrackDragUPP",
                "WETranslateDragUPP",
                "WEHiliteDropAreaUPP",
                "WEDrawTextUPP",
                "WEDrawTSMHiliteUPP",
                "WEPixelToCharUPP",
                "WECharToPixelUPP",
                "WELineBreakUPP",
                "WEWordBreakUPP",
                "WECharByteUPP",
                "WECharTypeUPP",
                "WEEraseUPP",
                "WEFluxUPP",
                "WENewObjectUPP",
                "WEDisposeObjectUPP",
                "WEDrawObjectUPP",
                "WEClickObjectUPP",
                "WEStreamObjectUPP",
                "WEHoverObjectUPP",
                "WERuler",              # XXXX To be done
                "WERuler_ptr",  # ditto
                "WEParaInfo",   # XXXX To be done
                "WEPrintSession",       # XXXX To be done
                "WEPrintOptions_ptr", # XXXX To be done
                ]

    def makerepairinstructions(self):
        return [
                ([("void_ptr", "*", "InMode"), ("SInt32", "*", "InMode")],
                 [("InBuffer", "*", "*")]),

                # WEContinuousStyle
                ([("WEStyleMode", "ioMode", "OutMode"), ("TextStyle", "outTextStyle", "OutMode")],
                 [("WEStyleMode", "*", "InOutMode"), ("TextStyle", "*", "*")]),

                # WECopyRange
                ([('Handle', 'outText', 'InMode'), ('StScrpHandle', 'outStyles', 'InMode'),
                ('WESoupHandle', 'outSoup', 'InMode')],
         [('OptHandle', '*', '*'), ('OptStScrpHandle', '*', '*'),
                ('OptSoupHandle', '*', '*')]),

                # WEInsert
                ([('StScrpHandle', 'inStyles', 'InMode'), ('WESoupHandle', 'inSoup', 'InMode')],
         [('OptStScrpHandle', '*', '*'), ('OptSoupHandle', '*', '*')]),

        # WEGetObjectOwner
        ("WEGetObjectOwner",
         [('WEReference', '*', 'ReturnMode')],
         [('ExistingWEReference', '*', 'ReturnMode')]),

        # WEFindParagraph
        ([("char_ptr", "inKey", "InMode")],
         [("stringptr", "*", "*")]),

                # WESetOneAttribute
                ([("void_ptr", "*", "InMode"), ("ByteCount", "*", "InMode")],
                 [("InBuffer", "*", "*")]),
                ]

if __name__ == "__main__":
    main()
