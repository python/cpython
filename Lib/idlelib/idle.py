import sys

if __spec__ is not None and not sys.flags.safe_path:
    # Remove the current directory, prepended by "python -m", so that
    # user files do not shadow IDLE's imports (gh-70331).
    del sys.path[0]

import os.path


# Enable running IDLE with idlelib in a non-standard location.
# This was once used to run development versions of IDLE.
# Because PEP 434 declared idle.py a public interface,
# removal should require deprecation.
idlelib_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if idlelib_dir not in sys.path:
    sys.path.insert(0, idlelib_dir)

from idlelib.pyshell import main  # This is subject to change
main()
