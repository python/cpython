# Echo server program using Unix sockets (handles one connection only)
from socket import *
FILE = 'blabla'             
s = socket(AF_UNIX, SOCK_STREAM)
s.bind(FILE)
print 'Sock name is: ['+s.getsockname()+']'
s.listen(1)
conn, addr = s.accept()
print 'Connected by', addr
while 1:
    data = conn.recv(1024)
    if not data: break
    conn.send(data)
conn.close()
