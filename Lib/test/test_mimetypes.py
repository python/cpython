import io
import mimetypes
from os import linesep
import pathlib
import sys
import unittest.mock

from test import support
from test.support import os_helper
from test.support.script_helper import run_python_until_end
from platform import win32_edition

try:
    import _winapi
except ImportError:
    _winapi = None


def setUpModule():
    global knownfiles
    knownfiles = mimetypes.knownfiles

    # Tell it we don't know about external files:
    mimetypes.knownfiles = []
    mimetypes.inited = False
    mimetypes._default_mime_types()


def tearDownModule():
    # Restore knownfiles to its initial state
    mimetypes.knownfiles = knownfiles


class MimeTypesTestCase(unittest.TestCase):
    def setUp(self):
        self.db = mimetypes.MimeTypes()

    def test_case_sensitivity(self):
        eq = self.assertEqual
        eq(self.db.guess_type("foobar.HTML"), self.db.guess_type("foobar.html"))
        eq(self.db.guess_type("foobar.TGZ"), self.db.guess_type("foobar.tgz"))
        eq(self.db.guess_type("foobar.tar.Z"), ("application/x-tar", "compress"))
        eq(self.db.guess_type("foobar.tar.z"), (None, None))

    def test_default_data(self):
        eq = self.assertEqual
        eq(self.db.guess_type("foo.html"), ("text/html", None))
        eq(self.db.guess_type("foo.HTML"), ("text/html", None))
        eq(self.db.guess_type("foo.tgz"), ("application/x-tar", "gzip"))
        eq(self.db.guess_type("foo.tar.gz"), ("application/x-tar", "gzip"))
        eq(self.db.guess_type("foo.tar.Z"), ("application/x-tar", "compress"))
        eq(self.db.guess_type("foo.tar.bz2"), ("application/x-tar", "bzip2"))
        eq(self.db.guess_type("foo.tar.xz"), ("application/x-tar", "xz"))

    def test_data_urls(self):
        eq = self.assertEqual
        guess_type = self.db.guess_type
        eq(guess_type("data:invalidDataWithoutComma"), (None, None))
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

    def test_read_mime_types(self):
        eq = self.assertEqual

        # Unreadable file returns None
        self.assertIsNone(mimetypes.read_mime_types("non-existent"))

        with os_helper.temp_dir() as directory:
            data = "x-application/x-unittest pyunit\n"
            file = pathlib.Path(directory, "sample.mimetype")
            file.write_text(data, encoding="utf-8")
            mime_dict = mimetypes.read_mime_types(file)
            eq(mime_dict[".pyunit"], "x-application/x-unittest")

        # bpo-41048: read_mime_types should read the rule file with 'utf-8' encoding.
        # Not with locale encoding. _bootlocale has been imported because io.open(...)
        # uses it.
        data = "application/no-mans-land  Fran\u00E7ais"
        filename = "filename"
        fp = io.StringIO(data)
        with unittest.mock.patch.object(mimetypes, 'open',
                                        return_value=fp) as mock_open:
            mime_dict = mimetypes.read_mime_types(filename)
            mock_open.assert_called_with(filename, encoding='utf-8')
        eq(mime_dict[".Français"], "application/no-mans-land")

    def test_non_standard_types(self):
        eq = self.assertEqual
        # First try strict
        eq(self.db.guess_type('foo.xul', strict=True), (None, None))
        eq(self.db.guess_extension('image/jpg', strict=True), None)
        eq(self.db.guess_extension('image/webp', strict=True), None)
        # And then non-strict
        eq(self.db.guess_type('foo.xul', strict=False), ('text/xul', None))
        eq(self.db.guess_type('foo.XUL', strict=False), ('text/xul', None))
        eq(self.db.guess_type('foo.invalid', strict=False), (None, None))
        eq(self.db.guess_extension('image/jpg', strict=False), '.jpg')
        eq(self.db.guess_extension('image/JPG', strict=False), '.jpg')
        eq(self.db.guess_extension('image/webp', strict=False), '.webp')

    def test_filename_with_url_delimiters(self):
        # bpo-38449: URL delimiters cases should be handled also.
        # They would have different mime types if interpreted as URL as
        # compared to when interpreted as filename because of the semicolon.
        eq = self.assertEqual
        gzip_expected = ('application/x-tar', 'gzip')
        eq(self.db.guess_type(";1.tar.gz"), gzip_expected)
        eq(self.db.guess_type("?1.tar.gz"), gzip_expected)
        eq(self.db.guess_type("#1.tar.gz"), gzip_expected)
        eq(self.db.guess_type("#1#.tar.gz"), gzip_expected)
        eq(self.db.guess_type(";1#.tar.gz"), gzip_expected)
        eq(self.db.guess_type(";&1=123;?.tar.gz"), gzip_expected)
        eq(self.db.guess_type("?k1=v1&k2=v2.tar.gz"), gzip_expected)
        eq(self.db.guess_type(r" \"\`;b&b&c |.tar.gz"), gzip_expected)

    def test_guess_all_types(self):
        # First try strict.  Use a set here for testing the results because if
        # test_urllib2 is run before test_mimetypes, global state is modified
        # such that the 'all' set will have more items in it.
        all = self.db.guess_all_extensions('text/plain', strict=True)
        self.assertTrue(set(all) >= {'.bat', '.c', '.h', '.ksh', '.pl', '.txt'})
        self.assertEqual(len(set(all)), len(all))  # no duplicates
        # And now non-strict
        all = self.db.guess_all_extensions('image/jpg', strict=False)
        self.assertEqual(all, ['.jpg'])
        # And now for no hits
        all = self.db.guess_all_extensions('image/jpg', strict=True)
        self.assertEqual(all, [])
        # And now for type existing in both strict and non-strict mappings.
        self.db.add_type('test-type', '.strict-ext')
        self.db.add_type('test-type', '.non-strict-ext', strict=False)
        all = self.db.guess_all_extensions('test-type', strict=False)
        self.assertEqual(all, ['.strict-ext', '.non-strict-ext'])
        all = self.db.guess_all_extensions('test-type')
        self.assertEqual(all, ['.strict-ext'])
        # Test that changing the result list does not affect the global state
        all.append('.no-such-ext')
        all = self.db.guess_all_extensions('test-type')
        self.assertNotIn('.no-such-ext', all)

    def test_encoding(self):
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

    @unittest.skipIf(sys.platform.startswith("win"), "Non-Windows only")
    def test_guess_known_extensions(self):
        # Issue 37529
        # The test fails on Windows because Windows adds mime types from the Registry
        # and that creates some duplicates.
        from mimetypes import types_map
        for v in types_map.values():
            self.assertIsNotNone(mimetypes.guess_extension(v))

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
            self.assertEqual(mimetypes.guess_extension('image/avif'), '.avif')
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

    @unittest.skipIf(not hasattr(_winapi, "_mimetypes_read_windows_registry"),
                     "read_windows_registry accelerator unavailable")
    def test_registry_accelerator(self):
        from_accel = {}
        from_reg = {}
        _winapi._mimetypes_read_windows_registry(
            lambda v, k: from_accel.setdefault(k, set()).add(v)
        )
        mimetypes.MimeTypes._read_windows_registry(
            lambda v, k: from_reg.setdefault(k, set()).add(v)
        )
        self.assertEqual(list(from_reg), list(from_accel))
        for k in from_reg:
            self.assertEqual(from_reg[k], from_accel[k])


