# Scan <Menus.h>, generating menugen.py.
import sys
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)

from scantools import Scanner

def main():
    input = "Menus.h"
    output = "menugen.py"
    defsoutput = TOOLBOXDIR + "Menus.py"
    scanner = MyScanner(input, output, defsoutput)
    scanner.scan()
    scanner.close()
    print("=== Testing definitions output code ===")
    exec(open(defsoutput).read(), {}, {})
    print("=== Done scanning and generating, now doing 'import menusupport' ===")
    import menusupport
    print("=== Done.  It's up to you to compile Menumodule.c ===")

class MyScanner(Scanner):

    def destination(self, type, name, arglist):
        classname = "Function"
        listname = "functions"
        if arglist:
            t, n, m = arglist[0]
            if t in ("MenuHandle", "MenuRef") and m == "InMode":
                classname = "Method"
                listname = "methods"
        return classname, listname

    def makeblacklistnames(self):
        return [
##                      "IsShowContextualMenuClick", # Can't find it in the library
##                      "InitContextualMenus", # ditto
                "GetMenuItemProperty",  # difficult for the moment
                "GetMenuItemPropertySize",
                "SetMenuItemProperty",
                "RemoveMenuItemProperty",
                "SetMenuCommandProperty",
                "GetMenuCommandProperty",
                "GetMenuTitle", # Funny arg/returnvalue
                "SetMenuTitle",
                "SetMenuTitleIcon", # void*
                # OS8 calls:
                'GetMenuItemRefCon2',
                'SetMenuItemRefCon2',
                'EnableItem',
                'DisableItem',
                'CheckItem',
                'CountMItems',
                'OpenDeskAcc',
                'SystemEdit',
                'SystemMenu',
                'SetMenuFlash',
                'InitMenus',
                'InitProcMenu',
                ]

    def makeblacklisttypes(self):
        return [
                'MCTableHandle',
                'MCEntryPtr',
                'MCTablePtr',
                'AEDesc_ptr', # For now: doable, but not easy
                'ProcessSerialNumber', # ditto
                "MenuDefSpecPtr", # Too difficult for now
                "MenuDefSpec_ptr", # ditto
                "MenuTrackingData",
                "void_ptr",     # Don't know yet.
                "EventRef",     # For now, not exported yet.
                "MenuItemDataPtr", # Not yet.
                "MenuItemDataRec_ptr",
                ]

    def makerepairinstructions(self):
        return [
                ([("Str255", "itemString", "InMode")],
                 [("*", "*", "OutMode")]),

                ([("void_ptr", "*", "InMode"), ("long", "*", "InMode")],
                 [("InBuffer", "*", "*")]),

                ([("void", "*", "OutMode"), ("long", "*", "InMode"),
                                            ("long", "*", "OutMode")],
                 [("VarVarOutBuffer", "*", "InOutMode")]),
                ([("MenuRef", 'outHierMenu', "OutMode")],
                 [("OptMenuRef", 'outHierMenu', "OutMode")]),
                ]

    def writeinitialdefs(self):
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")

if __name__ == "__main__":
    main()
