# Test suite for SocketServer.py

import test_support
from test_support import verbose, verify, TESTFN, TestSkipped
test_support.requires('network')

from SocketServer import *
import socket
import select
import time
import threading
import os

NREQ = 3
DELAY = 0.5

class MyMixinHandler:
    def handle(self):
        time.sleep(DELAY)
        line = self.rfile.readline()
        time.sleep(DELAY)
        self.wfile.write(line)

class MyStreamHandler(MyMixinHandler, StreamRequestHandler):
    pass

class MyDatagramHandler(MyMixinHandler, DatagramRequestHandler):
    pass

class MyMixinServer:
    def serve_a_few(self):
        for i in range(NREQ):
            self.handle_request()
    def handle_error(self, request, client_address):
        self.close_request(request)
        self.server_close()
        raise

teststring = "hello world\n"

def receive(sock, n, timeout=20):
    r, w, x = select.select([sock], [], [], timeout)
    if sock in r:
        return sock.recv(n)
    else:
        raise RuntimeError, "timed out on %s" % `sock`

def testdgram(proto, addr):
    s = socket.socket(proto, socket.SOCK_DGRAM)
    s.sendto(teststring, addr)
    buf = data = receive(s, 100)
    while data and '\n' not in buf:
        data = receive(s, 100)
        buf += data
    verify(buf == teststring)
    s.close()

def teststream(proto, addr):
    s = socket.socket(proto, socket.SOCK_STREAM)
    s.connect(addr)
    s.sendall(teststring)
    buf = data = receive(s, 100)
    while data and '\n' not in buf:
        data = receive(s, 100)
        buf += data
    verify(buf == teststring)
    s.close()

class ServerThread(threading.Thread):
    def __init__(self, addr, svrcls, hdlrcls):
        threading.Thread.__init__(self)
        self.__addr = addr
        self.__svrcls = svrcls
        self.__hdlrcls = hdlrcls
    def run(self):
        class svrcls(MyMixinServer, self.__svrcls):
            pass
        if verbose: print "thread: creating server"
        svr = svrcls(self.__addr, self.__hdlrcls)
        if verbose: print "thread: serving three times"
        svr.serve_a_few()
        if verbose: print "thread: done"

seed = 0
def pickport():
    global seed
    seed += 1
    return 10000 + (os.getpid() % 1000)*10 + seed

host = "localhost"
testfiles = []
def pickaddr(proto):
    if proto == socket.AF_INET:
        return (host, pickport())
    else:
        fn = TESTFN + str(pickport())
        testfiles.append(fn)
        return fn

def cleanup():
    for fn in testfiles:
        try:
            os.remove(fn)
        except os.error:
            pass
    testfiles[:] = []

def testloop(proto, servers, hdlrcls, testfunc):
    for svrcls in servers:
        addr = pickaddr(proto)
        if verbose:
            print "ADDR =", addr
            print "CLASS =", svrcls
        t = ServerThread(addr, svrcls, hdlrcls)
        if verbose: print "server created"
        t.start()
        if verbose: print "server running"
        for i in range(NREQ):
            time.sleep(DELAY)
            if verbose: print "test client", i
            testfunc(proto, addr)
        if verbose: print "waiting for server"
        t.join()
        if verbose: print "done"

tcpservers = [TCPServer, ThreadingTCPServer]
if hasattr(os, 'fork'):
    tcpservers.append(ForkingTCPServer)
udpservers = [UDPServer, ThreadingUDPServer]
if hasattr(os, 'fork'):
    udpservers.append(ForkingUDPServer)

if not hasattr(socket, 'AF_UNIX'):
    streamservers = []
    dgramservers = []
else:
    class ForkingUnixStreamServer(ForkingMixIn, UnixStreamServer): pass
    streamservers = [UnixStreamServer, ThreadingUnixStreamServer,
                     ForkingUnixStreamServer]
    class ForkingUnixDatagramServer(ForkingMixIn, UnixDatagramServer): pass
    dgramservers = [UnixDatagramServer, ThreadingUnixDatagramServer,
                    ForkingUnixDatagramServer]

def testall():
    testloop(socket.AF_INET, tcpservers, MyStreamHandler, teststream)
    testloop(socket.AF_INET, udpservers, MyDatagramHandler, testdgram)
    if hasattr(socket, 'AF_UNIX'):
        testloop(socket.AF_UNIX, streamservers, MyStreamHandler, teststream)
        # Alas, on Linux (at least) recvfrom() doesn't return a meaningful
        # client address so this cannot work:
        ##testloop(socket.AF_UNIX, dgramservers, MyDatagramHandler, testdgram)

def test_main():
    import imp
    if imp.lock_held():
        # If the import lock is held, the threads will hang.
        raise TestSkipped("can't run when import lock is held")

    try:
        testall()
    finally:
        cleanup()

if __name__ == "__main__":
    test_main()
