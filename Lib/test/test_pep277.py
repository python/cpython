# -*- coding: utf-8 -*-
# Test the Unicode versions of normal file functions
# open, os.open, os.stat. os.listdir, os.rename, os.remove, os.mkdir, os.chdir, os.rmdir
import os, unittest
from test.test_support import TESTFN, TestSkipped, TestFailed, run_suite
try:
    from nt import _getfullpathname
except ImportError:
    raise TestSkipped, "test works only on NT"

filenames = [
    "abc",
    unicode("ascii","utf-8"),
    unicode("Grüß-Gott","utf-8"),
    unicode("Γειά-σας","utf-8"),
    unicode("Здравствуйте","utf-8"),
    unicode("にぽん","utf-8"),
    unicode("השקצץס","utf-8"),
    unicode("曨曩曫","utf-8"),
    unicode("曨שんдΓß","utf-8"),
    ]

class UnicodeFileTests(unittest.TestCase):
    def setUp(self):
        self.files = [os.path.join(TESTFN, f) for f in filenames]
        try:
            os.mkdir(TESTFN)
        except OSError:
            pass
        for name in self.files:
            f = open(name, 'w')
            f.write((name+'\n').encode("utf-8"))
            f.close()
            os.stat(name)

    def tearDown(self):
        for name in self.files:
            os.unlink(name)
        os.rmdir(TESTFN)

    def _apply_failure(self, fn, filename, expected_exception,
                       check_fn_in_exception = True):
        try:
            fn(filename)
            raise TestFailed("Expected to fail calling '%s(%r)'"
                             % (fn.__name__, filename))
        except expected_exception, details:
            if check_fn_in_exception and details.filename != filename:
                    raise TestFailed("Function '%s(%r) failed with "
                                     "bad filename in the exception: %r"
                                     % (fn.__name__, filename,
                                        details.filename))

    def test_failures(self):
        # Pass non-existing Unicode filenames all over the place.
        for name in self.files:
            name = "not_" + name
            self._apply_failure(open, name, IOError)
            self._apply_failure(os.stat, name, OSError)
            self._apply_failure(os.chdir, name, OSError)
            self._apply_failure(os.rmdir, name, OSError)
            self._apply_failure(os.remove, name, OSError)
            # listdir may append a wildcard to the filename, so dont check
            self._apply_failure(os.listdir, name, OSError, False)

    def test_open(self):
        for name in self.files:
            f = open(name, 'w')
            f.write((name+'\n').encode("utf-8"))
            f.close()
            os.stat(name)

    def test_listdir(self):
        f1 = os.listdir(TESTFN)
        f1.sort()
        f2 = os.listdir(unicode(TESTFN,"mbcs"))
        f2.sort()
        print f1
        print f2

    def test_rename(self):
        for name in self.files:
            os.rename(name,"tmp")
            os.rename("tmp",name)

    def test_directory(self):
        dirname = unicode(os.path.join(TESTFN,"Grüß-曨曩曫"),"utf-8")
        filename = unicode("ß-曨曩曫","utf-8")
        oldwd = os.getcwd()
        os.mkdir(dirname)
        os.chdir(dirname)
        f = open(filename, 'w')
        f.write((filename + '\n').encode("utf-8"))
        f.close()
        print repr(_getfullpathname(filename))
        os.remove(filename)
        os.chdir(oldwd)
        os.rmdir(dirname)

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(UnicodeFileTests))
    run_suite(suite)

if __name__ == "__main__":
    test_main()
