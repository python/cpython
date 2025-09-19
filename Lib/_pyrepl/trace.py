from __future__ import annotations

import os
import sys

# types
if False:
    from typing import IO


trace_file: IO[str] | None = None
if trace_filename := os.environ.get("PYREPL_TRACE"):
    trace_file = open(trace_filename, "a")



if sys.platform == "emscripten":
    from posix import _emscripten_log

    def trace(line: str, *k: object, **kw: object) -> None:
        if "PYREPL_TRACE" not in os.environ:
            return
        if k or kw:
            line = line.format(*k, **kw)
        _emscripten_log(line)

else:
    def trace(line: str, *k: object, **kw: object) -> None:
        if trace_file is None:
            return
        if k or kw:
            line = line.format(*k, **kw)
        trace_file.write(line + "\n")
        trace_file.flush()
