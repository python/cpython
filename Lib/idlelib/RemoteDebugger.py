"""Support for remote Python debugging.

Some ASCII art to describe the structure:

       IN PYTHON SUBPROCESS          #             IN IDLE PROCESS
                                     #
                                     #        oid='gui_adapter'
                 +----------+        #       +------------+          +-----+
                 | GUIProxy |--remote#call-->| GUIAdapter |--calls-->| GUI |
+-----+--calls-->+----------+        #       +------------+          +-----+
| Idb |                               #                             /
+-----+<-calls--+------------+         #      +----------+<--calls-/
                | IdbAdapter |<--remote#call--| IdbProxy |
                +------------+         #      +----------+
                oid='idb_adapter'      #

The purpose of the Proxy and Adapter classes is to translate certain
arguments and return values that cannot be transported through the RPC
barrier, in particular frame and traceback objects.

"""

import sys
import rpc
import Debugger

debugging = 0

# In the PYTHON subprocess

frametable = {}
dicttable = {}
codetable = {}

def wrap_frame(frame):
    fid = id(frame)
    frametable[fid] = frame
    return fid

def wrap_info(info):
    if info is None:
        return None
    else:
        return None # XXX for now

class GUIProxy:

    def __init__(self, conn, gui_adap_oid):
        self.conn = conn
        self.oid = gui_adap_oid

    def interaction(self, message, frame, info=None):
        self.conn.remotecall(self.oid, "interaction",
                             (message, wrap_frame(frame), wrap_info(info)),
                             {})

class IdbAdapter:

    def __init__(self, idb):
        self.idb = idb

    def set_step(self):
        self.idb.set_step()

    def set_quit(self):
        self.idb.set_quit()

    def set_continue(self):
        self.idb.set_continue()

    def set_next(self, fid):
        frame = frametable[fid]
        self.idb.set_next(frame)

    def set_return(self, fid):
        frame = frametable[fid]
        self.idb.set_return(frame)

    def get_stack(self, fid, tbid):
        ##print >>sys.__stderr__, "get_stack(%s, %s)" % (`fid`, `tbid`)
        frame = frametable[fid]
        tb = None # XXX for now
        stack, i = self.idb.get_stack(frame, tb)
        ##print >>sys.__stderr__, "get_stack() ->", stack
        stack = [(wrap_frame(frame), k) for frame, k in stack]
        ##print >>sys.__stderr__, "get_stack() ->", stack
        return stack, i

    def run(self, cmd):
        import __main__
        self.idb.run(cmd, __main__.__dict__)

    def frame_attr(self, fid, name):
        frame = frametable[fid]
        return getattr(frame, name)

    def frame_globals(self, fid):
        frame = frametable[fid]
        dict = frame.f_globals
        did = id(dict)
        dicttable[did] = dict
        return did

    def frame_locals(self, fid):
        frame = frametable[fid]
        dict = frame.f_locals
        did = id(dict)
        dicttable[did] = dict
        return did

    def frame_code(self, fid):
        frame = frametable[fid]
        code = frame.f_code
        cid = id(code)
        codetable[cid] = code
        return cid

    def code_name(self, cid):
        code = codetable[cid]
        return code.co_name

    def code_filename(self, cid):
        code = codetable[cid]
        return code.co_filename

    def dict_keys(self, did):
        dict = dicttable[did]
        return dict.keys()

    def dict_item(self, did, key):
        dict = dicttable[did]
        value = dict[key]
        value = repr(value)
#          try:
#              # Test for picklability
#              import cPickle
#              pklstr = cPickle.dumps(value)
#          except:
#              print >>sys.__stderr__, "** dict_item pickle failed: ", value
#              raise    
#              #value = None
        return value

def start_debugger(conn, gui_adap_oid):
    "Launch debugger in the remote python subprocess"
    gui_proxy = GUIProxy(conn, gui_adap_oid)
    idb = Debugger.Idb(gui_proxy)
    idb_adap = IdbAdapter(idb)
    idb_adap_oid = "idb_adapter"
    conn.register(idb_adap_oid, idb_adap)
    return idb_adap_oid

# In the IDLE process

