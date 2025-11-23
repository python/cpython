"""Tests for forkserver preload functionality."""

import multiprocessing
import multiprocessing.forkserver
import unittest


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

    def test_preload_raise_exceptions_false_default(self):
        """Test that invalid modules are silently ignored by default."""
        # With raise_exceptions=False (default), invalid module is ignored
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

    def test_preload_raise_exceptions_false_explicit(self):
        """Test that invalid modules are silently ignored with raise_exceptions=False."""
        self.ctx.set_forkserver_preload(['nonexistent_module_xyz'], raise_exceptions=False)

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

    def test_preload_raise_exceptions_true_breaks_context(self):
        """Test that invalid modules with raise_exceptions=True breaks the forkserver."""
        self.ctx.set_forkserver_preload(['nonexistent_module_xyz'], raise_exceptions=True)

        # The forkserver should fail to start when it tries to import
        # The exception is raised during p.start() when trying to communicate
        # with the failed forkserver process
        r, w = self.ctx.Pipe(duplex=False)
        try:
            p = self.ctx.Process(target=self._send_value, args=(w, 42))
            with self.assertRaises((EOFError, ConnectionError, BrokenPipeError)):
                p.start()  # Exception raised here
        finally:
            # Ensure pipes are closed even if exception is raised
            w.close()
            r.close()

    def test_preload_valid_modules_with_raise_exceptions(self):
        """Test that valid modules work fine with raise_exceptions=True."""
        # Valid modules should work even with raise_exceptions=True
        self.ctx.set_forkserver_preload(['os', 'sys'], raise_exceptions=True)

        r, w = self.ctx.Pipe(duplex=False)
        p = self.ctx.Process(target=self._send_value, args=(w, 'success'))
        p.start()
        w.close()
        result = r.recv()
        r.close()
        p.join()

        self.assertEqual(result, 'success')
        self.assertEqual(p.exitcode, 0)


if __name__ == '__main__':
    unittest.main()
