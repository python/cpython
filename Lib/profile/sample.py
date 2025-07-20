import argparse
import _remote_debugging
import os
import pstats
import statistics
import sys
import sysconfig
import time
from collections import deque
from _colorize import ANSIColors

from .pstats_collector import PstatsCollector
from .stack_collector import CollapsedStackCollector

FREE_THREADED_BUILD = sysconfig.get_config_var("Py_GIL_DISABLED") is not None

class SampleProfiler:
    def __init__(self, pid, sample_interval_usec, all_threads):
        self.pid = pid
        self.sample_interval_usec = sample_interval_usec
        self.all_threads = all_threads
        if FREE_THREADED_BUILD:
            self.unwinder = _remote_debugging.RemoteUnwinder(
                self.pid, all_threads=self.all_threads
            )
        else:
            only_active_threads = bool(self.all_threads)
            self.unwinder = _remote_debugging.RemoteUnwinder(
                self.pid, only_active_thread=only_active_threads
            )
        # Track sample intervals and total sample count
        self.sample_intervals = deque(maxlen=100)
        self.total_samples = 0
        self.realtime_stats = False

    def sample(self, collector, duration_sec=10):
        sample_interval_sec = self.sample_interval_usec / 1_000_000
        running_time = 0
        num_samples = 0
        errors = 0
        start_time = next_time = time.perf_counter()
        last_sample_time = start_time
        realtime_update_interval = 1.0  # Update every second
        last_realtime_update = start_time

        while running_time < duration_sec:
            current_time = time.perf_counter()
            if next_time < current_time:
                try:
                    stack_frames = self.unwinder.get_stack_trace()
                    collector.collect(stack_frames)
                except ProcessLookupError:
                    break
                except (RuntimeError, UnicodeDecodeError, MemoryError, OSError):
                    errors += 1
                except Exception as e:
                    if not self._is_process_running():
                        break
                    raise e from None

                # Track actual sampling intervals for real-time stats
                if num_samples > 0:
                    actual_interval = current_time - last_sample_time
                    self.sample_intervals.append(
                        1.0 / actual_interval
                    )  # Convert to Hz
                    self.total_samples += 1

                    # Print real-time statistics if enabled
                    if (
                        self.realtime_stats
                        and (current_time - last_realtime_update)
                        >= realtime_update_interval
                    ):
                        self._print_realtime_stats()
                        last_realtime_update = current_time

                last_sample_time = current_time
                num_samples += 1
                next_time += sample_interval_sec

            running_time = time.perf_counter() - start_time

        # Clear real-time stats line if it was being displayed
        if self.realtime_stats and len(self.sample_intervals) > 0:
            print()  # Add newline after real-time stats

        print(f"Captured {num_samples} samples in {running_time:.2f} seconds")
        print(f"Sample rate: {num_samples / running_time:.2f} samples/sec")
        print(f"Error rate: {(errors / num_samples) * 100:.2f}%")

        expected_samples = int(duration_sec / sample_interval_sec)
        if num_samples < expected_samples:
            print(
                f"Warning: missed {expected_samples - num_samples} samples "
                f"from the expected total of {expected_samples} "
                f"({(expected_samples - num_samples) / expected_samples * 100:.2f}%)"
            )

    def _is_process_running(self):
        if sys.platform == "linux" or sys.platform == "darwin":
            try:
                os.kill(self.pid, 0)
                return True
            except ProcessLookupError:
                return False
        elif sys.platform == "win32":
            try:
                _remote_debugging.RemoteUnwinder(self.pid)
            except Exception:
                return False
            return True
        else:
            raise ValueError(f"Unsupported platform: {sys.platform}")

    def _print_realtime_stats(self):
        """Print real-time sampling statistics."""
        if len(self.sample_intervals) < 2:
            return

        # Calculate statistics on the Hz values (deque automatically maintains rolling window)
        hz_values = list(self.sample_intervals)
        mean_hz = statistics.mean(hz_values)
        min_hz = min(hz_values)
        max_hz = max(hz_values)

        # Calculate microseconds per sample for all metrics (1/Hz * 1,000,000)
        mean_us_per_sample = (1.0 / mean_hz) * 1_000_000 if mean_hz > 0 else 0
        min_us_per_sample = (
            (1.0 / max_hz) * 1_000_000 if max_hz > 0 else 0
        )  # Min time = Max Hz
        max_us_per_sample = (
            (1.0 / min_hz) * 1_000_000 if min_hz > 0 else 0
        )  # Max time = Min Hz

        # Clear line and print stats
        print(
            f"\r\033[K{ANSIColors.BOLD_BLUE}Real-time sampling stats:{ANSIColors.RESET} "
            f"{ANSIColors.YELLOW}Mean: {mean_hz:.1f}Hz ({mean_us_per_sample:.2f}µs){ANSIColors.RESET} "
            f"{ANSIColors.GREEN}Min: {min_hz:.1f}Hz ({max_us_per_sample:.2f}µs){ANSIColors.RESET} "
            f"{ANSIColors.RED}Max: {max_hz:.1f}Hz ({min_us_per_sample:.2f}µs){ANSIColors.RESET} "
            f"{ANSIColors.CYAN}Samples: {self.total_samples}{ANSIColors.RESET}",
            end="",
            flush=True,
        )


