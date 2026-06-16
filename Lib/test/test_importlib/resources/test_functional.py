import importlib
import importlib.resources as resources
import os
import unittest

from test.support import warnings_helper

from . import util


class StringAnchorMixin:
    anchor01 = 'data01'
    anchor02 = 'data02'


class ModuleAnchorMixin:
    @property
    def anchor01(self):
        return importlib.import_module('data01')

    @property
    def anchor02(self):
        return importlib.import_module('data02')


class FunctionalAPIBase:
    def setUp(self):
        super().setUp()
        self.load_fixture('data02')

    def _gen_resourcetxt_path_parts(self):
        """Yield various names of a text file in anchor02, each in a subTest"""
        for path_parts in (
            ('subdirectory', 'subsubdir', 'resource.txt'),
            ('subdirectory/subsubdir/resource.txt',),
            ('subdirectory/subsubdir', 'resource.txt'),
        ):
            with self.subTest(path_parts=path_parts):
                yield path_parts

    @staticmethod
    def remove_utf16_bom(string):
        """Remove an architecture-specific UTF-16 BOM prefix when present.

        Some platforms surface UTF-16 BOM bytes as escaped text when the
        fixture is intentionally decoded as UTF-8 with ``errors='backslashreplace'``.
        Strip that prefix so assertions validate content consistently."""
        for bom in ('\\xff\\xfe', '\\xfe\\xff', '\ufeff'):
            if string.startswith(bom):
                return string[len(bom) :]
        return string

    def test_read_text(self):
        assert (
            resources.read_text(self.anchor01, 'utf-8.file') == 'Hello, UTF-8 world!\n'
        )
        assert (
            resources.read_text(
                self.anchor02,
                'subdirectory',
                'subsubdir',
                'resource.txt',
                encoding='utf-8',
            )
            == 'a resource'
        )
        for path_parts in self._gen_resourcetxt_path_parts():
            assert (
                resources.read_text(
                    self.anchor02,
                    *path_parts,
                    encoding='utf-8',
                )
                == 'a resource'
            )
        # Use generic OSError, since e.g. attempting to read a directory can
        # fail with PermissionError rather than IsADirectoryError
        with self.assertRaises(OSError):
            resources.read_text(self.anchor01)
        with self.assertRaises((OSError, resources.abc.TraversalError)):
            resources.read_text(self.anchor01, 'no-such-file')
        with self.assertRaises(UnicodeDecodeError):
            resources.read_text(self.anchor01, 'utf-16.file')
        assert (
            resources.read_text(
                self.anchor01,
                'binary.file',
                encoding='latin1',
            )
            == '\x00\x01\x02\x03'
        )
        assert self.remove_utf16_bom(
            resources.read_text(
                self.anchor01,
                'utf-16.file',
                errors='backslashreplace',
            ),
        ) == 'Hello, UTF-16 world!\n'.encode('utf-16-le').decode(
            errors='backslashreplace',
        )

    def test_read_binary(self):
        assert (
            resources.read_binary(self.anchor01, 'utf-8.file')
            == b'Hello, UTF-8 world!\n'
        )
        for path_parts in self._gen_resourcetxt_path_parts():
            assert resources.read_binary(self.anchor02, *path_parts) == b'a resource'

    def test_open_text(self):
        with resources.open_text(self.anchor01, 'utf-8.file') as f:
            assert f.read() == 'Hello, UTF-8 world!\n'
        for path_parts in self._gen_resourcetxt_path_parts():
            with resources.open_text(
                self.anchor02,
                *path_parts,
                encoding='utf-8',
            ) as f:
                assert f.read() == 'a resource'
        # Use generic OSError, since e.g. attempting to read a directory can
        # fail with PermissionError rather than IsADirectoryError
        with self.assertRaises(OSError):
            resources.open_text(self.anchor01)
        with self.assertRaises((OSError, resources.abc.TraversalError)):
            resources.open_text(self.anchor01, 'no-such-file')
        with resources.open_text(self.anchor01, 'utf-16.file') as f:
            with self.assertRaises(UnicodeDecodeError):
                f.read()
        with resources.open_text(
            self.anchor01,
            'binary.file',
            encoding='latin1',
        ) as f:
            assert f.read() == '\x00\x01\x02\x03'
        with resources.open_text(
            self.anchor01,
            'utf-16.file',
            errors='backslashreplace',
        ) as f:
            assert self.remove_utf16_bom(f.read()) == 'Hello, UTF-16 world!\n'.encode(
                'utf-16-le'
            ).decode(
                errors='backslashreplace',
            )

    def test_open_binary(self):
        with resources.open_binary(self.anchor01, 'utf-8.file') as f:
            assert f.read() == b'Hello, UTF-8 world!\n'
        for path_parts in self._gen_resourcetxt_path_parts():
            with resources.open_binary(
                self.anchor02,
                *path_parts,
            ) as f:
                assert f.read() == b'a resource'

    def test_path(self):
        with resources.path(self.anchor01, 'utf-8.file') as path:
            with open(str(path), encoding='utf-8') as f:
                assert f.read() == 'Hello, UTF-8 world!\n'
        with resources.path(self.anchor01) as path:
            with open(os.path.join(path, 'utf-8.file'), encoding='utf-8') as f:
                assert f.read() == 'Hello, UTF-8 world!\n'

    def test_is_resource(self):
        is_resource = resources.is_resource
        assert is_resource(self.anchor01, 'utf-8.file')
        assert not is_resource(self.anchor01, 'no_such_file')
        assert not is_resource(self.anchor01)
        assert not is_resource(self.anchor01, 'subdirectory')
        for path_parts in self._gen_resourcetxt_path_parts():
            assert is_resource(self.anchor02, *path_parts)

    def test_contents(self):
        with warnings_helper.check_warnings((".*contents.*", DeprecationWarning)):
            c = resources.contents(self.anchor01)
        assert set(c) >= {'utf-8.file', 'utf-16.file', 'binary.file', 'subdirectory'}
        with (
            self.assertRaises(OSError),
            warnings_helper.check_warnings((
                ".*contents.*",
                DeprecationWarning,
            )),
        ):
            list(resources.contents(self.anchor01, 'utf-8.file'))

        for path_parts in self._gen_resourcetxt_path_parts():
            with (
                self.assertRaises((OSError, resources.abc.TraversalError)),
                warnings_helper.check_warnings((
                    ".*contents.*",
                    DeprecationWarning,
                )),
            ):
                list(resources.contents(self.anchor01, *path_parts))
        with warnings_helper.check_warnings((".*contents.*", DeprecationWarning)):
            c = resources.contents(self.anchor01, 'subdirectory')
        assert set(c) >= {'binary.file'}

    @warnings_helper.ignore_warnings(category=DeprecationWarning)
    def test_common_errors(self):
        for func in (
            resources.read_text,
            resources.read_binary,
            resources.open_text,
            resources.open_binary,
            resources.path,
            resources.is_resource,
            resources.contents,
        ):
            with self.subTest(func=func):
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
            resources.read_text,
            resources.open_text,
        ):
            with self.subTest(func=func):
                # Multiple path arguments need explicit encoding argument.
                with self.assertRaises(TypeError):
                    func(
                        self.anchor02,
                        'subdirectory',
                        'subsubdir',
                        'resource.txt',
                    )


class FunctionalAPITest_StringAnchor_Disk(
    StringAnchorMixin,
    FunctionalAPIBase,
    util.DiskSetup,
    unittest.TestCase,
):
    pass


class FunctionalAPITest_ModuleAnchor_Disk(
    ModuleAnchorMixin,
    FunctionalAPIBase,
    util.DiskSetup,
    unittest.TestCase,
):
    pass


class FunctionalAPITest_StringAnchor_Memory(
    StringAnchorMixin,
    FunctionalAPIBase,
    util.MemorySetup,
    unittest.TestCase,
):
    pass
