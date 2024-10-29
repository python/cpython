import nturl2path
import unittest
import urllib.parse


class nturl2path_Tests(unittest.TestCase):
    """Test pathname2url() and url2pathname()"""

    def test_basic(self):
        # Make sure simple tests pass
        expected_path = "parts\\of\\a\\path"
        expected_url = "parts/of/a/path"
        result = nturl2path.pathname2url(expected_path)
        self.assertEqual(expected_url, result,
                         "pathname2url() failed; %s != %s" %
                         (result, expected_url))
        result = nturl2path.url2pathname(expected_url)
        self.assertEqual(expected_path, result,
                         "url2pathame() failed; %s != %s" %
                         (result, expected_path))

    def test_quoting(self):
        # Test automatic quoting and unquoting works for pathnam2url() and
        # url2pathname() respectively
        given = "needs\\quot=ing\\here"
        expect = "needs/%s/here" % urllib.parse.quote("quot=ing")
        result = nturl2path.pathname2url(given)
        self.assertEqual(expect, result,
                         "pathname2url() failed; %s != %s" %
                         (expect, result))
        expect = given
        result = nturl2path.url2pathname(result)
        self.assertEqual(expect, result,
                         "url2pathname() failed; %s != %s" %
                         (expect, result))
        given = "make sure\\using_quote"
        expect = "%s/using_quote" % urllib.parse.quote("make sure")
        result = nturl2path.pathname2url(given)
        self.assertEqual(expect, result,
                         "pathname2url() failed; %s != %s" %
                         (expect, result))
        given = "make+sure/using_unquote"
        expect = "make+sure\\using_unquote"
        result = nturl2path.url2pathname(given)
        self.assertEqual(expect, result,
                         "url2pathname() failed; %s != %s" %
                         (expect, result))

    def test_pathname2url(self):
        # Test special prefixes are correctly handled in pathname2url()
        fn = nturl2path.pathname2url
        self.assertEqual(fn('\\\\?\\C:\\dir'), '///C:/dir')
        self.assertEqual(fn('\\\\?\\unc\\server\\share\\dir'), '/server/share/dir')
        self.assertEqual(fn("C:"), '///C:')
        self.assertEqual(fn("C:\\"), '///C:')
        self.assertEqual(fn('C:\\a\\b.c'), '///C:/a/b.c')
        self.assertEqual(fn('C:\\a\\b%#c'), '///C:/a/b%25%23c')
        self.assertEqual(fn('C:\\a\\b\xe9'), '///C:/a/b%C3%A9')
        self.assertEqual(fn('C:\\foo\\bar\\spam.foo'), "///C:/foo/bar/spam.foo")
        # Long drive letter
        self.assertRaises(IOError, fn, "XX:\\")
        # No drive letter
        self.assertEqual(fn("\\folder\\test\\"), '/folder/test/')
        self.assertEqual(fn("\\\\folder\\test\\"), '////folder/test/')
        self.assertEqual(fn("\\\\\\folder\\test\\"), '/////folder/test/')
        self.assertEqual(fn('\\\\some\\share\\'), '////some/share/')
        self.assertEqual(fn('\\\\some\\share\\a\\b.c'), '////some/share/a/b.c')
        self.assertEqual(fn('\\\\some\\share\\a\\b%#c\xe9'), '////some/share/a/b%25%23c%C3%A9')
        # Round-tripping
        urls = ['///C:',
                '/////folder/test/',
                '///C:/foo/bar/spam.foo']
        for url in urls:
            self.assertEqual(fn(nturl2path.url2pathname(url)), url)

    def test_url2pathname_win(self):
        fn = nturl2path.url2pathname
        self.assertEqual(fn('/C:/'), 'C:\\')
        self.assertEqual(fn("///C|"), 'C:')
        self.assertEqual(fn("///C:"), 'C:')
        self.assertEqual(fn('///C:/'), 'C:\\')
        self.assertEqual(fn('/C|//'), 'C:\\')
        self.assertEqual(fn('///C|/path'), 'C:\\path')
        # No DOS drive
        self.assertEqual(fn("///C/test/"), '\\\\\\C\\test\\')
        self.assertEqual(fn("////C/test/"), '\\\\C\\test\\')
        # DOS drive paths
        self.assertEqual(fn('C:/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('C|/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('/C|/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('///C|/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn("///C|/foo/bar/spam.foo"), 'C:\\foo\\bar\\spam.foo')
        # Non-ASCII drive letter
        self.assertRaises(IOError, fn, "///\u00e8|/")
        # UNC paths
        self.assertEqual(fn('//server/path/to/file'), '\\\\server\\path\\to\\file')
        self.assertEqual(fn('////server/path/to/file'), '\\\\server\\path\\to\\file')
        self.assertEqual(fn('/////server/path/to/file'), '\\\\\\server\\path\\to\\file')
        # Localhost paths
        self.assertEqual(fn('//localhost/C:/path/to/file'), 'C:\\path\\to\\file')
        self.assertEqual(fn('//localhost/C|/path/to/file'), 'C:\\path\\to\\file')
        # Round-tripping
        paths = ['C:',
                 r'\\\C\test\\',
                 r'C:\foo\bar\spam.foo']
        for path in paths:
            self.assertEqual(fn(nturl2path.pathname2url(path)), path)


if __name__ == '__main__':
    unittest.main()
