"""transpile.py — compatibility shim for the transpiler package.

The transpiler has moved to Tools/configure/transpiler/.
Run via:  uv run Tools/configure/transpile.py [-o OUTPUT]
      or: python -m transpiler [-o OUTPUT]  (with Tools/configure on sys.path)
"""

from __future__ import annotations

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from transpiler.transpile import main  # noqa: E402

if __name__ == "__main__":
    main()
