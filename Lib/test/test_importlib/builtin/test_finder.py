from test.test_importlib import abc, util

machinery = util.import_importlib('importlib.machinery')

import sys
import unittest
import warnings


@unittest.skipIf(util.BUILTINS.good_name is None, 'no reasonable builtin module')
class FindSpecTests(abc.FinderTests):

    """Test find_spec() for built-in modules."""

    def test_module(self):
        # Common case.
        with util.uncache(util.BUILTINS.good_name):
            found = self.machinery.BuiltinImporter.find_spec(util.BUILTINS.good_name)
            self.assertTrue(found)
            self.assertEqual(found.origin, 'built-in')

    # Built-in modules cannot be a package.
    test_package = None

    # Built-in modules cannot be in a package.
    test_module_in_package = None

    # Built-in modules cannot be a package.
    test_package_in_package = None

    # Built-in modules cannot be a package.
    test_package_over_module = None

    def test_failure(self):
        name = 'importlib'
        assert name not in sys.builtin_module_names
        spec = self.machinery.BuiltinImporter.find_spec(name)
        self.assertIsNone(spec)


(Frozen_FindSpecTests,
 Source_FindSpecTests
 ) = util.test_both(FindSpecTests, machinery=machinery)


if __name__ == '__main__':
    unittest.main()
