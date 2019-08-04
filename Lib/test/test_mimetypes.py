import io
import locale
import mimetypes
import pathlib
import sys
import unittest

from test import support
from platform import win32_edition

# Tell it we don't know about external files:
mimetypes.knownfiles = []
mimetypes.inited = False
mimetypes._default_mime_types()


class MimeTypesTestCase(unittest.TestCase):
    def setUp(self):
        self.db = mimetypes.MimeTypes()

    def test_default_data(self):
        eq = self.assertEqual
        eq(self.db.guess_type("foo.html"), ("text/html", None))
        eq(self.db.guess_type("foo.tgz"), ("application/x-tar", "gzip"))
        eq(self.db.guess_type("foo.tar.gz"), ("application/x-tar", "gzip"))
        eq(self.db.guess_type("foo.tar.Z"), ("application/x-tar", "compress"))
        eq(self.db.guess_type("foo.tar.bz2"), ("application/x-tar", "bzip2"))
        eq(self.db.guess_type("foo.tar.xz"), ("application/x-tar", "xz"))

    def test_data_urls(self):
        eq = self.assertEqual
        guess_type = self.db.guess_type
        eq(guess_type("data:,thisIsTextPlain"), ("text/plain", None))
        eq(guess_type("data:;base64,thisIsTextPlain"), ("text/plain", None))
        eq(guess_type("data:text/x-foo,thisIsTextXFoo"), ("text/x-foo", None))

    def test_file_parsing(self):
        eq = self.assertEqual
        sio = io.StringIO("x-application/x-unittest pyunit\n")
        self.db.readfp(sio)
        eq(self.db.guess_type("foo.pyunit"),
           ("x-application/x-unittest", None))
        eq(self.db.guess_extension("x-application/x-unittest"), ".pyunit")

    def test_non_standard_types(self):
        eq = self.assertEqual
        # First try strict
        eq(self.db.guess_type('foo.xul', strict=True), (None, None))
        eq(self.db.guess_extension('image/jpg', strict=True), None)
        # And then non-strict
        eq(self.db.guess_type('foo.xul', strict=False), ('text/xul', None))
        eq(self.db.guess_extension('image/jpg', strict=False), '.jpg')

    def test_guess_all_types(self):
        eq = self.assertEqual
        unless = self.assertTrue
        # First try strict.  Use a set here for testing the results because if
        # test_urllib2 is run before test_mimetypes, global state is modified
        # such that the 'all' set will have more items in it.
        all = set(self.db.guess_all_extensions('text/plain', strict=True))
        unless(all >= set(['.bat', '.c', '.h', '.ksh', '.pl', '.txt']))
        # And now non-strict
        all = self.db.guess_all_extensions('image/jpg', strict=False)
        all.sort()
        eq(all, ['.jpg'])
        # And now for no hits
        all = self.db.guess_all_extensions('image/jpg', strict=True)
        eq(all, [])

    def test_encoding(self):
        getpreferredencoding = locale.getpreferredencoding
        self.addCleanup(setattr, locale, 'getpreferredencoding',
                                 getpreferredencoding)
        locale.getpreferredencoding = lambda: 'ascii'

        filename = support.findfile("mime.types")
        mimes = mimetypes.MimeTypes([filename])
        exts = mimes.guess_all_extensions('application/vnd.geocube+xml',
                                          strict=True)
        self.assertEqual(exts, ['.g3', '.g\xb3'])

    def test_init_reinitializes(self):
        # Issue 4936: make sure an init starts clean
        # First, put some poison into the types table
        mimetypes.add_type('foo/bar', '.foobar')
        self.assertEqual(mimetypes.guess_extension('foo/bar'), '.foobar')
        # Reinitialize
        mimetypes.init()
        # Poison should be gone.
        self.assertEqual(mimetypes.guess_extension('foo/bar'), None)

    def test_preferred_extension(self):
        def check_extensions():
            self.assertEqual(mimetypes.guess_extension('application/octet-stream'), '.bin')
            self.assertEqual(mimetypes.guess_extension('application/postscript'), '.ps')
            self.assertEqual(mimetypes.guess_extension('application/vnd.apple.mpegurl'), '.m3u')
            self.assertEqual(mimetypes.guess_extension('application/vnd.ms-excel'), '.xls')
            self.assertEqual(mimetypes.guess_extension('application/vnd.ms-powerpoint'), '.ppt')
            self.assertEqual(mimetypes.guess_extension('application/x-texinfo'), '.texi')
            self.assertEqual(mimetypes.guess_extension('application/x-troff'), '.roff')
            self.assertEqual(mimetypes.guess_extension('application/xml'), '.xsl')
            self.assertEqual(mimetypes.guess_extension('audio/mpeg'), '.mp3')
            self.assertEqual(mimetypes.guess_extension('image/jpeg'), '.jpg')
            self.assertEqual(mimetypes.guess_extension('image/tiff'), '.tiff')
            self.assertEqual(mimetypes.guess_extension('message/rfc822'), '.eml')
            self.assertEqual(mimetypes.guess_extension('text/html'), '.html')
            self.assertEqual(mimetypes.guess_extension('text/plain'), '.txt')
            self.assertEqual(mimetypes.guess_extension('video/mpeg'), '.mpeg')
            self.assertEqual(mimetypes.guess_extension('video/quicktime'), '.mov')

        check_extensions()
        mimetypes.init()
        check_extensions()

    def test_init_stability(self):
        mimetypes.init()

        suffix_map = mimetypes.suffix_map
        encodings_map = mimetypes.encodings_map
        types_map = mimetypes.types_map
        common_types = mimetypes.common_types

        mimetypes.init()
        self.assertIsNot(suffix_map, mimetypes.suffix_map)
        self.assertIsNot(encodings_map, mimetypes.encodings_map)
        self.assertIsNot(types_map, mimetypes.types_map)
        self.assertIsNot(common_types, mimetypes.common_types)
        self.assertEqual(suffix_map, mimetypes.suffix_map)
        self.assertEqual(encodings_map, mimetypes.encodings_map)
        self.assertEqual(types_map, mimetypes.types_map)
        self.assertEqual(common_types, mimetypes.common_types)

    def test_path_like_ob(self):
        filename = "LICENSE.txt"
        filepath = pathlib.Path(filename)
        filepath_with_abs_dir = pathlib.Path('/dir/'+filename)
        filepath_relative = pathlib.Path('../dir/'+filename)
        path_dir = pathlib.Path('./')

        expected = self.db.guess_type(filename)

        self.assertEqual(self.db.guess_type(filepath), expected)
        self.assertEqual(self.db.guess_type(
            filepath_with_abs_dir), expected)
        self.assertEqual(self.db.guess_type(filepath_relative), expected)
        self.assertEqual(self.db.guess_type(path_dir), (None, None))

    def test_keywords_args_api(self):
        self.assertEqual(self.db.guess_type(
            url="foo.html", strict=True), ("text/html", None))
        self.assertEqual(self.db.guess_all_extensions(
            type='image/jpg', strict=True), [])
        self.assertEqual(self.db.guess_extension(
            type='image/jpg', strict=False), '.jpg')