def _determine_best_unit(max_value):
    """Determine the best unit (s, ms, μs) and scale factor for a maximum value."""
    if max_value >= 1.0:
        return "s", 1.0
    elif max_value >= 0.001:
        return "ms", 1000.0
    else:
        return "μs", 1000000.0


def print_sampled_stats(
    stats, sort=-1, limit=None, show_summary=True, sample_interval_usec=100
):
    # Get the stats data
    stats_list = []
    for func, (
        direct_calls,
        cumulative_calls,
        total_time,
        cumulative_time,
        callers,
    ) in stats.stats.items():
        stats_list.append(
            (
                func,
                direct_calls,
                cumulative_calls,
                total_time,
                cumulative_time,
                callers,
            )
        )

    # Calculate total samples for percentage calculations (using direct_calls)
    total_samples = sum(
        direct_calls for _, direct_calls, _, _, _, _ in stats_list
    )

    # Sort based on the requested field
    sort_field = sort
    if sort_field == -1:  # stdname
        stats_list.sort(key=lambda x: str(x[0]))
    elif sort_field == 0:  # nsamples (direct samples)
        stats_list.sort(key=lambda x: x[1], reverse=True)  # direct_calls
    elif sort_field == 1:  # tottime
        stats_list.sort(key=lambda x: x[3], reverse=True)  # total_time
    elif sort_field == 2:  # cumtime
        stats_list.sort(key=lambda x: x[4], reverse=True)  # cumulative_time
    elif sort_field == 3:  # sample%
        stats_list.sort(
            key=lambda x: (x[1] / total_samples * 100)
            if total_samples > 0
            else 0,
            reverse=True,  # direct_calls percentage
        )
    elif sort_field == 4:  # cumul%
        stats_list.sort(
            key=lambda x: (x[2] / total_samples * 100)
            if total_samples > 0
            else 0,
            reverse=True,  # cumulative_calls percentage
        )
    elif sort_field == 5:  # nsamples (cumulative samples)
        stats_list.sort(key=lambda x: x[2], reverse=True)  # cumulative_calls

    # Apply limit if specified
    if limit is not None:
        stats_list = stats_list[:limit]

    # Determine the best unit for time columns based on maximum values
    max_total_time = max(
        (total_time for _, _, _, total_time, _, _ in stats_list), default=0
    )
    max_cumulative_time = max(
        (cumulative_time for _, _, _, _, cumulative_time, _ in stats_list),
        default=0,
    )

    total_time_unit, total_time_scale = _determine_best_unit(max_total_time)
    cumulative_time_unit, cumulative_time_scale = _determine_best_unit(
        max_cumulative_time
    )

    # Define column widths for consistent alignment
    col_widths = {
        "nsamples": 15,  # "nsamples" column (inline/cumulative format)
        "sample_pct": 8,  # "sample%" column
        "tottime": max(12, len(f"tottime ({total_time_unit})")),
        "cum_pct": 8,  # "cumul%" column
        "cumtime": max(12, len(f"cumtime ({cumulative_time_unit})")),
    }

    # Print header with colors and proper alignment
    print(f"{ANSIColors.BOLD_BLUE}Profile Stats:{ANSIColors.RESET}")

    header_nsamples = f"{ANSIColors.BOLD_BLUE}{'nsamples':>{col_widths['nsamples']}}{ANSIColors.RESET}"
    header_sample_pct = f"{ANSIColors.BOLD_BLUE}{'sample%':>{col_widths['sample_pct']}}{ANSIColors.RESET}"
    header_tottime = f"{ANSIColors.BOLD_BLUE}{f'tottime ({total_time_unit})':>{col_widths['tottime']}}{ANSIColors.RESET}"
    header_cum_pct = f"{ANSIColors.BOLD_BLUE}{'cumul%':>{col_widths['cum_pct']}}{ANSIColors.RESET}"
    header_cumtime = f"{ANSIColors.BOLD_BLUE}{f'cumtime ({cumulative_time_unit})':>{col_widths['cumtime']}}{ANSIColors.RESET}"
    header_filename = (
        f"{ANSIColors.BOLD_BLUE}filename:lineno(function){ANSIColors.RESET}"
    )

    print(
        f"{header_nsamples}  {header_sample_pct}  {header_tottime}  {header_cum_pct}  {header_cumtime}  {header_filename}"
    )

    # Print each line with proper alignment
    for (
        func,
        direct_calls,
        cumulative_calls,
        total_time,
        cumulative_time,
        callers,
    ) in stats_list:
        # Calculate percentages
        sample_pct = (
            (direct_calls / total_samples * 100) if total_samples > 0 else 0
        )
        cum_pct = (
            (cumulative_calls / total_samples * 100)
            if total_samples > 0
            else 0
        )

        # Format values with proper alignment - always use A/B format
        nsamples_str = f"{direct_calls}/{cumulative_calls}"
        nsamples_str = f"{nsamples_str:>{col_widths['nsamples']}}"
        sample_pct_str = f"{sample_pct:{col_widths['sample_pct']}.1f}"
        tottime = f"{total_time * total_time_scale:{col_widths['tottime']}.3f}"
        cum_pct_str = f"{cum_pct:{col_widths['cum_pct']}.1f}"
        cumtime = f"{cumulative_time * cumulative_time_scale:{col_widths['cumtime']}.3f}"

        # Format the function name with colors
        func_name = (
            f"{ANSIColors.GREEN}{func[0]}{ANSIColors.RESET}:"
            f"{ANSIColors.YELLOW}{func[1]}{ANSIColors.RESET}("
            f"{ANSIColors.CYAN}{func[2]}{ANSIColors.RESET})"
        )

        # Print the formatted line with consistent spacing
        print(
            f"{nsamples_str}  {sample_pct_str}  {tottime}  {cum_pct_str}  {cumtime}  {func_name}"
        )

    # Print legend
    print(f"\n{ANSIColors.BOLD_BLUE}Legend:{ANSIColors.RESET}")
    print(
        f"  {ANSIColors.YELLOW}nsamples{ANSIColors.RESET}: Direct/Cumulative samples (direct executing / on call stack)"
    )
    print(
        f"  {ANSIColors.YELLOW}sample%{ANSIColors.RESET}: Percentage of total samples this function was directly executing"
    )
    print(
        f"  {ANSIColors.YELLOW}tottime{ANSIColors.RESET}: Estimated total time spent directly in this function"
    )
    print(
        f"  {ANSIColors.YELLOW}cumul%{ANSIColors.RESET}: Percentage of total samples when this function was on the call stack"
    )
    print(
        f"  {ANSIColors.YELLOW}cumtime{ANSIColors.RESET}: Estimated cumulative time (including time in called functions)"
    )
    print(
        f"  {ANSIColors.YELLOW}filename:lineno(function){ANSIColors.RESET}: Function location and name"
    )

    def _format_func_name(func):
        """Format function name with colors."""
        return (
            f"{ANSIColors.GREEN}{func[0]}{ANSIColors.RESET}:"
            f"{ANSIColors.YELLOW}{func[1]}{ANSIColors.RESET}("
            f"{ANSIColors.CYAN}{func[2]}{ANSIColors.RESET})"
        )

    def _print_top_functions(stats_list, title, key_func, format_line, n=3):
        """Print top N functions sorted by key_func with formatted output."""
        print(f"\n{ANSIColors.BOLD_BLUE}{title}:{ANSIColors.RESET}")
        sorted_stats = sorted(stats_list, key=key_func, reverse=True)
        for stat in sorted_stats[:n]:
            if line := format_line(stat):
                print(f"  {line}")

    # Print summary of interesting functions if enabled
    if show_summary and stats_list:
        print(
            f"\n{ANSIColors.BOLD_BLUE}Summary of Interesting Functions:{ANSIColors.RESET}"
        )

        # Aggregate stats by fully qualified function name (ignoring line numbers)
        func_aggregated = {}
        for (
            func,
            direct_calls,
            cumulative_calls,
            total_time,
            cumulative_time,
            callers,
        ) in stats_list:
            # Use filename:function_name as the key to get fully qualified name
            qualified_name = f"{func[0]}:{func[2]}"
            if qualified_name not in func_aggregated:
                func_aggregated[qualified_name] = [
                    0,
                    0,
                    0,
                    0,
                ]  # direct_calls, cumulative_calls, total_time, cumulative_time
            func_aggregated[qualified_name][0] += direct_calls
            func_aggregated[qualified_name][1] += cumulative_calls
            func_aggregated[qualified_name][2] += total_time
            func_aggregated[qualified_name][3] += cumulative_time

        # Convert aggregated data back to list format for processing
        aggregated_stats = []
        for qualified_name, (
            prim_calls,
            total_calls,
            total_time,
            cumulative_time,
        ) in func_aggregated.items():
            # Parse the qualified name back to filename and function name
            if ":" in qualified_name:
                filename, func_name = qualified_name.rsplit(":", 1)
            else:
                filename, func_name = "", qualified_name
            # Create a dummy func tuple with filename and function name for display
            dummy_func = (filename, "", func_name)
            aggregated_stats.append(
                (
                    dummy_func,
                    prim_calls,
                    total_calls,
                    total_time,
                    cumulative_time,
                    {},
                )
            )

        # Determine best units for summary metrics
        max_total_time = max(
            (total_time for _, _, _, total_time, _, _ in aggregated_stats),
            default=0,
        )
        max_cumulative_time = max(
            (
                cumulative_time
                for _, _, _, _, cumulative_time, _ in aggregated_stats
            ),
            default=0,
        )

        total_unit, total_scale = _determine_best_unit(max_total_time)
        cumulative_unit, cumulative_scale = _determine_best_unit(
            max_cumulative_time
        )

        # Functions with highest direct/cumulative ratio (hot spots)
        def format_hotspots(stat):
            func, direct_calls, cumulative_calls, total_time, _, _ = stat
            if direct_calls > 0 and cumulative_calls > 0:
                ratio = direct_calls / cumulative_calls
                direct_pct = (
                    (direct_calls / total_samples * 100)
                    if total_samples > 0
                    else 0
                )
                return (
                    f"{ratio:.3f} direct/cumulative ratio, "
                    f"{direct_pct:.1f}% direct samples: {_format_func_name(func)}"
                )
            return None

        _print_top_functions(
            aggregated_stats,
            "Functions with Highest Direct/Cumulative Ratio (Hot Spots)",
            key_func=lambda x: (x[1] / x[2]) if x[2] > 0 else 0,
            format_line=format_hotspots,
        )

        # Functions with highest call frequency (cumulative/direct difference)
        def format_call_frequency(stat):
            func, direct_calls, cumulative_calls, total_time, _, _ = stat
            if cumulative_calls > direct_calls:
                call_frequency = cumulative_calls - direct_calls
                cum_pct = (
                    (cumulative_calls / total_samples * 100)
                    if total_samples > 0
                    else 0
                )
                return (
                    f"{call_frequency:d} indirect calls, "
                    f"{cum_pct:.1f}% total stack presence: {_format_func_name(func)}"
                )
            return None

        _print_top_functions(
            aggregated_stats,
            "Functions with Highest Call Frequency (Indirect Calls)",
            key_func=lambda x: x[2] - x[1],  # Sort by (cumulative - direct)
            format_line=format_call_frequency,
        )

        # Functions with highest cumulative-to-direct multiplier (call magnification)
        def format_call_magnification(stat):
            func, direct_calls, cumulative_calls, total_time, _, _ = stat
            if direct_calls > 0 and cumulative_calls > direct_calls:
                multiplier = cumulative_calls / direct_calls
                indirect_calls = cumulative_calls - direct_calls
                return (
                    f"{multiplier:.1f}x call magnification, "
                    f"{indirect_calls:d} indirect calls from {direct_calls:d} direct: {_format_func_name(func)}"
                )
            return None

        _print_top_functions(
            aggregated_stats,
            "Functions with Highest Call Magnification (Cumulative/Direct)",
            key_func=lambda x: (x[2] / x[1])
            if x[1] > 0
            else 0,  # Sort by cumulative/direct ratio
            format_line=format_call_magnification,
        )


