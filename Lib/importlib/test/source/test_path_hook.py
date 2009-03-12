from importlib import _bootstrap
from . import util as source_util
import unittest


class PathHookTest(unittest.TestCase):

    """Test the path hook for source."""

    def test_success(self):
        # XXX Only work on existing directories?
        with source_util.create_modules('dummy') as mapping:
            self.assert_(hasattr(_bootstrap._FileFinder(mapping['.root']),
                                 'find_module'))


def test_main():
    from test.support import run_unittest
    run_unittest(PathHookTest)


if __name__ == '__main__':
    test_main()
