# Generate all the standard scripting suite packages.
# Note that this module needs *serious* hand-crafting because of all the
# absolute paths. It is, however, a great leap forward compared to the time
# when this information was only stored in Jack's brain:-)

import sys
import os
import gensuitemodule

DSTDIR="/Users/jack/src/python/Lib/plat-mac/lib-scriptpackages"

APPLESCRIPT="/Volumes/Sap/System Folder/Extensions/AppleScript"
CODEWARRIOR="/Volumes/Sap/Applications (Mac OS 9)/Metrowerks CodeWarrior 7.0/Metrowerks CodeWarrior/CodeWarrior IDE 4.2.5"
EXPLORER="/Volumes/Sap/Applications (Mac OS 9)/Internet Explorer 5/Internet Explorer"
FINDER="/Volumes/Sap/System Folder/Finder"
NETSCAPE="/Volumes/Sap/Applications (Mac OS 9)/Netscape Communicator\xe2\x84\xa2 Folder/Netscape Communicator\xe2\x84\xa2"
TERMINAL="/Applications/Utilities/Terminal.app/Contents/Resources/Terminal.rsrc"

gensuitemodule.processfile_fromresource(APPLESCRIPT,
	output=os.path.join(DSTDIR, 'StdSuites'),
	basepkgname='_builtinSuites',
	edit_modnames=[])
gensuitemodule.processfile_fromresource(CODEWARRIOR,
	output=os.path.join(DSTDIR, 'CodeWarrior'),
	basepkgname='StdSuites',
	edit_modnames=[])
gensuitemodule.processfile_fromresource(EXPLORER,
	output=os.path.join(DSTDIR, 'Explorer'),
	basepkgname='StdSuites',
	edit_modnames=[])
gensuitemodule.processfile_fromresource(FINDER,
	output=os.path.join(DSTDIR, 'Finder'),
	basepkgname='StdSuites',
	edit_modnames=[])
gensuitemodule.processfile_fromresource(NETSCAPE,
	output=os.path.join(DSTDIR, 'Netscape'),
	basepkgname='StdSuites',
	edit_modnames=[('WorldWideWeb_suite_2c__as_d', 'WorldWideWeb_suite')])
gensuitemodule.processfile_fromresource(TERMINAL,
	output=os.path.join(DSTDIR, 'Terminal'),
	basepkgname='StdSuites',
	edit_modnames=[],
	creatorsignature='trmx')
	