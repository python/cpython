import io
import pathlib
import unittest

from importlib import resources
from . import util


class CommonTests(util.CommonTests, unittest.TestCase):
    def execute(self, package, path):
        with resources.as_file(resources.files(package).joinpath(path)):
            pass


class PathTests:
    def test_reading(self):
        """
        Path should be readable and a pathlib.Path instance.
        """
        target = resources.files(self.data) / 'utf-8.file'
        with resources.as_file(target) as path:
            self.assertIsInstance(path, pathlib.Path)
            self.assertEndsWith(path.name, "utf-8.file")
            self.assertEqual('Hello, UTF-8 world!\n', path.read_text(encoding='utf-8'))


class PathDiskTests(PathTests, util.DiskSetup, unittest.TestCase):
    def test_natural_path(self):
        # Guarantee the internal implementation detail that
        # file-system-backed resources do not get the tempdir
        # treatment.
        target = resources.files(self.data) / 'utf-8.file'
        with resources.as_file(target) as path:
            assert 'data' in str(path)


class PathMemoryTests(PathTests, unittest.TestCase):
    def setUp(self):
        file = io.BytesIO(b'Hello, UTF-8 world!\n')
        self.addCleanup(file.close)
        self.data = util.create_package(
            file=file, path=FileNotFoundError("package exists only in memory")
        )
        self.data.__spec__.origin = None
        self.data.__spec__.has_location = False


class PathZipTests(PathTests, util.ZipSetup, unittest.TestCase):
    def test_remove_in_context_manager(self):
        """
        It is not an error if the file that was temporarily stashed on the
        file system is removed inside the `with` stanza.
        """
        target = resources.files(self.data) / 'utf-8.file'
        with resources.as_file(target) as path:
            path.unlink()


if __name__ == '__main__':
    unittest.main()
