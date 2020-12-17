# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc
import datetime
import os
from enum import Enum

import six

from cryptography import utils
from cryptography.hazmat.backends import _get_backend
from cryptography.hazmat.primitives.asymmetric import (
    dsa,
    ec,
    ed25519,
    ed448,
    rsa,
)
from cryptography.x509.extensions import Extension, ExtensionType
from cryptography.x509.name import Name
from cryptography.x509.oid import ObjectIdentifier


_EARLIEST_UTC_TIME = datetime.datetime(1950, 1, 1)


class AttributeNotFound(Exception):
    def __init__(self, msg, oid):
        super(AttributeNotFound, self).__init__(msg)
        self.oid = oid


def _reject_duplicate_extension(extension, extensions):
    # This is quadratic in the number of extensions
    for e in extensions:
        if e.oid == extension.oid:
            raise ValueError("This extension has already been set.")


def _reject_duplicate_attribute(oid, attributes):
    # This is quadratic in the number of attributes
    for attr_oid, _ in attributes:
        if attr_oid == oid:
            raise ValueError("This attribute has already been set.")


def _convert_to_naive_utc_time(time):
    """Normalizes a datetime to a naive datetime in UTC.

    time -- datetime to normalize. Assumed to be in UTC if not timezone
            aware.
    """
    if time.tzinfo is not None:
        offset = time.utcoffset()
        offset = offset if offset else datetime.timedelta()
        return time.replace(tzinfo=None) - offset
    else:
        return time


class Version(Enum):
    v1 = 0
    v3 = 2


def load_pem_x509_certificate(data, backend=None):
    backend = _get_backend(backend)
    return backend.load_pem_x509_certificate(data)


def load_der_x509_certificate(data, backend=None):
    backend = _get_backend(backend)
    return backend.load_der_x509_certificate(data)


def load_pem_x509_csr(data, backend=None):
    backend = _get_backend(backend)
    return backend.load_pem_x509_csr(data)


def load_der_x509_csr(data, backend=None):
    backend = _get_backend(backend)
    return backend.load_der_x509_csr(data)


def load_pem_x509_crl(data, backend=None):
    backend = _get_backend(backend)
    return backend.load_pem_x509_crl(data)


def load_der_x509_crl(data, backend=None):
    backend = _get_backend(backend)
    return backend.load_der_x509_crl(data)


class InvalidVersion(Exception):
    def __init__(self, msg, parsed_version):
        super(InvalidVersion, self).__init__(msg)
        self.parsed_version = parsed_version


@six.add_metaclass(abc.ABCMeta)
class Certificate(object):
    @abc.abstractmethod
    def fingerprint(self, algorithm):
        """
        Returns bytes using digest passed.
        """

    @abc.abstractproperty
    def serial_number(self):
        """
        Returns certificate serial number
        """

    @abc.abstractproperty
    def version(self):
        """
        Returns the certificate version
        """

    @abc.abstractmethod
    def public_key(self):
        """
        Returns the public key
        """

    @abc.abstractproperty
    def not_valid_before(self):
        """
        Not before time (represented as UTC datetime)
        """

    @abc.abstractproperty
    def not_valid_after(self):
        """
        Not after time (represented as UTC datetime)
        """

    @abc.abstractproperty
    def issuer(self):
        """
        Returns the issuer name object.
        """

    @abc.abstractproperty
    def subject(self):
        """
        Returns the subject name object.
        """

    @abc.abstractproperty
    def signature_hash_algorithm(self):
        """
        Returns a HashAlgorithm corresponding to the type of the digest signed
        in the certificate.
        """

    @abc.abstractproperty
    def signature_algorithm_oid(self):
        """
        Returns the ObjectIdentifier of the signature algorithm.
        """

    @abc.abstractproperty
    def extensions(self):
        """
        Returns an Extensions object.
        """

    @abc.abstractproperty
    def signature(self):
        """
        Returns the signature bytes.
        """

    @abc.abstractproperty
    def tbs_certificate_bytes(self):
        """
        Returns the tbsCertificate payload bytes as defined in RFC 5280.
        """

    @abc.abstractmethod
    def __eq__(self, other):
        """
        Checks equality.
        """

    @abc.abstractmethod
    def __ne__(self, other):
        """
        Checks not equal.
        """

    @abc.abstractmethod
    def __hash__(self):
        """
        Computes a hash.
        """

    @abc.abstractmethod
    def public_bytes(self, encoding):
        """
        Serializes the certificate to PEM or DER format.
        """


