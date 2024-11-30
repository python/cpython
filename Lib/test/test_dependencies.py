import unittest
import os
import sys

class TestDependencies(unittest.TestCase):

    def test_pyvenv_cfg_exists(self):
        venv_path = os.path.join(sys.prefix, 'pyvenv.cfg')
        self.assertTrue(os.path.exists(venv_path), f"{venv_path} does not exist")

    def test_shared_memory_access(self):
        try:
            import multiprocessing.shared_memory
        except ImportError:
            self.fail("multiprocessing.shared_memory module is not available")

    def test_ssl_module(self):
        try:
            import ssl
        except ImportError:
            self.fail("ssl module is not available")

    def test_pdb_module(self):
        try:
            import pdb
        except ImportError:
            self.fail("pdb module is not available")

    def test_warnings_module(self):
        try:
            import warnings
        except ImportError:
            self.fail("warnings module is not available")

if __name__ == '__main__':
    unittest.main()
