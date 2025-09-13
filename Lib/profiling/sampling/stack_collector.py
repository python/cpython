import base64
import collections
import functools
import importlib.resources
import json
import linecache
import os

from .collector import Collector


class StackTraceCollector(Collector):
    def collect(self, stack_frames):
        for frames in self._iter_all_frames(stack_frames):
            if not frames:
                continue
            self.process_frames(frames)

    def process_frames(self, frames):
        pass


class CollapsedStackCollector(StackTraceCollector):
    def __init__(self):
        self.stack_counter = collections.Counter()

    def process_frames(self, frames):
        call_tree = tuple(reversed(frames))
        self.stack_counter[call_tree] += 1

    def export(self, filename):
        lines = []
        for call_tree, count in self.stack_counter.items():
            stack_str = ";".join(
                f"{os.path.basename(f[0])}:{f[2]}:{f[1]}" for f in call_tree
            )
            lines.append((stack_str, count))

        lines.sort(key=lambda x: (-x[1], x[0]))

        with open(filename, "w") as f:
            for stack, count in lines:
                f.write(f"{stack} {count}\n")
        print(f"Collapsed stack output written to {filename}")


class FlamegraphCollector(StackTraceCollector):
    def __init__(self):
        self.stats = {}
        self._root = {"samples": 0, "children": {}}
        self._total_samples = 0
        self._func_intern = {}

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
        """Convert aggregated trie to d3-flamegraph format."""
        if self._total_samples == 0:
            return {"name": "No Data", "value": 0, "children": []}

        def convert_children(children, min_samples):
            out = []
            for func, node in children.items():
                samples = node["samples"]
                if samples < min_samples:
                    continue

                name = self._format_function_name(func)
                child_entry = {
                    "name": name,
                    "value": samples,
                    "children": [],
                    "filename": func[0],
                    "lineno": func[1],
                    "funcname": func[2],
                }

                source = self._get_source_lines(func)
                if source:
                    child_entry["source"] = source

                # Recurse
                child_entry["children"] = convert_children(
                    node["children"], min_samples
                )
                out.append(child_entry)

            out.sort(key=lambda x: (-x["value"], x["name"]))
            return out

        # Filter out very small functions (less than 0.1% of total samples)
        total_samples = self._total_samples
        min_samples = max(1, int(total_samples * 0.001))

        root_children = convert_children(self._root["children"], min_samples)
        if not root_children:
            return {"name": "No significant data", "value": 0, "children": []}

        # If we only have one root child, make it the root to avoid redundant level
        if len(root_children) == 1:
            main_child = root_children[0]
            main_child["name"] = f"Program Root: {main_child['name']}"
            main_child["stats"] = self.stats
            return main_child

        return {"name": "Program Root", "value": total_samples, "children": root_children, "stats": self.stats}

    def process_frames(self, frames):
        # Reverse to root->leaf
        call_tree = reversed(frames)
        self._root["samples"] += 1
        self._total_samples += 1

        current = self._root
        for func in call_tree:
            func = self._func_intern.setdefault(func, func)
            children = current["children"]
            node = children.get(func)
            if node is None:
                node = {"samples": 0, "children": {}}
                children[func] = node
            node["samples"] += 1
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
