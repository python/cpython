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

        # Most time-consuming functions (by total time)
        def format_time_consuming(stat):
            func, _, nc, tt, _, _ = stat
            if tt > 0:
                return (
                    f"{tt * tt_scale:8.3f} {tt_unit} total time, "
                    f"{(tt / nc) * tt_scale:8.3f} {tt_unit} per call: {_format_func_name(func)}"
                )
            return None

        _print_top_functions(
            stats_list,
            "Most Time-Consuming Functions",
            key_func=lambda x: x[3],
            format_line=format_time_consuming,
        )

        # Most called functions
        def format_most_called(stat):
            func, _, nc, tt, _, _ = stat
            if nc > 0:
                return (
                    f"{nc:8d} calls, {(tt / nc) * tt_scale:8.3f} {tt_unit} "
                    f"per call: {_format_func_name(func)}"
                )
            return None

        _print_top_functions(
            stats_list,
            "Most Called Functions",
            key_func=lambda x: x[2],
            format_line=format_most_called,
        )

        # Functions with highest per-call overhead
        def format_overhead(stat):
            func, _, nc, tt, _, _ = stat
            if nc > 0 and tt > 0:
                return (
                    f"{(tt / nc) * tt_scale:8.3f} {tt_unit} per call, "
                    f"{nc:8d} calls: {_format_func_name(func)}"
                )
            return None

        _print_top_functions(
            stats_list,
            "Functions with Highest Per-Call Overhead",
            key_func=lambda x: x[3] / x[2] if x[2] > 0 else 0,
            format_line=format_overhead,
        )

        # Functions with highest cumulative impact
        def format_cumulative(stat):
            func, _, nc, _, ct, _ = stat
            if ct > 0:
                return (
                    f"{ct * ct_scale:8.3f} {ct_unit} cumulative time, "
                    f"{(ct / nc) * ct_scale:8.3f} {ct_unit} per call: "
                    f"{_format_func_name(func)}"
                )
            return None

        _print_top_functions(
            stats_list,
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


def main():
    parser = argparse.ArgumentParser(
        description=(
            "Sample a process's stack frames.\n\n"
            "Sort options:\n"
            "  --sort-calls      Sort by number of calls (most called functions first)\n"
            "  --sort-time       Sort by total time (most time-consuming functions first)\n"
            "  --sort-cumulative Sort by cumulative time (functions with highest total impact first)\n"
            "  --sort-percall    Sort by time per call (functions with highest per-call overhead first)\n"
            "  --sort-cumpercall Sort by cumulative time per call (functions with highest cumulative overhead per call)\n"
            "  --sort-name       Sort by function name (alphabetical order)\n\n"
            "The default sort is by cumulative time (--sort-cumulative)."
            "Format descriptions:\n"
            "  pstats     Standard Python profiler output format\n"
            "  collapsed  Stack traces in collapsed format (file:function:line;file:function:line;... count)\n"
            "             Useful for generating flamegraphs with tools like flamegraph.pl"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        color=True,
    )
    parser.add_argument("pid", type=int, help="Process ID to sample.")
    parser.add_argument(
        "-i",
        "--interval",
        type=int,
        default=10,
        help="Sampling interval in microseconds (default: 10 usec)",
    )
    parser.add_argument(
        "-d",
        "--duration",
        type=int,
        default=10,
        help="Sampling duration in seconds (default: 10 seconds)",
    )
    parser.add_argument(
        "-a",
        "--all-threads",
        action="store_true",
        help="Sample all threads in the process",
    )
    parser.add_argument("-o", "--outfile", help="Save stats to <outfile>")
    parser.add_argument(
        "--no-color",
        action="store_true",
        help="Disable color output",
    )
    parser.add_argument(
        "-l",
        "--limit",
        type=int,
        help="Limit the number of rows in the output",
    )
    parser.add_argument(
        "--no-summary",
        action="store_true",
        help="Disable the summary section at the end of the output",
    )
    parser.add_argument(
        "--format",
        choices=["pstats", "collapsed"],
        default="pstats",
        help="Output format (default: pstats)",
    )

    # Add sorting options
    sort_group = parser.add_mutually_exclusive_group()
    sort_group.add_argument(
        "--sort-calls",
        action="store_const",
        const=0,
        dest="sort",
        help="Sort by number of calls (most called functions first)",
    )
    sort_group.add_argument(
        "--sort-time",
        action="store_const",
        const=1,
        dest="sort",
        help="Sort by total time (most time-consuming functions first)",
    )
    sort_group.add_argument(
        "--sort-cumulative",
        action="store_const",
        const=2,
        dest="sort",
        help="Sort by cumulative time (functions with highest total impact first)",
    )
    sort_group.add_argument(
        "--sort-percall",
        action="store_const",
        const=3,
        dest="sort",
        help="Sort by time per call (functions with highest per-call overhead first)",
    )
    sort_group.add_argument(
        "--sort-cumpercall",
        action="store_const",
        const=4,
        dest="sort",
        help="Sort by cumulative time per call (functions with highest cumulative overhead per call)",
    )
    sort_group.add_argument(
        "--sort-name",
        action="store_const",
        const=5,
        dest="sort",
        help="Sort by function name (alphabetical order)",
    )

    # Set default sort to cumulative time
    parser.set_defaults(sort=2)

    args = parser.parse_args()

    # Set color theme based on --no-color flag
    if args.no_color:
        _colorize.set_theme(_colorize.theme_no_color)

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
