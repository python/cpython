# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc

import six


@six.add_metaclass(abc.ABCMeta)
class CipherBackend(object):
    @abc.abstractmethod
    def cipher_supported(self, cipher, mode):
        """
        Return True if the given cipher and mode are supported.
        """

    @abc.abstractmethod
    def create_symmetric_encryption_ctx(self, cipher, mode):
        """
        Get a CipherContext that can be used for encryption.
        """

    @abc.abstractmethod
    def create_symmetric_decryption_ctx(self, cipher, mode):
        """
        Get a CipherContext that can be used for decryption.
        """


@six.add_metaclass(abc.ABCMeta)
class HashBackend(object):
    @abc.abstractmethod
    def hash_supported(self, algorithm):
        """
        Return True if the hash algorithm is supported by this backend.
        """

    @abc.abstractmethod
    def create_hash_ctx(self, algorithm):
        """
        Create a HashContext for calculating a message digest.
        """


@six.add_metaclass(abc.ABCMeta)
class HMACBackend(object):
    @abc.abstractmethod
    def hmac_supported(self, algorithm):
        """
        Return True if the hash algorithm is supported for HMAC by this
        backend.
        """

    @abc.abstractmethod
    def create_hmac_ctx(self, key, algorithm):
        """
        Create a context for calculating a message authentication code.
        """


@six.add_metaclass(abc.ABCMeta)
class CMACBackend(object):
    @abc.abstractmethod
    def cmac_algorithm_supported(self, algorithm):
        """
        Returns True if the block cipher is supported for CMAC by this backend
        """

    @abc.abstractmethod
    def create_cmac_ctx(self, algorithm):
        """
        Create a context for calculating a message authentication code.
        """


@six.add_metaclass(abc.ABCMeta)
class PBKDF2HMACBackend(object):
    @abc.abstractmethod
    def pbkdf2_hmac_supported(self, algorithm):
        """
        Return True if the hash algorithm is supported for PBKDF2 by this
        backend.
        """

    @abc.abstractmethod
    def derive_pbkdf2_hmac(
        self, algorithm, length, salt, iterations, key_material
    ):
        """
        Return length bytes derived from provided PBKDF2 parameters.
        """


@six.add_metaclass(abc.ABCMeta)
class RSABackend(object):
    @abc.abstractmethod
    def generate_rsa_private_key(self, public_exponent, key_size):
        """
        Generate an RSAPrivateKey instance with public_exponent and a modulus
        of key_size bits.
        """

    @abc.abstractmethod
    def rsa_padding_supported(self, padding):
        """
        Returns True if the backend supports the given padding options.
        """

    @abc.abstractmethod
    def generate_rsa_parameters_supported(self, public_exponent, key_size):
        """
        Returns True if the backend supports the given parameters for key
        generation.
        """

    @abc.abstractmethod
    def load_rsa_private_numbers(self, numbers):
        """
        Returns an RSAPrivateKey provider.
        """

    @abc.abstractmethod
    def load_rsa_public_numbers(self, numbers):
        """
        Returns an RSAPublicKey provider.
        """


@six.add_metaclass(abc.ABCMeta)
class DSABackend(object):
    @abc.abstractmethod
    def generate_dsa_parameters(self, key_size):
        """
        Generate a DSAParameters instance with a modulus of key_size bits.
        """

    @abc.abstractmethod
    def generate_dsa_private_key(self, parameters):
        """
        Generate a DSAPrivateKey instance with parameters as a DSAParameters
        object.
        """

    @abc.abstractmethod
    def generate_dsa_private_key_and_parameters(self, key_size):
        """
        Generate a DSAPrivateKey instance using key size only.
        """

    @abc.abstractmethod
    def dsa_hash_supported(self, algorithm):
        """
        Return True if the hash algorithm is supported by the backend for DSA.
        """

    @abc.abstractmethod
    def dsa_parameters_supported(self, p, q, g):
        """
        Return True if the parameters are supported by the backend for DSA.
        """

    @abc.abstractmethod
    def load_dsa_private_numbers(self, numbers):
        """
        Returns a DSAPrivateKey provider.
        """

    @abc.abstractmethod
    def load_dsa_public_numbers(self, numbers):
        """
        Returns a DSAPublicKey provider.
        """

    @abc.abstractmethod
    def load_dsa_parameter_numbers(self, numbers):
        """
        Returns a DSAParameters provider.
        """


