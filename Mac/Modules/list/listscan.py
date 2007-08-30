# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner

LONG = "Lists"
SHORT = "list"
OBJECT = "ListHandle"

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
            if t in ('ListHandle', 'ListRef') and m == "InMode":
                classname = "Method"
                listname = "methods"
        return classname, listname

    def makeblacklistnames(self):
        return [
                "LDispose",             # Done by removing the object
                "LSearch",              # We don't want to handle procs just yet
                "CreateCustomList",  # done manually
                "SetListDefinitionProc",

                # These have funny argument/return values
                "GetListViewBounds",
                "GetListCellIndent",
                "GetListCellSize",
                "GetListVisibleCells",
                "GetListClickLocation",
                "GetListMouseLocation",
                "GetListDataBounds",
                "SetListLastClick",
                ]

    def makeblacklisttypes(self):
        return [
                "ListClickLoopUPP",  # Too difficult for now
                "ListDefSpecPtr", # later
                ]

    def makerepairinstructions(self):
        return [
                ([('ListBounds_ptr', '*', 'InMode')],
                 [('Rect_ptr', '*', 'InMode')]),

                ([("Cell", "theCell", "OutMode")],
                 [("Cell", "theCell", "InOutMode")]),

                ([("void_ptr", "*", "InMode"), ("short", "*", "InMode")],
                 [("InBufferShortsize", "*", "*")]),

                ([("void", "*", "OutMode"), ("short", "*", "OutMode")],
                 [("VarOutBufferShortsize", "*", "InOutMode")]),

                # SetListCellIndent doesn't have const
                ([("Point", "indent", "OutMode")],
                 [("Point_ptr", "indent", "InMode")]),

                ]

    def writeinitialdefs(self):
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")


if __name__ == "__main__":
    main()
