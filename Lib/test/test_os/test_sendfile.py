import socket
import unittest
import os
import errno
import sys
import asyncio
from test.support import os_helper
from test.support import socket_helper
from .utils import create_file


def tearDownModule():
    asyncio.events._set_event_loop_policy(None)


@unittest.skipUnless(hasattr(os, 'sendfile'), "test needs os.sendfile()")
class TestSendfile(unittest.IsolatedAsyncioTestCase):

    DATA = b"12345abcde" * 16 * 1024  # 160 KiB
    SUPPORT_HEADERS_TRAILERS = (
        not sys.platform.startswith(("linux", "android", "solaris", "sunos")))
    requires_headers_trailers = unittest.skipUnless(SUPPORT_HEADERS_TRAILERS,
            'requires headers and trailers support')
    requires_32b = unittest.skipUnless(sys.maxsize < 2**32,
            'test is only meaningful on 32-bit builds')

    @classmethod
    def setUpClass(cls):
        create_file(os_helper.TESTFN, cls.DATA)

    @classmethod
    def tearDownClass(cls):
        os_helper.unlink(os_helper.TESTFN)

    @staticmethod
    async def chunks(reader):
        while not reader.at_eof():
            yield await reader.read()

    async def handle_new_client(self, reader, writer):
        self.server_buffer = b''.join([x async for x in self.chunks(reader)])
        writer.close()
        self.server.close()  # The test server processes a single client only

    async def asyncSetUp(self):
        self.server_buffer = b''
        self.server = await asyncio.start_server(self.handle_new_client,
                                                 socket_helper.HOSTv4)
        server_name = self.server.sockets[0].getsockname()
        self.client = socket.socket()
        self.client.setblocking(False)
        await asyncio.get_running_loop().sock_connect(self.client, server_name)
        self.sockno = self.client.fileno()
        self.file = open(os_helper.TESTFN, 'rb')
        self.fileno = self.file.fileno()

    async def asyncTearDown(self):
        self.file.close()
        self.client.close()
        await self.server.wait_closed()

    # Use the test subject instead of asyncio.loop.sendfile
    @staticmethod
    async def async_sendfile(*args, **kwargs):
        return await asyncio.to_thread(os.sendfile, *args, **kwargs)

    @staticmethod
    async def sendfile_wrapper(*args, **kwargs):
        """A higher level wrapper representing how an application is
        supposed to use sendfile().
        """
        while True:
            try:
                return await TestSendfile.async_sendfile(*args, **kwargs)
            except OSError as err:
                if err.errno == errno.ECONNRESET:
                    # disconnected
                    raise
                elif err.errno in (errno.EAGAIN, errno.EBUSY):
                    # we have to retry send data
                    continue
                else:
                    raise

    async def test_send_whole_file(self):
        # normal send
        total_sent = 0
        offset = 0
        nbytes = 4096
        while total_sent < len(self.DATA):
            sent = await self.sendfile_wrapper(self.sockno, self.fileno,
                                               offset, nbytes)
            if sent == 0:
                break
            offset += sent
            total_sent += sent
            self.assertTrue(sent <= nbytes)
            self.assertEqual(offset, total_sent)

        self.assertEqual(total_sent, len(self.DATA))
        self.client.shutdown(socket.SHUT_RDWR)
        self.client.close()
        await self.server.wait_closed()
        self.assertEqual(len(self.server_buffer), len(self.DATA))
        self.assertEqual(self.server_buffer, self.DATA)

    async def test_send_at_certain_offset(self):
        # start sending a file at a certain offset
        total_sent = 0
        offset = len(self.DATA) // 2
        must_send = len(self.DATA) - offset
        nbytes = 4096
        while total_sent < must_send:
            sent = await self.sendfile_wrapper(self.sockno, self.fileno,
                                               offset, nbytes)
            if sent == 0:
                break
            offset += sent
            total_sent += sent
            self.assertTrue(sent <= nbytes)

        self.client.shutdown(socket.SHUT_RDWR)
        self.client.close()
        await self.server.wait_closed()
        expected = self.DATA[len(self.DATA) // 2:]
        self.assertEqual(total_sent, len(expected))
        self.assertEqual(len(self.server_buffer), len(expected))
        self.assertEqual(self.server_buffer, expected)

    async def test_offset_overflow(self):
        # specify an offset > file size
        offset = len(self.DATA) + 4096
        try:
            sent = await self.async_sendfile(self.sockno, self.fileno,
                                             offset, 4096)
        except OSError as e:
            # Solaris can raise EINVAL if offset >= file length, ignore.
            if e.errno != errno.EINVAL:
                raise
        else:
            self.assertEqual(sent, 0)
        self.client.shutdown(socket.SHUT_RDWR)
        self.client.close()
        await self.server.wait_closed()
        self.assertEqual(self.server_buffer, b'')

    async def test_invalid_offset(self):
        with self.assertRaises(OSError) as cm:
            await self.async_sendfile(self.sockno, self.fileno, -1, 4096)
        self.assertEqual(cm.exception.errno, errno.EINVAL)

    async def test_invalid_count(self):
        with self.assertRaises(ValueError, msg="count cannot be negative"):
            await self.sendfile_wrapper(self.sockno, self.fileno, offset=0,
                                        count=-1)

    async def test_keywords(self):
        # Keyword arguments should be supported
        await self.async_sendfile(out_fd=self.sockno, in_fd=self.fileno,
                                  offset=0, count=4096)
        if self.SUPPORT_HEADERS_TRAILERS:
            await self.async_sendfile(out_fd=self.sockno, in_fd=self.fileno,
                                      offset=0, count=4096,
                                      headers=(), trailers=(), flags=0)

    # --- headers / trailers tests

    @requires_headers_trailers
    async def test_headers(self):
        total_sent = 0
        expected_data = b"x" * 512 + b"y" * 256 + self.DATA[:-1]
        sent = await self.async_sendfile(self.sockno, self.fileno, 0, 4096,
                                         headers=[b"x" * 512, b"y" * 256])
        self.assertLessEqual(sent, 512 + 256 + 4096)
        total_sent += sent
        offset = 4096
        while total_sent < len(expected_data):
            nbytes = min(len(expected_data) - total_sent, 4096)
            sent = await self.sendfile_wrapper(self.sockno, self.fileno,
                                               offset, nbytes)
            if sent == 0:
                break
            self.assertLessEqual(sent, nbytes)
            total_sent += sent
            offset += sent

        self.assertEqual(total_sent, len(expected_data))
        self.client.close()
        await self.server.wait_closed()
        self.assertEqual(hash(self.server_buffer), hash(expected_data))

    @requires_headers_trailers
    async def test_trailers(self):
        TESTFN2 = os_helper.TESTFN + "2"
        file_data = b"abcdef"

        self.addCleanup(os_helper.unlink, TESTFN2)
        create_file(TESTFN2, file_data)

        with open(TESTFN2, 'rb') as f:
            await self.async_sendfile(self.sockno, f.fileno(), 0, 5,
                                      trailers=[b"123456", b"789"])
            self.client.close()
            await self.server.wait_closed()
            self.assertEqual(self.server_buffer, b"abcde123456789")

    @requires_headers_trailers
    @requires_32b
    async def test_headers_overflow_32bits(self):
        self.server.handler_instance.accumulate = False
        with self.assertRaises(OSError) as cm:
            await self.async_sendfile(self.sockno, self.fileno, 0, 0,
                                      headers=[b"x" * 2**16] * 2**15)
        self.assertEqual(cm.exception.errno, errno.EINVAL)

    @requires_headers_trailers
    @requires_32b
    async def test_trailers_overflow_32bits(self):
        self.server.handler_instance.accumulate = False
        with self.assertRaises(OSError) as cm:
            await self.async_sendfile(self.sockno, self.fileno, 0, 0,
                                      trailers=[b"x" * 2**16] * 2**15)
        self.assertEqual(cm.exception.errno, errno.EINVAL)

    @requires_headers_trailers
    @unittest.skipUnless(hasattr(os, 'SF_NODISKIO'),
                         'test needs os.SF_NODISKIO')
    async def test_flags(self):
        try:
            await self.async_sendfile(self.sockno, self.fileno, 0, 4096,
                                      flags=os.SF_NODISKIO)
        except OSError as err:
            if err.errno not in (errno.EBUSY, errno.EAGAIN):
                raise


if __name__ == "__main__":
    unittest.main()
