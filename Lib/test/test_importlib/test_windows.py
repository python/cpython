from . import util as test_util
machinery = test_util.import_importlib('importlib.machinery')

import sys
import unittest


@unittest.skipUnless(sys.platform.startswith('win'), 'requires Windows')
class WindowsRegistryFinderTests:

    # XXX Need a test that finds the spec via the registry.

    def test_find_spec_missing(self):
        spec = self.machinery.WindowsRegistryFinder.find_spec('spam')
        self.assertIs(spec, None)

    def test_find_module_missing(self):
        loader = self.machinery.WindowsRegistryFinder.find_module('spam')
        self.assertIs(loader, None)


(Frozen_WindowsRegistryFinderTests,
 Source_WindowsRegistryFinderTests
 ) = test_util.test_both(WindowsRegistryFinderTests, machinery=machinery)
