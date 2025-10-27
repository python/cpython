import base64
import collections
import functools
import importlib.resources
import json
import linecache
import os
import site
import sys
import sysconfig

from .collector import Collector
from .string_table import StringTable


def get_python_path_info():
    """Get information about Python installation paths for module extraction."""
    info = {
        'stdlib': None,
        'site_packages': [],
        'sys_path': []
    }

    # Get standard library path from os module location (more reliable than sysconfig)
    if hasattr(os, '__file__') and os.__file__:
        stdlib_path = os.path.dirname(os.path.abspath(os.__file__))
        info['stdlib'] = os.path.normpath(stdlib_path)

    # Get site-packages directories using site.getsitepackages() and site.getusersitepackages()
    site_packages = []
    try:
        # Get system site-packages
        site_packages.extend([os.path.normpath(p) for p in site.getsitepackages()])
    except AttributeError:
        pass

    # Get user site-packages
    try:
        user_site = site.getusersitepackages()
        if user_site and os.path.exists(user_site):
            site_packages.append(os.path.normpath(user_site))
    except AttributeError:
        pass

    info['site_packages'] = site_packages

    # Include all sys.path entries (normalized)
    info['sys_path'] = [os.path.normpath(p) for p in sys.path if p]

    return info


def extract_module_name(filename, path_info):
    """Extract Python module name and type from file path using Python path information.

    Returns tuple of (module_name, module_type) where module_type is one of:
    'stdlib', 'site-packages', 'project', or 'other'
    """
    if not filename:
        return ('unknown', 'other')

    # Normalize the path
    filename = os.path.normpath(filename)

    # Try to match against sys.path entries to find the base
    best_match = None
    best_match_len = 0
    match_type = 'other'

    # First check if it's in stdlib
    if path_info['stdlib'] and filename.startswith(path_info['stdlib'] + os.sep):
        best_match = path_info['stdlib']
        best_match_len = len(best_match)
        match_type = 'stdlib'

    # Check site-packages
    if not best_match:
        for site_pkg in path_info['site_packages']:
            if filename.startswith(site_pkg + os.sep):
                if len(site_pkg) > best_match_len:
                    best_match = site_pkg
                    best_match_len = len(site_pkg)
                    match_type = 'site-packages'

    # Check other sys.path entries (actual filesystem paths)
    if not best_match:
        # Only mark as 'project' if it's an actual filesystem path
        if not filename.startswith('<') and not filename.startswith('['):
            for path_entry in path_info['sys_path']:
                if filename.startswith(path_entry + os.sep):
                    if len(path_entry) > best_match_len:
                        best_match = path_entry
                        best_match_len = len(path_entry)
                        match_type = 'project'

    if best_match:
        # Get relative path from the matched directory
        rel_path = os.path.relpath(filename, best_match)

        # Remove .py extension
        if rel_path.endswith('.py'):
            rel_path = rel_path[:-3]

        # Convert path separators to dots
        module_name = rel_path.replace(os.sep, '.')

        # Handle __init__ files - they represent the package itself
        if module_name.endswith('.__init__'):
            module_name = module_name[:-9]  # Remove '.__init__'

        return (module_name, match_type)

    # Fallback: just use the filename without extension
    base_name = os.path.basename(filename)
    if base_name.endswith('.py'):
        base_name = base_name[:-3]

    final_name = base_name if base_name != '__init__' else os.path.basename(os.path.dirname(filename))
    return (final_name, 'other')


class StackTraceCollector(Collector):
    def __init__(self, *, skip_idle=False):
        self.skip_idle = skip_idle

    def collect(self, stack_frames, skip_idle=False):
        for frames, thread_id in self._iter_all_frames(stack_frames, skip_idle=skip_idle):
            if not frames:
                continue
            self.process_frames(frames, thread_id)

    def process_frames(self, frames, thread_id):
        pass


