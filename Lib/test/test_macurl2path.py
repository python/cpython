import macurl2path
import unittest

class MacUrl2PathTestCase(unittest.TestCase):
    def test_url2pathname(self):
        self.assertEqual(":index.html", macurl2path.url2pathname("index.html"))
        self.assertEqual(":bar:index.html", macurl2path.url2pathname("bar/index.html"))
        self.assertEqual("foo:bar:index.html", macurl2path.url2pathname("/foo/bar/index.html"))
        self.assertEqual("foo:bar", macurl2path.url2pathname("/foo/bar/"))
        self.assertEqual("", macurl2path.url2pathname("/"))
        self.assertRaises(RuntimeError, macurl2path.url2pathname, "http://foo.com")
        self.assertEqual("index.html", macurl2path.url2pathname("///index.html"))
        self.assertRaises(RuntimeError, macurl2path.url2pathname, "//index.html")
        self.assertEqual(":index.html", macurl2path.url2pathname("./index.html"))
        self.assertEqual(":index.html", macurl2path.url2pathname("foo/../index.html"))
        self.assertEqual("::index.html", macurl2path.url2pathname("../index.html"))

    def test_pathname2url(self):
        self.assertEqual("drive", macurl2path.pathname2url("drive:"))
        self.assertEqual("drive/dir", macurl2path.pathname2url("drive:dir:"))
        self.assertEqual("drive/dir/file", macurl2path.pathname2url("drive:dir:file"))
        self.assertEqual("drive/file", macurl2path.pathname2url("drive:file"))
        self.assertEqual("file", macurl2path.pathname2url("file"))
        self.assertEqual("file", macurl2path.pathname2url(":file"))
        self.assertEqual("dir", macurl2path.pathname2url(":dir:"))
        self.assertEqual("dir/file", macurl2path.pathname2url(":dir:file"))
        self.assertRaises(RuntimeError, macurl2path.pathname2url, "/")
        self.assertEqual("dir/../file", macurl2path.pathname2url("dir::file"))

if __name__ == "__main__":
    unittest.main()
