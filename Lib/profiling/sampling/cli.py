"""Command-line interface for the sampling profiler."""

import argparse
import importlib.util
import locale
import os
import re
import selectors
import socket
import subprocess
import sys
import time
import webbrowser
from contextlib import nullcontext

from .errors import SamplingUnknownProcessError, SamplingModuleNotFoundError, SamplingScriptNotFoundError
from .sample import sample, sample_live, _is_process_running
from .pstats_collector import PstatsCollector
from .stack_collector import CollapsedStackCollector, FlamegraphCollector
from .heatmap_collector import HeatmapCollector
from .gecko_collector import GeckoCollector
from .binary_collector import BinaryCollector
from .binary_reader import BinaryReader
from .constants import (
    MICROSECONDS_PER_SECOND,
    PROFILING_MODE_ALL,
    PROFILING_MODE_WALL,
    PROFILING_MODE_CPU,
    PROFILING_MODE_GIL,
    PROFILING_MODE_EXCEPTION,
    SORT_MODE_NSAMPLES,
    SORT_MODE_TOTTIME,
    SORT_MODE_CUMTIME,
    SORT_MODE_SAMPLE_PCT,
    SORT_MODE_CUMUL_PCT,
    SORT_MODE_NSAMPLES_CUMUL,
)

try:
    from ._child_monitor import ChildProcessMonitor
except ImportError:
    # _remote_debugging module not available on this platform (e.g., WASI)
    ChildProcessMonitor = None

try:
    from .live_collector import LiveStatsCollector
except ImportError:
    LiveStatsCollector = None


class CustomFormatter(
    argparse.ArgumentDefaultsHelpFormatter,
    argparse.RawDescriptionHelpFormatter,
):
    """Custom formatter that combines default values display with raw description formatting."""
    pass


_HELP_DESCRIPTION = """Sample a process's stack frames and generate profiling data.

Examples:
  # Run and profile a script
  `python -m profiling.sampling run script.py arg1 arg2`

  # Attach to a running process
  `python -m profiling.sampling attach 1234`

  # Live interactive mode for a script
  `python -m profiling.sampling run --live script.py`

  # Live interactive mode for a running process
  `python -m profiling.sampling attach --live 1234`

Use `python -m profiling.sampling <command> --help` for command-specific help."""


# Constants for socket synchronization
_SYNC_TIMEOUT_SEC = 5.0
_PROCESS_KILL_TIMEOUT_SEC = 2.0
_READY_MESSAGE = b"ready"
_RECV_BUFFER_SIZE = 1024

# Format configuration
FORMAT_EXTENSIONS = {
    "pstats": "pstats",
    "collapsed": "txt",
    "flamegraph": "html",
    "gecko": "json",
    "heatmap": "html",
    "binary": "bin",
}

COLLECTOR_MAP = {
    "pstats": PstatsCollector,
    "collapsed": CollapsedStackCollector,
    "flamegraph": FlamegraphCollector,
    "gecko": GeckoCollector,
    "heatmap": HeatmapCollector,
    "binary": BinaryCollector,
}

def _setup_child_monitor(args, parent_pid):
    # Build CLI args for child profilers (excluding --subprocesses to avoid recursion)
    child_cli_args = _build_child_profiler_args(args)

    # Build output pattern
    output_pattern = _build_output_pattern(args)

    return ChildProcessMonitor(
        pid=parent_pid,
        cli_args=child_cli_args,
        output_pattern=output_pattern,
    )


def _get_child_monitor_context(args, pid):
    if getattr(args, 'subprocesses', False):
        return _setup_child_monitor(args, pid)
    return nullcontext()


def _build_child_profiler_args(args):
    child_args = []

    # Sampling options
    hz = MICROSECONDS_PER_SECOND // args.sample_interval_usec
    child_args.extend(["-r", str(hz)])
    if args.duration is not None:
        child_args.extend(["-d", str(args.duration)])
    if args.all_threads:
        child_args.append("-a")
    if args.realtime_stats:
        child_args.append("--realtime-stats")
    if args.native:
        child_args.append("--native")
    if not args.gc:
        child_args.append("--no-gc")
    if args.opcodes:
        child_args.append("--opcodes")
    if args.async_aware:
        child_args.append("--async-aware")
        async_mode = getattr(args, 'async_mode', 'running')
        if async_mode != "running":
            child_args.extend(["--async-mode", async_mode])

    # Mode options
    mode = getattr(args, 'mode', 'wall')
    if mode != "wall":
        child_args.extend(["--mode", mode])

    # Format options (skip pstats as it's the default)
    if args.format != "pstats":
        child_args.append(f"--{args.format}")

    return child_args


