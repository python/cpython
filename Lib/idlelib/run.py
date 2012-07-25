import sys
import io
import linecache
import time
import socket
import traceback
import _thread as thread
import threading
import queue
import tkinter

from idlelib import CallTips
from idlelib import AutoComplete

from idlelib import RemoteDebugger
from idlelib import RemoteObjectBrowser
from idlelib import StackViewer
from idlelib import rpc

import __main__

LOCALHOST = '127.0.0.1'

try:
    import warnings
except ImportError:
    pass
else:
    def idle_formatwarning_subproc(message, category, filename, lineno,
                                   line=None):
        """Format warnings the IDLE way"""
        s = "\nWarning (from warnings module):\n"
        s += '  File \"%s\", line %s\n' % (filename, lineno)
        if line is None:
            line = linecache.getline(filename, lineno)
        line = line.strip()
        if line:
            s += "    %s\n" % line
        s += "%s: %s\n" % (category.__name__, message)
        return s
    warnings.formatwarning = idle_formatwarning_subproc


tcl = tkinter.Tcl()


def handle_tk_events(tcl=tcl):
    """Process any tk events that are ready to be dispatched if tkinter
    has been imported, a tcl interpreter has been created and tk has been
    loaded."""
    tcl.eval("update")


# Thread shared globals: Establish a queue between a subthread (which handles
# the socket) and the main thread (which runs user code), plus global
# completion, exit and interruptable (the main thread) flags:

exit_now = False
quitting = False
interruptable = False

def main(del_exitfunc=False):
    """Start the Python execution server in a subprocess

    In the Python subprocess, RPCServer is instantiated with handlerclass
    MyHandler, which inherits register/unregister methods from RPCHandler via
    the mix-in class SocketIO.

    When the RPCServer 'server' is instantiated, the TCPServer initialization
    creates an instance of run.MyHandler and calls its handle() method.
    handle() instantiates a run.Executive object, passing it a reference to the
    MyHandler object.  That reference is saved as attribute rpchandler of the
    Executive instance.  The Executive methods have access to the reference and
    can pass it on to entities that they command
    (e.g. RemoteDebugger.Debugger.start_debugger()).  The latter, in turn, can
    call MyHandler(SocketIO) register/unregister methods via the reference to
    register and unregister themselves.

    """
    global exit_now
    global quitting
    global no_exitfunc
    no_exitfunc = del_exitfunc
    #time.sleep(15) # test subprocess not responding
    try:
        assert(len(sys.argv) > 1)
        port = int(sys.argv[-1])
    except:
        print("IDLE Subprocess: no IP port passed in sys.argv.",
              file=sys.__stderr__)
        return
    sys.argv[:] = [""]
    sockthread = threading.Thread(target=manage_socket,
                                  name='SockThread',
                                  args=((LOCALHOST, port),))
    sockthread.daemon = True
    sockthread.start()
    while 1:
        try:
            if exit_now:
                try:
                    exit()
                except KeyboardInterrupt:
                    # exiting but got an extra KBI? Try again!
                    continue
            try:
                seq, request = rpc.request_queue.get(block=True, timeout=0.05)
            except queue.Empty:
                handle_tk_events()
                continue
            method, args, kwargs = request
            ret = method(*args, **kwargs)
            rpc.response_queue.put((seq, ret))
        except KeyboardInterrupt:
            if quitting:
                exit_now = True
            continue
        except SystemExit:
            raise
        except:
            type, value, tb = sys.exc_info()
            try:
                print_exception()
                rpc.response_queue.put((seq, None))
            except:
                # Link didn't work, print same exception to __stderr__
                traceback.print_exception(type, value, tb, file=sys.__stderr__)
                exit()
            else:
                continue

def manage_socket(address):
    for i in range(3):
        time.sleep(i)
        try:
            server = MyRPCServer(address, MyHandler)
            break
        except socket.error as err:
            print("IDLE Subprocess: socket error: " + err.args[1] +
                  ", retrying....", file=sys.__stderr__)
            socket_error = err
    else:
        print("IDLE Subprocess: Connection to "
              "IDLE GUI failed, exiting.", file=sys.__stderr__)
        show_socket_error(socket_error, address)
        global exit_now
        exit_now = True
        return
    server.handle_request() # A single request only

