# Send UDP broadcast packets

MYPORT = 50000

import sys, time
from socket import *

s = socket(AF_INET, SOCK_DGRAM)
s.bind('', 0)
s.allowbroadcast(1)

while 1:
	data = `time.time()` + '\n'
	s.sendto(data, ('<broadcast>', MYPORT))
	time.sleep(2)

	
