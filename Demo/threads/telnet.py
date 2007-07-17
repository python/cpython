# Minimal interface to the Internet telnet protocol.
#
# *** modified to use threads ***
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


import sys, os, time
from socket import *
import thread

BUFSIZE = 8*1024

# Telnet protocol characters

IAC  = chr(255) # Interpret as command
DONT = chr(254)
DO   = chr(253)
WONT = chr(252)
WILL = chr(251)

def main():
    if len(sys.argv) < 2:
        sys.stderr.write('usage: telnet hostname [port]\n')
        sys.exit(2)
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
    except error as msg:
        sys.stderr.write('connect failed: %r\n' % (msg,))
        sys.exit(1)
    #
    thread.start_new(child, (s,))
    parent(s)

def parent(s):
    # read socket, write stdout
    iac = 0         # Interpret next char as command
    opt = ''        # Interpret next char as option
    while 1:
        data, dummy = s.recvfrom(BUFSIZE)
        if not data:
            # EOF -- exit
            sys.stderr.write( '(Closed by remote host)\n')
            sys.exit(1)
        cleandata = ''
        for c in data:
            if opt:
                print(ord(c))
##                              print '(replying: %r)' % (opt+c,)
                s.send(opt + c)
                opt = ''
            elif iac:
                iac = 0
                if c == IAC:
                    cleandata = cleandata + c
                elif c in (DO, DONT):
                    if c == DO: print('(DO)', end=' ')
                    else: print('(DONT)', end=' ')
                    opt = IAC + WONT
                elif c in (WILL, WONT):
                    if c == WILL: print('(WILL)', end=' ')
                    else: print('(WONT)', end=' ')
                    opt = IAC + DONT
                else:
                    print('(command)', ord(c))
            elif c == IAC:
                iac = 1
                print('(IAC)', end=' ')
            else:
                cleandata = cleandata + c
        sys.stdout.write(cleandata)
        sys.stdout.flush()
##              print 'Out:', repr(cleandata)

def child(s):
    # read stdin, write socket
    while 1:
        line = sys.stdin.readline()
##              print 'Got:', repr(line)
        if not line: break
        s.send(line)

main()
