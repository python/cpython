# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import functools

from cryptography import utils, x509
from cryptography.exceptions import UnsupportedAlgorithm
from cryptography.hazmat.backends.openssl.decode_asn1 import (
    _CRL_ENTRY_REASON_CODE_TO_ENUM,
    _asn1_integer_to_int,
    _asn1_string_to_bytes,
    _decode_x509_name,
    _obj2txt,
    _parse_asn1_generalized_time,
)
from cryptography.hazmat.backends.openssl.x509 import _Certificate
from cryptography.hazmat.primitives import serialization
from cryptography.x509.ocsp import (
    OCSPCertStatus,
    OCSPRequest,
    OCSPResponse,
    OCSPResponseStatus,
    _CERT_STATUS_TO_ENUM,
    _OIDS_TO_HASH,
    _RESPONSE_STATUS_TO_ENUM,
)


def _requires_successful_response(func):
    @functools.wraps(func)
    def wrapper(self, *args):
        if self.response_status != OCSPResponseStatus.SUCCESSFUL:
            raise ValueError(
                "OCSP response status is not successful so the property "
                "has no value"
            )
        else:
            return func(self, *args)

    return wrapper


def _issuer_key_hash(backend, cert_id):
    key_hash = backend._ffi.new("ASN1_OCTET_STRING **")
    res = backend._lib.OCSP_id_get0_info(
        backend._ffi.NULL,
        backend._ffi.NULL,
        key_hash,
        backend._ffi.NULL,
        cert_id,
    )
    backend.openssl_assert(res == 1)
    backend.openssl_assert(key_hash[0] != backend._ffi.NULL)
    return _asn1_string_to_bytes(backend, key_hash[0])


def _issuer_name_hash(backend, cert_id):
    name_hash = backend._ffi.new("ASN1_OCTET_STRING **")
    res = backend._lib.OCSP_id_get0_info(
        name_hash,
        backend._ffi.NULL,
        backend._ffi.NULL,
        backend._ffi.NULL,
        cert_id,
    )
    backend.openssl_assert(res == 1)
    backend.openssl_assert(name_hash[0] != backend._ffi.NULL)
    return _asn1_string_to_bytes(backend, name_hash[0])


def _serial_number(backend, cert_id):
    num = backend._ffi.new("ASN1_INTEGER **")
    res = backend._lib.OCSP_id_get0_info(
        backend._ffi.NULL, backend._ffi.NULL, backend._ffi.NULL, num, cert_id
    )
    backend.openssl_assert(res == 1)
    backend.openssl_assert(num[0] != backend._ffi.NULL)
    return _asn1_integer_to_int(backend, num[0])


def _hash_algorithm(backend, cert_id):
    asn1obj = backend._ffi.new("ASN1_OBJECT **")
    res = backend._lib.OCSP_id_get0_info(
        backend._ffi.NULL,
        asn1obj,
        backend._ffi.NULL,
        backend._ffi.NULL,
        cert_id,
    )
    backend.openssl_assert(res == 1)
    backend.openssl_assert(asn1obj[0] != backend._ffi.NULL)
    oid = _obj2txt(backend, asn1obj[0])
    try:
        return _OIDS_TO_HASH[oid]
    except KeyError:
        raise UnsupportedAlgorithm(
            "Signature algorithm OID: {} not recognized".format(oid)
        )