@six.add_metaclass(abc.ABCMeta)
class EllipticCurveBackend(object):
    @abc.abstractmethod
    def elliptic_curve_signature_algorithm_supported(
        self, signature_algorithm, curve
    ):
        """
        Returns True if the backend supports the named elliptic curve with the
        specified signature algorithm.
        """

    @abc.abstractmethod
    def elliptic_curve_supported(self, curve):
        """
        Returns True if the backend supports the named elliptic curve.
        """

    @abc.abstractmethod
    def generate_elliptic_curve_private_key(self, curve):
        """
        Return an object conforming to the EllipticCurvePrivateKey interface.
        """

    @abc.abstractmethod
    def load_elliptic_curve_public_numbers(self, numbers):
        """
        Return an EllipticCurvePublicKey provider using the given numbers.
        """

    @abc.abstractmethod
    def load_elliptic_curve_private_numbers(self, numbers):
        """
        Return an EllipticCurvePrivateKey provider using the given numbers.
        """

    @abc.abstractmethod
    def elliptic_curve_exchange_algorithm_supported(self, algorithm, curve):
        """
        Returns whether the exchange algorithm is supported by this backend.
        """

    @abc.abstractmethod
    def derive_elliptic_curve_private_key(self, private_value, curve):
        """
        Compute the private key given the private value and curve.
        """


@six.add_metaclass(abc.ABCMeta)
class PEMSerializationBackend(object):
    @abc.abstractmethod
    def load_pem_private_key(self, data, password):
        """
        Loads a private key from PEM encoded data, using the provided password
        if the data is encrypted.
        """

    @abc.abstractmethod
    def load_pem_public_key(self, data):
        """
        Loads a public key from PEM encoded data.
        """

    @abc.abstractmethod
    def load_pem_parameters(self, data):
        """
        Load encryption parameters from PEM encoded data.
        """


@six.add_metaclass(abc.ABCMeta)
class DERSerializationBackend(object):
    @abc.abstractmethod
    def load_der_private_key(self, data, password):
        """
        Loads a private key from DER encoded data. Uses the provided password
        if the data is encrypted.
        """

    @abc.abstractmethod
    def load_der_public_key(self, data):
        """
        Loads a public key from DER encoded data.
        """

    @abc.abstractmethod
    def load_der_parameters(self, data):
        """
        Load encryption parameters from DER encoded data.
        """


@six.add_metaclass(abc.ABCMeta)
class X509Backend(object):
    @abc.abstractmethod
    def load_pem_x509_certificate(self, data):
        """
        Load an X.509 certificate from PEM encoded data.
        """

    @abc.abstractmethod
    def load_der_x509_certificate(self, data):
        """
        Load an X.509 certificate from DER encoded data.
        """

    @abc.abstractmethod
    def load_der_x509_csr(self, data):
        """
        Load an X.509 CSR from DER encoded data.
        """

    @abc.abstractmethod
    def load_pem_x509_csr(self, data):
        """
        Load an X.509 CSR from PEM encoded data.
        """

    @abc.abstractmethod
    def create_x509_csr(self, builder, private_key, algorithm):
        """
        Create and sign an X.509 CSR from a CSR builder object.
        """

    @abc.abstractmethod
    def create_x509_certificate(self, builder, private_key, algorithm):
        """
        Create and sign an X.509 certificate from a CertificateBuilder object.
        """

    @abc.abstractmethod
    def create_x509_crl(self, builder, private_key, algorithm):
        """
        Create and sign an X.509 CertificateRevocationList from a
        CertificateRevocationListBuilder object.
        """

    @abc.abstractmethod
    def create_x509_revoked_certificate(self, builder):
        """
        Create a RevokedCertificate object from a RevokedCertificateBuilder
        object.
        """

    @abc.abstractmethod
    def x509_name_bytes(self, name):
        """
        Compute the DER encoded bytes of an X509 Name object.
        """


@six.add_metaclass(abc.ABCMeta)
class DHBackend(object):
    @abc.abstractmethod
    def generate_dh_parameters(self, generator, key_size):
        """
        Generate a DHParameters instance with a modulus of key_size bits.
        Using the given generator. Often 2 or 5.
        """

    @abc.abstractmethod
    def generate_dh_private_key(self, parameters):
        """
        Generate a DHPrivateKey instance with parameters as a DHParameters
        object.
        """

    @abc.abstractmethod
    def generate_dh_private_key_and_parameters(self, generator, key_size):
        """
        Generate a DHPrivateKey instance using key size only.
        Using the given generator. Often 2 or 5.
        """

    @abc.abstractmethod
    def load_dh_private_numbers(self, numbers):
        """
        Load a DHPrivateKey from DHPrivateNumbers
        """

    @abc.abstractmethod
    def load_dh_public_numbers(self, numbers):
        """
        Load a DHPublicKey from DHPublicNumbers.
        """

    @abc.abstractmethod
    def load_dh_parameter_numbers(self, numbers):
        """
        Load DHParameters from DHParameterNumbers.
        """

    @abc.abstractmethod
    def dh_parameters_supported(self, p, g, q=None):
        """
        Returns whether the backend supports DH with these parameter values.
        """

    @abc.abstractmethod
    def dh_x942_serialization_supported(self):
        """
        Returns True if the backend supports the serialization of DH objects
        with subgroup order (q).
        """


@six.add_metaclass(abc.ABCMeta)
class ScryptBackend(object):
    @abc.abstractmethod
    def derive_scrypt(self, key_material, salt, length, n, r, p):
        """
        Return bytes derived from provided Scrypt parameters.
        """
