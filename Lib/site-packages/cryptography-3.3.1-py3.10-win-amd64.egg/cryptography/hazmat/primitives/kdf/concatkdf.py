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
from cryptography.hazmat.backends.interfaces import HMACBackend
from cryptography.hazmat.backends.interfaces import HashBackend
from cryptography.hazmat.primitives import constant_time, hashes, hmac
from cryptography.hazmat.primitives.kdf import KeyDerivationFunction


def _int_to_u32be(n):
    return struct.pack(">I", n)


def _common_args_checks(algorithm, length, otherinfo):
    max_length = algorithm.digest_size * (2 ** 32 - 1)
    if length > max_length:
        raise ValueError(
            "Can not derive keys larger than {} bits.".format(max_length)
        )
    if otherinfo is not None:
        utils._check_bytes("otherinfo", otherinfo)


def _concatkdf_derive(key_material, length, auxfn, otherinfo):
    utils._check_byteslike("key_material", key_material)
    output = [b""]
    outlen = 0
    counter = 1

    while length > outlen:
        h = auxfn()
        h.update(_int_to_u32be(counter))
        h.update(key_material)
        h.update(otherinfo)
        output.append(h.finalize())
        outlen += len(output[-1])
        counter += 1

    return b"".join(output)[:length]


@utils.register_interface(KeyDerivationFunction)
class ConcatKDFHash(object):
    def __init__(self, algorithm, length, otherinfo, backend=None):
        backend = _get_backend(backend)

        _common_args_checks(algorithm, length, otherinfo)
        self._algorithm = algorithm
        self._length = length
        self._otherinfo = otherinfo
        if self._otherinfo is None:
            self._otherinfo = b""

        if not isinstance(backend, HashBackend):
            raise UnsupportedAlgorithm(
                "Backend object does not implement HashBackend.",
                _Reasons.BACKEND_MISSING_INTERFACE,
            )
        self._backend = backend
        self._used = False

    def _hash(self):
        return hashes.Hash(self._algorithm, self._backend)

    def derive(self, key_material):
        if self._used:
            raise AlreadyFinalized
        self._used = True
        return _concatkdf_derive(
            key_material, self._length, self._hash, self._otherinfo
        )

    def verify(self, key_material, expected_key):
        if not constant_time.bytes_eq(self.derive(key_material), expected_key):
            raise InvalidKey


@utils.register_interface(KeyDerivationFunction)
class ConcatKDFHMAC(object):
    def __init__(self, algorithm, length, salt, otherinfo, backend=None):
        backend = _get_backend(backend)

        _common_args_checks(algorithm, length, otherinfo)
        self._algorithm = algorithm
        self._length = length
        self._otherinfo = otherinfo
        if self._otherinfo is None:
            self._otherinfo = b""

        if salt is None:
            salt = b"\x00" * algorithm.block_size
        else:
            utils._check_bytes("salt", salt)

        self._salt = salt

        if not isinstance(backend, HMACBackend):
            raise UnsupportedAlgorithm(
                "Backend object does not implement HMACBackend.",
                _Reasons.BACKEND_MISSING_INTERFACE,
            )
        self._backend = backend
        self._used = False

    def _hmac(self):
        return hmac.HMAC(self._salt, self._algorithm, self._backend)

    def derive(self, key_material):
        if self._used:
            raise AlreadyFinalized
        self._used = True
        return _concatkdf_derive(
            key_material, self._length, self._hmac, self._otherinfo
        )

    def verify(self, key_material, expected_key):
        if not constant_time.bytes_eq(self.derive(key_material), expected_key):
            raise InvalidKey
