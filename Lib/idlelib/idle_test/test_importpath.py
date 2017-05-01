'''Test idlelib.importpath
'''
import sys
import os
import unittest
import tempfile
from idlelib import importpath


class ImportStdlibRandomConflicTest(unittest.TestCase):
    def setUp(self):
        # Need to restore the current work directory
        self.cwd = os.getcwd()

        # Remove random from sys.modules
        if 'random' in sys.modules:
            sys.modules.pop('random')

        if sys.path[0] != '':
            sys.path.insert(0, '')

    def tearDown(self):
        # Restore cwd
        os.chdir(self.cwd)

        # Restore sys.path
        if sys.path[0] != '':
            sys.path.insert(0, '')

    def test_import_random_should_import_stdlib(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create an empty random.py
            random_py = os.path.join(tmpdir, 'random.py')
            with open(random_py, 'w'):
                pass

            # Change dir to tempdir
            os.chdir(tmpdir)
            # Patch by _fix_import_path
            importpath._fix_import_path()

            # Import and check
            import random
            self.assertNotEqual(os.path.dirname(random.__file__), tmpdir.name)

    def test_import_random_should_import_locals(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create an empty random.py
            random_py = os.path.join(tmpdir, 'random.py')
            with open(random_py, 'w'):
                pass

            # Change dir to tempdir
            os.chdir(tmpdir)

            # Import and check
            import random
            self.assertEqual(os.path.dirname(random.__file__), tmpdir.name)


class ImportStdlibCtypesConflicTest(unittest.TestCase):
    def setUp(self):
        # Need to restore the current work directory
        self.cwd = os.getcwd()

        # Remove ctypes from sys.modules
        if 'ctypes' in sys.modules:
            sys.modules.pop('ctypes')

        if sys.path[0] != '':
            sys.path.insert(0, '')

    def tearDown(self):
        # Restore cwd
        os.chdir(self.cwd)

        # Restore sys.path
        if sys.path[0] != '':
            sys.path.insert(0, '')

    def test_import_ctypes_should_import_stdlib(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create an empty ctypes.py
            ctypes_py = os.path.join(tmpdir, 'ctypes.py')
            with open(ctypes_py, 'w'):
                pass

            # Change dir to tempdir
            os.chdir(tmpdir)

            # Patch by _fix_import_path
            importpath._fix_import_path()

            # Import and check
            import ctypes
            self.assertNotEqual(os.path.dirname(ctypes.__file__), tmpdir.name)

    def test_import_ctypes_should_import_locals(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create an empty ctypes.py
            ctypes_py = os.path.join(tmpdir, 'ctypes.py')
            with open(ctypes_py, 'w'):
                pass

            # Change dir to tempdir
            os.chdir(tmpdir)

            # Import and check
            import ctypes
            self.assertEqual(os.path.dirname(ctypes.__file__), tmpdir.name)


class ImportLocalsFooTest(unittest.TestCase):
    def setUp(self):
        # Need to restore the current work directory
        self.cwd = os.getcwd()

        # Remove foo from sys.modules
        if 'foo' in sys.modules:
            sys.modules.pop('foo')

        if sys.path[0] != '':
            sys.path.insert(0, '')

    def tearDown(self):
        # Restore cwd
        os.chdir(self.cwd)

        # Restore sys.path
        if sys.path[0] != '':
            sys.path.insert(0, '')

    def test_import_foo(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create an empty foo.py
            foo_py = os.path.join(tmpdir, 'foo.py')
            with open(foo_py, 'w'):
                pass

            # Change dir to tempdir
            os.chdir(tmpdir)

            # Import and check
            import foo
            self.assertEqual(os.path.dirname(foo.__file__), tmpdir.name)

    def test_import_foo_should_failed(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            # Create an empty foo.py
            foo_py = os.path.join(tmpdir, 'foo.py')
            with open(foo_py, 'w'):
                pass

            # Change dir to tempdir
            os.chdir(tmpdir)

            # Patch by _fix_import_path
            importpath._fix_import_path()

            # Import and check
            with self.assertRaises(ModuleNotFoundError):
                import foo


if __name__ == '__main__':
    unittest.main()
