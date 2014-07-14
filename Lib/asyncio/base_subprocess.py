import collections
import subprocess

from . import protocols
from . import transports
from .coroutines import coroutine
from .log import logger


class BaseSubprocessTransport(transports.SubprocessTransport):

    def __init__(self, loop, protocol, args, shell,
                 stdin, stdout, stderr, bufsize,
                 extra=None, **kwargs):
        super().__init__(extra)
        self._protocol = protocol
        self._loop = loop
        self._pid = None

        self._pipes = {}
        if stdin == subprocess.PIPE:
            self._pipes[0] = None
        if stdout == subprocess.PIPE:
            self._pipes[1] = None
        if stderr == subprocess.PIPE:
            self._pipes[2] = None
        self._pending_calls = collections.deque()
        self._finished = False
        self._returncode = None
        self._start(args=args, shell=shell, stdin=stdin, stdout=stdout,
                    stderr=stderr, bufsize=bufsize, **kwargs)
        self._pid = self._proc.pid
        self._extra['subprocess'] = self._proc
        if self._loop.get_debug():
            if isinstance(args, (bytes, str)):
                program = args
            else:
                program = args[0]
            logger.debug('process %r created: pid %s',
                         program, self._pid)

    def __repr__(self):
        info = [self.__class__.__name__, 'pid=%s' % self._pid]
        if self._returncode is not None:
            info.append('returncode=%s' % self._returncode)

        stdin = self._pipes.get(0)
        if stdin is not None:
            info.append('stdin=%s' % stdin.pipe)

        stdout = self._pipes.get(1)
        stderr = self._pipes.get(2)
        if stdout is not None and stderr is stdout:
            info.append('stdout=stderr=%s' % stdout.pipe)
        else:
            if stdout is not None:
                info.append('stdout=%s' % stdout.pipe)
            if stderr is not None:
                info.append('stderr=%s' % stderr.pipe)

        return '<%s>' % ' '.join(info)

    def _start(self, args, shell, stdin, stdout, stderr, bufsize, **kwargs):
        raise NotImplementedError

    def _make_write_subprocess_pipe_proto(self, fd):
        raise NotImplementedError

    def _make_read_subprocess_pipe_proto(self, fd):
        raise NotImplementedError

    def close(self):
        for proto in self._pipes.values():
            proto.pipe.close()
        if self._returncode is None:
            self.terminate()

    def get_pid(self):
        return self._pid

    def get_returncode(self):
        return self._returncode

    def get_pipe_transport(self, fd):
        if fd in self._pipes:
            return self._pipes[fd].pipe
        else:
            return None

    def send_signal(self, signal):
        self._proc.send_signal(signal)

    def terminate(self):
        self._proc.terminate()

    def kill(self):
        self._proc.kill()

    @coroutine
    def _post_init(self):
        proc = self._proc
        loop = self._loop
        if proc.stdin is not None:
            _, pipe = yield from loop.connect_write_pipe(
                lambda: WriteSubprocessPipeProto(self, 0),
                proc.stdin)
            self._pipes[0] = pipe
        if proc.stdout is not None:
            _, pipe = yield from loop.connect_read_pipe(
                lambda: ReadSubprocessPipeProto(self, 1),
                proc.stdout)
            self._pipes[1] = pipe
        if proc.stderr is not None:
            _, pipe = yield from loop.connect_read_pipe(
                lambda: ReadSubprocessPipeProto(self, 2),
                proc.stderr)
            self._pipes[2] = pipe

        assert self._pending_calls is not None

        self._loop.call_soon(self._protocol.connection_made, self)
        for callback, data in self._pending_calls:
            self._loop.call_soon(callback, *data)
        self._pending_calls = None

    def _call(self, cb, *data):
        if self._pending_calls is not None:
            self._pending_calls.append((cb, data))
        else:
            self._loop.call_soon(cb, *data)

    def _pipe_connection_lost(self, fd, exc):
        self._call(self._protocol.pipe_connection_lost, fd, exc)
        self._try_finish()

    def _pipe_data_received(self, fd, data):
        self._call(self._protocol.pipe_data_received, fd, data)

    def _process_exited(self, returncode):
        assert returncode is not None, returncode
        assert self._returncode is None, self._returncode
        if self._loop.get_debug():
            logger.info('%r exited with return code %r',
                        self, returncode)
        self._returncode = returncode
        self._call(self._protocol.process_exited)
        self._try_finish()

    def _try_finish(self):
        assert not self._finished
        if self._returncode is None:
            return
        if all(p is not None and p.disconnected
               for p in self._pipes.values()):
            self._finished = True
            self._loop.call_soon(self._call_connection_lost, None)

    def _call_connection_lost(self, exc):
        try:
            self._protocol.connection_lost(exc)
        finally:
            self._proc = None
            self._protocol = None
            self._loop = None


class WriteSubprocessPipeProto(protocols.BaseProtocol):

    def __init__(self, proc, fd):
        self.proc = proc
        self.fd = fd
        self.pipe = None
        self.disconnected = False

    def connection_made(self, transport):
        self.pipe = transport

    def __repr__(self):
        return ('<%s fd=%s pipe=%r>'
                % (self.__class__.__name__, self.fd, self.pipe))

    def connection_lost(self, exc):
        self.disconnected = True
        self.proc._pipe_connection_lost(self.fd, exc)

    def pause_writing(self):
        self.proc._protocol.pause_writing()

    def resume_writing(self):
        self.proc._protocol.resume_writing()


class ReadSubprocessPipeProto(WriteSubprocessPipeProto,
                              protocols.Protocol):

    def data_received(self, data):
        self.proc._pipe_data_received(self.fd, data)