@utils.register_interface(OCSPResponse)
class _OCSPResponse(object):
    def __init__(self, backend, ocsp_response):
        self._backend = backend
        self._ocsp_response = ocsp_response
        status = self._backend._lib.OCSP_response_status(self._ocsp_response)
        self._backend.openssl_assert(status in _RESPONSE_STATUS_TO_ENUM)
        self._status = _RESPONSE_STATUS_TO_ENUM[status]
        if self._status is OCSPResponseStatus.SUCCESSFUL:
            basic = self._backend._lib.OCSP_response_get1_basic(
                self._ocsp_response
            )
            self._backend.openssl_assert(basic != self._backend._ffi.NULL)
            self._basic = self._backend._ffi.gc(
                basic, self._backend._lib.OCSP_BASICRESP_free
            )
            num_resp = self._backend._lib.OCSP_resp_count(self._basic)
            if num_resp != 1:
                raise ValueError(
                    "OCSP response contains more than one SINGLERESP structure"
                    ", which this library does not support. "
                    "{} found".format(num_resp)
                )
            self._single = self._backend._lib.OCSP_resp_get0(self._basic, 0)
            self._backend.openssl_assert(
                self._single != self._backend._ffi.NULL
            )
            self._cert_id = self._backend._lib.OCSP_SINGLERESP_get0_id(
                self._single
            )
            self._backend.openssl_assert(
                self._cert_id != self._backend._ffi.NULL
            )

    response_status = utils.read_only_property("_status")

    @property
    @_requires_successful_response
    def signature_algorithm_oid(self):
        alg = self._backend._lib.OCSP_resp_get0_tbs_sigalg(self._basic)
        self._backend.openssl_assert(alg != self._backend._ffi.NULL)
        oid = _obj2txt(self._backend, alg.algorithm)
        return x509.ObjectIdentifier(oid)

    @property
    @_requires_successful_response
    def signature_hash_algorithm(self):
        oid = self.signature_algorithm_oid
        try:
            return x509._SIG_OIDS_TO_HASH[oid]
        except KeyError:
            raise UnsupportedAlgorithm(
                "Signature algorithm OID:{} not recognized".format(oid)
            )

    @property
    @_requires_successful_response
    def signature(self):
        sig = self._backend._lib.OCSP_resp_get0_signature(self._basic)
        self._backend.openssl_assert(sig != self._backend._ffi.NULL)
        return _asn1_string_to_bytes(self._backend, sig)

    @property
    @_requires_successful_response
    def tbs_response_bytes(self):
        respdata = self._backend._lib.OCSP_resp_get0_respdata(self._basic)
        self._backend.openssl_assert(respdata != self._backend._ffi.NULL)
        pp = self._backend._ffi.new("unsigned char **")
        res = self._backend._lib.i2d_OCSP_RESPDATA(respdata, pp)
        self._backend.openssl_assert(pp[0] != self._backend._ffi.NULL)
        pp = self._backend._ffi.gc(
            pp, lambda pointer: self._backend._lib.OPENSSL_free(pointer[0])
        )
        self._backend.openssl_assert(res > 0)
        return self._backend._ffi.buffer(pp[0], res)[:]

    @property
    @_requires_successful_response
    def certificates(self):
        sk_x509 = self._backend._lib.OCSP_resp_get0_certs(self._basic)
        num = self._backend._lib.sk_X509_num(sk_x509)
        certs = []
        for i in range(num):
            x509 = self._backend._lib.sk_X509_value(sk_x509, i)
            self._backend.openssl_assert(x509 != self._backend._ffi.NULL)
            cert = _Certificate(self._backend, x509)
            # We need to keep the OCSP response that the certificate came from
            # alive until the Certificate object itself goes out of scope, so
            # we give it a private reference.
            cert._ocsp_resp = self
            certs.append(cert)

        return certs

    @property
    @_requires_successful_response
    def responder_key_hash(self):
        _, asn1_string = self._responder_key_name()
        if asn1_string == self._backend._ffi.NULL:
            return None
        else:
            return _asn1_string_to_bytes(self._backend, asn1_string)

    @property
    @_requires_successful_response
    def responder_name(self):
        x509_name, _ = self._responder_key_name()
        if x509_name == self._backend._ffi.NULL:
            return None
        else:
            return _decode_x509_name(self._backend, x509_name)

    def _responder_key_name(self):
        asn1_string = self._backend._ffi.new("ASN1_OCTET_STRING **")
        x509_name = self._backend._ffi.new("X509_NAME **")
        res = self._backend._lib.OCSP_resp_get0_id(
            self._basic, asn1_string, x509_name
        )
        self._backend.openssl_assert(res == 1)
        return x509_name[0], asn1_string[0]

    @property
    @_requires_successful_response
    def produced_at(self):
        produced_at = self._backend._lib.OCSP_resp_get0_produced_at(
            self._basic
        )
        return _parse_asn1_generalized_time(self._backend, produced_at)

    @property
    @_requires_successful_response
    def certificate_status(self):
        status = self._backend._lib.OCSP_single_get0_status(
            self._single,
            self._backend._ffi.NULL,
            self._backend._ffi.NULL,
            self._backend._ffi.NULL,
            self._backend._ffi.NULL,
        )
        self._backend.openssl_assert(status in _CERT_STATUS_TO_ENUM)
        return _CERT_STATUS_TO_ENUM[status]

    @property
    @_requires_successful_response
    def revocation_time(self):
        if self.certificate_status is not OCSPCertStatus.REVOKED:
            return None

        asn1_time = self._backend._ffi.new("ASN1_GENERALIZEDTIME **")
        self._backend._lib.OCSP_single_get0_status(
            self._single,
            self._backend._ffi.NULL,
            asn1_time,
            self._backend._ffi.NULL,
            self._backend._ffi.NULL,
        )
        self._backend.openssl_assert(asn1_time[0] != self._backend._ffi.NULL)
        return _parse_asn1_generalized_time(self._backend, asn1_time[0])

    @property
    @_requires_successful_response
    def revocation_reason(self):
        if self.certificate_status is not OCSPCertStatus.REVOKED:
            return None

        reason_ptr = self._backend._ffi.new("int *")
        self._backend._lib.OCSP_single_get0_status(
            self._single,
            reason_ptr,
            self._backend._ffi.NULL,
            self._backend._ffi.NULL,
            self._backend._ffi.NULL,
        )
        # If no reason is encoded OpenSSL returns -1
        if reason_ptr[0] == -1:
            return None
        else:
            self._backend.openssl_assert(
                reason_ptr[0] in _CRL_ENTRY_REASON_CODE_TO_ENUM
            )
            return _CRL_ENTRY_REASON_CODE_TO_ENUM[reason_ptr[0]]

    @property
    @_requires_successful_response
    def this_update(self):
        asn1_time = self._backend._ffi.new("ASN1_GENERALIZEDTIME **")
        self._backend._lib.OCSP_single_get0_status(
            self._single,
            self._backend._ffi.NULL,
            self._backend._ffi.NULL,
            asn1_time,
            self._backend._ffi.NULL,
        )
        self._backend.openssl_assert(asn1_time[0] != self._backend._ffi.NULL)
        return _parse_asn1_generalized_time(self._backend, asn1_time[0])

    @property
    @_requires_successful_response
    def next_update(self):
        asn1_time = self._backend._ffi.new("ASN1_GENERALIZEDTIME **")
        self._backend._lib.OCSP_single_get0_status(
            self._single,
            self._backend._ffi.NULL,
            self._backend._ffi.NULL,
            self._backend._ffi.NULL,
            asn1_time,
        )
        if asn1_time[0] != self._backend._ffi.NULL:
            return _parse_asn1_generalized_time(self._backend, asn1_time[0])
        else:
            return None

    @property
    @_requires_successful_response
    def issuer_key_hash(self):
        return _issuer_key_hash(self._backend, self._cert_id)

    @property
    @_requires_successful_response
    def issuer_name_hash(self):
        return _issuer_name_hash(self._backend, self._cert_id)

    @property
    @_requires_successful_response
    def hash_algorithm(self):
        return _hash_algorithm(self._backend, self._cert_id)

    @property
    @_requires_successful_response
    def serial_number(self):
        return _serial_number(self._backend, self._cert_id)

    @utils.cached_property
    @_requires_successful_response
    def extensions(self):
        return self._backend._ocsp_basicresp_ext_parser.parse(self._basic)

    @utils.cached_property
    @_requires_successful_response
    def single_extensions(self):
        return self._backend._ocsp_singleresp_ext_parser.parse(self._single)

    def public_bytes(self, encoding):
        if encoding is not serialization.Encoding.DER:
            raise ValueError("The only allowed encoding value is Encoding.DER")

        bio = self._backend._create_mem_bio_gc()
        res = self._backend._lib.i2d_OCSP_RESPONSE_bio(
            bio, self._ocsp_response
        )
        self._backend.openssl_assert(res > 0)
        return self._backend._read_mem_bio(bio)


