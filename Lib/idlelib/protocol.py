"""protocol        (David Scherer <dscherer@cmu.edu>)

     This module implements a simple RPC or "distributed object" protocol.
     I am probably the 100,000th person to write this in Python, but, hey,
     it was fun.

     Contents:

       connectionLost is an exception that will be thrown by functions in
           the protocol module or calls to remote methods that fail because
           the remote program has closed the socket or because no connection
           could be established in the first place.

       Server( port=None, connection_hook=None ) creates a server on a
           well-known port, to which clients can connect.  When a client
           connects, a Connection is created for it.  If connection_hook
           is defined, then connection_hook( socket.getpeername() ) is called
           before a Connection is created, and if it returns false then the
           connection is refused.  connection_hook must be prepared to be
           called from any thread.
  
       Client( ip='127.0.0.1', port=None ) returns a Connection to a Server
           object at a well-known address and port.
  
       Connection( socket ) creates an RPC connection on an arbitrary socket,
           which must already be connected to another program.  You do not
           need to use this directly if you are using Client() or Server().
  
       publish( name, connect_function ) provides an object with the
           specified name to some or all Connections.  When another program
           calls Connection.getobject() with the specified name, the
           specified connect_function is called with the arguments

              connect_function( conn, addr )

           where conn is the Connection object to the requesting client and
           addr is the address returned by socket.getpeername().  If that
           function returns an object, that object becomes accessible to
           the caller.  If it returns None, the caller's request fails.

     Connection objects:

       .close() refuses additional RPC messages from the peer, and notifies
           the peer that the connection has been closed.  All pending remote
           method calls in either program will fail with a connectionLost
           exception.  Further remote method calls on this connection will
           also result in errors.

       .getobject(name) returns a proxy for the remote object with the
           specified name, if it exists and the peer permits us access.
           Otherwise, it returns None.  It may throw a connectionLost
           exception.  The returned proxy supports basic attribute access
           and method calls, and its methods have an extra attribute,
           .void, which is a function that has the same effect but always
           returns None.  This last capability is provided as a performance
           hack: object.method.void(params) can return without waiting for
           the remote process to respond, but object.method(params) needs
           to wait for a return value or exception.

       .rpc_loop(block=0) processes *incoming* messages for this connection.
           If block=1, it continues processing until an exception or return
           value is received, which is normally forever.  Otherwise it
           returns when all currently pending messages have been delivered.
           It may throw a connectionLost exception.

       .set_close_hook(f) specifies a function to be called when the remote
           object closes the connection during a call to rpc_loop().  This
           is a good way for servers to be notified when clients disconnect.

       .set_shutdown_hook(f) specifies a function called *immediately* when
           the receive loop detects that the connection has been lost.  The
           provided function must be prepared to run in any thread.

     Server objects:

       .rpc_loop() processes incoming messages on all connections, and
           returns when all pending messages have been processed.  It will
           *not* throw connectionLost exceptions; the
           Connection.set_close_hook() mechanism is much better for servers.
"""

import sys, os, string, types
import socket
from threading import Thread
from Queue import Queue, Empty
from cPickle import Pickler, Unpickler, PicklingError

class connectionLost:
    def __init__(self, what=""): self.what = what
    def __repr__(self): return self.what
    def __str__(self): return self.what

def getmethods(cls):
    "Returns a list of the names of the methods of a class."
    methods = []
    for b in cls.__bases__:
        methods = methods + getmethods(b)
    d = cls.__dict__
    for k in d.keys():
        if type(d[k])==types.FunctionType:
            methods.append(k)
    return methods

class methodproxy:
    "Proxy for a method of a remote object."
    def __init__(self, classp, name):
        self.classp=classp
        self.name=name
        self.client = classp.client
    def __call__(self, *args, **keywords):
        return self.client.call( 'm', self.classp.name, self.name, args, keywords )

    def void(self, *args, **keywords):
        self.client.call_void( 'm', self.classp.name,self.name,args,keywords)

class classproxy:
    "Proxy for a remote object."
    def __init__(self, client, name, methods):
        self.__dict__['client'] = client
        self.__dict__['name'] = name
        
        for m in methods:
            prox = methodproxy( self, m )
            self.__dict__[m] = prox

    def __getattr__(self, attr):
        return self.client.call( 'g', self.name, attr )

    def __setattr__(self, attr, value):
        self.client.call_void( 's', self.name, attr, value )

local_connect  = {}
def publish(name, connect_function):
    local_connect[name]=connect_function

class socketFile:
    "File emulator based on a socket.  Provides only blocking semantics for now."

    def __init__(self, socket):
        self.socket = socket
        self.buffer = ''

    def _recv(self,bytes):
        try:
            r=self.socket.recv(bytes)
        except:
            raise connectionLost()
        if not r:
            raise connectionLost()
        return r

    def write(self, string):
        try:
            self.socket.send( string )
        except:
            raise connectionLost()

    def read(self,bytes):
        x = bytes-len(self.buffer)
        while x>0:
            self.buffer=self.buffer+self._recv(x)
            x = bytes-len(self.buffer)
        s = self.buffer[:bytes]
        self.buffer=self.buffer[bytes:]
        return s

    def readline(self):
        while 1:
            f = string.find(self.buffer,'\n')
            if f>=0:
                s = self.buffer[:f+1]
                self.buffer=self.buffer[f+1:]
                return s
            self.buffer = self.buffer + self._recv(1024)


