"""RPC Client module."""

import sys
import socket
import pickle
import __builtin__
import os


# Default verbosity (0 = silent, 1 = print connections, 2 = print requests too)
VERBOSE = 1


class Client:

    """RPC Client class.  No need to derive a class -- it's fully generic."""

    def __init__(self, address, verbose = VERBOSE):
        self._pre_init(address, verbose)
        self._post_init()

    def _pre_init(self, address, verbose = VERBOSE):
        if type(address) == type(0):
            address = ('', address)
        self._address = address
        self._verbose = verbose
        if self._verbose: print "Connecting to %s ..." % repr(address)
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._socket.connect(address)
        if self._verbose: print "Connected."
        self._lastid = 0 # Last id for which a reply has been received
        self._nextid = 1 # Id of next request
        self._replies = {} # Unprocessed replies
        self._rf = self._socket.makefile('r')
        self._wf = self._socket.makefile('w')

    def _post_init(self):
        self._methods = self._call('.methods')

    def __del__(self):
        self._close()

    def _close(self):
        if self._rf: self._rf.close()
        self._rf = None
        if self._wf: self._wf.close()
        self._wf = None
        if self._socket: self._socket.close()
        self._socket = None

    def __getattr__(self, name):
        if name in self._methods:
            method = _stub(self, name)
            setattr(self, name, method) # XXX circular reference
            return method
        raise AttributeError, name

    def _setverbose(self, verbose):
        self._verbose = verbose

    def _call(self, name, *args):
        return self._vcall(name, args)

    def _vcall(self, name, args):
        return self._recv(self._vsend(name, args))

    def _send(self, name, *args):
        return self._vsend(name, args)

    def _send_noreply(self, name, *args):
        return self._vsend(name, args, 0)

    def _vsend_noreply(self, name, args):
        return self._vsend(name, args, 0)

    def _vsend(self, name, args, wantreply = 1):
        id = self._nextid
        self._nextid = id+1
        if not wantreply: id = -id
        request = (name, args, id)
        if self._verbose > 1: print "sending request: %s" % repr(request)
        wp = pickle.Pickler(self._wf)
        wp.dump(request)
        return id

    def _recv(self, id):
        exception, value, rid = self._vrecv(id)
        if rid != id:
            raise RuntimeError, "request/reply id mismatch: %d/%d" % (id, rid)
        if exception is None:
            return value
        x = exception
        if hasattr(__builtin__, exception):
            x = getattr(__builtin__, exception)
        elif exception in ('posix.error', 'mac.error'):
            x = os.error
        if x == exception:
            exception = x
        raise exception, value

    def _vrecv(self, id):
        self._flush()
        if self._replies.has_key(id):
            if self._verbose > 1: print "retrieving previous reply, id = %d" % id
            reply = self._replies[id]
            del self._replies[id]
            return reply
        aid = abs(id)
        while 1:
            if self._verbose > 1: print "waiting for reply, id = %d" % id
            rp = pickle.Unpickler(self._rf)
            reply = rp.load()
            del rp
            if self._verbose > 1: print "got reply: %s" % repr(reply)
            rid = reply[2]
            arid = abs(rid)
            if arid == aid:
                if self._verbose > 1: print "got it"
                return reply
            self._replies[rid] = reply
            if arid > aid:
                if self._verbose > 1: print "got higher id, assume all ok"
                return (None, None, id)

    def _flush(self):
        self._wf.flush()


from security import Security


class SecureClient(Client, Security):

    def __init__(self, *args):
        import string
        apply(self._pre_init, args)
        Security.__init__(self)
        self._wf.flush()
        line = self._rf.readline()
        challenge = string.atoi(string.strip(line))
        response = self._encode_challenge(challenge)
        line = repr(long(response))
        if line[-1] in 'Ll': line = line[:-1]
        self._wf.write(line + '\n')
        self._wf.flush()
        self._post_init()

class _stub:

    """Helper class for Client -- each instance serves as a method of the client."""

    def __init__(self, client, name):
        self._client = client
        self._name = name

    def __call__(self, *args):
        return self._client._vcall(self._name, args)