def _build_output_pattern(args):
    """Build output filename pattern for child profilers.

    The pattern uses {pid} as a placeholder which will be replaced with the
    actual child PID using str.replace(), so user filenames with braces are safe.
    """
    if args.outfile:
        # User specified output - add PID to filename
        base, ext = os.path.splitext(args.outfile)
        if ext:
            return f"{base}_{{pid}}{ext}"
        else:
            return f"{args.outfile}_{{pid}}"
    else:
        # Use default pattern based on format (consistent _ separator)
        extension = FORMAT_EXTENSIONS.get(args.format, "txt")
        if args.format == "heatmap":
            return "heatmap_{pid}"
        if args.format == "pstats":
            # pstats defaults to stdout, but for subprocesses we need files
            return "profile_{pid}.pstats"
        return f"{args.format}_{{pid}}.{extension}"


def _parse_mode(mode_string):
    """Convert mode string to mode constant."""
    mode_map = {
        "wall": PROFILING_MODE_WALL,
        "cpu": PROFILING_MODE_CPU,
        "gil": PROFILING_MODE_GIL,
        "exception": PROFILING_MODE_EXCEPTION,
    }
    return mode_map[mode_string]


def _check_process_died(process):
    """Check if process died and raise an error with stderr if available."""
    if process.poll() is None:
        return

    # Process died - try to get stderr for error message
    stderr_msg = ""
    if process.stderr:
        try:
            stderr_msg = process.stderr.read().decode().strip()
        except (OSError, UnicodeDecodeError):
            pass

    if stderr_msg:
        raise RuntimeError(stderr_msg)
    raise RuntimeError(f"Process exited with code {process.returncode}")


def _wait_for_ready_signal(sync_sock, process, timeout):
    """Wait for the ready signal from the subprocess, checking for early death."""
    deadline = time.monotonic() + timeout
    sel = selectors.DefaultSelector()
    sel.register(sync_sock, selectors.EVENT_READ)

    try:
        while True:
            _check_process_died(process)

            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise socket.timeout("timed out")

            if not sel.select(timeout=min(0.1, remaining)):
                continue

            conn, _ = sync_sock.accept()
            try:
                ready_signal = conn.recv(_RECV_BUFFER_SIZE)
            finally:
                conn.close()

            if ready_signal != _READY_MESSAGE:
                raise RuntimeError(f"Invalid ready signal received: {ready_signal!r}")
            return
    finally:
        sel.close()


def _run_with_sync(original_cmd, suppress_output=False):
    """Run a command with socket-based synchronization and return the process."""
    # Create a TCP socket for synchronization with better socket options
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sync_sock:
        # Set SO_REUSEADDR to avoid "Address already in use" errors
        sync_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sync_sock.bind(("127.0.0.1", 0))  # Let OS choose a free port
        sync_port = sync_sock.getsockname()[1]
        sync_sock.listen(1)
        sync_sock.settimeout(_SYNC_TIMEOUT_SEC)

        # Get current working directory to preserve it
        cwd = os.getcwd()

        # Build command using the sync coordinator
        target_args = original_cmd[1:]  # Remove python executable
        cmd = (
            sys.executable,
            "-m",
            "profiling.sampling._sync_coordinator",
            str(sync_port),
            cwd,
        ) + tuple(target_args)

        # Start the process with coordinator
        # When suppress_output=True (live mode), capture stderr so we can
        # report errors if the process dies before signaling ready.
        # When suppress_output=False (normal mode), let stderr inherit so
        # script errors print to the terminal.
        popen_kwargs = {}
        if suppress_output:
            popen_kwargs["stdin"] = subprocess.DEVNULL
            popen_kwargs["stdout"] = subprocess.DEVNULL
            popen_kwargs["stderr"] = subprocess.PIPE

        process = subprocess.Popen(cmd, **popen_kwargs)

        try:
            _wait_for_ready_signal(sync_sock, process, _SYNC_TIMEOUT_SEC)
        except socket.timeout:
            # If we timeout, kill the process and raise an error
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=_PROCESS_KILL_TIMEOUT_SEC)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            raise RuntimeError(
                "Process failed to signal readiness within timeout"
            )

        return process


_RATE_PATTERN = re.compile(r'''
      ^                   # Start of string
      (                   # Group 1: The numeric value
          \d+             #   One or more digits (integer part)
          (?:\.\d+)?      #   Optional: decimal point followed by digits
      )                   #   Examples: "10", "0.5", "100.25"
      (                   # Group 2: Optional unit suffix
          hz              #   "hz" - hertz
          | khz           #   "khz" - kilohertz
          | k             #   "k" - shorthand for kilohertz
      )?                  #   Suffix is optional (bare number = Hz)
      $                   # End of string
  ''', re.VERBOSE | re.IGNORECASE)