@six.add_metaclass(abc.ABCMeta)
class CertificateRevocationList(object):
    @abc.abstractmethod
    def public_bytes(self, encoding):
        """
        Serializes the CRL to PEM or DER format.
        """

    @abc.abstractmethod
    def fingerprint(self, algorithm):
        """
        Returns bytes using digest passed.
        """

    @abc.abstractmethod
    def get_revoked_certificate_by_serial_number(self, serial_number):
        """
        Returns an instance of RevokedCertificate or None if the serial_number
        is not in the CRL.
        """

    @abc.abstractproperty
    def signature_hash_algorithm(self):
        """
        Returns a HashAlgorithm corresponding to the type of the digest signed
        in the certificate.
        """

    @abc.abstractproperty
    def signature_algorithm_oid(self):
        """
        Returns the ObjectIdentifier of the signature algorithm.
        """

    @abc.abstractproperty
    def issuer(self):
        """
        Returns the X509Name with the issuer of this CRL.
        """

    @abc.abstractproperty
    def next_update(self):
        """
        Returns the date of next update for this CRL.
        """

    @abc.abstractproperty
    def last_update(self):
        """
        Returns the date of last update for this CRL.
        """

    @abc.abstractproperty
    def extensions(self):
        """
        Returns an Extensions object containing a list of CRL extensions.
        """

    @abc.abstractproperty
    def signature(self):
        """
        Returns the signature bytes.
        """

    @abc.abstractproperty
    def tbs_certlist_bytes(self):
        """
        Returns the tbsCertList payload bytes as defined in RFC 5280.
        """

    @abc.abstractmethod
    def __eq__(self, other):
        """
        Checks equality.
        """

    @abc.abstractmethod
    def __ne__(self, other):
        """
        Checks not equal.
        """

    @abc.abstractmethod
    def __len__(self):
        """
        Number of revoked certificates in the CRL.
        """

    @abc.abstractmethod
    def __getitem__(self, idx):
        """
        Returns a revoked certificate (or slice of revoked certificates).
        """

    @abc.abstractmethod
    def __iter__(self):
        """
        Iterator over the revoked certificates
        """

    @abc.abstractmethod
    def is_signature_valid(self, public_key):
        """
        Verifies signature of revocation list against given public key.
        """


@six.add_metaclass(abc.ABCMeta)
class CertificateSigningRequest(object):
    @abc.abstractmethod
    def __eq__(self, other):
        """
        Checks equality.
        """

    @abc.abstractmethod
    def __ne__(self, other):
        """
        Checks not equal.
        """

    @abc.abstractmethod
    def __hash__(self):
        """
        Computes a hash.
        """

    @abc.abstractmethod
    def public_key(self):
        """
        Returns the public key
        """

    @abc.abstractproperty
    def subject(self):
        """
        Returns the subject name object.
        """

    @abc.abstractproperty
    def signature_hash_algorithm(self):
        """
        Returns a HashAlgorithm corresponding to the type of the digest signed
        in the certificate.
        """

    @abc.abstractproperty
    def signature_algorithm_oid(self):
        """
        Returns the ObjectIdentifier of the signature algorithm.
        """

    @abc.abstractproperty
    def extensions(self):
        """
        Returns the extensions in the signing request.
        """

    @abc.abstractmethod
    def public_bytes(self, encoding):
        """
        Encodes the request to PEM or DER format.
        """

    @abc.abstractproperty
    def signature(self):
        """
        Returns the signature bytes.
        """

    @abc.abstractproperty
    def tbs_certrequest_bytes(self):
        """
        Returns the PKCS#10 CertificationRequestInfo bytes as defined in RFC
        2986.
        """

    @abc.abstractproperty
    def is_signature_valid(self):
        """
        Verifies signature of signing request.
        """

    @abc.abstractproperty
    def get_attribute_for_oid(self):
        """
        Get the attribute value for a given OID.
        """


@six.add_metaclass(abc.ABCMeta)
class RevokedCertificate(object):
    @abc.abstractproperty
    def serial_number(self):
        """
        Returns the serial number of the revoked certificate.
        """

    @abc.abstractproperty
    def revocation_date(self):
        """
        Returns the date of when this certificate was revoked.
        """

    @abc.abstractproperty
    def extensions(self):
        """
        Returns an Extensions object containing a list of Revoked extensions.
        """


