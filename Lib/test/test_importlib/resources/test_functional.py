import unittest
import os

from test.support.warnings_helper import ignore_warnings, check_warnings

import importlib.resources

# Since the functional API forwards to Traversable, we only test
# filesystem resources here -- not zip files, namespace packages etc.
# We do test for two kinds of Anchor, though.


class StringAnchorMixin:
    anchor01 = 'test.test_importlib.resources.data01'
    anchor02 = 'test.test_importlib.resources.data02'


class ModuleAnchorMixin:
    from test.test_importlib.resources import data01 as anchor01
    from test.test_importlib.resources import data02 as anchor02


class FunctionalAPIBase():
    def test_read_text(self):
        self.assertEqual(
            importlib.resources.read_text(self.anchor01, 'utf-8.file'),
            'Hello, UTF-8 world!\n',
        )
        self.assertEqual(
            importlib.resources.read_text(
                self.anchor02, 'subdirectory', 'subsubdir', 'resource.txt',
                encoding='utf-8',
            ),
            'a resource',
        )
        # Use generic OSError, since e.g. attempting to read a directory can
        # fail with PermissionError rather than IsADirectoryError
        with self.assertRaises(OSError):
            importlib.resources.read_text(self.anchor01)
        with self.assertRaises(OSError):
            importlib.resources.read_text(self.anchor01, 'no-such-file')
        with self.assertRaises(UnicodeDecodeError):
            importlib.resources.read_text(self.anchor01, 'utf-16.file')
        self.assertEqual(
            importlib.resources.read_text(
                self.anchor01, 'binary.file', encoding='latin1',
            ),
            '\x00\x01\x02\x03',
        )
        self.assertEqual(
            importlib.resources.read_text(
                self.anchor01, 'utf-16.file',
                errors='backslashreplace',
            ),
            'Hello, UTF-16 world!\n'.encode('utf-16').decode(
                errors='backslashreplace',
            )
        )

    def test_read_binary(self):
        self.assertEqual(
            importlib.resources.read_binary(self.anchor01, 'utf-8.file'),
            b'Hello, UTF-8 world!\n',
        )
        self.assertEqual(
            importlib.resources.read_binary(
                self.anchor02, 'subdirectory', 'subsubdir', 'resource.txt',
            ),
            b'a resource',
        )

    def test_open_text(self):
        with importlib.resources.open_text(self.anchor01, 'utf-8.file') as f:
            self.assertEqual(f.read(), 'Hello, UTF-8 world!\n')
        with importlib.resources.open_text(
            self.anchor02, 'subdirectory', 'subsubdir', 'resource.txt',
            encoding='utf-8',
        ) as f:
            self.assertEqual(f.read(), 'a resource')
        # Use generic OSError, since e.g. attempting to read a directory can
        # fail with PermissionError rather than IsADirectoryError
        with self.assertRaises(OSError):
            importlib.resources.open_text(self.anchor01)
        with self.assertRaises(OSError):
            importlib.resources.open_text(self.anchor01, 'no-such-file')
        with importlib.resources.open_text(self.anchor01, 'utf-16.file') as f:
            with self.assertRaises(UnicodeDecodeError):
                f.read()
        with importlib.resources.open_text(
            self.anchor01, 'binary.file', encoding='latin1',
        ) as f:
            self.assertEqual(f.read(), '\x00\x01\x02\x03')
        with importlib.resources.open_text(
            self.anchor01, 'utf-16.file',
            errors='backslashreplace',
        ) as f:
            self.assertEqual(
                f.read(),
                'Hello, UTF-16 world!\n'.encode('utf-16').decode(
                    errors='backslashreplace',
                )
            )

    def test_open_binary(self):
        with importlib.resources.open_binary(self.anchor01, 'utf-8.file') as f:
            self.assertEqual(f.read(), b'Hello, UTF-8 world!\n')
        with importlib.resources.open_binary(
            self.anchor02, 'subdirectory', 'subsubdir', 'resource.txt',
        ) as f:
            self.assertEqual(f.read(), b'a resource')

    def test_path(self):
        with importlib.resources.path(self.anchor01, 'utf-8.file') as path:
            with open(str(path)) as f:
                self.assertEqual(f.read(), 'Hello, UTF-8 world!\n')
        with importlib.resources.path(self.anchor01) as path:
            with open(os.path.join(path, 'utf-8.file')) as f:
                self.assertEqual(f.read(), 'Hello, UTF-8 world!\n')

    def test_is_resource(self):
        is_resource = importlib.resources.is_resource
        self.assertTrue(is_resource(self.anchor01, 'utf-8.file'))
        self.assertFalse(is_resource(self.anchor01, 'no_such_file'))
        self.assertFalse(is_resource(self.anchor01))
        self.assertFalse(is_resource(self.anchor01, 'subdirectory'))

    def test_contents(self):
        is_resource = importlib.resources.is_resource
        with check_warnings((".*contents.*", DeprecationWarning)):
            c = importlib.resources.contents(self.anchor01)
        self.assertGreaterEqual(
            set(c),
            {'utf-8.file', 'utf-16.file', 'binary.file', 'subdirectory'},
        )
        with (self.assertRaises(OSError),
              check_warnings((".*contents.*", DeprecationWarning)),
              ):
            importlib.resources.contents(self.anchor01, 'utf-8.file')
        with check_warnings((".*contents.*", DeprecationWarning)):
            c = importlib.resources.contents(self.anchor01, 'subdirectory')
        self.assertGreaterEqual(
            set(c),
            {'binary.file'},
        )

    @ignore_warnings(category=DeprecationWarning)
    def test_common_errors(self):
        for func in (
            importlib.resources.read_text,
            importlib.resources.read_binary,
            importlib.resources.open_text,
            importlib.resources.open_binary,
            importlib.resources.path,
            importlib.resources.is_resource,
            importlib.resources.contents,
        ):
            with self.subTest(func=func):
                # Rejecting path separators
                with self.assertRaises(ValueError):
                    func(self.anchor02, os.path.join(
                        'subdirectory', 'subsubdir', 'resource.txt',
                    ))
                # Rejecting None anchor
                with self.assertRaises(TypeError):
                    func(None)
                # Rejecting invalid anchor type
                with self.assertRaises((TypeError, AttributeError)):
                    func(1234)
                # Unknown module
                with self.assertRaises(ModuleNotFoundError):
                    func('$missing module$')

    def test_text_errors(self):
        for func in (
            importlib.resources.read_text,
            importlib.resources.open_text,
        ):
            with self.subTest(func=func):
                # Multiple path arguments need explicit encoding argument.
                with self.assertRaises(TypeError):
                    func(
                        self.anchor02, 'subdirectory',
                        'subsubdir', 'resource.txt',
                    )


class FunctionalAPITest_StringAnchor(
    unittest.TestCase, FunctionalAPIBase, StringAnchorMixin,
):
    pass


class FunctionalAPITest_ModuleAnchor(
    unittest.TestCase, FunctionalAPIBase, ModuleAnchorMixin,
):
    pass
