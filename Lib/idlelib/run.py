import sys
import time
import socket

import boolcheck

import CallTips
import RemoteDebugger
import RemoteObjectBrowser
import StackViewer
import rpc

import __main__

def main():
    """Start the Python execution server in a subprocess

    In the Python subprocess, RPCServer is instantiated with handlerclass
    MyHandler, which inherits register/unregister methods from RPCHandler via
    the mix-in class SocketIO.

    When the RPCServer svr is instantiated, the TCPServer initialization
    creates an instance of run.MyHandler and calls its handle() method.
    handle() instantiates a run.Executive object, passing it a reference to the
    MyHandler object.  That reference is saved as attribute rpchandler of the
    Executive instance.  The Executive methods have access to the reference and
    can pass it on to entities that they command
    (e.g. RemoteDebugger.Debugger.start_debugger()).  The latter, in turn, can
    call MyHandler(SocketIO) register/unregister methods via the reference to
    register and unregister themselves.

    """
    port = 8833
    if sys.argv[1:]:
        port = int(sys.argv[1])
    sys.argv[:] = [""]
    addr = ("localhost", port)
    for i in range(6):
        time.sleep(i)
        try:
            svr = rpc.RPCServer(addr, MyHandler)
            break
        except socket.error, err:
            if i < 3:
                print>>sys.__stderr__, ".. ",
            else:
                print>>sys.__stderr__,"\nPython subprocess socket error: "\
                                              + err[1] + ", retrying...."
    else:
        print>>sys.__stderr__, "\nConnection to Idle failed, exiting."
        sys.exit()
    svr.handle_request() # A single request only

class MyHandler(rpc.RPCHandler):

    def handle(self):
        executive = Executive(self)
        self.register("exec", executive)
        sys.stdin = self.get_remote_proxy("stdin")
        sys.stdout = self.get_remote_proxy("stdout")
        sys.stderr = self.get_remote_proxy("stderr")
        rpc.RPCHandler.handle(self)

class Executive:

    def __init__(self, rpchandler):
        self.rpchandler = rpchandler
        self.locals = __main__.__dict__
        self.calltip = CallTips.CallTips()

    def runcode(self, code):
        exec code in self.locals

    def start_the_debugger(self, gui_adap_oid):
        return RemoteDebugger.start_debugger(self.rpchandler, gui_adap_oid)

    def stop_the_debugger(self, idb_adap_oid):
        "Unregister the Idb Adapter.  Link objects and Idb then subject to GC"
        self.rpchandler.unregister(idb_adap_oid)

    def get_the_calltip(self, name):
        return self.calltip.fetch_tip(name)

    def stackviewer(self, flist_oid=None):
        if not hasattr(sys, "last_traceback"):
            return None
        flist = None
        if flist_oid is not None:
            flist = self.rpchandler.get_remote_proxy(flist_oid)
        tb = sys.last_traceback
        while tb and tb.tb_frame.f_globals["__name__"] in ["rpc", "run"]:
            tb = tb.tb_next
        item = StackViewer.StackTreeItem(flist, tb)
        return RemoteObjectBrowser.remote_object_tree_item(item)
