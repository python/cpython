"""
Child process monitoring for the sampling profiler.

This module monitors a target process for child process creation and spawns
separate profiler instances for each discovered child.
"""

import subprocess
import sys
import threading
import time

import _remote_debugging

# Polling interval for child process discovery
_CHILD_POLL_INTERVAL_SEC = 0.1

# Default timeout for waiting on child profilers
_DEFAULT_WAIT_TIMEOUT_SEC = 30.0

# Maximum number of child profilers to spawn (prevents resource exhaustion)
_MAX_CHILD_PROFILERS = 100

# Interval for cleaning up completed profilers (in polling cycles)
_CLEANUP_INTERVAL_CYCLES = 10


def get_child_pids(pid, recursive=True):
    """
    Get all child process IDs of the given process.

    Args:
        pid: Process ID of the parent process
        recursive: If True, return all descendants (children, grandchildren, etc.)

    Returns:
        List of child PIDs
    """
    return _remote_debugging.get_child_pids(pid, recursive=recursive)


def is_python_process(pid):
    """
    Check if a process is a Python process.

    Args:
        pid: Process ID to check

    Returns:
        bool: True if the process appears to be a Python process, False otherwise
    """
    return _remote_debugging.is_python_process(pid)


class ChildProcessMonitor:
    """
    Monitors a target process for child processes and spawns profilers for them.

    Use as a context manager:
        with ChildProcessMonitor(pid, cli_args, output_pattern) as monitor:
            # monitoring runs here
            monitor.wait_for_profilers()  # optional: wait before cleanup
        # cleanup happens automatically
    """

    def __init__(self, pid, cli_args, output_pattern):
        """
        Initialize the child process monitor.

        Args:
            pid: Parent process ID to monitor
            cli_args: CLI arguments to pass to child profilers
            output_pattern: Pattern for output files (format string with {pid})
        """
        self.parent_pid = pid
        self.cli_args = cli_args
        self.output_pattern = output_pattern

        self._known_children = set()
        self._spawned_profilers = []
        self._lock = threading.Lock()
        self._stop_event = threading.Event()
        self._monitor_thread = None
        self._poll_count = 0

    def __enter__(self):
        self._monitor_thread = threading.Thread(
            target=self._monitor_loop,
            daemon=True,
            name=f"child-monitor-{self.parent_pid}",
        )
        self._monitor_thread.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._stop_event.set()
        if self._monitor_thread is not None:
            self._monitor_thread.join(timeout=2.0)
            if self._monitor_thread.is_alive():
                print(
                    "Warning: Monitor thread did not stop cleanly",
                    file=sys.stderr,
                )

        # Wait for child profilers to complete naturally
        self.wait_for_profilers()

        # Terminate any remaining profilers
        with self._lock:
            profilers_to_cleanup = list(self._spawned_profilers)
            self._spawned_profilers.clear()

        for proc in profilers_to_cleanup:
            self._cleanup_process(proc)
        return False

    def _cleanup_process(self, proc, terminate_timeout=2.0, kill_timeout=1.0):
        if proc.poll() is not None:
            return  # Already terminated

        proc.terminate()
        try:
            proc.wait(timeout=terminate_timeout)
        except subprocess.TimeoutExpired:
            proc.kill()
            try:
                proc.wait(timeout=kill_timeout)
            except subprocess.TimeoutExpired:
                # Last resort: wait indefinitely to avoid zombie
                # SIGKILL should always work, but we must reap the process
                try:
                    proc.wait()
                except Exception:
                    pass

    @property
    def spawned_profilers(self):
        with self._lock:
            return list(self._spawned_profilers)

    def wait_for_profilers(self, timeout=_DEFAULT_WAIT_TIMEOUT_SEC):
        """
        Wait for all spawned child profilers to complete.

        Call this before exiting the context if you want profilers to finish
        their work naturally rather than being terminated.

        Args:
            timeout: Maximum time to wait in seconds
        """
        profilers = self.spawned_profilers
        if not profilers:
            return

        print(
            f"Waiting for {len(profilers)} child profiler(s) to complete...",
            file=sys.stderr,
        )

        deadline = time.monotonic() + timeout
        for proc in profilers:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            try:
                proc.wait(timeout=max(0.1, remaining))
            except subprocess.TimeoutExpired:
                pass

    def _monitor_loop(self):
        # Note: There is an inherent TOCTOU race between discovering a child
        # process and checking if it's Python. This is expected for process monitoring.
        while not self._stop_event.is_set():
            try:
                self._poll_count += 1

                # Periodically clean up completed profilers to avoid memory buildup
                if self._poll_count % _CLEANUP_INTERVAL_CYCLES == 0:
                    self._cleanup_completed_profilers()

                children = set(get_child_pids(self.parent_pid, recursive=True))

                with self._lock:
                    new_children = children - self._known_children
                    self._known_children.update(new_children)

                for child_pid in new_children:
                    # Only spawn profiler if this is actually a Python process
                    if is_python_process(child_pid):
                        self._spawn_profiler_for_child(child_pid)

            except ProcessLookupError:
                # Parent process exited, stop monitoring
                break
            except Exception as e:
                # Log error but continue monitoring
                print(
                    f"Warning: Error in child monitor loop: {e}",
                    file=sys.stderr,
                )

            self._stop_event.wait(timeout=_CHILD_POLL_INTERVAL_SEC)

    def _cleanup_completed_profilers(self):
        with self._lock:
            # Keep only profilers that are still running
            self._spawned_profilers = [
                p for p in self._spawned_profilers if p.poll() is None
            ]

    def _spawn_profiler_for_child(self, child_pid):
        if self._stop_event.is_set():
            return

        # Check if we've reached the maximum number of child profilers
        with self._lock:
            if len(self._spawned_profilers) >= _MAX_CHILD_PROFILERS:
                print(
                    f"Warning: Max child profilers ({_MAX_CHILD_PROFILERS}) reached, "
                    f"skipping PID {child_pid}",
                    file=sys.stderr,
                )
                return

        cmd = [
            sys.executable,
            "-m",
            "profiling.sampling",
            "attach",
            str(child_pid),
        ]
        cmd.extend(self._build_child_cli_args(child_pid))

        proc = None
        try:
            proc = subprocess.Popen(
                cmd,
                stdin=subprocess.DEVNULL,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            with self._lock:
                if self._stop_event.is_set():
                    self._cleanup_process(
                        proc, terminate_timeout=1.0, kill_timeout=1.0
                    )
                    return
                self._spawned_profilers.append(proc)

            print(
                f"Started profiler for child process {child_pid}",
                file=sys.stderr,
            )
        except Exception as e:
            if proc is not None:
                self._cleanup_process(
                    proc, terminate_timeout=1.0, kill_timeout=1.0
                )
            print(
                f"Warning: Failed to start profiler for child {child_pid}: {e}",
                file=sys.stderr,
            )

    def _build_child_cli_args(self, child_pid):
        args = list(self.cli_args)

        if self.output_pattern:
            # Use replace() instead of format() to handle user filenames with braces
            output_file = self.output_pattern.replace("{pid}", str(child_pid))
            found_output = False
            for i, arg in enumerate(args):
                if arg in ("-o", "--output") and i + 1 < len(args):
                    args[i + 1] = output_file
                    found_output = True
                    break
            if not found_output:
                args.extend(["-o", output_file])

        return args
