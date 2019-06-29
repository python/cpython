#!/usr/bin/env python3

"""
Remote python client.
Execute Python commands remotely and send output back.
"""

import sys
from socket import socket, AF_INET, SOCK_STREAM, SHUT_WR

PORT = 4127
BUFSIZE = 1024

def main():
    if len(sys.argv) < 3:
        print("usage: rpython host command")
        sys.exit(2)
    host = sys.argv[1]
    port = PORT
    i = host.find(':')
    if i >= 0:
        port = int(host[i+1:])
        host = host[:i]
    command = ' '.join(sys.argv[2:])
    with socket(AF_INET, SOCK_STREAM) as s:
        s.connect((host, port))
        s.send(command.encode())
        s.shutdown(SHUT_WR)
        reply = b''
        while True:
            data = s.recv(BUFSIZE)
            if not data:
                break
            reply += data
        print(reply.decode(), end=' ')

main()
