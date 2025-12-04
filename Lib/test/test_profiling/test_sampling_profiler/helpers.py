"""Helper utilities for sampling profiler tests."""

import contextlib
import socket
import subprocess
import sys
import unittest
from collections import namedtuple

from test.support import SHORT_TIMEOUT
from test.support.socket_helper import find_unused_port
from test.support.os_helper import unlink


PROCESS_VM_READV_SUPPORTED = False

try:
    from _remote_debugging import PROCESS_VM_READV_SUPPORTED  # noqa: F401
    import _remote_debugging  # noqa: F401
except ImportError:
    raise unittest.SkipTest(
        "Test only runs when _remote_debugging is available"
    )
else:
    import profiling.sampling  # noqa: F401
    from profiling.sampling.sample import SampleProfiler  # noqa: F401


skip_if_not_supported = unittest.skipIf(
    (
        sys.platform != "darwin"
        and sys.platform != "linux"
        and sys.platform != "win32"
    ),
    "Test only runs on Linux, Windows and MacOS",
)

SubprocessInfo = namedtuple("SubprocessInfo", ["process", "socket"])


@contextlib.contextmanager
def test_subprocess(script):
    """Context manager to create a test subprocess with socket synchronization.

    Args:
        script: Python code to execute in the subprocess

    Yields:
        SubprocessInfo: Named tuple with process and socket objects
    """
    # Find an unused port for socket communication
    port = find_unused_port()

    # Inject socket connection code at the beginning of the script
    socket_code = f"""
import socket
_test_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
_test_sock.connect(('localhost', {port}))
_test_sock.sendall(b"ready")
"""

    # Combine socket code with user script
    full_script = socket_code + script

    # Create server socket to wait for process to be ready
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind(("localhost", port))
    server_socket.settimeout(SHORT_TIMEOUT)
    server_socket.listen(1)

    proc = subprocess.Popen(
        [sys.executable, "-c", full_script],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    client_socket = None
    try:
        # Wait for process to connect and send ready signal
        client_socket, _ = server_socket.accept()
        server_socket.close()
        response = client_socket.recv(1024)
        if response != b"ready":
            raise RuntimeError(
                f"Unexpected response from subprocess: {response!r}"
            )

        yield SubprocessInfo(proc, client_socket)
    finally:
        if client_socket is not None:
            client_socket.close()
        if proc.poll() is None:
            proc.kill()
        proc.wait()


def close_and_unlink(file):
    """Close a file and unlink it from the filesystem."""
    file.close()
    unlink(file.name)
