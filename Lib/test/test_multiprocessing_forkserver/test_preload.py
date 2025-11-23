"""Tests for forkserver preload functionality."""

import multiprocessing
import multiprocessing.forkserver
import unittest
import warnings


class TestForkserverPreload(unittest.TestCase):
    """Tests for forkserver preload functionality."""

    def setUp(self):
        self.ctx = multiprocessing.get_context('forkserver')
        # Ensure clean state for each test
        multiprocessing.forkserver._forkserver._stop()

    def tearDown(self):
        # Clean up after tests
        multiprocessing.forkserver._forkserver._stop()
        self.ctx.set_forkserver_preload([])

    @staticmethod
    def _send_value(conn, value):
        """Helper to send a value through a connection."""
        conn.send(value)

    def test_preload_on_error_ignore_default(self):
        """Test that invalid modules are silently ignored by default."""
        # With on_error='ignore' (default), invalid module is ignored
        self.ctx.set_forkserver_preload(['nonexistent_module_xyz'])

        # Should be able to start a process without errors
        r, w = self.ctx.Pipe(duplex=False)
        p = self.ctx.Process(target=self._send_value, args=(w, 42))
        p.start()
        w.close()
        result = r.recv()
        r.close()
        p.join()

        self.assertEqual(result, 42)
        self.assertEqual(p.exitcode, 0)

    def test_preload_on_error_ignore_explicit(self):
        """Test that invalid modules are silently ignored with on_error='ignore'."""
        self.ctx.set_forkserver_preload(['nonexistent_module_xyz'], on_error='ignore')

        # Should be able to start a process without errors
        r, w = self.ctx.Pipe(duplex=False)
        p = self.ctx.Process(target=self._send_value, args=(w, 99))
        p.start()
        w.close()
        result = r.recv()
        r.close()
        p.join()

        self.assertEqual(result, 99)
        self.assertEqual(p.exitcode, 0)

    def test_preload_on_error_warn(self):
        """Test that invalid modules emit warnings with on_error='warn'."""
        self.ctx.set_forkserver_preload(['nonexistent_module_xyz'], on_error='warn')

        # Should still be able to start a process, warnings are in subprocess
        r, w = self.ctx.Pipe(duplex=False)
        p = self.ctx.Process(target=self._send_value, args=(w, 123))
        p.start()
        w.close()
        result = r.recv()
        r.close()
        p.join()

        self.assertEqual(result, 123)
        self.assertEqual(p.exitcode, 0)

    def test_preload_on_error_fail_breaks_context(self):
        """Test that invalid modules with on_error='fail' breaks the forkserver."""
        self.ctx.set_forkserver_preload(['nonexistent_module_xyz'], on_error='fail')

        # The forkserver should fail to start when it tries to import
        # The exception is raised during p.start() when trying to communicate
        # with the failed forkserver process
        r, w = self.ctx.Pipe(duplex=False)
        try:
            p = self.ctx.Process(target=self._send_value, args=(w, 42))
            with self.assertRaises((EOFError, ConnectionError, BrokenPipeError)) as cm:
                p.start()  # Exception raised here
            # Verify that the helpful note was added
            self.assertIn('Forkserver process may have crashed', str(cm.exception.__notes__[0]))
        finally:
            # Ensure pipes are closed even if exception is raised
            w.close()
            r.close()

    def test_preload_valid_modules_with_on_error_fail(self):
        """Test that valid modules work fine with on_error='fail'."""
        # Valid modules should work even with on_error='fail'
        self.ctx.set_forkserver_preload(['os', 'sys'], on_error='fail')

        r, w = self.ctx.Pipe(duplex=False)
        p = self.ctx.Process(target=self._send_value, args=(w, 'success'))
        p.start()
        w.close()
        result = r.recv()
        r.close()
        p.join()

        self.assertEqual(result, 'success')
        self.assertEqual(p.exitcode, 0)

    def test_preload_invalid_on_error_value(self):
        """Test that invalid on_error values raise ValueError."""
        with self.assertRaises(ValueError) as cm:
            self.ctx.set_forkserver_preload(['os'], on_error='invalid')
        self.assertIn("on_error must be 'ignore', 'warn', or 'fail'", str(cm.exception))


if __name__ == '__main__':
    unittest.main()
