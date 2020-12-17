# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc

import six

from cryptography import utils
from cryptography.exceptions import AlreadyFinalized
from cryptography.hazmat.bindings._padding import lib


@six.add_metaclass(abc.ABCMeta)
class PaddingContext(object):
    @abc.abstractmethod
    def update(self, data):
        """
        Pads the provided bytes and returns any available data as bytes.
        """

    @abc.abstractmethod
    def finalize(self):
        """
        Finalize the padding, returns bytes.
        """


def _byte_padding_check(block_size):
    if not (0 <= block_size <= 2040):
        raise ValueError("block_size must be in range(0, 2041).")

    if block_size % 8 != 0:
        raise ValueError("block_size must be a multiple of 8.")


def _byte_padding_update(buffer_, data, block_size):
    if buffer_ is None:
        raise AlreadyFinalized("Context was already finalized.")

    utils._check_byteslike("data", data)

    # six.PY2: Only coerce non-bytes objects to avoid triggering bad behavior
    # of future's newbytes type. Unconditionally call bytes() after Python 2
    # support is gone.
    buffer_ += data if isinstance(data, bytes) else bytes(data)

    finished_blocks = len(buffer_) // (block_size // 8)

    result = buffer_[: finished_blocks * (block_size // 8)]
    buffer_ = buffer_[finished_blocks * (block_size // 8) :]

    return buffer_, result


def _byte_padding_pad(buffer_, block_size, paddingfn):
    if buffer_ is None:
        raise AlreadyFinalized("Context was already finalized.")

    pad_size = block_size // 8 - len(buffer_)
    return buffer_ + paddingfn(pad_size)


def _byte_unpadding_update(buffer_, data, block_size):
    if buffer_ is None:
        raise AlreadyFinalized("Context was already finalized.")

    utils._check_byteslike("data", data)

    # six.PY2: Only coerce non-bytes objects to avoid triggering bad behavior
    # of future's newbytes type. Unconditionally call bytes() after Python 2
    # support is gone.
    buffer_ += data if isinstance(data, bytes) else bytes(data)

    finished_blocks = max(len(buffer_) // (block_size // 8) - 1, 0)

    result = buffer_[: finished_blocks * (block_size // 8)]
    buffer_ = buffer_[finished_blocks * (block_size // 8) :]

    return buffer_, result


def _byte_unpadding_check(buffer_, block_size, checkfn):
    if buffer_ is None:
        raise AlreadyFinalized("Context was already finalized.")

    if len(buffer_) != block_size // 8:
        raise ValueError("Invalid padding bytes.")

    valid = checkfn(buffer_, block_size // 8)

    if not valid:
        raise ValueError("Invalid padding bytes.")

    pad_size = six.indexbytes(buffer_, -1)
    return buffer_[:-pad_size]


class PKCS7(object):
    def __init__(self, block_size):
        _byte_padding_check(block_size)
        self.block_size = block_size

    def padder(self):
        return _PKCS7PaddingContext(self.block_size)

    def unpadder(self):
        return _PKCS7UnpaddingContext(self.block_size)


@utils.register_interface(PaddingContext)
class _PKCS7PaddingContext(object):
    def __init__(self, block_size):
        self.block_size = block_size
        # TODO: more copies than necessary, we should use zero-buffer (#193)
        self._buffer = b""

    def update(self, data):
        self._buffer, result = _byte_padding_update(
            self._buffer, data, self.block_size
        )
        return result

    def _padding(self, size):
        return six.int2byte(size) * size

    def finalize(self):
        result = _byte_padding_pad(
            self._buffer, self.block_size, self._padding
        )
        self._buffer = None
        return result


@utils.register_interface(PaddingContext)
class _PKCS7UnpaddingContext(object):
    def __init__(self, block_size):
        self.block_size = block_size
        # TODO: more copies than necessary, we should use zero-buffer (#193)
        self._buffer = b""

    def update(self, data):
        self._buffer, result = _byte_unpadding_update(
            self._buffer, data, self.block_size
        )
        return result

    def finalize(self):
        result = _byte_unpadding_check(
            self._buffer, self.block_size, lib.Cryptography_check_pkcs7_padding
        )
        self._buffer = None
        return result


class ANSIX923(object):
    def __init__(self, block_size):
        _byte_padding_check(block_size)
        self.block_size = block_size

    def padder(self):
        return _ANSIX923PaddingContext(self.block_size)

    def unpadder(self):
        return _ANSIX923UnpaddingContext(self.block_size)


@utils.register_interface(PaddingContext)
class _ANSIX923PaddingContext(object):
    def __init__(self, block_size):
        self.block_size = block_size
        # TODO: more copies than necessary, we should use zero-buffer (#193)
        self._buffer = b""

    def update(self, data):
        self._buffer, result = _byte_padding_update(
            self._buffer, data, self.block_size
        )
        return result

    def _padding(self, size):
        return six.int2byte(0) * (size - 1) + six.int2byte(size)

    def finalize(self):
        result = _byte_padding_pad(
            self._buffer, self.block_size, self._padding
        )
        self._buffer = None
        return result


@utils.register_interface(PaddingContext)
class _ANSIX923UnpaddingContext(object):
    def __init__(self, block_size):
        self.block_size = block_size
        # TODO: more copies than necessary, we should use zero-buffer (#193)
        self._buffer = b""

    def update(self, data):
        self._buffer, result = _byte_unpadding_update(
            self._buffer, data, self.block_size
        )
        return result

    def finalize(self):
        result = _byte_unpadding_check(
            self._buffer,
            self.block_size,
            lib.Cryptography_check_ansix923_padding,
        )
        self._buffer = None
        return result
