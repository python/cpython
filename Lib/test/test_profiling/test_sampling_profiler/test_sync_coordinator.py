"""Tests for the sampling profiler's synchronization coordinator."""

import os
import socket
import subprocess
import sys
import tempfile
import unittest

from test.support import SHORT_TIMEOUT, os_helper, requires_subprocess


@requires_subprocess()
class TestSyncCoordinatorScriptExecution(unittest.TestCase):
    """Tests for how the coordinator executes a target script."""

    def run_coordinator(self, cwd, target, *target_args):
        """Execute *target* from *cwd* with the sync coordinator.

        Returns the ``(stdout, stderr)`` captured from the coordinator.
        """
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind(("127.0.0.1", 0))
            server.listen(1)
            server.settimeout(SHORT_TIMEOUT)
            port = server.getsockname()[1]

            cmd = (
                sys.executable,
                "-m",
                "profiling.sampling._sync_coordinator",
                str(port),
                cwd,
                target,
            ) + tuple(target_args)
            process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            try:
                conn, _ = server.accept()
                with conn:
                    # _signal_readiness() sends b"ready" before running target.
                    self.assertEqual(conn.recv(64), b"ready")
                stdout, stderr = process.communicate(timeout=SHORT_TIMEOUT)
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()
        return stdout, stderr

    def test_script_directory_on_sys_path(self):
        # gh-158540: The coordinator must put the directory containing the
        # script (not the working directory) on sys.path, matching
        # ``python script.py``, so that modules next to the script can be
        # imported even when the coordinator runs from another directory.
        with tempfile.TemporaryDirectory() as tmpdir:
            script_dir = os.path.join(tmpdir, "sub")
            os.mkdir(script_dir)
            with open(os.path.join(script_dir, "helper.py"), "w") as f:
                f.write("message = 'helper imported'\n")
            with open(os.path.join(script_dir, "where.py"), "w") as f:
                f.write(
                    "import os\n"
                    "import sys\n"
                    "print('PATH0:', os.path.realpath(sys.path[0]))\n"
                    "import helper\n"
                    "print('HELPER:', helper.message)\n"
                )

            stdout, stderr = self.run_coordinator(
                tmpdir, os.path.join("sub", "where.py")
            )

        self.assertNotIn("ModuleNotFoundError", stderr)
        self.assertIn("HELPER: helper imported", stdout)
        self.assertIn(f"PATH0: {os.path.realpath(script_dir)}", stdout)

    @os_helper.skip_unless_symlink
    def test_symlinked_script_uses_real_directory(self):
        # gh-158540: ``python script.py`` resolves symlinks when computing
        # sys.path[0], so a symlinked script must import modules next to the
        # real script, not next to the link.
        with tempfile.TemporaryDirectory() as tmpdir:
            script_dir = os.path.join(tmpdir, "sub")
            os.mkdir(script_dir)
            with open(os.path.join(script_dir, "helper.py"), "w") as f:
                f.write("message = 'helper imported'\n")
            with open(os.path.join(script_dir, "where.py"), "w") as f:
                f.write(
                    "import os\n"
                    "import sys\n"
                    "print('PATH0:', os.path.realpath(sys.path[0]))\n"
                    "import helper\n"
                    "print('HELPER:', helper.message)\n"
                )
            os.symlink(
                os.path.join("sub", "where.py"),
                os.path.join(tmpdir, "link.py"),
            )

            stdout, stderr = self.run_coordinator(tmpdir, "link.py")

        self.assertNotIn("ModuleNotFoundError", stderr)
        self.assertIn("HELPER: helper imported", stdout)
        self.assertIn(f"PATH0: {os.path.realpath(script_dir)}", stdout)


if __name__ == "__main__":
    unittest.main()
