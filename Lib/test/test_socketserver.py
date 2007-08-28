# Test suite for SocketServer.py

from test import test_support
from test.test_support import (verbose, verify, TESTFN, TestSkipped,
                               reap_children)
test_support.requires('network')

from SocketServer import *
import socket
import errno
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

teststring = b"hello world\n"

def receive(sock, n, timeout=20):
    r, w, x = select.select([sock], [], [], timeout)
    if sock in r:
        return sock.recv(n)
    else:
        raise RuntimeError, "timed out on %r" % (sock,)

def testdgram(proto, addr):
    s = socket.socket(proto, socket.SOCK_DGRAM)
    s.sendto(teststring, addr)
    buf = data = receive(s, 100)
    while data and b'\n' not in buf:
        data = receive(s, 100)
        buf += data
    verify(buf == teststring)
    s.close()

def teststream(proto, addr):
    s = socket.socket(proto, socket.SOCK_STREAM)
    s.connect(addr)
    s.sendall(teststring)
    buf = data = receive(s, 100)
    while data and b'\n' not in buf:
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
        self.ready = threading.Event()
    def run(self):
        class svrcls(MyMixinServer, self.__svrcls):
            pass
        if verbose: print("thread: creating server")
        svr = svrcls(self.__addr, self.__hdlrcls)
        # pull the address out of the server in case it changed
        # this can happen if another process is using the port
        addr = svr.server_address
        if addr:
            self.__addr = addr
            if self.__addr != svr.socket.getsockname():
                raise RuntimeError('server_address was %s, expected %s' %
                                       (self.__addr, svr.socket.getsockname()))
        self.ready.set()
        if verbose: print("thread: serving three times")
        svr.serve_a_few()
        if verbose: print("thread: done")

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
        if os.name == 'os2':
            # AF_UNIX socket names on OS/2 require a specific prefix
            # which can't include a drive letter and must also use
            # backslashes as directory separators
            if fn[1] == ':':
                fn = fn[2:]
            if fn[0] in (os.sep, os.altsep):
                fn = fn[1:]
            fn = os.path.join('\socket', fn)
            if os.sep == '/':
                fn = fn.replace(os.sep, os.altsep)
            else:
                fn = fn.replace(os.altsep, os.sep)
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
            print("ADDR =", addr)
            print("CLASS =", svrcls)
        t = ServerThread(addr, svrcls, hdlrcls)
        if verbose: print("server created")
        t.start()
        if verbose: print("server running")
        for i in range(NREQ):
            t.ready.wait(10*DELAY)
            if not t.ready.isSet():
                raise RuntimeError("Server not ready within a reasonable time")
            if verbose: print("test client", i)
            testfunc(proto, addr)
        if verbose: print("waiting for server")
        t.join()
        if verbose: print("done")

class ForgivingTCPServer(TCPServer):
    # prevent errors if another process is using the port we want
    def server_bind(self):
        host, default_port = self.server_address
        # this code shamelessly stolen from test.test_support
        # the ports were changed to protect the innocent
        import sys
        for port in [default_port, 3434, 8798, 23833]:
            try:
                self.server_address = host, port
                TCPServer.server_bind(self)
                break
            except socket.error as e:
                (err, msg) = e
                if err != errno.EADDRINUSE:
                    raise
                print('  WARNING: failed to listen on port %d, trying another' % port, file=sys.__stderr__)

tcpservers = [ForgivingTCPServer, ThreadingTCPServer]
if hasattr(os, 'fork') and os.name not in ('os2',):
    tcpservers.append(ForkingTCPServer)
udpservers = [UDPServer, ThreadingUDPServer]
if hasattr(os, 'fork') and os.name not in ('os2',):
    udpservers.append(ForkingUDPServer)

if not hasattr(socket, 'AF_UNIX'):
    streamservers = []
    dgramservers = []
else:
    class ForkingUnixStreamServer(ForkingMixIn, UnixStreamServer): pass
    streamservers = [UnixStreamServer, ThreadingUnixStreamServer]
    if hasattr(os, 'fork') and os.name not in ('os2',):
        streamservers.append(ForkingUnixStreamServer)
    class ForkingUnixDatagramServer(ForkingMixIn, UnixDatagramServer): pass
    dgramservers = [UnixDatagramServer, ThreadingUnixDatagramServer]
    if hasattr(os, 'fork') and os.name not in ('os2',):
        dgramservers.append(ForkingUnixDatagramServer)

def sloppy_cleanup():
    # See http://python.org/sf/1540386
    # We need to reap children here otherwise a child from one server
    # can be left running for the next server and cause a test failure.
    time.sleep(DELAY)
    reap_children()

def testall():
    testloop(socket.AF_INET, tcpservers, MyStreamHandler, teststream)
    sloppy_cleanup()
    testloop(socket.AF_INET, udpservers, MyDatagramHandler, testdgram)
    if hasattr(socket, 'AF_UNIX'):
        sloppy_cleanup()
        testloop(socket.AF_UNIX, streamservers, MyStreamHandler, teststream)
        # Alas, on Linux (at least) recvfrom() doesn't return a meaningful
        # client address so this cannot work:
        ##testloop(socket.AF_UNIX, dgramservers, MyDatagramHandler, testdgram)

def test_main():
    import imp
    if imp.lock_held():
        # If the import lock is held, the threads will hang.
        raise TestSkipped("can't run when import lock is held")

    reap_children()
    try:
        testall()
    finally:
        cleanup()
    reap_children()

if __name__ == "__main__":
    test_main()
