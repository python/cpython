# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc

import six

from cryptography import utils
from cryptography.hazmat.backends import _get_backend


_MIN_MODULUS_SIZE = 512


def generate_parameters(generator, key_size, backend=None):
    backend = _get_backend(backend)
    return backend.generate_dh_parameters(generator, key_size)


class DHPrivateNumbers(object):
    def __init__(self, x, public_numbers):
        if not isinstance(x, six.integer_types):
            raise TypeError("x must be an integer.")

        if not isinstance(public_numbers, DHPublicNumbers):
            raise TypeError(
                "public_numbers must be an instance of " "DHPublicNumbers."
            )

        self._x = x
        self._public_numbers = public_numbers

    def __eq__(self, other):
        if not isinstance(other, DHPrivateNumbers):
            return NotImplemented

        return (
            self._x == other._x
            and self._public_numbers == other._public_numbers
        )

    def __ne__(self, other):
        return not self == other

    def private_key(self, backend=None):
        backend = _get_backend(backend)
        return backend.load_dh_private_numbers(self)

    public_numbers = utils.read_only_property("_public_numbers")
    x = utils.read_only_property("_x")


class DHPublicNumbers(object):
    def __init__(self, y, parameter_numbers):
        if not isinstance(y, six.integer_types):
            raise TypeError("y must be an integer.")

        if not isinstance(parameter_numbers, DHParameterNumbers):
            raise TypeError(
                "parameters must be an instance of DHParameterNumbers."
            )

        self._y = y
        self._parameter_numbers = parameter_numbers

    def __eq__(self, other):
        if not isinstance(other, DHPublicNumbers):
            return NotImplemented

        return (
            self._y == other._y
            and self._parameter_numbers == other._parameter_numbers
        )

    def __ne__(self, other):
        return not self == other

    def public_key(self, backend=None):
        backend = _get_backend(backend)
        return backend.load_dh_public_numbers(self)

    y = utils.read_only_property("_y")
    parameter_numbers = utils.read_only_property("_parameter_numbers")


class DHParameterNumbers(object):
    def __init__(self, p, g, q=None):
        if not isinstance(p, six.integer_types) or not isinstance(
            g, six.integer_types
        ):
            raise TypeError("p and g must be integers")
        if q is not None and not isinstance(q, six.integer_types):
            raise TypeError("q must be integer or None")

        if g < 2:
            raise ValueError("DH generator must be 2 or greater")

        if p.bit_length() < _MIN_MODULUS_SIZE:
            raise ValueError(
                "p (modulus) must be at least {}-bit".format(_MIN_MODULUS_SIZE)
            )

        self._p = p
        self._g = g
        self._q = q

    def __eq__(self, other):
        if not isinstance(other, DHParameterNumbers):
            return NotImplemented

        return (
            self._p == other._p and self._g == other._g and self._q == other._q
        )

    def __ne__(self, other):
        return not self == other

    def parameters(self, backend=None):
        backend = _get_backend(backend)
        return backend.load_dh_parameter_numbers(self)

    p = utils.read_only_property("_p")
    g = utils.read_only_property("_g")
    q = utils.read_only_property("_q")


@six.add_metaclass(abc.ABCMeta)
class DHParameters(object):
    @abc.abstractmethod
    def generate_private_key(self):
        """
        Generates and returns a DHPrivateKey.
        """

    @abc.abstractmethod
    def parameter_bytes(self, encoding, format):
        """
        Returns the parameters serialized as bytes.
        """

    @abc.abstractmethod
    def parameter_numbers(self):
        """
        Returns a DHParameterNumbers.
        """


DHParametersWithSerialization = DHParameters


@six.add_metaclass(abc.ABCMeta)
class DHPrivateKey(object):
    @abc.abstractproperty
    def key_size(self):
        """
        The bit length of the prime modulus.
        """

    @abc.abstractmethod
    def public_key(self):
        """
        The DHPublicKey associated with this private key.
        """

    @abc.abstractmethod
    def parameters(self):
        """
        The DHParameters object associated with this private key.
        """

    @abc.abstractmethod
    def exchange(self, peer_public_key):
        """
        Given peer's DHPublicKey, carry out the key exchange and
        return shared key as bytes.
        """


@six.add_metaclass(abc.ABCMeta)
class DHPrivateKeyWithSerialization(DHPrivateKey):
    @abc.abstractmethod
    def private_numbers(self):
        """
        Returns a DHPrivateNumbers.
        """

    @abc.abstractmethod
    def private_bytes(self, encoding, format, encryption_algorithm):
        """
        Returns the key serialized as bytes.
        """


@six.add_metaclass(abc.ABCMeta)
class DHPublicKey(object):
    @abc.abstractproperty
    def key_size(self):
        """
        The bit length of the prime modulus.
        """

    @abc.abstractmethod
    def parameters(self):
        """
        The DHParameters object associated with this public key.
        """

    @abc.abstractmethod
    def public_numbers(self):
        """
        Returns a DHPublicNumbers.
        """

    @abc.abstractmethod
    def public_bytes(self, encoding, format):
        """
        Returns the key serialized as bytes.
        """


DHPublicKeyWithSerialization = DHPublicKey