def _parse_sampling_rate(rate_str: str) -> int:
    """Parse sampling rate string to microseconds."""
    rate_str = rate_str.strip().lower()

    match = _RATE_PATTERN.match(rate_str)
    if not match:
        raise argparse.ArgumentTypeError(
            f"Invalid sampling rate format: {rate_str}. "
            "Expected: number followed by optional suffix (hz, khz, k) with no spaces (e.g., 10khz)"
        )

    number_part = match.group(1)
    suffix = match.group(2) or ''

    # Determine multiplier based on suffix
    suffix_map = {
        'hz': 1,
        'khz': 1000,
        'k': 1000,
    }
    multiplier = suffix_map.get(suffix, 1)
    hz = float(number_part) * multiplier
    if hz <= 0:
        raise argparse.ArgumentTypeError(f"Sampling rate must be positive: {rate_str}")

    interval_usec = int(MICROSECONDS_PER_SECOND / hz)
    if interval_usec < 1:
        raise argparse.ArgumentTypeError(f"Sampling rate too high: {rate_str}")

    return interval_usec


def _add_sampling_options(parser):
    """Add sampling configuration options to a parser."""
    sampling_group = parser.add_argument_group("Sampling configuration")
    sampling_group.add_argument(
        "-r",
        "--sampling-rate",
        type=_parse_sampling_rate,
        default="1khz",
        metavar="RATE",
        dest="sample_interval_usec",
        help="sampling rate (e.g., 10000, 10khz, 10k)",
    )
    sampling_group.add_argument(
        "-d",
        "--duration",
        type=int,
        default=None,
        metavar="SECONDS",
        help="Sampling duration (default: run to completion)",
    )
    sampling_group.add_argument(
        "-a",
        "--all-threads",
        action="store_true",
        help="Sample all threads in the process instead of just the main thread",
    )
    sampling_group.add_argument(
        "--realtime-stats",
        action="store_true",
        help="Print real-time sampling statistics (Hz, mean, min, max) during profiling",
    )
    sampling_group.add_argument(
        "--native",
        action="store_true",
        help='Include artificial "<native>" frames to denote calls to non-Python code',
    )
    sampling_group.add_argument(
        "--no-gc",
        action="store_false",
        dest="gc",
        help='Don\'t include artificial "<GC>" frames to denote active garbage collection',
    )
    sampling_group.add_argument(
        "--opcodes",
        action="store_true",
        help="Gather bytecode opcode information for instruction-level profiling "
        "(shows which bytecode instructions are executing, including specializations).",
    )
    sampling_group.add_argument(
        "--async-aware",
        action="store_true",
        help="Enable async-aware profiling (uses task-based stack reconstruction)",
    )
    sampling_group.add_argument(
        "--subprocesses",
        action="store_true",
        help="Also profile subprocesses. Each subprocess gets its own profiler and output file.",
    )
    sampling_group.add_argument(
        "--blocking",
        action="store_true",
        help="Stop all threads in target process before sampling to get consistent snapshots. "
        "Uses thread_suspend on macOS and ptrace on Linux. Adds overhead but ensures memory "
        "reads are from a frozen state.",
    )


def _add_mode_options(parser):
    """Add mode options to a parser."""
    mode_group = parser.add_argument_group("Mode options")
    mode_group.add_argument(
        "--mode",
        choices=["wall", "cpu", "gil", "exception"],
        default="wall",
        help="Sampling mode: wall (all samples), cpu (only samples when thread is on CPU), "
        "gil (only samples when thread holds the GIL), "
        "exception (only samples when thread has an active exception). "
        "Incompatible with --async-aware",
    )
    mode_group.add_argument(
        "--async-mode",
        choices=["running", "all"],
        default="running",
        help='Async profiling mode: "running" (only running task) '
        'or "all" (all tasks including waiting). Requires --async-aware',
    )


