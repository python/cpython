# Receive UDP packets transmitted by a broadcasting service

MYPORT = 50000

import sys
from socket import *

s = socket(AF_INET, SOCK_DGRAM)
s.bind('', MYPORT)

while 1:
	data = s.recv(1500)
	sys.stdout.write(data)
