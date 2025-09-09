import base64
import collections
import functools
import importlib.resources
import json
import linecache
import os

from .collector import Collector


class StackTraceCollector(Collector):
    def __init__(self):
        self.call_trees = []
        self.function_samples = collections.defaultdict(int)

    def _process_frames(self, frames):
        """Process a single thread's frame stack."""
        if not frames:
            return

        # Store the complete call stack (reverse order - root first)
        call_tree = list(reversed(frames))
        self.call_trees.append(call_tree)

        # Count samples per function
        for frame in frames:
            self.function_samples[frame] += 1

    def collect(self, stack_frames):
        for frames in self._iter_all_frames(stack_frames):
            self._process_frames(frames)


class CollapsedStackCollector(StackTraceCollector):
    def export(self, filename):
        stack_counter = collections.Counter()
        for call_tree in self.call_trees:
            # Call tree is already in root->leaf order
            stack_str = ";".join(
                f"{os.path.basename(f[0])}:{f[2]}:{f[1]}" for f in call_tree
            )
            stack_counter[stack_str] += 1

        with open(filename, "w") as f:
            for stack, count in stack_counter.items():
                f.write(f"{stack} {count}\n")
        print(f"Collapsed stack output written to {filename}")


class FlamegraphCollector(StackTraceCollector):
    def __init__(self):
        super().__init__()
        self.stats = {}

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

        # Debug output
        num_functions = len(flamegraph_data.get("children", []))
        total_time = flamegraph_data.get("value", 0)
        print(
            f"Flamegraph data: {num_functions} root functions, total samples: {total_time}"
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

    @functools.lru_cache(maxsize=None)
    def _format_function_name(self, func):
        filename, lineno, funcname = func

        if len(filename) > 50:
            parts = filename.split("/")
            if len(parts) > 2:
                filename = f".../{'/'.join(parts[-2:])}"

        return f"{funcname} ({filename}:{lineno})"

    def _convert_to_flamegraph_format(self):
        """Convert call trees to d3-flamegraph format with optimized hierarchy building"""
        if not self.call_trees:
            return {"name": "No Data", "value": 0, "children": []}

        unique_functions = set()
        for call_tree in self.call_trees:
            unique_functions.update(call_tree)

        func_to_name = {
            func: self._format_function_name(func) for func in unique_functions
        }

        root = {"name": "root", "children": {}, "samples": 0}

        for call_tree in self.call_trees:
            current_node = root
            current_node["samples"] += 1

            for func in call_tree:
                func_name = func_to_name[func]  # Use pre-computed name

                if func_name not in current_node["children"]:
                    current_node["children"][func_name] = {
                        "name": func_name,
                        "func": func,
                        "children": {},
                        "samples": 0,
                        "filename": func[0],
                        "lineno": func[1],
                        "funcname": func[2],
                    }

                current_node = current_node["children"][func_name]
                current_node["samples"] += 1

        def convert_node(node, min_samples=1):
            if node["samples"] < min_samples:
                return None

            source_code = None
            if "func" in node:
                source_code = self._get_source_lines(node["func"])

            result = {
                "name": node["name"],
                "value": node["samples"],
                "children": [],
            }

            if "filename" in node:
                result.update(
                    {
                        "filename": node["filename"],
                        "lineno": node["lineno"],
                        "funcname": node["funcname"],
                    }
                )

            if source_code:
                result["source"] = source_code

            # Recursively convert children
            child_nodes = []
            for child_name, child_node in node["children"].items():
                child_result = convert_node(child_node, min_samples)
                if child_result:
                    child_nodes.append(child_result)

            # Sort children by sample count (descending)
            child_nodes.sort(key=lambda x: x["value"], reverse=True)
            result["children"] = child_nodes

            return result

        # Filter out very small functions (less than 0.1% of total samples)
        total_samples = len(self.call_trees)
        min_samples = max(1, int(total_samples * 0.001))

        converted_root = convert_node(root, min_samples)

        if not converted_root or not converted_root["children"]:
            return {"name": "No significant data", "value": 0, "children": []}

        # If we only have one root child, make it the root to avoid redundant level
        if len(converted_root["children"]) == 1:
            main_child = converted_root["children"][0]
            main_child["name"] = f"Program Root: {main_child['name']}"
            main_child["stats"] = self.stats
            return main_child

        converted_root["name"] = "Program Root"
        converted_root["stats"] = self.stats
        return converted_root

    def _get_source_lines(self, func):
        filename, lineno, funcname = func

        try:
            # Get several lines around the function definition
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
            # If we can't get source code, return None
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
