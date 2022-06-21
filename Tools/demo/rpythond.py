#!/usr/bin/env python3

"""
Remote python server.
Execute Python commands remotely and send output back.

WARNING: This version has a gaping security hole -- it accepts requests
from any host on the internet!
"""

import sys
from socket import socket, AF_INET, SOCK_STREAM
import io
import traceback

PORT = 4127
BUFSIZE = 1024

def main():
    if len(sys.argv) > 1:
        port = int(sys.argv[1])
    else:
        port = PORT
    s = socket(AF_INET, SOCK_STREAM)
    s.bind(('', port))
    s.listen(1)
    while True:
        conn, (remotehost, remoteport) = s.accept()
        with conn:
            print('connection from', remotehost, remoteport)
            request = b''
            while True:
                data = conn.recv(BUFSIZE)
                if not data:
                    break
                request += data
            reply = execute(request.decode())
            conn.send(reply.encode())

def execute(request):
    stdout = sys.stdout
    stderr = sys.stderr
    sys.stdout = sys.stderr = fakefile = io.StringIO()
    try:
        try:
            exec(request, {}, {})
        except:
            print()
            traceback.print_exc(100)
    finally:
        sys.stderr = stderr
        sys.stdout = stdout
    return fakefile.getvalue()

try:
    main()
except KeyboardInterrupt:
    pass
