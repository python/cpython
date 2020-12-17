# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc

import six


@six.add_metaclass(abc.ABCMeta)
class AsymmetricSignatureContext(object):
    @abc.abstractmethod
    def update(self, data):
        """
        Processes the provided bytes and returns nothing.
        """

    @abc.abstractmethod
    def finalize(self):
        """
        Returns the signature as bytes.
        """


@six.add_metaclass(abc.ABCMeta)
class AsymmetricVerificationContext(object):
    @abc.abstractmethod
    def update(self, data):
        """
        Processes the provided bytes and returns nothing.
        """

    @abc.abstractmethod
    def verify(self):
        """
        Raises an exception if the bytes provided to update do not match the
        signature or the signature does not match the public key.
        """
