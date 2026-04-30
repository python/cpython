import base64
import collections
import functools
import importlib.resources
import json
import linecache
import os
import sys
import sysconfig

from ._css_utils import get_combined_css
from .collector import Collector, extract_lineno
from .opcode_utils import get_opcode_mapping
from .string_table import StringTable
from .module_utils import extract_module_name, get_python_path_info


class StackTraceCollector(Collector):
    def __init__(self, sample_interval_usec, *, skip_idle=False):
        self.sample_interval_usec = sample_interval_usec
        self.skip_idle = skip_idle

    def collect(self, stack_frames, timestamps_us=None):
        weight = len(timestamps_us) if timestamps_us else 1
        for frames, thread_id in self._iter_stacks(stack_frames, skip_idle=self.skip_idle):
            self.process_frames(frames, thread_id, weight=weight)

    def process_frames(self, frames, thread_id, weight=1):
        pass


class CollapsedStackCollector(StackTraceCollector):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.stack_counter = collections.Counter()

    def process_frames(self, frames, thread_id, weight=1):
        # Extract only (filename, lineno, funcname) - opcode not needed for collapsed stacks
        # frame is (filename, location, funcname, opcode)
        call_tree = tuple(
            (f[0], extract_lineno(f[1]), f[2]) for f in reversed(frames)
        )
        self.stack_counter[(call_tree, thread_id)] += weight

    def export(self, filename):
        lines = []
        for (call_tree, thread_id), count in self.stack_counter.items():
            parts = [f"tid:{thread_id}"]
            for file, line, func in call_tree:
                # This is what pstats does for "special" frames:
                if file == "~" and line == 0:
                    part = func
                else:
                    part = f"{os.path.basename(file)}:{func}:{line}"
                parts.append(part)
            stack_str = ";".join(parts)
            lines.append((stack_str, count))

        lines.sort(key=lambda x: (-x[1], x[0]))

        with open(filename, "w") as f:
            for stack, count in lines:
                f.write(f"{stack} {count}\n")
        print(f"Collapsed stack output written to {filename}")


