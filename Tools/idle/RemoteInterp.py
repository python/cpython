import select
import socket
import struct
import sys
import types

VERBOSE = None

class SocketProtocol:
    """A simple protocol for sending strings across a socket"""
    BUF_SIZE = 8192
    
    def __init__(self, sock):
        self.sock = sock
        self._buffer = ''
        self._closed = 0

    def close(self):
        self._closed = 1
        self.sock.close()

    def send(self, buf):
        """Encode buf and write it on the socket"""
        if VERBOSE:
            VERBOSE.write('send %d:%s\n' % (len(buf), `buf`))
        self.sock.send('%d:%s' % (len(buf), buf))

    def receive(self, timeout=0):
        """Get next complete string from socket or return None

        Raise EOFError on EOF
        """
        buf = self._read_from_buffer()
        if buf is not None:
            return buf
        recvbuf = self._read_from_socket(timeout)
        if recvbuf is None:
            return None
        if recvbuf == '' and self._buffer == '':
            raise EOFError
        if VERBOSE:
            VERBOSE.write('recv %s\n' % `recvbuf`)
        self._buffer = self._buffer + recvbuf
        r = self._read_from_buffer()
        return r

    def _read_from_socket(self, timeout):
        """Does not block"""
        if self._closed:
            return ''
        if timeout is not None:
            r, w, x = select.select([self.sock], [], [], timeout)
        if timeout is None or r:
            return self.sock.recv(self.BUF_SIZE)
        else:
            return None

    def _read_from_buffer(self):
        buf = self._buffer
        i = buf.find(':')
        if i == -1:
            return None
        buflen = int(buf[:i])
        enclen = i + 1 + buflen
        if len(buf) >= enclen:
            s = buf[i+1:enclen]
            self._buffer = buf[enclen:]
            return s
        else:
            self._buffer = buf
        return None

# helpers for registerHandler method below

def get_methods(obj):
    methods = []
    for name in dir(obj):
        attr = getattr(obj, name)
        if callable(attr):
            methods.append(name)
    if type(obj) == types.InstanceType:
        methods = methods + get_methods(obj.__class__)
    if type(obj) == types.ClassType:
        for super in obj.__bases__:
            methods = methods + get_methods(super)
    return methods

class CommandProtocol:
    def __init__(self, sockp):
        self.sockp = sockp
        self.seqno = 0
        self.handlers = {}

    def close(self):
        self.sockp.close()
        self.handlers.clear()

    def registerHandler(self, handler):
        """A Handler is an object with handle_XXX methods"""
        for methname in get_methods(handler):
            if methname[:7] == "handle_":
                name = methname[7:]
                self.handlers[name] = getattr(handler, methname)

    def send(self, cmd, arg='', seqno=None):
        if arg:
            msg = "%s %s" % (cmd, arg)
        else:
            msg = cmd
        if seqno is None:
            seqno = self.get_seqno()
        msgbuf = self.encode_seqno(seqno) + msg
        self.sockp.send(msgbuf)
        if cmd == "reply":
            return
        reply = self.sockp.receive(timeout=None)
        r_cmd, r_arg, r_seqno = self._decode_msg(reply)
        assert r_seqno == seqno and r_cmd == "reply", "bad reply"
        return r_arg

    def _decode_msg(self, msg):
        seqno = self.decode_seqno(msg[:self.SEQNO_ENC_LEN])
        msg = msg[self.SEQNO_ENC_LEN:]
        parts = msg.split(" ", 2)
        if len(parts) == 1:
            cmd = msg
            arg = ''
        else:
            cmd = parts[0]
            arg = parts[1]
        return cmd, arg, seqno

    def dispatch(self):
        msg = self.sockp.receive()
        if msg is None:
            return
        cmd, arg, seqno = self._decode_msg(msg)
        self._current_reply = seqno
        h = self.handlers.get(cmd, self.default_handler)
        try:
            r = h(arg)
        except TypeError, msg:
            raise TypeError, "handle_%s: %s" % (cmd, msg)
        if self._current_reply is None:
            if r is not None:
                sys.stderr.write("ignoring %s return value type %s\n" % \
                                 (cmd, type(r).__name__))
            return
        if r is None:
            r = ''
        if type(r) != types.StringType:
            raise ValueError, "invalid return type for %s" % cmd
        self.send("reply", r, seqno=seqno)

    def reply(self, arg=''):
        """Send a reply immediately

        otherwise reply will be sent when handler returns
        """
        self.send("reply", arg, self._current_reply)
        self._current_reply = None

    def default_handler(self, arg):
        sys.stderr.write("WARNING: unhandled message %s\n" % arg)
        return ''

    SEQNO_ENC_LEN = 4

    def get_seqno(self):
        seqno = self.seqno
        self.seqno = seqno + 1
        return seqno

    def encode_seqno(self, seqno):
        return struct.pack("I", seqno)

    def decode_seqno(self, buf):
        return struct.unpack("I", buf)[0]
                

