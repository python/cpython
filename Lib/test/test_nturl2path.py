import unittest

from test.support import warnings_helper


nturl2path = warnings_helper.import_deprecated("nturl2path")


class NTURL2PathTest(unittest.TestCase):
    """Test pathname2url() and url2pathname()"""

    def test_basic(self):
        # Make sure simple tests pass
        expected_path = r"parts\of\a\path"
        expected_url = "parts/of/a/path"
        result = nturl2path.pathname2url(expected_path)
        self.assertEqual(expected_url, result,
                         "pathname2url() failed; %s != %s" %
                         (result, expected_url))
        result = nturl2path.url2pathname(expected_url)
        self.assertEqual(expected_path, result,
                         "url2pathame() failed; %s != %s" %
                         (result, expected_path))

    def test_pathname2url(self):
        # Test special prefixes are correctly handled in pathname2url()
        fn = nturl2path.pathname2url
        self.assertEqual(fn('\\\\?\\C:\\dir'), '///C:/dir')
        self.assertEqual(fn('\\\\?\\unc\\server\\share\\dir'), '//server/share/dir')
        self.assertEqual(fn("C:"), '///C:')
        self.assertEqual(fn("C:\\"), '///C:/')
        self.assertEqual(fn('c:\\a\\b.c'), '///c:/a/b.c')
        self.assertEqual(fn('C:\\a\\b.c'), '///C:/a/b.c')
        self.assertEqual(fn('C:\\a\\b.c\\'), '///C:/a/b.c/')
        self.assertEqual(fn('C:\\a\\\\b.c'), '///C:/a//b.c')
        self.assertEqual(fn('C:\\a\\b%#c'), '///C:/a/b%25%23c')
        self.assertEqual(fn('C:\\a\\b\xe9'), '///C:/a/b%C3%A9')
        self.assertEqual(fn('C:\\foo\\bar\\spam.foo'), "///C:/foo/bar/spam.foo")
        # NTFS alternate data streams
        self.assertEqual(fn('C:\\foo:bar'), '///C:/foo%3Abar')
        self.assertEqual(fn('foo:bar'), 'foo%3Abar')
        # No drive letter
        self.assertEqual(fn("\\folder\\test\\"), '///folder/test/')
        self.assertEqual(fn("\\\\folder\\test\\"), '//folder/test/')
        self.assertEqual(fn("\\\\\\folder\\test\\"), '///folder/test/')
        self.assertEqual(fn('\\\\some\\share\\'), '//some/share/')
        self.assertEqual(fn('\\\\some\\share\\a\\b.c'), '//some/share/a/b.c')
        self.assertEqual(fn('\\\\some\\share\\a\\b%#c\xe9'), '//some/share/a/b%25%23c%C3%A9')
        # Alternate path separator
        self.assertEqual(fn('C:/a/b.c'), '///C:/a/b.c')
        self.assertEqual(fn('//some/share/a/b.c'), '//some/share/a/b.c')
        self.assertEqual(fn('//?/C:/dir'), '///C:/dir')
        self.assertEqual(fn('//?/unc/server/share/dir'), '//server/share/dir')
        # Round-tripping
        urls = ['///C:',
                '///folder/test/',
                '///C:/foo/bar/spam.foo']
        for url in urls:
            self.assertEqual(fn(nturl2path.url2pathname(url)), url)

    def test_url2pathname(self):
        fn = nturl2path.url2pathname
        self.assertEqual(fn('/'), '\\')
        self.assertEqual(fn('/C:/'), 'C:\\')
        self.assertEqual(fn("///C|"), 'C:')
        self.assertEqual(fn("///C:"), 'C:')
        self.assertEqual(fn('///C:/'), 'C:\\')
        self.assertEqual(fn('/C|//'), 'C:\\\\')
        self.assertEqual(fn('///C|/path'), 'C:\\path')
        # No DOS drive
        self.assertEqual(fn("///C/test/"), '\\C\\test\\')
        self.assertEqual(fn("////C/test/"), '\\\\C\\test\\')
        # DOS drive paths
        self.assertEqual(fn('c:/path/to/file'), 'c:\\path\\to\\file')
        self.assertEqual(fn('C:/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('C:/path/to/file/'), 'C:\\path\\to\\file\\')
        self.assertEqual(fn('C:/path/to//file'), 'C:\\path\\to\\\\file')
        self.assertEqual(fn('C|/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('/C|/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('///C|/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn("///C|/foo/bar/spam.foo"), 'C:\\foo\\bar\\spam.foo')
        # Colons in URI
        self.assertEqual(fn('///\u00e8|/'), '\u00e8:\\')
        self.assertEqual(fn('//host/share/spam.txt:eggs'), '\\\\host\\share\\spam.txt:eggs')
        self.assertEqual(fn('///c:/spam.txt:eggs'), 'c:\\spam.txt:eggs')
        # UNC paths
        self.assertEqual(fn('//server/path/to/file'), '\\\\server\\path\\to\\file')
        self.assertEqual(fn('////server/path/to/file'), '\\\\server\\path\\to\\file')
        self.assertEqual(fn('/////server/path/to/file'), '\\\\server\\path\\to\\file')
        # Localhost paths
        self.assertEqual(fn('//localhost/C:/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('//localhost/C|/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('//localhost/path/to/file'), '\\path\\to\\file')
        self.assertEqual(fn('//localhost//server/path/to/file'), '\\\\server\\path\\to\\file')
        # Percent-encoded forward slashes are preserved for backwards compatibility
        self.assertEqual(fn('C:/foo%2fbar'), 'C:\\foo/bar')
        self.assertEqual(fn('//server/share/foo%2fbar'), '\\\\server\\share\\foo/bar')
        # Round-tripping
        paths = ['C:',
                 r'\C\test\\',
                 r'C:\foo\bar\spam.foo']
        for path in paths:
            self.assertEqual(fn(nturl2path.pathname2url(path)), path)


if __name__ == '__main__':
    unittest.main()
