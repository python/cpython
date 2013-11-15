"""Tests for window_utils"""

import sys
import test.support
import unittest
import unittest.mock

if sys.platform != 'win32':
    raise unittest.SkipTest('Windows only')

import _winapi

from asyncio import windows_utils
from asyncio import _overlapped


class WinsocketpairTests(unittest.TestCase):

    def test_winsocketpair(self):
        ssock, csock = windows_utils.socketpair()

        csock.send(b'xxx')
        self.assertEqual(b'xxx', ssock.recv(1024))

        csock.close()
        ssock.close()

    @unittest.mock.patch('asyncio.windows_utils.socket')
    def test_winsocketpair_exc(self, m_socket):
        m_socket.socket.return_value.getsockname.return_value = ('', 12345)
        m_socket.socket.return_value.accept.return_value = object(), object()
        m_socket.socket.return_value.connect.side_effect = OSError()

        self.assertRaises(OSError, windows_utils.socketpair)


class PipeTests(unittest.TestCase):

    def test_pipe_overlapped(self):
        h1, h2 = windows_utils.pipe(overlapped=(True, True))
        try:
            ov1 = _overlapped.Overlapped()
            self.assertFalse(ov1.pending)
            self.assertEqual(ov1.error, 0)

            ov1.ReadFile(h1, 100)
            self.assertTrue(ov1.pending)
            self.assertEqual(ov1.error, _winapi.ERROR_IO_PENDING)
            ERROR_IO_INCOMPLETE = 996
            try:
                ov1.getresult()
            except OSError as e:
                self.assertEqual(e.winerror, ERROR_IO_INCOMPLETE)
            else:
                raise RuntimeError('expected ERROR_IO_INCOMPLETE')

            ov2 = _overlapped.Overlapped()
            self.assertFalse(ov2.pending)
            self.assertEqual(ov2.error, 0)

            ov2.WriteFile(h2, b"hello")
            self.assertIn(ov2.error, {0, _winapi.ERROR_IO_PENDING})

            res = _winapi.WaitForMultipleObjects([ov2.event], False, 100)
            self.assertEqual(res, _winapi.WAIT_OBJECT_0)

            self.assertFalse(ov1.pending)
            self.assertEqual(ov1.error, ERROR_IO_INCOMPLETE)
            self.assertFalse(ov2.pending)
            self.assertIn(ov2.error, {0, _winapi.ERROR_IO_PENDING})
            self.assertEqual(ov1.getresult(), b"hello")
        finally:
            _winapi.CloseHandle(h1)
            _winapi.CloseHandle(h2)

    def test_pipe_handle(self):
        h, _ = windows_utils.pipe(overlapped=(True, True))
        _winapi.CloseHandle(_)
        p = windows_utils.PipeHandle(h)
        self.assertEqual(p.fileno(), h)
        self.assertEqual(p.handle, h)

        # check garbage collection of p closes handle
        del p
        test.support.gc_collect()
        try:
            _winapi.CloseHandle(h)
        except OSError as e:
            self.assertEqual(e.winerror, 6)     # ERROR_INVALID_HANDLE
        else:
            raise RuntimeError('expected ERROR_INVALID_HANDLE')


class PopenTests(unittest.TestCase):

    def test_popen(self):
        command = r"""if 1:
            import sys
            s = sys.stdin.readline()
            sys.stdout.write(s.upper())
            sys.stderr.write('stderr')
            """
        msg = b"blah\n"

        p = windows_utils.Popen([sys.executable, '-c', command],
                                stdin=windows_utils.PIPE,
                                stdout=windows_utils.PIPE,
                                stderr=windows_utils.PIPE)

        for f in [p.stdin, p.stdout, p.stderr]:
            self.assertIsInstance(f, windows_utils.PipeHandle)

        ovin = _overlapped.Overlapped()
        ovout = _overlapped.Overlapped()
        overr = _overlapped.Overlapped()

        ovin.WriteFile(p.stdin.handle, msg)
        ovout.ReadFile(p.stdout.handle, 100)
        overr.ReadFile(p.stderr.handle, 100)

        events = [ovin.event, ovout.event, overr.event]
        # Super-long timeout for slow buildbots.
        res = _winapi.WaitForMultipleObjects(events, True, 10000)
        self.assertEqual(res, _winapi.WAIT_OBJECT_0)
        self.assertFalse(ovout.pending)
        self.assertFalse(overr.pending)
        self.assertFalse(ovin.pending)

        self.assertEqual(ovin.getresult(), len(msg))
        out = ovout.getresult().rstrip()
        err = overr.getresult().rstrip()

        self.assertGreater(len(out), 0)
        self.assertGreater(len(err), 0)
        # allow for partial reads...
        self.assertTrue(msg.upper().rstrip().startswith(out))
        self.assertTrue(b"stderr".startswith(err))


if __name__ == '__main__':
    unittest.main()
