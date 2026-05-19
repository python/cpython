"""
Sphinx extension to generate profiler trace data during docs build.

This extension executes a demo Python program with sys.settrace() to capture
the execution trace and injects it into the profiling visualization JS file.
"""

import json
import re
import sys
from io import StringIO
from pathlib import Path

from sphinx.errors import ExtensionError

DEMO_SOURCE = """\
def add(a, b):
    return a + b

def multiply(x, y):
    result = 0
    for i in range(y):
        result = add(result, x)
    return result

def calculate(a, b):
    sum_val = add(a, b)
    product = multiply(a, b)
    return sum_val + product

def main():
    result = calculate(3, 4)
    print(f"Result: {result}")

main()
"""

PLACEHOLDER = "/* PROFILING_TRACE_DATA */null"


def generate_trace(source: str) -> list[dict]:
    """
    Execute the source code with tracing enabled and capture execution events.
    """
    trace_events = []
    timestamp = [0]
    timestamp_step = 50
    tracing_active = [False]
    pending_line = [None]

    def tracer(frame, event, arg):
        if frame.f_code.co_filename != '<demo>':
            return tracer

        func_name = frame.f_code.co_name
        lineno = frame.f_lineno

        if event == 'line' and not tracing_active[0]:
            pending_line[0] = {'type': 'line', 'line': lineno}
            return tracer

        # Start tracing only once main() is called
        if event == 'call' and func_name == 'main':
            tracing_active[0] = True
            # Emit the buffered line event (the main() call line) at ts=0
            if pending_line[0]:
                pending_line[0]['ts'] = 0
                trace_events.append(pending_line[0])
                pending_line[0] = None
                timestamp[0] = timestamp_step

        # Skip events until we've entered main()
        if not tracing_active[0]:
            return tracer

        if event == 'call':
            trace_events.append({
                'type': 'call',
                'func': func_name,
                'line': lineno,
                'ts': timestamp[0],
            })
        elif event == 'line':
            trace_events.append({
                'type': 'line',
                'line': lineno,
                'ts': timestamp[0],
            })
        elif event == 'return':
            try:
                value = arg if arg is None else repr(arg)
            except Exception:
                value = '<unprintable>'
            trace_events.append({
                'type': 'return',
                'func': func_name,
                'ts': timestamp[0],
                'value': value,
            })

            if func_name == 'main':
                tracing_active[0] = False

        timestamp[0] += timestamp_step
        return tracer

    # Suppress print output during tracing
    old_stdout = sys.stdout
    sys.stdout = StringIO()

    old_trace = sys.gettrace()
    sys.settrace(tracer)
    try:
        code = compile(source, '<demo>', 'exec')
        exec(code, {'__name__': '__main__'})
    finally:
        sys.settrace(old_trace)
        sys.stdout = old_stdout

    return trace_events


def inject_trace(app, exception):
    if exception:
        return

    js_path = (
        Path(app.outdir) / '_static' / 'profiling-sampling-visualization.js'
    )

    if not js_path.exists():
        return

    trace = generate_trace(DEMO_SOURCE)

    demo_data = {'source': DEMO_SOURCE.rstrip(), 'trace': trace, 'samples': []}

    demo_json = json.dumps(demo_data, indent=2)
    content = js_path.read_text(encoding='utf-8')

    pattern = r"(const DEMO_SIMPLE\s*=\s*/\* PROFILING_TRACE_DATA \*/)[^;]+;"

    if re.search(pattern, content):
        content = re.sub(
            pattern, lambda m: f"{m.group(1)} {demo_json};", content
        )
        js_path.write_text(content, encoding='utf-8')
        print(
            f"profiling_trace: Injected {len(trace)} trace events into {js_path.name}"
        )
    else:
        raise ExtensionError(
            f"profiling_trace: Placeholder pattern not found in {js_path.name}"
        )


def setup(app):
    app.connect('build-finished', inject_trace)
    app.add_js_file('profiling-sampling-visualization.js')
    app.add_css_file('profiling-sampling-visualization.css')

    return {
        'version': '1.0',
        'parallel_read_safe': True,
        'parallel_write_safe': True,
    }
