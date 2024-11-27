# Test the hashlib module usedforsecurity wrappers under fips.
#
#  Copyright (C) 2024   Dimitri John Ledkov (dimitri.ledkov@surgut.co.uk)
#  Licensed to PSF under a Contributor Agreement.
#

import os
import sys
import unittest

OPENSSL_CONF_BACKUP = os.environ.get("OPENSSL_CONF")


class HashLibFIPSTestCase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if sys.modules.get("_hashlib") or sys.modules.get("_ssl"):
            raise AssertionError("_hashlib or _ssl already imported, too late to change OPENSSL_CONF.")
        # This openssl.cnf mocks FIPS mode without any digest
        # loaded. It means all digests must raise ValueError when
        # usedforsecurity=True via either openssl or builtin
        # constructors
        OPENSSL_CONF = os.path.join(os.path.dirname(__file__), "hashlibdata", "openssl.cnf")
        os.environ["OPENSSL_CONF"] = OPENSSL_CONF
        # Ensure hashlib is loading a fresh libcrypto with openssl
        # context affected by the above config file. Check if this can
        # be folded into test_hashlib.py, specifically if
        # import_fresh_module() results in a fresh library context
        import hashlib

    def setUp(self):
        try:
            from _hashlib import get_fips_mode
        except ImportError:
            self.skipTest('_hashlib not available')

        if get_fips_mode() != 1:
            self.skipTest('mocking fips mode failed')

    @classmethod
    def tearDownClass(cls):
        if OPENSSL_CONF_BACKUP is not None:
            os.environ["OPENSSL_CONF"] = OPENSSL_CONF_BACKUP
        else:
            os.environ.pop("OPENSSL_CONF", None)

    def test_algorithms_available(self):
        import hashlib
        self.assertTrue(set(hashlib.algorithms_guaranteed).
                            issubset(hashlib.algorithms_available))
        # all available algorithms must be loadable, bpo-47101
        self.assertNotIn("undefined", hashlib.algorithms_available)
        for name in hashlib.algorithms_available:
            digest = hashlib.new(name, usedforsecurity=False)

    def test_usedforsecurity_true(self):
        import hashlib
        for name in hashlib.algorithms_available:
            with self.assertRaises(ValueError):
                digest = hashlib.new(name, usedforsecurity=True)

if __name__ == "__main__":
    unittest.main()
