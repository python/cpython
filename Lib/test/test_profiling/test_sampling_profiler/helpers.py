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


def _wait_for_signal(sock, expected_signals, timeout=SHORT_TIMEOUT):
    """
    Wait for expected signal(s) from a socket with proper timeout and EOF handling.

    Args:
        sock: Connected socket to read from
        expected_signals: Single bytes object or list of bytes objects to wait for
        timeout: Socket timeout in seconds

    Returns:
        bytes: Complete accumulated response buffer

    Raises:
        RuntimeError: If connection closed before signal received or timeout
    """
    if isinstance(expected_signals, bytes):
        expected_signals = [expected_signals]

    sock.settimeout(timeout)
    buffer = b""

    while True:
        # Check if all expected signals are in buffer
        if all(sig in buffer for sig in expected_signals):
            return buffer

        try:
            chunk = sock.recv(4096)
            if not chunk:
                raise RuntimeError(
                    f"Connection closed before receiving expected signals. "
                    f"Expected: {expected_signals}, Got: {buffer[-200:]!r}"
                )
            buffer += chunk
        except socket.timeout:
            raise RuntimeError(
                f"Timeout waiting for signals. "
                f"Expected: {expected_signals}, Got: {buffer[-200:]!r}"
            ) from None
        except OSError as e:
            raise RuntimeError(
                f"Socket error while waiting for signals: {e}. "
                f"Expected: {expected_signals}, Got: {buffer[-200:]!r}"
            ) from None


def _cleanup_sockets(*sockets):
    """Safely close multiple sockets, ignoring errors."""
    for sock in sockets:
        if sock is not None:
            try:
                sock.close()
            except OSError:
                pass


def _cleanup_process(proc, timeout=SHORT_TIMEOUT):
    """Terminate a process gracefully, escalating to kill if needed."""
    if proc.poll() is not None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=timeout)
        return
    except subprocess.TimeoutExpired:
        pass
    proc.kill()
    try:
        proc.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        pass  # Process refuses to die, nothing more we can do


@contextlib.contextmanager
def test_subprocess(script, wait_for_working=False):
    """Context manager to create a test subprocess with socket synchronization.

    Args:
        script: Python code to execute in the subprocess. If wait_for_working
                is True, script should send b"working" after starting work.
        wait_for_working: If True, wait for both "ready" and "working" signals.
                         Default False for backward compatibility.

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
        server_socket = None

        # Wait for ready signal, and optionally working signal
        if wait_for_working:
            _wait_for_signal(client_socket, [b"ready", b"working"])
        else:
            _wait_for_signal(client_socket, b"ready")

        yield SubprocessInfo(proc, client_socket)
    finally:
        _cleanup_sockets(client_socket, server_socket)
        _cleanup_process(proc)


def close_and_unlink(file):
    """Close a file and unlink it from the filesystem."""
    file.close()
    unlink(file.name)
