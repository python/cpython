#! /usr/bin/env python

# Python implementation of an 'echo' tcp server: echo all data it receives.
#
# This is the simplest possible server, servicing a single request only.

import sys
from socket import *

# The standard echo port isn't very useful, it requires root permissions!
# ECHO_PORT = 7
ECHO_PORT = 50000 + 7
BUFSIZE = 1024

def main():
    if len(sys.argv) > 1:
        port = int(eval(sys.argv[1]))
    else:
        port = ECHO_PORT
    s = socket(AF_INET, SOCK_STREAM)
    s.bind(('', port))
    s.listen(1)
    conn, (remotehost, remoteport) = s.accept()
    print 'connected by', remotehost, remoteport
    while 1:
        data = conn.recv(BUFSIZE)
        if not data:
            break
        conn.send(data)

main()