def _add_format_options(parser, include_compression=True, include_binary=True):
    """Add output format options to a parser."""
    output_group = parser.add_argument_group("Output options")
    format_group = output_group.add_mutually_exclusive_group()
    format_group.add_argument(
        "--pstats",
        action="store_const",
        const="pstats",
        dest="format",
        help="Generate pstats output (default)",
    )
    format_group.add_argument(
        "--collapsed",
        action="store_const",
        const="collapsed",
        dest="format",
        help="Generate collapsed stack traces for flamegraphs",
    )
    format_group.add_argument(
        "--flamegraph",
        action="store_const",
        const="flamegraph",
        dest="format",
        help="Generate interactive HTML flamegraph visualization",
    )
    format_group.add_argument(
        "--gecko",
        action="store_const",
        const="gecko",
        dest="format",
        help="Generate Gecko format for Firefox Profiler",
    )
    format_group.add_argument(
        "--heatmap",
        action="store_const",
        const="heatmap",
        dest="format",
        help="Generate interactive HTML heatmap visualization with line-level sample counts",
    )
    if include_binary:
        format_group.add_argument(
            "--binary",
            action="store_const",
            const="binary",
            dest="format",
            help="Generate high-performance binary format (use 'replay' command to convert)",
        )
    parser.set_defaults(format="pstats")

    if include_compression:
        output_group.add_argument(
            "--compression",
            choices=["auto", "zstd", "none"],
            default="auto",
            help="Compression for binary format: auto (use zstd if available), zstd, none",
        )

    output_group.add_argument(
        "-o",
        "--output",
        dest="outfile",
        help="Output path (default: stdout for pstats text; with -o, pstats is binary). "
        "Auto-generated for other formats. For heatmap: directory name (default: heatmap_PID)",
    )
    output_group.add_argument(
        "--browser",
        action="store_true",
        help="Automatically open HTML output (flamegraph, heatmap) in browser. "
        "When using --subprocesses, only the main process opens the browser",
    )


def _add_pstats_options(parser):
    """Add pstats-specific display options to a parser."""
    pstats_group = parser.add_argument_group("pstats format options")
    pstats_group.add_argument(
        "--sort",
        choices=[
            "nsamples",
            "tottime",
            "cumtime",
            "sample-pct",
            "cumul-pct",
            "nsamples-cumul",
            "name",
        ],
        default=None,
        help="Sort order for pstats output (default: nsamples)",
    )
    pstats_group.add_argument(
        "-l",
        "--limit",
        type=int,
        default=None,
        help="Limit the number of rows in the output (default: 15)",
    )
    pstats_group.add_argument(
        "--no-summary",
        action="store_true",
        help="Disable the summary section in the pstats output",
    )


def _sort_to_mode(sort_choice):
    """Convert sort choice string to SORT_MODE constant."""
    sort_map = {
        "nsamples": SORT_MODE_NSAMPLES,
        "tottime": SORT_MODE_TOTTIME,
        "cumtime": SORT_MODE_CUMTIME,
        "sample-pct": SORT_MODE_SAMPLE_PCT,
        "cumul-pct": SORT_MODE_CUMUL_PCT,
        "nsamples-cumul": SORT_MODE_NSAMPLES_CUMUL,
        "name": -1,
    }
    return sort_map.get(sort_choice, SORT_MODE_NSAMPLES)

def _create_collector(format_type, sample_interval_usec, skip_idle, opcodes=False,
                      output_file=None, compression='auto'):
    """Create the appropriate collector based on format type.

    Args:
        format_type: The output format ('pstats', 'collapsed', 'flamegraph', 'gecko', 'heatmap', 'binary')
        sample_interval_usec: Sampling interval in microseconds
        skip_idle: Whether to skip idle samples
        opcodes: Whether to collect opcode information (only used by gecko format
                 for creating interval markers in Firefox Profiler)
        output_file: Output file path (required for binary format)
        compression: Compression type for binary format ('auto', 'zstd', 'none')

    Returns:
        A collector instance of the appropriate type
    """
    collector_class = COLLECTOR_MAP.get(format_type)
    if collector_class is None:
        raise ValueError(f"Unknown format: {format_type}")

    # Binary format requires output file and compression
    if format_type == "binary":
        if output_file is None:
            raise ValueError("Binary format requires an output file")
        return collector_class(output_file, sample_interval_usec, skip_idle=skip_idle,
                              compression=compression)

    # Gecko format never skips idle (it needs both GIL and CPU data)
    # and is the only format that uses opcodes for interval markers
    if format_type == "gecko":
        skip_idle = False
        return collector_class(sample_interval_usec, skip_idle=skip_idle, opcodes=opcodes)

    return collector_class(sample_interval_usec, skip_idle=skip_idle)


def _generate_output_filename(format_type, pid):
    """Generate output filename based on format and PID.

    Args:
        format_type: The output format
        pid: Process ID

    Returns:
        Generated filename
    """
    extension = FORMAT_EXTENSIONS.get(format_type, "txt")
    # For heatmap, use cleaner directory name without extension
    if format_type == "heatmap":
        return f"heatmap_{pid}"
    return f"{format_type}_{pid}.{extension}"


