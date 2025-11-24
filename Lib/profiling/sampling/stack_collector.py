"""Stack trace collectors for Python profiling with optimized sampling analysis."""

import base64
import collections
import functools
import importlib.resources
import json
import linecache
import os
from pathlib import Path

from .collector import Collector
from .string_table import StringTable


class StackTraceCollector(Collector):
    def __init__(self, sample_interval_usec, *, skip_idle=False):
        self.sample_interval_usec = sample_interval_usec
        self.skip_idle = skip_idle

    def collect(self, stack_frames, skip_idle=False):
        """Collect and process stack frames from profiling data.

        Args:
            stack_frames: Stack frame data from profiler
            skip_idle: Override instance setting for skipping idle frames
        """
        effective_skip_idle = skip_idle if skip_idle is not None else self.skip_idle

        for frames, thread_id in self._iter_all_frames(stack_frames, skip_idle=effective_skip_idle):
            if not frames:
                continue

            try:
                self.process_frames(frames, thread_id)
            except Exception:
                # Silently continue processing other frames if one fails
                pass

    def process_frames(self, frames, thread_id):
        """Process a single set of stack frames.

        This method should be implemented by subclasses to perform
        the actual analysis of the stack frames.

        Args:
            frames: List of frame tuples (filename, lineno, funcname)
            thread_id: ID of the thread these frames came from
        """
        raise NotImplementedError("Subclasses must implement process_frames")


class CollapsedStackCollector(StackTraceCollector):
    """Collector that generates collapsed stack traces for flame graph generation.

    This collector aggregates stack traces by counting identical call stacks,
    producing output suitable for tools like FlameGraph.
    """

    def __init__(self, *args, **kwargs):
        """Initialize with a counter for stack traces."""
        super().__init__(*args, **kwargs)
        self.stack_counter = collections.Counter()

    def process_frames(self, frames, thread_id):
        """Process frames by building a collapsed stack representation.

        Args:
            frames: List of frame tuples (filename, lineno, funcname)
            thread_id: Thread ID for this stack trace
        """
        # Reverse frames to get root->leaf order for collapsed stacks
        call_tree = tuple(reversed(frames))
        self.stack_counter[(call_tree, thread_id)] += 1

    def export(self, filename):
        """Export collapsed stacks to a file.

        Args:
            filename: Path where to write the collapsed stack output
        """
        if not self.stack_counter:
            print("Warning: No stack data to export")
            return

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

        # Sort by count (descending) then by stack string for deterministic output
        lines.sort(key=lambda x: (-x[1], x[0]))

        try:
            with open(filename, "w", encoding='utf-8') as f:
                for stack, count in lines:
                    f.write(f"{stack} {count}\n")
            print(f"Collapsed stack output written to {filename}")
        except OSError as e:
            print(f"Error: Failed to write collapsed stack output: {e}")
            raise


