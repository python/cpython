from test.test_importlib import util

machinery = util.import_importlib('importlib.machinery')

import unittest


@unittest.skipIf(util.EXTENSIONS is None or util.EXTENSIONS.filename is None,
                 'dynamic loading not supported or test module not available')
class PathHookTests:

    """Test the path hook for extension modules."""
    # XXX Should it only succeed for pre-existing directories?
    # XXX Should it only work for directories containing an extension module?

    def hook(self, entry):
        return self.machinery.FileFinder.path_hook(
                (self.machinery.ExtensionFileLoader,
                 self.machinery.EXTENSION_SUFFIXES))(entry)

    def test_success(self):
        # Path hook should handle a directory where a known extension module
        # exists.
        self.assertTrue(hasattr(self.hook(util.EXTENSIONS.path), 'find_spec'))


(Frozen_PathHooksTests,
 Source_PathHooksTests
 ) = util.test_both(PathHookTests, machinery=machinery)


if __name__ == '__main__':
    unittest.main()
