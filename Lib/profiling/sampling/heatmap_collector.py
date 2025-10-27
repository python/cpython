"""Heatmap collector for line-by-line sample intensity visualization."""

from collections import defaultdict
from pathlib import Path
from .collector import Collector

class HeatmapCollector(Collector):
    """Collect line-level sample counts for heatmap visualization."""
    
    def __init__(self, skip_idle=False):
        self.skip_idle = skip_idle
        # file_path -> line_number -> sample_count
        self.line_samples = defaultdict(lambda: defaultdict(int))
        # file_path -> set of function names seen
        self.file_functions = defaultdict(set)
        
    def collect(self, stack_frames):
        """Collect line-level sample counts from stack frames."""
        for frames, thread_id in self._iter_all_frames(stack_frames, skip_idle=self.skip_idle):
            for frame in frames:
                filename = frame.file_name
                lineno = frame.line_number
                function = frame.function_name
                
                # Skip internal/system files
                if filename.startswith('<') or filename.startswith('frozen '):
                    continue
                    
                self.line_samples[filename][lineno] += 1
                self.file_functions[filename].add(function)
    
    def export(self, filename):
        """Export heatmap data as HTML with color-coded source code."""
        if not self.line_samples:
            print("No samples were collected for heatmap.")
            return
        
        html = self._generate_html()
        
        with open(filename, 'w', encoding='utf-8') as f:
            f.write(html)
        
        print(f"Heatmap saved to {filename}")
    
    def _generate_html(self):
        """Generate HTML heatmap visualization."""
        # Calculate global max for color scaling
        global_max = max(
            max(line_counts.values()) 
            for line_counts in self.line_samples.values()
        )
        
        # Sort files by total samples
        files_by_samples = sorted(
            self.line_samples.items(),
            key=lambda x: sum(x[1].values()),
            reverse=True
        )
        
        # Generate HTML
        html_parts = [self._html_header()]
        
        # Add file sections
        for file_idx, (filepath, line_counts) in enumerate(files_by_samples):
            total_samples = sum(line_counts.values())
            html_parts.append(self._render_file_section(
                filepath, line_counts, total_samples, global_max, file_idx
            ))
        
        html_parts.append(self._html_footer())
        
        return '\n'.join(html_parts)
    
    def _html_header(self):
        """Generate HTML header with styles."""
        return '''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Profile Heatmap</title>
    <style>
        :root {
            --bg-primary: #1e1e1e;
            --bg-secondary: #2d2d2d;
            --text-primary: #d4d4d4;
            --text-secondary: #808080;
            --border: #3e3e3e;
            --accent: #007acc;
        }
        
        body {
            font-family: 'Consolas', 'Monaco', 'Courier New', monospace;
            background: var(--bg-primary);
            color: var(--text-primary);
            margin: 0;
            padding: 20px;
            line-height: 1.6;
        }
        
        h1 {
            color: var(--accent);
            border-bottom: 2px solid var(--border);
            padding-bottom: 10px;
            margin-top: 0;
        }
        
        .legend {
            background: var(--bg-secondary);
            border: 1px solid var(--border);
            border-radius: 4px;
            padding: 15px;
            margin-bottom: 30px;
        }
        
        .legend h2 {
            margin-top: 0;
            color: var(--accent);
            font-size: 1.1em;
        }
        
        .color-scale {
            display: flex;
            height: 30px;
            margin: 10px 0;
            border-radius: 3px;
            overflow: hidden;
        }
        
        .color-scale div {
            flex: 1;
        }
        
        .scale-labels {
            display: flex;
            justify-content: space-between;
            font-size: 0.9em;
            color: var(--text-secondary);
        }
        
        .file-section {
            background: var(--bg-secondary);
            border: 1px solid var(--border);
            border-radius: 4px;
            margin-bottom: 30px;
            overflow: hidden;
        }
        
        .file-header {
            background: var(--accent);
            color: white;
            padding: 12px 15px;
            font-weight: bold;
            cursor: pointer;
            user-select: none;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        
        .file-header:hover {
            background: #005a9e;
        }
        
        .file-stats {
            font-size: 0.9em;
            opacity: 0.9;
        }
        
        .file-content {
            padding: 0;
        }
        
        .code-table {
            width: 100%;
            border-collapse: collapse;
            font-size: 13px;
        }
        
        .code-line {
            transition: background 0.2s;
        }
        
        .code-line:hover {
            background: rgba(255, 255, 255, 0.05) !important;
        }
        
        .line-number {
            padding: 2px 15px;
            text-align: right;
            color: var(--text-secondary);
            border-right: 1px solid var(--border);
            user-select: none;
            min-width: 60px;
            font-weight: bold;
        }
        
        .sample-count {
            padding: 2px 15px;
            text-align: right;
            color: var(--text-secondary);
            border-right: 1px solid var(--border);
            min-width: 80px;
            font-weight: bold;
        }
        
        .code-content {
            padding: 2px 15px;
            white-space: pre;
            overflow-x: auto;
        }
        
        .collapsed .file-content {
            display: none;
        }
        
        .toggle-arrow {
            transition: transform 0.3s;
        }
        
        .collapsed .toggle-arrow {
            transform: rotate(-90deg);
        }
        
        .no-samples {
            opacity: 0.4;
        }
        
        /* Heatmap colors (blue to red) */
        .heat-0 { background-color: #2d2d2d; }
        .heat-1 { background-color: #1a4d6b; }
        .heat-2 { background-color: #2e5f85; }
        .heat-3 { background-color: #3d71a0; }
        .heat-4 { background-color: #4c83ba; }
        .heat-5 { background-color: #5b95d5; }
        .heat-6 { background-color: #7fa8d9; }
        .heat-7 { background-color: #a3bbdd; }
        .heat-8 { background-color: #c8cde2; }
        .heat-9 { background-color: #e8c8c0; }
        .heat-10 { background-color: #f0a89e; }
        .heat-11 { background-color: #f7887b; }
        .heat-12 { background-color: #ff6859; }
        .heat-13 { background-color: #ff4836; }
        .heat-14 { background-color: #ff2814; }
        .heat-15 { background-color: #ff0000; }
    </style>
    <script>
        function toggleFile(id) {
            const section = document.getElementById('file-' + id);
            section.classList.toggle('collapsed');
        }
        
        window.addEventListener('DOMContentLoaded', function() {
            // Expand first file by default, collapse others
            const sections = document.querySelectorAll('.file-section');
            sections.forEach((section, idx) => {
                if (idx > 0) {
                    section.classList.add('collapsed');
                }
            });
        });
    </script>
</head>
<body>
    <h1>🔥 Profile Heatmap</h1>
    <div class="legend">
        <h2>Color Legend</h2>
        <p>Lines are colored based on sample intensity (number of times observed during profiling).</p>
        <div class="color-scale">
            <div style="background: #1a4d6b;"></div>
            <div style="background: #2e5f85;"></div>
            <div style="background: #3d71a0;"></div>
            <div style="background: #4c83ba;"></div>
            <div style="background: #5b95d5;"></div>
            <div style="background: #7fa8d9;"></div>
            <div style="background: #a3bbdd;"></div>
            <div style="background: #c8cde2;"></div>
            <div style="background: #e8c8c0;"></div>
            <div style="background: #f0a89e;"></div>
            <div style="background: #f7887b;"></div>
            <div style="background: #ff6859;"></div>
            <div style="background: #ff4836;"></div>
            <div style="background: #ff2814;"></div>
            <div style="background: #ff0000;"></div>
        </div>
        <div class="scale-labels">
            <span>Cold (few samples)</span>
            <span>Hot (many samples)</span>
        </div>
        <p style="margin-bottom: 0;"><strong>Tip:</strong> Click on file headers to expand/collapse sections.</p>
    </div>
'''
    
    def _render_file_section(self, filepath, line_counts, total_samples, global_max, file_idx):
        """Render a single file section with heatmap."""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                source_lines = f.readlines()
        except (FileNotFoundError, IOError, UnicodeDecodeError):
            source_lines = ["<source code not available>"]
        
        # Calculate percentages and heat levels
        max_samples = max(line_counts.values())
        
        # Build table rows
        rows = []
        for lineno, line in enumerate(source_lines, start=1):
            samples = line_counts.get(lineno, 0)
            
            # Calculate heat level (0-15)
            if samples == 0:
                heat_class = "heat-0 no-samples"
            else:
                # Log scale for better visualization
                import math
                normalized = math.log(samples + 1) / math.log(global_max + 1)
                heat_level = min(15, int(normalized * 15) + 1)
                heat_class = f"heat-{heat_level}"
            
            # Escape HTML in source code
            line_html = line.rstrip('\n').replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
            if not line_html:
                line_html = ' '  # Preserve empty lines
            
            sample_display = str(samples) if samples > 0 else ''
            
            rows.append(f'''            <tr class="code-line {heat_class}">
                <td class="line-number">{lineno}</td>
                <td class="sample-count">{sample_display}</td>
                <td class="code-content">{line_html}</td>
            </tr>''')
        
        # Get relative path for display
        try:
            display_path = str(Path(filepath).resolve())
        except:
            display_path = filepath
        
        percentage = (total_samples / sum(sum(lc.values()) for lc in self.line_samples.values())) * 100
        
        return f'''    <div class="file-section" id="file-{file_idx}">
        <div class="file-header" onclick="toggleFile({file_idx})">
            <span>
                <span class="toggle-arrow">▼</span>
                {display_path}
            </span>
            <span class="file-stats">{total_samples} samples ({percentage:.1f}%)</span>
        </div>
        <div class="file-content">
            <table class="code-table">
{''.join(rows)}
            </table>
        </div>
    </div>'''
    
    def _html_footer(self):
        """Generate HTML footer."""
        return '''</body>
</html>'''

