# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc
from enum import Enum

import six

from cryptography import utils
from cryptography.hazmat.backends import _get_backend


def load_pem_private_key(data, password, backend=None):
    backend = _get_backend(backend)
    return backend.load_pem_private_key(data, password)


def load_pem_public_key(data, backend=None):
    backend = _get_backend(backend)
    return backend.load_pem_public_key(data)


def load_pem_parameters(data, backend=None):
    backend = _get_backend(backend)
    return backend.load_pem_parameters(data)


def load_der_private_key(data, password, backend=None):
    backend = _get_backend(backend)
    return backend.load_der_private_key(data, password)


def load_der_public_key(data, backend=None):
    backend = _get_backend(backend)
    return backend.load_der_public_key(data)


def load_der_parameters(data, backend=None):
    backend = _get_backend(backend)
    return backend.load_der_parameters(data)


class Encoding(Enum):
    PEM = "PEM"
    DER = "DER"
    OpenSSH = "OpenSSH"
    Raw = "Raw"
    X962 = "ANSI X9.62"
    SMIME = "S/MIME"


class PrivateFormat(Enum):
    PKCS8 = "PKCS8"
    TraditionalOpenSSL = "TraditionalOpenSSL"
    Raw = "Raw"
    OpenSSH = "OpenSSH"


class PublicFormat(Enum):
    SubjectPublicKeyInfo = "X.509 subjectPublicKeyInfo with PKCS#1"
    PKCS1 = "Raw PKCS#1"
    OpenSSH = "OpenSSH"
    Raw = "Raw"
    CompressedPoint = "X9.62 Compressed Point"
    UncompressedPoint = "X9.62 Uncompressed Point"


class ParameterFormat(Enum):
    PKCS3 = "PKCS3"


@six.add_metaclass(abc.ABCMeta)
class KeySerializationEncryption(object):
    pass


@utils.register_interface(KeySerializationEncryption)
class BestAvailableEncryption(object):
    def __init__(self, password):
        if not isinstance(password, bytes) or len(password) == 0:
            raise ValueError("Password must be 1 or more bytes.")

        self.password = password


@utils.register_interface(KeySerializationEncryption)
class NoEncryption(object):
    pass
