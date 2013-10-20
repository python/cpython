import os
import sys
import unittest

if sys.platform != 'win32':
    raise unittest.SkipTest('Windows only')

import asyncio

from asyncio import windows_events
from asyncio import protocols
from asyncio import streams
from asyncio import transports
from asyncio import test_utils


class UpperProto(protocols.Protocol):
    def __init__(self):
        self.buf = []

    def connection_made(self, trans):
        self.trans = trans

    def data_received(self, data):
        self.buf.append(data)
        if b'\n' in data:
            self.trans.write(b''.join(self.buf).upper())
            self.trans.close()


class ProactorTests(unittest.TestCase):

    def setUp(self):
        self.loop = windows_events.ProactorEventLoop()
        asyncio.set_event_loop(None)

    def tearDown(self):
        self.loop.close()
        self.loop = None

    def test_close(self):
        a, b = self.loop._socketpair()
        trans = self.loop._make_socket_transport(a, protocols.Protocol())
        f = asyncio.async(self.loop.sock_recv(b, 100))
        trans.close()
        self.loop.run_until_complete(f)
        self.assertEqual(f.result(), b'')

    def test_double_bind(self):
        ADDRESS = r'\\.\pipe\test_double_bind-%s' % os.getpid()
        server1 = windows_events.PipeServer(ADDRESS)
        with self.assertRaises(PermissionError):
            server2 = windows_events.PipeServer(ADDRESS)
        server1.close()

    def test_pipe(self):
        res = self.loop.run_until_complete(self._test_pipe())
        self.assertEqual(res, 'done')

    def _test_pipe(self):
        ADDRESS = r'\\.\pipe\_test_pipe-%s' % os.getpid()

        with self.assertRaises(FileNotFoundError):
            yield from self.loop.create_pipe_connection(
                protocols.Protocol, ADDRESS)

        [server] = yield from self.loop.start_serving_pipe(
            UpperProto, ADDRESS)
        self.assertIsInstance(server, windows_events.PipeServer)

        clients = []
        for i in range(5):
            stream_reader = streams.StreamReader(loop=self.loop)
            protocol = streams.StreamReaderProtocol(stream_reader)
            trans, proto = yield from self.loop.create_pipe_connection(
                lambda:protocol, ADDRESS)
            self.assertIsInstance(trans, transports.Transport)
            self.assertEqual(protocol, proto)
            clients.append((stream_reader, trans))

        for i, (r, w) in enumerate(clients):
            w.write('lower-{}\n'.format(i).encode())

        for i, (r, w) in enumerate(clients):
            response = yield from r.readline()
            self.assertEqual(response, 'LOWER-{}\n'.format(i).encode())
            w.close()

        server.close()

        with self.assertRaises(FileNotFoundError):
            yield from self.loop.create_pipe_connection(
                protocols.Protocol, ADDRESS)

        return 'done'


if __name__ == '__main__':
    unittest.main()
