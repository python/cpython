import unittest
from unittest import mock
import test._test_multiprocessing

import os
import sys
from test import support

if support.PGO:
    raise unittest.SkipTest("test is not helpful for PGO")

if sys.platform == "win32":
    raise unittest.SkipTest("forkserver is not available on Windows")

import multiprocessing
import multiprocessing.connection
import multiprocessing.forkserver

test._test_multiprocessing.install_tests_in_module_dict(globals(), 'forkserver')


class TestForkserverControlAuthentication(unittest.TestCase):
    def setUp(self):
        super().setUp()
        self.context = multiprocessing.get_context("forkserver")
        self.pool = self.context.Pool(processes=1, maxtasksperchild=4)
        self.assertEqual(self.pool.apply(eval, ("2+2",)), 4)
        self.forkserver = multiprocessing.forkserver._forkserver
        self.addr = self.forkserver._forkserver_address
        self.assertTrue(self.addr)
        self.authkey = self.forkserver._forkserver_authkey
        self.assertGreater(len(self.authkey), 15)
        self.assertTrue(self.forkserver._forkserver_pid)

    def tearDown(self):
        self.pool.terminate()
        self.pool.join()
        super().tearDown()

    def test_auth_works(self):
        """FYI: An 'EOFError: ran out of input' from a worker is normal."""
        # First, demonstrate that a raw auth handshake as Client makes
        # does not raise.
        client = multiprocessing.connection.Client(
                self.addr, authkey=self.authkey)
        client.close()

        # Now use forkserver code to do the same thing and more.
        status_r, data_w = self.forkserver.connect_to_new_process([])
        # It is normal for this to trigger an EOFError on stderr from the
        # process... it is expecting us to send over a pickle of a Process
        # instance to tell it what to do.
        # If the authentication handshake and subsequent file descriptor
        # sending dance had failed, an exception would've been raised.
        os.close(data_w)
        os.close(status_r)

    def test_no_auth_fails(self):
        with mock.patch.object(self.forkserver, '_forkserver_authkey', None):
            # With no authkey set, the connection this makes will fail to
            # do the file descriptor transfer over the pipe.
            with self.assertRaisesRegex(RuntimeError, 'not receive ack'):
                status_r, data_w = self.forkserver.connect_to_new_process([])


if __name__ == '__main__':
    unittest.main()