def _open_in_browser(path):
    """Open a file or directory in the default web browser.

    Args:
        path: File path or directory path to open

    For directories (heatmap), opens the index.html file inside.
    """
    abs_path = os.path.abspath(path)

    # For heatmap directories, open the index.html file
    if os.path.isdir(abs_path):
        index_path = os.path.join(abs_path, 'index.html')
        if os.path.exists(index_path):
            abs_path = index_path
        else:
            print(f"Warning: Could not find index.html in {path}", file=sys.stderr)
            return

    file_url = f"file://{abs_path}"
    try:
        webbrowser.open(file_url)
    except Exception as e:
        print(f"Warning: Could not open browser: {e}", file=sys.stderr)


def _handle_output(collector, args, pid, mode):
    """Handle output for the collector based on format and arguments.

    Args:
        collector: The collector instance with profiling data
        args: Parsed command-line arguments
        pid: Process ID (for generating filenames)
        mode: Profiling mode used
    """
    if args.format == "binary":
        # Binary format already wrote to file incrementally, just finalize
        collector.export(None)
        filename = collector.filename
        print(f"Binary profile written to {filename} ({collector.total_samples} samples)")
    elif args.format == "pstats":
        if args.outfile:
            # If outfile is a directory, generate filename inside it
            if os.path.isdir(args.outfile):
                filename = os.path.join(args.outfile, _generate_output_filename(args.format, pid))
                collector.export(filename)
            else:
                collector.export(args.outfile)
        else:
            # Print to stdout with defaults applied
            sort_choice = args.sort if args.sort is not None else "nsamples"
            limit = args.limit if args.limit is not None else 15
            sort_mode = _sort_to_mode(sort_choice)
            collector.print_stats(
                sort_mode, limit, not args.no_summary, mode
            )
    else:
        # Export to file
        if args.outfile and os.path.isdir(args.outfile):
            # If outfile is a directory, generate filename inside it
            filename = os.path.join(args.outfile, _generate_output_filename(args.format, pid))
        else:
            filename = args.outfile or _generate_output_filename(args.format, pid)
        collector.export(filename)

        # Auto-open browser for HTML output if --browser flag is set
        if args.format in ('flamegraph', 'heatmap') and getattr(args, 'browser', False):
            _open_in_browser(filename)


def _validate_args(args, parser):
    """Validate format-specific options and live mode requirements.

    Args:
        args: Parsed command-line arguments
        parser: ArgumentParser instance for error reporting
    """
    # Replay command has no special validation needed
    if getattr(args, 'command', None) == "replay":
        return

    # Warn about blocking mode with aggressive sampling intervals
    if args.blocking and args.sample_interval_usec < 100:
        print(
            f"Warning: --blocking with a {args.sample_interval_usec} µs interval will stop all threads "
            f"{1_000_000 // args.sample_interval_usec} times per second. "
            "Consider using --sampling-rate 1khz or lower to reduce overhead.",
            file=sys.stderr
        )

    # Check if live mode is available
    if hasattr(args, 'live') and args.live and LiveStatsCollector is None:
        parser.error(
            "Live mode requires the curses module, which is not available."
        )

    # --subprocesses is incompatible with --live
    if hasattr(args, 'subprocesses') and args.subprocesses:
        if ChildProcessMonitor is None:
            parser.error(
                "--subprocesses is not available on this platform "
                "(requires _remote_debugging module)."
            )
        if hasattr(args, 'live') and args.live:
            parser.error("--subprocesses is incompatible with --live mode.")

    # Async-aware mode is incompatible with --native, --no-gc, --mode, and --all-threads
    if getattr(args, 'async_aware', False):
        issues = []
        if args.native:
            issues.append("--native")
        if not args.gc:
            issues.append("--no-gc")
        if hasattr(args, 'mode') and args.mode != "wall":
            issues.append(f"--mode={args.mode}")
        if hasattr(args, 'all_threads') and args.all_threads:
            issues.append("--all-threads")
        if issues:
            parser.error(
                f"Options {', '.join(issues)} are incompatible with --async-aware. "
                "Async-aware profiling uses task-based stack reconstruction."
            )

    # --async-mode requires --async-aware
    if hasattr(args, 'async_mode') and args.async_mode != "running" and not getattr(args, 'async_aware', False):
        parser.error("--async-mode requires --async-aware to be enabled.")

    # Live mode is incompatible with format options
    if hasattr(args, 'live') and args.live:
        if args.format != "pstats":
            format_flag = f"--{args.format}"
            parser.error(
                f"--live is incompatible with {format_flag}. Live mode uses a TUI interface."
            )

        # Live mode is also incompatible with pstats-specific options
        issues = []
        if args.sort is not None:
            issues.append("--sort")
        if args.limit is not None:
            issues.append("--limit")
        if args.no_summary:
            issues.append("--no-summary")

        if issues:
            parser.error(
                f"Options {', '.join(issues)} are incompatible with --live. "
                "Live mode uses a TUI interface with its own controls."
            )
        return

    # Validate gecko mode doesn't use non-wall mode
    if args.format == "gecko" and getattr(args, 'mode', 'wall') != "wall":
        parser.error(
            "--mode option is incompatible with --gecko. "
            "Gecko format automatically includes both GIL-holding and CPU status analysis."
        )

    # Validate --opcodes is only used with compatible formats
    opcodes_compatible_formats = ("live", "gecko", "flamegraph", "heatmap", "binary")
    if getattr(args, 'opcodes', False) and args.format not in opcodes_compatible_formats:
        parser.error(
            f"--opcodes is only compatible with {', '.join('--' + f for f in opcodes_compatible_formats)}."
        )

    # Validate pstats-specific options are only used with pstats format
    if args.format != "pstats":
        issues = []
        if args.sort is not None:
            issues.append("--sort")
        if args.limit is not None:
            issues.append("--limit")
        if args.no_summary:
            issues.append("--no-summary")

        if issues:
            format_flag = f"--{args.format}"
            parser.error(
                f"Options {', '.join(issues)} are only valid with --pstats, not {format_flag}"
            )


