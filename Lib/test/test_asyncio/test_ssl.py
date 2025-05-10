# Contains code from https://github.com/MagicStack/uvloop/tree/v0.16.0
# SPDX-License-Identifier: PSF-2.0 AND (MIT OR Apache-2.0)
# SPDX-FileCopyrightText: Copyright (c) 2015-2021 MagicStack Inc.  http://magic.io

import asyncio
import contextlib
import gc
import logging
import select
import socket
import sys
import tempfile
import threading
import time
import unittest.mock
import weakref
import unittest

try:
    import ssl
except ImportError:
    ssl = None

from test import support
from test.test_asyncio import utils as test_utils


MACOS = (sys.platform == 'darwin')
BUF_MULTIPLIER = 1024 if not MACOS else 64


def tearDownModule():
    asyncio._set_event_loop_policy(None)


class MyBaseProto(asyncio.Protocol):
    connected = None
    done = None

    def __init__(self, loop=None):
        self.transport = None
        self.state = 'INITIAL'
        self.nbytes = 0
        if loop is not None:
            self.connected = asyncio.Future(loop=loop)
            self.done = asyncio.Future(loop=loop)

    def connection_made(self, transport):
        self.transport = transport
        assert self.state == 'INITIAL', self.state
        self.state = 'CONNECTED'
        if self.connected:
            self.connected.set_result(None)

    def data_received(self, data):
        assert self.state == 'CONNECTED', self.state
        self.nbytes += len(data)

    def eof_received(self):
        assert self.state == 'CONNECTED', self.state
        self.state = 'EOF'

    def connection_lost(self, exc):
        assert self.state in ('CONNECTED', 'EOF'), self.state
        self.state = 'CLOSED'
        if self.done:
            self.done.set_result(None)


class MessageOutFilter(logging.Filter):
    def __init__(self, msg):
        self.msg = msg

    def filter(self, record):
        if self.msg in record.msg:
            return False
        return True


