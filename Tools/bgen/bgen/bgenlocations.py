#
# Local customizations
#
import sys, os
# Where to find the Universal Header include files:
MWERKSDIR="Moes:Applications (Mac OS 9):Metrowerks CodeWarrior 7.0:Metrowerks CodeWarrior:"
INCLUDEDIR=MWERKSDIR + "MacOS Support:Universal:Interfaces:CIncludes:"

# Where to put the python definitions file:
TOOLBOXDIR=os.path.join(sys.prefix, ":Mac:Lib:Carbon:")

# Creator for C files:
CREATOR="CWIE"
