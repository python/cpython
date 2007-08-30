# Scan an Apple header file, generating a Python file of generator calls.

import sys
import os
from bgenlocations import TOOLBOXDIR, BGENDIR
sys.path.append(BGENDIR)
from scantools import Scanner_OSX

LONG = "Files"
SHORT = "file"

def main():
    input = ["Files.h", "Aliases.h", "Finder.h"]
    output = SHORT + "gen.py"
    defsoutput = TOOLBOXDIR + LONG + ".py"
    scanner = MyScanner(input, output, defsoutput)
    scanner.scan()
    scanner.close()
    scanner.gentypetest(SHORT+"typetest.py")
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
            # Funny special case
            if len(arglist) > 2:
                t, n, m = arglist[1]
                if t == "AliasHandle" and m == "InMode":
                    classname = "Arg2MethodGenerator"
                    listname = "alias_methods"
                    return classname, listname
            # Normal cases
            t, n, m = arglist[0]
            if t == "AliasHandle" and m == "InMode":
                classname = "Method"
                listname = "alias_methods"
            if t == "FSSpec_ptr" and m == "InMode":
                classname = "Method"
                listname = "fsspec_methods"
            if t == "FSRef_ptr" and m == "InMode":
                classname = "Method"
                listname = "fsref_methods"
        return classname, listname

    def makeblacklistnames(self):
        return [
                # Constants with incompatible definitions
                "kioACAccessOwnerMask",
                "kFSCatInfoReserved",
                "kFSIterateReserved",
                "kSystemFolderType",

                "FSRefMakePath", # Do this manually
#                       "ResolveAlias", # Do this manually
#                       "ResolveAliasWithMountFlags", # Do this manually
#                       "FollowFinderAlias", # Do this manually

                "FSRead", # Couldn't be bothered
                "FSWrite", # ditto
                "FSReadFork", # ditto
                "FSWriteFork", # ditto

                # Old routines:
                "GetWDInfo",
                "OpenWD",
                "CloseWD",
                "FInitQueue",
                "rstflock",
                "setflock",
                "setfinfo",
                "fsrename",
                "fsdelete",
                "create",
                "flushvol",
                "eject",
                "umountvol",
                "setvol",
                "getvol",
                "getfinfo",
                "getvinfo",
                "fsopen",
                "RstFLock",
                "SetFLock",
                "SetFInfo",
                "Rename",
                "OpenRF",
                "FSDelete",
                "Create",
                "GetVol",
                "GetFInfo",
                "GetVInfo",
                "FSOpen",
                "Eject",
                "SetVol",
                "openrf",
                "unmountvol",
                "OpenDF",

                ]

    def makeblacklisttypes(self):
        return [
                "CInfoPBPtr", # Old stuff
                "CMovePBPtr", # Old stuff
                "ParmBlkPtr", # Old stuff
                "HParmBlkPtr", # Old stuff
                "DTPBPtr", # Old stuff
                "FCBPBPtr", # Old stuff
                "QHdrPtr", # Old stuff
                "CSParamPtr", # Old stuff
                "FSCatalogBulkParam", # old stuff
                "FSForkCBInfoParam", # old stuff
                "FSForkIOParam", # old stuff
                "FSRefParam",  # old stuff
                "FSVolumeInfoParam", # old stuff
                "WDPBPtr", # old stuff
                "XCInfoPBPtr", # old stuff
                "XVolumeParamPtr", # old stuff


                "CatPositionRec", # State variable, not too difficult
                "FSIterator", # Should become an object
                "FSForkInfo", # Lots of fields, difficult struct
                "FSSearchParams", # Also catsearch stuff
                "FSVolumeInfo", # big struct
                "FSVolumeInfo_ptr", # big struct

                "IOCompletionProcPtr", # proc pointer
                "IOCompletionUPP", # Proc pointer
                "AliasFilterProcPtr",
                "AliasFilterUPP",
                "FNSubscriptionUPP",

                "FNSubscriptionRef", # Lazy, for now.
                ]

    def makerepairinstructions(self):
        return [
                # Various ways to give pathnames
                ([('char_ptr', '*', 'InMode')],
                 [('stringptr', '*', 'InMode')]
                ),

                # Unicode filenames passed as length, buffer
                ([('UniCharCount', '*', 'InMode'),
                  ('UniChar_ptr', '*', 'InMode')],
                 [('UnicodeReverseInBuffer', '*', 'InMode')]
                ),
                # Wrong guess
                ([('Str63', 'theString', 'InMode')],
                 [('Str63', 'theString', 'OutMode')]),

                # Yet another way to give a pathname:-)
                ([('short', 'fullPathLength', 'InMode'),
                  ('void_ptr', 'fullPath', 'InMode')],
                 [('FullPathName', 'fullPath', 'InMode')]),

                # Various ResolveAliasFileXXXX functions
                ([('FSSpec', 'theSpec', 'OutMode')],
                 [('FSSpec_ptr', 'theSpec', 'InOutMode')]),

                ([('FSRef', 'theRef', 'OutMode')],
                 [('FSRef_ptr', 'theRef', 'InOutMode')]),

                # The optional FSSpec to all ResolveAlias and NewAlias methods
                ([('FSSpec_ptr', 'fromFile', 'InMode')],
         [('OptFSSpecPtr', 'fromFile', 'InMode')]),

                ([('FSRef_ptr', 'fromFile', 'InMode')],
         [('OptFSRefPtr', 'fromFile', 'InMode')]),

##              # FSCatalogInfo input handling
##                      ([('FSCatalogInfoBitmap', 'whichInfo', 'InMode'),
##                ('FSCatalogInfo_ptr', 'catalogInfo', 'InMode')],
##               [('FSCatalogInfoAndBitmap_in', 'catalogInfo', 'InMode')]),
##
##              # FSCatalogInfo output handling
##                      ([('FSCatalogInfoBitmap', 'whichInfo', 'InMode'),
##                ('FSCatalogInfo', 'catalogInfo', 'OutMode')],
##               [('FSCatalogInfoAndBitmap_out', 'catalogInfo', 'InOutMode')]),
##

        ]


    def writeinitialdefs(self):
        self.defsfile.write("def FOUR_CHAR_CODE(x): return x\n")
        self.defsfile.write("true = True\n")
        self.defsfile.write("false = False\n")

if __name__ == "__main__":
    main()
