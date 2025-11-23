"""Command-line interface for the sampling profiler."""

import argparse
import os
import socket
import subprocess
import sys

from .sample import sample, sample_live
from .pstats_collector import PstatsCollector
from .stack_collector import CollapsedStackCollector, FlamegraphCollector
from .gecko_collector import GeckoCollector
from .constants import (
    PROFILING_MODE_ALL,
    PROFILING_MODE_WALL,
    PROFILING_MODE_CPU,
    PROFILING_MODE_GIL,
    SORT_MODE_NSAMPLES,
    SORT_MODE_TOTTIME,
    SORT_MODE_CUMTIME,
    SORT_MODE_SAMPLE_PCT,
    SORT_MODE_CUMUL_PCT,
    SORT_MODE_NSAMPLES_CUMUL,
)

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

Commands:
  run        Run and profile a script or module
  attach     Attach to and profile a running process

Examples:
  # Run and profile a script
  python -m profiling.sampling run script.py arg1 arg2

  # Attach to a running process
  python -m profiling.sampling attach 1234

  # Live interactive mode for a script
  python -m profiling.sampling run --live script.py

  # Live interactive mode for a running process
  python -m profiling.sampling attach --live 1234

Use 'python -m profiling.sampling <command> --help' for command-specific help."""


# Constants for socket synchronization
_SYNC_TIMEOUT = 5.0
_PROCESS_KILL_TIMEOUT = 2.0
_READY_MESSAGE = b"ready"
_RECV_BUFFER_SIZE = 1024

# Format configuration
FORMAT_EXTENSIONS = {
    "pstats": "pstats",
    "collapsed": "txt",
    "flamegraph": "html",
    "gecko": "json",
}

COLLECTOR_MAP = {
    "pstats": PstatsCollector,
    "collapsed": CollapsedStackCollector,
    "flamegraph": FlamegraphCollector,
    "gecko": GeckoCollector,
}


def _parse_mode(mode_string):
    """Convert mode string to mode constant."""
    mode_map = {
        "wall": PROFILING_MODE_WALL,
        "cpu": PROFILING_MODE_CPU,
        "gil": PROFILING_MODE_GIL,
    }
    return mode_map[mode_string]


def _run_with_sync(original_cmd, suppress_output=False):
    """Run a command with socket-based synchronization and return the process."""
    # Create a TCP socket for synchronization with better socket options
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sync_sock:
        # Set SO_REUSEADDR to avoid "Address already in use" errors
        sync_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sync_sock.bind(("127.0.0.1", 0))  # Let OS choose a free port
        sync_port = sync_sock.getsockname()[1]
        sync_sock.listen(1)
        sync_sock.settimeout(_SYNC_TIMEOUT)

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
        # Suppress stdout/stderr if requested (for live mode)
        popen_kwargs = {}
        if suppress_output:
            popen_kwargs["stdin"] = subprocess.DEVNULL
            popen_kwargs["stdout"] = subprocess.DEVNULL
            popen_kwargs["stderr"] = subprocess.DEVNULL

        process = subprocess.Popen(cmd, **popen_kwargs)

        try:
            # Wait for ready signal with timeout
            with sync_sock.accept()[0] as conn:
                ready_signal = conn.recv(_RECV_BUFFER_SIZE)

                if ready_signal != _READY_MESSAGE:
                    raise RuntimeError(
                        f"Invalid ready signal received: {ready_signal!r}"
                    )

        except socket.timeout:
            # If we timeout, kill the process and raise an error
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=_PROCESS_KILL_TIMEOUT)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            raise RuntimeError(
                "Process failed to signal readiness within timeout"
            )

        return process


def _add_sampling_options(parser):
    """Add sampling configuration options to a parser."""
    sampling_group = parser.add_argument_group("Sampling configuration")
    sampling_group.add_argument(
        "-i",
        "--interval",
        type=int,
        default=100,
        metavar="MICROSECONDS",
        help="sampling interval",
    )
    sampling_group.add_argument(
        "-d",
        "--duration",
        type=int,
        default=10,
        metavar="SECONDS",
        help="Sampling duration",
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


def _add_mode_options(parser):
    """Add mode options to a parser."""
    mode_group = parser.add_argument_group("Mode options")
    mode_group.add_argument(
        "--mode",
        choices=["wall", "cpu", "gil"],
        default="wall",
        help="Sampling mode: wall (all samples), cpu (only samples when thread is on CPU), "
        "gil (only samples when thread holds the GIL)",
    )


def _add_format_options(parser):
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
    parser.set_defaults(format="pstats")

    output_group.add_argument(
        "-o",
        "--output",
        dest="outfile",
        help="Save output to a file (default: stdout for pstats, "
        "auto-generated filename for other formats)",
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


def _create_collector(format_type, interval, skip_idle):
    """Create the appropriate collector based on format type.

    Args:
        format_type: The output format ('pstats', 'collapsed', 'flamegraph', 'gecko')
        interval: Sampling interval in microseconds
        skip_idle: Whether to skip idle samples

    Returns:
        A collector instance of the appropriate type
    """
    collector_class = COLLECTOR_MAP.get(format_type)
    if collector_class is None:
        raise ValueError(f"Unknown format: {format_type}")

    # Gecko format never skips idle (it needs both GIL and CPU data)
    if format_type == "gecko":
        skip_idle = False

    return collector_class(interval, skip_idle=skip_idle)


def _generate_output_filename(format_type, pid):
    """Generate output filename based on format and PID.

    Args:
        format_type: The output format
        pid: Process ID

    Returns:
        Generated filename
    """
    extension = FORMAT_EXTENSIONS.get(format_type, "txt")
    return f"{format_type}.{pid}.{extension}"


def _handle_output(collector, args, pid, mode):
    """Handle output for the collector based on format and arguments.

    Args:
        collector: The collector instance with profiling data
        args: Parsed command-line arguments
        pid: Process ID (for generating filenames)
        mode: Profiling mode used
    """
    if args.format == "pstats":
        if args.outfile:
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
        filename = args.outfile or _generate_output_filename(args.format, pid)
        collector.export(filename)


def _validate_args(args, parser):
    """Validate format-specific options and live mode requirements.

    Args:
        args: Parsed command-line arguments
        parser: ArgumentParser instance for error reporting
    """
    # Check if live mode is available
    if hasattr(args, 'live') and args.live and LiveStatsCollector is None:
        parser.error(
            "Live mode requires the curses module, which is not available."
        )

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
    if args.format == "gecko" and args.mode != "wall":
        parser.error(
            "--mode option is incompatible with --gecko. "
            "Gecko format automatically includes both GIL-holding and CPU status analysis."
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
  python -m profiling.sampling run -m mymodule arg1 arg2

  # Generate flamegraph from a script
  python -m profiling.sampling run --flamegraph -o output.html script.py

  # Profile with custom interval and duration
  python -m profiling.sampling run -i 50 -d 30 script.py

  # Save collapsed stacks to file
  python -m profiling.sampling run --collapsed -o stacks.txt script.py

  # Live interactive mode for a script
  python -m profiling.sampling run --live script.py""",
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
  python -m profiling.sampling attach -a --sort tottime 1234

  # Live interactive mode for a running process
  python -m profiling.sampling attach --live 1234""",
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

    # Parse arguments
    args = parser.parse_args()

    # Validate arguments
    _validate_args(args, parser)

    # Command dispatch table
    command_handlers = {
        "run": _handle_run,
        "attach": _handle_attach,
    }

    # Execute the appropriate command
    handler = command_handlers.get(args.command)
    if handler:
        handler(args)
    else:
        parser.error(f"Unknown command: {args.command}")


def _handle_attach(args):
    """Handle the 'attach' command."""
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

    # Create the appropriate collector
    collector = _create_collector(args.format, args.interval, skip_idle)

    # Sample the process
    collector = sample(
        args.pid,
        collector,
        duration_sec=args.duration,
        all_threads=args.all_threads,
        realtime_stats=args.realtime_stats,
        mode=mode,
        native=args.native,
        gc=args.gc,
    )

    # Handle output
    _handle_output(collector, args, args.pid, mode)


def _handle_run(args):
    """Handle the 'run' command."""
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
    process = _run_with_sync(cmd, suppress_output=False)

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

    # Create the appropriate collector
    collector = _create_collector(args.format, args.interval, skip_idle)

    # Profile the subprocess
    try:
        collector = sample(
            process.pid,
            collector,
            duration_sec=args.duration,
            all_threads=args.all_threads,
            realtime_stats=args.realtime_stats,
            mode=mode,
            native=args.native,
            gc=args.gc,
        )

        # Handle output
        _handle_output(collector, args, process.pid, mode)
    finally:
        # Clean up the subprocess
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=_PROCESS_KILL_TIMEOUT)
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
        args.interval,
        skip_idle=skip_idle,
        sort_by="tottime",  # Default initial sort
        limit=20,  # Default limit
        pid=pid,
        mode=mode,
    )

    # Sample in live mode
    sample_live(
        pid,
        collector,
        duration_sec=args.duration,
        all_threads=args.all_threads,
        realtime_stats=args.realtime_stats,
        mode=mode,
        native=args.native,
        gc=args.gc,
    )


def _handle_live_run(args):
    """Handle live mode for running a script/module."""
    # Build the command to run
    if args.module:
        cmd = (sys.executable, "-m", args.target, *args.args)
    else:
        cmd = (sys.executable, args.target, *args.args)

    # Run with synchronization, suppressing output for live mode
    process = _run_with_sync(cmd, suppress_output=True)

    mode = _parse_mode(args.mode)

    # Determine skip_idle based on mode
    skip_idle = mode != PROFILING_MODE_WALL

    # Create live collector with default settings
    collector = LiveStatsCollector(
        args.interval,
        skip_idle=skip_idle,
        sort_by="tottime",  # Default initial sort
        limit=20,  # Default limit
        pid=process.pid,
        mode=mode,
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
            native=args.native,
            gc=args.gc,
        )
    finally:
        # Clean up the subprocess
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=_PROCESS_KILL_TIMEOUT)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


if __name__ == "__main__":
    main()
