# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

from cryptography import utils
from cryptography.exceptions import (
    AlreadyFinalized,
    InvalidKey,
    UnsupportedAlgorithm,
    _Reasons,
)
from cryptography.hazmat.backends import _get_backend
from cryptography.hazmat.backends.interfaces import PBKDF2HMACBackend
from cryptography.hazmat.primitives import constant_time
from cryptography.hazmat.primitives.kdf import KeyDerivationFunction


@utils.register_interface(KeyDerivationFunction)
class PBKDF2HMAC(object):
    def __init__(self, algorithm, length, salt, iterations, backend=None):
        backend = _get_backend(backend)
        if not isinstance(backend, PBKDF2HMACBackend):
            raise UnsupportedAlgorithm(
                "Backend object does not implement PBKDF2HMACBackend.",
                _Reasons.BACKEND_MISSING_INTERFACE,
            )

        if not backend.pbkdf2_hmac_supported(algorithm):
            raise UnsupportedAlgorithm(
                "{} is not supported for PBKDF2 by this backend.".format(
                    algorithm.name
                ),
                _Reasons.UNSUPPORTED_HASH,
            )
        self._used = False
        self._algorithm = algorithm
        self._length = length
        utils._check_bytes("salt", salt)
        self._salt = salt
        self._iterations = iterations
        self._backend = backend

    def derive(self, key_material):
        if self._used:
            raise AlreadyFinalized("PBKDF2 instances can only be used once.")
        self._used = True

        utils._check_byteslike("key_material", key_material)
        return self._backend.derive_pbkdf2_hmac(
            self._algorithm,
            self._length,
            self._salt,
            self._iterations,
            key_material,
        )

    def verify(self, key_material, expected_key):
        derived_key = self.derive(key_material)
        if not constant_time.bytes_eq(derived_key, expected_key):
            raise InvalidKey("Keys do not match.")