class CertificateSigningRequestBuilder(object):
    def __init__(self, subject_name=None, extensions=[], attributes=[]):
        """
        Creates an empty X.509 certificate request (v1).
        """
        self._subject_name = subject_name
        self._extensions = extensions
        self._attributes = attributes

    def subject_name(self, name):
        """
        Sets the certificate requestor's distinguished name.
        """
        if not isinstance(name, Name):
            raise TypeError("Expecting x509.Name object.")
        if self._subject_name is not None:
            raise ValueError("The subject name may only be set once.")
        return CertificateSigningRequestBuilder(
            name, self._extensions, self._attributes
        )

    def add_extension(self, extension, critical):
        """
        Adds an X.509 extension to the certificate request.
        """
        if not isinstance(extension, ExtensionType):
            raise TypeError("extension must be an ExtensionType")

        extension = Extension(extension.oid, critical, extension)
        _reject_duplicate_extension(extension, self._extensions)

        return CertificateSigningRequestBuilder(
            self._subject_name,
            self._extensions + [extension],
            self._attributes,
        )

    def add_attribute(self, oid, value):
        """
        Adds an X.509 attribute with an OID and associated value.
        """
        if not isinstance(oid, ObjectIdentifier):
            raise TypeError("oid must be an ObjectIdentifier")

        if not isinstance(value, bytes):
            raise TypeError("value must be bytes")

        _reject_duplicate_attribute(oid, self._attributes)

        return CertificateSigningRequestBuilder(
            self._subject_name,
            self._extensions,
            self._attributes + [(oid, value)],
        )

    def sign(self, private_key, algorithm, backend=None):
        """
        Signs the request using the requestor's private key.
        """
        backend = _get_backend(backend)
        if self._subject_name is None:
            raise ValueError("A CertificateSigningRequest must have a subject")
        return backend.create_x509_csr(self, private_key, algorithm)


