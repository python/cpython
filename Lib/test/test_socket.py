# Not tested:
#       socket.fromfd()
#       sktobj.getsockopt()
#       sktobj.recvfrom()
#       sktobj.sendto()
#       sktobj.setblocking()
#       sktobj.setsockopt()
#       sktobj.shutdown()


from test_support import verbose, TestFailed
import socket
import os
import time

def missing_ok(str):
    try:
        getattr(socket, str)
    except AttributeError:
        pass

try: raise socket.error
except socket.error: print "socket.error"

socket.AF_INET

socket.SOCK_STREAM
socket.SOCK_DGRAM
socket.SOCK_RAW
socket.SOCK_RDM
socket.SOCK_SEQPACKET

for optional in ("AF_UNIX",

                 "SO_DEBUG", "SO_ACCEPTCONN", "SO_REUSEADDR", "SO_KEEPALIVE",
                 "SO_DONTROUTE", "SO_BROADCAST", "SO_USELOOPBACK", "SO_LINGER",
                 "SO_OOBINLINE", "SO_REUSEPORT", "SO_SNDBUF", "SO_RCVBUF",
                 "SO_SNDLOWAT", "SO_RCVLOWAT", "SO_SNDTIMEO", "SO_RCVTIMEO",
                 "SO_ERROR", "SO_TYPE", "SOMAXCONN",

                 "MSG_OOB", "MSG_PEEK", "MSG_DONTROUTE", "MSG_EOR",
                 "MSG_TRUNC", "MSG_CTRUNC", "MSG_WAITALL", "MSG_BTAG",
                 "MSG_ETAG",

                 "SOL_SOCKET",

                 "IPPROTO_IP", "IPPROTO_ICMP", "IPPROTO_IGMP",
                 "IPPROTO_GGP", "IPPROTO_TCP", "IPPROTO_EGP",
                 "IPPROTO_PUP", "IPPROTO_UDP", "IPPROTO_IDP",
                 "IPPROTO_HELLO", "IPPROTO_ND", "IPPROTO_TP",
                 "IPPROTO_XTP", "IPPROTO_EON", "IPPROTO_BIP",
                 "IPPROTO_RAW", "IPPROTO_MAX",

                 "IPPORT_RESERVED", "IPPORT_USERRESERVED",

                 "INADDR_ANY", "INADDR_BROADCAST", "INADDR_LOOPBACK",
                 "INADDR_UNSPEC_GROUP", "INADDR_ALLHOSTS_GROUP",
                 "INADDR_MAX_LOCAL_GROUP", "INADDR_NONE",

                 "IP_OPTIONS", "IP_HDRINCL", "IP_TOS", "IP_TTL",
                 "IP_RECVOPTS", "IP_RECVRETOPTS", "IP_RECVDSTADDR",
                 "IP_RETOPTS", "IP_MULTICAST_IF", "IP_MULTICAST_TTL",
                 "IP_MULTICAST_LOOP", "IP_ADD_MEMBERSHIP",
                 "IP_DROP_MEMBERSHIP",
                 ):
    missing_ok(optional)

socktype = socket.SocketType
hostname = socket.gethostname()
ip = socket.gethostbyname(hostname)
hname, aliases, ipaddrs = socket.gethostbyaddr(ip)
all_host_names = [hname] + aliases

if verbose:
    print hostname
    print ip
    print hname, aliases, ipaddrs
    print all_host_names

for name in all_host_names:
    if name.find('.'):
        break
else:
    print 'FQDN not found'

if hasattr(socket, 'getservbyname'):
    print socket.getservbyname('telnet', 'tcp')
    try:
        socket.getservbyname('telnet', 'udp')
    except socket.error:
        pass

import sys
if not sys.platform.startswith('java'):
    try:
        # On some versions, this loses a reference
        orig = sys.getrefcount(__name__)
        socket.getnameinfo(__name__,0)
    except SystemError:
        if sys.getrefcount(__name__) <> orig:
            raise TestFailed,"socket.getnameinfo loses a reference"

try:
    # On some versions, this crashes the interpreter.
    socket.getnameinfo(('x', 0, 0, 0), 0)
except socket.error:
    pass

canfork = hasattr(os, 'fork')
try:
    PORT = 50007
    msg = 'socket test\n'
    if not canfork or os.fork():
        # parent is server
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind(("127.0.0.1", PORT))
        s.listen(1)
        if verbose:
            print 'parent accepting'
        if canfork:
            conn, addr = s.accept()
            if verbose:
                print 'connected by', addr
            # couple of interesting tests while we've got a live socket
            f = conn.fileno()
            if verbose:
                print 'fileno:', f
            p = conn.getpeername()
            if verbose:
                print 'peer:', p
            n = conn.getsockname()
            if verbose:
                print 'sockname:', n
            f = conn.makefile()
            if verbose:
                print 'file obj:', f
            data = conn.recv(1024)
            if verbose:
                print 'received:', data
            conn.sendall(data)

            # Perform a few tests on the windows file object
            if verbose:
                print "Staring _fileobject tests..."
            f = socket._fileobject (conn, 'rb', 8192)
            first_seg = f.read(7)
            second_seg = f.read(5)
            if not first_seg == 'socket ' or not second_seg == 'test\n':
                print "Error performing read with the python _fileobject class"
                os._exit (1)
            elif verbose:
                print "_fileobject buffered read works"
            f.write (data)
            f.flush ()

            buf = ''
            while 1:
                char = f.read(1)
                if not char:
                    print "Error performing unbuffered read with the python ", \
                          "_fileobject class"
                    os._exit (1)
                buf += char
                if buf == msg:
                    if verbose:
                        print "__fileobject unbuffered read works"
                    break
            if verbose:
                # If we got this far, write() must work as well
                print "__fileobject write works"
            f.write(buf)
            f.flush()

            line = f.readline()
            if not line == msg:
                print "Error perferming readline with the python _fileobject class"
                os._exit (1)
            f.write(line)
            f.flush()
            if verbose:
                print "__fileobject readline works"

            conn.close()
    else:
        try:
            # child is client
            time.sleep(5)
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            if verbose:
                print 'child connecting'
            s.connect(("127.0.0.1", PORT))

            iteration = 0
            while 1:
                s.send(msg)
                data = s.recv(12)
                if not data:
                    break
                if msg != data:
                    print "parent/client mismatch. Failed in %s iteration. Received: [%s]" \
                          %(iteration, data)
                time.sleep (1)
                iteration += 1
            s.close()
        finally:
            os._exit(1)
except socket.error, msg:
    raise TestFailed, msg
