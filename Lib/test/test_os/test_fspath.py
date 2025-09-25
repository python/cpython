import unittest
import os
import types
from test.support.os_helper import FakePath


class TestPEP519(unittest.TestCase):

    # Abstracted so it can be overridden to test pure Python implementation
    # if a C version is provided.
    fspath = staticmethod(os.fspath)

    def test_return_bytes(self):
        for b in b'hello', b'goodbye', b'some/path/and/file':
            self.assertEqual(b, self.fspath(b))

    def test_return_string(self):
        for s in 'hello', 'goodbye', 'some/path/and/file':
            self.assertEqual(s, self.fspath(s))

    def test_fsencode_fsdecode(self):
        for p in "path/like/object", b"path/like/object":
            pathlike = FakePath(p)

            self.assertEqual(p, self.fspath(pathlike))
            self.assertEqual(b"path/like/object", os.fsencode(pathlike))
            self.assertEqual("path/like/object", os.fsdecode(pathlike))

    def test_pathlike(self):
        self.assertEqual('#feelthegil', self.fspath(FakePath('#feelthegil')))
        self.assertIsSubclass(FakePath, os.PathLike)
        self.assertIsInstance(FakePath('x'), os.PathLike)

    def test_garbage_in_exception_out(self):
        vapor = type('blah', (), {})
        for o in int, type, os, vapor():
            self.assertRaises(TypeError, self.fspath, o)

    def test_argument_required(self):
        self.assertRaises(TypeError, self.fspath)

    def test_bad_pathlike(self):
        # __fspath__ returns a value other than str or bytes.
        self.assertRaises(TypeError, self.fspath, FakePath(42))
        # __fspath__ attribute that is not callable.
        c = type('foo', (), {})
        c.__fspath__ = 1
        self.assertRaises(TypeError, self.fspath, c())
        # __fspath__ raises an exception.
        self.assertRaises(ZeroDivisionError, self.fspath,
                          FakePath(ZeroDivisionError()))

    def test_pathlike_subclasshook(self):
        # bpo-38878: subclasshook causes subclass checks
        # true on abstract implementation.
        class A(os.PathLike):
            pass
        self.assertNotIsSubclass(FakePath, A)
        self.assertIsSubclass(FakePath, os.PathLike)

    def test_pathlike_class_getitem(self):
        self.assertIsInstance(os.PathLike[bytes], types.GenericAlias)

    def test_pathlike_subclass_slots(self):
        class A(os.PathLike):
            __slots__ = ()
            def __fspath__(self):
                return ''
        self.assertNotHasAttr(A(), '__dict__')

    def test_fspath_set_to_None(self):
        class Foo:
            __fspath__ = None

        class Bar:
            def __fspath__(self):
                return 'bar'

        class Baz(Bar):
            __fspath__ = None

        good_error_msg = (
            r"expected str, bytes or os.PathLike object, not {}".format
        )

        with self.assertRaisesRegex(TypeError, good_error_msg("Foo")):
            self.fspath(Foo())

        self.assertEqual(self.fspath(Bar()), 'bar')

        with self.assertRaisesRegex(TypeError, good_error_msg("Baz")):
            self.fspath(Baz())

        with self.assertRaisesRegex(TypeError, good_error_msg("Foo")):
            open(Foo())

        with self.assertRaisesRegex(TypeError, good_error_msg("Baz")):
            open(Baz())

        other_good_error_msg = (
            r"should be string, bytes or os.PathLike, not {}".format
        )

        with self.assertRaisesRegex(TypeError, other_good_error_msg("Foo")):
            os.rename(Foo(), "foooo")

        with self.assertRaisesRegex(TypeError, other_good_error_msg("Baz")):
            os.rename(Baz(), "bazzz")

# Only test if the C version is provided, otherwise TestPEP519 already tested
# the pure Python implementation.
if hasattr(os, "_fspath"):
    class TestPEP519PurePython(TestPEP519):

        """Explicitly test the pure Python implementation of os.fspath()."""

        fspath = staticmethod(os._fspath)


if __name__ == "__main__":
    unittest.main()
