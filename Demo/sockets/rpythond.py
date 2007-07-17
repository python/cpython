#! /usr/bin/env python

# Remote python server.
# Execute Python commands remotely and send output back.
# WARNING: This version has a gaping security hole -- it accepts requests
# from any host on the Internet!

import sys
from socket import *
import io
import traceback

PORT = 4127
BUFSIZE = 1024

def main():
    if len(sys.argv) > 1:
        port = int(eval(sys.argv[1]))
    else:
        port = PORT
    s = socket(AF_INET, SOCK_STREAM)
    s.bind(('', port))
    s.listen(1)
    while 1:
        conn, (remotehost, remoteport) = s.accept()
        print('connected by', remotehost, remoteport)
        request = ''
        while 1:
            data = conn.recv(BUFSIZE)
            if not data:
                break
            request = request + data
        reply = execute(request)
        conn.send(reply)
        conn.close()

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

main()
