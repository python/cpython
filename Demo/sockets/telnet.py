#! /usr/bin/env python

# Minimal interface to the Internet telnet protocol.
#
# It refuses all telnet options and does not recognize any of the other
# telnet commands, but can still be used to connect in line-by-line mode.
# It's also useful to play with a number of other services,
# like time, finger, smtp and even ftp.
#
# Usage: telnet host [port]
#
# The port may be a service name or a decimal port number;
# it defaults to 'telnet'.


import sys, posix, time
from socket import *

BUFSIZE = 1024

# Telnet protocol characters

IAC  = chr(255) # Interpret as command
DONT = chr(254)
DO   = chr(253)
WONT = chr(252)
WILL = chr(251)

def main():
    host = sys.argv[1]
    try:
        hostaddr = gethostbyname(host)
    except error:
        sys.stderr.write(sys.argv[1] + ': bad host name\n')
        sys.exit(2)
    #
    if len(sys.argv) > 2:
        servname = sys.argv[2]
    else:
        servname = 'telnet'
    #
    if '0' <= servname[:1] <= '9':
        port = eval(servname)
    else:
        try:
            port = getservbyname(servname, 'tcp')
        except error:
            sys.stderr.write(servname + ': bad tcp service name\n')
            sys.exit(2)
    #
    s = socket(AF_INET, SOCK_STREAM)
    #
    try:
        s.connect((host, port))
    except error, msg:
        sys.stderr.write('connect failed: ' + repr(msg) + '\n')
        sys.exit(1)
    #
    pid = posix.fork()
    #
    if pid == 0:
        # child -- read stdin, write socket
        while 1:
            line = sys.stdin.readline()
            s.send(line)
    else:
        # parent -- read socket, write stdout
        iac = 0         # Interpret next char as command
        opt = ''        # Interpret next char as option
        while 1:
            data = s.recv(BUFSIZE)
            if not data:
                # EOF; kill child and exit
                sys.stderr.write( '(Closed by remote host)\n')
                posix.kill(pid, 9)
                sys.exit(1)
            cleandata = ''
            for c in data:
                if opt:
                    print ord(c)
                    s.send(opt + c)
                    opt = ''
                elif iac:
                    iac = 0
                    if c == IAC:
                        cleandata = cleandata + c
                    elif c in (DO, DONT):
                        if c == DO: print '(DO)',
                        else: print '(DONT)',
                        opt = IAC + WONT
                    elif c in (WILL, WONT):
                        if c == WILL: print '(WILL)',
                        else: print '(WONT)',
                        opt = IAC + DONT
                    else:
                        print '(command)', ord(c)
                elif c == IAC:
                    iac = 1
                    print '(IAC)',
                else:
                    cleandata = cleandata + c
            sys.stdout.write(cleandata)
            sys.stdout.flush()


try:
    main()
except KeyboardInterrupt:
    pass
