"""Tests for 'site'.

Tests assume the initial paths in sys.path once the interpreter has begun
executing have not been removed.

"""
import unittest
from test.test_support import TestSkipped, run_unittest, TESTFN
import __builtin__
import os
import sys
import encodings
# Need to make sure to not import 'site' if someone specified ``-S`` at the
# command-line.  Detect this by just making sure 'site' has not been imported
# already.
if "site" in sys.modules:
    import site
else:
    raise TestSkipped("importation of site.py suppressed")

class HelperFunctionsTests(unittest.TestCase):
    """Tests for helper functions.

    The setting of the encoding (set using sys.setdefaultencoding) used by
    the Unicode implementation is not tested.

    """

    def setUp(self):
        """Save a copy of sys.path"""
        self.sys_path = sys.path[:]

    def tearDown(self):
        """Restore sys.path"""
        sys.path = self.sys_path

    def test_makepath(self):
        # Test makepath() have an absolute path for its first return value
        # and a case-normalized version of the absolute path for its
        # second value.
        path_parts = ("Beginning", "End")
        original_dir = os.path.join(*path_parts)
        abs_dir, norm_dir = site.makepath(*path_parts)
        self.failUnlessEqual(os.path.abspath(original_dir), abs_dir)
        if original_dir == os.path.normcase(original_dir):
            self.failUnlessEqual(abs_dir, norm_dir)
        else:
            self.failUnlessEqual(os.path.normcase(abs_dir), norm_dir)

    def test_init_pathinfo(self):
        dir_set = site._init_pathinfo()
        for entry in [site.makepath(path)[1] for path in sys.path
                        if path and os.path.isdir(path)]:
            self.failUnless(entry in dir_set,
                            "%s from sys.path not found in set returned "
                            "by _init_pathinfo(): %s" % (entry, dir_set))

    def pth_file_tests(self, pth_file):
        """Contain common code for testing results of reading a .pth file"""
        self.failUnless(pth_file.imported in sys.modules,
                "%s not in sys.path" % pth_file.imported)
        self.failUnless(site.makepath(pth_file.good_dir_path)[0] in sys.path)
        self.failUnless(not os.path.exists(pth_file.bad_dir_path))

    def test_addpackage(self):
        # Make sure addpackage() imports if the line starts with 'import',
        # adds directories to sys.path for any line in the file that is not a
        # comment or import that is a valid directory name for where the .pth
        # file resides; invalid directories are not added
        pth_file = PthFile()
        pth_file.cleanup(prep=True)  # to make sure that nothing is
                                      # pre-existing that shouldn't be
        try:
            pth_file.create()
            site.addpackage(pth_file.base_dir, pth_file.filename, set())
            self.pth_file_tests(pth_file)
        finally:
            pth_file.cleanup()

    def test_addsitedir(self):
        # Same tests for test_addpackage since addsitedir() essentially just
        # calls addpackage() for every .pth file in the directory
        pth_file = PthFile()
        pth_file.cleanup(prep=True) # Make sure that nothing is pre-existing
                                    # that is tested for
        try:
            pth_file.create()
            site.addsitedir(pth_file.base_dir, set())
            self.pth_file_tests(pth_file)
        finally:
            pth_file.cleanup()

