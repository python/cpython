"""fgbgtest - See how many CPU cycles we get"""

import time

loopct = 0L
oldloopct = 0L
oldt = time.time()

while 1:
	t = time.time()
	if t - oldt >= 1:
		if oldloopct:
			print loopct-oldloopct,'in one second'
		oldloopct = loopct
		oldt = time.time()
	loopct = loopct + 1
	
