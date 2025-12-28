"""Heatmap collector for Python profiling with line-level execution heat visualization."""

import base64
import collections
import html
import importlib.resources
import json
import locale
import math
import os
import platform
import site
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Tuple

from ._css_utils import get_combined_css
from ._format_utils import fmt
from .collector import normalize_location, extract_lineno
from .opcode_utils import get_opcode_info, format_opcode
from .stack_collector import StackTraceCollector


# ============================================================================
# Data Classes
# ============================================================================

@dataclass
class FileStats:
    """Statistics for a single profiled file."""
    filename: str
    module_name: str
    module_type: str
    total_samples: int
    total_self_samples: int
    num_lines: int
    max_samples: int
    max_self_samples: int
    percentage: float = 0.0


@dataclass
class TreeNode:
    """Node in the hierarchical file tree structure."""
    files: List[FileStats] = field(default_factory=list)
    samples: int = 0
    count: int = 0
    children: Dict[str, 'TreeNode'] = field(default_factory=dict)


# ============================================================================
# Module Path Analysis
# ============================================================================

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


# ============================================================================
# Helper Classes
# ============================================================================

class _TemplateLoader:
    """Loads and caches HTML/CSS/JS templates for heatmap generation."""

    def __init__(self):
        """Load all templates and assets once."""
        self.index_template = None
        self.file_template = None
        self.index_css = None
        self.index_js = None
        self.file_css = None
        self.file_js = None
        self.logo_html = None

        self._load_templates()

    def _load_templates(self):
        """Load all template files from _heatmap_assets."""
        try:
            template_dir = importlib.resources.files(__package__)
            assets_dir = template_dir / "_heatmap_assets"

            # Load HTML templates
            self.index_template = (assets_dir / "heatmap_index_template.html").read_text(encoding="utf-8")
            self.file_template = (assets_dir / "heatmap_pyfile_template.html").read_text(encoding="utf-8")

            # Load CSS (same file used for both index and file pages)
            css_content = get_combined_css("heatmap")
            self.index_css = css_content
            self.file_css = css_content

            # Load JS
            shared_js = (assets_dir / "heatmap_shared.js").read_text(encoding="utf-8")
            self.index_js = f"{shared_js}\n{(assets_dir / 'heatmap_index.js').read_text(encoding='utf-8')}"
            self.file_js = f"{shared_js}\n{(assets_dir / 'heatmap.js').read_text(encoding='utf-8')}"

            # Load Tachyon logo
            logo_dir = template_dir / "_assets"
            try:
                png_path = logo_dir / "tachyon-logo.png"
                b64_logo = base64.b64encode(png_path.read_bytes()).decode("ascii")
                self.logo_html = f'<img src="data:image/png;base64,{b64_logo}" alt="Tachyon logo" class="python-logo"/>'
            except (FileNotFoundError, IOError) as e:
                self.logo_html = '<div class="python-logo-placeholder"></div>'
                print(f"Warning: Could not load Tachyon logo: {e}")

        except (FileNotFoundError, IOError) as e:
            raise RuntimeError(f"Failed to load heatmap template files: {e}") from e


class _TreeBuilder:
    """Builds hierarchical tree structure from file statistics."""

    @staticmethod
    def build_file_tree(file_stats: List[FileStats]) -> Dict[str, TreeNode]:
        """Build hierarchical tree grouped by module type, then by module structure.

        Args:
            file_stats: List of FileStats objects

        Returns:
            Dictionary mapping module types to their tree roots
        """
        # Group by module type first
        type_groups = {'stdlib': [], 'site-packages': [], 'project': [], 'other': []}
        for stat in file_stats:
            type_groups[stat.module_type].append(stat)

        # Build tree for each type
        trees = {}
        for module_type, stats in type_groups.items():
            if not stats:
                continue

            root_node = TreeNode()

            for stat in stats:
                module_name = stat.module_name
                parts = module_name.split('.')

                # Navigate/create tree structure
                current_node = root_node
                for i, part in enumerate(parts):
                    if i == len(parts) - 1:
                        # Last part - store the file
                        current_node.files.append(stat)
                    else:
                        # Intermediate part - create or navigate
                        if part not in current_node.children:
                            current_node.children[part] = TreeNode()
                        current_node = current_node.children[part]

            # Calculate aggregate stats for this type's tree
            _TreeBuilder._calculate_node_stats(root_node)
            trees[module_type] = root_node

        return trees

    @staticmethod
    def _calculate_node_stats(node: TreeNode) -> Tuple[int, int]:
        """Recursively calculate aggregate statistics for tree nodes.

        Args:
            node: TreeNode to calculate stats for

        Returns:
            Tuple of (total_samples, file_count)
        """
        total_samples = 0
        file_count = 0

        # Count files at this level
        for file_stat in node.files:
            total_samples += file_stat.total_samples
            file_count += 1

        # Recursively process children
        for child in node.children.values():
            child_samples, child_count = _TreeBuilder._calculate_node_stats(child)
            total_samples += child_samples
            file_count += child_count

        node.samples = total_samples
        node.count = file_count
        return total_samples, file_count