class Connection (Thread):
    debug = 0
    def __init__(self, socket):
        self.local_objects = {}
        self.socket = socket
        self.name = socket.getpeername()
        self.socketfile = socketFile(socket)
        self.queue = Queue(-1)
        self.refuse_messages = 0
        self.cmds = { 'm': self.r_meth,
                      'g': self.r_get,
                      's': self.r_set,
                      'o': self.r_geto,
                      'e': self.r_exc,
                     #'r' handled by rpc_loop
                    }

        Thread.__init__(self)
        self.setDaemon(1)
        self.start()

    def getobject(self, name):
        methods = self.call( 'o', name )
        if methods is None: return None
        return classproxy(self, name, methods)

    # close_hook is called from rpc_loop(), like a normal remote method
    #   invocation
    def set_close_hook(self,hook): self.close_hook = hook

    # shutdown_hook is called directly from the run() thread, and needs
    #   to be "thread safe"
    def set_shutdown_hook(self,hook): self.shutdown_hook = hook

    close_hook = None
    shutdown_hook = None

    def close(self):
        self._shutdown()
        self.refuse_messages = 1

    def call(self, c, *args):
        self.send( (c, args, 1 ) )
        return self.rpc_loop( block = 1 )

    def call_void(self, c, *args):
        try:
            self.send( (c, args, 0 ) )
        except:
            pass
   
    # the following methods handle individual RPC calls:

    def r_geto(self, obj):
        c = local_connect.get(obj)
        if not c: return None
        o = c(self, self.name)
        if not o: return None
        self.local_objects[obj] = o
        return getmethods(o.__class__)

    def r_meth(self, obj, name, args, keywords):
        return apply( getattr(self.local_objects[obj],name), args, keywords)

    def r_get(self, obj, name):       
        return getattr(self.local_objects[obj],name)

    def r_set(self, obj, name, value):
        setattr(self.local_objects[obj],name,value)

    def r_exc(self, e, v):
        raise e, v

    def rpc_exec(self, cmd, arg, ret):
        if self.refuse_messages: return
        if self.debug: print cmd,arg,ret
        if ret:
            try:
                r=apply(self.cmds.get(cmd), arg)
                self.send( ('r', r, 0) )
            except:
                try:
                    self.send( ('e', sys.exc_info()[:2], 0) )
                except PicklingError:
                    self.send( ('e', (TypeError, 'Unpicklable exception.'), 0 ) )
        else:
            # we cannot report exceptions to the caller, so
            #   we report them in this process.
            r=apply(self.cmds.get(cmd), arg)

    # the following methods implement the RPC and message loops:

    def rpc_loop(self, block=0):
        if self.refuse_messages: raise connectionLost('(already closed)')
        try:
            while 1:
                try:
                    cmd, arg, ret = self.queue.get( block )
                except Empty:
                    return None
                if cmd=='r': return arg
                self.rpc_exec(cmd,arg,ret)
        except connectionLost:
            if self.close_hook:
                self.close_hook()
                self.close_hook = None
            raise

    def run(self):
        try:
            while 1:
                data = self.recv()
                self.queue.put( data )
        except:
            self.queue.put( ('e', sys.exc_info()[:2], 0) )

    # The following send raw pickled data to the peer

    def send(self, data):
        try:
            Pickler(self.socketfile,1).dump( data )
        except connectionLost:
            self._shutdown()
            if self.shutdown_hook: self.shutdown_hook()
            raise

    def recv(self):
        try:
            return Unpickler(self.socketfile).load()
        except connectionLost:
            self._shutdown()
            if self.shutdown_hook: self.shutdown_hook()
            raise
        except:
            raise

    def _shutdown(self):
        try:
            self.socket.shutdown(1)
            self.socket.close()
        except:
            pass


class Server (Thread):
    default_port = 0x1D1E   # "IDlE"

    def __init__(self, port=None, connection_hook=None):
        self.connections = []
        self.port = port or self.default_port
        self.connection_hook = connection_hook

        try:
            self.wellknown = s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.bind(('', self.port))
            s.listen(3)
        except:
            raise connectionLost

        Thread.__init__(self)
        self.setDaemon(1)
        self.start()

    def run(self):
        s = self.wellknown
        while 1:
            conn, addr = s.accept()
            if self.connection_hook and not self.connection_hook(addr):
                try:
                    conn.shutdown(1)
                except:
                    pass
                continue
            self.connections.append( Connection(conn) )

    def rpc_loop(self):
        cns = self.connections[:]
        for c in cns:
            try:
                c.rpc_loop(block = 0)
            except connectionLost:
                if c in self.connections:
                    self.connections.remove(c)

def Client(ip='127.0.0.1', port=None):
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((ip,port or Server.default_port))
    except socket.error, what:
        raise connectionLost(str(what))
    except:
        raise connectionLost()
    return Connection(s)
