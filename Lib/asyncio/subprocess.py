__all__ = ['create_subprocess_exec', 'create_subprocess_shell']

import subprocess

from . import events
from . import protocols
from . import streams
from . import tasks
from .coroutines import coroutine
from .log import logger


PIPE = subprocess.PIPE
STDOUT = subprocess.STDOUT
DEVNULL = subprocess.DEVNULL


class SubprocessStreamProtocol(streams.FlowControlMixin,
                               protocols.SubprocessProtocol):
    """Like StreamReaderProtocol, but for a subprocess."""

    def __init__(self, limit, loop):
        super().__init__(loop=loop)
        self._limit = limit
        self.stdin = self.stdout = self.stderr = None
        self._transport = None

    def __repr__(self):
        info = [self.__class__.__name__]
        if self.stdin is not None:
            info.append('stdin=%r' % self.stdin)
        if self.stdout is not None:
            info.append('stdout=%r' % self.stdout)
        if self.stderr is not None:
            info.append('stderr=%r' % self.stderr)
        return '<%s>' % ' '.join(info)

    def connection_made(self, transport):
        self._transport = transport

        stdout_transport = transport.get_pipe_transport(1)
        if stdout_transport is not None:
            self.stdout = streams.StreamReader(limit=self._limit,
                                               loop=self._loop)
            self.stdout.set_transport(stdout_transport)

        stderr_transport = transport.get_pipe_transport(2)
        if stderr_transport is not None:
            self.stderr = streams.StreamReader(limit=self._limit,
                                               loop=self._loop)
            self.stderr.set_transport(stderr_transport)

        stdin_transport = transport.get_pipe_transport(0)
        if stdin_transport is not None:
            self.stdin = streams.StreamWriter(stdin_transport,
                                              protocol=self,
                                              reader=None,
                                              loop=self._loop)

    def pipe_data_received(self, fd, data):
        if fd == 1:
            reader = self.stdout
        elif fd == 2:
            reader = self.stderr
        else:
            reader = None
        if reader is not None:
            reader.feed_data(data)

    def pipe_connection_lost(self, fd, exc):
        if fd == 0:
            pipe = self.stdin
            if pipe is not None:
                pipe.close()
            self.connection_lost(exc)
            return
        if fd == 1:
            reader = self.stdout
        elif fd == 2:
            reader = self.stderr
        else:
            reader = None
        if reader != None:
            if exc is None:
                reader.feed_eof()
            else:
                reader.set_exception(exc)

    def process_exited(self):
        self._transport.close()
        self._transport = None


class Process:
    def __init__(self, transport, protocol, loop):
        self._transport = transport
        self._protocol = protocol
        self._loop = loop
        self.stdin = protocol.stdin
        self.stdout = protocol.stdout
        self.stderr = protocol.stderr
        self.pid = transport.get_pid()

    def __repr__(self):
        return '<%s %s>' % (self.__class__.__name__, self.pid)

    @property
    def returncode(self):
        return self._transport.get_returncode()

    @coroutine
    def wait(self):
        """Wait until the process exit and return the process return code.

        This method is a coroutine."""
        return (yield from self._transport._wait())

    def send_signal(self, signal):
        self._transport.send_signal(signal)

    def terminate(self):
        self._transport.terminate()

    def kill(self):
        self._transport.kill()

    @coroutine
    def _feed_stdin(self, input):
        debug = self._loop.get_debug()
        self.stdin.write(input)
        if debug:
            logger.debug('%r communicate: feed stdin (%s bytes)',
                        self, len(input))
        try:
            yield from self.stdin.drain()
        except (BrokenPipeError, ConnectionResetError) as exc:
            # communicate() ignores BrokenPipeError and ConnectionResetError
            if debug:
                logger.debug('%r communicate: stdin got %r', self, exc)

        if debug:
            logger.debug('%r communicate: close stdin', self)
        self.stdin.close()

    @coroutine
    def _noop(self):
        return None

    @coroutine
    def _read_stream(self, fd):
        transport = self._transport.get_pipe_transport(fd)
        if fd == 2:
            stream = self.stderr
        else:
            assert fd == 1
            stream = self.stdout
        if self._loop.get_debug():
            name = 'stdout' if fd == 1 else 'stderr'
            logger.debug('%r communicate: read %s', self, name)
        output = yield from stream.read()
        if self._loop.get_debug():
            name = 'stdout' if fd == 1 else 'stderr'
            logger.debug('%r communicate: close %s', self, name)
        transport.close()
        return output

    @coroutine
    def communicate(self, input=None):
        if input:
            stdin = self._feed_stdin(input)
        else:
            stdin = self._noop()
        if self.stdout is not None:
            stdout = self._read_stream(1)
        else:
            stdout = self._noop()
        if self.stderr is not None:
            stderr = self._read_stream(2)
        else:
            stderr = self._noop()
        stdin, stdout, stderr = yield from tasks.gather(stdin, stdout, stderr,
                                                        loop=self._loop)
        yield from self.wait()
        return (stdout, stderr)


@coroutine
def create_subprocess_shell(cmd, stdin=None, stdout=None, stderr=None,
                            loop=None, limit=streams._DEFAULT_LIMIT, **kwds):
    if loop is None:
        loop = events.get_event_loop()
    protocol_factory = lambda: SubprocessStreamProtocol(limit=limit,
                                                        loop=loop)
    transport, protocol = yield from loop.subprocess_shell(
                                            protocol_factory,
                                            cmd, stdin=stdin, stdout=stdout,
                                            stderr=stderr, **kwds)
    return Process(transport, protocol, loop)

@coroutine
def create_subprocess_exec(program, *args, stdin=None, stdout=None,
                           stderr=None, loop=None,
                           limit=streams._DEFAULT_LIMIT, **kwds):
    if loop is None:
        loop = events.get_event_loop()
    protocol_factory = lambda: SubprocessStreamProtocol(limit=limit,
                                                        loop=loop)
    transport, protocol = yield from loop.subprocess_exec(
                                            protocol_factory,
                                            program, *args,
                                            stdin=stdin, stdout=stdout,
                                            stderr=stderr, **kwds)
    return Process(transport, protocol, loop)