class StdioRedirector:
    """Redirect sys.std{in,out,err} to a set of file-like objects"""
    
    def __init__(self, stdin, stdout, stderr):
        self.stdin = stdin
        self.stdout = stdout
        self.stderr = stderr

    def redirect(self):
        self.save()
        sys.stdin = self.stdin
        sys.stdout = self.stdout
        sys.stderr = self.stderr

    def save(self):
        self._stdin = sys.stdin
        self._stdout = sys.stdout
        self._stderr = sys.stderr

    def restore(self):
        sys.stdin = self._stdin
        sys.stdout = self._stdout
        sys.stderr = self._stderr

class IOWrapper:
    """Send output from a file-like object across a SocketProtocol

    XXX Should this be more tightly integrated with the CommandProtocol?
    """

    def __init__(self, name, cmdp):
        self.name = name
        self.cmdp = cmdp
        self.buffer = []

class InputWrapper(IOWrapper):
    def write(self, buf):
        # XXX what should this do on Windows?
        raise IOError, (9, '[Errno 9] Bad file descriptor')

    def read(self, arg=None):
        if arg is not None:
            if arg <= 0:
                return ''
        else:
            arg = 0
        return self.cmdp.send(self.name, "read,%s" % arg)

    def readline(self):
        return self.cmdp.send(self.name, "readline")

class OutputWrapper(IOWrapper):
    def write(self, buf):
        self.cmdp.send(self.name, buf)

    def read(self, arg=None):
        return ''

class RemoteInterp:
    def __init__(self, sock):
        self._sock = SocketProtocol(sock)
        self._cmd = CommandProtocol(self._sock)
        self._cmd.registerHandler(self)

    def run(self):
        try:
            while 1:
                self._cmd.dispatch()
        except EOFError:
            pass

    def handle_execfile(self, arg):
        self._cmd.reply()
        io = StdioRedirector(InputWrapper("stdin", self._cmd),
                             OutputWrapper("stdout", self._cmd),
                             OutputWrapper("stderr", self._cmd))
        io.redirect()
        execfile(arg, {'__name__':'__main__'})
        io.restore()
        self._cmd.send("terminated")

    def handle_quit(self, arg):
        self._cmd.reply()
        self._cmd.close()

def startRemoteInterp(id):
    import os
    # UNIX domain sockets are simpler for starters
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.bind("/var/tmp/ri.%s" % id)
    try:
        sock.listen(1)
        cli, addr = sock.accept()
        rinterp = RemoteInterp(cli)
        rinterp.run()
    finally:
        os.unlink("/var/tmp/ri.%s" % id)

class RIClient:
    """Client of the remote interpreter"""
    def __init__(self, sock):
        self._sock = SocketProtocol(sock)
        self._cmd = CommandProtocol(self._sock)
        self._cmd.registerHandler(self)

    def execfile(self, file):
        self._cmd.send("execfile", file)

    def run(self):
        try:
            while 1:
                self._cmd.dispatch()
        except EOFError:
            pass
        
    def handle_stdout(self, buf):
        sys.stdout.write(buf)
##        sys.stdout.flush()

    def handle_stderr(self, buf):
        sys.stderr.write(buf)

    def handle_stdin(self, arg):
        if arg == "readline":
            return sys.stdin.readline()
        i = arg.find(",") + 1
        bytes = int(arg[i:])
        if bytes == 0:
            return sys.stdin.read()
        else:
            return sys.stdin.read(bytes)

    def handle_terminated(self, arg):
        self._cmd.reply()
        self._cmd.send("quit")
        self._cmd.close()

def riExec(id, file):
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.connect("/var/tmp/ri.%s" % id)
    cli = RIClient(sock)
    cli.execfile(file)
    cli.run()

if __name__ == "__main__":
    import sys
    import getopt

    SERVER = 1
    opts, args = getopt.getopt(sys.argv[1:], 'cv')
    for o, v in opts:
        if o == '-c':
            SERVER = 0
        elif o == '-v':
            VERBOSE = sys.stderr
    id = args[0]

    if SERVER:
        startRemoteInterp(id)
    else:
        file = args[1]
        riExec(id, file)        
    
