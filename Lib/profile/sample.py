import argparse
import _remote_debugging
import pstats
import statistics
import time
from collections import deque
from _colorize import ANSIColors

from .pstats_collector import PstatsCollector
from .stack_collectors import CollapsedStackCollector


class SampleProfiler:
    def __init__(self, pid, sample_interval_usec, all_threads):
        self.pid = pid
        self.sample_interval_usec = sample_interval_usec
        self.all_threads = all_threads
        self.unwinder = _remote_debugging.RemoteUnwinder(
            self.pid, all_threads=self.all_threads
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
                except (RuntimeError, UnicodeDecodeError, OSError):
                    errors += 1

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


def print_sampled_stats(stats, sort=-1, limit=None, show_summary=True):
    # Get the stats data
    stats_list = []
    for func, (cc, nc, tt, ct, callers) in stats.stats.items():
        stats_list.append((func, cc, nc, tt, ct, callers))

    # Sort based on the requested field
    sort_field = sort
    if sort_field == -1:  # stdname
        stats_list.sort(key=lambda x: str(x[0]))
    elif sort_field == 0:  # calls
        stats_list.sort(key=lambda x: x[2], reverse=True)
    elif sort_field == 1:  # time
        stats_list.sort(key=lambda x: x[3], reverse=True)
    elif sort_field == 2:  # cumulative
        stats_list.sort(key=lambda x: x[4], reverse=True)
    elif sort_field == 3:  # percall
        stats_list.sort(
            key=lambda x: x[3] / x[2] if x[2] > 0 else 0, reverse=True
        )
    elif sort_field == 4:  # cumpercall
        stats_list.sort(
            key=lambda x: x[4] / x[2] if x[2] > 0 else 0, reverse=True
        )

    # Apply limit if specified
    if limit is not None:
        stats_list = stats_list[:limit]

    # Determine the best unit for each column based on maximum values
    max_tt = max((tt for _, _, _, tt, _, _ in stats_list), default=0)
    max_ct = max((ct for _, _, _, _, ct, _ in stats_list), default=0)
    max_percall = max(
        (tt / nc if nc > 0 else 0 for _, _, nc, tt, _, _ in stats_list),
        default=0,
    )
    max_cumpercall = max(
        (ct / nc if nc > 0 else 0 for _, _, nc, _, ct, _ in stats_list),
        default=0,
    )

    tt_unit, tt_scale = _determine_best_unit(max_tt)
    ct_unit, ct_scale = _determine_best_unit(max_ct)
    percall_unit, percall_scale = _determine_best_unit(max_percall)
    cumpercall_unit, cumpercall_scale = _determine_best_unit(max_cumpercall)

    # Define column widths for consistent alignment, accounting for unit labels
    col_widths = {
        "nsamples": 10,
        "tottime": max(12, len(f"tottime ({tt_unit})")),
        "percall": max(12, len(f"percall ({percall_unit})")),
        "cumtime": max(12, len(f"cumtime ({ct_unit})")),
        "cumpercall": max(14, len(f"cumpercall ({cumpercall_unit})")),
    }

    # Print header with colors and proper alignment
    print(f"{ANSIColors.BOLD_BLUE}Profile Stats:{ANSIColors.RESET}")

    header_nsamples = f"{ANSIColors.BOLD_BLUE}{'nsamples':>{col_widths['nsamples']}}{ANSIColors.RESET}"
    header_tottime = f"{ANSIColors.BOLD_BLUE}{f'tottime ({tt_unit})':>{col_widths['tottime']}}{ANSIColors.RESET}"
    header_percall = f"{ANSIColors.BOLD_BLUE}{f'percall ({percall_unit})':>{col_widths['percall']}}{ANSIColors.RESET}"
    header_cumtime = f"{ANSIColors.BOLD_BLUE}{f'cumtime ({ct_unit})':>{col_widths['cumtime']}}{ANSIColors.RESET}"
    header_cumpercall = f"{ANSIColors.BOLD_BLUE}{f'cumpercall ({cumpercall_unit})':>{col_widths['cumpercall']}}{ANSIColors.RESET}"
    header_filename = (
        f"{ANSIColors.BOLD_BLUE}filename:lineno(function){ANSIColors.RESET}"
    )

    print(
        f"{header_nsamples}  {header_tottime}  {header_percall}  {header_cumtime}  {header_cumpercall}  {header_filename}"
    )

    # Print each line with proper alignment
    for func, cc, nc, tt, ct, callers in stats_list:
        if nc != cc:
            ncalls = f"{nc}/{cc}"
        else:
            ncalls = str(nc)

        # Format time values with column-specific units (no unit suffix since it's in header)
        tottime = f"{tt * tt_scale:{col_widths['tottime']}.3f}"
        percall = (
            f"{(tt / nc) * percall_scale:{col_widths['percall']}.3f}"
            if nc > 0
            else f"{'N/A':>{col_widths['percall']}}"
        )
        cumtime = f"{ct * ct_scale:{col_widths['cumtime']}.3f}"
        cumpercall = (
            f"{(ct / nc) * cumpercall_scale:{col_widths['cumpercall']}.3f}"
            if nc > 0
            else f"{'N/A':>{col_widths['cumpercall']}}"
        )

        # Format the function name with colors
        func_name = (
            f"{ANSIColors.GREEN}{func[0]}{ANSIColors.RESET}:"
            f"{ANSIColors.YELLOW}{func[1]}{ANSIColors.RESET}("
            f"{ANSIColors.CYAN}{func[2]}{ANSIColors.RESET})"
        )

        # Print the formatted line with consistent spacing
        print(
            f"{ncalls:>{col_widths['nsamples']}}  {tottime}  {percall}  {cumtime}  {cumpercall}  {func_name}"
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
            prim_calls,
            total_calls,
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
                ]  # prim_calls, total_calls, total_time, cumulative_time
            func_aggregated[qualified_name][0] += prim_calls
            func_aggregated[qualified_name][1] += total_calls
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
        max_per_call_time = max(
            (
                total_time / total_calls if total_calls > 0 else 0
                for _, _, total_calls, total_time, _, _ in aggregated_stats
            ),
            default=0,
        )
        max_cumulative_per_call = max(
            (
                cumulative_time / total_calls if total_calls > 0 else 0
                for _, _, total_calls, _, cumulative_time, _ in aggregated_stats
            ),
            default=0,
        )

        total_unit, total_scale = _determine_best_unit(max_total_time)
        cumulative_unit, cumulative_scale = _determine_best_unit(
            max_cumulative_time
        )
        per_call_unit, per_call_scale = _determine_best_unit(max_per_call_time)
        cumulative_per_call_unit, cumulative_per_call_scale = (
            _determine_best_unit(max_cumulative_per_call)
        )

        # Most time-consuming functions (by total time)
        def format_time_consuming(stat):
            func, _, total_calls, total_time, _, _ = stat
            if total_time > 0:
                return (
                    f"{total_time * total_scale:.3f} {total_unit} total time, "
                    f"{(total_time / total_calls) * per_call_scale:.3f} {per_call_unit} per call: {_format_func_name(func)}"
                )
            return None

        _print_top_functions(
            aggregated_stats,
            "Most Time-Consuming Functions",
            key_func=lambda x: x[3],
            format_line=format_time_consuming,
        )

        # Most called functions
        def format_most_called(stat):
            func, _, total_calls, total_time, _, _ = stat
            if total_calls > 0:
                return (
                    f"{total_calls:d} calls, {(total_time / total_calls) * per_call_scale:.3f} {per_call_unit} "
                    f"per call: {_format_func_name(func)}"
                )
            return None

        _print_top_functions(
            aggregated_stats,
            "Most Called Functions",
            key_func=lambda x: x[2],
            format_line=format_most_called,
        )

        # Functions with highest per-call overhead
        def format_overhead(stat):
            func, _, total_calls, total_time, _, _ = stat
            if total_calls > 0 and total_time > 0:
                return (
                    f"{(total_time / total_calls) * per_call_scale:.3f} {per_call_unit} per call, "
                    f"{total_calls:d} calls: {_format_func_name(func)}"
                )
            return None

        _print_top_functions(
            aggregated_stats,
            "Functions with Highest Per-Call Overhead",
            key_func=lambda x: x[3] / x[2] if x[2] > 0 else 0,
            format_line=format_overhead,
        )

        # Functions with highest cumulative impact
        def format_cumulative(stat):
            func, _, total_calls, _, cumulative_time, _ = stat
            if cumulative_time > 0:
                return (
                    f"{cumulative_time * cumulative_scale:.3f} {cumulative_unit} cumulative time, "
                    f"{(cumulative_time / total_calls) * cumulative_per_call_scale:.3f} {cumulative_per_call_unit} per call: "
                    f"{_format_func_name(func)}"
                )
            return None

        _print_top_functions(
            aggregated_stats,
            "Functions with Highest Cumulative Impact",
            key_func=lambda x: x[4],
            format_line=format_cumulative,
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
        print_sampled_stats(stats, sort, limit, show_summary)
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
            "  python -m profile.sample -a --sort-time 1234\n"
            "\n"
            "  # Profile for 1 minute with 1ms sampling interval\n"
            "  python -m profile.sample -i 1000 -d 60 1234\n"
            "\n"
            "  # Show only top 20 functions sorted by calls\n"
            "  python -m profile.sample --sort-calls -l 20 1234\n"
            "\n"
            "  # Profile all threads and save collapsed stacks\n"
            "  python -m profile.sample -a --collapsed -o stacks.txt 1234\n"
            "\n"
            "  # Profile with real-time sampling statistics\n"
            "  python -m profile.sample --realtime-stats 1234"
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
        "--sort-calls",
        action="store_const",
        const=0,
        dest="sort",
        help="Sort by number of calls",
    )
    sort_group.add_argument(
        "--sort-time",
        action="store_const",
        const=1,
        dest="sort",
        help="Sort by total time",
    )
    sort_group.add_argument(
        "--sort-cumulative",
        action="store_const",
        const=2,
        dest="sort",
        help="Sort by cumulative time (default)",
    )
    sort_group.add_argument(
        "--sort-percall",
        action="store_const",
        const=3,
        dest="sort",
        help="Sort by time per call",
    )
    sort_group.add_argument(
        "--sort-cumpercall",
        action="store_const",
        const=4,
        dest="sort",
        help="Sort by cumulative time per call",
    )
    sort_group.add_argument(
        "--sort-name",
        action="store_const",
        const=5,
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

    # Set sort value - use the specified sort or None if not specified
    # The default sort=2 is handled by the sample function itself
    sort_value = args.sort

    # Build the sample function call arguments
    sample_kwargs = {
        "sample_interval_usec": args.interval,
        "duration_sec": args.duration,
        "filename": args.outfile,
        "all_threads": args.all_threads,
        "limit": args.limit,
        "sort": sort_value,
        "show_summary": not args.no_summary,
        "output_format": args.format,
    }

    # Only add realtime_stats if it's explicitly set to True
    if args.realtime_stats:
        sample_kwargs["realtime_stats"] = args.realtime_stats

    sample(args.pid, **sample_kwargs)


if __name__ == "__main__":
    main()
