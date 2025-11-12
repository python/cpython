import sys

try:
    import layout  # noqa: F401
except ImportError:
    # Failed to import our package, which likely means we were started directly
    # Add the additional search path needed to locate our module.
    from pathlib import Path

    sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from layout.main import main

sys.exit(int(main() or 0))