class PthFile(object):
    """Helper class for handling testing of .pth files"""

    def __init__(self, filename_base=TESTFN, imported="time",
                    good_dirname="__testdir__", bad_dirname="__bad"):
        """Initialize instance variables"""
        self.filename = filename_base + ".pth"
        self.base_dir = os.path.abspath('')
        self.file_path = os.path.join(self.base_dir, self.filename)
        self.imported = imported
        self.good_dirname = good_dirname
        self.bad_dirname = bad_dirname
        self.good_dir_path = os.path.join(self.base_dir, self.good_dirname)
        self.bad_dir_path = os.path.join(self.base_dir, self.bad_dirname)

    def create(self):
        """Create a .pth file with a comment, blank lines, an ``import
        <self.imported>``, a line with self.good_dirname, and a line with
        self.bad_dirname.

        Creation of the directory for self.good_dir_path (based off of
        self.good_dirname) is also performed.

        Make sure to call self.cleanup() to undo anything done by this method.

        """
        FILE = open(self.file_path, 'w')
        try:
            print>>FILE, "#import @bad module name"
            print>>FILE, "\n"
            print>>FILE, "import %s" % self.imported
            print>>FILE, self.good_dirname
            print>>FILE, self.bad_dirname
        finally:
            FILE.close()
        os.mkdir(self.good_dir_path)

    def cleanup(self, prep=False):
        """Make sure that the .pth file is deleted, self.imported is not in
        sys.modules, and that both self.good_dirname and self.bad_dirname are
        not existing directories."""
        if os.path.exists(self.file_path):
            os.remove(self.file_path)
        if prep:
            self.imported_module = sys.modules.get(self.imported)
            if self.imported_module:
                del sys.modules[self.imported]
        else:
            if self.imported_module:
                sys.modules[self.imported] = self.imported_module
        if os.path.exists(self.good_dir_path):
            os.rmdir(self.good_dir_path)
        if os.path.exists(self.bad_dir_path):
            os.rmdir(self.bad_dir_path)

class ImportSideEffectTests(unittest.TestCase):
    """Test side-effects from importing 'site'."""

    def setUp(self):
        """Make a copy of sys.path"""
        self.sys_path = sys.path[:]

    def tearDown(self):
        """Restore sys.path"""
        sys.path = self.sys_path

    def test_abs__file__(self):
        # Make sure all imported modules have their __file__ attribute
        # as an absolute path.
        # Handled by abs__file__()
        site.abs__file__()
        for module in (sys, os, __builtin__):
            try:
                self.failUnless(os.path.isabs(module.__file__), `module`)
            except AttributeError:
                continue
        # We could try everything in sys.modules; however, when regrtest.py
        # runs something like test_frozen before test_site, then we will
        # be testing things loaded *after* test_site did path normalization

    def test_no_duplicate_paths(self):
        # No duplicate paths should exist in sys.path
        # Handled by removeduppaths()
        site.removeduppaths()
        seen_paths = set()
        for path in sys.path:
            self.failUnless(path not in seen_paths)
            seen_paths.add(path)

    def test_add_build_dir(self):
        # Test that the build directory's Modules directory is used when it
        # should be.
        # XXX: implement
        pass

    def test_setting_quit(self):
        # 'quit' and 'exit' should be injected into __builtin__
        self.failUnless(hasattr(__builtin__, "quit"))
        self.failUnless(hasattr(__builtin__, "exit"))

    def test_setting_copyright(self):
        # 'copyright' and 'credits' should be in __builtin__
        self.failUnless(hasattr(__builtin__, "copyright"))
        self.failUnless(hasattr(__builtin__, "credits"))

    def test_setting_help(self):
        # 'help' should be set in __builtin__
        self.failUnless(hasattr(__builtin__, "help"))

    def test_aliasing_mbcs(self):
        if sys.platform == "win32":
            import locale
            if locale.getdefaultlocale()[1].startswith('cp'):
                for value in encodings.aliases.aliases.itervalues():
                    if value == "mbcs":
                        break
                else:
                    self.fail("did not alias mbcs")

    def test_setdefaultencoding_removed(self):
        # Make sure sys.setdefaultencoding is gone
        self.failUnless(not hasattr(sys, "setdefaultencoding"))

    def test_sitecustomize_executed(self):
        # If sitecustomize is available, it should have been imported.
        if not sys.modules.has_key("sitecustomize"):
            try:
                import sitecustomize
            except ImportError:
                pass
            else:
                self.fail("sitecustomize not imported automatically")




def test_main():
    run_unittest(HelperFunctionsTests, ImportSideEffectTests)



if __name__ == "__main__":
    test_main()
