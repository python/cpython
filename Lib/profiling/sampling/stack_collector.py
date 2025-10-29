"""Stack trace collectors for Python profiling with optimized sampling analysis."""

import base64
import collections
import functools
import importlib.resources
import json
import linecache
import os
import platform
import site
import sys
from pathlib import Path

from .collector import Collector
from .string_table import StringTable


def get_python_path_info():
    """Get information about Python installation paths for module extraction.

    Returns:
        dict: Dictionary containing stdlib path, site-packages paths, and sys.path entries.
    """
    info = {
        'stdlib': None,
        'site_packages': [],
        'sys_path': []
    }

    # Get standard library path from os module location
    try:
        if hasattr(os, '__file__') and os.__file__:
            info['stdlib'] = Path(os.__file__).parent
    except (AttributeError, OSError):
        pass  # Silently continue if we can't determine stdlib path

    # Get site-packages directories
    site_packages = []
    try:
        site_packages.extend(Path(p) for p in site.getsitepackages())
    except (AttributeError, OSError):
        pass  # Continue without site packages if unavailable

    # Get user site-packages
    try:
        user_site = site.getusersitepackages()
        if user_site and Path(user_site).exists():
            site_packages.append(Path(user_site))
    except (AttributeError, OSError):
        pass  # Continue without user site packages

    info['site_packages'] = site_packages
    info['sys_path'] = [Path(p) for p in sys.path if p]

    return info


def extract_module_name(filename, path_info):
    """Extract Python module name and type from file path.

    Args:
        filename: Path to the Python file
        path_info: Dictionary from get_python_path_info()

    Returns:
        tuple: (module_name, module_type) where module_type is one of:
               'stdlib', 'site-packages', 'project', or 'other'
    """
    if not filename:
        return ('unknown', 'other')

    try:
        file_path = Path(filename)
    except (ValueError, OSError):
        return (str(filename), 'other')

    # Check if it's in stdlib
    if path_info['stdlib'] and _is_subpath(file_path, path_info['stdlib']):
        try:
            rel_path = file_path.relative_to(path_info['stdlib'])
            return (_path_to_module(rel_path), 'stdlib')
        except ValueError:
            pass

    # Check site-packages
    for site_pkg in path_info['site_packages']:
        if _is_subpath(file_path, site_pkg):
            try:
                rel_path = file_path.relative_to(site_pkg)
                return (_path_to_module(rel_path), 'site-packages')
            except ValueError:
                continue

    # Check other sys.path entries (project files)
    if not str(file_path).startswith(('<', '[')):  # Skip special files
        for path_entry in path_info['sys_path']:
            if _is_subpath(file_path, path_entry):
                try:
                    rel_path = file_path.relative_to(path_entry)
                    return (_path_to_module(rel_path), 'project')
                except ValueError:
                    continue

    # Fallback: just use the filename
    return (_path_to_module(file_path), 'other')


def _is_subpath(file_path, parent_path):
    try:
        file_path.relative_to(parent_path)
        return True
    except (ValueError, OSError):
        return False


def _path_to_module(path):
    if isinstance(path, str):
        path = Path(path)

    # Remove .py extension
    if path.suffix == '.py':
        path = path.with_suffix('')

    # Convert path separators to dots
    parts = path.parts

    # Handle __init__ files - they represent the package itself
    if parts and parts[-1] == '__init__':
        parts = parts[:-1]

    return '.'.join(parts) if parts else path.stem


class StackTraceCollector(Collector):
    """Base class for stack trace analysis collectors.

    This abstract base class provides common functionality for processing
    stack traces from profiling data. Subclasses should implement the
    process_frames method to handle the actual analysis.

    Args:
        skip_idle: If True, skip idle threads in frame processing
    """

    def __init__(self, *, skip_idle=False):
        """Initialize the collector with optional idle thread skipping."""
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
            # Format as semicolon-separated stack with thread info
            stack_str = ";".join(
                f"{os.path.basename(frame[0])}:{frame[2]}:{frame[1]}"
                for frame in call_tree
            )
            lines.append((f"tid:{thread_id};{stack_str}", count))

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

        # Optimize path display for long filenames
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

        # Load base template and assets
        html_template = (template_dir / "flamegraph_template.html").read_text(encoding="utf-8")
        html_template = self._inline_first_party_assets(html_template, template_dir)
        html_template = self._inline_vendor_assets(html_template, template_dir)
        html_template = self._inline_logo(html_template, template_dir)

        # Replace data placeholder
        return html_template.replace("{{FLAMEGRAPH_DATA}}", data_json)

    def _inline_first_party_assets(self, html_template, template_dir):
        css_content = (template_dir / "flamegraph.css").read_text(encoding="utf-8")
        js_content = (template_dir / "flamegraph.js").read_text(encoding="utf-8")

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