class FlamegraphCollector(StackTraceCollector):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.stats = {}
        self._root = {"samples": 0, "children": {}, "threads": set()}
        self._total_samples = 0
        self._sample_count = 0  # Track actual number of samples (not thread traces)
        self._func_intern = {}
        self._string_table = StringTable()
        self._module_cache = {}
        self._all_threads = set()

        # Thread status statistics (similar to LiveStatsCollector)
        self.thread_status_counts = {
            "has_gil": 0,
            "on_cpu": 0,
            "gil_requested": 0,
            "unknown": 0,
            "has_exception": 0,
            "total": 0,
        }
        self.samples_with_gc_frames = 0

        # Per-thread statistics
        self.per_thread_stats = {}  # {thread_id: {has_gil, on_cpu, gil_requested, unknown, has_exception, total, gc_samples}}

    def collect(self, stack_frames, timestamps_us=None):
        """Override to track thread status statistics before processing frames."""
        # Weight is number of timestamps (samples with identical stack)
        weight = len(timestamps_us) if timestamps_us else 1

        # Increment sample count by weight
        self._sample_count += weight

        # Collect both aggregate and per-thread statistics using base method
        status_counts, has_gc_frame, per_thread_stats = self._collect_thread_status_stats(stack_frames)

        # Merge aggregate status counts (multiply by weight)
        for key in status_counts:
            self.thread_status_counts[key] += status_counts[key] * weight

        # Update aggregate GC frame count
        if has_gc_frame:
            self.samples_with_gc_frames += weight

        # Merge per-thread statistics (multiply by weight)
        for thread_id, stats in per_thread_stats.items():
            if thread_id not in self.per_thread_stats:
                self.per_thread_stats[thread_id] = {
                    "has_gil": 0,
                    "on_cpu": 0,
                    "gil_requested": 0,
                    "unknown": 0,
                    "has_exception": 0,
                    "total": 0,
                    "gc_samples": 0,
                }
            for key, value in stats.items():
                self.per_thread_stats[thread_id][key] += value * weight

        # Call parent collect to process frames
        super().collect(stack_frames, timestamps_us)

    def set_stats(self, sample_interval_usec, duration_sec, sample_rate,
                  error_rate=None, missed_samples=None, mode=None):
        """Set profiling statistics to include in flamegraph data."""
        self.stats = {
            "sample_interval_usec": sample_interval_usec,
            "duration_sec": duration_sec,
            "sample_rate": sample_rate,
            "error_rate": error_rate,
            "missed_samples": missed_samples,
            "mode": mode
        }

    def export(self, filename):
        flamegraph_data = self._convert_to_flamegraph_format()

        # Debug output with string table statistics
        num_functions = len(flamegraph_data.get("children", []))
        total_time = flamegraph_data.get("value", 0)
        string_count = len(self._string_table)
        s1 = "" if num_functions == 1 else "s"
        s2 = "" if total_time == 1 else "s"
        s3 = "" if string_count == 1 else "s"
        print(
            f"Flamegraph data: {num_functions} root function{s1}, "
            f"{total_time} total sample{s2}, "
            f"{string_count} unique string{s3}"
        )

        if num_functions == 0:
            print(
                "Warning: No functions found in profiling data. Check if sampling captured any data."
            )
            return

        html_content = self._create_flamegraph_html(flamegraph_data)

        with open(filename, "w", encoding="utf-8") as f:
            f.write(html_content)

        print(f"Flamegraph saved to: {filename}")

    @staticmethod
    @functools.lru_cache(maxsize=None)
    def _format_function_name(func):
        filename, lineno, funcname = func

        # Special frames like <GC> and <native> should not show file:line
        if filename == "~" and lineno == 0:
            return funcname

        if len(filename) > 50:
            parts = filename.split("/")
            if len(parts) > 2:
                filename = f".../{'/'.join(parts[-2:])}"

        return f"{funcname} ({filename}:{lineno})"

    @staticmethod
    @functools.lru_cache(maxsize=None)
    def _format_module_name(func, module_name):
        filename, lineno, funcname = func

        # Special frames like <GC> and <native> should not show file:line
        if filename == "~" and lineno == 0:
            return funcname

        return f"{funcname} ({module_name}:{lineno})"

    def _get_module_name(self, filename, path_info):
        module_name = self._module_cache.get(filename)
        if module_name is None:
            module_name, _ = extract_module_name(filename, path_info)
            self._module_cache[filename] = module_name
        return module_name

    def _convert_to_flamegraph_format(self):
        if self._total_samples == 0:
            return {
                "name": self._string_table.intern("No Data"),
                "value": 0,
                "children": [],
                "threads": [],
                "strings": self._string_table.get_strings()
            }

        def convert_children(children, min_samples, path_info):
            out = []
            for func, node in children.items():
                samples = node["samples"]
                if samples < min_samples:
                    continue

                # Intern all string components for maximum efficiency
                filename_idx = self._string_table.intern(func[0])
                funcname_idx = self._string_table.intern(func[2])
                module_name = self._get_module_name(func[0], path_info)

                module_idx = self._string_table.intern(module_name)
                name_idx = self._string_table.intern(self._format_function_name(func))
                label_idx = self._string_table.intern(self._format_module_name(func, module_name))

                child_entry = {
                    "name": name_idx,
                    "label": label_idx,
                    "value": samples,
                    "self": node.get("self", 0),
                    "children": [],
                    "filename": filename_idx,
                    "module": module_idx,
                    "lineno": func[1],
                    "funcname": funcname_idx,
                    "threads": sorted(list(node.get("threads", set()))),
                }

                source = self._get_source_lines(func)
                if source:
                    # Intern source lines for memory efficiency
                    source_indices = [self._string_table.intern(line) for line in source]
                    child_entry["source"] = source_indices

                # Include opcode data if available
                opcodes = node.get("opcodes", {})
                if opcodes:
                    child_entry["opcodes"] = dict(opcodes)

                # Recurse
                child_entry["children"] = convert_children(
                    node["children"], min_samples, path_info
                )
                out.append(child_entry)

            # Sort by value (descending) then by name index for consistent ordering
            out.sort(key=lambda x: (-x["value"], x["name"]))
            return out

        # Filter out very small functions (less than 0.1% of total samples)
        total_samples = self._total_samples
        min_samples = max(1, int(total_samples * 0.001))
        path_info = get_python_path_info()

        root_children = convert_children(self._root["children"], min_samples, path_info)
        if not root_children:
            return {
                "name": self._string_table.intern("No significant data"),
                "value": 0,
                "children": [],
                "strings": self._string_table.get_strings()
            }

        # Calculate thread status percentages for display
        is_free_threaded = bool(sysconfig.get_config_var("Py_GIL_DISABLED"))
        total_threads = max(1, self.thread_status_counts["total"])
        thread_stats = {
            "has_gil_pct": (self.thread_status_counts["has_gil"] / total_threads) * 100,
            "on_cpu_pct": (self.thread_status_counts["on_cpu"] / total_threads) * 100,
            "gil_requested_pct": (self.thread_status_counts["gil_requested"] / total_threads) * 100,
            "has_exception_pct": (self.thread_status_counts["has_exception"] / total_threads) * 100,
            "gc_pct": (self.samples_with_gc_frames / max(1, self._sample_count)) * 100,
            "free_threaded": is_free_threaded,
            **self.thread_status_counts
        }

        # Calculate per-thread statistics with percentages
        per_thread_stats_with_pct = {}
        total_samples_denominator = max(1, self._sample_count)
        for thread_id, stats in self.per_thread_stats.items():
            total = max(1, stats["total"])
            per_thread_stats_with_pct[thread_id] = {
                "has_gil_pct": (stats["has_gil"] / total) * 100,
                "on_cpu_pct": (stats["on_cpu"] / total) * 100,
                "gil_requested_pct": (stats["gil_requested"] / total) * 100,
                "has_exception_pct": (stats["has_exception"] / total) * 100,
                "gc_pct": (stats["gc_samples"] / total_samples_denominator) * 100,
                **stats
            }

        # Build opcode mapping for JS
        opcode_mapping = get_opcode_mapping()

        # If we only have one root child, make it the root to avoid redundant level
        if len(root_children) == 1:
            main_child = root_children[0]
            # Update name and label to indicate it's the program root
            old_name = self._string_table.get_string(main_child["name"])
            main_child["name"] = self._string_table.intern(f"Program Root: {old_name}")
            old_label = self._string_table.get_string(main_child["label"])
            main_child["label"] = self._string_table.intern(f"Program Root: {old_label}")
            main_child["stats"] = {
                **self.stats,
                "thread_stats": thread_stats,
                "per_thread_stats": per_thread_stats_with_pct
            }
            main_child["threads"] = sorted(list(self._all_threads))
            main_child["strings"] = self._string_table.get_strings()
            main_child["opcode_mapping"] = opcode_mapping
            return main_child

        program_root_idx = self._string_table.intern("Program Root")
        return {
            "name": program_root_idx,
            "label": program_root_idx,
            "value": total_samples,
            "children": root_children,
            "stats": {
                **self.stats,
                "thread_stats": thread_stats,
                "per_thread_stats": per_thread_stats_with_pct
            },
            "threads": sorted(list(self._all_threads)),
            "strings": self._string_table.get_strings(),
            "opcode_mapping": opcode_mapping
        }

    def process_frames(self, frames, thread_id, weight=1):
        """Process stack frames into flamegraph tree structure.

        Args:
            frames: List of (filename, location, funcname, opcode) tuples in
                    leaf-to-root order. location is (lineno, end_lineno, col_offset, end_col_offset).
                    opcode is None if not gathered.
            thread_id: Thread ID for this stack trace
            weight: Number of samples this stack represents (for batched RLE)
        """
        # Reverse to root->leaf order for tree building
        self._root["samples"] += weight
        self._total_samples += weight
        self._root["threads"].add(thread_id)
        self._all_threads.add(thread_id)

        current = self._root
        for filename, location, funcname, opcode in reversed(frames):
            lineno = extract_lineno(location)
            func = (filename, lineno, funcname)
            func = self._func_intern.setdefault(func, func)

            node = current["children"].get(func)
            if node is None:
                node = {"samples": 0, "children": {}, "threads": set(), "opcodes": collections.Counter(), "self": 0}
                current["children"][func] = node
            node["samples"] += weight
            node["threads"].add(thread_id)

            if opcode is not None:
                node["opcodes"][opcode] += weight

            current = node

        if current is not self._root:
            current["self"] += weight

    def _get_source_lines(self, func):
        filename, lineno, _ = func

        try:
            lines = []
            start_line = max(1, lineno - 2)
            end_line = lineno + 3

            for line_num in range(start_line, end_line):
                line = linecache.getline(filename, line_num)
                if line.strip():
                    marker = "→ " if line_num == lineno else "  "
                    lines.append(f"{marker}{line_num}: {line.rstrip()}")

            return lines if lines else None

        except Exception:
            return None

    def _create_flamegraph_html(self, data):
        data_json = json.dumps(data)

        template_dir = importlib.resources.files(__package__)
        vendor_dir = template_dir / "_vendor"
        assets_dir = template_dir / "_assets"

        d3_path = vendor_dir / "d3" / "7.8.5" / "d3.min.js"
        d3_flame_graph_dir = vendor_dir /  "d3-flame-graph" / "4.1.3"
        fg_css_path = d3_flame_graph_dir / "d3-flamegraph.css"
        fg_js_path = d3_flame_graph_dir / "d3-flamegraph.min.js"
        fg_tooltip_js_path = d3_flame_graph_dir / "d3-flamegraph-tooltip.min.js"

        html_template = (template_dir / "_flamegraph_assets" / "flamegraph_template.html").read_text(encoding="utf-8")
        css_content = get_combined_css("flamegraph")
        base_js = (template_dir / "_shared_assets" / "base.js").read_text(encoding="utf-8")
        component_js = (template_dir / "_flamegraph_assets" / "flamegraph.js").read_text(encoding="utf-8")
        js_content = f"{base_js}\n{component_js}"

        # Set title and subtitle based on whether this is a differential flamegraph
        is_differential = data.get("stats", {}).get("is_differential", False)
        if is_differential:
            title = "Tachyon Profiler - Differential Flamegraph Report"
            subtitle = "Differential Flamegraph Report"
        else:
            title = "Tachyon Profiler - Flamegraph Report"
            subtitle = "Flamegraph Report"

        html_template = html_template.replace("{{TITLE}}", title)
        html_template = html_template.replace("{{SUBTITLE}}", subtitle)

        # Inline first-party CSS/JS
        html_template = html_template.replace(
            "<!-- INLINE_CSS -->", f"<style>\n{css_content}\n</style>"
        )
        html_template = html_template.replace(
            "<!-- INLINE_JS -->", f"<script>\n{js_content}\n</script>"
        )

        png_path = assets_dir / "tachyon-logo.png"
        b64_logo = base64.b64encode(png_path.read_bytes()).decode("ascii")

        # Let CSS control size; keep markup simple
        logo_html = f'<img src="data:image/png;base64,{b64_logo}" alt="Tachyon logo"/>'
        html_template = html_template.replace("<!-- INLINE_LOGO -->", logo_html)
        html_template = html_template.replace(
            "<!-- PYTHON_VERSION -->", f"{sys.version_info.major}.{sys.version_info.minor}"
        )

        d3_js = d3_path.read_text(encoding="utf-8")
        fg_css = fg_css_path.read_text(encoding="utf-8")
        fg_js = fg_js_path.read_text(encoding="utf-8")
        fg_tooltip_js = fg_tooltip_js_path.read_text(encoding="utf-8")

        html_template = html_template.replace(
            "<!-- INLINE_VENDOR_D3_JS -->",
            f"<script>\n{d3_js}\n</script>",
        )
        html_template = html_template.replace(
            "<!-- INLINE_VENDOR_FLAMEGRAPH_CSS -->",
            f"<style>\n{fg_css}\n</style>",
        )
        html_template = html_template.replace(
            "<!-- INLINE_VENDOR_FLAMEGRAPH_JS -->",
            f"<script>\n{fg_js}\n</script>",
        )
        html_template = html_template.replace(
            "<!-- INLINE_VENDOR_FLAMEGRAPH_TOOLTIP_JS -->",
            f"<script>\n{fg_tooltip_js}\n</script>",
        )

        # Replace the placeholder with actual data
        html_content = html_template.replace(
            "{{FLAMEGRAPH_DATA}}", data_json
        )

        return html_content


