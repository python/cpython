"""RPC Implemention, originally written for the Python Idle IDE

For security reasons, GvR requested that Idle's Python execution server process
connect to the Idle process, which listens for the connection.  Since Idle has
has only one client per server, this was not a limitation.

   +---------------------------------+ +-------------+
   | SocketServer.BaseRequestHandler | | SocketIO    |
   +---------------------------------+ +-------------+
                   ^                   | register()  |
                   |                   | unregister()|
                   |                   +-------------+
                   |                      ^  ^
                   |                      |  |
                   | + -------------------+  |
                   | |                       |
   +-------------------------+        +-----------------+
   | RPCHandler              |        | RPCClient       |
   | [attribute of RPCServer]|        |                 |
   +-------------------------+        +-----------------+

The RPCServer handler class is expected to provide register/unregister methods.
RPCHandler inherits the mix-in class SocketIO, which provides these methods.

See the Idle run.main() docstring for further information on how this was
accomplished in Idle.

"""

import sys
import socket
import select
import SocketServer
import struct
import cPickle as pickle
import threading
import traceback
import copy_reg
import types
import marshal

def unpickle_code(ms):
    co = marshal.loads(ms)
    assert isinstance(co, types.CodeType)
    return co

def pickle_code(co):
    assert isinstance(co, types.CodeType)
    ms = marshal.dumps(co)
    return unpickle_code, (ms,)

def unpickle_function(ms):
    return ms

def pickle_function(fn):
    assert isinstance(fn, type.FunctionType)
    return `fn`
 
copy_reg.pickle(types.CodeType, pickle_code, unpickle_code)
copy_reg.pickle(types.FunctionType, pickle_function, unpickle_function)

BUFSIZE = 8*1024

class RPCServer(SocketServer.TCPServer):

    def __init__(self, addr, handlerclass=None):
        if handlerclass is None:
            handlerclass = RPCHandler
# XXX KBK 25Jun02 Not used in Idlefork.
#        self.objtable = objecttable 
        SocketServer.TCPServer.__init__(self, addr, handlerclass)

    def server_bind(self):
        "Override TCPServer method, no bind() phase for connecting entity"
        pass

    def server_activate(self):
        """Override TCPServer method, connect() instead of listen()
        
        Due to the reversed connection, self.server_address is actually the
        address of the Idle Client to which we are connecting.

        """
        self.socket.connect(self.server_address)
        
    def get_request(self):
        "Override TCPServer method, return already connected socket"
        return self.socket, self.server_address
        

# XXX The following two methods are not currently used in Idlefork.
#      def register(self, oid, object):
#          self.objtable[oid] = object

#      def unregister(self, oid):
#          try:
#              del self.objtable[oid]
#          except KeyError:
#              pass


objecttable = {}