def show_socket_error(err, address):
    import tkinter
    import tkinter.messagebox as tkMessageBox
    root = tkinter.Tk()
    root.withdraw()
    if err.args[0] == 61: # connection refused
        msg = "IDLE's subprocess can't connect to %s:%d.  This may be due "\
              "to your personal firewall configuration.  It is safe to "\
              "allow this internal connection because no data is visible on "\
              "external ports." % address
        tkMessageBox.showerror("IDLE Subprocess Error", msg, parent=root)
    else:
        tkMessageBox.showerror("IDLE Subprocess Error",
                               "Socket Error: %s" % err.args[1])
    root.destroy()

def print_exception():
    import linecache
    linecache.checkcache()
    flush_stdout()
    efile = sys.stderr
    typ, val, tb = excinfo = sys.exc_info()
    sys.last_type, sys.last_value, sys.last_traceback = excinfo
    tbe = traceback.extract_tb(tb)
    print('Traceback (most recent call last):', file=efile)
    exclude = ("run.py", "rpc.py", "threading.py", "queue.py",
               "RemoteDebugger.py", "bdb.py")
    cleanup_traceback(tbe, exclude)
    traceback.print_list(tbe, file=efile)
    lines = traceback.format_exception_only(typ, val)
    for line in lines:
        print(line, end='', file=efile)

def cleanup_traceback(tb, exclude):
    "Remove excluded traces from beginning/end of tb; get cached lines"
    orig_tb = tb[:]
    while tb:
        for rpcfile in exclude:
            if tb[0][0].count(rpcfile):
                break    # found an exclude, break for: and delete tb[0]
        else:
            break        # no excludes, have left RPC code, break while:
        del tb[0]
    while tb:
        for rpcfile in exclude:
            if tb[-1][0].count(rpcfile):
                break
        else:
            break
        del tb[-1]
    if len(tb) == 0:
        # exception was in IDLE internals, don't prune!
        tb[:] = orig_tb[:]
        print("** IDLE Internal Exception: ", file=sys.stderr)
    rpchandler = rpc.objecttable['exec'].rpchandler
    for i in range(len(tb)):
        fn, ln, nm, line = tb[i]
        if nm == '?':
            nm = "-toplevel-"
        if not line and fn.startswith("<pyshell#"):
            line = rpchandler.remotecall('linecache', 'getline',
                                              (fn, ln), {})
        tb[i] = fn, ln, nm, line

def flush_stdout():
    """XXX How to do this now?"""

def exit():
    """Exit subprocess, possibly after first clearing exit functions.

    If config-main.cfg/.def 'General' 'delete-exitfunc' is True, then any
    functions registered with atexit will be removed before exiting.
    (VPython support)

    """
    if no_exitfunc:
        import atexit
        atexit._clear()
    sys.exit(0)

class MyRPCServer(rpc.RPCServer):

    def handle_error(self, request, client_address):
        """Override RPCServer method for IDLE

        Interrupt the MainThread and exit server if link is dropped.

        """
        global quitting
        try:
            raise
        except SystemExit:
            raise
        except EOFError:
            global exit_now
            exit_now = True
            thread.interrupt_main()
        except:
            erf = sys.__stderr__
            print('\n' + '-'*40, file=erf)
            print('Unhandled server exception!', file=erf)
            print('Thread: %s' % threading.current_thread().name, file=erf)
            print('Client Address: ', client_address, file=erf)
            print('Request: ', repr(request), file=erf)
            traceback.print_exc(file=erf)
            print('\n*** Unrecoverable, server exiting!', file=erf)
            print('-'*40, file=erf)
            quitting = True
            thread.interrupt_main()

