# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import struct

import six

from cryptography.exceptions import UnsupportedAlgorithm, _Reasons
from cryptography.hazmat.backends import _get_backend
from cryptography.hazmat.backends.interfaces import HMACBackend
from cryptography.hazmat.primitives import constant_time, hmac
from cryptography.hazmat.primitives.hashes import SHA1, SHA256, SHA512
from cryptography.hazmat.primitives.twofactor import InvalidToken
from cryptography.hazmat.primitives.twofactor.utils import _generate_uri


class HOTP(object):
    def __init__(
        self, key, length, algorithm, backend=None, enforce_key_length=True
    ):
        backend = _get_backend(backend)
        if not isinstance(backend, HMACBackend):
            raise UnsupportedAlgorithm(
                "Backend object does not implement HMACBackend.",
                _Reasons.BACKEND_MISSING_INTERFACE,
            )

        if len(key) < 16 and enforce_key_length is True:
            raise ValueError("Key length has to be at least 128 bits.")

        if not isinstance(length, six.integer_types):
            raise TypeError("Length parameter must be an integer type.")

        if length < 6 or length > 8:
            raise ValueError("Length of HOTP has to be between 6 to 8.")

        if not isinstance(algorithm, (SHA1, SHA256, SHA512)):
            raise TypeError("Algorithm must be SHA1, SHA256 or SHA512.")

        self._key = key
        self._length = length
        self._algorithm = algorithm
        self._backend = backend

    def generate(self, counter):
        truncated_value = self._dynamic_truncate(counter)
        hotp = truncated_value % (10 ** self._length)
        return "{0:0{1}}".format(hotp, self._length).encode()

    def verify(self, hotp, counter):
        if not constant_time.bytes_eq(self.generate(counter), hotp):
            raise InvalidToken("Supplied HOTP value does not match.")

    def _dynamic_truncate(self, counter):
        ctx = hmac.HMAC(self._key, self._algorithm, self._backend)
        ctx.update(struct.pack(">Q", counter))
        hmac_value = ctx.finalize()

        offset = six.indexbytes(hmac_value, len(hmac_value) - 1) & 0b1111
        p = hmac_value[offset : offset + 4]
        return struct.unpack(">I", p)[0] & 0x7FFFFFFF

    def get_provisioning_uri(self, account_name, counter, issuer):
        return _generate_uri(
            self, "hotp", account_name, issuer, [("counter", int(counter))]
        )
