import os

trace_filename = os.environ.get("PYREPL_TRACE")

if trace_filename is not None:
    trace_file = open(trace_filename, "a")
else:
    trace_file = None


def trace(line, *k, **kw):
    if trace_file is None:
        return
    if k or kw:
        line = line.format(*k, **kw)
    trace_file.write(line + "\n")
    trace_file.flush()
