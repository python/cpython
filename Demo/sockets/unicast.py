# Send UDP broadcast packets

MYPORT = 50000

import sys, time
from socket import *

s = socket(AF_INET, SOCK_DGRAM)
s.bind(('', 0))

while 1:
    data = repr(time.time()) + '\n'
    s.sendto(data, ('', MYPORT))
    time.sleep(2)
