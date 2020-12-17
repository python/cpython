# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import base64
import binascii
import os
import struct
import time

import six

from cryptography import utils
from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.backends import _get_backend
from cryptography.hazmat.primitives import hashes, padding
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.hmac import HMAC


class InvalidToken(Exception):
    pass


_MAX_CLOCK_SKEW = 60


class Fernet(object):
    def __init__(self, key, backend=None):
        backend = _get_backend(backend)

        key = base64.urlsafe_b64decode(key)
        if len(key) != 32:
            raise ValueError(
                "Fernet key must be 32 url-safe base64-encoded bytes."
            )

        self._signing_key = key[:16]
        self._encryption_key = key[16:]
        self._backend = backend

    @classmethod
    def generate_key(cls):
        return base64.urlsafe_b64encode(os.urandom(32))

    def encrypt(self, data):
        return self.encrypt_at_time(data, int(time.time()))

    def encrypt_at_time(self, data, current_time):
        iv = os.urandom(16)
        return self._encrypt_from_parts(data, current_time, iv)

    def _encrypt_from_parts(self, data, current_time, iv):
        utils._check_bytes("data", data)

        padder = padding.PKCS7(algorithms.AES.block_size).padder()
        padded_data = padder.update(data) + padder.finalize()
        encryptor = Cipher(
            algorithms.AES(self._encryption_key), modes.CBC(iv), self._backend
        ).encryptor()
        ciphertext = encryptor.update(padded_data) + encryptor.finalize()

        basic_parts = (
            b"\x80" + struct.pack(">Q", current_time) + iv + ciphertext
        )

        h = HMAC(self._signing_key, hashes.SHA256(), backend=self._backend)
        h.update(basic_parts)
        hmac = h.finalize()
        return base64.urlsafe_b64encode(basic_parts + hmac)

    def decrypt(self, token, ttl=None):
        timestamp, data = Fernet._get_unverified_token_data(token)
        return self._decrypt_data(data, timestamp, ttl, int(time.time()))

    def decrypt_at_time(self, token, ttl, current_time):
        if ttl is None:
            raise ValueError(
                "decrypt_at_time() can only be used with a non-None ttl"
            )
        timestamp, data = Fernet._get_unverified_token_data(token)
        return self._decrypt_data(data, timestamp, ttl, current_time)

    def extract_timestamp(self, token):
        timestamp, data = Fernet._get_unverified_token_data(token)
        # Verify the token was not tampered with.
        self._verify_signature(data)
        return timestamp

    @staticmethod
    def _get_unverified_token_data(token):
        utils._check_bytes("token", token)
        try:
            data = base64.urlsafe_b64decode(token)
        except (TypeError, binascii.Error):
            raise InvalidToken

        if not data or six.indexbytes(data, 0) != 0x80:
            raise InvalidToken

        try:
            (timestamp,) = struct.unpack(">Q", data[1:9])
        except struct.error:
            raise InvalidToken
        return timestamp, data

    def _verify_signature(self, data):
        h = HMAC(self._signing_key, hashes.SHA256(), backend=self._backend)
        h.update(data[:-32])
        try:
            h.verify(data[-32:])
        except InvalidSignature:
            raise InvalidToken

    def _decrypt_data(self, data, timestamp, ttl, current_time):
        if ttl is not None:
            if timestamp + ttl < current_time:
                raise InvalidToken

            if current_time + _MAX_CLOCK_SKEW < timestamp:
                raise InvalidToken

        self._verify_signature(data)

        iv = data[9:25]
        ciphertext = data[25:-32]
        decryptor = Cipher(
            algorithms.AES(self._encryption_key), modes.CBC(iv), self._backend
        ).decryptor()
        plaintext_padded = decryptor.update(ciphertext)
        try:
            plaintext_padded += decryptor.finalize()
        except ValueError:
            raise InvalidToken
        unpadder = padding.PKCS7(algorithms.AES.block_size).unpadder()

        unpadded = unpadder.update(plaintext_padded)
        try:
            unpadded += unpadder.finalize()
        except ValueError:
            raise InvalidToken
        return unpadded


class MultiFernet(object):
    def __init__(self, fernets):
        fernets = list(fernets)
        if not fernets:
            raise ValueError(
                "MultiFernet requires at least one Fernet instance"
            )
        self._fernets = fernets

    def encrypt(self, msg):
        return self.encrypt_at_time(msg, int(time.time()))

    def encrypt_at_time(self, msg, current_time):
        return self._fernets[0].encrypt_at_time(msg, current_time)

    def rotate(self, msg):
        timestamp, data = Fernet._get_unverified_token_data(msg)
        for f in self._fernets:
            try:
                p = f._decrypt_data(data, timestamp, None, None)
                break
            except InvalidToken:
                pass
        else:
            raise InvalidToken

        iv = os.urandom(16)
        return self._fernets[0]._encrypt_from_parts(p, timestamp, iv)

    def decrypt(self, msg, ttl=None):
        for f in self._fernets:
            try:
                return f.decrypt(msg, ttl)
            except InvalidToken:
                pass
        raise InvalidToken

    def decrypt_at_time(self, msg, ttl, current_time):
        for f in self._fernets:
            try:
                return f.decrypt_at_time(msg, ttl, current_time)
            except InvalidToken:
                pass
        raise InvalidToken