def sample(
    pid,
    *,
    sort=2,
    sample_interval_usec=100,
    duration_sec=10,
    filename=None,
    all_threads=False,
    limit=None,
    show_summary=True,
    output_format="pstats",
    realtime_stats=False,
):
    profiler = SampleProfiler(
        pid, sample_interval_usec, all_threads=all_threads
    )
    profiler.realtime_stats = realtime_stats

    collector = None
    match output_format:
        case "pstats":
            collector = PstatsCollector(sample_interval_usec)
        case "collapsed":
            collector = CollapsedStackCollector()
            filename = filename or f"collapsed.{pid}.txt"
        case _:
            raise ValueError(f"Invalid output format: {output_format}")

    profiler.sample(collector, duration_sec)

    if output_format == "pstats" and not filename:
        stats = pstats.SampledStats(collector).strip_dirs()
        print_sampled_stats(
            stats, sort, limit, show_summary, sample_interval_usec
        )
    else:
        collector.export(filename)


def _validate_collapsed_format_args(args, parser):
    # Check for incompatible pstats options
    invalid_opts = []

    # Get list of pstats-specific options
    pstats_options = {"sort": None, "limit": None, "no_summary": False}

    # Find the default values from the argument definitions
    for action in parser._actions:
        if action.dest in pstats_options and hasattr(action, "default"):
            pstats_options[action.dest] = action.default

    # Check if any pstats-specific options were provided by comparing with defaults
    for opt, default in pstats_options.items():
        if getattr(args, opt) != default:
            invalid_opts.append(opt.replace("no_", ""))

    if invalid_opts:
        parser.error(
            f"The following options are only valid with --pstats format: {', '.join(invalid_opts)}"
        )

    # Set default output filename for collapsed format
    if not args.outfile:
        args.outfile = f"collapsed.{args.pid}.txt"