class FlamegraphCollector(StackTraceCollector):
    """Collector that generates interactive flame graph visualizations.

    This collector builds a hierarchical representation of stack traces
    and generates self-contained HTML flame graphs using D3.js.
    """

    def __init__(self, *args, **kwargs):
        """Initialize the flame graph collector."""
        super().__init__(*args, **kwargs)
        self.stats = {}
        self._root = {"samples": 0, "children": {}, "threads": set()}
        self._total_samples = 0
        self._func_intern = {}  # Function interning for memory efficiency
        self._string_table = StringTable()
        self._all_threads = set()

    def set_stats(self, sample_interval_usec, duration_sec, sample_rate, error_rate=None):
        """Set profiling statistics to include in flame graph data.

        Args:
            sample_interval_usec: Sampling interval in microseconds
            duration_sec: Total profiling duration in seconds
            sample_rate: Effective sampling rate
            error_rate: Optional error rate during profiling
        """
        self.stats = {
            "sample_interval_usec": sample_interval_usec,
            "duration_sec": duration_sec,
            "sample_rate": sample_rate,
            "error_rate": error_rate
        }

    def process_frames(self, frames, thread_id):
        """Process frames by building a hierarchical call tree.

        Args:
            frames: List of frame tuples (filename, lineno, funcname)
            thread_id: Thread ID for this stack trace
        """
        # Reverse to root->leaf order for tree building
        call_tree = reversed(frames)
        self._root["samples"] += 1
        self._total_samples += 1
        self._root["threads"].add(thread_id)
        self._all_threads.add(thread_id)

        current = self._root
        for func in call_tree:
            # Intern function tuples to save memory
            func = self._func_intern.setdefault(func, func)
            children = current["children"]
            node = children.get(func)
            if node is None:
                node = {"samples": 0, "children": {}, "threads": set()}
                children[func] = node
            node["samples"] += 1
            node["threads"].add(thread_id)
            current = node

    def export(self, filename):
        """Export flame graph as a self-contained HTML file.

        Args:
            filename: Path where to write the HTML flame graph
        """
        try:
            flamegraph_data = self._convert_to_flamegraph_format()
            self._print_export_stats(flamegraph_data)

            if not flamegraph_data.get("children"):
                print("Warning: No functions found in profiling data. Check if sampling captured any data.")
                return

            html_content = self._create_flamegraph_html(flamegraph_data)

            with open(filename, "w", encoding="utf-8") as f:
                f.write(html_content)

            print(f"Flamegraph saved to: {filename}")

        except Exception as e:
            print(f"Error: Failed to export flame graph: {e}")
            raise

    def _print_export_stats(self, flamegraph_data):
        num_functions = len(flamegraph_data.get("children", []))
        total_time = flamegraph_data.get("value", 0)
        string_count = len(self._string_table)
        print(
            f"Flamegraph data: {num_functions} root functions, "
            f"total samples: {total_time}, {string_count} unique strings"
        )

    @staticmethod
    @functools.lru_cache(maxsize=None)
    def _format_function_name(func):
        filename, lineno, funcname = func

        # Special frames like <GC> and <native> should not show file:line
        if filename == "~" and lineno == 0:
            return funcname

        if len(filename) > 50:
            path_parts = filename.split("/")
            if len(path_parts) > 2:
                filename = f".../{'/'.join(path_parts[-2:])}"

        return f"{funcname} ({filename}:{lineno})"

    def _convert_to_flamegraph_format(self):
        """Convert aggregated trie to d3-flamegraph format with string table optimization."""
        if self._total_samples == 0:
            return {
                "name": self._string_table.intern("No Data"),
                "value": 0,
                "children": [],
                "threads": [],
                "strings": self._string_table.get_strings()
            }

        def convert_children(children, min_samples):
            out = []
            for func, node in children.items():
                samples = node["samples"]
                if samples < min_samples:
                    continue

                # Intern all string components for maximum efficiency
                filename_idx = self._string_table.intern(func[0])
                funcname_idx = self._string_table.intern(func[2])
                name_idx = self._string_table.intern(self._format_function_name(func))

                child_entry = {
                    "name": name_idx,
                    "value": samples,
                    "children": [],
                    "filename": filename_idx,
                    "lineno": func[1],
                    "funcname": funcname_idx,
                    "threads": sorted(list(node.get("threads", set()))),
                }

                source = self._get_source_lines(func)
                if source:
                    # Intern source lines for memory efficiency
                    source_indices = [self._string_table.intern(line) for line in source]
                    child_entry["source"] = source_indices

                # Recurse
                child_entry["children"] = convert_children(
                    node["children"], min_samples
                )
                out.append(child_entry)

            # Sort by value (descending) then by name index for consistent ordering
            out.sort(key=lambda x: (-x["value"], x["name"]))
            return out

        # Filter out very small functions (less than 0.1% of total samples)
        total_samples = self._total_samples
        min_samples = max(1, int(total_samples * 0.001))

        root_children = convert_children(self._root["children"], min_samples)
        if not root_children:
            return {
                "name": self._string_table.intern("No significant data"),
                "value": 0,
                "children": [],
                "strings": self._string_table.get_strings()
            }

        # If we only have one root child, make it the root to avoid redundant level
        if len(root_children) == 1:
            main_child = root_children[0]
            # Update the name to indicate it's the program root
            old_name = self._string_table.get_string(main_child["name"])
            new_name = f"Program Root: {old_name}"
            main_child["name"] = self._string_table.intern(new_name)
            main_child["stats"] = self.stats
            main_child["threads"] = sorted(list(self._all_threads))
            main_child["strings"] = self._string_table.get_strings()
            return main_child

        return {
            "name": self._string_table.intern("Program Root"),
            "value": total_samples,
            "children": root_children,
            "stats": self.stats,
            "threads": sorted(list(self._all_threads)),
            "strings": self._string_table.get_strings()
        }

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
        """Create self-contained HTML for the flame graph visualization.

        Args:
            data: Flame graph data structure

        Returns:
            str: Complete HTML content with inlined assets
        """
        data_json = json.dumps(data)
        template_dir = importlib.resources.files(__package__)
        flamegraph_assets_dir = template_dir / "_flamegraph_assets"

        # Load base template and assets
        html_template = (flamegraph_assets_dir / "flamegraph_template.html").read_text(encoding="utf-8")
        html_template = self._inline_first_party_assets(html_template, flamegraph_assets_dir)
        html_template = self._inline_vendor_assets(html_template, template_dir)
        html_template = self._inline_logo(html_template, template_dir)

        # Replace data placeholder
        return html_template.replace("{{FLAMEGRAPH_DATA}}", data_json)

    def _inline_first_party_assets(self, html_template, flamegraph_assets_dir):
        css_content = (flamegraph_assets_dir / "flamegraph.css").read_text(encoding="utf-8")
        js_content = (flamegraph_assets_dir / "flamegraph.js").read_text(encoding="utf-8")

        html_template = html_template.replace(
            "<!-- INLINE_CSS -->", f"<style>\n{css_content}\n</style>"
        )
        html_template = html_template.replace(
            "<!-- INLINE_JS -->", f"<script>\n{js_content}\n</script>"
        )
        return html_template

    def _inline_vendor_assets(self, html_template, template_dir):
        vendor_dir = template_dir / "_vendor"
        d3_flame_graph_dir = vendor_dir / "d3-flame-graph" / "4.1.3"

        # Load vendor assets
        d3_js = (vendor_dir / "d3" / "7.8.5" / "d3.min.js").read_text(encoding="utf-8")
        fg_css = (d3_flame_graph_dir / "d3-flamegraph.css").read_text(encoding="utf-8")
        fg_js = (d3_flame_graph_dir / "d3-flamegraph.min.js").read_text(encoding="utf-8")
        fg_tooltip_js = (d3_flame_graph_dir / "d3-flamegraph-tooltip.min.js").read_text(encoding="utf-8")

        # Inline vendor assets
        replacements = [
            ("<!-- INLINE_VENDOR_D3_JS -->", f"<script>\n{d3_js}\n</script>"),
            ("<!-- INLINE_VENDOR_FLAMEGRAPH_CSS -->", f"<style>\n{fg_css}\n</style>"),
            ("<!-- INLINE_VENDOR_FLAMEGRAPH_JS -->", f"<script>\n{fg_js}\n</script>"),
            ("<!-- INLINE_VENDOR_FLAMEGRAPH_TOOLTIP_JS -->", f"<script>\n{fg_tooltip_js}\n</script>"),
        ]

        for placeholder, replacement in replacements:
            html_template = html_template.replace(placeholder, replacement)

        return html_template

    def _inline_logo(self, html_template, template_dir):
        png_path = template_dir / "_assets" / "python-logo-only.png"
        b64_logo = base64.b64encode(png_path.read_bytes()).decode("ascii")
        logo_html = f'<img src="data:image/png;base64,{b64_logo}" alt="Python logo"/>'
        return html_template.replace("<!-- INLINE_LOGO -->", logo_html)
