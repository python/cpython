#
# Local customizations for generating the Carbon interface modules.
# Edit this file to reflect where things should be on your system.
# Note that pathnames are unix-style for OSX MachoPython/unix-Python,
# but mac-style for MacPython, whether running on OS9 or OSX.
#

import sys, os

Error = "bgenlocations.Error"
#
# Where bgen is. For unix-Python bgen isn't installed, so you have to refer to
# the source tree here.
if sys.platform == 'mac':
	# For MacPython we know where it is
	def _pardir(p): return os.path.split(p)[0]
	BGENDIR=os.path.join(sys.prefix, "Tools", "bgen", "bgen")
else:
	# for unix-Python we don't know, please set it yourself.
	BGENDIR="/Users/jack/src/python/Tools/bgen/bgen"

#
# Where to find the Universal Header include files. If you have CodeWarrior
# installed you can use the Universal Headers from there, otherwise you can
# download them from the Apple website. Bgen can handle both unix- and mac-style
# end of lines, so don't worry about that.
#
if sys.platform == 'mac':
	_MWERKSDIR="Macintosh HD:Applications (Mac OS 9):Metrowerks CodeWarrior 7.0:Metrowerks CodeWarrior"
else:
	_MWERKSDIR="/Volumes/Sap/Applications (Mac OS 9)/Metrowerks CodeWarrior 7.0/Metrowerks CodeWarrior/"
INCLUDEDIR=os.path.join(_MWERKSDIR, "MacOS Support", "Universal", "Interfaces", "CIncludes")

#
# Where to put the python definitions files. Note that, on unix-Python,
# if you want to commit your changes to the CVS repository this should refer to
# your source directory, not your installed directory.
#
if sys.platform == 'mac':
	TOOLBOXDIR=os.path.join(sys.prefix, "Mac", "Lib", "Carbon")
else:
	TOOLBOXDIR="/Users/jack/src/python/Mac/Lib/Carbon"

# Creator for C files:
CREATOR="CWIE"

if not os.path.exists(BGENDIR):
	raise Error, "Please fix bgenlocations.py, BGENDIR does not exist: %s" % BGENDIR
if not os.path.exists(INCLUDEDIR):
	raise Error, "Please fix bgenlocations.py, INCLUDEDIR does not exist: %s" % INCLUDEDIR
if not os.path.exists(TOOLBOXDIR):
	raise Error, "Please fix bgenlocations.py, TOOLBOXDIR does not exist: %s" % TOOLBOXDIR
	
# Sigh, due to the way these are used make sure they end with : or /.
if BGENDIR[-1] != os.sep:
	BGENDIR = BGENDIR + os.sep
if INCLUDEDIR[-1] != os.sep:
	INCLUDEDIR = INCLUDEDIR + os.sep
if TOOLBOXDIR[-1] != os.sep:
	TOOLBOXDIR = TOOLBOXDIR + os.sep
	
