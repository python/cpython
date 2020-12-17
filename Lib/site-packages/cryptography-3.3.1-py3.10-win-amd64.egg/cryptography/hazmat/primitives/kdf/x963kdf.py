# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import struct

from cryptography import utils
from cryptography.exceptions import (
    AlreadyFinalized,
    InvalidKey,
    UnsupportedAlgorithm,
    _Reasons,
)
from cryptography.hazmat.backends import _get_backend
from cryptography.hazmat.backends.interfaces import HashBackend
from cryptography.hazmat.primitives import constant_time, hashes
from cryptography.hazmat.primitives.kdf import KeyDerivationFunction


def _int_to_u32be(n):
    return struct.pack(">I", n)


@utils.register_interface(KeyDerivationFunction)
class X963KDF(object):
    def __init__(self, algorithm, length, sharedinfo, backend=None):
        backend = _get_backend(backend)

        max_len = algorithm.digest_size * (2 ** 32 - 1)
        if length > max_len:
            raise ValueError(
                "Can not derive keys larger than {} bits.".format(max_len)
            )
        if sharedinfo is not None:
            utils._check_bytes("sharedinfo", sharedinfo)

        self._algorithm = algorithm
        self._length = length
        self._sharedinfo = sharedinfo

        if not isinstance(backend, HashBackend):
            raise UnsupportedAlgorithm(
                "Backend object does not implement HashBackend.",
                _Reasons.BACKEND_MISSING_INTERFACE,
            )
        self._backend = backend
        self._used = False

    def derive(self, key_material):
        if self._used:
            raise AlreadyFinalized
        self._used = True
        utils._check_byteslike("key_material", key_material)
        output = [b""]
        outlen = 0
        counter = 1

        while self._length > outlen:
            h = hashes.Hash(self._algorithm, self._backend)
            h.update(key_material)
            h.update(_int_to_u32be(counter))
            if self._sharedinfo is not None:
                h.update(self._sharedinfo)
            output.append(h.finalize())
            outlen += len(output[-1])
            counter += 1

        return b"".join(output)[: self._length]

    def verify(self, key_material, expected_key):
        if not constant_time.bytes_eq(self.derive(key_material), expected_key):
            raise InvalidKey