@unittest.skipIf(ssl is None, 'No ssl module')
class TestSSL(test_utils.TestCase):

    PAYLOAD_SIZE = 1024 * 100
    TIMEOUT = support.LONG_TIMEOUT

    def setUp(self):
        super().setUp()
        self.loop = asyncio.new_event_loop()
        self.set_event_loop(self.loop)
        self.addCleanup(self.loop.close)

    def tearDown(self):
        # just in case if we have transport close callbacks
        if not self.loop.is_closed():
            test_utils.run_briefly(self.loop)

        self.doCleanups()
        support.gc_collect()
        super().tearDown()

    def tcp_server(self, server_prog, *,
                   family=socket.AF_INET,
                   addr=None,
                   timeout=support.SHORT_TIMEOUT,
                   backlog=1,
                   max_clients=10):

        if addr is None:
            if family == getattr(socket, "AF_UNIX", None):
                with tempfile.NamedTemporaryFile() as tmp:
                    addr = tmp.name
            else:
                addr = ('127.0.0.1', 0)

        sock = socket.socket(family, socket.SOCK_STREAM)

        if timeout is None:
            raise RuntimeError('timeout is required')
        if timeout <= 0:
            raise RuntimeError('only blocking sockets are supported')
        sock.settimeout(timeout)

        try:
            sock.bind(addr)
            sock.listen(backlog)
        except OSError as ex:
            sock.close()
            raise ex

        return TestThreadedServer(
            self, sock, server_prog, timeout, max_clients)

    def tcp_client(self, client_prog,
                   family=socket.AF_INET,
                   timeout=support.SHORT_TIMEOUT):

        sock = socket.socket(family, socket.SOCK_STREAM)

        if timeout is None:
            raise RuntimeError('timeout is required')
        if timeout <= 0:
            raise RuntimeError('only blocking sockets are supported')
        sock.settimeout(timeout)

        return TestThreadedClient(
            self, sock, client_prog, timeout)

    def unix_server(self, *args, **kwargs):
        return self.tcp_server(*args, family=socket.AF_UNIX, **kwargs)

    def unix_client(self, *args, **kwargs):
        return self.tcp_client(*args, family=socket.AF_UNIX, **kwargs)

    def _create_server_ssl_context(self, certfile, keyfile=None):
        sslcontext = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        sslcontext.options |= ssl.OP_NO_SSLv2
        sslcontext.load_cert_chain(certfile, keyfile)
        return sslcontext

    def _create_client_ssl_context(self, *, disable_verify=True):
        sslcontext = ssl.create_default_context()
        sslcontext.check_hostname = False
        if disable_verify:
            sslcontext.verify_mode = ssl.CERT_NONE
        return sslcontext

    @contextlib.contextmanager
    def _silence_eof_received_warning(self):
        # TODO This warning has to be fixed in asyncio.
        logger = logging.getLogger('asyncio')
        filter = MessageOutFilter('has no effect when using ssl')
        logger.addFilter(filter)
        try:
            yield
        finally:
            logger.removeFilter(filter)

    def _abort_socket_test(self, ex):
        try:
            self.loop.stop()
        finally:
            self.fail(ex)

    def new_loop(self):
        return asyncio.new_event_loop()

    def new_policy(self):
        return asyncio.DefaultEventLoopPolicy()

    async def wait_closed(self, obj):
        if not isinstance(obj, asyncio.StreamWriter):
            return
        try:
            await obj.wait_closed()
        except (BrokenPipeError, ConnectionError):
            pass

    @support.bigmemtest(size=25, memuse=90*2**20, dry_run=False)
    def test_create_server_ssl_1(self, size):
        CNT = 0           # number of clients that were successful
        TOTAL_CNT = size  # total number of clients that test will create
        TIMEOUT = support.LONG_TIMEOUT  # timeout for this test

        A_DATA = b'A' * 1024 * BUF_MULTIPLIER
        B_DATA = b'B' * 1024 * BUF_MULTIPLIER

        sslctx = self._create_server_ssl_context(
            test_utils.ONLYCERT, test_utils.ONLYKEY
        )
        client_sslctx = self._create_client_ssl_context()

        clients = []

        async def handle_client(reader, writer):
            nonlocal CNT

            data = await reader.readexactly(len(A_DATA))
            self.assertEqual(data, A_DATA)
            writer.write(b'OK')

            data = await reader.readexactly(len(B_DATA))
            self.assertEqual(data, B_DATA)
            writer.writelines([b'SP', bytearray(b'A'), memoryview(b'M')])

            await writer.drain()
            writer.close()

            CNT += 1

        async def test_client(addr):
            fut = asyncio.Future()

            def prog(sock):
                try:
                    sock.starttls(client_sslctx)
                    sock.connect(addr)
                    sock.send(A_DATA)

                    data = sock.recv_all(2)
                    self.assertEqual(data, b'OK')

                    sock.send(B_DATA)
                    data = sock.recv_all(4)
                    self.assertEqual(data, b'SPAM')

                    sock.close()

                except Exception as ex:
                    self.loop.call_soon_threadsafe(fut.set_exception, ex)
                else:
                    self.loop.call_soon_threadsafe(fut.set_result, None)

            client = self.tcp_client(prog)
            client.start()
            clients.append(client)

            await fut

        async def start_server():
            extras = {}
            extras = dict(ssl_handshake_timeout=support.SHORT_TIMEOUT)

            srv = await asyncio.start_server(
                handle_client,
                '127.0.0.1', 0,
                family=socket.AF_INET,
                ssl=sslctx,
                **extras)

            try:
                srv_socks = srv.sockets
                self.assertTrue(srv_socks)

                addr = srv_socks[0].getsockname()

                tasks = []
                for _ in range(TOTAL_CNT):
                    tasks.append(test_client(addr))

                await asyncio.wait_for(asyncio.gather(*tasks), TIMEOUT)

            finally:
                self.loop.call_soon(srv.close)
                await srv.wait_closed()

        with self._silence_eof_received_warning():
            self.loop.run_until_complete(start_server())

        self.assertEqual(CNT, TOTAL_CNT)

        for client in clients:
            client.stop()

    def test_create_connection_ssl_1(self):
        self.loop.set_exception_handler(None)

        CNT = 0
        TOTAL_CNT = 25

        A_DATA = b'A' * 1024 * BUF_MULTIPLIER
        B_DATA = b'B' * 1024 * BUF_MULTIPLIER

        sslctx = self._create_server_ssl_context(
            test_utils.ONLYCERT,
            test_utils.ONLYKEY
        )
        client_sslctx = self._create_client_ssl_context()

        def server(sock):
            sock.starttls(
                sslctx,
                server_side=True)

            data = sock.recv_all(len(A_DATA))
            self.assertEqual(data, A_DATA)
            sock.send(b'OK')

            data = sock.recv_all(len(B_DATA))
            self.assertEqual(data, B_DATA)
            sock.send(b'SPAM')

            sock.close()

        async def client(addr):
            extras = {}
            extras = dict(ssl_handshake_timeout=support.SHORT_TIMEOUT)

            reader, writer = await asyncio.open_connection(
                *addr,
                ssl=client_sslctx,
                server_hostname='',
                **extras)

            writer.write(A_DATA)
            self.assertEqual(await reader.readexactly(2), b'OK')

            writer.write(B_DATA)
            self.assertEqual(await reader.readexactly(4), b'SPAM')

            nonlocal CNT
            CNT += 1

            writer.close()
            await self.wait_closed(writer)

        async def client_sock(addr):
            sock = socket.socket()
            sock.connect(addr)
            reader, writer = await asyncio.open_connection(
                sock=sock,
                ssl=client_sslctx,
                server_hostname='')

            writer.write(A_DATA)
            self.assertEqual(await reader.readexactly(2), b'OK')

            writer.write(B_DATA)
            self.assertEqual(await reader.readexactly(4), b'SPAM')

            nonlocal CNT
            CNT += 1

            writer.close()
            await self.wait_closed(writer)
            sock.close()

        def run(coro):
            nonlocal CNT
            CNT = 0

            async def _gather(*tasks):
                # trampoline
                return await asyncio.gather(*tasks)

            with self.tcp_server(server,
                                 max_clients=TOTAL_CNT,
                                 backlog=TOTAL_CNT) as srv:
                tasks = []
                for _ in range(TOTAL_CNT):
                    tasks.append(coro(srv.addr))

                self.loop.run_until_complete(_gather(*tasks))

            self.assertEqual(CNT, TOTAL_CNT)

        with self._silence_eof_received_warning():
            run(client)

        with self._silence_eof_received_warning():
            run(client_sock)

    def test_create_connection_ssl_slow_handshake(self):
        client_sslctx = self._create_client_ssl_context()

        # silence error logger
        self.loop.set_exception_handler(lambda *args: None)

        def server(sock):
            try:
                sock.recv_all(1024 * 1024)
            except ConnectionAbortedError:
                pass
            finally:
                sock.close()

        async def client(addr):
            reader, writer = await asyncio.open_connection(
                *addr,
                ssl=client_sslctx,
                server_hostname='',
                ssl_handshake_timeout=1.0)
            writer.close()
            await self.wait_closed(writer)

        with self.tcp_server(server,
                             max_clients=1,
                             backlog=1) as srv:

            with self.assertRaisesRegex(
                    ConnectionAbortedError,
                    r'SSL handshake.*is taking longer'):

                self.loop.run_until_complete(client(srv.addr))

    def test_create_connection_ssl_failed_certificate(self):
        # silence error logger
        self.loop.set_exception_handler(lambda *args: None)

        sslctx = self._create_server_ssl_context(
            test_utils.ONLYCERT,
            test_utils.ONLYKEY
        )
        client_sslctx = self._create_client_ssl_context(disable_verify=False)

        def server(sock):
            try:
                sock.starttls(
                    sslctx,
                    server_side=True)
                sock.connect()
            except (ssl.SSLError, OSError):
                pass
            finally:
                sock.close()

        async def client(addr):
            reader, writer = await asyncio.open_connection(
                *addr,
                ssl=client_sslctx,
                server_hostname='',
                ssl_handshake_timeout=support.SHORT_TIMEOUT)
            writer.close()
            await self.wait_closed(writer)

        with self.tcp_server(server,
                             max_clients=1,
                             backlog=1) as srv:

            with self.assertRaises(ssl.SSLCertVerificationError):
                self.loop.run_until_complete(client(srv.addr))

    def test_ssl_handshake_timeout(self):
        # bpo-29970: Check that a connection is aborted if handshake is not
        # completed in timeout period, instead of remaining open indefinitely
        client_sslctx = test_utils.simple_client_sslcontext()

        # silence error logger
        messages = []
        self.loop.set_exception_handler(lambda loop, ctx: messages.append(ctx))

        server_side_aborted = False

        def server(sock):
            nonlocal server_side_aborted
            try:
                sock.recv_all(1024 * 1024)
            except ConnectionAbortedError:
                server_side_aborted = True
            finally:
                sock.close()

        async def client(addr):
            await asyncio.wait_for(
                self.loop.create_connection(
                    asyncio.Protocol,
                    *addr,
                    ssl=client_sslctx,
                    server_hostname='',
                    ssl_handshake_timeout=10.0),
                0.5)

        with self.tcp_server(server,
                             max_clients=1,
                             backlog=1) as srv:

            with self.assertRaises(asyncio.TimeoutError):
                self.loop.run_until_complete(client(srv.addr))

        self.assertTrue(server_side_aborted)

        # Python issue #23197: cancelling a handshake must not raise an
        # exception or log an error, even if the handshake failed
        self.assertEqual(messages, [])

    def test_ssl_handshake_connection_lost(self):
        # #246: make sure that no connection_lost() is called before
        # connection_made() is called first

        client_sslctx = test_utils.simple_client_sslcontext()

        # silence error logger
        self.loop.set_exception_handler(lambda loop, ctx: None)

        connection_made_called = False
        connection_lost_called = False

        def server(sock):
            sock.recv(1024)
            # break the connection during handshake
            sock.close()

        class ClientProto(asyncio.Protocol):
            def connection_made(self, transport):
                nonlocal connection_made_called
                connection_made_called = True

            def connection_lost(self, exc):
                nonlocal connection_lost_called
                connection_lost_called = True

        async def client(addr):
            await self.loop.create_connection(
                ClientProto,
                *addr,
                ssl=client_sslctx,
                server_hostname=''),

        with self.tcp_server(server,
                             max_clients=1,
                             backlog=1) as srv:

            with self.assertRaises(ConnectionResetError):
                self.loop.run_until_complete(client(srv.addr))

        if connection_lost_called:
            if connection_made_called:
                self.fail("unexpected call to connection_lost()")
            else:
                self.fail("unexpected call to connection_lost() without"
                          "calling connection_made()")
        elif connection_made_called:
            self.fail("unexpected call to connection_made()")

    def test_ssl_connect_accepted_socket(self):
        proto = ssl.PROTOCOL_TLS_SERVER
        server_context = ssl.SSLContext(proto)
        server_context.load_cert_chain(test_utils.ONLYCERT, test_utils.ONLYKEY)
        if hasattr(server_context, 'check_hostname'):
            server_context.check_hostname = False
        server_context.verify_mode = ssl.CERT_NONE

        client_context = ssl.SSLContext(proto)
        if hasattr(server_context, 'check_hostname'):
            client_context.check_hostname = False
        client_context.verify_mode = ssl.CERT_NONE

    def test_connect_accepted_socket(self, server_ssl=None, client_ssl=None):
        loop = self.loop

        class MyProto(MyBaseProto):

            def connection_lost(self, exc):
                super().connection_lost(exc)
                loop.call_soon(loop.stop)

            def data_received(self, data):
                super().data_received(data)
                self.transport.write(expected_response)

        lsock = socket.socket(socket.AF_INET)
        lsock.bind(('127.0.0.1', 0))
        lsock.listen(1)
        addr = lsock.getsockname()

        message = b'test data'
        response = None
        expected_response = b'roger'

        def client():
            nonlocal response
            try:
                csock = socket.socket(socket.AF_INET)
                if client_ssl is not None:
                    csock = client_ssl.wrap_socket(csock)
                csock.connect(addr)
                csock.sendall(message)
                response = csock.recv(99)
                csock.close()
            except Exception as exc:
                print(
                    "Failure in client thread in test_connect_accepted_socket",
                    exc)

        thread = threading.Thread(target=client, daemon=True)
        thread.start()

        conn, _ = lsock.accept()
        proto = MyProto(loop=loop)
        proto.loop = loop

        extras = {}
        if server_ssl:
            extras = dict(ssl_handshake_timeout=support.SHORT_TIMEOUT)

        f = loop.create_task(
            loop.connect_accepted_socket(
                (lambda: proto), conn, ssl=server_ssl,
                **extras))
        loop.run_forever()
        conn.close()
        lsock.close()

        thread.join(1)
        self.assertFalse(thread.is_alive())
        self.assertEqual(proto.state, 'CLOSED')
        self.assertEqual(proto.nbytes, len(message))
        self.assertEqual(response, expected_response)
        tr, _ = f.result()

        if server_ssl:
            self.assertIn('SSL', tr.__class__.__name__)

        tr.close()
        # let it close
        self.loop.run_until_complete(asyncio.sleep(0.1))

    def test_start_tls_client_corrupted_ssl(self):
        self.loop.set_exception_handler(lambda loop, ctx: None)

        sslctx = test_utils.simple_server_sslcontext()
        client_sslctx = test_utils.simple_client_sslcontext()

        def server(sock):
            orig_sock = sock.dup()
            try:
                sock.starttls(
                    sslctx,
                    server_side=True)
                sock.sendall(b'A\n')
                sock.recv_all(1)
                orig_sock.send(b'please corrupt the SSL connection')
            except ssl.SSLError:
                pass
            finally:
                sock.close()
                orig_sock.close()

        async def client(addr):
            reader, writer = await asyncio.open_connection(
                *addr,
                ssl=client_sslctx,
                server_hostname='')

            self.assertEqual(await reader.readline(), b'A\n')
            writer.write(b'B')
            with self.assertRaises(ssl.SSLError):
                await reader.readline()
            writer.close()
            try:
                await self.wait_closed(writer)
            except ssl.SSLError:
                pass
            return 'OK'

        with self.tcp_server(server,
                             max_clients=1,
                             backlog=1) as srv:

            res = self.loop.run_until_complete(client(srv.addr))

        self.assertEqual(res, 'OK')

    def test_start_tls_client_reg_proto_1(self):
        HELLO_MSG = b'1' * self.PAYLOAD_SIZE

        server_context = test_utils.simple_server_sslcontext()
        client_context = test_utils.simple_client_sslcontext()

        def serve(sock):
            sock.settimeout(self.TIMEOUT)

            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.starttls(server_context, server_side=True)

            sock.sendall(b'O')
            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.unwrap()
            sock.close()

        class ClientProto(asyncio.Protocol):
            def __init__(self, on_data, on_eof):
                self.on_data = on_data
                self.on_eof = on_eof
                self.con_made_cnt = 0

            def connection_made(proto, tr):
                proto.con_made_cnt += 1
                # Ensure connection_made gets called only once.
                self.assertEqual(proto.con_made_cnt, 1)

            def data_received(self, data):
                self.on_data.set_result(data)

            def eof_received(self):
                self.on_eof.set_result(True)

        async def client(addr):
            await asyncio.sleep(0.5)

            on_data = self.loop.create_future()
            on_eof = self.loop.create_future()

            tr, proto = await self.loop.create_connection(
                lambda: ClientProto(on_data, on_eof), *addr)

            tr.write(HELLO_MSG)
            new_tr = await self.loop.start_tls(tr, proto, client_context)

            self.assertEqual(await on_data, b'O')
            new_tr.write(HELLO_MSG)
            await on_eof

            new_tr.close()

        with self.tcp_server(serve, timeout=self.TIMEOUT) as srv:
            self.loop.run_until_complete(
                asyncio.wait_for(client(srv.addr),
                                 timeout=support.SHORT_TIMEOUT))

    def test_create_connection_memory_leak(self):
        HELLO_MSG = b'1' * self.PAYLOAD_SIZE

        server_context = self._create_server_ssl_context(
            test_utils.ONLYCERT, test_utils.ONLYKEY)
        client_context = self._create_client_ssl_context()

        def serve(sock):
            sock.settimeout(self.TIMEOUT)

            sock.starttls(server_context, server_side=True)

            sock.sendall(b'O')
            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.unwrap()
            sock.close()

        class ClientProto(asyncio.Protocol):
            def __init__(self, on_data, on_eof):
                self.on_data = on_data
                self.on_eof = on_eof
                self.con_made_cnt = 0

            def connection_made(proto, tr):
                # XXX: We assume user stores the transport in protocol
                proto.tr = tr
                proto.con_made_cnt += 1
                # Ensure connection_made gets called only once.
                self.assertEqual(proto.con_made_cnt, 1)

            def data_received(self, data):
                self.on_data.set_result(data)

            def eof_received(self):
                self.on_eof.set_result(True)

        async def client(addr):
            await asyncio.sleep(0.5)

            on_data = self.loop.create_future()
            on_eof = self.loop.create_future()

            tr, proto = await self.loop.create_connection(
                lambda: ClientProto(on_data, on_eof), *addr,
                ssl=client_context)

            self.assertEqual(await on_data, b'O')
            tr.write(HELLO_MSG)
            await on_eof

            tr.close()

        with self.tcp_server(serve, timeout=self.TIMEOUT) as srv:
            self.loop.run_until_complete(
                asyncio.wait_for(client(srv.addr),
                                 timeout=support.SHORT_TIMEOUT))

        # No garbage is left for SSL client from loop.create_connection, even
        # if user stores the SSLTransport in corresponding protocol instance
        client_context = weakref.ref(client_context)
        self.assertIsNone(client_context())

    def test_start_tls_client_buf_proto_1(self):
        HELLO_MSG = b'1' * self.PAYLOAD_SIZE

        server_context = test_utils.simple_server_sslcontext()
        client_context = test_utils.simple_client_sslcontext()

        client_con_made_calls = 0

        def serve(sock):
            sock.settimeout(self.TIMEOUT)

            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.starttls(server_context, server_side=True)

            sock.sendall(b'O')
            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.sendall(b'2')
            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.unwrap()
            sock.close()

        class ClientProtoFirst(asyncio.BufferedProtocol):
            def __init__(self, on_data):
                self.on_data = on_data
                self.buf = bytearray(1)

            def connection_made(self, tr):
                nonlocal client_con_made_calls
                client_con_made_calls += 1

            def get_buffer(self, sizehint):
                return self.buf

            def buffer_updated(self, nsize):
                assert nsize == 1
                self.on_data.set_result(bytes(self.buf[:nsize]))

            def eof_received(self):
                pass

        class ClientProtoSecond(asyncio.Protocol):
            def __init__(self, on_data, on_eof):
                self.on_data = on_data
                self.on_eof = on_eof
                self.con_made_cnt = 0

            def connection_made(self, tr):
                nonlocal client_con_made_calls
                client_con_made_calls += 1

            def data_received(self, data):
                self.on_data.set_result(data)

            def eof_received(self):
                self.on_eof.set_result(True)

        async def client(addr):
            await asyncio.sleep(0.5)

            on_data1 = self.loop.create_future()
            on_data2 = self.loop.create_future()
            on_eof = self.loop.create_future()

            tr, proto = await self.loop.create_connection(
                lambda: ClientProtoFirst(on_data1), *addr)

            tr.write(HELLO_MSG)
            new_tr = await self.loop.start_tls(tr, proto, client_context)

            self.assertEqual(await on_data1, b'O')
            new_tr.write(HELLO_MSG)

            new_tr.set_protocol(ClientProtoSecond(on_data2, on_eof))
            self.assertEqual(await on_data2, b'2')
            new_tr.write(HELLO_MSG)
            await on_eof

            new_tr.close()

            # connection_made() should be called only once -- when
            # we establish connection for the first time. Start TLS
            # doesn't call connection_made() on application protocols.
            self.assertEqual(client_con_made_calls, 1)

        with self.tcp_server(serve, timeout=self.TIMEOUT) as srv:
            self.loop.run_until_complete(
                asyncio.wait_for(client(srv.addr),
                                 timeout=self.TIMEOUT))

    def test_start_tls_slow_client_cancel(self):
        HELLO_MSG = b'1' * self.PAYLOAD_SIZE

        client_context = test_utils.simple_client_sslcontext()
        server_waits_on_handshake = self.loop.create_future()

        def serve(sock):
            sock.settimeout(self.TIMEOUT)

            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            try:
                self.loop.call_soon_threadsafe(
                    server_waits_on_handshake.set_result, None)
                data = sock.recv_all(1024 * 1024)
            except ConnectionAbortedError:
                pass
            finally:
                sock.close()

        class ClientProto(asyncio.Protocol):
            def __init__(self, on_data, on_eof):
                self.on_data = on_data
                self.on_eof = on_eof
                self.con_made_cnt = 0

            def connection_made(proto, tr):
                proto.con_made_cnt += 1
                # Ensure connection_made gets called only once.
                self.assertEqual(proto.con_made_cnt, 1)

            def data_received(self, data):
                self.on_data.set_result(data)

            def eof_received(self):
                self.on_eof.set_result(True)

        async def client(addr):
            await asyncio.sleep(0.5)

            on_data = self.loop.create_future()
            on_eof = self.loop.create_future()

            tr, proto = await self.loop.create_connection(
                lambda: ClientProto(on_data, on_eof), *addr)

            tr.write(HELLO_MSG)

            await server_waits_on_handshake

            with self.assertRaises(asyncio.TimeoutError):
                await asyncio.wait_for(
                    self.loop.start_tls(tr, proto, client_context),
                    0.5)

        with self.tcp_server(serve, timeout=self.TIMEOUT) as srv:
            self.loop.run_until_complete(
                asyncio.wait_for(client(srv.addr),
                                 timeout=support.SHORT_TIMEOUT))

    def test_start_tls_server_1(self):
        HELLO_MSG = b'1' * self.PAYLOAD_SIZE

        server_context = test_utils.simple_server_sslcontext()
        client_context = test_utils.simple_client_sslcontext()

        def client(sock, addr):
            sock.settimeout(self.TIMEOUT)

            sock.connect(addr)
            data = sock.recv_all(len(HELLO_MSG))
            self.assertEqual(len(data), len(HELLO_MSG))

            sock.starttls(client_context)
            sock.sendall(HELLO_MSG)

            sock.unwrap()
            sock.close()

        class ServerProto(asyncio.Protocol):
            def __init__(self, on_con, on_eof, on_con_lost):
                self.on_con = on_con
                self.on_eof = on_eof
                self.on_con_lost = on_con_lost
                self.data = b''

            def connection_made(self, tr):
                self.on_con.set_result(tr)

            def data_received(self, data):
                self.data += data

            def eof_received(self):
                self.on_eof.set_result(1)

            def connection_lost(self, exc):
                if exc is None:
                    self.on_con_lost.set_result(None)
                else:
                    self.on_con_lost.set_exception(exc)

        async def main(proto, on_con, on_eof, on_con_lost):
            tr = await on_con
            tr.write(HELLO_MSG)

            self.assertEqual(proto.data, b'')

            new_tr = await self.loop.start_tls(
                tr, proto, server_context,
                server_side=True,
                ssl_handshake_timeout=self.TIMEOUT)

            await on_eof
            await on_con_lost
            self.assertEqual(proto.data, HELLO_MSG)
            new_tr.close()

        async def run_main():
            on_con = self.loop.create_future()
            on_eof = self.loop.create_future()
            on_con_lost = self.loop.create_future()
            proto = ServerProto(on_con, on_eof, on_con_lost)

            server = await self.loop.create_server(
                lambda: proto, '127.0.0.1', 0)
            addr = server.sockets[0].getsockname()

            with self.tcp_client(lambda sock: client(sock, addr),
                                 timeout=self.TIMEOUT):
                await asyncio.wait_for(
                    main(proto, on_con, on_eof, on_con_lost),
                    timeout=self.TIMEOUT)

            server.close()
            await server.wait_closed()

        self.loop.run_until_complete(run_main())

    @support.bigmemtest(size=25, memuse=90*2**20, dry_run=False)
    def test_create_server_ssl_over_ssl(self, size):
        CNT = 0           # number of clients that were successful
        TOTAL_CNT = size  # total number of clients that test will create
        TIMEOUT = support.LONG_TIMEOUT  # timeout for this test

        A_DATA = b'A' * 1024 * BUF_MULTIPLIER
        B_DATA = b'B' * 1024 * BUF_MULTIPLIER

        sslctx_1 = self._create_server_ssl_context(
            test_utils.ONLYCERT, test_utils.ONLYKEY)
        client_sslctx_1 = self._create_client_ssl_context()
        sslctx_2 = self._create_server_ssl_context(
            test_utils.ONLYCERT, test_utils.ONLYKEY)
        client_sslctx_2 = self._create_client_ssl_context()

        clients = []

        async def handle_client(reader, writer):
            nonlocal CNT

            data = await reader.readexactly(len(A_DATA))
            self.assertEqual(data, A_DATA)
            writer.write(b'OK')

            data = await reader.readexactly(len(B_DATA))
            self.assertEqual(data, B_DATA)
            writer.writelines([b'SP', bytearray(b'A'), memoryview(b'M')])

            await writer.drain()
            writer.close()

            CNT += 1

        class ServerProtocol(asyncio.StreamReaderProtocol):
            def connection_made(self, transport):
                super_ = super()
                transport.pause_reading()
                fut = self._loop.create_task(self._loop.start_tls(
                    transport, self, sslctx_2, server_side=True))

                def cb(_):
                    try:
                        tr = fut.result()
                    except Exception as ex:
                        super_.connection_lost(ex)
                    else:
                        super_.connection_made(tr)
                fut.add_done_callback(cb)

        def server_protocol_factory():
            reader = asyncio.StreamReader()
            protocol = ServerProtocol(reader, handle_client)
            return protocol

        async def test_client(addr):
            fut = asyncio.Future()

            def prog(sock):
                try:
                    sock.connect(addr)
                    sock.starttls(client_sslctx_1)

                    # because wrap_socket() doesn't work correctly on
                    # SSLSocket, we have to do the 2nd level SSL manually
                    incoming = ssl.MemoryBIO()
                    outgoing = ssl.MemoryBIO()
                    sslobj = client_sslctx_2.wrap_bio(incoming, outgoing)

                    def do(func, *args):
                        while True:
                            try:
                                rv = func(*args)
                                break
                            except ssl.SSLWantReadError:
                                if outgoing.pending:
                                    sock.send(outgoing.read())
                                incoming.write(sock.recv(65536))
                        if outgoing.pending:
                            sock.send(outgoing.read())
                        return rv

                    do(sslobj.do_handshake)

                    do(sslobj.write, A_DATA)
                    data = do(sslobj.read, 2)
                    self.assertEqual(data, b'OK')

                    do(sslobj.write, B_DATA)
                    data = b''
                    while True:
                        chunk = do(sslobj.read, 4)
                        if not chunk:
                            break
                        data += chunk
                    self.assertEqual(data, b'SPAM')

                    do(sslobj.unwrap)
                    sock.close()

                except Exception as ex:
                    self.loop.call_soon_threadsafe(fut.set_exception, ex)
                    sock.close()
                else:
                    self.loop.call_soon_threadsafe(fut.set_result, None)

            client = self.tcp_client(prog)
            client.start()
            clients.append(client)

            await fut

        async def start_server():
            extras = {}

            srv = await self.loop.create_server(
                server_protocol_factory,
                '127.0.0.1', 0,
                family=socket.AF_INET,
                ssl=sslctx_1,
                **extras)

            try:
                srv_socks = srv.sockets
                self.assertTrue(srv_socks)

                addr = srv_socks[0].getsockname()

                tasks = []
                for _ in range(TOTAL_CNT):
                    tasks.append(test_client(addr))

                await asyncio.wait_for(asyncio.gather(*tasks), TIMEOUT)

            finally:
                self.loop.call_soon(srv.close)
                await srv.wait_closed()

        with self._silence_eof_received_warning():
            self.loop.run_until_complete(start_server())

        self.assertEqual(CNT, TOTAL_CNT)

        for client in clients:
            client.stop()

    def test_shutdown_cleanly(self):
        CNT = 0
        TOTAL_CNT = 25

        A_DATA = b'A' * 1024 * BUF_MULTIPLIER

        sslctx = self._create_server_ssl_context(
            test_utils.ONLYCERT, test_utils.ONLYKEY)
        client_sslctx = self._create_client_ssl_context()

        def server(sock):
            sock.starttls(
                sslctx,
                server_side=True)

            data = sock.recv_all(len(A_DATA))
            self.assertEqual(data, A_DATA)
            sock.send(b'OK')

            sock.unwrap()

            sock.close()

        async def client(addr):
            extras = {}
            extras = dict(ssl_handshake_timeout=support.SHORT_TIMEOUT)

            reader, writer = await asyncio.open_connection(
                *addr,
                ssl=client_sslctx,
                server_hostname='',
                **extras)

            writer.write(A_DATA)
            self.assertEqual(await reader.readexactly(2), b'OK')

            self.assertEqual(await reader.read(), b'')

            nonlocal CNT
            CNT += 1

            writer.close()
            await self.wait_closed(writer)

        def run(coro):
            nonlocal CNT
            CNT = 0

            async def _gather(*tasks):
                return await asyncio.gather(*tasks)

            with self.tcp_server(server,
                                 max_clients=TOTAL_CNT,
                                 backlog=TOTAL_CNT) as srv:
                tasks = []
                for _ in range(TOTAL_CNT):
                    tasks.append(coro(srv.addr))

                self.loop.run_until_complete(
                    _gather(*tasks))

            self.assertEqual(CNT, TOTAL_CNT)

        with self._silence_eof_received_warning():
            run(client)

    def test_flush_before_shutdown(self):
        CHUNK = 1024 * 128
        SIZE = 32

        sslctx = self._create_server_ssl_context(
            test_utils.ONLYCERT, test_utils.ONLYKEY)
        client_sslctx = self._create_client_ssl_context()

        future = None

        def server(sock):
            sock.starttls(sslctx, server_side=True)
            self.assertEqual(sock.recv_all(4), b'ping')
            sock.send(b'pong')
            time.sleep(0.5)  # hopefully stuck the TCP buffer
            data = sock.recv_all(CHUNK * SIZE)
            self.assertEqual(len(data), CHUNK * SIZE)
            sock.close()

        def run(meth):
            def wrapper(sock):
                try:
                    meth(sock)
                except Exception as ex:
                    self.loop.call_soon_threadsafe(future.set_exception, ex)
                else:
                    self.loop.call_soon_threadsafe(future.set_result, None)
            return wrapper

        async def client(addr):
            nonlocal future
            future = self.loop.create_future()
            reader, writer = await asyncio.open_connection(
                *addr,
                ssl=client_sslctx,
                server_hostname='')
            sslprotocol = writer.transport._ssl_protocol
            writer.write(b'ping')
            data = await reader.readexactly(4)
            self.assertEqual(data, b'pong')

            sslprotocol.pause_writing()
            for _ in range(SIZE):
                writer.write(b'x' * CHUNK)

            writer.close()
            sslprotocol.resume_writing()

            await self.wait_closed(writer)
            try:
                data = await reader.read()
                self.assertEqual(data, b'')
            except ConnectionResetError:
                pass
            await future

        with self.tcp_server(run(server)) as srv:
            self.loop.run_until_complete(client(srv.addr))

    def test_remote_shutdown_receives_trailing_data(self):
        CHUNK = 1024 * 128
        SIZE = 32

        sslctx = self._create_server_ssl_context(
            test_utils.ONLYCERT,
            test_utils.ONLYKEY
        )
        client_sslctx = self._create_client_ssl_context()
        future = None

        def server(sock):
            incoming = ssl.MemoryBIO()
            outgoing = ssl.MemoryBIO()
            sslobj = sslctx.wrap_bio(incoming, outgoing, server_side=True)

            while True:
                try:
                    sslobj.do_handshake()
                except ssl.SSLWantReadError:
                    if outgoing.pending:
                        sock.send(outgoing.read())
                    incoming.write(sock.recv(16384))
                else:
                    if outgoing.pending:
                        sock.send(outgoing.read())
                    break

            while True:
                try:
                    data = sslobj.read(4)
                except ssl.SSLWantReadError:
                    incoming.write(sock.recv(16384))
                else:
                    break

            self.assertEqual(data, b'ping')
            sslobj.write(b'pong')
            sock.send(outgoing.read())

            time.sleep(0.2)  # wait for the peer to fill its backlog

            # send close_notify but don't wait for response
            with self.assertRaises(ssl.SSLWantReadError):
                sslobj.unwrap()
            sock.send(outgoing.read())

            # should receive all data
            data_len = 0
            while True:
                try:
                    chunk = len(sslobj.read(16384))
                    data_len += chunk
                except ssl.SSLWantReadError:
                    incoming.write(sock.recv(16384))
                except ssl.SSLZeroReturnError:
                    break

            self.assertEqual(data_len, CHUNK * SIZE)

            # verify that close_notify is received
            sslobj.unwrap()

            sock.close()

        def eof_server(sock):
            sock.starttls(sslctx, server_side=True)
            self.assertEqual(sock.recv_all(4), b'ping')
            sock.send(b'pong')

            time.sleep(0.2)  # wait for the peer to fill its backlog

            # send EOF
            sock.shutdown(socket.SHUT_WR)

            # should receive all data
            data = sock.recv_all(CHUNK * SIZE)
            self.assertEqual(len(data), CHUNK * SIZE)

            sock.close()

        async def client(addr):
            nonlocal future
            future = self.loop.create_future()

            reader, writer = await asyncio.open_connection(
                *addr,
                ssl=client_sslctx,
                server_hostname='')
            writer.write(b'ping')
            data = await reader.readexactly(4)
            self.assertEqual(data, b'pong')

            # fill write backlog in a hacky way - renegotiation won't help
            for _ in range(SIZE):
                writer.transport._test__append_write_backlog(b'x' * CHUNK)

            try:
                data = await reader.read()
                self.assertEqual(data, b'')
            except (BrokenPipeError, ConnectionResetError):
                pass

            await future

            writer.close()
            await self.wait_closed(writer)

        def run(meth):
            def wrapper(sock):
                try:
                    meth(sock)
                except Exception as ex:
                    self.loop.call_soon_threadsafe(future.set_exception, ex)
                else:
                    self.loop.call_soon_threadsafe(future.set_result, None)
            return wrapper

        with self.tcp_server(run(server)) as srv:
            self.loop.run_until_complete(client(srv.addr))

        with self.tcp_server(run(eof_server)) as srv:
            self.loop.run_until_complete(client(srv.addr))

    def test_remote_shutdown_receives_trailing_data_on_slow_socket(self):
        # This test is the same as test_remote_shutdown_receives_trailing_data,
        # except it simulates a socket that is not able to write data in time,
        # thus triggering different code path in _SelectorSocketTransport.
        # This triggers bug gh-115514, also tested using mocks in
        # test.test_asyncio.test_selector_events.SelectorSocketTransportTests.test_write_buffer_after_close
        # The slow path is triggered here by setting SO_SNDBUF, see code and comment below.

        CHUNK = 1024 * 128
        SIZE = 32

        sslctx = self._create_server_ssl_context(
            test_utils.ONLYCERT,
            test_utils.ONLYKEY
        )
        client_sslctx = self._create_client_ssl_context()
        future = None

        def server(sock):
            incoming = ssl.MemoryBIO()
            outgoing = ssl.MemoryBIO()
            sslobj = sslctx.wrap_bio(incoming, outgoing, server_side=True)

            while True:
                try:
                    sslobj.do_handshake()
                except ssl.SSLWantReadError:
                    if outgoing.pending:
                        sock.send(outgoing.read())
                    incoming.write(sock.recv(16384))
                else:
                    if outgoing.pending:
                        sock.send(outgoing.read())
                    break

            while True:
                try:
                    data = sslobj.read(4)
                except ssl.SSLWantReadError:
                    incoming.write(sock.recv(16384))
                else:
                    break

            self.assertEqual(data, b'ping')
            sslobj.write(b'pong')
            sock.send(outgoing.read())

            time.sleep(0.2)  # wait for the peer to fill its backlog

            # send close_notify but don't wait for response
            with self.assertRaises(ssl.SSLWantReadError):
                sslobj.unwrap()
            sock.send(outgoing.read())

            # should receive all data
            data_len = 0
            while True:
                try:
                    chunk = len(sslobj.read(16384))
                    data_len += chunk
                except ssl.SSLWantReadError:
                    incoming.write(sock.recv(16384))
                except ssl.SSLZeroReturnError:
                    break

            self.assertEqual(data_len, CHUNK * SIZE*2)

            # verify that close_notify is received
            sslobj.unwrap()

            sock.close()

        def eof_server(sock):
            sock.starttls(sslctx, server_side=True)
            self.assertEqual(sock.recv_all(4), b'ping')
            sock.send(b'pong')

            time.sleep(0.2)  # wait for the peer to fill its backlog

            # send EOF
            sock.shutdown(socket.SHUT_WR)

            # should receive all data
            data = sock.recv_all(CHUNK * SIZE)
            self.assertEqual(len(data), CHUNK * SIZE)

            sock.close()

        async def client(addr):
            nonlocal future
            future = self.loop.create_future()

            reader, writer = await asyncio.open_connection(
                *addr,
                ssl=client_sslctx,
                server_hostname='')
            writer.write(b'ping')
            data = await reader.readexactly(4)
            self.assertEqual(data, b'pong')

            # fill write backlog in a hacky way - renegotiation won't help
            for _ in range(SIZE*2):
                writer.transport._test__append_write_backlog(b'x' * CHUNK)

            try:
                data = await reader.read()
                self.assertEqual(data, b'')
            except (BrokenPipeError, ConnectionResetError):
                pass

            # Make sure _SelectorSocketTransport enters the delayed write
            # path in its `write` method by wrapping socket in a fake class
            # that acts as if there is not enough space in socket buffer.
            # This triggers bug gh-115514, also tested using mocks in
            # test.test_asyncio.test_selector_events.SelectorSocketTransportTests.test_write_buffer_after_close
            socket_transport = writer.transport._ssl_protocol._transport

            class SocketWrapper:
                def __init__(self, sock) -> None:
                    self.sock = sock

                def __getattr__(self, name):
                    return getattr(self.sock, name)

                def send(self, data):
                    # Fake that our write buffer is full, send only half
                    to_send = len(data)//2
                    return self.sock.send(data[:to_send])

            def _fake_full_write_buffer(data):
                if socket_transport._read_ready_cb is None and not isinstance(socket_transport._sock, SocketWrapper):
                    socket_transport._sock = SocketWrapper(socket_transport._sock)
                return unittest.mock.DEFAULT

            with unittest.mock.patch.object(
                socket_transport, "write",
                wraps=socket_transport.write,
                side_effect=_fake_full_write_buffer
            ):
                await future

                writer.close()
                await self.wait_closed(writer)

        def run(meth):
            def wrapper(sock):
                try:
                    meth(sock)
                except Exception as ex:
                    self.loop.call_soon_threadsafe(future.set_exception, ex)
                else:
                    self.loop.call_soon_threadsafe(future.set_result, None)
            return wrapper

        with self.tcp_server(run(server)) as srv:
            self.loop.run_until_complete(client(srv.addr))

        with self.tcp_server(run(eof_server)) as srv:
            self.loop.run_until_complete(client(srv.addr))

    def test_connect_timeout_warning(self):
        s = socket.socket(socket.AF_INET)
        s.bind(('127.0.0.1', 0))
        addr = s.getsockname()

        async def test():
            try:
                await asyncio.wait_for(
                    self.loop.create_connection(asyncio.Protocol,
                                                *addr, ssl=True),
                    0.1)
            except (ConnectionRefusedError, asyncio.TimeoutError):
                pass
            else:
                self.fail('TimeoutError is not raised')

        with s:
            try:
                with self.assertWarns(ResourceWarning) as cm:
                    self.loop.run_until_complete(test())
                    gc.collect()
                    gc.collect()
                    gc.collect()
            except AssertionError as e:
                self.assertEqual(str(e), 'ResourceWarning not triggered')
            else:
                self.fail('Unexpected ResourceWarning: {}'.format(cm.warning))

    def test_handshake_timeout_handler_leak(self):
        s = socket.socket(socket.AF_INET)
        s.bind(('127.0.0.1', 0))
        s.listen(1)
        addr = s.getsockname()

        async def test(ctx):
            try:
                await asyncio.wait_for(
                    self.loop.create_connection(asyncio.Protocol, *addr,
                                                ssl=ctx),
                    0.1)
            except (ConnectionRefusedError, asyncio.TimeoutError):
                pass
            else:
                self.fail('TimeoutError is not raised')

        with s:
            ctx = ssl.create_default_context()
            self.loop.run_until_complete(test(ctx))
            ctx = weakref.ref(ctx)

        # SSLProtocol should be DECREF to 0
        self.assertIsNone(ctx())

    def test_shutdown_timeout_handler_leak(self):
        loop = self.loop

        def server(sock):
            sslctx = self._create_server_ssl_context(
                test_utils.ONLYCERT,
                test_utils.ONLYKEY
            )
            sock = sslctx.wrap_socket(sock, server_side=True)
            sock.recv(32)
            sock.close()

        class Protocol(asyncio.Protocol):
            def __init__(self):
                self.fut = asyncio.Future(loop=loop)

            def connection_lost(self, exc):
                self.fut.set_result(None)

        async def client(addr, ctx):
            tr, pr = await loop.create_connection(Protocol, *addr, ssl=ctx)
            tr.close()
            await pr.fut

        with self.tcp_server(server) as srv:
            ctx = self._create_client_ssl_context()
            loop.run_until_complete(client(srv.addr, ctx))
            ctx = weakref.ref(ctx)

        # asyncio has no shutdown timeout, but it ends up with a circular
        # reference loop - not ideal (introduces gc glitches), but at least
        # not leaking
        gc.collect()
        gc.collect()
        gc.collect()

        # SSLProtocol should be DECREF to 0
        self.assertIsNone(ctx())

    def test_shutdown_timeout_handler_not_set(self):
        loop = self.loop
        eof = asyncio.Event()
        extra = None

        def server(sock):
            sslctx = self._create_server_ssl_context(
                test_utils.ONLYCERT,
                test_utils.ONLYKEY
            )
            sock = sslctx.wrap_socket(sock, server_side=True)
            sock.send(b'hello')
            assert sock.recv(1024) == b'world'
            sock.send(b'extra bytes')
            # sending EOF here
            sock.shutdown(socket.SHUT_WR)
            loop.call_soon_threadsafe(eof.set)
            # make sure we have enough time to reproduce the issue
            assert sock.recv(1024) == b''
            sock.close()

        class Protocol(asyncio.Protocol):
            def __init__(self):
                self.fut = asyncio.Future(loop=loop)
                self.transport = None

            def connection_made(self, transport):
                self.transport = transport

            def data_received(self, data):
                if data == b'hello':
                    self.transport.write(b'world')
                    # pause reading would make incoming data stay in the sslobj
                    self.transport.pause_reading()
                else:
                    nonlocal extra
                    extra = data

            def connection_lost(self, exc):
                if exc is None:
                    self.fut.set_result(None)
                else:
                    self.fut.set_exception(exc)

        async def client(addr):
            ctx = self._create_client_ssl_context()
            tr, pr = await loop.create_connection(Protocol, *addr, ssl=ctx)
            await eof.wait()
            tr.resume_reading()
            await pr.fut
            tr.close()
            assert extra == b'extra bytes'

        with self.tcp_server(server) as srv:
            loop.run_until_complete(client(srv.addr))


