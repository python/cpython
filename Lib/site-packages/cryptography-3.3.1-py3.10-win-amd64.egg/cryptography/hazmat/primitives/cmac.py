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
from cryptography.hazmat.backends.interfaces import CMACBackend
from cryptography.hazmat.primitives import ciphers


class CMAC(object):
    def __init__(self, algorithm, backend=None, ctx=None):
        backend = _get_backend(backend)
        if not isinstance(backend, CMACBackend):
            raise UnsupportedAlgorithm(
                "Backend object does not implement CMACBackend.",
                _Reasons.BACKEND_MISSING_INTERFACE,
            )

        if not isinstance(algorithm, ciphers.BlockCipherAlgorithm):
            raise TypeError("Expected instance of BlockCipherAlgorithm.")
        self._algorithm = algorithm

        self._backend = backend
        if ctx is None:
            self._ctx = self._backend.create_cmac_ctx(self._algorithm)
        else:
            self._ctx = ctx

    def update(self, data):
        if self._ctx is None:
            raise AlreadyFinalized("Context was already finalized.")

        utils._check_bytes("data", data)
        self._ctx.update(data)

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

    def copy(self):
        if self._ctx is None:
            raise AlreadyFinalized("Context was already finalized.")
        return CMAC(
            self._algorithm, backend=self._backend, ctx=self._ctx.copy()
        )