class CertificateBuilder(object):
    def __init__(
        self,
        issuer_name=None,
        subject_name=None,
        public_key=None,
        serial_number=None,
        not_valid_before=None,
        not_valid_after=None,
        extensions=[],
    ):
        self._version = Version.v3
        self._issuer_name = issuer_name
        self._subject_name = subject_name
        self._public_key = public_key
        self._serial_number = serial_number
        self._not_valid_before = not_valid_before
        self._not_valid_after = not_valid_after
        self._extensions = extensions

    def issuer_name(self, name):
        """
        Sets the CA's distinguished name.
        """
        if not isinstance(name, Name):
            raise TypeError("Expecting x509.Name object.")
        if self._issuer_name is not None:
            raise ValueError("The issuer name may only be set once.")
        return CertificateBuilder(
            name,
            self._subject_name,
            self._public_key,
            self._serial_number,
            self._not_valid_before,
            self._not_valid_after,
            self._extensions,
        )

    def subject_name(self, name):
        """
        Sets the requestor's distinguished name.
        """
        if not isinstance(name, Name):
            raise TypeError("Expecting x509.Name object.")
        if self._subject_name is not None:
            raise ValueError("The subject name may only be set once.")
        return CertificateBuilder(
            self._issuer_name,
            name,
            self._public_key,
            self._serial_number,
            self._not_valid_before,
            self._not_valid_after,
            self._extensions,
        )

    def public_key(self, key):
        """
        Sets the requestor's public key (as found in the signing request).
        """
        if not isinstance(
            key,
            (
                dsa.DSAPublicKey,
                rsa.RSAPublicKey,
                ec.EllipticCurvePublicKey,
                ed25519.Ed25519PublicKey,
                ed448.Ed448PublicKey,
            ),
        ):
            raise TypeError(
                "Expecting one of DSAPublicKey, RSAPublicKey,"
                " EllipticCurvePublicKey, Ed25519PublicKey or"
                " Ed448PublicKey."
            )
        if self._public_key is not None:
            raise ValueError("The public key may only be set once.")
        return CertificateBuilder(
            self._issuer_name,
            self._subject_name,
            key,
            self._serial_number,
            self._not_valid_before,
            self._not_valid_after,
            self._extensions,
        )

    def serial_number(self, number):
        """
        Sets the certificate serial number.
        """
        if not isinstance(number, six.integer_types):
            raise TypeError("Serial number must be of integral type.")
        if self._serial_number is not None:
            raise ValueError("The serial number may only be set once.")
        if number <= 0:
            raise ValueError("The serial number should be positive.")

        # ASN.1 integers are always signed, so most significant bit must be
        # zero.
        if number.bit_length() >= 160:  # As defined in RFC 5280
            raise ValueError(
                "The serial number should not be more than 159 " "bits."
            )
        return CertificateBuilder(
            self._issuer_name,
            self._subject_name,
            self._public_key,
            number,
            self._not_valid_before,
            self._not_valid_after,
            self._extensions,
        )

    def not_valid_before(self, time):
        """
        Sets the certificate activation time.
        """
        if not isinstance(time, datetime.datetime):
            raise TypeError("Expecting datetime object.")
        if self._not_valid_before is not None:
            raise ValueError("The not valid before may only be set once.")
        time = _convert_to_naive_utc_time(time)
        if time < _EARLIEST_UTC_TIME:
            raise ValueError(
                "The not valid before date must be on or after"
                " 1950 January 1)."
            )
        if self._not_valid_after is not None and time > self._not_valid_after:
            raise ValueError(
                "The not valid before date must be before the not valid after "
                "date."
            )
        return CertificateBuilder(
            self._issuer_name,
            self._subject_name,
            self._public_key,
            self._serial_number,
            time,
            self._not_valid_after,
            self._extensions,
        )

    def not_valid_after(self, time):
        """
        Sets the certificate expiration time.
        """
        if not isinstance(time, datetime.datetime):
            raise TypeError("Expecting datetime object.")
        if self._not_valid_after is not None:
            raise ValueError("The not valid after may only be set once.")
        time = _convert_to_naive_utc_time(time)
        if time < _EARLIEST_UTC_TIME:
            raise ValueError(
                "The not valid after date must be on or after"
                " 1950 January 1."
            )
        if (
            self._not_valid_before is not None
            and time < self._not_valid_before
        ):
            raise ValueError(
                "The not valid after date must be after the not valid before "
                "date."
            )
        return CertificateBuilder(
            self._issuer_name,
            self._subject_name,
            self._public_key,
            self._serial_number,
            self._not_valid_before,
            time,
            self._extensions,
        )

    def add_extension(self, extension, critical):
        """
        Adds an X.509 extension to the certificate.
        """
        if not isinstance(extension, ExtensionType):
            raise TypeError("extension must be an ExtensionType")

        extension = Extension(extension.oid, critical, extension)
        _reject_duplicate_extension(extension, self._extensions)

        return CertificateBuilder(
            self._issuer_name,
            self._subject_name,
            self._public_key,
            self._serial_number,
            self._not_valid_before,
            self._not_valid_after,
            self._extensions + [extension],
        )

    def sign(self, private_key, algorithm, backend=None):
        """
        Signs the certificate using the CA's private key.
        """
        backend = _get_backend(backend)
        if self._subject_name is None:
            raise ValueError("A certificate must have a subject name")

        if self._issuer_name is None:
            raise ValueError("A certificate must have an issuer name")

        if self._serial_number is None:
            raise ValueError("A certificate must have a serial number")

        if self._not_valid_before is None:
            raise ValueError("A certificate must have a not valid before time")

        if self._not_valid_after is None:
            raise ValueError("A certificate must have a not valid after time")

        if self._public_key is None:
            raise ValueError("A certificate must have a public key")

        return backend.create_x509_certificate(self, private_key, algorithm)