###############################################################################
# Socket Testing Utilities
###############################################################################


class TestSocketWrapper:

    def __init__(self, sock):
        self.__sock = sock

    def recv_all(self, n):
        buf = b''
        while len(buf) < n:
            data = self.recv(n - len(buf))
            if data == b'':
                raise ConnectionAbortedError
            buf += data
        return buf

    def starttls(self, ssl_context, *,
                 server_side=False,
                 server_hostname=None,
                 do_handshake_on_connect=True):

        assert isinstance(ssl_context, ssl.SSLContext)

        ssl_sock = ssl_context.wrap_socket(
            self.__sock, server_side=server_side,
            server_hostname=server_hostname,
            do_handshake_on_connect=do_handshake_on_connect)

        if server_side:
            ssl_sock.do_handshake()

        self.__sock.close()
        self.__sock = ssl_sock

    def __getattr__(self, name):
        return getattr(self.__sock, name)

    def __repr__(self):
        return '<{} {!r}>'.format(type(self).__name__, self.__sock)


class SocketThread(threading.Thread):

    def stop(self):
        self._active = False
        self.join()

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, *exc):
        self.stop()


class TestThreadedClient(SocketThread):

    def __init__(self, test, sock, prog, timeout):
        threading.Thread.__init__(self, None, None, 'test-client')
        self.daemon = True

        self._timeout = timeout
        self._sock = sock
        self._active = True
        self._prog = prog
        self._test = test

    def run(self):
        try:
            self._prog(TestSocketWrapper(self._sock))
        except (KeyboardInterrupt, SystemExit):
            raise
        except BaseException as ex:
            self._test._abort_socket_test(ex)