@unittest.skipUnless(sys.platform.startswith("win"), "Windows only")
class Win32MimeTypesTestCase(unittest.TestCase):
    def setUp(self):
        # ensure all entries actually come from the Windows registry
        self.original_types_map = mimetypes.types_map.copy()
        mimetypes.types_map.clear()
        mimetypes.init()
        self.db = mimetypes.MimeTypes()

    def tearDown(self):
        # restore default settings
        mimetypes.types_map.clear()
        mimetypes.types_map.update(self.original_types_map)

    @unittest.skipIf(win32_edition() in ('NanoServer', 'WindowsCoreHeadless', 'IoTEdgeOS'),
                                         "MIME types registry keys unavailable")
    def test_registry_parsing(self):
        # the original, minimum contents of the MIME database in the
        # Windows registry is undocumented AFAIK.
        # Use file types that should *always* exist:
        eq = self.assertEqual
        eq(self.db.guess_type("foo.txt"), ("text/plain", None))
        eq(self.db.guess_type("image.jpg"), ("image/jpeg", None))
        eq(self.db.guess_type("image.png"), ("image/png", None))


class MiscTestCase(unittest.TestCase):
    def test__all__(self):
        support.check__all__(self, mimetypes)


if __name__ == "__main__":
    unittest.main()