def main():
    # Create the main parser
    parser = argparse.ArgumentParser(
        description=(
            "Sample a process's stack frames and generate profiling data.\n"
            "Supports two output formats:\n"
            "  - pstats: Detailed profiling statistics with sorting options\n"
            "  - collapsed: Stack traces for generating flamegraphs\n"
            "\n"
            "Examples:\n"
            "  # Profile process 1234 for 10 seconds with default settings\n"
            "  python -m profile.sample 1234\n"
            "\n"
            "  # Profile with custom interval and duration, save to file\n"
            "  python -m profile.sample -i 50 -d 30 -o profile.stats 1234\n"
            "\n"
            "  # Generate collapsed stacks for flamegraph\n"
            "  python -m profile.sample --collapsed 1234\n"
            "\n"
            "  # Profile all threads, sort by total time\n"
            "  python -m profile.sample -a --sort-tottime 1234\n"
            "\n"
            "  # Profile for 1 minute with 1ms sampling interval\n"
            "  python -m profile.sample -i 1000 -d 60 1234\n"
            "\n"
            "  # Show only top 20 functions sorted by direct samples\n"
            "  python -m profile.sample --sort-nsamples -l 20 1234\n"
            "\n"
            "  # Profile all threads and save collapsed stacks\n"
            "  python -m profile.sample -a --collapsed -o stacks.txt 1234\n"
            "\n"
            "  # Profile with real-time sampling statistics\n"
            "  python -m profile.sample --realtime-stats 1234\n"
            "\n"
            "  # Sort by sample percentage to find most sampled functions\n"
            "  python -m profile.sample --sort-sample-pct 1234\n"
            "\n"
            "  # Sort by cumulative samples to find functions most on call stack\n"
            "  python -m profile.sample --sort-nsamples-cumul 1234"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    # Required arguments
    parser.add_argument("pid", type=int, help="Process ID to sample")

    # Sampling options
    sampling_group = parser.add_argument_group("Sampling configuration")
    sampling_group.add_argument(
        "-i",
        "--interval",
        type=int,
        default=100,
        help="Sampling interval in microseconds (default: 100)",
    )
    sampling_group.add_argument(
        "-d",
        "--duration",
        type=int,
        default=10,
        help="Sampling duration in seconds (default: 10)",
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
        default=False,
        help="Print real-time sampling statistics (Hz, mean, min, max, stdev) during profiling",
    )

    # Output format selection
    output_group = parser.add_argument_group("Output options")
    output_format = output_group.add_mutually_exclusive_group()
    output_format.add_argument(
        "--pstats",
        action="store_const",
        const="pstats",
        dest="format",
        default="pstats",
        help="Generate pstats output (default)",
    )
    output_format.add_argument(
        "--collapsed",
        action="store_const",
        const="collapsed",
        dest="format",
        help="Generate collapsed stack traces for flamegraphs",
    )

    output_group.add_argument(
        "-o",
        "--outfile",
        help="Save output to a file (if omitted, prints to stdout for pstats, "
        "or saves to collapsed.<pid>.txt for collapsed format)",
    )

    # pstats-specific options
    pstats_group = parser.add_argument_group("pstats format options")
    sort_group = pstats_group.add_mutually_exclusive_group()
    sort_group.add_argument(
        "--sort-nsamples",
        action="store_const",
        const=0,
        dest="sort",
        help="Sort by number of direct samples (nsamples column)",
    )
    sort_group.add_argument(
        "--sort-tottime",
        action="store_const",
        const=1,
        dest="sort",
        help="Sort by total time (tottime column)",
    )
    sort_group.add_argument(
        "--sort-cumtime",
        action="store_const",
        const=2,
        dest="sort",
        help="Sort by cumulative time (cumtime column, default)",
    )
    sort_group.add_argument(
        "--sort-sample-pct",
        action="store_const",
        const=3,
        dest="sort",
        help="Sort by sample percentage (sample%% column)",
    )
    sort_group.add_argument(
        "--sort-cumul-pct",
        action="store_const",
        const=4,
        dest="sort",
        help="Sort by cumulative sample percentage (cumul%% column)",
    )
    sort_group.add_argument(
        "--sort-nsamples-cumul",
        action="store_const",
        const=5,
        dest="sort",
        help="Sort by cumulative samples (nsamples column, cumulative part)",
    )
    sort_group.add_argument(
        "--sort-name",
        action="store_const",
        const=-1,
        dest="sort",
        help="Sort by function name",
    )

    pstats_group.add_argument(
        "-l",
        "--limit",
        type=int,
        help="Limit the number of rows in the output",
        default=15,
    )
    pstats_group.add_argument(
        "--no-summary",
        action="store_true",
        help="Disable the summary section in the output",
    )

    args = parser.parse_args()

    # Validate format-specific arguments
    if args.format == "collapsed":
        _validate_collapsed_format_args(args, parser)

    sort_value = args.sort if args.sort is not None else 2

    sample(
        args.pid,
        sample_interval_usec=args.interval,
        duration_sec=args.duration,
        filename=args.outfile,
        all_threads=args.all_threads,
        limit=args.limit,
        sort=sort_value,
        show_summary=not args.no_summary,
        output_format=args.format,
        realtime_stats=args.realtime_stats,
    )


if __name__ == "__main__":
    main()