@utils.register_interface(OCSPRequest)
class _OCSPRequest(object):
    def __init__(self, backend, ocsp_request):
        if backend._lib.OCSP_request_onereq_count(ocsp_request) > 1:
            raise NotImplementedError(
                "OCSP request contains more than one request"
            )
        self._backend = backend
        self._ocsp_request = ocsp_request
        self._request = self._backend._lib.OCSP_request_onereq_get0(
            self._ocsp_request, 0
        )
        self._backend.openssl_assert(self._request != self._backend._ffi.NULL)
        self._cert_id = self._backend._lib.OCSP_onereq_get0_id(self._request)
        self._backend.openssl_assert(self._cert_id != self._backend._ffi.NULL)

    @property
    def issuer_key_hash(self):
        return _issuer_key_hash(self._backend, self._cert_id)

    @property
    def issuer_name_hash(self):
        return _issuer_name_hash(self._backend, self._cert_id)

    @property
    def serial_number(self):
        return _serial_number(self._backend, self._cert_id)

    @property
    def hash_algorithm(self):
        return _hash_algorithm(self._backend, self._cert_id)

    @utils.cached_property
    def extensions(self):
        return self._backend._ocsp_req_ext_parser.parse(self._ocsp_request)

    def public_bytes(self, encoding):
        if encoding is not serialization.Encoding.DER:
            raise ValueError("The only allowed encoding value is Encoding.DER")

        bio = self._backend._create_mem_bio_gc()
        res = self._backend._lib.i2d_OCSP_REQUEST_bio(bio, self._ocsp_request)
        self._backend.openssl_assert(res > 0)
        return self._backend._read_mem_bio(bio)
