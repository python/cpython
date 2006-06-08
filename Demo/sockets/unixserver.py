# Echo server demo using Unix sockets (handles one connection only)
# Piet van Oostrum

import os
from socket import *

FILE = 'unix-socket'
s = socket(AF_UNIX, SOCK_STREAM)
s.bind(FILE)

print 'Sock name is: ['+s.getsockname()+']'

# Wait for a connection
s.listen(1)
conn, addr = s.accept()

while True:
    data = conn.recv(1024)
    if not data:
        break
    conn.send(data)

conn.close()
os.unlink(FILE)
