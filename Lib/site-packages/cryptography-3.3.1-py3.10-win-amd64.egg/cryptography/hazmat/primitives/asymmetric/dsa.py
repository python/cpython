# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc

import six

from cryptography import utils
from cryptography.hazmat.backends import _get_backend


@six.add_metaclass(abc.ABCMeta)
class DSAParameters(object):
    @abc.abstractmethod
    def generate_private_key(self):
        """
        Generates and returns a DSAPrivateKey.
        """


@six.add_metaclass(abc.ABCMeta)
class DSAParametersWithNumbers(DSAParameters):
    @abc.abstractmethod
    def parameter_numbers(self):
        """
        Returns a DSAParameterNumbers.
        """


@six.add_metaclass(abc.ABCMeta)
class DSAPrivateKey(object):
    @abc.abstractproperty
    def key_size(self):
        """
        The bit length of the prime modulus.
        """

    @abc.abstractmethod
    def public_key(self):
        """
        The DSAPublicKey associated with this private key.
        """

    @abc.abstractmethod
    def parameters(self):
        """
        The DSAParameters object associated with this private key.
        """

    @abc.abstractmethod
    def signer(self, signature_algorithm):
        """
        Returns an AsymmetricSignatureContext used for signing data.
        """

    @abc.abstractmethod
    def sign(self, data, algorithm):
        """
        Signs the data
        """


@six.add_metaclass(abc.ABCMeta)
class DSAPrivateKeyWithSerialization(DSAPrivateKey):
    @abc.abstractmethod
    def private_numbers(self):
        """
        Returns a DSAPrivateNumbers.
        """

    @abc.abstractmethod
    def private_bytes(self, encoding, format, encryption_algorithm):
        """
        Returns the key serialized as bytes.
        """


@six.add_metaclass(abc.ABCMeta)
class DSAPublicKey(object):
    @abc.abstractproperty
    def key_size(self):
        """
        The bit length of the prime modulus.
        """

    @abc.abstractmethod
    def parameters(self):
        """
        The DSAParameters object associated with this public key.
        """

    @abc.abstractmethod
    def verifier(self, signature, signature_algorithm):
        """
        Returns an AsymmetricVerificationContext used for signing data.
        """

    @abc.abstractmethod
    def public_numbers(self):
        """
        Returns a DSAPublicNumbers.
        """

    @abc.abstractmethod
    def public_bytes(self, encoding, format):
        """
        Returns the key serialized as bytes.
        """

    @abc.abstractmethod
    def verify(self, signature, data, algorithm):
        """
        Verifies the signature of the data.
        """


DSAPublicKeyWithSerialization = DSAPublicKey


def generate_parameters(key_size, backend=None):
    backend = _get_backend(backend)
    return backend.generate_dsa_parameters(key_size)


def generate_private_key(key_size, backend=None):
    backend = _get_backend(backend)
    return backend.generate_dsa_private_key_and_parameters(key_size)


def _check_dsa_parameters(parameters):
    if parameters.p.bit_length() not in [1024, 2048, 3072, 4096]:
        raise ValueError(
            "p must be exactly 1024, 2048, 3072, or 4096 bits long"
        )
    if parameters.q.bit_length() not in [160, 224, 256]:
        raise ValueError("q must be exactly 160, 224, or 256 bits long")

    if not (1 < parameters.g < parameters.p):
        raise ValueError("g, p don't satisfy 1 < g < p.")


def _check_dsa_private_numbers(numbers):
    parameters = numbers.public_numbers.parameter_numbers
    _check_dsa_parameters(parameters)
    if numbers.x <= 0 or numbers.x >= parameters.q:
        raise ValueError("x must be > 0 and < q.")

    if numbers.public_numbers.y != pow(parameters.g, numbers.x, parameters.p):
        raise ValueError("y must be equal to (g ** x % p).")


class DSAParameterNumbers(object):
    def __init__(self, p, q, g):
        if (
            not isinstance(p, six.integer_types)
            or not isinstance(q, six.integer_types)
            or not isinstance(g, six.integer_types)
        ):
            raise TypeError(
                "DSAParameterNumbers p, q, and g arguments must be integers."
            )

        self._p = p
        self._q = q
        self._g = g

    p = utils.read_only_property("_p")
    q = utils.read_only_property("_q")
    g = utils.read_only_property("_g")

    def parameters(self, backend=None):
        backend = _get_backend(backend)
        return backend.load_dsa_parameter_numbers(self)

    def __eq__(self, other):
        if not isinstance(other, DSAParameterNumbers):
            return NotImplemented

        return self.p == other.p and self.q == other.q and self.g == other.g

    def __ne__(self, other):
        return not self == other

    def __repr__(self):
        return (
            "<DSAParameterNumbers(p={self.p}, q={self.q}, "
            "g={self.g})>".format(self=self)
        )


class DSAPublicNumbers(object):
    def __init__(self, y, parameter_numbers):
        if not isinstance(y, six.integer_types):
            raise TypeError("DSAPublicNumbers y argument must be an integer.")

        if not isinstance(parameter_numbers, DSAParameterNumbers):
            raise TypeError(
                "parameter_numbers must be a DSAParameterNumbers instance."
            )

        self._y = y
        self._parameter_numbers = parameter_numbers

    y = utils.read_only_property("_y")
    parameter_numbers = utils.read_only_property("_parameter_numbers")

    def public_key(self, backend=None):
        backend = _get_backend(backend)
        return backend.load_dsa_public_numbers(self)

    def __eq__(self, other):
        if not isinstance(other, DSAPublicNumbers):
            return NotImplemented

        return (
            self.y == other.y
            and self.parameter_numbers == other.parameter_numbers
        )

    def __ne__(self, other):
        return not self == other

    def __repr__(self):
        return (
            "<DSAPublicNumbers(y={self.y}, "
            "parameter_numbers={self.parameter_numbers})>".format(self=self)
        )


class DSAPrivateNumbers(object):
    def __init__(self, x, public_numbers):
        if not isinstance(x, six.integer_types):
            raise TypeError("DSAPrivateNumbers x argument must be an integer.")

        if not isinstance(public_numbers, DSAPublicNumbers):
            raise TypeError(
                "public_numbers must be a DSAPublicNumbers instance."
            )
        self._public_numbers = public_numbers
        self._x = x

    x = utils.read_only_property("_x")
    public_numbers = utils.read_only_property("_public_numbers")

    def private_key(self, backend=None):
        backend = _get_backend(backend)
        return backend.load_dsa_private_numbers(self)

    def __eq__(self, other):
        if not isinstance(other, DSAPrivateNumbers):
            return NotImplemented

        return (
            self.x == other.x and self.public_numbers == other.public_numbers
        )

    def __ne__(self, other):
        return not self == other
