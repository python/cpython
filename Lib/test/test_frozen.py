# Test the frozen module defined in frozen.c.
# Currently test_frozen fails:
#   Implementing pep3102(keyword only argument) needs changes in
#   code object, which needs modification to marshal.
#   However, to regenerate hard-coded marshal data in frozen.c,
#   we need to run Tools/freeze/freeze.py, which currently doesn't work
#   because Lib/modulefinder.py cannot handle relative module import
#   This test will keep failing until Lib/modulefinder.py is fixed

from test.test_support import TestFailed
import sys, os

try:
    import __hello__
except ImportError as x:
    raise TestFailed, "import __hello__ failed:" + str(x)

try:
    import __phello__
except ImportError as x:
    raise TestFailed, "import __phello__ failed:" + str(x)

try:
    import __phello__.spam
except ImportError as x:
    raise TestFailed, "import __phello__.spam failed:" + str(x)

if sys.platform != "mac":  # On the Mac this import does succeed.
    try:
        import __phello__.foo
    except ImportError:
        pass
    else:
        raise TestFailed, "import __phello__.foo should have failed"
