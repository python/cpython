"""Test icglue module by printing all preferences"""

import icglue
import Res

ici = icglue.ICStart('Pyth')
ici.ICFindConfigFile()
h = Res.Resource("")

ici.ICBegin(1)
numprefs = ici.ICCountPref()
print "Number of preferences:", numprefs

for i in range(1, numprefs+1):
	key = ici.ICGetIndPref(i)
	print "Key:  ", key
	
	h.data = ""
	attrs = ici.ICFindPrefHandle(key, h)
	print "Attr: ", attrs
	print "Data: ", `h.data[:64]`

ici.ICEnd()
del ici

import sys
sys.exit(1)
