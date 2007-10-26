# Test the Unicode versions of normal file functions
# open, os.open, os.stat. os.listdir, os.rename, os.remove, os.mkdir, os.chdir, os.rmdir
import sys, os, unittest
from test import test_support
if not os.path.supports_unicode_filenames:
    raise test_support.TestSkipped("test works only on NT+")

filenames = [
    'abc',
    'ascii',
    'Gr\xfc\xdf-Gott',
    '\u0393\u03b5\u03b9\u03ac-\u03c3\u03b1\u03c2',
    '\u0417\u0434\u0440\u0430\u0432\u0441\u0442\u0432\u0443\u0439\u0442\u0435',
    '\u306b\u307d\u3093',
    '\u05d4\u05e9\u05e7\u05e6\u05e5\u05e1',
    '\u66e8\u66e9\u66eb',
    '\u66e8\u05e9\u3093\u0434\u0393\xdf',
    ]

# Destroy directory dirname and all files under it, to one level.
def deltree(dirname):
    # Don't hide legitimate errors:  if one of these suckers exists, it's
    # an error if we can't remove it.
    if os.path.exists(dirname):
        # must pass unicode to os.listdir() so we get back unicode results.
        for fname in os.listdir(str(dirname)):
            os.unlink(os.path.join(dirname, fname))
        os.rmdir(dirname)

class UnicodeFileTests(unittest.TestCase):
    files = [os.path.join(test_support.TESTFN, f) for f in filenames]

    def setUp(self):
        try:
            os.mkdir(test_support.TESTFN)
        except OSError:
            pass
        for name in self.files:
            f = open(name, 'wb')
            f.write((name+'\n').encode("utf-8"))
            f.close()
            os.stat(name)

    def tearDown(self):
        deltree(test_support.TESTFN)

    def _apply_failure(self, fn, filename, expected_exception,
                       check_fn_in_exception = True):
        try:
            fn(filename)
            raise test_support.TestFailed("Expected to fail calling '%s(%r)'"
                             % (fn.__name__, filename))
        except expected_exception as details:
            if check_fn_in_exception and details.filename != filename:
                raise test_support.TestFailed("Function '%s(%r) failed with "
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
            f = open(name, 'wb')
            f.write((name+'\n').encode("utf-8"))
            f.close()
            os.stat(name)

    def test_listdir(self):
        f1 = os.listdir(test_support.TESTFN)
        # Printing f1 is not appropriate, as specific filenames
        # returned depend on the local encoding
        f2 = os.listdir(str(test_support.TESTFN.encode("utf-8"),
                                sys.getfilesystemencoding()))
        f2.sort()
        print(f2)

    def test_rename(self):
        for name in self.files:
            os.rename(name,"tmp")
            os.rename("tmp",name)

    def test_directory(self):
        dirname = os.path.join(test_support.TESTFN,'Gr\xfc\xdf-\u66e8\u66e9\u66eb')
        filename = '\xdf-\u66e8\u66e9\u66eb'
        oldwd = os.getcwd()
        os.mkdir(dirname)
        os.chdir(dirname)
        f = open(filename, 'wb')
        f.write((filename + '\n').encode("utf-8"))
        f.close()
        print(repr(filename))
        os.access(filename,os.R_OK)
        os.remove(filename)
        os.chdir(oldwd)
        os.rmdir(dirname)

def test_main():
    try:
        test_support.run_unittest(UnicodeFileTests)
    finally:
        deltree(test_support.TESTFN)

if __name__ == "__main__":
    test_main()
