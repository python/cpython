# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

from enum import Enum

from six.moves import range

from cryptography import utils
from cryptography.exceptions import (
    AlreadyFinalized,
    InvalidKey,
    UnsupportedAlgorithm,
    _Reasons,
)
from cryptography.hazmat.backends import _get_backend
from cryptography.hazmat.backends.interfaces import HMACBackend
from cryptography.hazmat.primitives import constant_time, hashes, hmac
from cryptography.hazmat.primitives.kdf import KeyDerivationFunction


class Mode(Enum):
    CounterMode = "ctr"


class CounterLocation(Enum):
    BeforeFixed = "before_fixed"
    AfterFixed = "after_fixed"


@utils.register_interface(KeyDerivationFunction)
class KBKDFHMAC(object):
    def __init__(
        self,
        algorithm,
        mode,
        length,
        rlen,
        llen,
        location,
        label,
        context,
        fixed,
        backend=None,
    ):
        backend = _get_backend(backend)
        if not isinstance(backend, HMACBackend):
            raise UnsupportedAlgorithm(
                "Backend object does not implement HMACBackend.",
                _Reasons.BACKEND_MISSING_INTERFACE,
            )

        if not isinstance(algorithm, hashes.HashAlgorithm):
            raise UnsupportedAlgorithm(
                "Algorithm supplied is not a supported hash algorithm.",
                _Reasons.UNSUPPORTED_HASH,
            )

        if not backend.hmac_supported(algorithm):
            raise UnsupportedAlgorithm(
                "Algorithm supplied is not a supported hmac algorithm.",
                _Reasons.UNSUPPORTED_HASH,
            )

        if not isinstance(mode, Mode):
            raise TypeError("mode must be of type Mode")

        if not isinstance(location, CounterLocation):
            raise TypeError("location must be of type CounterLocation")

        if (label or context) and fixed:
            raise ValueError(
                "When supplying fixed data, " "label and context are ignored."
            )

        if rlen is None or not self._valid_byte_length(rlen):
            raise ValueError("rlen must be between 1 and 4")

        if llen is None and fixed is None:
            raise ValueError("Please specify an llen")

        if llen is not None and not isinstance(llen, int):
            raise TypeError("llen must be an integer")

        if label is None:
            label = b""

        if context is None:
            context = b""

        utils._check_bytes("label", label)
        utils._check_bytes("context", context)
        self._algorithm = algorithm
        self._mode = mode
        self._length = length
        self._rlen = rlen
        self._llen = llen
        self._location = location
        self._label = label
        self._context = context
        self._backend = backend
        self._used = False
        self._fixed_data = fixed

    def _valid_byte_length(self, value):
        if not isinstance(value, int):
            raise TypeError("value must be of type int")

        value_bin = utils.int_to_bytes(1, value)
        if not 1 <= len(value_bin) <= 4:
            return False
        return True

    def derive(self, key_material):
        if self._used:
            raise AlreadyFinalized

        utils._check_byteslike("key_material", key_material)
        self._used = True

        # inverse floor division (equivalent to ceiling)
        rounds = -(-self._length // self._algorithm.digest_size)

        output = [b""]

        # For counter mode, the number of iterations shall not be
        # larger than 2^r-1, where r <= 32 is the binary length of the counter
        # This ensures that the counter values used as an input to the
        # PRF will not repeat during a particular call to the KDF function.
        r_bin = utils.int_to_bytes(1, self._rlen)
        if rounds > pow(2, len(r_bin) * 8) - 1:
            raise ValueError("There are too many iterations.")

        for i in range(1, rounds + 1):
            h = hmac.HMAC(key_material, self._algorithm, backend=self._backend)

            counter = utils.int_to_bytes(i, self._rlen)
            if self._location == CounterLocation.BeforeFixed:
                h.update(counter)

            h.update(self._generate_fixed_input())

            if self._location == CounterLocation.AfterFixed:
                h.update(counter)

            output.append(h.finalize())

        return b"".join(output)[: self._length]

    def _generate_fixed_input(self):
        if self._fixed_data and isinstance(self._fixed_data, bytes):
            return self._fixed_data

        l_val = utils.int_to_bytes(self._length * 8, self._llen)

        return b"".join([self._label, b"\x00", self._context, l_val])

    def verify(self, key_material, expected_key):
        if not constant_time.bytes_eq(self.derive(key_material), expected_key):
            raise InvalidKey
