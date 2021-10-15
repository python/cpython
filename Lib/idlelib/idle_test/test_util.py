"Test constants and functions found in idlelib.util"

import unittest
from unittest import mock
from idlelib import util


class ExtensionTest(unittest.TestCase):
    def test_pyi(self):
        self.assertIn('.pyi', util.PYTHON_EXTENSIONS)

    def test_py(self):
        self.assertIn('.py', util.PYTHON_EXTENSIONS)

    def test_pyw(self):
        self.assertIn('.pyw', util.PYTHON_EXTENSIONS)


class IsPythonExtensionTest(unittest.TestCase):    
    def test_positives(self):
        for extension in util.PYTHON_EXTENSIONS:
            self.assertTrue(util.is_python_extension(extension))

    def test_negatives(self):
        NEGATIVES = '.pyc', '.p', 'py', '.pyy', '..py', '.ppy'
        for bad_extension in NEGATIVES:
            self.assertFalse(util.is_python_extension(bad_extension))


class IsPythonSourceTest(unittest.TestCase):
    def test_empty_string_filepath(self):
        self.assertTrue(util.is_python_source(''))

    def test_directory_filepath(self):
        with mock.patch('os.path.isdir', return_value=True):
            self.assertTrue(util.is_python_source('asdfoijf9245-98-$%^&*.json'))

    def test_python_extension(self):
        with mock.patch('idlelib.util.is_python_extension', return_value=True):
            self.assertTrue(util.is_python_source('asdfe798723%^&*^%£(.docx'))

    def test_bad_filepath_no_firstline(self):
        with (
            mock.patch('os.path.isdir', return_value=False),
            mock.patch('idlelib.util.is_python_extension', return_value=False)
        ):
            self.assertFalse(util.is_python_source('%^*iupoukjcj&)9675889'))

    def test_bad_filepath_good_firstline(self):        
        with (
            mock.patch('os.path.isdir', return_value=False),
            mock.patch('idlelib.util.is_python_extension', return_value=False)
        ):
            self.assertTrue(util.is_python_source(
                'ad^%^&*yoi724)', '#!python'
            ))
            self.assertTrue(util.is_python_source(
                '7890J:IUJjhu435465$£^%£', '#!     python'
            ))
            self.assertTrue(util.is_python_source(
                '$%^&554657jhasfnmnbnnnm', '#!asdfkpoijeqrjiumnbjypythonasdfjlh'
            ))

    def test_bad_filepath_bad_firstline(self):
        with (
            mock.patch('os.path.isdir', return_value=False),
            mock.patch('idlelib.util.is_python_extension', return_value=False)
        ):
            self.assertFalse(util.is_python_source(
                'tyyuiuoiu', '#python!'
            ))
            self.assertFalse(util.is_python_source(
                '796923568%^&**$', '!#python'
            ))
            self.assertFalse(util.is_python_source(
                '67879iy987kjl87', 'python#!'
            ))


if __name__ == '__main__':
    unittest.main(verbosity=2)
