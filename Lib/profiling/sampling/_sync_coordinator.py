"""
Internal synchronization coordinator for the sample profiler.

This module is used internally by the sample profiler to coordinate
the startup of target processes. It should not be called directly by users.
"""

import os
import sys
import socket
import runpy
import time
from typing import List, NoReturn


class CoordinatorError(Exception):
    """Base exception for coordinator errors."""
    pass


class ArgumentError(CoordinatorError):
    """Raised when invalid arguments are provided."""
    pass


class SyncError(CoordinatorError):
    """Raised when synchronization with profiler fails."""
    pass


class TargetError(CoordinatorError):
    """Raised when target execution fails."""
    pass


def _validate_arguments(args: List[str]) -> tuple[int, str, List[str]]:
    """
    Validate and parse command line arguments.

    Args:
        args: Command line arguments including script name

    Returns:
        Tuple of (sync_port, working_directory, target_args)

    Raises:
        ArgumentError: If arguments are invalid
    """
    if len(args) < 4:
        raise ArgumentError(
            "Insufficient arguments. Expected: <sync_port> <cwd> <target> [args...]"
        )

    try:
        sync_port = int(args[1])
        if not (1 <= sync_port <= 65535):
            raise ValueError("Port out of range")
    except ValueError as e:
        raise ArgumentError(f"Invalid sync port '{args[1]}': {e}") from e

    cwd = args[2]
    if not os.path.isdir(cwd):
        raise ArgumentError(f"Working directory does not exist: {cwd}")

    target_args = args[3:]
    if not target_args:
        raise ArgumentError("No target specified")

    return sync_port, cwd, target_args


# Constants for socket communication
_MAX_RETRIES = 3
_INITIAL_RETRY_DELAY = 0.1
_SOCKET_TIMEOUT = 2.0
_READY_MESSAGE = b"ready"


def _signal_readiness(sync_port: int) -> None:
    """
    Signal readiness to the profiler via TCP socket.

    Args:
        sync_port: Port number where profiler is listening

    Raises:
        SyncError: If unable to signal readiness
    """
    last_error = None

    for attempt in range(_MAX_RETRIES):
        try:
            # Use context manager for automatic cleanup
            with socket.create_connection(("127.0.0.1", sync_port), timeout=_SOCKET_TIMEOUT) as sock:
                sock.send(_READY_MESSAGE)
                return
        except (socket.error, OSError) as e:
            last_error = e
            if attempt < _MAX_RETRIES - 1:
                # Exponential backoff before retry
                time.sleep(_INITIAL_RETRY_DELAY * (2 ** attempt))

    # If we get here, all retries failed
    raise SyncError(f"Failed to signal readiness after {_MAX_RETRIES} attempts: {last_error}") from last_error


def _setup_environment(cwd: str) -> None:
    """
    Set up the execution environment.

    Args:
        cwd: Working directory to change to

    Raises:
        TargetError: If unable to set up environment
    """
    try:
        os.chdir(cwd)
    except OSError as e:
        raise TargetError(f"Failed to change to directory {cwd}: {e}") from e

    # Add current directory to sys.path if not present (for module imports)
    if cwd not in sys.path:
        sys.path.insert(0, cwd)


def _execute_module(module_name: str, module_args: List[str]) -> None:
    """
    Execute a Python module.

    Args:
        module_name: Name of the module to execute
        module_args: Arguments to pass to the module

    Raises:
        TargetError: If module execution fails
    """
    # Replace sys.argv to match how Python normally runs modules
    # When running 'python -m module args', sys.argv is ["__main__.py", "args"]
    sys.argv = [f"__main__.py"] + module_args

    try:
        runpy.run_module(module_name, run_name="__main__", alter_sys=True)
    except ImportError as e:
        raise TargetError(f"Module '{module_name}' not found: {e}") from e
    except SystemExit:
        # SystemExit is normal for modules
        pass
    except Exception as e:
        raise TargetError(f"Error executing module '{module_name}': {e}") from e


def _execute_script(script_path: str, script_args: List[str], cwd: str) -> None:
    """
    Execute a Python script.

    Args:
        script_path: Path to the script to execute
        script_args: Arguments to pass to the script
        cwd: Current working directory for path resolution

    Raises:
        TargetError: If script execution fails
    """
    # Make script path absolute if it isn't already
    if not os.path.isabs(script_path):
        script_path = os.path.join(cwd, script_path)

    if not os.path.isfile(script_path):
        raise TargetError(f"Script not found: {script_path}")

    # Replace sys.argv to match original script call
    sys.argv = [script_path] + script_args

    try:
        with open(script_path, 'rb') as f:
            source_code = f.read()

        # Compile and execute the script
        code = compile(source_code, script_path, 'exec')
        exec(code, {'__name__': '__main__', '__file__': script_path})
    except FileNotFoundError as e:
        raise TargetError(f"Script file not found: {script_path}") from e
    except PermissionError as e:
        raise TargetError(f"Permission denied reading script: {script_path}") from e
    except SyntaxError as e:
        raise TargetError(f"Syntax error in script {script_path}: {e}") from e
    except SystemExit:
        # SystemExit is normal for scripts
        pass
    except Exception as e:
        raise TargetError(f"Error executing script '{script_path}': {e}") from e


def main() -> NoReturn:
    """
    Main coordinator function.

    This function coordinates the startup of a target Python process
    with the sample profiler by signaling when the process is ready
    to be profiled.
    """
    try:
        # Parse and validate arguments
        sync_port, cwd, target_args = _validate_arguments(sys.argv)

        # Set up execution environment
        _setup_environment(cwd)

        # Signal readiness to profiler
        _signal_readiness(sync_port)

        # Execute the target
        if target_args[0] == "-m":
            # Module execution
            if len(target_args) < 2:
                raise ArgumentError("Module name required after -m")

            module_name = target_args[1]
            module_args = target_args[2:]
            _execute_module(module_name, module_args)
        else:
            # Script execution
            script_path = target_args[0]
            script_args = target_args[1:]
            _execute_script(script_path, script_args, cwd)

    except CoordinatorError as e:
        print(f"Profiler coordinator error: {e}", file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        print("Interrupted", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Unexpected error in profiler coordinator: {e}", file=sys.stderr)
        sys.exit(1)

    # Normal exit
    sys.exit(0)


if __name__ == "__main__":
    main()