class FrameProxy:

    def __init__(self, conn, fid):
        self._conn = conn
        self._fid = fid
        self._oid = "idb_adapter"
        self._dictcache = {}

    def __getattr__(self, name):
        if name[:1] == "_":
            raise AttributeError, name
        if name == "f_code":
            return self._get_f_code()
        if name == "f_globals":
            return self._get_f_globals()
        if name == "f_locals":
            return self._get_f_locals()
        return self._conn.remotecall(self._oid, "frame_attr",
                                     (self._fid, name), {})

    def _get_f_code(self):
        cid = self._conn.remotecall(self._oid, "frame_code", (self._fid,), {})
        return CodeProxy(self._conn, self._oid, cid)

    def _get_f_globals(self):
        did = self._conn.remotecall(self._oid, "frame_globals",
                                    (self._fid,), {})
        return self._get_dict_proxy(did)

    def _get_f_locals(self):
        did = self._conn.remotecall(self._oid, "frame_locals",
                                    (self._fid,), {})
        return self._get_dict_proxy(did)

    def _get_dict_proxy(self, did):
        if self._dictcache.has_key(did):
            return self._dictcache[did]
        dp = DictProxy(self._conn, self._oid, did)
        self._dictcache[did] = dp
        return dp

class CodeProxy:

    def __init__(self, conn, oid, cid):
        self._conn = conn
        self._oid = oid
        self._cid = cid

    def __getattr__(self, name):
        if name == "co_name":
            return self._conn.remotecall(self._oid, "code_name",
                                         (self._cid,), {})
        if name == "co_filename":
            return self._conn.remotecall(self._oid, "code_filename",
                                         (self._cid,), {})

class DictProxy:

    def __init__(self, conn, oid, did):
        self._conn = conn
        self._oid = oid
        self._did = did

    def keys(self):
        return self._conn.remotecall(self._oid, "dict_keys", (self._did,), {})

    def __getitem__(self, key):
        return self._conn.remotecall(self._oid, "dict_item",
                                     (self._did, key), {})

    def __getattr__(self, name):
        ##print >>sys.__stderr__, "failed DictProxy.__getattr__:", name
        raise AttributeError, name

class GUIAdapter:

    def __init__(self, conn, gui):
        self.conn = conn
        self.gui = gui

    def interaction(self, message, fid, iid):
        ##print "interaction: (%s, %s, %s)" % (`message`,`fid`, `iid`)
        frame = FrameProxy(self.conn, fid)
        info = None # XXX for now
        self.gui.interaction(message, frame, info)

class IdbProxy:

    def __init__(self, conn, oid):
        self.oid = oid
        self.conn = conn

    def call(self, methodname, *args, **kwargs):
        ##print "call %s %s %s" % (methodname, args, kwargs)
        value = self.conn.remotecall(self.oid, methodname, args, kwargs)
        ##print "return %s" % `value`
        return value

    def run(self, cmd, locals):
        # Ignores locals on purpose!
        self.call("run", cmd)

    def get_stack(self, frame, tb):
        stack, i = self.call("get_stack", frame._fid, None)
        stack = [(FrameProxy(self.conn, fid), k) for fid, k in stack]
        return stack, i

    def set_continue(self):
        self.call("set_continue")

    def set_step(self):
        self.call("set_step")

    def set_next(self, frame):
        self.call("set_next", frame._fid)

    def set_return(self, frame):
        self.call("set_return", frame._fid)

    def set_quit(self):
        self.call("set_quit")

def start_remote_debugger(conn, pyshell):
    """Start the subprocess debugger, initialize the debugger GUI and RPC link

    Start the debugger in the remote Python process.  Instantiate IdbProxy,
    Debugger GUI, and Debugger GUIAdapter objects, and link them together.

    The GUIAdapter will handle debugger GUI interaction requests coming from
    the subprocess debugger via the GUIProxy.

    The IdbAdapter will pass execution and environment requests coming from the
    Idle debugger GUI to the subprocess debugger via the IdbProxy.

    """
    gui_adap_oid = "gui_adapter"
    idb_adap_oid = conn.remotecall("exec", "start_the_debugger",\
                                   (gui_adap_oid,), {})
    idb_proxy = IdbProxy(conn, idb_adap_oid)
    gui = Debugger.Debugger(pyshell, idb_proxy)
    gui_adap = GUIAdapter(conn, gui)
    conn.register(gui_adap_oid, gui_adap)
    return gui
