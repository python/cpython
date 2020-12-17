# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

from cryptography import utils
from cryptography.exceptions import (
    AlreadyFinalized,
    UnsupportedAlgorithm,
    _Reasons,
)
from cryptography.hazmat.backends import _get_backend
from cryptography.hazmat.backends.interfaces import HMACBackend
from cryptography.hazmat.primitives import hashes


@utils.register_interface(hashes.HashContext)
class HMAC(object):
    def __init__(self, key, algorithm, backend=None, ctx=None):
        backend = _get_backend(backend)
        if not isinstance(backend, HMACBackend):
            raise UnsupportedAlgorithm(
                "Backend object does not implement HMACBackend.",
                _Reasons.BACKEND_MISSING_INTERFACE,
            )

        if not isinstance(algorithm, hashes.HashAlgorithm):
            raise TypeError("Expected instance of hashes.HashAlgorithm.")
        self._algorithm = algorithm

        self._backend = backend
        self._key = key
        if ctx is None:
            self._ctx = self._backend.create_hmac_ctx(key, self.algorithm)
        else:
            self._ctx = ctx

    algorithm = utils.read_only_property("_algorithm")

    def update(self, data):
        if self._ctx is None:
            raise AlreadyFinalized("Context was already finalized.")
        utils._check_byteslike("data", data)
        self._ctx.update(data)

    def copy(self):
        if self._ctx is None:
            raise AlreadyFinalized("Context was already finalized.")
        return HMAC(
            self._key,
            self.algorithm,
            backend=self._backend,
            ctx=self._ctx.copy(),
        )

    def finalize(self):
        if self._ctx is None:
            raise AlreadyFinalized("Context was already finalized.")
        digest = self._ctx.finalize()
        self._ctx = None
        return digest

    def verify(self, signature):
        utils._check_bytes("signature", signature)
        if self._ctx is None:
            raise AlreadyFinalized("Context was already finalized.")

        ctx, self._ctx = self._ctx, None
        ctx.verify(signature)
