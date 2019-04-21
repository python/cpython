# Test the Unicode versions of normal file functions
# open, os.open, os.stat. os.listdir, os.rename, os.remove, os.mkdir, os.chdir, os.rmdir
import os
import sys
import unittest
import warnings
from unicodedata import normalize
from test import support

filenames = [
    '1_abc',
    '2_ascii',
    '3_Gr\xfc\xdf-Gott',
    '4_\u0393\u03b5\u03b9\u03ac-\u03c3\u03b1\u03c2',
    '5_\u0417\u0434\u0440\u0430\u0432\u0441\u0442\u0432\u0443\u0439\u0442\u0435',
    '6_\u306b\u307d\u3093',
    '7_\u05d4\u05e9\u05e7\u05e6\u05e5\u05e1',
    '8_\u66e8\u66e9\u66eb',
    '9_\u66e8\u05e9\u3093\u0434\u0393\xdf',
    # Specific code points: fn, NFC(fn) and NFKC(fn) all different
    '10_\u1fee\u1ffd',
    ]

# Mac OS X decomposes Unicode names, using Normal Form D.
# http://developer.apple.com/mac/library/qa/qa2001/qa1173.html
# "However, most volume formats do not follow the exact specification for
# these normal forms.  For example, HFS Plus uses a variant of Normal Form D
# in which U+2000 through U+2FFF, U+F900 through U+FAFF, and U+2F800 through
# U+2FAFF are not decomposed."
if sys.platform != 'darwin':
    filenames.extend([
        # Specific code points: NFC(fn), NFD(fn), NFKC(fn) and NFKD(fn) all different
        '11_\u0385\u03d3\u03d4',
        '12_\u00a8\u0301\u03d2\u0301\u03d2\u0308', # == NFD('\u0385\u03d3\u03d4')
        '13_\u0020\u0308\u0301\u038e\u03ab',       # == NFKC('\u0385\u03d3\u03d4')
        '14_\u1e9b\u1fc1\u1fcd\u1fce\u1fcf\u1fdd\u1fde\u1fdf\u1fed',

        # Specific code points: fn, NFC(fn) and NFKC(fn) all different
        '15_\u1fee\u1ffd\ufad1',
        '16_\u2000\u2000\u2000A',
        '17_\u2001\u2001\u2001A',
        '18_\u2003\u2003\u2003A',  # == NFC('\u2001\u2001\u2001A')
        '19_\u0020\u0020\u0020A',  # '\u0020' == ' ' == NFKC('\u2000') ==
                                   #  NFKC('\u2001') == NFKC('\u2003')
    ])


# Is it Unicode-friendly?
if not os.path.supports_unicode_filenames:
    fsencoding = sys.getfilesystemencoding()
    try:
        for name in filenames:
            name.encode(fsencoding)
    except UnicodeEncodeError:
        raise unittest.SkipTest("only NT+ and systems with "
                                "Unicode-friendly filesystem encoding")


class UnicodeFileTests(unittest.TestCase):
    files = set(filenames)
    normal_form = None

    def setUp(self):
        try:
            os.mkdir(support.TESTFN)
        except FileExistsError:
            pass
        self.addCleanup(support.rmtree, support.TESTFN)

        files = set()
        for name in self.files:
            name = os.path.join(support.TESTFN, self.norm(name))
            with open(name, 'wb') as f:
                f.write((name+'\n').encode("utf-8"))
            os.stat(name)
            files.add(name)
        self.files = files

    def norm(self, s):
        if self.normal_form:
            return normalize(self.normal_form, s)
        return s

    def _apply_failure(self, fn, filename,
                       expected_exception=FileNotFoundError,
                       check_filename=True):
        with self.assertRaises(expected_exception) as c:
            fn(filename)
        exc_filename = c.exception.filename
        if check_filename:
            self.assertEqual(exc_filename, filename, "Function '%s(%a) failed "
                             "with bad filename in the exception: %a" %
                             (fn.__name__, filename, exc_filename))

    def test_failures(self):
        # Pass non-existing Unicode filenames all over the place.
        for name in self.files:
            name = "not_" + name
            self._apply_failure(open, name)
            self._apply_failure(os.stat, name)
            self._apply_failure(os.chdir, name)
            self._apply_failure(os.rmdir, name)
            self._apply_failure(os.remove, name)
            self._apply_failure(os.listdir, name)

    if sys.platform == 'win32':
        # Windows is lunatic. Issue #13366.
        _listdir_failure = NotADirectoryError, FileNotFoundError
    else:
        _listdir_failure = NotADirectoryError

    def test_open(self):
        for name in self.files:
            f = open(name, 'wb')
            f.write((name+'\n').encode("utf-8"))
            f.close()
            os.stat(name)
            self._apply_failure(os.listdir, name, self._listdir_failure)

    # Skip the test on darwin, because darwin does normalize the filename to
    # NFD (a variant of Unicode NFD form). Normalize the filename to NFC, NFKC,
    # NFKD in Python is useless, because darwin will normalize it later and so
    # open(), os.stat(), etc. don't raise any exception.
    @unittest.skipIf(sys.platform == 'darwin', 'irrelevant test on Mac OS X')
    def test_normalize(self):
        files = set(self.files)
        others = set()
        for nf in set(['NFC', 'NFD', 'NFKC', 'NFKD']):
            others |= set(normalize(nf, file) for file in files)
        others -= files
        for name in others:
            self._apply_failure(open, name)
            self._apply_failure(os.stat, name)
            self._apply_failure(os.chdir, name)
            self._apply_failure(os.rmdir, name)
            self._apply_failure(os.remove, name)
            self._apply_failure(os.listdir, name)

    # Skip the test on darwin, because darwin uses a normalization different
    # than Python NFD normalization: filenames are different even if we use
    # Python NFD normalization.
    @unittest.skipIf(sys.platform == 'darwin', 'irrelevant test on Mac OS X')
    def test_listdir(self):
        sf0 = set(self.files)
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            f1 = os.listdir(support.TESTFN.encode(sys.getfilesystemencoding()))
        f2 = os.listdir(support.TESTFN)
        sf2 = set(os.path.join(support.TESTFN, f) for f in f2)
        self.assertEqual(sf0, sf2, "%a != %a" % (sf0, sf2))
        self.assertEqual(len(f1), len(f2))

    def test_rename(self):
        for name in self.files:
            os.rename(name, "tmp")
            os.rename("tmp", name)

    def test_directory(self):
        dirname = os.path.join(support.TESTFN, 'Gr\xfc\xdf-\u66e8\u66e9\u66eb')
        filename = '\xdf-\u66e8\u66e9\u66eb'
        with support.temp_cwd(dirname):
            with open(filename, 'wb') as f:
                f.write((filename + '\n').encode("utf-8"))
            os.access(filename,os.R_OK)
            os.remove(filename)


class UnicodeNFCFileTests(UnicodeFileTests):
    normal_form = 'NFC'


class UnicodeNFDFileTests(UnicodeFileTests):
    normal_form = 'NFD'


class UnicodeNFKCFileTests(UnicodeFileTests):
    normal_form = 'NFKC'


class UnicodeNFKDFileTests(UnicodeFileTests):
    normal_form = 'NFKD'


def test_main():
    support.run_unittest(
        UnicodeFileTests,
        UnicodeNFCFileTests,
        UnicodeNFDFileTests,
        UnicodeNFKCFileTests,
        UnicodeNFKDFileTests,
    )


if __name__ == "__main__":
    test_main()