class CollapsedStackCollector(StackTraceCollector):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.stack_counter = collections.Counter()

    def process_frames(self, frames, thread_id):
        call_tree = tuple(reversed(frames))
        self.stack_counter[(call_tree, thread_id)] += 1

    def export(self, filename):
        lines = []
        for (call_tree, thread_id), count in self.stack_counter.items():
            stack_str = ";".join(
                f"{os.path.basename(f[0])}:{f[2]}:{f[1]}" for f in call_tree
            )
            lines.append((f"tid:{thread_id};{stack_str}", count))

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
        self._func_intern = {}
        self._string_table = StringTable()
        self._all_threads = set()

    def set_stats(self, sample_interval_usec, duration_sec, sample_rate, error_rate=None):
        """Set profiling statistics to include in flamegraph data."""
        self.stats = {
            "sample_interval_usec": sample_interval_usec,
            "duration_sec": duration_sec,
            "sample_rate": sample_rate,
            "error_rate": error_rate
        }

    def export(self, filename):
        flamegraph_data = self._convert_to_flamegraph_format()

        # Debug output with string table statistics
        num_functions = len(flamegraph_data.get("children", []))
        total_time = flamegraph_data.get("value", 0)
        string_count = len(self._string_table)
        print(
            f"Flamegraph data: {num_functions} root functions, total samples: {total_time}, "
            f"{string_count} unique strings"
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

        if len(filename) > 50:
            parts = filename.split("/")
            if len(parts) > 2:
                filename = f".../{'/'.join(parts[-2:])}"

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

    def process_frames(self, frames, thread_id):
        # Reverse to root->leaf
        call_tree = reversed(frames)
        self._root["samples"] += 1
        self._total_samples += 1
        self._root["threads"].add(thread_id)
        self._all_threads.add(thread_id)

        current = self._root
        for func in call_tree:
            func = self._func_intern.setdefault(func, func)
            children = current["children"]
            node = children.get(func)
            if node is None:
                node = {"samples": 0, "children": {}, "threads": set()}
                children[func] = node
            node["samples"] += 1
            node["threads"].add(thread_id)
            current = node

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

        html_template = (template_dir / "flamegraph_template.html").read_text(encoding="utf-8")
        css_content = (template_dir / "flamegraph.css").read_text(encoding="utf-8")
        js_content = (template_dir / "flamegraph.js").read_text(encoding="utf-8")

        # Inline first-party CSS/JS
        html_template = html_template.replace(
            "<!-- INLINE_CSS -->", f"<style>\n{css_content}\n</style>"
        )
        html_template = html_template.replace(
            "<!-- INLINE_JS -->", f"<script>\n{js_content}\n</script>"
        )

        png_path = assets_dir / "python-logo-only.png"
        b64_logo = base64.b64encode(png_path.read_bytes()).decode("ascii")

        # Let CSS control size; keep markup simple
        logo_html = f'<img src="data:image/png;base64,{b64_logo}" alt="Python logo"/>'
        html_template = html_template.replace("<!-- INLINE_LOGO -->", logo_html)

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


class HeatmapCollector(StackTraceCollector):
    """Collector that generates coverage.py-style heatmap HTML output with line intensity."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        # Map from (filename, lineno) -> sample count
        self.line_samples = collections.Counter()
        # Map from filename -> {lineno: count}
        self.file_samples = collections.defaultdict(lambda: collections.Counter())
        # Map from (caller_file, caller_line) -> [(callee_file, callee_line, callee_func)]
        self.call_graph = collections.defaultdict(list)
        # Map from (callee_file, callee_line) -> [(caller_file, caller_line, caller_func)]
        self.callers_graph = collections.defaultdict(list)
        # Map from (filename, funcname) -> lineno (for reverse lookup)
        self.function_definitions = {}
        self._total_samples = 0
        self._path_info = get_python_path_info()
        self.stats = {}

    def set_stats(self, sample_interval_usec, duration_sec, sample_rate, error_rate=None, **kwargs):
        """Set profiling statistics to include in heatmap output."""
        import platform
        import sys

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
        """Process stack frames and count samples per line."""
        self._total_samples += 1

        # Count each line in the stack and build call graph
        for i, (filename, lineno, funcname) in enumerate(frames):
            # Skip internal or invalid files
            if not filename or filename.startswith('<') or filename.startswith('['):
                continue

            # Skip invalid frames with corrupted filename data (line 0 with bare __init__)
            # These come from C code when frame info is unavailable
            if filename == "__init__" and lineno == 0:
                continue

            self.line_samples[(filename, lineno)] += 1
            self.file_samples[filename][lineno] += 1

            # Record function definition
            if funcname and (filename, funcname) not in self.function_definitions:
                self.function_definitions[(filename, funcname)] = lineno

            # Build call graph: frame[i+1] called frame[i]
            # Frames are ordered from innermost (current, index 0) to outermost (root)
            # So frame[i+1] is the caller, and frame[i] is the callee
            # The line number in frame[i+1] is where the call was made
            if i + 1 < len(frames):
                callee_filename, callee_lineno, callee_funcname = frames[i]
                caller_filename, caller_lineno, caller_funcname = frames[i + 1]

                if not callee_filename.startswith('<') and not callee_filename.startswith('['):
                    # Get the callee's function definition line
                    callee_def_line = self.function_definitions.get((callee_filename, callee_funcname), callee_lineno)

                    # Record that caller line calls the callee function (at its definition)
                    caller_key = (caller_filename, caller_lineno)
                    callee_info = (callee_filename, callee_def_line, callee_funcname)
                    if callee_info not in self.call_graph[caller_key]:
                        self.call_graph[caller_key].append(callee_info)

                    # Also record the reverse: callee function (at definition) is called by caller line
                    callee_key = (callee_filename, callee_def_line)
                    caller_info = (caller_filename, caller_lineno, caller_funcname)
                    if caller_info not in self.callers_graph[callee_key]:
                        self.callers_graph[callee_key].append(caller_info)

    def export(self, output_path):
        """Export heatmap data as HTML files in a directory."""
        import os
        import html

        # If output_path is a directory, use it; otherwise create one based on the name
        if output_path.endswith('.html'):
            output_dir = output_path[:-5]  # Remove .html extension
        else:
            output_dir = output_path

        os.makedirs(output_dir, exist_ok=True)

        # Calculate statistics for each file
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

        # Sort by total samples descending
        file_stats.sort(key=lambda x: x['total_samples'], reverse=True)

        # Calculate percentages relative to the max (top file = 100%)
        if file_stats:
            max_total = file_stats[0]['total_samples']
            for stat in file_stats:
                stat['percentage'] = (stat['total_samples'] / max_total * 100) if max_total > 0 else 0

        # Build file index mapping filename -> html filename
        self.file_index = {}
        for i, stat in enumerate(file_stats):
            safe_name = f"file_{i:04d}.html"
            self.file_index[stat['filename']] = safe_name

        # Generate individual file HTML reports
        file_links = []
        for i, stat in enumerate(file_stats):
            safe_name = self.file_index[stat['filename']]
            file_path = os.path.join(output_dir, safe_name)

            self._generate_file_html(
                file_path,
                stat['filename'],
                self.file_samples[stat['filename']],
                stat
            )

            file_links.append({
                'html_file': safe_name,
                'stat': stat
            })

        # Generate index.html
        index_path = os.path.join(output_dir, 'index.html')
        self._generate_index_html(index_path, file_links)

        print(f"Heatmap output written to {output_dir}/")
        print(f"  - Index: {index_path}")
        print(f"  - {len(file_links)} source file(s) analyzed")

    def _generate_index_html(self, index_path, file_links):
        """Generate index.html with list of all profiled files."""
        import html
        import base64
        import importlib.resources

        # Load template, CSS, JS and assets
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
        table_rows = []
        for link in file_links:
            stat = link['stat']
            full_path = html.escape(stat['filename'])
            module_name = html.escape(stat['module_name'])
            module_type = stat['module_type']
            badge_class = f"badge-{module_type}"
            bar_width = min(100, stat['percentage'] * 2)  # Scale for visibility

            row = f'''
                        <tr data-module-type="{module_type}">
                            <td><a href="{link['html_file']}" class="file-link" title="{full_path}">{full_path}</a></td>
                            <td>{module_name}</td>
                            <td><span class="module-badge {badge_class}">{module_type}</span></td>
                            <td style="text-align: right;">{stat['total_samples']:,}</td>
                            <td style="text-align: right;">{stat['num_lines']}</td>
                            <td style="text-align: right;">{stat['percentage']:.2f}%</td>
                            <td style="text-align: left;">
                                <div class="heatmap-bar" style="width: {bar_width}px;" title="{stat['percentage']:.2f}%"></div>
                            </td>
                        </tr>'''
            table_rows.append(row)

        # Populate template
        replacements = {
            "<!-- INLINE_CSS -->": f"<style>\n{css_content}\n</style>",
            "<!-- INLINE_JS -->": f"<script>\n{js_content}\n</script>",
            "<!-- PYTHON_LOGO -->": logo_html,
            "<!-- NUM_FILES -->": str(len(file_links)),
            "<!-- TOTAL_SAMPLES -->": f"{self._total_samples:,}",
            "<!-- DURATION -->": f"{self.stats.get('duration_sec', 0):.1f}s",
            "<!-- SAMPLE_RATE -->": f"{self.stats.get('sample_rate', 0):.1f}",
            "<!-- TABLE_ROWS -->": ''.join(table_rows),
        }

        html_content = template_content
        for placeholder, value in replacements.items():
            html_content = html_content.replace(placeholder, value)

        with open(index_path, 'w', encoding='utf-8') as f:
            f.write(html_content)

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
            with open(filename, 'r', encoding='utf-8', errors='replace') as f:
                source_lines = f.readlines()
        except (IOError, OSError):
            source_lines = []

        # Find max sample count for color scaling
        max_samples = max(line_counts.values()) if line_counts else 1

        # Generate HTML for each line
        code_lines_html = []
        for line_num, line_content in enumerate(source_lines, start=1):
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

            # Get and deduplicate navigation data
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
            # Use data attribute for background color instead of inline style
            code_lines_html.append(
                f'        <div class="code-line" data-bg-color="{bg_color}" id="line-{line_num}"{title_attr}>\n'
                f'            <div class="line-number">{line_num}</div>\n'
                f'            <div class="line-samples">{samples_display}</div>\n'
                f'            <div class="line-content">{line_html}</div>\n'
                f'            {nav_buttons_html}\n'
                f'        </div>\n'
            )

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

        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(html_content)

