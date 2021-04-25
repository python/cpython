from . import util as test_util
machinery = test_util.import_importlib('importlib.machinery')

import os
import re
import sys
import unittest
import warnings
from test import support
from test.support import import_helper
from contextlib import contextmanager
from .util import temp_module

import_helper.import_module('winreg', required_on=['win'])
from winreg import (
    CreateKey, HKEY_CURRENT_USER,
    SetValue, REG_SZ, KEY_ALL_ACCESS,
    EnumKey, CloseKey, DeleteKey, OpenKey
)

def get_platform():
    # Port of distutils.util.get_platform().
    TARGET_TO_PLAT = {
            'x86' : 'win32',
            'x64' : 'win-amd64',
            'arm' : 'win-arm32',
        }
    if ('VSCMD_ARG_TGT_ARCH' in os.environ and
        os.environ['VSCMD_ARG_TGT_ARCH'] in TARGET_TO_PLAT):
        return TARGET_TO_PLAT[os.environ['VSCMD_ARG_TGT_ARCH']]
    elif 'amd64' in sys.version.lower():
        return 'win-amd64'
    elif '(arm)' in sys.version.lower():
        return 'win-arm32'
    elif '(arm64)' in sys.version.lower():
        return 'win-arm64'
    else:
        return sys.platform

def delete_registry_tree(root, subkey):
    try:
        hkey = OpenKey(root, subkey, access=KEY_ALL_ACCESS)
    except OSError:
        # subkey does not exist
        return
    while True:
        try:
            subsubkey = EnumKey(hkey, 0)
        except OSError:
            # no more subkeys
            break
        delete_registry_tree(hkey, subsubkey)
    CloseKey(hkey)
    DeleteKey(root, subkey)

@contextmanager
def setup_module(machinery, name, path=None):
    if machinery.WindowsRegistryFinder.DEBUG_BUILD:
        root = machinery.WindowsRegistryFinder.REGISTRY_KEY_DEBUG
    else:
        root = machinery.WindowsRegistryFinder.REGISTRY_KEY
    key = root.format(fullname=name,
                      sys_version='%d.%d' % sys.version_info[:2])
    try:
        with temp_module(name, "a = 1") as location:
            subkey = CreateKey(HKEY_CURRENT_USER, key)
            if path is None:
                path = location + ".py"
            SetValue(subkey, "", REG_SZ, path)
            yield
    finally:
        if machinery.WindowsRegistryFinder.DEBUG_BUILD:
            key = os.path.dirname(key)
        delete_registry_tree(HKEY_CURRENT_USER, key)


@unittest.skipUnless(sys.platform.startswith('win'), 'requires Windows')
class WindowsRegistryFinderTests:
    # The module name is process-specific, allowing for
    # simultaneous runs of the same test on a single machine.
    test_module = "spamham{}".format(os.getpid())

    def test_find_spec_missing(self):
        spec = self.machinery.WindowsRegistryFinder.find_spec('spam')
        self.assertIs(spec, None)

    def test_find_module_missing(self):
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            loader = self.machinery.WindowsRegistryFinder.find_module('spam')
        self.assertIs(loader, None)

    def test_module_found(self):
        with setup_module(self.machinery, self.test_module):
            loader = self.machinery.WindowsRegistryFinder.find_module(self.test_module)
            spec = self.machinery.WindowsRegistryFinder.find_spec(self.test_module)
            self.assertIsNot(loader, None)
            self.assertIsNot(spec, None)

    def test_module_not_found(self):
        with setup_module(self.machinery, self.test_module, path="."):
            loader = self.machinery.WindowsRegistryFinder.find_module(self.test_module)
            spec = self.machinery.WindowsRegistryFinder.find_spec(self.test_module)
            self.assertIsNone(loader)
            self.assertIsNone(spec)

(Frozen_WindowsRegistryFinderTests,
 Source_WindowsRegistryFinderTests
 ) = test_util.test_both(WindowsRegistryFinderTests, machinery=machinery)

@unittest.skipUnless(sys.platform.startswith('win'), 'requires Windows')
class WindowsExtensionSuffixTests:
    def test_tagged_suffix(self):
        suffixes = self.machinery.EXTENSION_SUFFIXES
        expected_tag = ".cp{0.major}{0.minor}-{1}.pyd".format(sys.version_info,
            re.sub('[^a-zA-Z0-9]', '_', get_platform()))
        try:
            untagged_i = suffixes.index(".pyd")
        except ValueError:
            untagged_i = suffixes.index("_d.pyd")
            expected_tag = "_d" + expected_tag

        self.assertIn(expected_tag, suffixes)

        # Ensure the tags are in the correct order.
        tagged_i = suffixes.index(expected_tag)
        self.assertLess(tagged_i, untagged_i)

(Frozen_WindowsExtensionSuffixTests,
 Source_WindowsExtensionSuffixTests
 ) = test_util.test_both(WindowsExtensionSuffixTests, machinery=machinery)


@unittest.skipUnless(sys.platform.startswith('win'), 'requires Windows')
class WindowsBootstrapPathTests(unittest.TestCase):
    def check_join(self, expected, *inputs):
        from importlib._bootstrap_external import _path_join
        actual = _path_join(*inputs)
        if expected.casefold() == actual.casefold():
            return
        self.assertEqual(expected, actual)

    def test_path_join(self):
        self.check_join(r"C:\A\B", "C:\\", "A", "B")
        self.check_join(r"C:\A\B", "D:\\", "D", "C:\\", "A", "B")
        self.check_join(r"C:\A\B", "C:\\", "A", "C:B")
        self.check_join(r"C:\A\B", "C:\\", "A\\B")
        self.check_join(r"C:\A\B", r"C:\A\B")

        self.check_join("D:A", r"D:", "A")
        self.check_join("D:A", r"C:\B\C", "D:", "A")
        self.check_join("D:A", r"C:\B\C", r"D:A")

        self.check_join(r"A\B\C", "A", "B", "C")
        self.check_join(r"A\B\C", "A", r"B\C")
        self.check_join(r"A\B/C", "A", "B/C")
        self.check_join(r"A\B\C", "A/", "B\\", "C")

        # Dots are not normalised by this function
        self.check_join(r"A\../C", "A", "../C")
        self.check_join(r"A.\.\B", "A.", ".", "B")

        self.check_join(r"\\Server\Share\A\B\C", r"\\Server\Share", "A", "B", "C")
        self.check_join(r"\\Server\Share\A\B\C", r"\\Server\Share", "D", r"\A", "B", "C")
        self.check_join(r"\\Server\Share\A\B\C", r"\\Server2\Share2", "D",
                                                 r"\\Server\Share", "A", "B", "C")
        self.check_join(r"\\Server\Share\A\B\C", r"\\Server", r"\Share", "A", "B", "C")
        self.check_join(r"\\Server\Share", r"\\Server\Share")
        self.check_join(r"\\Server\Share\\", r"\\Server\Share\\")

        # Handle edge cases with empty segments
        self.check_join("C:\\A", "C:/A", "")
        self.check_join("C:\\", "C:/", "")
        self.check_join("C:", "C:", "")
        self.check_join("//Server/Share\\", "//Server/Share/", "")
        self.check_join("//Server/Share\\", "//Server/Share", "")
