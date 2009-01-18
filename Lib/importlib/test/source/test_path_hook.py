import importlib
from .. import support
import unittest


class PathHookTest(unittest.TestCase):

    """Test the path hook for source."""

    def test_success(self):
        # XXX Only work on existing directories?
        with support.create_modules('dummy') as mapping:
            self.assert_(hasattr(importlib.FileImporter(mapping['.root']),
                                 'find_module'))


def test_main():
    from test.support import run_unittest
    run_unittest(PathHookTest)


if __name__ == '__main__':
    test_main()
