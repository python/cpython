from test.test_importlib import abc, util

machinery = util.import_importlib('importlib.machinery')

import unittest
import sys


class FinderTests(abc.FinderTests):

    """Test the finder for extension modules."""

    def setUp(self):
        if not self.machinery.EXTENSION_SUFFIXES:
            raise unittest.SkipTest("Requires dynamic loading support.")
        if util.EXTENSIONS.name in sys.builtin_module_names:
            raise unittest.SkipTest(
                f"{util.EXTENSIONS.name} is a builtin module"
            )

    def find_spec(self, fullname):
        importer = self.machinery.FileFinder(util.EXTENSIONS.path,
                                            (self.machinery.ExtensionFileLoader,
                                             self.machinery.EXTENSION_SUFFIXES))

        return importer.find_spec(fullname)

    def test_module(self):
        self.assertTrue(self.find_spec(util.EXTENSIONS.name))

    # No extension module as an __init__ available for testing.
    test_package = test_package_in_package = None

    # No extension module in a package available for testing.
    test_module_in_package = None

    # Extension modules cannot be an __init__ for a package.
    test_package_over_module = None

    def test_failure(self):
        self.assertIsNone(self.find_spec('asdfjkl;'))


(Frozen_FinderTests,
 Source_FinderTests
 ) = util.test_both(FinderTests, machinery=machinery)


if __name__ == '__main__':
    unittest.main()
