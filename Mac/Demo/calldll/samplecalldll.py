#
# Test calldll. Tell the user how often menus flash, and let her change it.
#

import calldll
import sys

# Obtain a reference to the library with the toolbox calls
interfacelib = calldll.getlibrary('InterfaceLib')

# Get the routines we need (see LowMem.h for details)
LMGetMenuFlash = calldll.newcall(interfacelib.LMGetMenuFlash, 'Short')
LMSetMenuFlash = calldll.newcall(interfacelib.LMSetMenuFlash, 'None', 'InShort')

print "Menus currently flash",LMGetMenuFlash(),"times."
print "How often would you like them to flash?",

# Note: we use input(), so you can try passing non-integer objects
newflash = input()
LMSetMenuFlash(newflash)

print "Okay, menus now flash", LMGetMenuFlash(),"times."

sys.exit(1)   # So the window stays on-screen