def main():
    """Main entry point for the CLI."""
    # Set locale for number formatting, restore on exit
    old_locale = locale.setlocale(locale.LC_ALL, None)
    locale.setlocale(locale.LC_ALL, "")
    try:
        _main()
    finally:
        locale.setlocale(locale.LC_ALL, old_locale)


def _main():
    # Create the main parser
    parser = argparse.ArgumentParser(
        description=_HELP_DESCRIPTION,
        formatter_class=CustomFormatter,
    )

    # Create subparsers for commands
    subparsers = parser.add_subparsers(
        dest="command", required=True, help="Command to run"
    )

    # === RUN COMMAND ===
    run_parser = subparsers.add_parser(
        "run",
        help="Run and profile a script or module",
        formatter_class=CustomFormatter,
        description="""Run and profile a Python script or module

Examples:
  # Run and profile a module
  `python -m profiling.sampling run -m mymodule arg1 arg2`

  # Generate flamegraph from a script
  `python -m profiling.sampling run --flamegraph -o output.html script.py`

  # Profile with custom rate and duration
  `python -m profiling.sampling run -r 5khz -d 30 script.py`

  # Save collapsed stacks to file
  `python -m profiling.sampling run --collapsed -o stacks.txt script.py`

  # Live interactive mode for a script
  `python -m profiling.sampling run --live script.py`""",
    )
    run_parser.add_argument(
        "-m",
        "--module",
        action="store_true",
        help="Run target as a module (like python -m)",
    )
    run_parser.add_argument(
        "target",
        help="Script file or module name to profile",
    )
    run_parser.add_argument(
        "args",
        nargs=argparse.REMAINDER,
        help="Arguments to pass to the script or module",
    )
    run_parser.add_argument(
        "--live",
        action="store_true",
        help="Interactive TUI profiler (top-like interface, press 'q' to quit, 's' to cycle sort)",
    )
    _add_sampling_options(run_parser)
    _add_mode_options(run_parser)
    _add_format_options(run_parser)
    _add_pstats_options(run_parser)

    # === ATTACH COMMAND ===
    attach_parser = subparsers.add_parser(
        "attach",
        help="Attach to and profile a running process",
        formatter_class=CustomFormatter,
        description="""Attach to a running process and profile it

Examples:
  # Profile all threads, sort by total time
  `python -m profiling.sampling attach -a --sort tottime 1234`

  # Live interactive mode for a running process
  `python -m profiling.sampling attach --live 1234`""",
    )
    attach_parser.add_argument(
        "pid",
        type=int,
        help="Process ID to attach to",
    )
    attach_parser.add_argument(
        "--live",
        action="store_true",
        help="Interactive TUI profiler (top-like interface, press 'q' to quit, 's' to cycle sort)",
    )
    _add_sampling_options(attach_parser)
    _add_mode_options(attach_parser)
    _add_format_options(attach_parser)
    _add_pstats_options(attach_parser)

    # === REPLAY COMMAND ===
    replay_parser = subparsers.add_parser(
        "replay",
        help="Replay a binary profile and convert to another format",
        formatter_class=CustomFormatter,
        description="""Replay a binary profile file and convert to another format

Examples:
  # Convert binary to flamegraph
  `python -m profiling.sampling replay --flamegraph -o output.html profile.bin`

  # Convert binary to pstats and print to stdout
  `python -m profiling.sampling replay profile.bin`

  # Convert binary to gecko format
  `python -m profiling.sampling replay --gecko -o profile.json profile.bin`""",
    )
    replay_parser.add_argument(
        "input_file",
        help="Binary profile file to replay",
    )
    _add_format_options(replay_parser, include_compression=False, include_binary=False)
    _add_pstats_options(replay_parser)

    # Parse arguments
    args = parser.parse_args()

    # Validate arguments
    _validate_args(args, parser)

    # Command dispatch table
    command_handlers = {
        "run": _handle_run,
        "attach": _handle_attach,
        "replay": _handle_replay,
    }

    # Execute the appropriate command
    handler = command_handlers.get(args.command)
    if handler:
        handler(args)
    else:
        parser.error(f"Unknown command: {args.command}")


