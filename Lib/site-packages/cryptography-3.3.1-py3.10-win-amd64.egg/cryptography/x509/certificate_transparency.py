# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc
from enum import Enum

import six


class LogEntryType(Enum):
    X509_CERTIFICATE = 0
    PRE_CERTIFICATE = 1


class Version(Enum):
    v1 = 0


@six.add_metaclass(abc.ABCMeta)
class SignedCertificateTimestamp(object):
    @abc.abstractproperty
    def version(self):
        """
        Returns the SCT version.
        """

    @abc.abstractproperty
    def log_id(self):
        """
        Returns an identifier indicating which log this SCT is for.
        """

    @abc.abstractproperty
    def timestamp(self):
        """
        Returns the timestamp for this SCT.
        """

    @abc.abstractproperty
    def entry_type(self):
        """
        Returns whether this is an SCT for a certificate or pre-certificate.
        """
