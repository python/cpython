# Show off SndPlay (and some resource manager functions).
# Get a list of all 'snd ' resources in the system and play them all.

from Res import *
from Snd import *

ch = SndNewChannel(0, 0, None)
print "Channel:", ch

type = 'snd '

for i in range(CountResources(type)):
	r = GetIndResource(type, i+1)
	print r.GetResInfo(), r.size
	if r.GetResInfo()[0] == 1:
		print "Skipping simple beep"
		continue
	ch.SndPlay(r, 0)
