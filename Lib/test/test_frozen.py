"""Basic test of the frozen module (source is in Python/frozen.c)."""

# The Python/frozen.c source code contains a marshalled Python module
# and therefore depends on the marshal format as well as the bytecode
# format.  If those formats have been changed then frozen.c needs to be
# updated.
#
# The test_importlib also tests this module but because those tests
# are much more complicated, it might be unclear why they are failing.
# Invalid marshalled data in frozen.c could case the interpreter to
# crash when __hello__ is imported.

import sys
import unittest
from test.support import captured_stdout
from importlib import util


class TestFrozen(unittest.TestCase):
    def test_frozen(self):
        name = '__hello__'
        if name in sys.modules:
            del sys.modules[name]
        with captured_stdout() as out:
            import __hello__
        self.assertEqual(out.getvalue(), 'Hello world!\n')


if __name__ == '__main__':
    unittest.main()
