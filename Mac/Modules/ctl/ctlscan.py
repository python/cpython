# Scan <Controls.h>, generating ctlgen.py.
import sys
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)

from scantools import Scanner

def main():
#       input = "Controls.h" # Universal Headers < 3.3
    input = ["Controls.h", "ControlDefinitions.h"] # Universal Headers >= 3.3
    output = "ctlgen.py"
    defsoutput = TOOLBOXDIR + "Controls.py"
    scanner = MyScanner(input, output, defsoutput)
    scanner.scan()
    scanner.close()
    print "=== Testing definitions output code ==="
    execfile(defsoutput, {}, {})
    print "=== Done scanning and generating, now doing 'import ctlsupport' ==="
    import ctlsupport
    print "=== Done.  It's up to you to compile Ctlmodule.c ==="

class MyScanner(Scanner):

    def destination(self, type, name, arglist):
        classname = "Function"
        listname = "functions"
        if arglist:
            t, n, m = arglist[0]
            if t in ("ControlHandle", "ControlRef") and m == "InMode":
                classname = "Method"
                listname = "methods"
        return classname, listname

    def writeinitialdefs(self):
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
        self.defsfile.write("from Carbon.TextEdit import *\n")
        self.defsfile.write("from Carbon.QuickDraw import *\n")
        self.defsfile.write("from Carbon.Dragconst import *\n")
        self.defsfile.write("from Carbon.CarbonEvents import *\n")
        self.defsfile.write("from Carbon.Appearance import *\n")
        self.defsfile.write("kDataBrowserItemAnyState = -1\n")
        self.defsfile.write("kControlBevelButtonCenterPopupGlyphTag = -1\n")
        self.defsfile.write("kDataBrowserClientPropertyFlagsMask = 0xFF000000\n")
        self.defsfile.write("\n")

    def makeblacklistnames(self):
        return [
                'FindControlUnderMouse', # Generated manually, returns an existing control, not a new one.
                'DisposeControl', # Generated manually
                'KillControls', # Implied by close of dialog
                'SetCtlAction',
                'TrackControl', # Generated manually
                'HandleControlClick',   # Generated manually
                'SetControlData',       # Generated manually
                'GetControlData',       # Generated manually
                'kControlBevelButtonCenterPopupGlyphTag', # Constant with funny definition
                'kDataBrowserClientPropertyFlagsMask',  # ditto
                'kDataBrowserItemAnyState',   # and ditto
                # The following are unavailable for static 68k (appearance manager)
##                      'GetBevelButtonMenuValue',
##                      'SetBevelButtonMenuValue',
##                      'GetBevelButtonMenuHandle',
##                      'SetBevelButtonTransform',
                'SetBevelButtonGraphicAlignment',
                'SetBevelButtonTextAlignment',
                'SetBevelButtonTextPlacement',
##                      'SetImageWellTransform',
##                      'GetTabContentRect',
##                      'SetTabEnabled',
##                      'SetDisclosureTriangleLastValue',
##                      # Unavailable in CW Pro 3 libraries
##                      'SetUpControlTextColor',
##                      # Unavailable in Jack's CW Pro 5.1 libraries
##                      'GetControlRegion',
##                      'RemoveControlProperty',
##                      'IsValidControlHandle',
##                      'SetControl32BitMinimum',
##                      'GetControl32BitMinimum',
##                      'SetControl32BitMaximum',
##                      'GetControl32BitMaximum',
##                      'SetControl32BitValue',
##                      'GetControl32BitValue',
##                      'SetControlViewSize',
##                      'GetControlViewSize',
                # Generally Bad News
                'GetControlProperty',
                'SetControlProperty',
                'GetControlPropertySize',
                'SendControlMessage', # Parameter changed from long to void* from UH3.3 to UH3.4
                'CreateTabsControl',  # wrote manually
                'GetControlAction',  # too much effort for too little usefulness

                # too lazy for now
                'GetImageWellContentInfo',
                'GetBevelButtonContentInfo',
                # OS8 only
                'GetAuxiliaryControlRecord',
                'SetControlColor',
                ]

    def makeblacklisttypes(self):
        return [
                'ProcPtr',
#                       'ControlActionUPP',
                'Ptr',
                'ControlDefSpec', # Don't know how to do this yet
                'ControlDefSpec_ptr', # ditto
                'Collection', # Ditto
                # not-yet-supported stuff in Universal Headers 3.4:
                'ControlColorUPP',
                'ControlKind',  # XXX easy: 2-tuple containing 2 OSType's
#                       'ControlTabEntry_ptr', # XXX needed for tabs
#                       'ControlButtonContentInfoPtr',
#                       'ControlButtonContentInfo',  # XXX ugh: a union
#                       'ControlButtonContentInfo_ptr',  # XXX ugh: a union
                'ListDefSpec_ptr',  # XXX see _Listmodule.c, tricky but possible
                'DataBrowserItemID_ptr',  # XXX array of UInt32, for BrowserView
                'DataBrowserItemUPP',
                'DataBrowserItemDataRef', # XXX void *
                'DataBrowserCallbacks', # difficult struct
                'DataBrowserCallbacks_ptr',
                'DataBrowserCustomCallbacks',
                'DataBrowserCustomCallbacks_ptr',
##                      'DataBrowserTableViewColumnDesc',
##                      'DataBrowserListViewColumnDesc',
                'CFDataRef',
                'DataBrowserListViewHeaderDesc', # difficult struct
                ]

    def makerepairinstructions(self):
        return [
                ([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
                 [("InBuffer", "*", "*")]),

                ([("void", "*", "OutMode"), ("long", "*", "InMode"),
                                            ("long", "*", "OutMode")],
                 [("VarVarOutBuffer", "*", "InOutMode")]),

##                      # For TrackControl
##                      ([("ProcPtr", "actionProc", "InMode")],
##                       [("FakeType('(ControlActionUPP)0')", "*", "*")]),
##                      ([("ControlActionUPP", "actionProc", "InMode")],
##                       [("FakeType('(ControlActionUPP)0')", "*", "*")]),

                # For GetControlTitle
                ([('Str255', 'title', 'InMode')],
                 [('Str255', 'title', 'OutMode')]),

                ([("ControlHandle", "*", "OutMode")],
                 [("ExistingControlHandle", "*", "*")]),
                ([("ControlRef", "*", "OutMode")],      # Ditto, for Universal Headers
                 [("ExistingControlHandle", "*", "*")]),

                ([("Rect_ptr", "*", "ReturnMode")], # GetControlBounds
                 [("void", "*", "ReturnMode")]),

                ([("DataBrowserListViewColumnDesc", "*", "OutMode")],
                 [("DataBrowserListViewColumnDesc", "*", "InMode")]),

                ([("ControlButtonContentInfoPtr", 'outContent', "InMode")],
                 [("ControlButtonContentInfoPtr", '*', "OutMode")]),

                ([("ControlButtonContentInfo", '*', "OutMode")],
                 [("ControlButtonContentInfo", '*', "InMode")]),

                ([("ControlActionUPP", 'liveTrackingProc', "InMode")],
                 [("ControlActionUPPNewControl", 'liveTrackingProc', "InMode")]),
                ]

if __name__ == "__main__":
    main()
