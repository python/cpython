# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc
import datetime
from enum import Enum

import six

from cryptography import x509
from cryptography.hazmat.primitives import hashes
from cryptography.x509.base import (
    _EARLIEST_UTC_TIME,
    _convert_to_naive_utc_time,
    _reject_duplicate_extension,
)


_OIDS_TO_HASH = {
    "1.3.14.3.2.26": hashes.SHA1(),
    "2.16.840.1.101.3.4.2.4": hashes.SHA224(),
    "2.16.840.1.101.3.4.2.1": hashes.SHA256(),
    "2.16.840.1.101.3.4.2.2": hashes.SHA384(),
    "2.16.840.1.101.3.4.2.3": hashes.SHA512(),
}


class OCSPResponderEncoding(Enum):
    HASH = "By Hash"
    NAME = "By Name"


class OCSPResponseStatus(Enum):
    SUCCESSFUL = 0
    MALFORMED_REQUEST = 1
    INTERNAL_ERROR = 2
    TRY_LATER = 3
    SIG_REQUIRED = 5
    UNAUTHORIZED = 6


_RESPONSE_STATUS_TO_ENUM = {x.value: x for x in OCSPResponseStatus}
_ALLOWED_HASHES = (
    hashes.SHA1,
    hashes.SHA224,
    hashes.SHA256,
    hashes.SHA384,
    hashes.SHA512,
)


def _verify_algorithm(algorithm):
    if not isinstance(algorithm, _ALLOWED_HASHES):
        raise ValueError(
            "Algorithm must be SHA1, SHA224, SHA256, SHA384, or SHA512"
        )


class OCSPCertStatus(Enum):
    GOOD = 0
    REVOKED = 1
    UNKNOWN = 2


_CERT_STATUS_TO_ENUM = {x.value: x for x in OCSPCertStatus}


def load_der_ocsp_request(data):
    from cryptography.hazmat.backends.openssl.backend import backend

    return backend.load_der_ocsp_request(data)


def load_der_ocsp_response(data):
    from cryptography.hazmat.backends.openssl.backend import backend

    return backend.load_der_ocsp_response(data)


class OCSPRequestBuilder(object):
    def __init__(self, request=None, extensions=[]):
        self._request = request
        self._extensions = extensions

    def add_certificate(self, cert, issuer, algorithm):
        if self._request is not None:
            raise ValueError("Only one certificate can be added to a request")

        _verify_algorithm(algorithm)
        if not isinstance(cert, x509.Certificate) or not isinstance(
            issuer, x509.Certificate
        ):
            raise TypeError("cert and issuer must be a Certificate")

        return OCSPRequestBuilder((cert, issuer, algorithm), self._extensions)

    def add_extension(self, extension, critical):
        if not isinstance(extension, x509.ExtensionType):
            raise TypeError("extension must be an ExtensionType")

        extension = x509.Extension(extension.oid, critical, extension)
        _reject_duplicate_extension(extension, self._extensions)

        return OCSPRequestBuilder(
            self._request, self._extensions + [extension]
        )

    def build(self):
        from cryptography.hazmat.backends.openssl.backend import backend

        if self._request is None:
            raise ValueError("You must add a certificate before building")

        return backend.create_ocsp_request(self)


class _SingleResponse(object):
    def __init__(
        self,
        cert,
        issuer,
        algorithm,
        cert_status,
        this_update,
        next_update,
        revocation_time,
        revocation_reason,
    ):
        if not isinstance(cert, x509.Certificate) or not isinstance(
            issuer, x509.Certificate
        ):
            raise TypeError("cert and issuer must be a Certificate")

        _verify_algorithm(algorithm)
        if not isinstance(this_update, datetime.datetime):
            raise TypeError("this_update must be a datetime object")
        if next_update is not None and not isinstance(
            next_update, datetime.datetime
        ):
            raise TypeError("next_update must be a datetime object or None")

        self._cert = cert
        self._issuer = issuer
        self._algorithm = algorithm
        self._this_update = this_update
        self._next_update = next_update

        if not isinstance(cert_status, OCSPCertStatus):
            raise TypeError(
                "cert_status must be an item from the OCSPCertStatus enum"
            )
        if cert_status is not OCSPCertStatus.REVOKED:
            if revocation_time is not None:
                raise ValueError(
                    "revocation_time can only be provided if the certificate "
                    "is revoked"
                )
            if revocation_reason is not None:
                raise ValueError(
                    "revocation_reason can only be provided if the certificate"
                    " is revoked"
                )
        else:
            if not isinstance(revocation_time, datetime.datetime):
                raise TypeError("revocation_time must be a datetime object")

            revocation_time = _convert_to_naive_utc_time(revocation_time)
            if revocation_time < _EARLIEST_UTC_TIME:
                raise ValueError(
                    "The revocation_time must be on or after"
                    " 1950 January 1."
                )

            if revocation_reason is not None and not isinstance(
                revocation_reason, x509.ReasonFlags
            ):
                raise TypeError(
                    "revocation_reason must be an item from the ReasonFlags "
                    "enum or None"
                )

        self._cert_status = cert_status
        self._revocation_time = revocation_time
        self._revocation_reason = revocation_reason


