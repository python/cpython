from test.test_importlib import util

machinery = util.import_importlib('importlib.machinery')

import unittest


class PathHookTest:

    """Test the path hook for source."""

    def path_hook(self):
        return self.machinery.FileFinder.path_hook((self.machinery.SourceFileLoader,
            self.machinery.SOURCE_SUFFIXES))

    def test_success(self):
        with util.create_modules('dummy') as mapping:
            self.assertTrue(hasattr(self.path_hook()(mapping['.root']),
                                    'find_spec'))

    def test_empty_string(self):
        # The empty string represents the cwd.
        self.assertTrue(hasattr(self.path_hook()(''), 'find_spec'))


(Frozen_PathHookTest,
 Source_PathHooktest
 ) = util.test_both(PathHookTest, machinery=machinery)


if __name__ == '__main__':
    unittest.main()
