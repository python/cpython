from test.support import swap_attr
import unittest

from importlib import resources
from . import data01
from . import util


class CommonTests(util.CommonResourceTests, unittest.TestCase):
    def execute(self, package, path):
        with resources.path(package, path):
            pass


class PathTests:
    def test_reading(self):
        # Path should be readable.
        # Test also implicitly verifies the returned object is a pathlib.Path
        # instance.
        with resources.path(self.data, 'utf-8.file') as path:
            # pathlib.Path.read_text() was introduced in Python 3.5.
            with path.open('r', encoding='utf-8') as file:
                text = file.read()
            self.assertEqual('Hello, UTF-8 world!\n', text)


class PathDiskTests(PathTests, unittest.TestCase):
    data = data01

    def test_package_spec_origin_is_None(self):
        import pydoc_data
        spec = pydoc_data.__spec__
        # Emulate importing from non-file source by setting spec.origin = None.
        # Barge past path's sanity checks by ensuring spec.loader.is_resource
        # returns False.
        with swap_attr(spec, "origin", None), \
            swap_attr(spec.loader, "is_resource", lambda *args: False), \
            resources.path(pydoc_data, '_pydoc.css') as p:
            pass


class PathZipTests(PathTests, util.ZipSetup, unittest.TestCase):
    def test_remove_in_context_manager(self):
        # It is not an error if the file that was temporarily stashed on the
        # file system is removed inside the `with` stanza.
        with resources.path(self.data, 'utf-8.file') as path:
            path.unlink()


if __name__ == '__main__':
    unittest.main()
