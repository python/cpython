# Test the frozen module defined in frozen.c.

from test_support import TestFailed
import sys, os

try:
    import __hello__
except ImportError, x:
    raise TestFailed, "import __hello__ failed:", x

try:
    import __phello__
except ImportError, x:
    raise TestFailed, "import __phello__ failed:", x

try:
    import __phello__.spam
except ImportError, x:
    raise TestFailed, "import __phello__.spam failed:", x

try:
    import __phello__.foo
except ImportError:
    pass
else:
    raise TestFailed, "import __phello__.foo should have failed"
