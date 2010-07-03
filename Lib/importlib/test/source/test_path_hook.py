from importlib import _bootstrap
from . import util as source_util
import unittest


class PathHookTest(unittest.TestCase):

    """Test the path hook for source."""

    def test_success(self):
        with source_util.create_modules('dummy') as mapping:
            self.assertTrue(hasattr(_bootstrap._file_path_hook(mapping['.root']),
                                 'find_module'))

    def test_empty_string(self):
        # The empty string represents the cwd.
        self.assertTrue(hasattr(_bootstrap._file_path_hook(''), 'find_module'))


def test_main():
    from test.support import run_unittest
    run_unittest(PathHookTest)


if __name__ == '__main__':
    test_main()
