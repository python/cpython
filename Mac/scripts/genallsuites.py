# Generate all the standard scripting suite packages.
# Note that this module needs *serious* hand-crafting because of all the
# absolute paths. It is, however, a great leap forward compared to the time
# when this information was only stored in Jack's brain:-)

import sys
import os
import gensuitemodule

verbose=sys.stdout

DSTDIR="/Users/jack/src/python/Lib/plat-mac/lib-scriptpackages"
OS9DISK="/Volumes/Moes"

APPLESCRIPT=OS9DISK + "/Systeemmap/Extensies/AppleScript"
SYSTEMEVENTS="/System/Library/CoreServices/System Events.app"

CODEWARRIOR=OS9DISK + "/Applications (Mac OS 9)/Metrowerks CodeWarrior 7.0/Metrowerks CodeWarrior/CodeWarrior IDE 4.2.6"
EXPLORER="/Applications/Internet Explorer.app"
FINDER="/System/Library/CoreServices/Finder.app"
NETSCAPE=OS9DISK + "/Applications (Mac OS 9)/Netscape Communicator\xe2\x84\xa2 Folder/Netscape Communicator\xe2\x84\xa2"
TERMINAL="/Applications/Utilities/Terminal.app"

gensuitemodule.processfile_fromresource(APPLESCRIPT,
        output=os.path.join(DSTDIR, 'StdSuites'),
        basepkgname='_builtinSuites',
        edit_modnames=[], verbose=verbose)
gensuitemodule.processfile(SYSTEMEVENTS,
        output=os.path.join(DSTDIR, 'SystemEvents'),
        basepkgname='StdSuites',
        edit_modnames=[('Disk_2d_Folder_2d_File_Suite', 'Disk_Folder_File_Suite')],
        verbose=verbose)
gensuitemodule.processfile(CODEWARRIOR,
        output=os.path.join(DSTDIR, 'CodeWarrior'),
        basepkgname='StdSuites',
        edit_modnames=[], verbose=verbose)
gensuitemodule.processfile(EXPLORER,
        output=os.path.join(DSTDIR, 'Explorer'),
        basepkgname='StdSuites',
        edit_modnames=[], verbose=verbose)
gensuitemodule.processfile(FINDER,
        output=os.path.join(DSTDIR, 'Finder'),
        basepkgname='StdSuites',
        edit_modnames=[], verbose=verbose)
gensuitemodule.processfile(NETSCAPE,
        output=os.path.join(DSTDIR, 'Netscape'),
        basepkgname='StdSuites',
        edit_modnames=[('WorldWideWeb_suite_2c__as_d', 'WorldWideWeb_suite')], verbose=verbose)
gensuitemodule.processfile(TERMINAL,
        output=os.path.join(DSTDIR, 'Terminal'),
        basepkgname='StdSuites',
        edit_modnames=[], verbose=verbose)
