"""Tests for forkserver preload functionality."""

import multiprocessing
import tempfile
import unittest
from multiprocessing import forkserver


class TestForkserverPreload(unittest.TestCase):
    """Tests for forkserver preload functionality."""

    def setUp(self):
        self.ctx = multiprocessing.get_context('forkserver')
        forkserver._forkserver._stop()

    def tearDown(self):
        forkserver._forkserver._stop()
        self.ctx.set_forkserver_preload([])

    @staticmethod
    def _send_value(conn, value):
        """Helper to send a value through a connection."""
        conn.send(value)

    def test_preload_on_error_ignore_default(self):
        """Test that invalid modules are silently ignored by default."""
        self.ctx.set_forkserver_preload(['nonexistent_module_xyz'])

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

        r, w = self.ctx.Pipe(duplex=False)
        try:
            p = self.ctx.Process(target=self._send_value, args=(w, 42))
            with self.assertRaises((EOFError, ConnectionError, BrokenPipeError)) as cm:
                p.start()
            notes = getattr(cm.exception, '__notes__', [])
            self.assertTrue(notes, "Expected exception to have __notes__")
            self.assertIn('Forkserver process may have crashed', notes[0])
        finally:
            w.close()
            r.close()

    def test_preload_valid_modules_with_on_error_fail(self):
        """Test that valid modules work fine with on_error='fail'."""
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


class TestHandlePreload(unittest.TestCase):
    """Unit tests for _handle_preload() function."""

    def test_handle_preload_main_on_error_fail(self):
        """Test that __main__ import failures raise with on_error='fail'."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.py') as f:
            f.write('raise RuntimeError("test error in __main__")\n')
            f.flush()
            with self.assertRaises(RuntimeError) as cm:
                forkserver._handle_preload(['__main__'], main_path=f.name, on_error='fail')
            self.assertIn("test error in __main__", str(cm.exception))

    def test_handle_preload_main_on_error_warn(self):
        """Test that __main__ import failures warn with on_error='warn'."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.py') as f:
            f.write('raise ImportError("test import error")\n')
            f.flush()
            with self.assertWarns(ImportWarning) as cm:
                forkserver._handle_preload(['__main__'], main_path=f.name, on_error='warn')
            self.assertIn("Failed to preload __main__", str(cm.warning))
            self.assertIn("test import error", str(cm.warning))

    def test_handle_preload_main_on_error_ignore(self):
        """Test that __main__ import failures are ignored with on_error='ignore'."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.py') as f:
            f.write('raise ImportError("test import error")\n')
            f.flush()
            forkserver._handle_preload(['__main__'], main_path=f.name, on_error='ignore')

    def test_handle_preload_main_valid(self):
        """Test that valid __main__ preload works."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.py') as f:
            f.write('test_var = 42\n')
            f.flush()
            forkserver._handle_preload(['__main__'], main_path=f.name, on_error='fail')

    def test_handle_preload_module_on_error_fail(self):
        """Test that module import failures raise with on_error='fail'."""
        with self.assertRaises(ModuleNotFoundError):
            forkserver._handle_preload(['nonexistent_test_module_xyz'], on_error='fail')

    def test_handle_preload_module_on_error_warn(self):
        """Test that module import failures warn with on_error='warn'."""
        with self.assertWarns(ImportWarning) as cm:
            forkserver._handle_preload(['nonexistent_test_module_xyz'], on_error='warn')
        self.assertIn("Failed to preload module", str(cm.warning))

    def test_handle_preload_module_on_error_ignore(self):
        """Test that module import failures are ignored with on_error='ignore'."""
        forkserver._handle_preload(['nonexistent_test_module_xyz'], on_error='ignore')

    def test_handle_preload_combined(self):
        """Test preloading both __main__ and modules."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.py') as f:
            f.write('import sys\n')
            f.flush()
            forkserver._handle_preload(['__main__', 'os', 'sys'], main_path=f.name, on_error='fail')


if __name__ == '__main__':
    unittest.main()