class MiscTestCase(unittest.TestCase):
    def test__all__(self):
        support.check__all__(self, mimetypes)


class MimetypesCliTestCase(unittest.TestCase):

    def mimetypes_cmd(cls, *args, **kwargs):
        result, _ = run_python_until_end('-m', 'mimetypes', *args)
        return result.rc, result.out.decode(), result.err.decode()

    def test_help_option(self):
        retcode, out, err = self.mimetypes_cmd('-h')
        self.assertEqual(retcode, 0)
        self.assertIn('usage: mimetypes.py', out)
        self.assertEqual(err, '')

    def test_invalid_option(self):
        retcode, out, err = self.mimetypes_cmd('--invalid')
        self.assertEqual(retcode, 2)
        self.assertEqual(out, '')
        self.assertIn('usage: mimetypes.py', err)

    def test_guess_extension(self):
        retcode, out, err = self.mimetypes_cmd('-l', '-e', 'image/jpg')
        self.assertEqual(retcode, 0)
        self.assertEqual(out, f'.jpg{linesep}')
        self.assertEqual(err, '')

        retcode, out, err = self.mimetypes_cmd('-e', 'image/jpg')
        self.assertEqual(retcode, 1)
        self.assertEqual(out, '')
        self.assertEqual(err, f'error: unknown type image/jpg{linesep}')

        retcode, out, err = self.mimetypes_cmd('-e', 'image/jpeg')
        self.assertEqual(retcode, 0)
        self.assertEqual(out, f'.jpg{linesep}')
        self.assertEqual(err, '')

    def test_guess_type(self):
        retcode, out, err = self.mimetypes_cmd('-l', 'foo.webp')
        self.assertEqual(retcode, 0)
        self.assertEqual(out, f'type: image/webp encoding: None{linesep}')
        self.assertEqual(err, '')

    @unittest.skipIf(
        sys.platform == 'darwin',
        'macOS lists common_types in mime.types thus making them always known'
    )
    def test_guess_type_conflicting_with_mimetypes(self):
        retcode, out, err = self.mimetypes_cmd('foo.webp')
        self.assertEqual(retcode, 1)
        self.assertEqual(out, '')
        self.assertEqual(err, f'error: unknown extension of foo.webp{linesep}')

if __name__ == "__main__":
    unittest.main()