class SocketIO:

    debugging = 0

    def __init__(self, sock, objtable=None, debugging=None):
        self.mainthread = threading.currentThread()
        if debugging is not None:
            self.debugging = debugging
        self.sock = sock
        if objtable is None:
            objtable = objecttable
        self.objtable = objtable
        self.statelock = threading.Lock()
        self.responses = {}
        self.cvars = {}

    def close(self):
        sock = self.sock
        self.sock = None
        if sock is not None:
            sock.close()

    def debug(self, *args):
        if not self.debugging:
            return
        s = str(threading.currentThread().getName())
        for a in args:
            s = s + " " + str(a)
        s = s + "\n"
        sys.__stderr__.write(s)

    def register(self, oid, object):
        self.objtable[oid] = object

    def unregister(self, oid):
        try:
            del self.objtable[oid]
        except KeyError:
            pass

    def localcall(self, request):
        self.debug("localcall:", request) 
        try:
            how, (oid, methodname, args, kwargs) = request
        except TypeError:
            return ("ERROR", "Bad request format")
        assert how == "call"
        if not self.objtable.has_key(oid):
            return ("ERROR", "Unknown object id: %s" % `oid`)
        obj = self.objtable[oid]
        if methodname == "__methods__":
            methods = {}
            _getmethods(obj, methods)
            return ("OK", methods)
        if methodname == "__attributes__":
            attributes = {}
            _getattributes(obj, attributes)
            return ("OK", attributes)
        if not hasattr(obj, methodname):
            return ("ERROR", "Unsupported method name: %s" % `methodname`)
        method = getattr(obj, methodname)
        try:
            ret = method(*args, **kwargs)
            if isinstance(ret, RemoteObject):
                ret = remoteref(ret)
            return ("OK", ret)
        except:
            ##traceback.print_exc(file=sys.__stderr__)
            typ, val, tb = info = sys.exc_info()
            sys.last_type, sys.last_value, sys.last_traceback = info
            if isinstance(typ, type(Exception)):
                # Class exceptions
                mod = typ.__module__
                name = typ.__name__
                if issubclass(typ, Exception):
                    args = val.args
                else:
                    args = (str(val),)
            else:
                # String exceptions
                mod = None
                name = typ
                args = (str(val),)
            tb = traceback.extract_tb(tb)
            return ("EXCEPTION", (mod, name, args, tb))

    def remotecall(self, oid, methodname, args, kwargs):
        self.debug("remotecall:", oid, methodname, args, kwargs) 
        seq = self.asynccall(oid, methodname, args, kwargs)
        return self.asyncreturn(seq)

    def asynccall(self, oid, methodname, args, kwargs):
        request = ("call", (oid, methodname, args, kwargs))
        seq = self.putrequest(request)
        return seq

    def asyncreturn(self, seq):
        response = self.getresponse(seq)
        return self.decoderesponse(response)

    def decoderesponse(self, response):
        how, what = response
        if how == "OK":
            return what
        if how == "EXCEPTION":
            mod, name, args, tb = what
            self.traceback = tb
            if mod:
                try:
                    __import__(mod)
                    module = sys.modules[mod]
                except ImportError:
                    pass
                else:
                    try:
                        cls = getattr(module, name)
                    except AttributeError:
                        pass
                    else:
                        raise getattr(__import__(mod), name)(*args)
            raise name, args
        if how == "ERROR":
            raise RuntimeError, what
        raise SystemError, (how, what)

    def mainloop(self):
        try:
            self.getresponse(None)
        except EOFError:
            pass

    def getresponse(self, myseq):
        response = self._getresponse(myseq)
        if response is not None:
            how, what = response
            if how == "OK":
                response = how, self._proxify(what)
        return response

    def _proxify(self, obj):
        if isinstance(obj, RemoteProxy):
            return RPCProxy(self, obj.oid)
        if isinstance(obj, types.ListType):
            return map(self._proxify, obj)
        # XXX Check for other types -- not currently needed
        return obj

    def _getresponse(self, myseq):
        if threading.currentThread() is self.mainthread:
            # Main thread: does all reading of requests and responses
            while 1:
                response = self.pollresponse(myseq, None)
                if response is not None:
                    return response
        else:
            # Auxiliary thread: wait for notification from main thread
            cvar = threading.Condition(self.statelock)
            self.statelock.acquire()
            self.cvars[myseq] = cvar
            while not self.responses.has_key(myseq):
                cvar.wait()
            response = self.responses[myseq]
            del self.responses[myseq]
            del self.cvars[myseq]
            self.statelock.release()
            return response

    def putrequest(self, request):
        seq = self.newseq()
        self.putmessage((seq, request))
        return seq

    nextseq = 0

    def newseq(self):
        self.nextseq = seq = self.nextseq + 2
        return seq

    def putmessage(self, message):
        try:
            s = pickle.dumps(message)
        except:
            print >>sys.__stderr__, "Cannot pickle:", `message`
            raise
        s = struct.pack("<i", len(s)) + s
        while len(s) > 0:
            n = self.sock.send(s)
            s = s[n:]

    def ioready(self, wait=0.0):
        r, w, x = select.select([self.sock.fileno()], [], [], wait)
        return len(r)

    buffer = ""
    bufneed = 4
    bufstate = 0 # meaning: 0 => reading count; 1 => reading data

    def pollpacket(self, wait=0.0):
        self._stage0()
        if len(self.buffer) < self.bufneed:
            if not self.ioready(wait):
                return None
            try:
                s = self.sock.recv(BUFSIZE)
            except socket.error:
                raise EOFError
            if len(s) == 0:
                raise EOFError
            self.buffer += s
            self._stage0()
        return self._stage1()

    def _stage0(self):
        if self.bufstate == 0 and len(self.buffer) >= 4:
            s = self.buffer[:4]
            self.buffer = self.buffer[4:]
            self.bufneed = struct.unpack("<i", s)[0]
            self.bufstate = 1

    def _stage1(self):
        if self.bufstate == 1 and len(self.buffer) >= self.bufneed:
            packet = self.buffer[:self.bufneed]
            self.buffer = self.buffer[self.bufneed:]
            self.bufneed = 4
            self.bufstate = 0
            return packet

    def pollmessage(self, wait=0.0):
        packet = self.pollpacket(wait)
        if packet is None:
            return None
        try:
            message = pickle.loads(packet)
        except:
            print >>sys.__stderr__, "-----------------------"
            print >>sys.__stderr__, "cannot unpickle packet:", `packet`
            traceback.print_stack(file=sys.__stderr__)
            print >>sys.__stderr__, "-----------------------"
            raise
        return message

    def pollresponse(self, myseq, wait=0.0):
        # Loop while there's no more buffered input or until specific response
        while 1:
            message = self.pollmessage(wait)
            if message is None:
                return None
            wait = 0.0
            seq, resq = message
            if resq[0] == "call":
                response = self.localcall(resq)
                self.putmessage((seq, response))
                continue
            elif seq == myseq:
                return resq
            else:
                self.statelock.acquire()
                self.responses[seq] = resq
                cv = self.cvars.get(seq)
                if cv is not None:
                    cv.notify()
                self.statelock.release()
                continue
            
#----------------- end class SocketIO --------------------

class RemoteObject:
    # Token mix-in class
    pass