class HeatmapCollector(StackTraceCollector):
    """Collector that generates coverage.py-style heatmap HTML output with line intensity.

    This collector creates detailed HTML reports showing which lines of code
    were executed most frequently during profiling, similar to coverage.py
    but showing execution "heat" rather than just coverage.
    """

    def __init__(self, *args, **kwargs):
        """Initialize the heatmap collector with data structures for analysis."""
        super().__init__(*args, **kwargs)

        # Sample counting data structures
        self.line_samples = collections.Counter()  # (filename, lineno) -> count
        self.file_samples = collections.defaultdict(collections.Counter)  # filename -> {lineno: count}

        # Call graph data structures for navigation
        self.call_graph = collections.defaultdict(list)  # (caller_file, caller_line) -> [callees]
        self.callers_graph = collections.defaultdict(list)  # (callee_file, callee_line) -> [callers]
        self.function_definitions = {}  # (filename, funcname) -> lineno

        # Statistics and metadata
        self._total_samples = 0
        self._path_info = get_python_path_info()
        self.stats = {}

    def set_stats(self, sample_interval_usec, duration_sec, sample_rate, error_rate=None, **kwargs):
        """Set profiling statistics to include in heatmap output.

        Args:
            sample_interval_usec: Sampling interval in microseconds
            duration_sec: Total profiling duration in seconds
            sample_rate: Effective sampling rate
            error_rate: Optional error rate during profiling
            **kwargs: Additional statistics to include
        """
        self.stats = {
            "sample_interval_usec": sample_interval_usec,
            "duration_sec": duration_sec,
            "sample_rate": sample_rate,
            "error_rate": error_rate,
            "python_version": sys.version,
            "python_implementation": platform.python_implementation(),
            "platform": platform.platform(),
        }
        self.stats.update(kwargs)

    def process_frames(self, frames, thread_id):
        """Process stack frames and count samples per line.

        Args:
            frames: List of frame tuples (filename, lineno, funcname)
            thread_id: Thread ID for this stack trace
        """
        self._total_samples += 1

        # Count each line in the stack and build call graph
        for i, frame_info in enumerate(frames):
            filename, lineno, funcname = frame_info

            if not self._is_valid_frame(filename, lineno):
                continue

            self._record_line_sample(filename, lineno, funcname)

            # Build call graph for adjacent frames
            if i + 1 < len(frames):
                self._record_call_relationship(frames[i], frames[i + 1])

    def _is_valid_frame(self, filename, lineno):
        # Skip internal or invalid files
        if not filename or filename.startswith('<') or filename.startswith('['):
            return False

        # Skip invalid frames with corrupted filename data
        # These come from C code when frame info is unavailable
        if filename == "__init__" and lineno == 0:
            return False

        return True

    def _record_line_sample(self, filename, lineno, funcname):
        self.line_samples[(filename, lineno)] += 1
        self.file_samples[filename][lineno] += 1

        # Record function definition location
        if funcname and (filename, funcname) not in self.function_definitions:
            self.function_definitions[(filename, funcname)] = lineno

    def _record_call_relationship(self, callee_frame, caller_frame):
        callee_filename, callee_lineno, callee_funcname = callee_frame
        caller_filename, caller_lineno, caller_funcname = caller_frame

        # Skip internal files for call graph
        if callee_filename.startswith('<') or callee_filename.startswith('['):
            return

        # Get the callee's function definition line
        callee_def_line = self.function_definitions.get(
            (callee_filename, callee_funcname), callee_lineno
        )

        # Record caller -> callee relationship
        caller_key = (caller_filename, caller_lineno)
        callee_info = (callee_filename, callee_def_line, callee_funcname)
        if callee_info not in self.call_graph[caller_key]:
            self.call_graph[caller_key].append(callee_info)

        # Record callee <- caller relationship
        callee_key = (callee_filename, callee_def_line)
        caller_info = (caller_filename, caller_lineno, caller_funcname)
        if caller_info not in self.callers_graph[callee_key]:
            self.callers_graph[callee_key].append(caller_info)

    def export(self, output_path):
        """Export heatmap data as HTML files in a directory.

        Args:
            output_path: Path where to create the heatmap output directory
        """
        if not self.file_samples:
            print("Warning: No heatmap data to export")
            return

        try:
            output_dir = self._prepare_output_directory(output_path)
            file_stats = self._calculate_file_stats()
            self._create_file_index(file_stats)

            # Generate individual file reports
            self._generate_file_reports(output_dir, file_stats)

            # Generate index page
            self._generate_index_html(output_dir / 'index.html', file_stats)

            self._print_export_summary(output_dir, file_stats)

        except Exception as e:
            print(f"Error: Failed to export heatmap: {e}")
            raise

    def _prepare_output_directory(self, output_path):
        output_dir = Path(output_path)
        if output_dir.suffix == '.html':
            output_dir = output_dir.with_suffix('')

        output_dir.mkdir(exist_ok=True)
        return output_dir

    def _create_file_index(self, file_stats):
        self.file_index = {
            stat['filename']: f"file_{i:04d}.html"
            for i, stat in enumerate(file_stats)
        }

    def _generate_file_reports(self, output_dir, file_stats):
        for stat in file_stats:
            file_path = output_dir / self.file_index[stat['filename']]
            self._generate_file_html(
                file_path,
                stat['filename'],
                self.file_samples[stat['filename']],
                stat
            )

    def _print_export_summary(self, output_dir, file_stats):
        print(f"Heatmap output written to {output_dir}/")
        print(f"  - Index: {output_dir / 'index.html'}")
        print(f"  - {len(file_stats)} source file(s) analyzed")

    def _calculate_file_stats(self):
        """Calculate statistics for each file."""
        file_stats = []
        for filename, line_counts in self.file_samples.items():
            total_samples = sum(line_counts.values())
            num_lines = len(line_counts)
            max_samples = max(line_counts.values()) if line_counts else 0
            module_name, module_type = extract_module_name(filename, self._path_info)

            file_stats.append({
                'filename': filename,
                'module_name': module_name,
                'module_type': module_type,
                'total_samples': total_samples,
                'num_lines': num_lines,
                'max_samples': max_samples,
                'percentage': 0  # Will be calculated after sorting
            })

        # Sort by total samples and calculate percentages
        file_stats.sort(key=lambda x: x['total_samples'], reverse=True)
        if file_stats:
            max_total = file_stats[0]['total_samples']
            for stat in file_stats:
                stat['percentage'] = (stat['total_samples'] / max_total * 100) if max_total > 0 else 0

        return file_stats

    def _generate_index_html(self, index_path, file_stats):
        """Generate index.html with list of all profiled files."""
        import html
        import base64
        import importlib.resources

        # Load template and assets
        template_dir = importlib.resources.files(__package__)
        assets_dir = template_dir / "_assets"

        template_content = (template_dir / "heatmap_index_template.html").read_text(encoding="utf-8")
        css_content = (template_dir / "heatmap.css").read_text(encoding="utf-8")
        js_content = (template_dir / "heatmap_index.js").read_text(encoding="utf-8")

        # Load Python logo
        png_path = assets_dir / "python-logo-only.png"
        b64_logo = base64.b64encode(png_path.read_bytes()).decode("ascii")
        logo_html = f'<img src="data:image/png;base64,{b64_logo}" alt="Python logo" class="python-logo"/>'

        # Build table rows
        table_rows = self._build_table_rows(file_stats)

        # Populate template
        replacements = {
            "<!-- INLINE_CSS -->": f"<style>\n{css_content}\n</style>",
            "<!-- INLINE_JS -->": f"<script>\n{js_content}\n</script>",
            "<!-- PYTHON_LOGO -->": logo_html,
            "<!-- NUM_FILES -->": str(len(file_stats)),
            "<!-- TOTAL_SAMPLES -->": f"{self._total_samples:,}",
            "<!-- DURATION -->": f"{self.stats.get('duration_sec', 0):.1f}s",
            "<!-- SAMPLE_RATE -->": f"{self.stats.get('sample_rate', 0):.1f}",
            "<!-- TABLE_ROWS -->": ''.join(table_rows),
        }

        html_content = template_content
        for placeholder, value in replacements.items():
            html_content = html_content.replace(placeholder, value)

        index_path.write_text(html_content, encoding='utf-8')

    def _build_table_rows(self, file_stats):
        """Build HTML table rows for the file listing."""
        import html

        table_rows = []
        for stat in file_stats:
            full_path = html.escape(stat['filename'])
            module_name = html.escape(stat['module_name'])
            module_type = stat['module_type']
            badge_class = f"badge-{module_type}"

            # Calculate heatmap color for intensity visualization
            intensity = stat['percentage'] / 100.0
            r, g, b, alpha = self._calculate_intensity_color(intensity)
            bg_color = f"rgba({r}, {g}, {b}, {alpha})"
            bar_width = stat['percentage']  # Use actual percentage for bar width

            html_file = self.file_index[stat['filename']]
            row = f'''
                        <tr data-module-type="{module_type}">
                            <td><a href="{html_file}" class="file-link" title="{full_path}">{full_path}</a></td>
                            <td>{module_name}</td>
                            <td><span class="module-badge {badge_class}">{module_type}</span></td>
                            <td style="text-align: right;">{stat['total_samples']:,}</td>
                            <td style="text-align: right;">{stat['num_lines']}</td>
                            <td style="text-align: left;">
                                <div class="heatmap-bar" style="width: {bar_width}px; background-color: {bg_color}; height: 16px; border-radius: 2px;" title="{stat['percentage']:.2f}% relative to hottest file"></div>
                            </td>
                        </tr>'''
            table_rows.append(row)
        return table_rows

    def _calculate_intensity_color(self, intensity):
        """Calculate RGB color and alpha for given intensity (0-1 range).

        Returns (r, g, b, alpha) tuple representing the heatmap color gradient:
        blue -> green -> yellow -> orange -> red
        """
        # Color stops with (threshold, rgb_func, alpha_func)
        stops = [
            (0.2, lambda i: (0, int(150 * i * 5), 255), lambda i: 0.3),
            (0.4, lambda i: (0, 255, int(255 * (1 - (i - 0.2) * 5))), lambda i: 0.4),
            (0.6, lambda i: (int(255 * (i - 0.4) * 5), 255, 0), lambda i: 0.5),
            (0.8, lambda i: (255, int(200 - 100 * (i - 0.6) * 5), 0), lambda i: 0.6),
            (1.0, lambda i: (255, int(100 * (1 - (i - 0.8) * 5)), 0), lambda i: 0.7 + 0.15 * (i - 0.8) * 5),
        ]

        for threshold, rgb_func, alpha_func in stops:
            if intensity < threshold or threshold == 1.0:
                r, g, b = rgb_func(intensity)
                return (r, g, b, alpha_func(intensity))

        # Fallback (should not reach here)
        return (255, 0, 0, 0.75)

    def _deduplicate_by_function(self, items):
        """Remove duplicate entries based on (file, function) key."""
        seen = {}
        result = []
        for file, line, func in items:
            key = (file, func)
            if key not in seen:
                seen[key] = True
                result.append((file, line, func))
        return result

    def _create_navigation_button(self, items, btn_class, arrow):
        """Create HTML for a navigation button (caller/callee).

        Args:
            items: List of (file, line, func) tuples
            btn_class: CSS class ('caller' or 'callee')
            arrow: Arrow symbol ('↑' or '↓')

        Returns:
            HTML string for button, or empty string if no valid items
        """
        import html
        import json
        import os

        # Filter valid items (must be in index and have valid line number)
        valid_items = [(f, l, fn) for f, l, fn in items if f in self.file_index and l > 0]
        if not valid_items:
            return ""

        if len(valid_items) == 1:
            file, line, func = valid_items[0]
            target_html = self.file_index[file]
            nav_data = json.dumps({'link': f"{target_html}#line-{line}", 'func': func})
            title = f"Go to {btn_class}: {html.escape(func)}"
            return f'<button class="nav-btn {btn_class}" data-nav=\'{html.escape(nav_data)}\' title="{title}">{arrow}</button>'

        # Multiple items - create menu
        items_data = [
            {
                'file': os.path.basename(file),
                'func': func,
                'link': f"{self.file_index[file]}#line-{line}"
            }
            for file, line, func in valid_items
        ]
        items_json = html.escape(json.dumps(items_data))
        title = f"{len(items_data)} {btn_class}s"
        return f'<button class="nav-btn {btn_class}" data-nav-multi=\'{items_json}\' title="{title}">{arrow}</button>'

    def _generate_file_html(self, output_path, filename, line_counts, file_stat):
        """Generate HTML for a single source file with heatmap coloring."""
        import html
        import importlib.resources

        # Load template, CSS, and JS
        template_dir = importlib.resources.files(__package__)
        template_content = (template_dir / "heatmap_pyfile_template.html").read_text(encoding="utf-8")
        css_content = (template_dir / "heatmap.css").read_text(encoding="utf-8")
        js_content = (template_dir / "heatmap.js").read_text(encoding="utf-8")

        # Read source file
        try:
            source_lines = Path(filename).read_text(encoding='utf-8', errors='replace').splitlines()
        except (IOError, OSError):
            source_lines = []

        # Generate HTML for each line
        max_samples = max(line_counts.values()) if line_counts else 1
        code_lines_html = [
            self._build_line_html(line_num, line_content, line_counts, max_samples, filename)
            for line_num, line_content in enumerate(source_lines, start=1)
        ]

        # Populate template
        replacements = {
            "<!-- FILENAME -->": html.escape(filename),
            "<!-- TOTAL_SAMPLES -->": f"{file_stat['total_samples']:,}",
            "<!-- NUM_LINES -->": str(file_stat['num_lines']),
            "<!-- PERCENTAGE -->": f"{file_stat['percentage']:.2f}",
            "<!-- MAX_SAMPLES -->": str(file_stat['max_samples']),
            "<!-- CODE_LINES -->": ''.join(code_lines_html),
            "<!-- INLINE_CSS -->": f"<style>\n{css_content}\n</style>",
            "<!-- INLINE_JS -->": f"<script>\n{js_content}\n</script>",
        }

        html_content = template_content
        for placeholder, value in replacements.items():
            html_content = html_content.replace(placeholder, value)

        output_path.write_text(html_content, encoding='utf-8')

    def _build_line_html(self, line_num, line_content, line_counts, max_samples, filename):
        """Build HTML for a single line of source code."""
        import html

        samples = line_counts.get(line_num, 0)

        # Calculate color and sample display
        if samples > 0:
            intensity = samples / max_samples
            r, g, b, alpha = self._calculate_intensity_color(intensity)
            bg_color = f"rgba({r}, {g}, {b}, {alpha})"
            samples_display = f"{samples:,}"
            tooltip = f"{samples:,} samples"
        else:
            bg_color = "transparent"
            samples_display = ""
            tooltip = ""

        # Get navigation data
        line_key = (filename, line_num)
        callers = self._deduplicate_by_function(self.callers_graph.get(line_key, []))
        callees = self._deduplicate_by_function(self.call_graph.get(line_key, []))

        # Build navigation buttons
        caller_btn = self._create_navigation_button(callers, 'caller', '↑')
        callee_btn = self._create_navigation_button(callees, 'callee', '↓')
        nav_buttons_html = f'<div class="line-nav-buttons">{caller_btn}{callee_btn}</div>' if (caller_btn or callee_btn) else ''

        # Build line HTML
        line_html = html.escape(line_content.rstrip('\n'))
        title_attr = f' title="{html.escape(tooltip)}"' if tooltip else ""

        return (
            f'        <div class="code-line" data-bg-color="{bg_color}" id="line-{line_num}"{title_attr}>\n'
            f'            <div class="line-number">{line_num}</div>\n'
            f'            <div class="line-samples">{samples_display}</div>\n'
            f'            <div class="line-content">{line_html}</div>\n'
            f'            {nav_buttons_html}\n'
            f'        </div>\n'
        )

