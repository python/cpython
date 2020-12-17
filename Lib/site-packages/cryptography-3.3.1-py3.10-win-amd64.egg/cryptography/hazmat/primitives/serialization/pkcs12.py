# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

from cryptography import x509
from cryptography.hazmat.backends import _get_backend
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import dsa, ec, rsa


def load_key_and_certificates(data, password, backend=None):
    backend = _get_backend(backend)
    return backend.load_key_and_certificates_from_pkcs12(data, password)


def serialize_key_and_certificates(name, key, cert, cas, encryption_algorithm):
    if key is not None and not isinstance(
        key,
        (
            rsa.RSAPrivateKeyWithSerialization,
            dsa.DSAPrivateKeyWithSerialization,
            ec.EllipticCurvePrivateKeyWithSerialization,
        ),
    ):
        raise TypeError("Key must be RSA, DSA, or EllipticCurve private key.")
    if cert is not None and not isinstance(cert, x509.Certificate):
        raise TypeError("cert must be a certificate")

    if cas is not None:
        cas = list(cas)
        if not all(isinstance(val, x509.Certificate) for val in cas):
            raise TypeError("all values in cas must be certificates")

    if not isinstance(
        encryption_algorithm, serialization.KeySerializationEncryption
    ):
        raise TypeError(
            "Key encryption algorithm must be a "
            "KeySerializationEncryption instance"
        )

    if key is None and cert is None and not cas:
        raise ValueError("You must supply at least one of key, cert, or cas")

    backend = _get_backend(None)
    return backend.serialize_key_and_certificates_to_pkcs12(
        name, key, cert, cas, encryption_algorithm
    )
