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
import string

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
    if string.find(name, '.'):
        break
else:
    print 'FQDN not found'

print socket.getservbyname('telnet', 'tcp')
try:
    socket.getservbyname('telnet', 'udp')
except socket.error:
    pass


canfork = hasattr(os, 'fork')
try:
    PORT = 50007
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
            while 1:
                data = conn.recv(1024)
                if not data:
                    break
                if verbose:
                    print 'received:', data
                conn.send(data)
            conn.close()
    else:
        try:
            # child is client
            time.sleep(5)
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            if verbose:
                print 'child connecting'
            s.connect(("127.0.0.1", PORT))
            msg = 'socket test'
            s.send(msg)
            data = s.recv(1024)
            if msg <> data:
                print 'parent/client mismatch'
            s.close()
        finally:
            os._exit(1)
except socket.error, msg:
    raise TestFailed, msg
