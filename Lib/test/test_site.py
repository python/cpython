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
import tempfile
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
    
    def test_addpackage(self):
        # Make sure addpackage() imports if the line starts with 'import',
        # otherwise add a directory combined from sitedir and 'name'.
        # Must also skip comment lines.
        dir_path, file_name, new_dir  = createpth()
        try:
            site.addpackage(dir_path, file_name, set())
            self.failUnless(site.makepath(os.path.join(dir_path, new_dir))[0] in
                    sys.path)
        finally:
            cleanuppth(dir_path, file_name, new_dir)
    
    def test_addsitedir(self):
        dir_path, file_name, new_dir = createpth()
        try:
            site.addsitedir(dir_path, set())
            self.failUnless(site.makepath(os.path.join(dir_path, new_dir))[0] in
            sys.path)
        finally:
            cleanuppth(dir_path, file_name, new_dir)

def createpth():
    """Create a temporary .pth file at the returned location and return the
    directory where it was created, the pth file name, and the directory
    specified in the pth file.

    Make sure to delete the file when finished.

    """
    pth_dirname = "__testdir__"
    file_name = TESTFN + ".pth"
    full_dirname = os.path.dirname(os.path.abspath(file_name))
    FILE = file(os.path.join(full_dirname, file_name), 'w')
    try:
        print>>FILE, "#import @bad module name"
        print>>FILE, ''
        print>>FILE, "import os"
        print>>FILE, pth_dirname
    finally:
        FILE.close()
    os.mkdir(os.path.join(full_dirname, pth_dirname))
    return full_dirname, file_name, pth_dirname

def cleanuppth(full_dirname, file_name, pth_dirname):
    """Clean up what createpth() made"""
    os.remove(os.path.join(full_dirname, file_name))
    os.rmdir(os.path.join(full_dirname, pth_dirname))

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
        for module in sys.modules.values():
            try:
                self.failUnless(os.path.isabs(module.__file__))
            except AttributeError:
                continue

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

    def test_sitepackages(self):
        # There should be a path that ends in site-packages
        for path in sys.path:
            if path.endswith("site-packages"):
                break
        else:
            self.fail("'site-packages' directory missing'")

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
