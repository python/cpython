"""Tests for asyncio/controller.py."""

import socket
import unittest

from asyncio import (
    CancelledError, StreamReader, StreamReaderProtocol, get_event_loop)
from asyncio.controller import Controller


class NoopServer(StreamReaderProtocol):
    def __init__(self, loop=None):
        self.loop = loop if loop else get_event_loop()
        super().__init__(
            StreamReader(loop=self.loop),
            client_connected_cb=self._client_connected_cb,
            loop=self.loop)
        self.transport = None

    def _client_connected_cb(self, reader, writer):
        # This is redundant since we subclass StreamReaderProtocol, but I like
        # the shorter names.
        self._reader = reader
        self._writer = writer

    def connection_made(self, transport):
        super().connection_made(transport)
        self.transport = transport
        self._handler_coroutine = self.loop.create_task(self._handle_client())

    def connection_lost(self, error):
        super().connection_lost(error)
        self.transport = None

    def eof_received(self):
        self._handler_coroutine.cancel()
        if self.transport is not None:                        # pragma: nopy34
            return super().eof_received()

    async def push(self, status):
        if isinstance(status, bytes):
            response = status + b'\r\n'
        else:
            response = bytes(status + '\r\n', 'ascii')
        self._writer.write(response)
        await self._writer.drain()

    async def _handle_client(self):
        while self.transport is not None:
            try:
                line = await self._reader.readline()
            except (ConnectionResetError, CancelledError) as error:
                self.connection_lost(error)
                return
            line = line.rstrip(b'\r\n')
            if not line:
                await self.push('500 Syntax')
                continue
            command = line.upper().decode(encoding='ascii')
            if command == 'NOOP':
                await self.push('250 OK')
            else:
                await self.push('500 Bad command: {}'.format(command))


class EchoController(Controller):
    def factory(self):
        return NoopServer()


class TestController(unittest.TestCase):
    def setUp(self):
        self.controller = EchoController('localhost', 9999)
        self.connection = None

    def connect(self):
        self.connection = socket.socket()
        self.connection.connect(
            (self.controller.hostname, self.controller.port))

    def say(self, command):
        self.connection.send(command + b'\r\n')
        response = self.connection.recv(4096)
        return response.strip(b'\r\n')

    def test_started(self):
        self.controller.start()
        self.addCleanup(self.controller.stop)
        self.connect()
        response = self.say(b'NOOP')
        self.assertEqual(response, b'250 OK')

    def test_keep_going(self):
        self.controller.start()
        self.addCleanup(self.controller.stop)
        self.connect()
        response = self.say(b'OOPS')
        self.assertEqual(response, b'500 Bad command: OOPS')
        response = self.say(b'NOOP')
        self.assertEqual(response, b'250 OK')

    def test_not_started(self):
        self.assertRaises(ConnectionRefusedError, self.connect)

    def test_stop(self):
        self.controller.start()
        # It's okay to try to stop the controller more than once.
        self.addCleanup(self.controller.stop)
        self.connect()
        response = self.say(b'NOOP')
        self.assertEqual(response, b'250 OK')
        self.controller.stop()
        self.assertRaises(ConnectionRefusedError, self.connect)
