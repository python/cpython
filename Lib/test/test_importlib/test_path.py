import io
import unittest

from importlib import resources
from . import data01
from . import util


class CommonTests(util.CommonResourceTests, unittest.TestCase):
    def execute(self, package, path):
        with resources.path(package, path):
            pass


class PathTests:
    UTF_8_FILE_CONTENTS = b'Hello, UTF-8 world!\n'

    def test_reading(self):
        # Path should be readable.
        # Test also implicitly verifies the returned object is a pathlib.Path
        # instance.
        with resources.path(self.data, 'utf-8.file') as path:
            # pathlib.Path.read_text() was introduced in Python 3.5.
            with path.open('r', encoding='utf-8') as file:
                text = file.read()
            self.assertEqual(self.UTF_8_FILE_CONTENTS.decode(), text)


class PathDiskTests(PathTests, unittest.TestCase):
    data = data01


class PathMemoryTests(PathTests, unittest.TestCase):
    def setUp(self):
        file = io.BytesIO(self.UTF_8_FILE_CONTENTS)
        self.addCleanup(file.close)
        self.data = util.create_package(
            file=file, path=FileNotFoundError("package exists only in memory"))
        self.data.__spec__.origin = None
        self.data.__spec__.has_location = False


class PathZipTests(PathTests, util.ZipSetup, unittest.TestCase):
    def test_remove_in_context_manager(self):
        # It is not an error if the file that was temporarily stashed on the
        # file system is removed inside the `with` stanza.
        with resources.path(self.data, 'utf-8.file') as path:
            path.unlink()


if __name__ == '__main__':
    unittest.main()
