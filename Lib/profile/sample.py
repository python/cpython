import argparse
import _colorize
import _remote_debugging
import pstats
import time
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

    def sample(self, collector, duration_sec=10):
        sample_interval_sec = self.sample_interval_usec / 1_000_000
        running_time = 0
        num_samples = 0
        errors = 0
        start_time = next_time = time.perf_counter()
        while running_time < duration_sec:
            if next_time < time.perf_counter():
                try:
                    stack_frames = self.unwinder.get_stack_trace()
                    collector.collect(stack_frames)
                except (RuntimeError, UnicodeDecodeError, OSError):
                    errors += 1

                num_samples += 1
                next_time += sample_interval_sec

            running_time = time.perf_counter() - start_time

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


def print_sampled_stats(stats, sort=-1, limit=None, show_summary=True):
    if not isinstance(sort, tuple):
        sort = (sort,)

    # Get the stats data
    stats_list = []
    for func, (cc, nc, tt, ct, callers) in stats.stats.items():
        stats_list.append((func, cc, nc, tt, ct, callers))

    # Sort based on the requested field
    sort_field = sort[0]
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

    # Find the maximum values for each column to determine units
    max_tt = max((tt for _, _, _, tt, _, _ in stats_list), default=0)
    max_ct = max((ct for _, _, _, _, ct, _ in stats_list), default=0)

    # Determine appropriate units and format strings
    if max_tt >= 1.0:
        tt_unit = "s"
        tt_scale = 1.0
    elif max_tt >= 0.001:
        tt_unit = "ms"
        tt_scale = 1000.0
    else:
        tt_unit = "μs"
        tt_scale = 1000000.0

    if max_ct >= 1.0:
        ct_unit = "s"
        ct_scale = 1.0
    elif max_ct >= 0.001:
        ct_unit = "ms"
        ct_scale = 1000.0
    else:
        ct_unit = "μs"
        ct_scale = 1000000.0

    # Print header with colors and units
    header = (
        f"{ANSIColors.BOLD_BLUE}Profile Stats:{ANSIColors.RESET}\n"
        f"{ANSIColors.BOLD_BLUE}nsamples{ANSIColors.RESET} "
        f"{ANSIColors.BOLD_BLUE}tottime ({tt_unit}){ANSIColors.RESET} "
        f"{ANSIColors.BOLD_BLUE}persample ({tt_unit}){ANSIColors.RESET} "
        f"{ANSIColors.BOLD_BLUE}cumtime ({ct_unit}){ANSIColors.RESET} "
        f"{ANSIColors.BOLD_BLUE}persample ({ct_unit}){ANSIColors.RESET} "
        f"{ANSIColors.BOLD_BLUE}filename:lineno(function){ANSIColors.RESET}"
    )
    print(header)

    # Print each line with colors
    for func, cc, nc, tt, ct, callers in stats_list:
        if nc != cc:
            ncalls = f"{nc}/{cc}"
        else:
            ncalls = str(nc)

        # Format numbers with proper alignment and precision (no colors)
        tottime = f"{tt * tt_scale:8.3f}"
        percall = f"{(tt / nc) * tt_scale:8.3f}" if nc > 0 else "    N/A"
        cumtime = f"{ct * ct_scale:8.3f}"
        cumpercall = f"{(ct / nc) * ct_scale:8.3f}" if nc > 0 else "    N/A"

        # Format the function name with colors
        func_name = (
            f"{ANSIColors.GREEN}{func[0]}{ANSIColors.RESET}:"
            f"{ANSIColors.YELLOW}{func[1]}{ANSIColors.RESET}("
            f"{ANSIColors.CYAN}{func[2]}{ANSIColors.RESET})"
        )

        # Print the formatted line
        print(
            f"{ncalls:>8}  {tottime}    {percall}        {cumtime}    {cumpercall}        {func_name}"
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

        # Most time-consuming functions (by total time)
        def format_time_consuming(stat):
            func, _, total_calls, total_time, _, _ = stat
            if total_time > 0:
                return (
                    f"{total_time * tt_scale:8.3f} {tt_unit} total time, "
                    f"{(total_time / total_calls) * tt_scale:8.3f} {tt_unit} per call: {_format_func_name(func)}"
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
                    f"{total_calls:8d} calls, {(total_time / total_calls) * tt_scale:8.3f} {tt_unit} "
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
                    f"{(total_time / total_calls) * tt_scale:8.3f} {tt_unit} per call, "
                    f"{total_calls:8d} calls: {_format_func_name(func)}"
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
                    f"{cumulative_time * ct_scale:8.3f} {ct_unit} cumulative time, "
                    f"{(cumulative_time / total_calls) * ct_scale:8.3f} {ct_unit} per call: "
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
    sort=-1,
    sample_interval_usec=100,
    duration_sec=10,
    filename=None,
    all_threads=False,
    limit=None,
    show_summary=True,
    output_format="pstats",
):
    profiler = SampleProfiler(
        pid, sample_interval_usec, all_threads=all_threads
    )

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
            "  python -m profile.sample -a --collapsed -o stacks.txt 1234"
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
        default=2,
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

    sample(
        args.pid,
        sample_interval_usec=args.interval,
        duration_sec=args.duration,
        filename=args.outfile,
        all_threads=args.all_threads,
        limit=args.limit,
        sort=args.sort,
        show_summary=not args.no_summary,
        output_format=args.format,
    )


if __name__ == "__main__":
    main()