def remoteref(obj):
    oid = id(obj)
    objecttable[oid] = obj
    return RemoteProxy(oid)

class RemoteProxy:

    def __init__(self, oid):
        self.oid = oid

class RPCHandler(SocketServer.BaseRequestHandler, SocketIO):

    debugging = 0

    def __init__(self, sock, addr, svr):
        svr.current_handler = self ## cgt xxx
        SocketIO.__init__(self, sock)
        SocketServer.BaseRequestHandler.__init__(self, sock, addr, svr)

    def handle(self):
        "handle() method required by SocketServer"
        self.mainloop()

    def get_remote_proxy(self, oid):
        return RPCProxy(self, oid)

class RPCClient(SocketIO):

    nextseq = 1 # Requests coming from the client are odd numbered

    def __init__(self, address, family=socket.AF_INET, type=socket.SOCK_STREAM):
        self.sock = socket.socket(family, type)
        self.sock.bind(address)
        self.sock.listen(1)

    def accept(self):
        newsock, address = self.sock.accept()
        if address[0] == '127.0.0.1':
            print>>sys.__stderr__, "Idle accepted connection from ", address
            SocketIO.__init__(self, newsock)
        else:
            print>>sys.__stderr__, "Invalid host: ", address
            raise socket.error

    def get_remote_proxy(self, oid):
        return RPCProxy(self, oid)

class RPCProxy:

    __methods = None
    __attributes = None

    def __init__(self, sockio, oid):
        self.sockio = sockio
        self.oid = oid

    def __getattr__(self, name):
        if self.__methods is None:
            self.__getmethods()
        if self.__methods.get(name):
            return MethodProxy(self.sockio, self.oid, name)
        if self.__attributes is None:
            self.__getattributes()
        if not self.__attributes.has_key(name):
            raise AttributeError, name
    __getattr__.DebuggerStepThrough=1

    def __getattributes(self):
        self.__attributes = self.sockio.remotecall(self.oid,
                                                "__attributes__", (), {})

    def __getmethods(self):
        self.__methods = self.sockio.remotecall(self.oid,
                                                "__methods__", (), {})

def _getmethods(obj, methods):
    # Helper to get a list of methods from an object
    # Adds names to dictionary argument 'methods'
    for name in dir(obj):
        attr = getattr(obj, name)
        if callable(attr):
            methods[name] = 1
    if type(obj) == types.InstanceType:
        _getmethods(obj.__class__, methods)
    if type(obj) == types.ClassType:
        for super in obj.__bases__:
            _getmethods(super, methods)

def _getattributes(obj, attributes):
    for name in dir(obj):
        attr = getattr(obj, name)
        if not callable(attr):
           attributes[name] = 1

class MethodProxy:

    def __init__(self, sockio, oid, name):
        self.sockio = sockio
        self.oid = oid
        self.name = name

    def __call__(self, *args, **kwargs):
        value = self.sockio.remotecall(self.oid, self.name, args, kwargs)
        return value

#
# Self Test
#

def testServer(addr):
    # XXX 25 Jul 02 KBK needs update to use rpc.py register/unregister methods
    class RemotePerson:
        def __init__(self,name):
            self.name = name 
        def greet(self, name):
            print "(someone called greet)"
            print "Hello %s, I am %s." % (name, self.name)
            print
        def getName(self):
            print "(someone called getName)"
            print 
            return self.name
        def greet_this_guy(self, name):
            print "(someone called greet_this_guy)"
            print "About to greet %s ..." % name
            remote_guy = self.server.current_handler.get_remote_proxy(name)
            remote_guy.greet("Thomas Edison")
            print "Done."
            print
            
    person = RemotePerson("Thomas Edison")
    svr = RPCServer(addr)
    svr.register('thomas', person)
    person.server = svr # only required if callbacks are used

    # svr.serve_forever()
    svr.handle_request()  # process once only

def testClient(addr):
    "demonstrates RPC Client"
    # XXX 25 Jul 02 KBK needs update to use rpc.py register/unregister methods
    import time
    clt=RPCClient(addr)
    thomas = clt.get_remote_proxy("thomas")
    print "The remote person's name is ..."
    print thomas.getName()
    # print clt.remotecall("thomas", "getName", (), {})
    print
    time.sleep(1)
    print "Getting remote thomas to say hi..."
    thomas.greet("Alexander Bell")
    #clt.remotecall("thomas","greet",("Alexander Bell",), {})
    print "Done."
    print 
    time.sleep(2)
    # demonstrates remote server calling local instance
    class LocalPerson:
        def __init__(self,name):
            self.name = name 
        def greet(self, name):
            print "You've greeted me!"
        def getName(self):
            return self.name
    person = LocalPerson("Alexander Bell")
    clt.register("alexander",person)
    thomas.greet_this_guy("alexander")
    # clt.remotecall("thomas","greet_this_guy",("alexander",), {})

def test():
    addr=("localhost",8833)
    if len(sys.argv) == 2:
        if sys.argv[1]=='-server':
            testServer(addr)
            return
    testClient(addr)

if __name__ == '__main__':
    test()

        
