#
# Local customizations
#
import sys, os
# Where to find the Universal Header include files:
MWERKSDIR="/Applications/Metrowerks CodeWarrior 7.0/Metrowerks CodeWarrior/"
INCLUDEDIR=os.path.join(MWERKSDIR, "MacOS Support", "Universal", "Interfaces", "CIncludes")

# Where to put the python definitions file:
TOOLBOXDIR=os.path.join(sys.prefix, "Mac", "Lib", "Carbon")

# Creator for C files:
CREATOR="CWIE"
