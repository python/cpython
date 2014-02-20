__all__ = ['create_subprocess_exec', 'create_subprocess_shell']

import collections
import subprocess

from . import events
from . import futures
from . import protocols
from . import streams
from . import tasks


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
        self.waiter = futures.Future(loop=loop)
        self._waiters = collections.deque()
        self._transport = None

    def connection_made(self, transport):
        self._transport = transport
        if transport.get_pipe_transport(1):
            self.stdout = streams.StreamReader(limit=self._limit,
                                               loop=self._loop)
        if transport.get_pipe_transport(2):
            self.stderr = streams.StreamReader(limit=self._limit,
                                               loop=self._loop)
        stdin = transport.get_pipe_transport(0)
        if stdin is not None:
            self.stdin = streams.StreamWriter(stdin,
                                              protocol=self,
                                              reader=None,
                                              loop=self._loop)
        self.waiter.set_result(None)

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
        # wake up futures waiting for wait()
        returncode = self._transport.get_returncode()
        while self._waiters:
            waiter = self._waiters.popleft()
            waiter.set_result(returncode)


class Process:
    def __init__(self, transport, protocol, loop):
        self._transport = transport
        self._protocol = protocol
        self._loop = loop
        self.stdin = protocol.stdin
        self.stdout = protocol.stdout
        self.stderr = protocol.stderr
        self.pid = transport.get_pid()

    @property
    def returncode(self):
        return self._transport.get_returncode()

    @tasks.coroutine
    def wait(self):
        """Wait until the process exit and return the process return code."""
        returncode = self._transport.get_returncode()
        if returncode is not None:
            return returncode

        waiter = futures.Future(loop=self._loop)
        self._protocol._waiters.append(waiter)
        yield from waiter
        return waiter.result()

    def _check_alive(self):
        if self._transport.get_returncode() is not None:
            raise ProcessLookupError()

    def send_signal(self, signal):
        self._check_alive()
        self._transport.send_signal(signal)

    def terminate(self):
        self._check_alive()
        self._transport.terminate()

    def kill(self):
        self._check_alive()
        self._transport.kill()

    @tasks.coroutine
    def _feed_stdin(self, input):
        self.stdin.write(input)
        yield from self.stdin.drain()
        self.stdin.close()

    @tasks.coroutine
    def _noop(self):
        return None

    @tasks.coroutine
    def _read_stream(self, fd):
        transport = self._transport.get_pipe_transport(fd)
        if fd == 2:
            stream = self.stderr
        else:
            assert fd == 1
            stream = self.stdout
        output = yield from stream.read()
        transport.close()
        return output

    @tasks.coroutine
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


@tasks.coroutine
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
    yield from protocol.waiter
    return Process(transport, protocol, loop)

@tasks.coroutine
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
    yield from protocol.waiter
    return Process(transport, protocol, loop)