class OCSPResponseBuilder(object):
    def __init__(
        self, response=None, responder_id=None, certs=None, extensions=[]
    ):
        self._response = response
        self._responder_id = responder_id
        self._certs = certs
        self._extensions = extensions

    def add_response(
        self,
        cert,
        issuer,
        algorithm,
        cert_status,
        this_update,
        next_update,
        revocation_time,
        revocation_reason,
    ):
        if self._response is not None:
            raise ValueError("Only one response per OCSPResponse.")

        singleresp = _SingleResponse(
            cert,
            issuer,
            algorithm,
            cert_status,
            this_update,
            next_update,
            revocation_time,
            revocation_reason,
        )
        return OCSPResponseBuilder(
            singleresp,
            self._responder_id,
            self._certs,
            self._extensions,
        )

    def responder_id(self, encoding, responder_cert):
        if self._responder_id is not None:
            raise ValueError("responder_id can only be set once")
        if not isinstance(responder_cert, x509.Certificate):
            raise TypeError("responder_cert must be a Certificate")
        if not isinstance(encoding, OCSPResponderEncoding):
            raise TypeError(
                "encoding must be an element from OCSPResponderEncoding"
            )

        return OCSPResponseBuilder(
            self._response,
            (responder_cert, encoding),
            self._certs,
            self._extensions,
        )

    def certificates(self, certs):
        if self._certs is not None:
            raise ValueError("certificates may only be set once")
        certs = list(certs)
        if len(certs) == 0:
            raise ValueError("certs must not be an empty list")
        if not all(isinstance(x, x509.Certificate) for x in certs):
            raise TypeError("certs must be a list of Certificates")
        return OCSPResponseBuilder(
            self._response,
            self._responder_id,
            certs,
            self._extensions,
        )

    def add_extension(self, extension, critical):
        if not isinstance(extension, x509.ExtensionType):
            raise TypeError("extension must be an ExtensionType")

        extension = x509.Extension(extension.oid, critical, extension)
        _reject_duplicate_extension(extension, self._extensions)

        return OCSPResponseBuilder(
            self._response,
            self._responder_id,
            self._certs,
            self._extensions + [extension],
        )

    def sign(self, private_key, algorithm):
        from cryptography.hazmat.backends.openssl.backend import backend

        if self._response is None:
            raise ValueError("You must add a response before signing")
        if self._responder_id is None:
            raise ValueError("You must add a responder_id before signing")

        return backend.create_ocsp_response(
            OCSPResponseStatus.SUCCESSFUL, self, private_key, algorithm
        )

    @classmethod
    def build_unsuccessful(cls, response_status):
        from cryptography.hazmat.backends.openssl.backend import backend

        if not isinstance(response_status, OCSPResponseStatus):
            raise TypeError(
                "response_status must be an item from OCSPResponseStatus"
            )
        if response_status is OCSPResponseStatus.SUCCESSFUL:
            raise ValueError("response_status cannot be SUCCESSFUL")

        return backend.create_ocsp_response(response_status, None, None, None)


@six.add_metaclass(abc.ABCMeta)
class OCSPRequest(object):
    @abc.abstractproperty
    def issuer_key_hash(self):
        """
        The hash of the issuer public key
        """

    @abc.abstractproperty
    def issuer_name_hash(self):
        """
        The hash of the issuer name
        """

    @abc.abstractproperty
    def hash_algorithm(self):
        """
        The hash algorithm used in the issuer name and key hashes
        """

    @abc.abstractproperty
    def serial_number(self):
        """
        The serial number of the cert whose status is being checked
        """

    @abc.abstractmethod
    def public_bytes(self, encoding):
        """
        Serializes the request to DER
        """

    @abc.abstractproperty
    def extensions(self):
        """
        The list of request extensions. Not single request extensions.
        """


@six.add_metaclass(abc.ABCMeta)
class OCSPResponse(object):
    @abc.abstractproperty
    def response_status(self):
        """
        The status of the response. This is a value from the OCSPResponseStatus
        enumeration
        """

    @abc.abstractproperty
    def signature_algorithm_oid(self):
        """
        The ObjectIdentifier of the signature algorithm
        """

    @abc.abstractproperty
    def signature_hash_algorithm(self):
        """
        Returns a HashAlgorithm corresponding to the type of the digest signed
        """

    @abc.abstractproperty
    def signature(self):
        """
        The signature bytes
        """

    @abc.abstractproperty
    def tbs_response_bytes(self):
        """
        The tbsResponseData bytes
        """

    @abc.abstractproperty
    def certificates(self):
        """
        A list of certificates used to help build a chain to verify the OCSP
        response. This situation occurs when the OCSP responder uses a delegate
        certificate.
        """

    @abc.abstractproperty
    def responder_key_hash(self):
        """
        The responder's key hash or None
        """

    @abc.abstractproperty
    def responder_name(self):
        """
        The responder's Name or None
        """

    @abc.abstractproperty
    def produced_at(self):
        """
        The time the response was produced
        """

    @abc.abstractproperty
    def certificate_status(self):
        """
        The status of the certificate (an element from the OCSPCertStatus enum)
        """

    @abc.abstractproperty
    def revocation_time(self):
        """
        The date of when the certificate was revoked or None if not
        revoked.
        """

    @abc.abstractproperty
    def revocation_reason(self):
        """
        The reason the certificate was revoked or None if not specified or
        not revoked.
        """

    @abc.abstractproperty
    def this_update(self):
        """
        The most recent time at which the status being indicated is known by
        the responder to have been correct
        """

    @abc.abstractproperty
    def next_update(self):
        """
        The time when newer information will be available
        """

    @abc.abstractproperty
    def issuer_key_hash(self):
        """
        The hash of the issuer public key
        """

    @abc.abstractproperty
    def issuer_name_hash(self):
        """
        The hash of the issuer name
        """

    @abc.abstractproperty
    def hash_algorithm(self):
        """
        The hash algorithm used in the issuer name and key hashes
        """

    @abc.abstractproperty
    def serial_number(self):
        """
        The serial number of the cert whose status is being checked
        """

    @abc.abstractproperty
    def extensions(self):
        """
        The list of response extensions. Not single response extensions.
        """

    @abc.abstractproperty
    def single_extensions(self):
        """
        The list of single response extensions. Not response extensions.
        """
