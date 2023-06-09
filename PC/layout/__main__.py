import os
import sys

try:
    import layout
except ImportError:
    # Failed to import our package, which likely means we were started directly
    # Add the additional search path needed to locate our module.
    sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.realpath(__file__))))

from layout.main import main

sys.exit(int(main() or 0))
