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
        self.assertFalse(util.is_python_extension('.pyc'))
        self.assertFalse(util.is_python_extension('.p'))
        self.assertFalse(util.is_python_extension('.pyy'))
        self.assertFalse(util.is_python_extension('..py'))
        self.assertFalse(util.is_python_extension('.ppy'))


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
        with (
            mock.patch('os.path.isdir', return_value=False),
            mock.patch('idlelib.util.is_python_extension', return_value=False)
        ):
            self.assertTrue(util.is_python_source(
                filepath='ad^%^&*yoi724)',
                firstline='#!python'
            ))
            self.assertTrue(util.is_python_source(
                filepath='7890J:IUJjhu435465$£^%£',
                firstline='#!     python'
            ))
            self.assertTrue(util.is_python_source(
                filepath='$%^&554657jhasfnmnbnnnm',
                firstline='#!asdfkpoijeqrjiumnbjypythonasdfjlh'
            ))

    def test_bad_filepath_bad_firstline(self):
        with (
            mock.patch('os.path.isdir', return_value=False),
            mock.patch('idlelib.util.is_python_extension', return_value=False)
        ):
            self.assertFalse(util.is_python_source(
                filepath='tyyuiuoiu',
                firstline='#python!'
            ))
            self.assertFalse(util.is_python_source(
                filepath='796923568%^&**$',
                firstline='!#python'
            ))
            self.assertFalse(util.is_python_source(
                filepath='67879iy987kjl87',
                firstline='python#!'
            ))


if __name__ == '__main__':
    unittest.main(verbosity=2)
