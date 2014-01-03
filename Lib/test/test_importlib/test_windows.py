from . import util
frozen_machinery, source_machinery = util.import_importlib('importlib.machinery')

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


class Frozen_WindowsRegistryFinderTests(WindowsRegistryFinderTests,
                                        unittest.TestCase):
    machinery = frozen_machinery


class Source_WindowsRegistryFinderTests(WindowsRegistryFinderTests,
                                        unittest.TestCase):
    machinery = source_machinery
