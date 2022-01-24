"Test constants and functions found in idlelib.util"

import unittest
from unittest import mock
from idlelib import util


class ExtensionTest(unittest.TestCase):
    def test_extensions(self):
        for extension in {'.pyi', '.py', '.pyw'}:
            with self.subTest(extension=extension):
                self.assertIn(extension, util.PYTHON_EXTENSIONS)


class IsPythonExtensionTest(unittest.TestCase):    
    def test_positives(self):
        for extension in util.PYTHON_EXTENSIONS:
            with self.subTest(extension=extension):
                self.assertTrue(util.is_python_extension(extension))

    def test_negatives(self):
        for bad_extension in {'.pyc', '.p', '.pyy', '..py', '.ppy'}:
            with self.subTest(bad_extension=bad_extension):
                self.assertFalse(util.is_python_extension(bad_extension))


class IsPythonSourceTest(unittest.TestCase):
    def test_empty_string_filepath(self):
        self.assertTrue(util.is_python_source(''))

    def test_directory_filepath(self):
        with mock.patch('os.path.isdir', return_value=True):
            self.assertTrue(
                util.is_python_source(filepath='asdfoijf9245-98-$%^&*.json')
            )

    def test_filepath_with_python_extension(self):
        with (
            mock.patch('os.path.isdir', return_value=False),
            mock.patch('idlelib.util.is_python_extension', return_value=True)
        ):
            self.assertTrue(
                util.is_python_source(filepath='asdfe798723%^&*^%£(.docx')
            )

    def test_bad_filepath_no_firstline(self):
        with (
            mock.patch('os.path.isdir', return_value=False),
            mock.patch('idlelib.util.is_python_extension', return_value=False)
        ):
            self.assertFalse(
                util.is_python_source(filepath='%^*iupoukjcj&)9675889')
            )

    def test_bad_filepath_good_firstline(self):
        test_cases = (
            ('ad^%^&*yoi724)', '#!python'),
            ('7890J:IUJjhu435465$£^%£', '#!     python'),
            ('$%^&554657jhasfnmnbnnnm', '#!asdfkpoijeqrjiumnbjypythonasdfjlh')
        )

        with (
            mock.patch('os.path.isdir', return_value=False),
            mock.patch('idlelib.util.is_python_extension', return_value=False)
        ):
            for filepath, firstline in test_cases:
                with self.subTest(filepath=filepath, firstline=firstline):
                    self.assertTrue(util.is_python_source(
                        filepath=filepath,
                        firstline=firstline
                    ))

    def test_bad_filepath_bad_firstline(self):
        test_cases = (
            ('tyyuiuoiu', '#python!'),
            ('796923568%^&**$', '!#python'),
            ('67879iy987kjl87', 'python#!')
        )

        with (
            mock.patch('os.path.isdir', return_value=False),
            mock.patch('idlelib.util.is_python_extension', return_value=False)
        ):
            for filepath, firstline in test_cases:
                with self.subTest(filepath=filepath, firstline=firstline):
                    self.assertFalse(util.is_python_source(
                        filepath=filepath,
                        firstline=firstline
                    ))


if __name__ == '__main__':
    unittest.main(verbosity=2)
