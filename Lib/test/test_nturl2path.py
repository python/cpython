import unittest
from nturl2path import url2pathname, pathname2url


class URL2PathNameTests(unittest.TestCase):

    def test_converting_drive_letter(self):
        self.assertEqual(url2pathname("///C|"), 'C:')
        self.assertEqual(url2pathname("///C:"), 'C:')
        self.assertEqual(url2pathname("///C|/"), 'C:\\')

    def test_converting_when_no_drive_letter(self):
        # cannot end a raw string in \
        self.assertEqual(url2pathname("///C/test/"), r'\\\C\test' '\\')
        self.assertEqual(url2pathname("////C/test/"), r'\\C\test' '\\')

    def test_simple_compare(self):
        self.assertEqual(url2pathname("///C|/foo/bar/spam.foo"),
                         r'C:\foo\bar\spam.foo')

    def test_non_ascii_drive_letter(self):
        self.assertRaises(IOError, url2pathname, "///\u00e8|/")

    def test_roundtrip_url2pathname(self):
        list_of_paths = ['C:',
                         r'\\\C\test\\',
                         r'C:\foo\bar\spam.foo'
                         ]
        for path in list_of_paths:
            self.assertEqual(url2pathname(pathname2url(path)), path)

class PathName2URLTests(unittest.TestCase):
    def test_converting_drive_letter(self):
        self.assertEqual(pathname2url("C:"), '///C:')
        self.assertEqual(pathname2url("C:\\"), '///C:')

    def test_converting_when_no_drive_letter(self):
        self.assertEqual(pathname2url(r"\\\folder\test" "\\"),
                         '/////folder/test/')
        self.assertEqual(pathname2url(r"\\folder\test" "\\"),
                         '////folder/test/')
        self.assertEqual(pathname2url(r"\folder\test" "\\"),
                         '/folder/test/')

    def test_simple_compare(self):
        self.assertEqual(pathname2url(r'C:\foo\bar\spam.foo'),
                         "///C:/foo/bar/spam.foo" )

    def test_long_drive_letter(self):
        self.assertRaises(IOError, pathname2url, "XX:\\")

    def test_roundtrip_pathname2url(self):
        list_of_paths = ['///C:',
                         '/////folder/test/',
                         '///C:/foo/bar/spam.foo']
        for path in list_of_paths:
            self.assertEqual(pathname2url(url2pathname(path)), path)


if __name__ == "__main__":
    unittest.main()