def _handle_attach(args):
    """Handle the 'attach' command."""
    if not _is_process_running(args.pid):
        raise SamplingUnknownProcessError(args.pid)
    # Check if live mode is requested
    if args.live:
        _handle_live_attach(args, args.pid)
        return

    # Use PROFILING_MODE_ALL for gecko format
    mode = (
        PROFILING_MODE_ALL
        if args.format == "gecko"
        else _parse_mode(args.mode)
    )

    # Determine skip_idle based on mode
    skip_idle = (
        mode != PROFILING_MODE_WALL if mode != PROFILING_MODE_ALL else False
    )

    output_file = None
    if args.format == "binary":
        output_file = args.outfile or _generate_output_filename(args.format, args.pid)

    # Create the appropriate collector
    collector = _create_collector(
        args.format, args.sample_interval_usec, skip_idle, args.opcodes,
        output_file=output_file,
        compression=getattr(args, 'compression', 'auto')
    )

    with _get_child_monitor_context(args, args.pid):
        collector = sample(
            args.pid,
            collector,
            duration_sec=args.duration,
            all_threads=args.all_threads,
            realtime_stats=args.realtime_stats,
            mode=mode,
            async_aware=args.async_mode if args.async_aware else None,
            native=args.native,
            gc=args.gc,
            opcodes=args.opcodes,
            blocking=args.blocking,
        )
        _handle_output(collector, args, args.pid, mode)


def _handle_run(args):
    """Handle the 'run' command."""
    # Validate target exists before launching subprocess
    if args.module:
        # Temporarily add cwd to sys.path so we can find modules in the
        # current directory, matching the coordinator's behavior
        cwd = os.getcwd()
        added_cwd = False
        if cwd not in sys.path:
            sys.path.insert(0, cwd)
            added_cwd = True
        try:
            if importlib.util.find_spec(args.target) is None:
                raise SamplingModuleNotFoundError(args.target)
        finally:
            if added_cwd:
                sys.path.remove(cwd)
    else:
        if not os.path.exists(args.target):
            raise SamplingScriptNotFoundError(args.target)

    # Check if live mode is requested
    if args.live:
        _handle_live_run(args)
        return

    # Build the command to run
    if args.module:
        cmd = (sys.executable, "-m", args.target, *args.args)
    else:
        cmd = (sys.executable, args.target, *args.args)

    # Run with synchronization
    try:
        process = _run_with_sync(cmd, suppress_output=False)
    except RuntimeError as e:
        sys.exit(f"Error: {e}")

    # Use PROFILING_MODE_ALL for gecko format
    mode = (
        PROFILING_MODE_ALL
        if args.format == "gecko"
        else _parse_mode(args.mode)
    )

    # Determine skip_idle based on mode
    skip_idle = (
        mode != PROFILING_MODE_WALL if mode != PROFILING_MODE_ALL else False
    )

    output_file = None
    if args.format == "binary":
        output_file = args.outfile or _generate_output_filename(args.format, process.pid)

    # Create the appropriate collector
    collector = _create_collector(
        args.format, args.sample_interval_usec, skip_idle, args.opcodes,
        output_file=output_file,
        compression=getattr(args, 'compression', 'auto')
    )

    with _get_child_monitor_context(args, process.pid):
        try:
            collector = sample(
                process.pid,
                collector,
                duration_sec=args.duration,
                all_threads=args.all_threads,
                realtime_stats=args.realtime_stats,
                mode=mode,
                async_aware=args.async_mode if args.async_aware else None,
                native=args.native,
                gc=args.gc,
                opcodes=args.opcodes,
                blocking=args.blocking,
            )
            _handle_output(collector, args, process.pid, mode)
        finally:
            # Terminate the main subprocess - child profilers finish when their
            # target processes exit
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=_PROCESS_KILL_TIMEOUT_SEC)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


