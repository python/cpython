# Scan an Apple header file, generating a Python file of generator calls.

import sys
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)

from scantools import Scanner

LONG = "Dialogs"
SHORT = "dlg"
OBJECT = "DialogPtr"

def main():
    input = LONG + ".h"
    output = SHORT + "gen.py"
    defsoutput = TOOLBOXDIR + LONG + ".py"
    scanner = MyScanner(input, output, defsoutput)
    scanner.scan()
    scanner.close()
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
            if t in ("DialogPtr", "DialogRef") and m == "InMode":
                classname = "Method"
                listname = "methods"
        return classname, listname

    def makeblacklistnames(self):
        return [
                'InitDialogs',
                'ErrorSound',
                # Dialogs are disposed when the object is deleted
                'CloseDialog',
                'DisposDialog',
                'DisposeDialog',
                'UpdtDialog',
                'CouldAlert',
                'FreeAlert',
                'CouldDialog',
                'FreeDialog',
                'GetStdFilterProc',
                'GetDialogParent',
##                      # Can't find these in the CW Pro 3 libraries
                'SetDialogMovableModal',
                'GetDialogControlNotificationProc',
                'SetGrafPortOfDialog', # Funny, and probably not useful
                # Can't find these:
                'CloseStandardSheet',
                'RunStandardAlert',
                ]

    def makeblacklisttypes(self):
        return [
                "AlertStdAlertParamPtr",        # Too much work, for now
                "AlertStdAlertParamRec",        # ditto
                "AlertStdAlertParamRec_ptr",    # ditto
                "AlertStdCFStringAlertParamPtr",        # ditto
                "AlertStdCFStringAlertParamRec",
                "AlertStdCFStringAlertParamRec_ptr",
                "QTModelessCallbackProcPtr",
                ]

    def makerepairinstructions(self):
        return [
                ([("Str255", "*", "InMode")],
                 [("*", "*", "OutMode")]),

                ([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
                 [("InBuffer", "*", "*")]),

                ([("void", "*", "OutMode"), ("long", "*", "InMode"),
                                            ("long", "*", "OutMode")],
                 [("VarVarOutBuffer", "*", "InOutMode")]),

                # GetDialogItem return handle is optional
                ([("Handle", "item", "OutMode")],
                 [("OptHandle", "item", "OutMode")]),

                # NewDialog ETC.
                ([("void", "*", "OutMode")],
                 [("NullStorage", "*", "InMode")]),

                ([("DialogPtr", "*", "OutMode")],
                 [("ExistingDialogPtr", "*", "*")]),
                ([("DialogRef", "*", "OutMode")],
                 [("ExistingDialogPtr", "*", "*")]),
                ([("WindowPtr", "*", "OutMode")],
                 [("ExistingWindowPtr", "*", "*")]),
                ([("WindowPtr", "*", "ReturnMode")],
                 [("ExistingWindowPtr", "*", "*")]),

                # StdFilterProc
                ([('EventRecord', 'event', 'OutMode'),
                  ('DialogItemIndex', 'itemHit', 'OutMode')],
                 [('EventRecord', 'event', 'InOutMode'),
                  ('DialogItemIndex', 'itemHit', 'InOutMode')])

                ]

    def writeinitialdefs(self):
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")


if __name__ == "__main__":
    main()