class _RPCFile(io.TextIOBase):
    """Wrapper class for the RPC proxy to typecheck arguments
    that may not support pickling. The base class is there only
    to support type tests; all implementations come from the remote
    object."""

    def __init__(self, rpc):
        super.__setattr__(self, 'rpc', rpc)

    def __getattribute__(self, name):
        # When accessing the 'rpc' attribute, or 'write', use ours
        if name in ('rpc', 'write', 'writelines'):
            return io.TextIOBase.__getattribute__(self, name)
        # Else only look into the remote object only
        return getattr(self.rpc, name)

    def __setattr__(self, name, value):
        return setattr(self.rpc, name, value)

    @staticmethod
    def _ensure_string(func):
        def f(self, s):
            if not isinstance(s, str):
                raise TypeError('must be str, not ' + type(s).__name__)
            return func(self, s)
        return f

class _RPCOutputFile(_RPCFile):
    @_RPCFile._ensure_string
    def write(self, s):
        if not isinstance(s, str):
            raise TypeError('must be str, not ' + type(s).__name__)
        return self.rpc.write(s)

class _RPCInputFile(_RPCFile):
    @_RPCFile._ensure_string
    def write(self, s):
        raise io.UnsupportedOperation("not writable")
    writelines = write

class MyHandler(rpc.RPCHandler):

    def handle(self):
        """Override base method"""
        executive = Executive(self)
        self.register("exec", executive)
        self.console = self.get_remote_proxy("stdin")
        sys.stdin = _RPCInputFile(self.console)
        sys.stdout = _RPCOutputFile(self.get_remote_proxy("stdout"))
        sys.stderr = _RPCOutputFile(self.get_remote_proxy("stderr"))
        sys.displayhook = rpc.displayhook
        # page help() text to shell.
        import pydoc # import must be done here to capture i/o binding
        pydoc.pager = pydoc.plainpager
        from idlelib import IOBinding
        sys.stdin.encoding = sys.stdout.encoding = \
                             sys.stderr.encoding = IOBinding.encoding
        self.interp = self.get_remote_proxy("interp")
        rpc.RPCHandler.getresponse(self, myseq=None, wait=0.05)

    def exithook(self):
        "override SocketIO method - wait for MainThread to shut us down"
        time.sleep(10)

    def EOFhook(self):
        "Override SocketIO method - terminate wait on callback and exit thread"
        global quitting
        quitting = True
        thread.interrupt_main()

    def decode_interrupthook(self):
        "interrupt awakened thread"
        global quitting
        quitting = True
        thread.interrupt_main()


class Executive(object):

    def __init__(self, rpchandler):
        self.rpchandler = rpchandler
        self.locals = __main__.__dict__
        self.calltip = CallTips.CallTips()
        self.autocomplete = AutoComplete.AutoComplete()

    def runcode(self, code):
        global interruptable
        try:
            self.usr_exc_info = None
            interruptable = True
            try:
                exec(code, self.locals)
            finally:
                interruptable = False
        except:
            self.usr_exc_info = sys.exc_info()
            if quitting:
                exit()
            # even print a user code SystemExit exception, continue
            print_exception()
            jit = self.rpchandler.console.getvar("<<toggle-jit-stack-viewer>>")
            if jit:
                self.rpchandler.interp.open_remote_stack_viewer()
        else:
            flush_stdout()

    def interrupt_the_server(self):
        if interruptable:
            thread.interrupt_main()

    def start_the_debugger(self, gui_adap_oid):
        return RemoteDebugger.start_debugger(self.rpchandler, gui_adap_oid)

    def stop_the_debugger(self, idb_adap_oid):
        "Unregister the Idb Adapter.  Link objects and Idb then subject to GC"
        self.rpchandler.unregister(idb_adap_oid)

    def get_the_calltip(self, name):
        return self.calltip.fetch_tip(name)

    def get_the_completion_list(self, what, mode):
        return self.autocomplete.fetch_completions(what, mode)

    def stackviewer(self, flist_oid=None):
        if self.usr_exc_info:
            typ, val, tb = self.usr_exc_info
        else:
            return None
        flist = None
        if flist_oid is not None:
            flist = self.rpchandler.get_remote_proxy(flist_oid)
        while tb and tb.tb_frame.f_globals["__name__"] in ["rpc", "run"]:
            tb = tb.tb_next
        sys.last_type = typ
        sys.last_value = val
        item = StackViewer.StackTreeItem(flist, tb)
        return RemoteObjectBrowser.remote_object_tree_item(item)