class TestThreadedServer(SocketThread):

    def __init__(self, test, sock, prog, timeout, max_clients):
        threading.Thread.__init__(self, None, None, 'test-server')
        self.daemon = True

        self._clients = 0
        self._finished_clients = 0
        self._max_clients = max_clients
        self._timeout = timeout
        self._sock = sock
        self._active = True

        self._prog = prog

        self._s1, self._s2 = socket.socketpair()
        self._s1.setblocking(False)

        self._test = test

    def stop(self):
        try:
            if self._s2 and self._s2.fileno() != -1:
                try:
                    self._s2.send(b'stop')
                except OSError:
                    pass
        finally:
            super().stop()
            self._sock.close()
            self._s1.close()
            self._s2.close()

    def run(self):
        self._sock.setblocking(False)
        self._run()

    def _run(self):
        while self._active:
            if self._clients >= self._max_clients:
                return

            r, w, x = select.select(
                [self._sock, self._s1], [], [], self._timeout)

            if self._s1 in r:
                return

            if self._sock in r:
                try:
                    conn, addr = self._sock.accept()
                except BlockingIOError:
                    continue
                except socket.timeout:
                    if not self._active:
                        return
                    else:
                        raise
                else:
                    self._clients += 1
                    conn.settimeout(self._timeout)
                    try:
                        with conn:
                            self._handle_client(conn)
                    except (KeyboardInterrupt, SystemExit):
                        raise
                    except BaseException as ex:
                        self._active = False
                        try:
                            raise
                        finally:
                            self._test._abort_socket_test(ex)

    def _handle_client(self, sock):
        self._prog(TestSocketWrapper(sock))

    @property
    def addr(self):
        return self._sock.getsockname()
