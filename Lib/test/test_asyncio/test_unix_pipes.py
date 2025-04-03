"""
Functional tests for Unix transport pipes, designed around opening two sockets
on the localhost and sending information between
"""

import unittest
import sys
import os
import tempfile
import threading

if sys.platform == 'win32':
    raise unittest.SkipTest('UNIX only')

import asyncio
from asyncio import unix_events


def tearDownModule():
    asyncio.set_event_loop_policy(None)


class UnixReadPipeTransportFuncTests(unittest.TestCase):
    """
    Verify that transports on Unix can facilitate reading and have access to
    all state methods: verify using class internals
    """

    async def register_read_handle(self):
        """
        Wait for read_handle and then register it to the loop
        """
        self.transport, self.protocol = await self.loop.connect_read_pipe(
            asyncio.BaseProtocol,
            self.read_handle,
        )

    def setup_read_handle(self):
        """
        Open the read handle and record it in an attribute
        """
        self.read_handle = open(self.pipe, "r")

    def setup_write_handle(self):
        """
        Open the write handle and record it in an attribute
        """
        self.write_handle = open(self.pipe, "w")

    def setUp(self):
        """
        Create the UNIX pipe and register the read end to the loop, and connect
        a write handle asynchronously
        """
        self.loop = asyncio.get_event_loop()
        self.temp_dir = tempfile.TemporaryDirectory(suffix="async_unix_events")
        self.pipe = os.path.join(self.temp_dir.name, "unix_pipe")
        os.mkfifo(self.pipe)

        # Set the threads to open the handles going
        r_handle_thread = threading.Thread(target=self.setup_read_handle)
        r_handle_thread.start()
        w_handle_thread = threading.Thread(target=self.setup_write_handle)
        w_handle_thread.start()

        # Wait for pipe pair to connect
        r_handle_thread.join()
        w_handle_thread.join()

        # Once pipe is connected, get the read transport
        self.loop.run_until_complete(self.register_read_handle())

        self.assertIsInstance(self.transport,
                              unix_events._UnixReadPipeTransport)

    def tearDown(self):
        """
        Destroy the read transport and the pipe
        """
        self.transport.close()
        self.write_handle.close()
        self.read_handle.close()
        self.loop._run_once()
        os.unlink(self.pipe)
        self.temp_dir.cleanup()
        self.loop.close()

    def test_is_reading(self):
        """
        Verify that is_reading returns True unless transport is closed/closing
        or paused
        """
        self.assertTrue(self.transport.is_reading())
        self.transport.pause_reading()
        self.assertFalse(self.transport.is_reading())
        self.transport.resume_reading()
        self.assertTrue(self.transport.is_reading())
        self.transport.close()
        self.assertFalse(self.transport.is_reading())


if __name__ == "__main__":
    unittest.main()
