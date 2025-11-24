"""Heatmap collector for Python profiling with line-level execution heat visualization."""

import base64
import collections
import importlib.resources
import json
import platform
import site
import sys
from pathlib import Path

from .stack_collector import StackTraceCollector


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
        import os
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


class HeatmapCollector(StackTraceCollector):
    """Collector that generates coverage.py-style heatmap HTML output with line intensity.

    This collector creates detailed HTML reports showing which lines of code
    were executed most frequently during profiling, similar to coverage.py
    but showing execution "heat" rather than just coverage.
    """

    # Color gradient configuration
    # Color stops: blue -> green -> yellow -> orange -> red
    COLOR_STOP_1_THRESHOLD = 0.2  # Blue to cyan transition
    COLOR_STOP_2_THRESHOLD = 0.4  # Cyan to green transition
    COLOR_STOP_3_THRESHOLD = 0.6  # Green to yellow transition
    COLOR_STOP_4_THRESHOLD = 0.8  # Yellow to orange transition
    COLOR_STOP_5_THRESHOLD = 1.0  # Orange to red transition

    # Alpha (opacity) values for color gradient
    ALPHA_VERY_COLD = 0.3    # Lowest intensity
    ALPHA_COLD = 0.4
    ALPHA_MEDIUM = 0.5
    ALPHA_WARM = 0.6
    ALPHA_HOT_BASE = 0.7
    ALPHA_HOT_RANGE = 0.15   # Additional alpha for hottest colors

    # Multiplier for gradient calculations
    GRADIENT_MULTIPLIER = 5  # Used for scaling gradients within each color stop

    # File naming and formatting
    FILE_INDEX_FORMAT = "file_{:04d}.html"  # Format for generated HTML filenames
    HEATMAP_BAR_HEIGHT = 16  # Height in pixels for index page heatmap bars

    def __init__(self, *args, **kwargs):
        """Initialize the heatmap collector with data structures for analysis."""
        super().__init__(*args, **kwargs)

        # Sample counting data structures
        self.line_samples = collections.Counter()  # (filename, lineno) -> cumulative count
        self.file_samples = collections.defaultdict(collections.Counter)  # filename -> {lineno: cumulative count}
        self.line_self_samples = collections.Counter()  # (filename, lineno) -> self/leaf count
        self.file_self_samples = collections.defaultdict(collections.Counter)  # filename -> {lineno: self count}

        # Call graph data structures for navigation
        self.call_graph = collections.defaultdict(list)  # (caller_file, caller_line) -> [callees]
        self.callers_graph = collections.defaultdict(list)  # (callee_file, callee_line) -> [callers]
        self.function_definitions = {}  # (filename, funcname) -> lineno

        # Edge counting for call path analysis
        # Tracks how many samples each call edge contributes
        self.edge_samples = collections.Counter()  # ((caller_file, caller_line), (callee_file, callee_line)) -> count

        # Statistics and metadata
        self._total_samples = 0
        self._path_info = get_python_path_info()
        self.stats = {}

        # Cache for color calculations to improve performance
        self._color_cache = {}

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
                    frames[0] is the leaf (top of stack, where execution is)
            thread_id: Thread ID for this stack trace
        """
        self._total_samples += 1

        # Count each line in the stack and build call graph
        for i, frame_info in enumerate(frames):
            filename, lineno, funcname = frame_info

            if not self._is_valid_frame(filename, lineno):
                continue

            # frames[0] is the leaf - where execution is actually happening
            is_leaf = (i == 0)
            self._record_line_sample(filename, lineno, funcname, is_leaf=is_leaf)

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

    def _record_line_sample(self, filename, lineno, funcname, is_leaf=False):
        # Track cumulative samples (all occurrences in stack)
        self.line_samples[(filename, lineno)] += 1
        self.file_samples[filename][lineno] += 1

        # Track self/leaf samples (only when at top of stack)
        if is_leaf:
            self.line_self_samples[(filename, lineno)] += 1
            self.file_self_samples[filename][lineno] += 1

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

        # Count this call edge for path analysis
        edge_key = (caller_key, callee_key)
        self.edge_samples[edge_key] += 1

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

        try:
            output_dir.mkdir(exist_ok=True, parents=True)
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to create output directory {output_dir}: {e}") from e

        return output_dir

    def _create_file_index(self, file_stats):
        self.file_index = {
            stat['filename']: self.FILE_INDEX_FORMAT.format(i)
            for i, stat in enumerate(file_stats)
        }

    def _generate_file_reports(self, output_dir, file_stats):
        for stat in file_stats:
            file_path = output_dir / self.file_index[stat['filename']]
            # Filter out lines with -1 (special frames)
            line_counts = self.file_samples[stat['filename']]
            valid_line_counts = {line: count for line, count in line_counts.items() if line >= 0}

            # Get self samples for this file
            self_counts = self.file_self_samples.get(stat['filename'], {})
            valid_self_counts = {line: count for line, count in self_counts.items() if line >= 0}

            self._generate_file_html(
                file_path,
                stat['filename'],
                valid_line_counts,
                valid_self_counts,
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
            # Skip special frames (files named '~' or with invalid line numbers)
            if filename in ('~', '...', '.') or filename.startswith('<') or filename.startswith('['):
                continue

            # Filter out lines with -1 (special frames)
            valid_line_counts = {line: count for line, count in line_counts.items() if line >= 0}
            if not valid_line_counts:
                continue

            # Get self samples for this file
            self_line_counts = self.file_self_samples.get(filename, {})
            valid_self_counts = {line: count for line, count in self_line_counts.items() if line >= 0}

            total_samples = sum(valid_line_counts.values())
            total_self_samples = sum(valid_self_counts.values())
            num_lines = len(valid_line_counts)
            max_samples = max(valid_line_counts.values())
            max_self_samples = max(valid_self_counts.values()) if valid_self_counts else 0
            module_name, module_type = extract_module_name(filename, self._path_info)

            file_stats.append({
                'filename': filename,
                'module_name': module_name,
                'module_type': module_type,
                'total_samples': total_samples,
                'total_self_samples': total_self_samples,
                'num_lines': num_lines,
                'max_samples': max_samples,
                'max_self_samples': max_self_samples,
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
        try:
            template_dir = importlib.resources.files(__package__)
            assets_dir = template_dir / "_assets"

            template_content = (template_dir / "heatmap_index_template.html").read_text(encoding="utf-8")
            css_content = (template_dir / "heatmap.css").read_text(encoding="utf-8")
            js_content = (template_dir / "heatmap_index.js").read_text(encoding="utf-8")
        except (FileNotFoundError, IOError) as e:
            raise RuntimeError(f"Failed to load heatmap template files: {e}") from e

        # Load Python logo
        try:
            png_path = assets_dir / "python-logo-only.png"
            b64_logo = base64.b64encode(png_path.read_bytes()).decode("ascii")
            logo_html = f'<img src="data:image/png;base64,{b64_logo}" alt="Python logo" class="python-logo"/>'
        except (FileNotFoundError, IOError) as e:
            # Fall back to no logo if image file is missing
            logo_html = '<div class="python-logo-placeholder"></div>'
            print(f"Warning: Could not load Python logo: {e}")

        # Build hierarchical sections
        tree = self._build_file_tree(file_stats)
        sections_html = self._build_hierarchical_html(tree, file_stats)

        # Populate template
        replacements = {
            "<!-- INLINE_CSS -->": f"<style>\n{css_content}\n</style>",
            "<!-- INLINE_JS -->": f"<script>\n{js_content}\n</script>",
            "<!-- PYTHON_LOGO -->": logo_html,
            "<!-- NUM_FILES -->": str(len(file_stats)),
            "<!-- TOTAL_SAMPLES -->": f"{self._total_samples:,}",
            "<!-- DURATION -->": f"{self.stats.get('duration_sec', 0):.1f}s",
            "<!-- SAMPLE_RATE -->": f"{self.stats.get('sample_rate', 0):.1f}",
            "<!-- SECTIONS_HTML -->": sections_html,
        }

        html_content = template_content
        for placeholder, value in replacements.items():
            html_content = html_content.replace(placeholder, value)

        try:
            index_path.write_text(html_content, encoding='utf-8')
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to write index file {index_path}: {e}") from e

    def _build_file_tree(self, file_stats):
        """Build hierarchical tree grouped by module type, then by module structure."""
        # Group by module type first
        type_groups = {'stdlib': [], 'site-packages': [], 'project': [], 'other': []}
        for stat in file_stats:
            type_groups[stat['module_type']].append(stat)

        # Build tree for each type
        trees = {}
        for module_type, stats in type_groups.items():
            if not stats:
                continue

            type_tree = {}
            for stat in stats:
                module_name = stat['module_name']
                parts = module_name.split('.')

                # Navigate/create tree structure
                current = type_tree
                for i, part in enumerate(parts):
                    if i == len(parts) - 1:
                        # Last part - store the file
                        if '__files__' not in current:
                            current['__files__'] = []
                        current['__files__'].append(stat)
                    else:
                        # Intermediate part - create or navigate
                        if part not in current:
                            current[part] = {'__files__': [], '__samples__': 0, '__count__': 0}
                        current = current[part]

            # Calculate aggregate stats for this type's tree
            def calculate_stats(node):
                total_samples = 0
                file_count = 0

                if '__files__' in node:
                    for file_stat in node['__files__']:
                        total_samples += file_stat['total_samples']
                        file_count += 1

                for key, child in node.items():
                    if key not in ('__files__', '__samples__', '__count__'):
                        child_samples, child_count = calculate_stats(child)
                        total_samples += child_samples
                        file_count += child_count

                node['__samples__'] = total_samples
                node['__count__'] = file_count
                return total_samples, file_count

            calculate_stats(type_tree)
            trees[module_type] = type_tree

        return trees

    def _build_hierarchical_html(self, trees, file_stats):
        """Build hierarchical HTML with type sections and collapsible module folders."""
        import html as html_module

        def render_folder(node, name, level=1):
            indent = '  ' * level
            parts = []

            # Get stats for this folder
            samples = node.get('__samples__', 0)
            count = node.get('__count__', 0)

            # Render folder (collapsed by default)
            parts.append(f'{indent}<div class="folder-node collapsed" data-level="{level}">')
            parts.append(f'{indent}  <div class="folder-header" onclick="toggleFolder(this)">')
            parts.append(f'{indent}    <span class="folder-icon">▶</span>')
            parts.append(f'{indent}    <span class="folder-name">📁 {html_module.escape(name)}</span>')
            parts.append(f'{indent}    <span class="folder-stats">({count} files, {samples:,} samples)</span>')
            parts.append(f'{indent}  </div>')
            parts.append(f'{indent}  <div class="folder-content" style="display: none;">')

            # Render sub-folders
            subfolders = [(k, v) for k, v in node.items() if k not in ('__files__', '__samples__', '__count__')]
            subfolders.sort(key=lambda x: x[1].get('__samples__', 0), reverse=True)

            for subfolder_name, subfolder_node in subfolders:
                parts.append(render_folder(subfolder_node, subfolder_name, level + 1))

            # Render files in this folder
            files = node.get('__files__', [])
            if files:
                files.sort(key=lambda x: x['total_samples'], reverse=True)

                parts.append(f'{indent}    <div class="files-list">')
                for stat in files:
                    full_path = html_module.escape(stat['filename'])
                    module_name = html_module.escape(stat['module_name'])

                    intensity = stat['percentage'] / 100.0
                    r, g, b, alpha = self._calculate_intensity_color(intensity)
                    bg_color = f"rgba({r}, {g}, {b}, {alpha})"
                    bar_width = min(stat['percentage'], 100)

                    html_file = self.file_index[stat['filename']]

                    parts.append(f'{indent}      <div class="file-item">')
                    parts.append(f'{indent}        <a href="{html_file}" class="file-link" title="{full_path}">📄 {module_name}</a>')
                    parts.append(f'{indent}        <span class="file-samples">{stat["total_samples"]:,} samples</span>')
                    parts.append(f'{indent}        <div class="heatmap-bar" style="width: {bar_width}px; background-color: {bg_color}; height: {self.HEATMAP_BAR_HEIGHT}px;"></div>')
                    parts.append(f'{indent}      </div>')

                parts.append(f'{indent}    </div>')

            parts.append(f'{indent}  </div>')
            parts.append(f'{indent}</div>')

            return '\n'.join(parts)

        # Build sections for each module type
        type_names = {
            'stdlib': '📚 Standard Library',
            'site-packages': '📦 Site Packages',
            'project': '🏗️ Project Files',
            'other': '📄 Other Files'
        }

        sections = []
        for module_type in ['project', 'stdlib', 'site-packages', 'other']:
            if module_type not in trees:
                continue

            tree = trees[module_type]
            total_samples = tree.get('__samples__', 0) if isinstance(tree, dict) else 0
            file_count = tree.get('__count__', 0) if isinstance(tree, dict) else 0

            # Calculate totals from tree root
            for key, node in tree.items():
                if key not in ('__files__', '__samples__', '__count__'):
                    total_samples += node.get('__samples__', 0)
                    file_count += node.get('__count__', 0)

            # Project starts expanded, stdlib/site-packages start collapsed
            is_collapsed = module_type in ('stdlib', 'site-packages')
            icon = '▶' if is_collapsed else '▼'
            content_style = ' style="display: none;"' if is_collapsed else ''

            section_html = f'''
<div class="type-section">
  <div class="type-header" onclick="toggleTypeSection(this)">
    <span class="type-icon">{icon}</span>
    <span class="type-title">{type_names[module_type]}</span>
    <span class="type-stats">({file_count} files, {total_samples:,} samples)</span>
  </div>
  <div class="type-content"{content_style}>
'''

            # Render root folders
            root_folders = [(k, v) for k, v in tree.items() if k not in ('__files__', '__samples__', '__count__')]
            root_folders.sort(key=lambda x: x[1].get('__samples__', 0), reverse=True)

            for folder_name, folder_node in root_folders:
                section_html += render_folder(folder_node, folder_name, 1)

            # Render root files (files not in any module)
            root_files = tree.get('__files__', [])
            if root_files:
                root_files.sort(key=lambda x: x['total_samples'], reverse=True)
                section_html += '    <div class="files-list">\n'
                for stat in root_files:
                    full_path = html_module.escape(stat['filename'])
                    module_name = html_module.escape(stat['module_name'])
                    intensity = stat['percentage'] / 100.0
                    r, g, b, alpha = self._calculate_intensity_color(intensity)
                    bg_color = f"rgba({r}, {g}, {b}, {alpha})"
                    bar_width = min(stat['percentage'], 100)
                    html_file = self.file_index[stat['filename']]

                    section_html += f'''      <div class="file-item">
        <a href="{html_file}" class="file-link" title="{full_path}">📄 {module_name}</a>
        <span class="file-samples">{stat["total_samples"]:,} samples</span>
        <div class="heatmap-bar" style="width: {bar_width}px; background-color: {bg_color}; height: {self.HEATMAP_BAR_HEIGHT}px;"></div>
      </div>\n'''

                section_html += '    </div>\n'

            section_html += '  </div>\n</div>\n'
            sections.append(section_html)

        return '\n'.join(sections)

    def _calculate_intensity_color(self, intensity):
        """Calculate RGB color and alpha for given intensity (0-1 range).

        Returns (r, g, b, alpha) tuple representing the heatmap color gradient:
        blue -> green -> yellow -> orange -> red

        Results are cached to improve performance when the same intensity
        values are calculated multiple times.
        """
        # Round to 3 decimal places for cache key to balance hits vs memory
        cache_key = round(intensity, 3)
        if cache_key in self._color_cache:
            return self._color_cache[cache_key]

        m = self.GRADIENT_MULTIPLIER

        # Color stops with (threshold, rgb_func, alpha_func)
        stops = [
            (self.COLOR_STOP_1_THRESHOLD,
             lambda i: (0, int(150 * i * m), 255),
             lambda i: self.ALPHA_VERY_COLD),
            (self.COLOR_STOP_2_THRESHOLD,
             lambda i: (0, 255, int(255 * (1 - (i - self.COLOR_STOP_1_THRESHOLD) * m))),
             lambda i: self.ALPHA_COLD),
            (self.COLOR_STOP_3_THRESHOLD,
             lambda i: (int(255 * (i - self.COLOR_STOP_2_THRESHOLD) * m), 255, 0),
             lambda i: self.ALPHA_MEDIUM),
            (self.COLOR_STOP_4_THRESHOLD,
             lambda i: (255, int(200 - 100 * (i - self.COLOR_STOP_3_THRESHOLD) * m), 0),
             lambda i: self.ALPHA_WARM),
            (self.COLOR_STOP_5_THRESHOLD,
             lambda i: (255, int(100 * (1 - (i - self.COLOR_STOP_4_THRESHOLD) * m)), 0),
             lambda i: self.ALPHA_HOT_BASE + self.ALPHA_HOT_RANGE * (i - self.COLOR_STOP_4_THRESHOLD) * m),
        ]

        result = None
        for threshold, rgb_func, alpha_func in stops:
            if intensity < threshold or threshold == self.COLOR_STOP_5_THRESHOLD:
                r, g, b = rgb_func(intensity)
                result = (r, g, b, alpha_func(intensity))
                break

        # Fallback (should not reach here)
        if result is None:
            result = (255, 0, 0, 0.75)

        # Cache the result
        self._color_cache[cache_key] = result
        return result

    def _get_edge_counts(self, line_key, items, is_caller):
        """Get sample counts for each caller/callee edge.

        Args:
            line_key: (filename, lineno) tuple for the current line
            items: List of (file, line, func) tuples for callers or callees
            is_caller: True if items are callers, False if callees

        Returns:
            List of (file, line, func, count) tuples sorted by count descending
        """
        result = []
        for file, line, func in items:
            edge_line_key = (file, line)
            if is_caller:
                # For callers: edge is from (file, line) to line_key
                edge_key = (edge_line_key, line_key)
            else:
                # For callees: edge is from line_key to (file, line)
                edge_key = (line_key, edge_line_key)

            count = self.edge_samples.get(edge_key, 0)
            result.append((file, line, func, count))

        # Sort by count descending
        result.sort(key=lambda x: x[3], reverse=True)
        return result

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

    def _create_navigation_button(self, items_with_counts, btn_class, arrow):
        """Create HTML for a navigation button (caller/callee) with sample counts.

        Args:
            items_with_counts: List of (file, line, func, count) tuples
            btn_class: CSS class ('caller' or 'callee')
            arrow: Arrow symbol ('↑' or '↓')

        Returns:
            HTML string for button, or empty string if no valid items
        """
        import html
        import json
        import os

        # Filter valid items (must be in index and have valid line number)
        valid_items = [(f, l, fn, cnt) for f, l, fn, cnt in items_with_counts if f in self.file_index and l > 0]
        if not valid_items:
            return ""

        if len(valid_items) == 1:
            file, line, func, count = valid_items[0]
            target_html = self.file_index[file]
            nav_data = json.dumps({'link': f"{target_html}#line-{line}", 'func': func})
            title = f"Go to {btn_class}: {html.escape(func)} ({count:,} samples)"
            return f'<button class="nav-btn {btn_class}" data-nav=\'{html.escape(nav_data)}\' title="{title}">{arrow}</button>'

        # Multiple items - create menu with counts
        total_samples = sum(cnt for _, _, _, cnt in valid_items)
        items_data = [
            {
                'file': os.path.basename(file),
                'func': func,
                'count': count,
                'link': f"{self.file_index[file]}#line-{line}"
            }
            for file, line, func, count in valid_items
        ]
        items_json = html.escape(json.dumps(items_data))
        title = f"{len(items_data)} {btn_class}s ({total_samples:,} samples)"
        return f'<button class="nav-btn {btn_class}" data-nav-multi=\'{items_json}\' title="{title}">{arrow}</button>'

    def _generate_file_html(self, output_path, filename, line_counts, self_counts, file_stat):
        """Generate HTML for a single source file with heatmap coloring."""
        import html
        import importlib.resources

        # Load template, CSS, and JS
        try:
            template_dir = importlib.resources.files(__package__)
            template_content = (template_dir / "heatmap_pyfile_template.html").read_text(encoding="utf-8")
            css_content = (template_dir / "heatmap.css").read_text(encoding="utf-8")
            js_content = (template_dir / "heatmap.js").read_text(encoding="utf-8")
        except (FileNotFoundError, IOError) as e:
            raise RuntimeError(f"Failed to load heatmap template files: {e}") from e

        # Read source file
        try:
            source_lines = Path(filename).read_text(encoding='utf-8', errors='replace').splitlines()
        except (IOError, OSError) as e:
            # Skip warning for special/invalid paths
            if not (filename.startswith('<') or filename.startswith('[') or
                    filename in ('~', '...', '.') or len(filename) < 2):
                print(f"Warning: Could not read source file {filename}: {e}")
            source_lines = [f"# Source file not available: {filename}"]

        # Generate HTML for each line
        max_samples = max(line_counts.values()) if line_counts else 1
        max_self_samples = max(self_counts.values()) if self_counts else 1
        code_lines_html = [
            self._build_line_html(line_num, line_content, line_counts, self_counts,
                                 max_samples, max_self_samples, filename)
            for line_num, line_content in enumerate(source_lines, start=1)
        ]

        # Populate template
        replacements = {
            "<!-- FILENAME -->": html.escape(filename),
            "<!-- TOTAL_SAMPLES -->": f"{file_stat['total_samples']:,}",
            "<!-- TOTAL_SELF_SAMPLES -->": f"{file_stat['total_self_samples']:,}",
            "<!-- NUM_LINES -->": str(file_stat['num_lines']),
            "<!-- PERCENTAGE -->": f"{file_stat['percentage']:.2f}",
            "<!-- MAX_SAMPLES -->": str(file_stat['max_samples']),
            "<!-- MAX_SELF_SAMPLES -->": str(file_stat['max_self_samples']),
            "<!-- CODE_LINES -->": ''.join(code_lines_html),
            "<!-- INLINE_CSS -->": f"<style>\n{css_content}\n</style>",
            "<!-- INLINE_JS -->": f"<script>\n{js_content}\n</script>",
        }

        html_content = template_content
        for placeholder, value in replacements.items():
            html_content = html_content.replace(placeholder, value)

        try:
            output_path.write_text(html_content, encoding='utf-8')
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to write file {output_path}: {e}") from e

    def _build_line_html(self, line_num, line_content, line_counts, self_counts,
                         max_samples, max_self_samples, filename):
        """Build HTML for a single line of source code with both self and cumulative counts."""
        import html

        cumulative_samples = line_counts.get(line_num, 0)
        self_samples = self_counts.get(line_num, 0)

        # Calculate color based on self samples (default, will be toggleable via JS)
        # Store both cumulative and self intensities for JS to toggle
        if cumulative_samples > 0:
            cumulative_intensity = cumulative_samples / max_samples if max_samples > 0 else 0
            self_intensity = self_samples / max_self_samples if max_self_samples > 0 and self_samples > 0 else 0

            # Default to self-based coloring
            intensity = self_intensity if self_samples > 0 else cumulative_intensity
            r, g, b, alpha = self._calculate_intensity_color(intensity)
            bg_color = f"rgba({r}, {g}, {b}, {alpha})"

            # Calculate colors for both modes (for JS toggle)
            if self_samples > 0:
                self_r, self_g, self_b, self_alpha = self._calculate_intensity_color(self_intensity)
                self_bg_color = f"rgba({self_r}, {self_g}, {self_b}, {self_alpha})"
            else:
                self_bg_color = "transparent"

            cumulative_r, cumulative_g, cumulative_b, cumulative_alpha = self._calculate_intensity_color(cumulative_intensity)
            cumulative_bg_color = f"rgba({cumulative_r}, {cumulative_g}, {cumulative_b}, {cumulative_alpha})"

            self_display = f"{self_samples:,}" if self_samples > 0 else ""
            cumulative_display = f"{cumulative_samples:,}"
            tooltip = f"Self: {self_samples:,}, Cumulative: {cumulative_samples:,}"
        else:
            bg_color = "transparent"
            self_bg_color = "transparent"
            cumulative_bg_color = "transparent"
            self_display = ""
            cumulative_display = ""
            tooltip = ""

        # Get navigation data with edge counts
        line_key = (filename, line_num)
        caller_list = self._deduplicate_by_function(self.callers_graph.get(line_key, []))
        callee_list = self._deduplicate_by_function(self.call_graph.get(line_key, []))

        # Get edge counts for each caller/callee
        callers_with_counts = self._get_edge_counts(line_key, caller_list, is_caller=True)
        callees_with_counts = self._get_edge_counts(line_key, callee_list, is_caller=False)

        # Build navigation buttons with counts
        caller_btn = self._create_navigation_button(callers_with_counts, 'caller', '▲')
        callee_btn = self._create_navigation_button(callees_with_counts, 'callee', '▼')
        nav_buttons_html = f'<div class="line-nav-buttons">{caller_btn}{callee_btn}</div>' if (caller_btn or callee_btn) else ''

        # Build line HTML with two sample columns
        line_html = html.escape(line_content.rstrip('\n'))
        title_attr = f' title="{html.escape(tooltip)}"' if tooltip else ""

        return (
            f'        <div class="code-line" data-bg-color="{bg_color}" '
            f'data-self-color="{self_bg_color}" data-cumulative-color="{cumulative_bg_color}" '
            f'id="line-{line_num}"{title_attr}>\n'
            f'            <div class="line-number">{line_num}</div>\n'
            f'            <div class="line-samples-self">{self_display}</div>\n'
            f'            <div class="line-samples-cumulative">{cumulative_display}</div>\n'
            f'            <div class="line-content">{line_html}</div>\n'
            f'            {nav_buttons_html}\n'
            f'        </div>\n'
        )
