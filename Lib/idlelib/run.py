import sys
import rpc

def main():
    port = 8833
    if sys.argv[1:]:
        port = int(sys.argv[1])
    sys.argv[:] = [""]
    addr = ("localhost", port)
    svr = rpc.RPCServer(addr, MyHandler)
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
        import __main__
        self.locals = __main__.__dict__

    def runcode(self, code):
        exec code in self.locals

    def start_the_debugger(self, gui_adap_oid):
        import RemoteDebugger
        return RemoteDebugger.start_debugger(self.rpchandler, gui_adap_oid)

    def stop_the_debugger(self, idb_adap_oid):
        "Unregister the Idb Adapter.  Link objects and Idb then subject to GC"
        self.rpchandler.unregister(idb_adap_oid)

    def stackviewer(self, flist_oid=None):
        if not hasattr(sys, "last_traceback"):
            return None
        flist = None
        if flist_oid is not None:
            flist = self.rpchandler.get_remote_proxy(flist_oid)
        import RemoteObjectBrowser
        import StackViewer
        tb = sys.last_traceback
        while tb and tb.tb_frame.f_globals["__name__"] in ["rpc", "run"]:
            tb = tb.tb_next
        item = StackViewer.StackTreeItem(flist, tb)
        return RemoteObjectBrowser.remote_object_tree_item(item)
