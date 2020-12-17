# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

from cryptography.exceptions import UnsupportedAlgorithm, _Reasons
from cryptography.hazmat.backends import _get_backend
from cryptography.hazmat.backends.interfaces import HMACBackend
from cryptography.hazmat.primitives import constant_time
from cryptography.hazmat.primitives.twofactor import InvalidToken
from cryptography.hazmat.primitives.twofactor.hotp import HOTP
from cryptography.hazmat.primitives.twofactor.utils import _generate_uri


class TOTP(object):
    def __init__(
        self,
        key,
        length,
        algorithm,
        time_step,
        backend=None,
        enforce_key_length=True,
    ):
        backend = _get_backend(backend)
        if not isinstance(backend, HMACBackend):
            raise UnsupportedAlgorithm(
                "Backend object does not implement HMACBackend.",
                _Reasons.BACKEND_MISSING_INTERFACE,
            )

        self._time_step = time_step
        self._hotp = HOTP(key, length, algorithm, backend, enforce_key_length)

    def generate(self, time):
        counter = int(time / self._time_step)
        return self._hotp.generate(counter)

    def verify(self, totp, time):
        if not constant_time.bytes_eq(self.generate(time), totp):
            raise InvalidToken("Supplied TOTP value does not match.")

    def get_provisioning_uri(self, account_name, issuer):
        return _generate_uri(
            self._hotp,
            "totp",
            account_name,
            issuer,
            [("period", int(self._time_step))],
        )
