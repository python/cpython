"""Tests for forkserver preload functionality."""

import contextlib
import multiprocessing
import os
import shutil
import sys
import tempfile
import unittest
from multiprocessing import forkserver, spawn


class TestForkserverPreload(unittest.TestCase):
    """Tests for forkserver preload functionality."""

    def setUp(self):
        self._saved_warnoptions = sys.warnoptions.copy()
        # Remove warning options that would convert ImportWarning to errors:
        # - 'error' converts all warnings to errors
        # - 'error::ImportWarning' specifically converts ImportWarning
        # Keep other specific options like 'error::BytesWarning' that
        # subprocess's _args_from_interpreter_flags() expects to remove
        sys.warnoptions[:] = [
            opt for opt in sys.warnoptions
            if opt not in ('error', 'error::ImportWarning')
        ]
        self.ctx = multiprocessing.get_context('forkserver')
        forkserver._forkserver._stop()

    def tearDown(self):
        sys.warnoptions[:] = self._saved_warnoptions
        forkserver._forkserver._stop()

    @staticmethod
    def _send_value(conn, value):
        """Send value through connection. Static method to be picklable as Process target."""
        conn.send(value)

    @contextlib.contextmanager
    def capture_forkserver_stderr(self):
        """Capture stderr from forkserver by preloading a module that redirects it.

        Yields (module_name, capture_file_path). The capture file can be read
        after the forkserver has processed preloads. This works because
        forkserver.main() calls util._flush_std_streams() after preloading,
        ensuring captured output is written before we read it.
        """
        tmpdir = tempfile.mkdtemp()
        capture_module = os.path.join(tmpdir, '_capture_stderr.py')
        capture_file = os.path.join(tmpdir, 'stderr.txt')
        try:
            with open(capture_module, 'w') as f:
                # Use line buffering (buffering=1) to ensure warnings are written.
                # Enable ImportWarning since it's ignored by default.
                f.write(
                    f'import sys, warnings; '
                    f'sys.stderr = open({capture_file!r}, "w", buffering=1); '
                    f'warnings.filterwarnings("always", category=ImportWarning)\n'
                )
            sys.path.insert(0, tmpdir)
            yield '_capture_stderr', capture_file
        finally:
            sys.path.remove(tmpdir)
            shutil.rmtree(tmpdir, ignore_errors=True)

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
        with self.capture_forkserver_stderr() as (capture_mod, stderr_file):
            self.ctx.set_forkserver_preload(
                [capture_mod, 'nonexistent_module_xyz'], on_error='warn')

            r, w = self.ctx.Pipe(duplex=False)
            p = self.ctx.Process(target=self._send_value, args=(w, 123))
            p.start()
            w.close()
            result = r.recv()
            r.close()
            p.join()

            self.assertEqual(result, 123)
            self.assertEqual(p.exitcode, 0)

            with open(stderr_file) as f:
                stderr_output = f.read()
            self.assertIn('nonexistent_module_xyz', stderr_output)
            self.assertIn('ImportWarning', stderr_output)

    def test_preload_on_error_fail_breaks_context(self):
        """Test that invalid modules with on_error='fail' breaks the forkserver."""
        with self.capture_forkserver_stderr() as (capture_mod, stderr_file):
            self.ctx.set_forkserver_preload(
                [capture_mod, 'nonexistent_module_xyz'], on_error='fail')

            r, w = self.ctx.Pipe(duplex=False)
            try:
                p = self.ctx.Process(target=self._send_value, args=(w, 42))
                with self.assertRaises((EOFError, ConnectionError, BrokenPipeError)) as cm:
                    p.start()
                notes = getattr(cm.exception, '__notes__', [])
                self.assertTrue(notes, "Expected exception to have __notes__")
                self.assertIn('Forkserver process may have crashed', notes[0])

                with open(stderr_file) as f:
                    stderr_output = f.read()
                self.assertIn('nonexistent_module_xyz', stderr_output)
                self.assertIn('ModuleNotFoundError', stderr_output)
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

    def setUp(self):
        self._saved_main = sys.modules['__main__']

    def tearDown(self):
        spawn.old_main_modules.clear()
        sys.modules['__main__'] = self._saved_main

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