def _handle_live_attach(args, pid):
    """Handle live mode for an existing process."""
    mode = _parse_mode(args.mode)

    # Determine skip_idle based on mode
    skip_idle = mode != PROFILING_MODE_WALL

    # Create live collector with default settings
    collector = LiveStatsCollector(
        args.sample_interval_usec,
        skip_idle=skip_idle,
        sort_by="tottime",  # Default initial sort
        limit=20,  # Default limit
        pid=pid,
        mode=mode,
        opcodes=args.opcodes,
        async_aware=args.async_mode if args.async_aware else None,
    )

    # Sample in live mode
    sample_live(
        pid,
        collector,
        duration_sec=args.duration,
        all_threads=args.all_threads,
        realtime_stats=args.realtime_stats,
        mode=mode,
        async_aware=args.async_mode if args.async_aware else None,
        native=args.native,
        gc=args.gc,
        opcodes=args.opcodes,
        blocking=args.blocking,
    )


def _handle_live_run(args):
    """Handle live mode for running a script/module."""
    # Build the command to run
    if args.module:
        cmd = (sys.executable, "-m", args.target, *args.args)
    else:
        cmd = (sys.executable, args.target, *args.args)

    # Run with synchronization, suppressing output for live mode
    try:
        process = _run_with_sync(cmd, suppress_output=True)
    except RuntimeError as e:
        sys.exit(f"Error: {e}")

    mode = _parse_mode(args.mode)

    # Determine skip_idle based on mode
    skip_idle = mode != PROFILING_MODE_WALL

    # Create live collector with default settings
    collector = LiveStatsCollector(
        args.sample_interval_usec,
        skip_idle=skip_idle,
        sort_by="tottime",  # Default initial sort
        limit=20,  # Default limit
        pid=process.pid,
        mode=mode,
        opcodes=args.opcodes,
        async_aware=args.async_mode if args.async_aware else None,
    )

    # Profile the subprocess in live mode
    try:
        sample_live(
            process.pid,
            collector,
            duration_sec=args.duration,
            all_threads=args.all_threads,
            realtime_stats=args.realtime_stats,
            mode=mode,
            async_aware=args.async_mode if args.async_aware else None,
            native=args.native,
            gc=args.gc,
            opcodes=args.opcodes,
            blocking=args.blocking,
        )
    finally:
        # Clean up the subprocess and get any error output
        returncode = process.poll()
        if returncode is None:
            # Process still running - terminate it
            process.terminate()
            try:
                process.wait(timeout=_PROCESS_KILL_TIMEOUT_SEC)
            except subprocess.TimeoutExpired:
                process.kill()
        # Ensure process is fully terminated
        process.wait()
        # Read any stderr output (tracebacks, errors, etc.)
        if process.stderr:
            with process.stderr:
                try:
                    stderr = process.stderr.read()
                    if stderr:
                        print(stderr.decode(), file=sys.stderr)
                except (OSError, ValueError):
                    # Ignore errors if pipe is already closed
                    pass


def _handle_replay(args):
    """Handle the 'replay' command - convert binary profile to another format."""
    if not os.path.exists(args.input_file):
        sys.exit(f"Error: Input file not found: {args.input_file}")

    with BinaryReader(args.input_file) as reader:
        info = reader.get_info()
        interval = info['sample_interval_us']

        print(f"Replaying {info['sample_count']} samples from {args.input_file}")
        print(f"  Sample interval: {interval} us")
        print(f"  Compression: {'zstd' if info.get('compression_type', 0) == 1 else 'none'}")

        collector = _create_collector(args.format, interval, skip_idle=False)

        def progress_callback(current, total):
            if total > 0:
                pct = current / total
                bar_width = 40
                filled = int(bar_width * pct)
                bar = '█' * filled + '░' * (bar_width - filled)
                print(f"\r  [{bar}] {pct*100:5.1f}% ({current:,}/{total:,})", end="", flush=True)

        count = reader.replay_samples(collector, progress_callback)
        print()

        if args.format == "pstats":
            if args.outfile:
                collector.export(args.outfile)
            else:
                sort_choice = args.sort if args.sort is not None else "nsamples"
                limit = args.limit if args.limit is not None else 15
                sort_mode = _sort_to_mode(sort_choice)
                collector.print_stats(sort_mode, limit, not args.no_summary, PROFILING_MODE_WALL)
        else:
            filename = args.outfile or _generate_output_filename(args.format, os.getpid())
            collector.export(filename)

            # Auto-open browser for HTML output if --browser flag is set
            if args.format in ('flamegraph', 'heatmap') and getattr(args, 'browser', False):
                _open_in_browser(filename)

        print(f"Replayed {count} samples")


if __name__ == "__main__":
    main()
