# Echo client demo using Unix sockets
# Piet van Oostrum
from socket import *
FILE = 'blabla'
s = socket(AF_UNIX, SOCK_STREAM)
s.connect(FILE)
s.send('Hello, world')
data = s.recv(1024)
s.close()
print 'Received', `data`