class CertificateRevocationListBuilder(object):
    def __init__(
        self,
        issuer_name=None,
        last_update=None,
        next_update=None,
        extensions=[],
        revoked_certificates=[],
    ):
        self._issuer_name = issuer_name
        self._last_update = last_update
        self._next_update = next_update
        self._extensions = extensions
        self._revoked_certificates = revoked_certificates

    def issuer_name(self, issuer_name):
        if not isinstance(issuer_name, Name):
            raise TypeError("Expecting x509.Name object.")
        if self._issuer_name is not None:
            raise ValueError("The issuer name may only be set once.")
        return CertificateRevocationListBuilder(
            issuer_name,
            self._last_update,
            self._next_update,
            self._extensions,
            self._revoked_certificates,
        )

    def last_update(self, last_update):
        if not isinstance(last_update, datetime.datetime):
            raise TypeError("Expecting datetime object.")
        if self._last_update is not None:
            raise ValueError("Last update may only be set once.")
        last_update = _convert_to_naive_utc_time(last_update)
        if last_update < _EARLIEST_UTC_TIME:
            raise ValueError(
                "The last update date must be on or after" " 1950 January 1."
            )
        if self._next_update is not None and last_update > self._next_update:
            raise ValueError(
                "The last update date must be before the next update date."
            )
        return CertificateRevocationListBuilder(
            self._issuer_name,
            last_update,
            self._next_update,
            self._extensions,
            self._revoked_certificates,
        )

    def next_update(self, next_update):
        if not isinstance(next_update, datetime.datetime):
            raise TypeError("Expecting datetime object.")
        if self._next_update is not None:
            raise ValueError("Last update may only be set once.")
        next_update = _convert_to_naive_utc_time(next_update)
        if next_update < _EARLIEST_UTC_TIME:
            raise ValueError(
                "The last update date must be on or after" " 1950 January 1."
            )
        if self._last_update is not None and next_update < self._last_update:
            raise ValueError(
                "The next update date must be after the last update date."
            )
        return CertificateRevocationListBuilder(
            self._issuer_name,
            self._last_update,
            next_update,
            self._extensions,
            self._revoked_certificates,
        )

    def add_extension(self, extension, critical):
        """
        Adds an X.509 extension to the certificate revocation list.
        """
        if not isinstance(extension, ExtensionType):
            raise TypeError("extension must be an ExtensionType")

        extension = Extension(extension.oid, critical, extension)
        _reject_duplicate_extension(extension, self._extensions)
        return CertificateRevocationListBuilder(
            self._issuer_name,
            self._last_update,
            self._next_update,
            self._extensions + [extension],
            self._revoked_certificates,
        )

    def add_revoked_certificate(self, revoked_certificate):
        """
        Adds a revoked certificate to the CRL.
        """
        if not isinstance(revoked_certificate, RevokedCertificate):
            raise TypeError("Must be an instance of RevokedCertificate")

        return CertificateRevocationListBuilder(
            self._issuer_name,
            self._last_update,
            self._next_update,
            self._extensions,
            self._revoked_certificates + [revoked_certificate],
        )

    def sign(self, private_key, algorithm, backend=None):
        backend = _get_backend(backend)
        if self._issuer_name is None:
            raise ValueError("A CRL must have an issuer name")

        if self._last_update is None:
            raise ValueError("A CRL must have a last update time")

        if self._next_update is None:
            raise ValueError("A CRL must have a next update time")

        return backend.create_x509_crl(self, private_key, algorithm)


class RevokedCertificateBuilder(object):
    def __init__(
        self, serial_number=None, revocation_date=None, extensions=[]
    ):
        self._serial_number = serial_number
        self._revocation_date = revocation_date
        self._extensions = extensions

    def serial_number(self, number):
        if not isinstance(number, six.integer_types):
            raise TypeError("Serial number must be of integral type.")
        if self._serial_number is not None:
            raise ValueError("The serial number may only be set once.")
        if number <= 0:
            raise ValueError("The serial number should be positive")

        # ASN.1 integers are always signed, so most significant bit must be
        # zero.
        if number.bit_length() >= 160:  # As defined in RFC 5280
            raise ValueError(
                "The serial number should not be more than 159 " "bits."
            )
        return RevokedCertificateBuilder(
            number, self._revocation_date, self._extensions
        )

    def revocation_date(self, time):
        if not isinstance(time, datetime.datetime):
            raise TypeError("Expecting datetime object.")
        if self._revocation_date is not None:
            raise ValueError("The revocation date may only be set once.")
        time = _convert_to_naive_utc_time(time)
        if time < _EARLIEST_UTC_TIME:
            raise ValueError(
                "The revocation date must be on or after" " 1950 January 1."
            )
        return RevokedCertificateBuilder(
            self._serial_number, time, self._extensions
        )

    def add_extension(self, extension, critical):
        if not isinstance(extension, ExtensionType):
            raise TypeError("extension must be an ExtensionType")

        extension = Extension(extension.oid, critical, extension)
        _reject_duplicate_extension(extension, self._extensions)
        return RevokedCertificateBuilder(
            self._serial_number,
            self._revocation_date,
            self._extensions + [extension],
        )

    def build(self, backend=None):
        backend = _get_backend(backend)
        if self._serial_number is None:
            raise ValueError("A revoked certificate must have a serial number")
        if self._revocation_date is None:
            raise ValueError(
                "A revoked certificate must have a revocation date"
            )

        return backend.create_x509_revoked_certificate(self)


def random_serial_number():
    return utils.int_from_bytes(os.urandom(20), "big") >> 1