class _HtmlRenderer:
    """Renders hierarchical tree structures as HTML."""

    def __init__(self, file_index: Dict[str, str]):
        """Initialize renderer with file index.

        Args:
            file_index: Mapping from filenames to HTML file names
        """
        self.file_index = file_index
        self.heatmap_bar_height = 16

    def render_hierarchical_html(self, trees: Dict[str, TreeNode]) -> str:
        """Build hierarchical HTML with type sections and collapsible module folders.

        Args:
            trees: Dictionary mapping module types to tree roots

        Returns:
            Complete HTML string for all sections
        """
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

            # Project starts expanded, others start collapsed
            is_collapsed = module_type in {'stdlib', 'site-packages', 'other'}
            icon = '▶' if is_collapsed else '▼'
            content_style = ' style="display: none;"' if is_collapsed else ''

            file_word = "file" if tree.count == 1 else "files"
            sample_word = "sample" if tree.samples == 1 else "samples"
            section_html = f'''
<div class="type-section">
  <div class="type-header" onclick="toggleTypeSection(this)">
    <span class="type-icon">{icon}</span>
    <span class="type-title">{type_names[module_type]}</span>
    <span class="type-stats">({tree.count} {file_word}, {tree.samples:n} {sample_word})</span>
  </div>
  <div class="type-content"{content_style}>
'''

            # Render root folders
            root_folders = sorted(tree.children.items(),
                                key=lambda x: x[1].samples, reverse=True)

            for folder_name, folder_node in root_folders:
                section_html += self._render_folder(folder_node, folder_name, level=1)

            # Render root files (files not in any module)
            if tree.files:
                sorted_files = sorted(tree.files, key=lambda x: x.total_samples, reverse=True)
                section_html += '    <div class="files-list">\n'
                for stat in sorted_files:
                    section_html += self._render_file_item(stat, indent='      ')
                section_html += '    </div>\n'

            section_html += '  </div>\n</div>\n'
            sections.append(section_html)

        return '\n'.join(sections)

    def _render_folder(self, node: TreeNode, name: str, level: int = 1) -> str:
        """Render a single folder node recursively.

        Args:
            node: TreeNode to render
            name: Display name for the folder
            level: Nesting level for indentation

        Returns:
            HTML string for this folder and its contents
        """
        indent = '  ' * level
        parts = []

        # Render folder header (collapsed by default)
        file_word = "file" if node.count == 1 else "files"
        sample_word = "sample" if node.samples == 1 else "samples"
        parts.append(f'{indent}<div class="folder-node collapsed" data-level="{level}">')
        parts.append(f'{indent}  <div class="folder-header" onclick="toggleFolder(this)">')
        parts.append(f'{indent}    <span class="folder-icon">▶</span>')
        parts.append(f'{indent}    <span class="folder-name">📁 {html.escape(name)}</span>')
        parts.append(f'{indent}    <span class="folder-stats">'
                     f'({node.count} {file_word}, {node.samples:n} {sample_word})</span>')
        parts.append(f'{indent}  </div>')
        parts.append(f'{indent}  <div class="folder-content" style="display: none;">')

        # Render sub-folders sorted by sample count
        subfolders = sorted(node.children.items(),
                          key=lambda x: x[1].samples, reverse=True)

        for subfolder_name, subfolder_node in subfolders:
            parts.append(self._render_folder(subfolder_node, subfolder_name, level + 1))

        # Render files in this folder
        if node.files:
            sorted_files = sorted(node.files, key=lambda x: x.total_samples, reverse=True)
            parts.append(f'{indent}    <div class="files-list">')
            for stat in sorted_files:
                parts.append(self._render_file_item(stat, indent=f'{indent}      '))
            parts.append(f'{indent}    </div>')

        parts.append(f'{indent}  </div>')
        parts.append(f'{indent}</div>')

        return '\n'.join(parts)

    def _render_file_item(self, stat: FileStats, indent: str = '') -> str:
        """Render a single file item with heatmap bar.

        Args:
            stat: FileStats object
            indent: Indentation string

        Returns:
            HTML string for file item
        """
        full_path = html.escape(stat.filename)
        module_name = html.escape(stat.module_name)

        intensity = stat.percentage / 100.0
        bar_width = min(stat.percentage, 100)

        html_file = self.file_index[stat.filename]
        s = "" if stat.total_samples == 1 else "s"

        return (f'{indent}<div class="file-item">\n'
                f'{indent}  <a href="{html_file}" class="file-link" title="{full_path}">📄 {module_name}</a>\n'
                f'{indent}  <span class="file-samples">{stat.total_samples:n} sample{s}</span>\n'
                f'{indent}  <div class="heatmap-bar-container"><div class="heatmap-bar" style="width: {bar_width}px; height: {self.heatmap_bar_height}px;" data-intensity="{intensity:.3f}"></div></div>\n'
                f'{indent}</div>\n')


