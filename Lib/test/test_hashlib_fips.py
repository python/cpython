# Test the hashlib module usedforsecurity wrappers under fips.
#
#  Copyright (C) 2024   Dimitri John Ledkov (dimitri.ledkov@surgut.co.uk)
#  Licensed to PSF under a Contributor Agreement.
#

import os
import unittest

# This openssl.cnf mocks FIPS mode without any digest loaded. It means
# all digests must raise ValueError when usedforsecurity=True via
# either openssl or builtin constructors
OPENSSL_CONF = os.path.join(os.path.dirname(__file__), "hashlibdata", "openssl.cnf")
os.environ["OPENSSL_CONF"] = OPENSSL_CONF

# Ensure hashlib is loading a fresh libcrypto with openssl context
# affected by the above config file. Check if this can be folded into test_hashlib.py, specifically if import_fresh_module() results in a fresh Open

import hashlib

class HashLibFIPSTestCase(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        try:
            from _hashlib import get_fips_mode
        except ImportError:
            self.skipTest('_hashlib not available')

        if get_fips_mode() != 1:
            self.skipTest('mock fips mode setup failed')

        super(HashLibFIPSTestCase, self).__init__(*args, **kwargs)

    def test_algorithms_available(self):
        self.assertTrue(set(hashlib.algorithms_guaranteed).
                            issubset(hashlib.algorithms_available))
        # all available algorithms must be loadable, bpo-47101
        self.assertNotIn("undefined", hashlib.algorithms_available)
        for name in hashlib.algorithms_available:
            digest = hashlib.new(name, usedforsecurity=False)

    def test_usedforsecurity_true(self):
        for name in hashlib.algorithms_available:
            with self.assertRaises(ValueError):
                digest = hashlib.new(name, usedforsecurity=True)

if __name__ == "__main__":
    unittest.main()
