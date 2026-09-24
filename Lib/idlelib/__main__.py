"""
IDLE main entry point

Run IDLE as python -m idlelib
"""
import sys

if not sys.flags.safe_path:
    # Remove the current directory, prepended by "python -m", so that
    # user files do not shadow IDLE's imports (gh-70331).
    del sys.path[0]

import idlelib.pyshell
idlelib.pyshell.main()