# ============================================================================
# Main Collector Class
# ============================================================================

class HeatmapCollector(StackTraceCollector):
    """Collector that generates coverage.py-style heatmap HTML output with line intensity.

    This collector creates detailed HTML reports showing which lines of code
    were executed most frequently during profiling, similar to coverage.py
    but showing execution "heat" rather than just coverage.
    """

    # File naming and formatting constants
    FILE_INDEX_FORMAT = "file_{:04d}.html"

    def __init__(self, *args, **kwargs):
        """Initialize the heatmap collector with data structures for analysis."""
        super().__init__(*args, **kwargs)

        # Sample counting data structures
        self.line_samples = collections.Counter()
        self.file_samples = collections.defaultdict(collections.Counter)
        self.line_self_samples = collections.Counter()
        self.file_self_samples = collections.defaultdict(collections.Counter)

        # Call graph data structures for navigation (sets for O(1) deduplication)
        self.call_graph = collections.defaultdict(set)
        self.callers_graph = collections.defaultdict(set)
        self.function_definitions = {}

        # Edge counting for call path analysis
        self.edge_samples = collections.Counter()

        # Bytecode-level tracking data structures
        # Track samples per (file, lineno) -> {opcode: {'count': N, 'locations': set()}}
        # Locations are deduplicated via set to minimize memory usage
        self.line_opcodes = collections.defaultdict(dict)

        # Statistics and metadata
        self._total_samples = 0
        self._path_info = get_python_path_info()
        self.stats = {}

        # Opcode collection flag
        self.opcodes_enabled = False

        # Template loader (loads all templates once)
        self._template_loader = _TemplateLoader()

        # File index (populated during export)
        self.file_index = {}

        # Reusable set for deduplicating line locations within a single sample.
        # This avoids over-counting recursive functions in cumulative stats.
        self._seen_lines = set()

    def set_stats(self, sample_interval_usec, duration_sec, sample_rate, error_rate=None, missed_samples=None, **kwargs):
        """Set profiling statistics to include in heatmap output.

        Args:
            sample_interval_usec: Sampling interval in microseconds
            duration_sec: Total profiling duration in seconds
            sample_rate: Effective sampling rate
            error_rate: Optional error rate during profiling
            missed_samples: Optional percentage of missed samples
            **kwargs: Additional statistics to include
        """
        self.stats = {
            "sample_interval_usec": sample_interval_usec,
            "duration_sec": duration_sec,
            "sample_rate": sample_rate,
            "error_rate": error_rate,
            "missed_samples": missed_samples,
            "python_version": sys.version,
            "python_implementation": platform.python_implementation(),
            "platform": platform.platform(),
        }
        self.stats.update(kwargs)

    def process_frames(self, frames, thread_id, weight=1):
        """Process stack frames and count samples per line.

        Args:
            frames: List of (filename, location, funcname, opcode) tuples in
                    leaf-to-root order. location is (lineno, end_lineno, col_offset, end_col_offset).
                    opcode is None if not gathered.
            thread_id: Thread ID for this stack trace
            weight: Number of samples this stack represents (for batched RLE)
        """
        self._total_samples += weight
        self._seen_lines.clear()

        for i, (filename, location, funcname, opcode) in enumerate(frames):
            # Normalize location to 4-tuple format
            lineno, end_lineno, col_offset, end_col_offset = normalize_location(location)

            if not self._is_valid_frame(filename, lineno):
                continue

            # frames[0] is the leaf - where execution is actually happening
            is_leaf = (i == 0)
            line_key = (filename, lineno)
            count_cumulative = line_key not in self._seen_lines
            if count_cumulative:
                self._seen_lines.add(line_key)

            self._record_line_sample(filename, lineno, funcname, is_leaf=is_leaf,
                                     count_cumulative=count_cumulative, weight=weight)

            if opcode is not None:
                # Set opcodes_enabled flag when we first encounter opcode data
                self.opcodes_enabled = True
                self._record_bytecode_sample(filename, lineno, opcode,
                                             end_lineno, col_offset, end_col_offset,
                                             weight=weight)

            # Build call graph for adjacent frames (relationships are deduplicated anyway)
            if i + 1 < len(frames):
                next_frame = frames[i + 1]
                next_lineno = extract_lineno(next_frame[1])
                self._record_call_relationship(
                    (filename, lineno, funcname),
                    (next_frame[0], next_lineno, next_frame[2])
                )

    def _is_valid_frame(self, filename, lineno):
        """Check if a frame should be included in the heatmap."""
        # Skip internal or invalid files
        if not filename or filename.startswith('<') or filename.startswith('['):
            return False

        # Skip invalid frames with corrupted filename data
        if filename == "__init__" and lineno == 0:
            return False

        return True

    def _record_line_sample(self, filename, lineno, funcname, is_leaf=False,
                            count_cumulative=True, weight=1):
        """Record a sample for a specific line."""
        # Track cumulative samples (all occurrences in stack)
        if count_cumulative:
            self.line_samples[(filename, lineno)] += weight
            self.file_samples[filename][lineno] += weight

        # Track self/leaf samples (only when at top of stack)
        if is_leaf:
            self.line_self_samples[(filename, lineno)] += weight
            self.file_self_samples[filename][lineno] += weight

        # Record function definition location
        if funcname and (filename, funcname) not in self.function_definitions:
            self.function_definitions[(filename, funcname)] = lineno

    def _record_bytecode_sample(self, filename, lineno, opcode,
                                end_lineno=None, col_offset=None, end_col_offset=None,
                                weight=1):
        """Record a sample for a specific bytecode instruction.

        Args:
            filename: Source filename
            lineno: Line number
            opcode: Opcode number being executed
            end_lineno: End line number (may be -1 if not available)
            col_offset: Column offset in UTF-8 bytes (may be -1 if not available)
            end_col_offset: End column offset in UTF-8 bytes (may be -1 if not available)
            weight: Number of samples this represents (for batched RLE)
        """
        key = (filename, lineno)

        # Initialize opcode entry if needed - use set for location deduplication
        if opcode not in self.line_opcodes[key]:
            self.line_opcodes[key][opcode] = {'count': 0, 'locations': set()}

        self.line_opcodes[key][opcode]['count'] += weight

        # Store unique location info if column offset is available (not -1)
        if col_offset is not None and col_offset >= 0:
            # Use tuple as set key for deduplication
            loc_key = (end_lineno, col_offset, end_col_offset)
            self.line_opcodes[key][opcode]['locations'].add(loc_key)

    def _get_bytecode_data_for_line(self, filename, lineno):
        """Get bytecode disassembly data for instructions on a specific line.

        Args:
            filename: Source filename
            lineno: Line number

        Returns:
            List of dicts with instruction info, sorted by samples descending
        """
        key = (filename, lineno)
        opcode_data = self.line_opcodes.get(key, {})

        result = []
        for opcode, data in opcode_data.items():
            info = get_opcode_info(opcode)
            # Handle both old format (int count) and new format (dict with count/locations)
            if isinstance(data, dict):
                count = data.get('count', 0)
                raw_locations = data.get('locations', set())
                # Convert set of tuples to list of dicts for JSON serialization
                if isinstance(raw_locations, set):
                    locations = [
                        {'end_lineno': loc[0], 'col_offset': loc[1], 'end_col_offset': loc[2]}
                        for loc in raw_locations
                    ]
                else:
                    locations = raw_locations
            else:
                count = data
                locations = []

            result.append({
                'opcode': opcode,
                'opname': format_opcode(opcode),
                'base_opname': info['base_opname'],
                'is_specialized': info['is_specialized'],
                'samples': count,
                'locations': locations,
            })

        # Sort by samples descending, then by opcode number
        result.sort(key=lambda x: (-x['samples'], x['opcode']))
        return result

    def _record_call_relationship(self, callee_frame, caller_frame):
        """Record caller/callee relationship between adjacent frames."""
        callee_filename, callee_lineno, callee_funcname = callee_frame
        caller_filename, caller_lineno, caller_funcname = caller_frame

        # Skip internal files for call graph
        if callee_filename.startswith('<') or callee_filename.startswith('['):
            return

        # Get the callee's function definition line
        callee_def_line = self.function_definitions.get(
            (callee_filename, callee_funcname), callee_lineno
        )

        # Record caller -> callee relationship (set handles deduplication)
        caller_key = (caller_filename, caller_lineno)
        callee_info = (callee_filename, callee_def_line, callee_funcname)
        self.call_graph[caller_key].add(callee_info)

        # Record callee <- caller relationship (set handles deduplication)
        callee_key = (callee_filename, callee_def_line)
        caller_info = (caller_filename, caller_lineno, caller_funcname)
        self.callers_graph[callee_key].add(caller_info)

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
        """Create output directory for heatmap files."""
        output_dir = Path(output_path)
        if output_dir.suffix == '.html':
            output_dir = output_dir.with_suffix('')

        try:
            output_dir.mkdir(exist_ok=True, parents=True)
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to create output directory {output_dir}: {e}") from e

        return output_dir

    def _create_file_index(self, file_stats: List[FileStats]):
        """Create mapping from filenames to HTML file names."""
        self.file_index = {
            stat.filename: self.FILE_INDEX_FORMAT.format(i)
            for i, stat in enumerate(file_stats)
        }

    def _generate_file_reports(self, output_dir, file_stats: List[FileStats]):
        """Generate HTML report for each source file."""
        for stat in file_stats:
            file_path = output_dir / self.file_index[stat.filename]
            line_counts = self.file_samples[stat.filename]
            valid_line_counts = {line: count for line, count in line_counts.items() if line >= 0}

            self_counts = self.file_self_samples.get(stat.filename, {})
            valid_self_counts = {line: count for line, count in self_counts.items() if line >= 0}

            self._generate_file_html(
                file_path,
                stat.filename,
                valid_line_counts,
                valid_self_counts,
                stat
            )

    def _print_export_summary(self, output_dir, file_stats: List[FileStats]):
        """Print summary of exported heatmap."""
        print(f"Heatmap output written to {output_dir}/")
        print(f"  - Index: {output_dir / 'index.html'}")
        s = "" if len(file_stats) == 1 else "s"
        print(f"  - {len(file_stats)} source file{s} analyzed")

    def _calculate_file_stats(self) -> List[FileStats]:
        """Calculate statistics for each file.

        Returns:
            List of FileStats objects sorted by total samples
        """
        file_stats = []
        for filename, line_counts in self.file_samples.items():
            # Skip special frames
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

            file_stats.append(FileStats(
                filename=filename,
                module_name=module_name,
                module_type=module_type,
                total_samples=total_samples,
                total_self_samples=total_self_samples,
                num_lines=num_lines,
                max_samples=max_samples,
                max_self_samples=max_self_samples,
                percentage=0.0
            ))

        # Sort by total samples and calculate percentages
        file_stats.sort(key=lambda x: x.total_samples, reverse=True)
        if file_stats:
            max_total = file_stats[0].total_samples
            for stat in file_stats:
                stat.percentage = (stat.total_samples / max_total * 100) if max_total > 0 else 0

        return file_stats

    def _generate_index_html(self, index_path: Path, file_stats: List[FileStats]):
        """Generate index.html with list of all profiled files."""
        # Build hierarchical tree
        tree = _TreeBuilder.build_file_tree(file_stats)

        # Render tree as HTML
        renderer = _HtmlRenderer(self.file_index)
        sections_html = renderer.render_hierarchical_html(tree)

        # Format error rate and missed samples with bar classes
        error_rate = self.stats.get('error_rate')
        if error_rate is not None:
            error_rate_str = f"{fmt(error_rate)}%"
            error_rate_width = min(error_rate, 100)
            # Determine bar color class based on rate
            if error_rate < 5:
                error_rate_class = "good"
            elif error_rate < 15:
                error_rate_class = "warning"
            else:
                error_rate_class = "error"
        else:
            error_rate_str = "N/A"
            error_rate_width = 0
            error_rate_class = "good"

        missed_samples = self.stats.get('missed_samples')
        if missed_samples is not None:
            missed_samples_str = f"{fmt(missed_samples)}%"
            missed_samples_width = min(missed_samples, 100)
            if missed_samples < 5:
                missed_samples_class = "good"
            elif missed_samples < 15:
                missed_samples_class = "warning"
            else:
                missed_samples_class = "error"
        else:
            missed_samples_str = "N/A"
            missed_samples_width = 0
            missed_samples_class = "good"

        # Populate template
        replacements = {
            "<!-- INLINE_CSS -->": f"<style>\n{self._template_loader.index_css}\n</style>",
            "<!-- INLINE_JS -->": f"<script>\n{self._template_loader.index_js}\n</script>",
            "<!-- PYTHON_LOGO -->": self._template_loader.logo_html,
            "<!-- PYTHON_VERSION -->": f"{sys.version_info.major}.{sys.version_info.minor}",
            "<!-- NUM_FILES -->": f"{len(file_stats):n}",
            "<!-- TOTAL_SAMPLES -->": f"{self._total_samples:n}",
            "<!-- DURATION -->": fmt(self.stats.get('duration_sec', 0)),
            "<!-- SAMPLE_RATE -->": fmt(self.stats.get('sample_rate', 0)),
            "<!-- ERROR_RATE -->": error_rate_str,
            "<!-- ERROR_RATE_WIDTH -->": str(error_rate_width),
            "<!-- ERROR_RATE_CLASS -->": error_rate_class,
            "<!-- MISSED_SAMPLES -->": missed_samples_str,
            "<!-- MISSED_SAMPLES_WIDTH -->": str(missed_samples_width),
            "<!-- MISSED_SAMPLES_CLASS -->": missed_samples_class,
            "<!-- SECTIONS_HTML -->": sections_html,
        }

        html_content = self._template_loader.index_template
        for placeholder, value in replacements.items():
            html_content = html_content.replace(placeholder, value)

        try:
            index_path.write_text(html_content, encoding='utf-8')
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to write index file {index_path}: {e}") from e

    def _generate_file_html(self, output_path: Path, filename: str,
                          line_counts: Dict[int, int], self_counts: Dict[int, int],
                          file_stat: FileStats):
        """Generate HTML for a single source file with heatmap coloring."""
        # Read source file
        try:
            source_lines = Path(filename).read_text(encoding='utf-8', errors='replace').splitlines()
        except (IOError, OSError) as e:
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
            "<!-- TOTAL_SAMPLES -->": f"{file_stat.total_samples:n}",
            "<!-- TOTAL_SELF_SAMPLES -->": f"{file_stat.total_self_samples:n}",
            "<!-- NUM_LINES -->": f"{file_stat.num_lines:n}",
            "<!-- PERCENTAGE -->": fmt(file_stat.percentage, 2),
            "<!-- MAX_SAMPLES -->": f"{file_stat.max_samples:n}",
            "<!-- MAX_SELF_SAMPLES -->": f"{file_stat.max_self_samples:n}",
            "<!-- CODE_LINES -->": ''.join(code_lines_html),
            "<!-- INLINE_CSS -->": f"<style>\n{self._template_loader.file_css}\n</style>",
            "<!-- INLINE_JS -->": f"<script>\n{self._template_loader.file_js}\n</script>",
            "<!-- PYTHON_LOGO -->": self._template_loader.logo_html,
            "<!-- PYTHON_VERSION -->": f"{sys.version_info.major}.{sys.version_info.minor}",
        }

        html_content = self._template_loader.file_template
        for placeholder, value in replacements.items():
            html_content = html_content.replace(placeholder, value)

        try:
            output_path.write_text(html_content, encoding='utf-8')
        except (IOError, OSError) as e:
            raise RuntimeError(f"Failed to write file {output_path}: {e}") from e

    def _build_line_html(self, line_num: int, line_content: str,
                        line_counts: Dict[int, int], self_counts: Dict[int, int],
                        max_samples: int, max_self_samples: int, filename: str) -> str:
        """Build HTML for a single line of source code."""
        cumulative_samples = line_counts.get(line_num, 0)
        self_samples = self_counts.get(line_num, 0)

        # Calculate colors for both self and cumulative modes
        if cumulative_samples > 0:
            log_cumulative = math.log(cumulative_samples + 1)
            log_max = math.log(max_samples + 1)
            cumulative_intensity = log_cumulative / log_max if log_max > 0 else 0

            if self_samples > 0 and max_self_samples > 0:
                log_self = math.log(self_samples + 1)
                log_max_self = math.log(max_self_samples + 1)
                self_intensity = log_self / log_max_self if log_max_self > 0 else 0
            else:
                self_intensity = 0

            self_display = f"{self_samples:n}" if self_samples > 0 else ""
            cumulative_display = f"{cumulative_samples:n}"
            tooltip = f"Self: {self_samples:n}, Total: {cumulative_samples:n}"
        else:
            cumulative_intensity = 0
            self_intensity = 0
            self_display = ""
            cumulative_display = ""
            tooltip = ""

        # Get bytecode data for this line (if any)
        bytecode_data = self._get_bytecode_data_for_line(filename, line_num)
        has_bytecode = len(bytecode_data) > 0 and cumulative_samples > 0

        # Build bytecode toggle button if data is available
        bytecode_btn_html = ''
        bytecode_panel_html = ''
        if has_bytecode:
            bytecode_json = html.escape(json.dumps(bytecode_data))

            # Calculate specialization percentage
            total_samples = sum(d['samples'] for d in bytecode_data)
            specialized_samples = sum(d['samples'] for d in bytecode_data if d['is_specialized'])
            spec_pct = int(100 * specialized_samples / total_samples) if total_samples > 0 else 0

            bytecode_btn_html = (
                f'<button class="bytecode-toggle" data-bytecode=\'{bytecode_json}\' '
                f'data-spec-pct="{spec_pct}" '
                f'onclick="toggleBytecode(this)" title="Show bytecode">&#9654;</button>'
            )
            # Wrapper contains columns + content panel
            bytecode_panel_html = (
                f'        <div class="bytecode-wrapper" id="bytecode-wrapper-{line_num}">\n'
                f'            <div class="bytecode-columns">'
                f'<div class="line-number"></div>'
                f'<div class="line-samples-self"></div>'
                f'<div class="line-samples-cumulative"></div>'
                f'</div>\n'
                f'            <div class="bytecode-panel" id="bytecode-{line_num}"></div>\n'
                f'        </div>\n'
            )
        elif self.opcodes_enabled:
            # Add invisible spacer to maintain consistent indentation when opcodes are enabled
            bytecode_btn_html = '<div class="bytecode-spacer"></div>'

        # Get navigation buttons
        nav_buttons_html = self._build_navigation_buttons(filename, line_num)

        # Build line HTML with instruction highlights if available
        line_html = self._render_source_with_highlights(line_content, line_num,
                                                         filename, bytecode_data)
        title_attr = f' title="{html.escape(tooltip)}"' if tooltip else ""

        # Specialization color for toggle mode (green gradient based on spec %)
        spec_color_attr = ''
        if has_bytecode:
            spec_color = self._format_specialization_color(spec_pct)
            spec_color_attr = f'data-spec-color="{spec_color}" '

        return (
            f'        <div class="code-line" '
            f'data-self-intensity="{self_intensity:.3f}" '
            f'data-cumulative-intensity="{cumulative_intensity:.3f}" '
            f'{spec_color_attr}'
            f'id="line-{line_num}"{title_attr}>\n'
            f'            <div class="line-number">{line_num}</div>\n'
            f'            <div class="line-samples-self">{self_display}</div>\n'
            f'            <div class="line-samples-cumulative">{cumulative_display}</div>\n'
            f'            {bytecode_btn_html}\n'
            f'            <div class="line-content">{line_html}</div>\n'
            f'            {nav_buttons_html}\n'
            f'        </div>\n'
            f'{bytecode_panel_html}'
        )

    def _render_source_with_highlights(self, line_content: str, line_num: int,
                                        filename: str, bytecode_data: list) -> str:
        """Render source line with instruction highlight spans.

        Simple: collect ranges with sample counts, assign each byte position to
        smallest covering range, then emit spans for contiguous runs with sample data.
        """
        content = line_content.rstrip('\n')
        if not content:
            return ''

        # Collect all (start, end) -> {samples, opcodes} mapping from instructions
        # Multiple instructions may share the same range, so we sum samples and collect opcodes
        range_data = {}
        for instr in bytecode_data:
            samples = instr.get('samples', 0)
            opname = instr.get('opname', '')
            for loc in instr.get('locations', []):
                if loc.get('end_lineno', line_num) == line_num:
                    start, end = loc.get('col_offset', -1), loc.get('end_col_offset', -1)
                    if start >= 0 and end >= 0:
                        key = (start, end)
                        if key not in range_data:
                            range_data[key] = {'samples': 0, 'opcodes': []}
                        range_data[key]['samples'] += samples
                        if opname and opname not in range_data[key]['opcodes']:
                            range_data[key]['opcodes'].append(opname)

        if not range_data:
            return html.escape(content)

        # For each byte position, find the smallest covering range
        byte_to_range = {}
        for (start, end) in range_data.keys():
            for pos in range(start, end):
                if pos not in byte_to_range:
                    byte_to_range[pos] = (start, end)
                else:
                    # Keep smaller range
                    old_start, old_end = byte_to_range[pos]
                    if (end - start) < (old_end - old_start):
                        byte_to_range[pos] = (start, end)

        # Calculate totals for percentage and intensity
        total_line_samples = sum(d['samples'] for d in range_data.values())
        max_range_samples = max(d['samples'] for d in range_data.values()) if range_data else 1

        # Render character by character
        result = []
        byte_offset = 0
        char_idx = 0
        current_range = None
        span_chars = []

        def flush_span():
            nonlocal span_chars, current_range
            if span_chars:
                text = html.escape(''.join(span_chars))
                if current_range:
                    data = range_data.get(current_range, {'samples': 0, 'opcodes': []})
                    samples = data['samples']
                    opcodes = ', '.join(data['opcodes'][:3])  # Top 3 opcodes
                    if len(data['opcodes']) > 3:
                        opcodes += f" +{len(data['opcodes']) - 3} more"
                    pct = int(100 * samples / total_line_samples) if total_line_samples > 0 else 0
                    result.append(f'<span class="instr-span" '
                                  f'data-col-start="{current_range[0]}" '
                                  f'data-col-end="{current_range[1]}" '
                                  f'data-samples="{samples}" '
                                  f'data-max-samples="{max_range_samples}" '
                                  f'data-pct="{pct}" '
                                  f'data-opcodes="{html.escape(opcodes)}">{text}</span>')
                else:
                    result.append(text)
                span_chars = []

        while char_idx < len(content):
            char = content[char_idx]
            char_bytes = len(char.encode('utf-8'))
            char_range = byte_to_range.get(byte_offset)

            if char_range != current_range:
                flush_span()
                current_range = char_range

            span_chars.append(char)
            byte_offset += char_bytes
            char_idx += 1

        flush_span()
        return ''.join(result)

    def _format_specialization_color(self, spec_pct: int) -> str:
        """Format specialization color based on percentage.

        Uses a gradient from gray (0%) through orange (50%) to green (100%).
        """
        # Normalize to 0-1
        ratio = spec_pct / 100.0

        if ratio >= 0.5:
            # Orange to green (50-100%)
            t = (ratio - 0.5) * 2  # 0 to 1
            r = int(255 * (1 - t))  # 255 -> 0
            g = int(180 + 75 * t)   # 180 -> 255
            b = int(50 * (1 - t))   # 50 -> 0
        else:
            # Gray to orange (0-50%)
            t = ratio * 2  # 0 to 1
            r = int(158 + 97 * t)   # 158 -> 255
            g = int(158 + 22 * t)   # 158 -> 180
            b = int(158 - 108 * t)  # 158 -> 50

        alpha = 0.15 + 0.25 * ratio  # 0.15 to 0.4
        return f"rgba({r}, {g}, {b}, {alpha})"

    def _build_navigation_buttons(self, filename: str, line_num: int) -> str:
        """Build navigation buttons for callers/callees."""
        line_key = (filename, line_num)
        caller_list = self._deduplicate_by_function(self.callers_graph.get(line_key, set()))
        callee_list = self._deduplicate_by_function(self.call_graph.get(line_key, set()))

        # Get edge counts for each caller/callee
        callers_with_counts = self._get_edge_counts(line_key, caller_list, is_caller=True)
        callees_with_counts = self._get_edge_counts(line_key, callee_list, is_caller=False)

        # Build navigation buttons with counts
        caller_btn = self._create_navigation_button(callers_with_counts, 'caller', '▲')
        callee_btn = self._create_navigation_button(callees_with_counts, 'callee', '▼')

        if caller_btn or callee_btn:
            return f'<div class="line-nav-buttons">{caller_btn}{callee_btn}</div>'
        return ''

    def _get_edge_counts(self, line_key: Tuple[str, int],
                        items: List[Tuple[str, int, str]],
                        is_caller: bool) -> List[Tuple[str, int, str, int]]:
        """Get sample counts for each caller/callee edge."""
        result = []
        for file, line, func in items:
            edge_line_key = (file, line)
            if is_caller:
                edge_key = (edge_line_key, line_key)
            else:
                edge_key = (line_key, edge_line_key)

            count = self.edge_samples.get(edge_key, 0)
            result.append((file, line, func, count))

        result.sort(key=lambda x: x[3], reverse=True)
        return result

    def _deduplicate_by_function(self, items) -> List[Tuple[str, int, str]]:
        """Remove duplicate entries based on (file, function) key.

        Args:
            items: Iterable of (file, line, func) tuples (set or list)
        """
        seen = {}
        result = []
        for file, line, func in items:
            key = (file, func)
            if key not in seen:
                seen[key] = True
                result.append((file, line, func))
        return result

    def _create_navigation_button(self, items_with_counts: List[Tuple[str, int, str, int]],
                                 btn_class: str, arrow: str) -> str:
        """Create HTML for a navigation button with sample counts."""
        # Filter valid items
        valid_items = [(f, l, fn, cnt) for f, l, fn, cnt in items_with_counts
                      if f in self.file_index and l > 0]
        if not valid_items:
            return ""

        if len(valid_items) == 1:
            file, line, func, count = valid_items[0]
            target_html = self.file_index[file]
            nav_data = json.dumps({'link': f"{target_html}#line-{line}", 'func': func})
            title = f"Go to {btn_class}: {html.escape(func)} ({count:n} samples)"
            return f'<button class="nav-btn {btn_class}" data-nav=\'{html.escape(nav_data)}\' title="{title}">{arrow}</button>'

        # Multiple items - create menu
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
        title = f"{len(items_data)} {btn_class}s ({total_samples:n} samples)"
        return f'<button class="nav-btn {btn_class}" data-nav-multi=\'{items_json}\' title="{title}">{arrow}</button>'
