import collections
import marshal

from _colorize import ANSIColors
from .collector import Collector


class PstatsCollector(Collector):
    def __init__(self, sample_interval_usec, *, skip_idle=False):
        self.result = collections.defaultdict(
            lambda: dict(total_rec_calls=0, direct_calls=0, cumulative_calls=0)
        )
        self.stats = {}
        self.sample_interval_usec = sample_interval_usec
        self.callers = collections.defaultdict(
            lambda: collections.defaultdict(int)
        )
        self.skip_idle = skip_idle

    def _process_frames(self, frames):
        """Process a single thread's frame stack."""
        if not frames:
            return

        # Process each frame in the stack to track cumulative calls
        for frame in frames:
            location = (frame.filename, frame.lineno, frame.funcname)
            self.result[location]["cumulative_calls"] += 1

        # The top frame gets counted as an inline call (directly executing)
        top_location = (frames[0].filename, frames[0].lineno, frames[0].funcname)
        self.result[top_location]["direct_calls"] += 1

        # Track caller-callee relationships for call graph
        for i in range(1, len(frames)):
            callee_frame = frames[i - 1]
            caller_frame = frames[i]

            callee = (callee_frame.filename, callee_frame.lineno, callee_frame.funcname)
            caller = (caller_frame.filename, caller_frame.lineno, caller_frame.funcname)

            self.callers[callee][caller] += 1

    def collect(self, stack_frames):
        for frames, thread_id in self._iter_all_frames(stack_frames, skip_idle=self.skip_idle):
            self._process_frames(frames)

    def export(self, filename):
        self.create_stats()
        self._dump_stats(filename)

    def _dump_stats(self, file):
        stats_with_marker = dict(self.stats)
        stats_with_marker[("__sampled__",)] = True
        with open(file, "wb") as f:
            marshal.dump(stats_with_marker, f)

    # Needed for compatibility with pstats.Stats
    def create_stats(self):
        sample_interval_sec = self.sample_interval_usec / 1_000_000
        callers = {}
        for fname, call_counts in self.result.items():
            total = call_counts["direct_calls"] * sample_interval_sec
            cumulative_calls = call_counts["cumulative_calls"]
            cumulative = cumulative_calls * sample_interval_sec
            callers = dict(self.callers.get(fname, {}))
            self.stats[fname] = (
                call_counts["direct_calls"],  # cc = direct calls for sample percentage
                cumulative_calls,  # nc = cumulative calls for cumulative percentage
                total,
                cumulative,
                callers,
            )

    def print_stats(self, sort=-1, limit=None, show_summary=True, mode=None):
        """Print formatted statistics to stdout."""
        import pstats
        from .constants import PROFILING_MODE_CPU

        # Create stats object
        stats = pstats.SampledStats(self).strip_dirs()
        if not stats.stats:
            print("No samples were collected.")
            if mode == PROFILING_MODE_CPU:
                print("This can happen in CPU mode when all threads are idle.")
            return

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

        total_time_unit, total_time_scale = self._determine_best_unit(max_total_time)
        cumulative_time_unit, cumulative_time_scale = self._determine_best_unit(
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

        # Print summary of interesting functions if enabled
        if show_summary and stats_list:
            self._print_summary(stats_list, total_samples)

    @staticmethod
    def _determine_best_unit(max_value):
        """Determine the best unit (s, ms, μs) and scale factor for a maximum value."""
        if max_value >= 1.0:
            return "s", 1.0
        elif max_value >= 0.001:
            return "ms", 1000.0
        else:
            return "μs", 1000000.0

    def _print_summary(self, stats_list, total_samples):
        """Print summary of interesting functions."""
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

        total_unit, total_scale = self._determine_best_unit(max_total_time)
        cumulative_unit, cumulative_scale = self._determine_best_unit(
            max_cumulative_time
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
