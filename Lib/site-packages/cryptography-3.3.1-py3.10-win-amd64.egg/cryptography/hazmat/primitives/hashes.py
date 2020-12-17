# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc

import six

from cryptography import utils
from cryptography.exceptions import (
    AlreadyFinalized,
    UnsupportedAlgorithm,
    _Reasons,
)
from cryptography.hazmat.backends import _get_backend
from cryptography.hazmat.backends.interfaces import HashBackend


@six.add_metaclass(abc.ABCMeta)
class HashAlgorithm(object):
    @abc.abstractproperty
    def name(self):
        """
        A string naming this algorithm (e.g. "sha256", "md5").
        """

    @abc.abstractproperty
    def digest_size(self):
        """
        The size of the resulting digest in bytes.
        """


@six.add_metaclass(abc.ABCMeta)
class HashContext(object):
    @abc.abstractproperty
    def algorithm(self):
        """
        A HashAlgorithm that will be used by this context.
        """

    @abc.abstractmethod
    def update(self, data):
        """
        Processes the provided bytes through the hash.
        """

    @abc.abstractmethod
    def finalize(self):
        """
        Finalizes the hash context and returns the hash digest as bytes.
        """

    @abc.abstractmethod
    def copy(self):
        """
        Return a HashContext that is a copy of the current context.
        """


@six.add_metaclass(abc.ABCMeta)
class ExtendableOutputFunction(object):
    """
    An interface for extendable output functions.
    """


@utils.register_interface(HashContext)
class Hash(object):
    def __init__(self, algorithm, backend=None, ctx=None):
        backend = _get_backend(backend)
        if not isinstance(backend, HashBackend):
            raise UnsupportedAlgorithm(
                "Backend object does not implement HashBackend.",
                _Reasons.BACKEND_MISSING_INTERFACE,
            )

        if not isinstance(algorithm, HashAlgorithm):
            raise TypeError("Expected instance of hashes.HashAlgorithm.")
        self._algorithm = algorithm

        self._backend = backend

        if ctx is None:
            self._ctx = self._backend.create_hash_ctx(self.algorithm)
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
        return Hash(
            self.algorithm, backend=self._backend, ctx=self._ctx.copy()
        )

    def finalize(self):
        if self._ctx is None:
            raise AlreadyFinalized("Context was already finalized.")
        digest = self._ctx.finalize()
        self._ctx = None
        return digest


@utils.register_interface(HashAlgorithm)
class SHA1(object):
    name = "sha1"
    digest_size = 20
    block_size = 64


@utils.register_interface(HashAlgorithm)
class SHA512_224(object):  # noqa: N801
    name = "sha512-224"
    digest_size = 28
    block_size = 128


@utils.register_interface(HashAlgorithm)
class SHA512_256(object):  # noqa: N801
    name = "sha512-256"
    digest_size = 32
    block_size = 128


@utils.register_interface(HashAlgorithm)
class SHA224(object):
    name = "sha224"
    digest_size = 28
    block_size = 64


@utils.register_interface(HashAlgorithm)
class SHA256(object):
    name = "sha256"
    digest_size = 32
    block_size = 64


@utils.register_interface(HashAlgorithm)
class SHA384(object):
    name = "sha384"
    digest_size = 48
    block_size = 128


@utils.register_interface(HashAlgorithm)
class SHA512(object):
    name = "sha512"
    digest_size = 64
    block_size = 128


@utils.register_interface(HashAlgorithm)
class SHA3_224(object):  # noqa: N801
    name = "sha3-224"
    digest_size = 28


@utils.register_interface(HashAlgorithm)
class SHA3_256(object):  # noqa: N801
    name = "sha3-256"
    digest_size = 32


@utils.register_interface(HashAlgorithm)
class SHA3_384(object):  # noqa: N801
    name = "sha3-384"
    digest_size = 48


@utils.register_interface(HashAlgorithm)
class SHA3_512(object):  # noqa: N801
    name = "sha3-512"
    digest_size = 64


@utils.register_interface(HashAlgorithm)
@utils.register_interface(ExtendableOutputFunction)
class SHAKE128(object):
    name = "shake128"

    def __init__(self, digest_size):
        if not isinstance(digest_size, six.integer_types):
            raise TypeError("digest_size must be an integer")

        if digest_size < 1:
            raise ValueError("digest_size must be a positive integer")

        self._digest_size = digest_size

    digest_size = utils.read_only_property("_digest_size")


@utils.register_interface(HashAlgorithm)
@utils.register_interface(ExtendableOutputFunction)
class SHAKE256(object):
    name = "shake256"

    def __init__(self, digest_size):
        if not isinstance(digest_size, six.integer_types):
            raise TypeError("digest_size must be an integer")

        if digest_size < 1:
            raise ValueError("digest_size must be a positive integer")

        self._digest_size = digest_size

    digest_size = utils.read_only_property("_digest_size")


@utils.register_interface(HashAlgorithm)
class MD5(object):
    name = "md5"
    digest_size = 16
    block_size = 64


@utils.register_interface(HashAlgorithm)
class BLAKE2b(object):
    name = "blake2b"
    _max_digest_size = 64
    _min_digest_size = 1
    block_size = 128

    def __init__(self, digest_size):

        if digest_size != 64:
            raise ValueError("Digest size must be 64")

        self._digest_size = digest_size

    digest_size = utils.read_only_property("_digest_size")


@utils.register_interface(HashAlgorithm)
class BLAKE2s(object):
    name = "blake2s"
    block_size = 64
    _max_digest_size = 32
    _min_digest_size = 1

    def __init__(self, digest_size):

        if digest_size != 32:
            raise ValueError("Digest size must be 32")

        self._digest_size = digest_size

    digest_size = utils.read_only_property("_digest_size")
