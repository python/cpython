"""Basic test of the frozen module (source is in Python/frozen.c)."""

# The Python/frozen.c source code contains a marshalled Python module
# and therefore depends on the marshal format as well as the bytecode
# format.  If those formats have been changed then frozen.c needs to be
# updated.
#
# The test_importlib also tests this module but because those tests
# are much more complicated, it might be unclear why they are failing.

import sys
import unittest
from test.support import captured_stdout, impl_detail


class TestFrozen(unittest.TestCase):
    def test_frozen(self):
        name = '__hello__'
        if name in sys.modules:
            del sys.modules[name]
        # Invalid marshalled data in frozen.c could case the interpreter to
        # crash when __hello__ is imported.  Ensure we can import the module
        # and that it generates the correct output.
        with captured_stdout() as out:
            import __hello__
        self.assertEqual(out.getvalue(), 'Hello world!\n')

    @impl_detail('code object line table', cpython=True)
    def test_frozen_linetab(self):
        with captured_stdout():
            import __hello__
            # get the code object for the module
            co = __hello__.__spec__.loader.get_code('__hello__')
        # verify that line number table is as expected.  This is intended to
        # catch bugs like bpo-43372.  The __hello__ module would import
        # successfully but the frozen code was out-of-date and co_lines()
        # would cause a crash.
        self.assertEqual(list(co.co_lines()), [(0, 4, 1), (4, 16, 2)])


if __name__ == '__main__':
    unittest.main()
