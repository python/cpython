import mimetypes
import StringIO
import unittest

import test_support

# Tell it we don't know about external files:
mimetypes.knownfiles = []


class MimeTypesTestCase(unittest.TestCase):
    def setUp(self):
        self.db = mimetypes.MimeTypes()

    def test_default_data(self):
        self.assertEqual(self.db.guess_type("foo.html"),
                         ("text/html", None))
        self.assertEqual(self.db.guess_type("foo.tgz"),
                         ("application/x-tar", "gzip"))
        self.assertEqual(self.db.guess_type("foo.tar.gz"),
                         ("application/x-tar", "gzip"))
        self.assertEqual(self.db.guess_type("foo.tar.Z"),
                         ("application/x-tar", "compress"))

    def test_data_urls(self):
        self.assertEqual(self.db.guess_type("data:,thisIsTextPlain"),
                         ("text/plain", None))
        self.assertEqual(self.db.guess_type("data:;base64,thisIsTextPlain"),
                         ("text/plain", None))
        self.assertEqual(self.db.guess_type("data:text/x-foo,thisIsTextXFoo"),
                         ("text/x-foo", None))

    def test_file_parsing(self):
        sio = StringIO.StringIO("x-application/x-unittest pyunit\n")
        self.db.readfp(sio)
        self.assertEqual(self.db.guess_type("foo.pyunit"),
                         ("x-application/x-unittest", None))
        self.assertEqual(self.db.guess_extension("x-application/x-unittest"),
                         ".pyunit")

    def test_non_standard_types(self):
        # First try strict
        self.assertEqual(self.db.guess_type('foo.xul', strict=1),
                         (None, None))
        self.assertEqual(self.db.guess_extension('image/jpg', strict=1),
                         None)
        # And then non-strict
        self.assertEqual(self.db.guess_type('foo.xul', strict=0),
                         ('text/xul', None))
        self.assertEqual(self.db.guess_extension('image/jpg', strict=0),
                         '.jpg')


def test_main():
    test_support.run_unittest(MimeTypesTestCase)


if __name__ == "__main__":
    test_main()