class DiffFlamegraphCollector(FlamegraphCollector):
    """Differential flamegraph collector that compares against a baseline binary profile."""

    def __init__(self, sample_interval_usec, *, baseline_binary_path, skip_idle=False):
        super().__init__(sample_interval_usec, skip_idle=skip_idle)
        if not os.path.exists(baseline_binary_path):
            raise ValueError(f"Baseline file not found: {baseline_binary_path}")
        self.baseline_binary_path = baseline_binary_path
        self._baseline_collector = None
        self._elided_paths = set()

    def _load_baseline(self):
        """Load baseline profile from binary file."""
        from .binary_reader import BinaryReader

        with BinaryReader(self.baseline_binary_path) as reader:
            info = reader.get_info()

            baseline_collector = FlamegraphCollector(
                sample_interval_usec=info['sample_interval_us'],
                skip_idle=self.skip_idle
            )

            reader.replay_samples(baseline_collector)

        self._baseline_collector = baseline_collector

    def _aggregate_path_samples(self, root_node, path=None):
        """Aggregate samples by stack path, excluding line numbers for cross-profile matching."""
        if path is None:
            path = ()

        stats = {}

        for func, node in root_node["children"].items():
            filename, _lineno, funcname = func
            func_key = (filename, funcname)
            path_key = path + (func_key,)

            total_samples = node.get("samples", 0)
            self_samples = node.get("self", 0)

            if path_key in stats:
                stats[path_key]["total"] += total_samples
                stats[path_key]["self"] += self_samples
            else:
                stats[path_key] = {
                    "total": total_samples,
                    "self": self_samples
                }

            child_stats = self._aggregate_path_samples(node, path_key)
            for key, data in child_stats.items():
                if key in stats:
                    stats[key]["total"] += data["total"]
                    stats[key]["self"] += data["self"]
                else:
                    stats[key] = data

        return stats

    def _convert_to_flamegraph_format(self):
        """Convert to flamegraph format with differential annotations."""
        if self._baseline_collector is None:
            self._load_baseline()

        current_flamegraph = super()._convert_to_flamegraph_format()

        current_stats = self._aggregate_path_samples(self._root)
        baseline_stats = self._aggregate_path_samples(self._baseline_collector._root)

        # Scale baseline values to make them comparable, accounting for both
        # sample count differences and sample interval differences.
        baseline_total = self._baseline_collector._total_samples
        if baseline_total > 0 and self._total_samples > 0:
            current_time = self._total_samples * self.sample_interval_usec
            baseline_time = baseline_total * self._baseline_collector.sample_interval_usec
            scale = current_time / baseline_time
        elif baseline_total > 0:
            # Current profile is empty - use interval-based scale for elided display
            scale = self.sample_interval_usec / self._baseline_collector.sample_interval_usec
        else:
            scale = 1.0

        self._annotate_nodes_with_diff(current_flamegraph, current_stats, baseline_stats, scale)
        self._add_elided_flamegraph(current_flamegraph, current_stats, baseline_stats, scale)

        return current_flamegraph

    def _annotate_nodes_with_diff(self, current_flamegraph, current_stats, baseline_stats, scale):
        """Annotate each node in the tree with diff metadata."""
        if "stats" not in current_flamegraph:
            current_flamegraph["stats"] = {}

        current_flamegraph["stats"]["baseline_samples"] = self._baseline_collector._total_samples
        current_flamegraph["stats"]["current_samples"] = self._total_samples
        current_flamegraph["stats"]["baseline_scale"] = scale
        current_flamegraph["stats"]["is_differential"] = True

        if self._is_promoted_root(current_flamegraph):
            self._add_diff_data_to_node(current_flamegraph, (), current_stats, baseline_stats, scale)
        else:
            for child in current_flamegraph["children"]:
                self._add_diff_data_to_node(child, (), current_stats, baseline_stats, scale)

    def _add_diff_data_to_node(self, node, path, current_stats, baseline_stats, scale):
        """Recursively add diff metadata to nodes."""
        func_key = self._extract_func_key(node, self._string_table)
        path_key = path + (func_key,) if func_key else path

        current_data = current_stats.get(path_key, {"total": 0, "self": 0})
        baseline_data = baseline_stats.get(path_key, {"total": 0, "self": 0})

        current_self = current_data["self"]
        baseline_self = baseline_data["self"] * scale
        baseline_total = baseline_data["total"] * scale

        diff = current_self - baseline_self
        if baseline_self > 0:
            diff_pct = (diff / baseline_self) * 100.0
        elif current_self > 0:
            diff_pct = 100.0
        else:
            diff_pct = 0.0

        node["baseline"] = baseline_self
        node["baseline_total"] = baseline_total
        node["self_time"] = current_self
        node["diff"] = diff
        node["diff_pct"] = diff_pct

        if "children" in node and node["children"]:
            for child in node["children"]:
                self._add_diff_data_to_node(child, path_key, current_stats, baseline_stats, scale)

    def _is_promoted_root(self, data):
        """Check if the data represents a promoted root node."""
        return "filename" in data and "funcname" in data

    def _add_elided_flamegraph(self, current_flamegraph, current_stats, baseline_stats, scale):
        """Calculate elided paths and add elided flamegraph to stats."""
        self._elided_paths = baseline_stats.keys() - current_stats.keys()

        current_flamegraph["stats"]["elided_count"] = len(self._elided_paths)

        if self._elided_paths:
            elided_flamegraph = self._build_elided_flamegraph(baseline_stats, scale)
            if elided_flamegraph:
                current_flamegraph["stats"]["elided_flamegraph"] = elided_flamegraph

    def _build_elided_flamegraph(self, baseline_stats, scale):
        """Build flamegraph containing only elided paths from baseline.

        This re-runs the base conversion pipeline on the baseline collector
        to produce a complete formatted flamegraph, then prunes it to keep
        only elided paths.
        """
        if not self._baseline_collector or not self._elided_paths:
            return None

        # Suppress source line collection for elided nodes - these functions
        # no longer exist in the current profile, so source lines from the
        # current machine's filesystem would be misleading or unavailable.
        orig_get_source = self._baseline_collector._get_source_lines
        self._baseline_collector._get_source_lines = lambda func: None
        try:
            baseline_data = self._baseline_collector._convert_to_flamegraph_format()
        finally:
            self._baseline_collector._get_source_lines = orig_get_source

        # Remove non-elided nodes and recalculate values
        if not self._extract_elided_nodes(baseline_data, path=()):
            return None

        self._add_elided_metadata(baseline_data, baseline_stats, scale, path=())

        # Merge only profiling metadata, not thread-level stats
        for key in ("sample_interval_usec", "duration_sec", "sample_rate",
                    "error_rate", "missed_samples", "mode"):
            if key in self.stats:
                baseline_data["stats"][key] = self.stats[key]
        baseline_data["stats"]["is_differential"] = True
        baseline_data["stats"]["baseline_samples"] = self._baseline_collector._total_samples
        baseline_data["stats"]["current_samples"] = self._total_samples

        return baseline_data

    def _extract_elided_nodes(self, node, path):
        """Remove non-elided nodes and recalculate values bottom-up."""
        if not node:
            return False

        func_key = self._extract_func_key(node, self._baseline_collector._string_table)
        current_path = path + (func_key,) if func_key else path

        is_elided = current_path in self._elided_paths if func_key else False

        if "children" in node:
            # Filter children, keeping only those with elided descendants
            elided_children = []
            total_value = 0
            for child in node["children"]:
                if self._extract_elided_nodes(child, current_path):
                    elided_children.append(child)
                    total_value += child.get("value", 0)
            node["children"] = elided_children

            # Recalculate value for structural (non-elided) ancestor nodes;
            # elided nodes keep their original value to preserve self-samples
            if elided_children and not is_elided:
                node["value"] = total_value

        # Keep this node if it's elided or has elided descendants
        return is_elided or bool(node.get("children"))

    def _add_elided_metadata(self, node, baseline_stats, scale, path):
        """Add differential metadata showing this path disappeared."""
        if not node:
            return

        func_key = self._extract_func_key(node, self._baseline_collector._string_table)
        current_path = path + (func_key,) if func_key else path

        if func_key and current_path in baseline_stats:
            baseline_data = baseline_stats[current_path]
            baseline_self = baseline_data["self"] * scale
            baseline_total = baseline_data["total"] * scale

            node["baseline"] = baseline_self
            node["baseline_total"] = baseline_total
            node["diff"] = -baseline_self
        else:
            node["baseline"] = 0
            node["baseline_total"] = 0
            node["diff"] = 0

        node["self_time"] = 0
        # Elided paths have zero current self-time, so the change is always
        # -100% when there was actual baseline self-time to lose.
        # For internal nodes with no baseline self-time, use 0% to avoid
        # misleading tooltips.
        if baseline_self > 0:
            node["diff_pct"] = -100.0
        else:
            node["diff_pct"] = 0.0

        if "children" in node and node["children"]:
            for child in node["children"]:
                self._add_elided_metadata(child, baseline_stats, scale, current_path)

    def _extract_func_key(self, node, string_table):
        """Extract (filename, funcname) key from node, excluding line numbers.

        Line numbers are excluded to match functions even if they moved.
        Returns None for root nodes that don't have function information.
        """
        if "filename" not in node or "funcname" not in node:
            return None
        filename = string_table.get_string(node["filename"])
        funcname = string_table.get_string(node["funcname"])
        return (filename, funcname)
