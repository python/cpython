# Test the frozen module defined in frozen.c.

from test.test_support import TestFailed
import sys, os

try:
    import __hello__
except ImportError, x:
    raise TestFailed, "import __hello__ failed:" + str(x)

try:
    import __phello__
except ImportError, x:
    raise TestFailed, "import __phello__ failed:" + str(x)

try:
    import __phello__.spam
except ImportError, x:
    raise TestFailed, "import __phello__.spam failed:" + str(x)

if sys.platform != "mac":  # On the Mac this import does succeed.
    try:
        import __phello__.foo
    except ImportError:
        pass
    else:
        raise TestFailed, "import __phello__.foo should have failed"
