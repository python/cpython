# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import abc
import datetime
import hashlib
import ipaddress
from enum import Enum

import six

from cryptography import utils
from cryptography.hazmat._der import (
    BIT_STRING,
    DERReader,
    OBJECT_IDENTIFIER,
    SEQUENCE,
)
from cryptography.hazmat.primitives import constant_time, serialization
from cryptography.hazmat.primitives.asymmetric.ec import EllipticCurvePublicKey
from cryptography.hazmat.primitives.asymmetric.rsa import RSAPublicKey
from cryptography.x509.certificate_transparency import (
    SignedCertificateTimestamp,
)
from cryptography.x509.general_name import GeneralName, IPAddress, OtherName
from cryptography.x509.name import RelativeDistinguishedName
from cryptography.x509.oid import (
    CRLEntryExtensionOID,
    ExtensionOID,
    OCSPExtensionOID,
    ObjectIdentifier,
)


def _key_identifier_from_public_key(public_key):
    if isinstance(public_key, RSAPublicKey):
        data = public_key.public_bytes(
            serialization.Encoding.DER,
            serialization.PublicFormat.PKCS1,
        )
    elif isinstance(public_key, EllipticCurvePublicKey):
        data = public_key.public_bytes(
            serialization.Encoding.X962,
            serialization.PublicFormat.UncompressedPoint,
        )
    else:
        # This is a very slow way to do this.
        serialized = public_key.public_bytes(
            serialization.Encoding.DER,
            serialization.PublicFormat.SubjectPublicKeyInfo,
        )

        reader = DERReader(serialized)
        with reader.read_single_element(SEQUENCE) as public_key_info:
            algorithm = public_key_info.read_element(SEQUENCE)
            public_key = public_key_info.read_element(BIT_STRING)

        # Double-check the algorithm structure.
        with algorithm:
            algorithm.read_element(OBJECT_IDENTIFIER)
            if not algorithm.is_empty():
                # Skip the optional parameters field.
                algorithm.read_any_element()

        # BIT STRING contents begin with the number of padding bytes added. It
        # must be zero for SubjectPublicKeyInfo structures.
        if public_key.read_byte() != 0:
            raise ValueError("Invalid public key encoding")

        data = public_key.data

    return hashlib.sha1(data).digest()


def _make_sequence_methods(field_name):
    def len_method(self):
        return len(getattr(self, field_name))

    def iter_method(self):
        return iter(getattr(self, field_name))

    def getitem_method(self, idx):
        return getattr(self, field_name)[idx]

    return len_method, iter_method, getitem_method


class DuplicateExtension(Exception):
    def __init__(self, msg, oid):
        super(DuplicateExtension, self).__init__(msg)
        self.oid = oid


class ExtensionNotFound(Exception):
    def __init__(self, msg, oid):
        super(ExtensionNotFound, self).__init__(msg)
        self.oid = oid


@six.add_metaclass(abc.ABCMeta)
class ExtensionType(object):
    @abc.abstractproperty
    def oid(self):
        """
        Returns the oid associated with the given extension type.
        """


class Extensions(object):
    def __init__(self, extensions):
        self._extensions = extensions

    def get_extension_for_oid(self, oid):
        for ext in self:
            if ext.oid == oid:
                return ext

        raise ExtensionNotFound("No {} extension was found".format(oid), oid)

    def get_extension_for_class(self, extclass):
        if extclass is UnrecognizedExtension:
            raise TypeError(
                "UnrecognizedExtension can't be used with "
                "get_extension_for_class because more than one instance of the"
                " class may be present."
            )

        for ext in self:
            if isinstance(ext.value, extclass):
                return ext

        raise ExtensionNotFound(
            "No {} extension was found".format(extclass), extclass.oid
        )

    __len__, __iter__, __getitem__ = _make_sequence_methods("_extensions")

    def __repr__(self):
        return "<Extensions({})>".format(self._extensions)


@utils.register_interface(ExtensionType)
class CRLNumber(object):
    oid = ExtensionOID.CRL_NUMBER

    def __init__(self, crl_number):
        if not isinstance(crl_number, six.integer_types):
            raise TypeError("crl_number must be an integer")

        self._crl_number = crl_number

    def __eq__(self, other):
        if not isinstance(other, CRLNumber):
            return NotImplemented

        return self.crl_number == other.crl_number

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(self.crl_number)

    def __repr__(self):
        return "<CRLNumber({})>".format(self.crl_number)

    crl_number = utils.read_only_property("_crl_number")


@utils.register_interface(ExtensionType)
class AuthorityKeyIdentifier(object):
    oid = ExtensionOID.AUTHORITY_KEY_IDENTIFIER

    def __init__(
        self,
        key_identifier,
        authority_cert_issuer,
        authority_cert_serial_number,
    ):
        if (authority_cert_issuer is None) != (
            authority_cert_serial_number is None
        ):
            raise ValueError(
                "authority_cert_issuer and authority_cert_serial_number "
                "must both be present or both None"
            )

        if authority_cert_issuer is not None:
            authority_cert_issuer = list(authority_cert_issuer)
            if not all(
                isinstance(x, GeneralName) for x in authority_cert_issuer
            ):
                raise TypeError(
                    "authority_cert_issuer must be a list of GeneralName "
                    "objects"
                )

        if authority_cert_serial_number is not None and not isinstance(
            authority_cert_serial_number, six.integer_types
        ):
            raise TypeError("authority_cert_serial_number must be an integer")

        self._key_identifier = key_identifier
        self._authority_cert_issuer = authority_cert_issuer
        self._authority_cert_serial_number = authority_cert_serial_number

    @classmethod
    def from_issuer_public_key(cls, public_key):
        digest = _key_identifier_from_public_key(public_key)
        return cls(
            key_identifier=digest,
            authority_cert_issuer=None,
            authority_cert_serial_number=None,
        )

    @classmethod
    def from_issuer_subject_key_identifier(cls, ski):
        return cls(
            key_identifier=ski.digest,
            authority_cert_issuer=None,
            authority_cert_serial_number=None,
        )

    def __repr__(self):
        return (
            "<AuthorityKeyIdentifier(key_identifier={0.key_identifier!r}, "
            "authority_cert_issuer={0.authority_cert_issuer}, "
            "authority_cert_serial_number={0.authority_cert_serial_number}"
            ")>".format(self)
        )

    def __eq__(self, other):
        if not isinstance(other, AuthorityKeyIdentifier):
            return NotImplemented

        return (
            self.key_identifier == other.key_identifier
            and self.authority_cert_issuer == other.authority_cert_issuer
            and self.authority_cert_serial_number
            == other.authority_cert_serial_number
        )

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        if self.authority_cert_issuer is None:
            aci = None
        else:
            aci = tuple(self.authority_cert_issuer)
        return hash(
            (self.key_identifier, aci, self.authority_cert_serial_number)
        )

    key_identifier = utils.read_only_property("_key_identifier")
    authority_cert_issuer = utils.read_only_property("_authority_cert_issuer")
    authority_cert_serial_number = utils.read_only_property(
        "_authority_cert_serial_number"
    )


@utils.register_interface(ExtensionType)
class SubjectKeyIdentifier(object):
    oid = ExtensionOID.SUBJECT_KEY_IDENTIFIER

    def __init__(self, digest):
        self._digest = digest

    @classmethod
    def from_public_key(cls, public_key):
        return cls(_key_identifier_from_public_key(public_key))

    digest = utils.read_only_property("_digest")

    def __repr__(self):
        return "<SubjectKeyIdentifier(digest={0!r})>".format(self.digest)

    def __eq__(self, other):
        if not isinstance(other, SubjectKeyIdentifier):
            return NotImplemented

        return constant_time.bytes_eq(self.digest, other.digest)

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(self.digest)


@utils.register_interface(ExtensionType)
class AuthorityInformationAccess(object):
    oid = ExtensionOID.AUTHORITY_INFORMATION_ACCESS

    def __init__(self, descriptions):
        descriptions = list(descriptions)
        if not all(isinstance(x, AccessDescription) for x in descriptions):
            raise TypeError(
                "Every item in the descriptions list must be an "
                "AccessDescription"
            )

        self._descriptions = descriptions

    __len__, __iter__, __getitem__ = _make_sequence_methods("_descriptions")

    def __repr__(self):
        return "<AuthorityInformationAccess({})>".format(self._descriptions)

    def __eq__(self, other):
        if not isinstance(other, AuthorityInformationAccess):
            return NotImplemented

        return self._descriptions == other._descriptions

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(tuple(self._descriptions))


@utils.register_interface(ExtensionType)
class SubjectInformationAccess(object):
    oid = ExtensionOID.SUBJECT_INFORMATION_ACCESS

    def __init__(self, descriptions):
        descriptions = list(descriptions)
        if not all(isinstance(x, AccessDescription) for x in descriptions):
            raise TypeError(
                "Every item in the descriptions list must be an "
                "AccessDescription"
            )

        self._descriptions = descriptions

    __len__, __iter__, __getitem__ = _make_sequence_methods("_descriptions")

    def __repr__(self):
        return "<SubjectInformationAccess({})>".format(self._descriptions)

    def __eq__(self, other):
        if not isinstance(other, SubjectInformationAccess):
            return NotImplemented

        return self._descriptions == other._descriptions

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(tuple(self._descriptions))


class AccessDescription(object):
    def __init__(self, access_method, access_location):
        if not isinstance(access_method, ObjectIdentifier):
            raise TypeError("access_method must be an ObjectIdentifier")

        if not isinstance(access_location, GeneralName):
            raise TypeError("access_location must be a GeneralName")

        self._access_method = access_method
        self._access_location = access_location

    def __repr__(self):
        return (
            "<AccessDescription(access_method={0.access_method}, access_locati"
            "on={0.access_location})>".format(self)
        )

    def __eq__(self, other):
        if not isinstance(other, AccessDescription):
            return NotImplemented

        return (
            self.access_method == other.access_method
            and self.access_location == other.access_location
        )

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash((self.access_method, self.access_location))

    access_method = utils.read_only_property("_access_method")
    access_location = utils.read_only_property("_access_location")


@utils.register_interface(ExtensionType)
class BasicConstraints(object):
    oid = ExtensionOID.BASIC_CONSTRAINTS

    def __init__(self, ca, path_length):
        if not isinstance(ca, bool):
            raise TypeError("ca must be a boolean value")

        if path_length is not None and not ca:
            raise ValueError("path_length must be None when ca is False")

        if path_length is not None and (
            not isinstance(path_length, six.integer_types) or path_length < 0
        ):
            raise TypeError(
                "path_length must be a non-negative integer or None"
            )

        self._ca = ca
        self._path_length = path_length

    ca = utils.read_only_property("_ca")
    path_length = utils.read_only_property("_path_length")

    def __repr__(self):
        return (
            "<BasicConstraints(ca={0.ca}, " "path_length={0.path_length})>"
        ).format(self)

    def __eq__(self, other):
        if not isinstance(other, BasicConstraints):
            return NotImplemented

        return self.ca == other.ca and self.path_length == other.path_length

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash((self.ca, self.path_length))


@utils.register_interface(ExtensionType)
class DeltaCRLIndicator(object):
    oid = ExtensionOID.DELTA_CRL_INDICATOR

    def __init__(self, crl_number):
        if not isinstance(crl_number, six.integer_types):
            raise TypeError("crl_number must be an integer")

        self._crl_number = crl_number

    crl_number = utils.read_only_property("_crl_number")

    def __eq__(self, other):
        if not isinstance(other, DeltaCRLIndicator):
            return NotImplemented

        return self.crl_number == other.crl_number

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(self.crl_number)

    def __repr__(self):
        return "<DeltaCRLIndicator(crl_number={0.crl_number})>".format(self)


@utils.register_interface(ExtensionType)
class CRLDistributionPoints(object):
    oid = ExtensionOID.CRL_DISTRIBUTION_POINTS

    def __init__(self, distribution_points):
        distribution_points = list(distribution_points)
        if not all(
            isinstance(x, DistributionPoint) for x in distribution_points
        ):
            raise TypeError(
                "distribution_points must be a list of DistributionPoint "
                "objects"
            )

        self._distribution_points = distribution_points

    __len__, __iter__, __getitem__ = _make_sequence_methods(
        "_distribution_points"
    )

    def __repr__(self):
        return "<CRLDistributionPoints({})>".format(self._distribution_points)

    def __eq__(self, other):
        if not isinstance(other, CRLDistributionPoints):
            return NotImplemented

        return self._distribution_points == other._distribution_points

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(tuple(self._distribution_points))


@utils.register_interface(ExtensionType)
class FreshestCRL(object):
    oid = ExtensionOID.FRESHEST_CRL

    def __init__(self, distribution_points):
        distribution_points = list(distribution_points)
        if not all(
            isinstance(x, DistributionPoint) for x in distribution_points
        ):
            raise TypeError(
                "distribution_points must be a list of DistributionPoint "
                "objects"
            )

        self._distribution_points = distribution_points

    __len__, __iter__, __getitem__ = _make_sequence_methods(
        "_distribution_points"
    )

    def __repr__(self):
        return "<FreshestCRL({})>".format(self._distribution_points)

    def __eq__(self, other):
        if not isinstance(other, FreshestCRL):
            return NotImplemented

        return self._distribution_points == other._distribution_points

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(tuple(self._distribution_points))


class DistributionPoint(object):
    def __init__(self, full_name, relative_name, reasons, crl_issuer):
        if full_name and relative_name:
            raise ValueError(
                "You cannot provide both full_name and relative_name, at "
                "least one must be None."
            )

        if full_name:
            full_name = list(full_name)
            if not all(isinstance(x, GeneralName) for x in full_name):
                raise TypeError(
                    "full_name must be a list of GeneralName objects"
                )

        if relative_name:
            if not isinstance(relative_name, RelativeDistinguishedName):
                raise TypeError(
                    "relative_name must be a RelativeDistinguishedName"
                )

        if crl_issuer:
            crl_issuer = list(crl_issuer)
            if not all(isinstance(x, GeneralName) for x in crl_issuer):
                raise TypeError(
                    "crl_issuer must be None or a list of general names"
                )

        if reasons and (
            not isinstance(reasons, frozenset)
            or not all(isinstance(x, ReasonFlags) for x in reasons)
        ):
            raise TypeError("reasons must be None or frozenset of ReasonFlags")

        if reasons and (
            ReasonFlags.unspecified in reasons
            or ReasonFlags.remove_from_crl in reasons
        ):
            raise ValueError(
                "unspecified and remove_from_crl are not valid reasons in a "
                "DistributionPoint"
            )

        if reasons and not crl_issuer and not (full_name or relative_name):
            raise ValueError(
                "You must supply crl_issuer, full_name, or relative_name when "
                "reasons is not None"
            )

        self._full_name = full_name
        self._relative_name = relative_name
        self._reasons = reasons
        self._crl_issuer = crl_issuer

    def __repr__(self):
        return (
            "<DistributionPoint(full_name={0.full_name}, relative_name={0.rela"
            "tive_name}, reasons={0.reasons}, "
            "crl_issuer={0.crl_issuer})>".format(self)
        )

    def __eq__(self, other):
        if not isinstance(other, DistributionPoint):
            return NotImplemented

        return (
            self.full_name == other.full_name
            and self.relative_name == other.relative_name
            and self.reasons == other.reasons
            and self.crl_issuer == other.crl_issuer
        )

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        if self.full_name is not None:
            fn = tuple(self.full_name)
        else:
            fn = None

        if self.crl_issuer is not None:
            crl_issuer = tuple(self.crl_issuer)
        else:
            crl_issuer = None

        return hash((fn, self.relative_name, self.reasons, crl_issuer))

    full_name = utils.read_only_property("_full_name")
    relative_name = utils.read_only_property("_relative_name")
    reasons = utils.read_only_property("_reasons")
    crl_issuer = utils.read_only_property("_crl_issuer")


class ReasonFlags(Enum):
    unspecified = "unspecified"
    key_compromise = "keyCompromise"
    ca_compromise = "cACompromise"
    affiliation_changed = "affiliationChanged"
    superseded = "superseded"
    cessation_of_operation = "cessationOfOperation"
    certificate_hold = "certificateHold"
    privilege_withdrawn = "privilegeWithdrawn"
    aa_compromise = "aACompromise"
    remove_from_crl = "removeFromCRL"


@utils.register_interface(ExtensionType)
class PolicyConstraints(object):
    oid = ExtensionOID.POLICY_CONSTRAINTS

    def __init__(self, require_explicit_policy, inhibit_policy_mapping):
        if require_explicit_policy is not None and not isinstance(
            require_explicit_policy, six.integer_types
        ):
            raise TypeError(
                "require_explicit_policy must be a non-negative integer or "
                "None"
            )

        if inhibit_policy_mapping is not None and not isinstance(
            inhibit_policy_mapping, six.integer_types
        ):
            raise TypeError(
                "inhibit_policy_mapping must be a non-negative integer or None"
            )

        if inhibit_policy_mapping is None and require_explicit_policy is None:
            raise ValueError(
                "At least one of require_explicit_policy and "
                "inhibit_policy_mapping must not be None"
            )

        self._require_explicit_policy = require_explicit_policy
        self._inhibit_policy_mapping = inhibit_policy_mapping

    def __repr__(self):
        return (
            u"<PolicyConstraints(require_explicit_policy={0.require_explicit"
            u"_policy}, inhibit_policy_mapping={0.inhibit_policy_"
            u"mapping})>".format(self)
        )

    def __eq__(self, other):
        if not isinstance(other, PolicyConstraints):
            return NotImplemented

        return (
            self.require_explicit_policy == other.require_explicit_policy
            and self.inhibit_policy_mapping == other.inhibit_policy_mapping
        )

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(
            (self.require_explicit_policy, self.inhibit_policy_mapping)
        )

    require_explicit_policy = utils.read_only_property(
        "_require_explicit_policy"
    )
    inhibit_policy_mapping = utils.read_only_property(
        "_inhibit_policy_mapping"
    )


@utils.register_interface(ExtensionType)
class CertificatePolicies(object):
    oid = ExtensionOID.CERTIFICATE_POLICIES

    def __init__(self, policies):
        policies = list(policies)
        if not all(isinstance(x, PolicyInformation) for x in policies):
            raise TypeError(
                "Every item in the policies list must be a "
                "PolicyInformation"
            )

        self._policies = policies

    __len__, __iter__, __getitem__ = _make_sequence_methods("_policies")

    def __repr__(self):
        return "<CertificatePolicies({})>".format(self._policies)

    def __eq__(self, other):
        if not isinstance(other, CertificatePolicies):
            return NotImplemented

        return self._policies == other._policies

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(tuple(self._policies))


class PolicyInformation(object):
    def __init__(self, policy_identifier, policy_qualifiers):
        if not isinstance(policy_identifier, ObjectIdentifier):
            raise TypeError("policy_identifier must be an ObjectIdentifier")

        self._policy_identifier = policy_identifier

        if policy_qualifiers:
            policy_qualifiers = list(policy_qualifiers)
            if not all(
                isinstance(x, (six.text_type, UserNotice))
                for x in policy_qualifiers
            ):
                raise TypeError(
                    "policy_qualifiers must be a list of strings and/or "
                    "UserNotice objects or None"
                )

        self._policy_qualifiers = policy_qualifiers

    def __repr__(self):
        return (
            "<PolicyInformation(policy_identifier={0.policy_identifier}, polic"
            "y_qualifiers={0.policy_qualifiers})>".format(self)
        )

    def __eq__(self, other):
        if not isinstance(other, PolicyInformation):
            return NotImplemented

        return (
            self.policy_identifier == other.policy_identifier
            and self.policy_qualifiers == other.policy_qualifiers
        )

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        if self.policy_qualifiers is not None:
            pq = tuple(self.policy_qualifiers)
        else:
            pq = None

        return hash((self.policy_identifier, pq))

    policy_identifier = utils.read_only_property("_policy_identifier")
    policy_qualifiers = utils.read_only_property("_policy_qualifiers")


class UserNotice(object):
    def __init__(self, notice_reference, explicit_text):
        if notice_reference and not isinstance(
            notice_reference, NoticeReference
        ):
            raise TypeError(
                "notice_reference must be None or a NoticeReference"
            )

        self._notice_reference = notice_reference
        self._explicit_text = explicit_text

    def __repr__(self):
        return (
            "<UserNotice(notice_reference={0.notice_reference}, explicit_text="
            "{0.explicit_text!r})>".format(self)
        )

    def __eq__(self, other):
        if not isinstance(other, UserNotice):
            return NotImplemented

        return (
            self.notice_reference == other.notice_reference
            and self.explicit_text == other.explicit_text
        )

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash((self.notice_reference, self.explicit_text))

    notice_reference = utils.read_only_property("_notice_reference")
    explicit_text = utils.read_only_property("_explicit_text")


class NoticeReference(object):
    def __init__(self, organization, notice_numbers):
        self._organization = organization
        notice_numbers = list(notice_numbers)
        if not all(isinstance(x, int) for x in notice_numbers):
            raise TypeError("notice_numbers must be a list of integers")

        self._notice_numbers = notice_numbers

    def __repr__(self):
        return (
            "<NoticeReference(organization={0.organization!r}, notice_numbers="
            "{0.notice_numbers})>".format(self)
        )

    def __eq__(self, other):
        if not isinstance(other, NoticeReference):
            return NotImplemented

        return (
            self.organization == other.organization
            and self.notice_numbers == other.notice_numbers
        )

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash((self.organization, tuple(self.notice_numbers)))

    organization = utils.read_only_property("_organization")
    notice_numbers = utils.read_only_property("_notice_numbers")


@utils.register_interface(ExtensionType)
class ExtendedKeyUsage(object):
    oid = ExtensionOID.EXTENDED_KEY_USAGE

    def __init__(self, usages):
        usages = list(usages)
        if not all(isinstance(x, ObjectIdentifier) for x in usages):
            raise TypeError(
                "Every item in the usages list must be an ObjectIdentifier"
            )

        self._usages = usages

    __len__, __iter__, __getitem__ = _make_sequence_methods("_usages")

    def __repr__(self):
        return "<ExtendedKeyUsage({})>".format(self._usages)

    def __eq__(self, other):
        if not isinstance(other, ExtendedKeyUsage):
            return NotImplemented

        return self._usages == other._usages

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(tuple(self._usages))


@utils.register_interface(ExtensionType)
class OCSPNoCheck(object):
    oid = ExtensionOID.OCSP_NO_CHECK

    def __eq__(self, other):
        if not isinstance(other, OCSPNoCheck):
            return NotImplemented

        return True

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(OCSPNoCheck)

    def __repr__(self):
        return "<OCSPNoCheck()>"


@utils.register_interface(ExtensionType)
class PrecertPoison(object):
    oid = ExtensionOID.PRECERT_POISON

    def __eq__(self, other):
        if not isinstance(other, PrecertPoison):
            return NotImplemented

        return True

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(PrecertPoison)

    def __repr__(self):
        return "<PrecertPoison()>"


@utils.register_interface(ExtensionType)
class TLSFeature(object):
    oid = ExtensionOID.TLS_FEATURE

    def __init__(self, features):
        features = list(features)
        if (
            not all(isinstance(x, TLSFeatureType) for x in features)
            or len(features) == 0
        ):
            raise TypeError(
                "features must be a list of elements from the TLSFeatureType "
                "enum"
            )

        self._features = features

    __len__, __iter__, __getitem__ = _make_sequence_methods("_features")

    def __repr__(self):
        return "<TLSFeature(features={0._features})>".format(self)

    def __eq__(self, other):
        if not isinstance(other, TLSFeature):
            return NotImplemented

        return self._features == other._features

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(tuple(self._features))


class TLSFeatureType(Enum):
    # status_request is defined in RFC 6066 and is used for what is commonly
    # called OCSP Must-Staple when present in the TLS Feature extension in an
    # X.509 certificate.
    status_request = 5
    # status_request_v2 is defined in RFC 6961 and allows multiple OCSP
    # responses to be provided. It is not currently in use by clients or
    # servers.
    status_request_v2 = 17


_TLS_FEATURE_TYPE_TO_ENUM = {x.value: x for x in TLSFeatureType}


@utils.register_interface(ExtensionType)
class InhibitAnyPolicy(object):
    oid = ExtensionOID.INHIBIT_ANY_POLICY

    def __init__(self, skip_certs):
        if not isinstance(skip_certs, six.integer_types):
            raise TypeError("skip_certs must be an integer")

        if skip_certs < 0:
            raise ValueError("skip_certs must be a non-negative integer")

        self._skip_certs = skip_certs

    def __repr__(self):
        return "<InhibitAnyPolicy(skip_certs={0.skip_certs})>".format(self)

    def __eq__(self, other):
        if not isinstance(other, InhibitAnyPolicy):
            return NotImplemented

        return self.skip_certs == other.skip_certs

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(self.skip_certs)

    skip_certs = utils.read_only_property("_skip_certs")


@utils.register_interface(ExtensionType)
class KeyUsage(object):
    oid = ExtensionOID.KEY_USAGE

    def __init__(
        self,
        digital_signature,
        content_commitment,
        key_encipherment,
        data_encipherment,
        key_agreement,
        key_cert_sign,
        crl_sign,
        encipher_only,
        decipher_only,
    ):
        if not key_agreement and (encipher_only or decipher_only):
            raise ValueError(
                "encipher_only and decipher_only can only be true when "
                "key_agreement is true"
            )

        self._digital_signature = digital_signature
        self._content_commitment = content_commitment
        self._key_encipherment = key_encipherment
        self._data_encipherment = data_encipherment
        self._key_agreement = key_agreement
        self._key_cert_sign = key_cert_sign
        self._crl_sign = crl_sign
        self._encipher_only = encipher_only
        self._decipher_only = decipher_only

    digital_signature = utils.read_only_property("_digital_signature")
    content_commitment = utils.read_only_property("_content_commitment")
    key_encipherment = utils.read_only_property("_key_encipherment")
    data_encipherment = utils.read_only_property("_data_encipherment")
    key_agreement = utils.read_only_property("_key_agreement")
    key_cert_sign = utils.read_only_property("_key_cert_sign")
    crl_sign = utils.read_only_property("_crl_sign")

    @property
    def encipher_only(self):
        if not self.key_agreement:
            raise ValueError(
                "encipher_only is undefined unless key_agreement is true"
            )
        else:
            return self._encipher_only

    @property
    def decipher_only(self):
        if not self.key_agreement:
            raise ValueError(
                "decipher_only is undefined unless key_agreement is true"
            )
        else:
            return self._decipher_only

    def __repr__(self):
        try:
            encipher_only = self.encipher_only
            decipher_only = self.decipher_only
        except ValueError:
            # Users found None confusing because even though encipher/decipher
            # have no meaning unless key_agreement is true, to construct an
            # instance of the class you still need to pass False.
            encipher_only = False
            decipher_only = False

        return (
            "<KeyUsage(digital_signature={0.digital_signature}, "
            "content_commitment={0.content_commitment}, "
            "key_encipherment={0.key_encipherment}, "
            "data_encipherment={0.data_encipherment}, "
            "key_agreement={0.key_agreement}, "
            "key_cert_sign={0.key_cert_sign}, crl_sign={0.crl_sign}, "
            "encipher_only={1}, decipher_only={2})>"
        ).format(self, encipher_only, decipher_only)

    def __eq__(self, other):
        if not isinstance(other, KeyUsage):
            return NotImplemented

        return (
            self.digital_signature == other.digital_signature
            and self.content_commitment == other.content_commitment
            and self.key_encipherment == other.key_encipherment
            and self.data_encipherment == other.data_encipherment
            and self.key_agreement == other.key_agreement
            and self.key_cert_sign == other.key_cert_sign
            and self.crl_sign == other.crl_sign
            and self._encipher_only == other._encipher_only
            and self._decipher_only == other._decipher_only
        )

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(
            (
                self.digital_signature,
                self.content_commitment,
                self.key_encipherment,
                self.data_encipherment,
                self.key_agreement,
                self.key_cert_sign,
                self.crl_sign,
                self._encipher_only,
                self._decipher_only,
            )
        )


@utils.register_interface(ExtensionType)
class NameConstraints(object):
    oid = ExtensionOID.NAME_CONSTRAINTS

    def __init__(self, permitted_subtrees, excluded_subtrees):
        if permitted_subtrees is not None:
            permitted_subtrees = list(permitted_subtrees)
            if not all(isinstance(x, GeneralName) for x in permitted_subtrees):
                raise TypeError(
                    "permitted_subtrees must be a list of GeneralName objects "
                    "or None"
                )

            self._validate_ip_name(permitted_subtrees)

        if excluded_subtrees is not None:
            excluded_subtrees = list(excluded_subtrees)
            if not all(isinstance(x, GeneralName) for x in excluded_subtrees):
                raise TypeError(
                    "excluded_subtrees must be a list of GeneralName objects "
                    "or None"
                )

            self._validate_ip_name(excluded_subtrees)

        if permitted_subtrees is None and excluded_subtrees is None:
            raise ValueError(
                "At least one of permitted_subtrees and excluded_subtrees "
                "must not be None"
            )

        self._permitted_subtrees = permitted_subtrees
        self._excluded_subtrees = excluded_subtrees

    def __eq__(self, other):
        if not isinstance(other, NameConstraints):
            return NotImplemented

        return (
            self.excluded_subtrees == other.excluded_subtrees
            and self.permitted_subtrees == other.permitted_subtrees
        )

    def __ne__(self, other):
        return not self == other

    def _validate_ip_name(self, tree):
        if any(
            isinstance(name, IPAddress)
            and not isinstance(
                name.value, (ipaddress.IPv4Network, ipaddress.IPv6Network)
            )
            for name in tree
        ):
            raise TypeError(
                "IPAddress name constraints must be an IPv4Network or"
                " IPv6Network object"
            )

    def __repr__(self):
        return (
            u"<NameConstraints(permitted_subtrees={0.permitted_subtrees}, "
            u"excluded_subtrees={0.excluded_subtrees})>".format(self)
        )

    def __hash__(self):
        if self.permitted_subtrees is not None:
            ps = tuple(self.permitted_subtrees)
        else:
            ps = None

        if self.excluded_subtrees is not None:
            es = tuple(self.excluded_subtrees)
        else:
            es = None

        return hash((ps, es))

    permitted_subtrees = utils.read_only_property("_permitted_subtrees")
    excluded_subtrees = utils.read_only_property("_excluded_subtrees")


class Extension(object):
    def __init__(self, oid, critical, value):
        if not isinstance(oid, ObjectIdentifier):
            raise TypeError(
                "oid argument must be an ObjectIdentifier instance."
            )

        if not isinstance(critical, bool):
            raise TypeError("critical must be a boolean value")

        self._oid = oid
        self._critical = critical
        self._value = value

    oid = utils.read_only_property("_oid")
    critical = utils.read_only_property("_critical")
    value = utils.read_only_property("_value")

    def __repr__(self):
        return (
            "<Extension(oid={0.oid}, critical={0.critical}, "
            "value={0.value})>"
        ).format(self)

    def __eq__(self, other):
        if not isinstance(other, Extension):
            return NotImplemented

        return (
            self.oid == other.oid
            and self.critical == other.critical
            and self.value == other.value
        )

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash((self.oid, self.critical, self.value))


class GeneralNames(object):
    def __init__(self, general_names):
        general_names = list(general_names)
        if not all(isinstance(x, GeneralName) for x in general_names):
            raise TypeError(
                "Every item in the general_names list must be an "
                "object conforming to the GeneralName interface"
            )

        self._general_names = general_names

    __len__, __iter__, __getitem__ = _make_sequence_methods("_general_names")

    def get_values_for_type(self, type):
        # Return the value of each GeneralName, except for OtherName instances
        # which we return directly because it has two important properties not
        # just one value.
        objs = (i for i in self if isinstance(i, type))
        if type != OtherName:
            objs = (i.value for i in objs)
        return list(objs)

    def __repr__(self):
        return "<GeneralNames({})>".format(self._general_names)

    def __eq__(self, other):
        if not isinstance(other, GeneralNames):
            return NotImplemented

        return self._general_names == other._general_names

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(tuple(self._general_names))


@utils.register_interface(ExtensionType)
class SubjectAlternativeName(object):
    oid = ExtensionOID.SUBJECT_ALTERNATIVE_NAME

    def __init__(self, general_names):
        self._general_names = GeneralNames(general_names)

    __len__, __iter__, __getitem__ = _make_sequence_methods("_general_names")

    def get_values_for_type(self, type):
        return self._general_names.get_values_for_type(type)

    def __repr__(self):
        return "<SubjectAlternativeName({})>".format(self._general_names)

    def __eq__(self, other):
        if not isinstance(other, SubjectAlternativeName):
            return NotImplemented

        return self._general_names == other._general_names

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(self._general_names)


@utils.register_interface(ExtensionType)
class IssuerAlternativeName(object):
    oid = ExtensionOID.ISSUER_ALTERNATIVE_NAME

    def __init__(self, general_names):
        self._general_names = GeneralNames(general_names)

    __len__, __iter__, __getitem__ = _make_sequence_methods("_general_names")

    def get_values_for_type(self, type):
        return self._general_names.get_values_for_type(type)

    def __repr__(self):
        return "<IssuerAlternativeName({})>".format(self._general_names)

    def __eq__(self, other):
        if not isinstance(other, IssuerAlternativeName):
            return NotImplemented

        return self._general_names == other._general_names

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(self._general_names)


@utils.register_interface(ExtensionType)
class CertificateIssuer(object):
    oid = CRLEntryExtensionOID.CERTIFICATE_ISSUER

    def __init__(self, general_names):
        self._general_names = GeneralNames(general_names)

    __len__, __iter__, __getitem__ = _make_sequence_methods("_general_names")

    def get_values_for_type(self, type):
        return self._general_names.get_values_for_type(type)

    def __repr__(self):
        return "<CertificateIssuer({})>".format(self._general_names)

    def __eq__(self, other):
        if not isinstance(other, CertificateIssuer):
            return NotImplemented

        return self._general_names == other._general_names

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(self._general_names)


@utils.register_interface(ExtensionType)
class CRLReason(object):
    oid = CRLEntryExtensionOID.CRL_REASON

    def __init__(self, reason):
        if not isinstance(reason, ReasonFlags):
            raise TypeError("reason must be an element from ReasonFlags")

        self._reason = reason

    def __repr__(self):
        return "<CRLReason(reason={})>".format(self._reason)

    def __eq__(self, other):
        if not isinstance(other, CRLReason):
            return NotImplemented

        return self.reason == other.reason

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(self.reason)

    reason = utils.read_only_property("_reason")


@utils.register_interface(ExtensionType)
class InvalidityDate(object):
    oid = CRLEntryExtensionOID.INVALIDITY_DATE

    def __init__(self, invalidity_date):
        if not isinstance(invalidity_date, datetime.datetime):
            raise TypeError("invalidity_date must be a datetime.datetime")

        self._invalidity_date = invalidity_date

    def __repr__(self):
        return "<InvalidityDate(invalidity_date={})>".format(
            self._invalidity_date
        )

    def __eq__(self, other):
        if not isinstance(other, InvalidityDate):
            return NotImplemented

        return self.invalidity_date == other.invalidity_date

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(self.invalidity_date)

    invalidity_date = utils.read_only_property("_invalidity_date")


@utils.register_interface(ExtensionType)
class PrecertificateSignedCertificateTimestamps(object):
    oid = ExtensionOID.PRECERT_SIGNED_CERTIFICATE_TIMESTAMPS

    def __init__(self, signed_certificate_timestamps):
        signed_certificate_timestamps = list(signed_certificate_timestamps)
        if not all(
            isinstance(sct, SignedCertificateTimestamp)
            for sct in signed_certificate_timestamps
        ):
            raise TypeError(
                "Every item in the signed_certificate_timestamps list must be "
                "a SignedCertificateTimestamp"
            )
        self._signed_certificate_timestamps = signed_certificate_timestamps

    __len__, __iter__, __getitem__ = _make_sequence_methods(
        "_signed_certificate_timestamps"
    )

    def __repr__(self):
        return "<PrecertificateSignedCertificateTimestamps({})>".format(
            list(self)
        )

    def __hash__(self):
        return hash(tuple(self._signed_certificate_timestamps))

    def __eq__(self, other):
        if not isinstance(other, PrecertificateSignedCertificateTimestamps):
            return NotImplemented

        return (
            self._signed_certificate_timestamps
            == other._signed_certificate_timestamps
        )

    def __ne__(self, other):
        return not self == other


@utils.register_interface(ExtensionType)
class SignedCertificateTimestamps(object):
    oid = ExtensionOID.SIGNED_CERTIFICATE_TIMESTAMPS

    def __init__(self, signed_certificate_timestamps):
        signed_certificate_timestamps = list(signed_certificate_timestamps)
        if not all(
            isinstance(sct, SignedCertificateTimestamp)
            for sct in signed_certificate_timestamps
        ):
            raise TypeError(
                "Every item in the signed_certificate_timestamps list must be "
                "a SignedCertificateTimestamp"
            )
        self._signed_certificate_timestamps = signed_certificate_timestamps

    __len__, __iter__, __getitem__ = _make_sequence_methods(
        "_signed_certificate_timestamps"
    )

    def __repr__(self):
        return "<SignedCertificateTimestamps({})>".format(list(self))

    def __hash__(self):
        return hash(tuple(self._signed_certificate_timestamps))

    def __eq__(self, other):
        if not isinstance(other, SignedCertificateTimestamps):
            return NotImplemented

        return (
            self._signed_certificate_timestamps
            == other._signed_certificate_timestamps
        )

    def __ne__(self, other):
        return not self == other


@utils.register_interface(ExtensionType)
class OCSPNonce(object):
    oid = OCSPExtensionOID.NONCE

    def __init__(self, nonce):
        if not isinstance(nonce, bytes):
            raise TypeError("nonce must be bytes")

        self._nonce = nonce

    def __eq__(self, other):
        if not isinstance(other, OCSPNonce):
            return NotImplemented

        return self.nonce == other.nonce

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(self.nonce)

    def __repr__(self):
        return "<OCSPNonce(nonce={0.nonce!r})>".format(self)

    nonce = utils.read_only_property("_nonce")


@utils.register_interface(ExtensionType)
class IssuingDistributionPoint(object):
    oid = ExtensionOID.ISSUING_DISTRIBUTION_POINT

    def __init__(
        self,
        full_name,
        relative_name,
        only_contains_user_certs,
        only_contains_ca_certs,
        only_some_reasons,
        indirect_crl,
        only_contains_attribute_certs,
    ):
        if only_some_reasons and (
            not isinstance(only_some_reasons, frozenset)
            or not all(isinstance(x, ReasonFlags) for x in only_some_reasons)
        ):
            raise TypeError(
                "only_some_reasons must be None or frozenset of ReasonFlags"
            )

        if only_some_reasons and (
            ReasonFlags.unspecified in only_some_reasons
            or ReasonFlags.remove_from_crl in only_some_reasons
        ):
            raise ValueError(
                "unspecified and remove_from_crl are not valid reasons in an "
                "IssuingDistributionPoint"
            )

        if not (
            isinstance(only_contains_user_certs, bool)
            and isinstance(only_contains_ca_certs, bool)
            and isinstance(indirect_crl, bool)
            and isinstance(only_contains_attribute_certs, bool)
        ):
            raise TypeError(
                "only_contains_user_certs, only_contains_ca_certs, "
                "indirect_crl and only_contains_attribute_certs "
                "must all be boolean."
            )

        crl_constraints = [
            only_contains_user_certs,
            only_contains_ca_certs,
            indirect_crl,
            only_contains_attribute_certs,
        ]

        if len([x for x in crl_constraints if x]) > 1:
            raise ValueError(
                "Only one of the following can be set to True: "
                "only_contains_user_certs, only_contains_ca_certs, "
                "indirect_crl, only_contains_attribute_certs"
            )

        if not any(
            [
                only_contains_user_certs,
                only_contains_ca_certs,
                indirect_crl,
                only_contains_attribute_certs,
                full_name,
                relative_name,
                only_some_reasons,
            ]
        ):
            raise ValueError(
                "Cannot create empty extension: "
                "if only_contains_user_certs, only_contains_ca_certs, "
                "indirect_crl, and only_contains_attribute_certs are all False"
                ", then either full_name, relative_name, or only_some_reasons "
                "must have a value."
            )

        self._only_contains_user_certs = only_contains_user_certs
        self._only_contains_ca_certs = only_contains_ca_certs
        self._indirect_crl = indirect_crl
        self._only_contains_attribute_certs = only_contains_attribute_certs
        self._only_some_reasons = only_some_reasons
        self._full_name = full_name
        self._relative_name = relative_name

    def __repr__(self):
        return (
            "<IssuingDistributionPoint(full_name={0.full_name}, "
            "relative_name={0.relative_name}, "
            "only_contains_user_certs={0.only_contains_user_certs}, "
            "only_contains_ca_certs={0.only_contains_ca_certs}, "
            "only_some_reasons={0.only_some_reasons}, "
            "indirect_crl={0.indirect_crl}, "
            "only_contains_attribute_certs="
            "{0.only_contains_attribute_certs})>".format(self)
        )

    def __eq__(self, other):
        if not isinstance(other, IssuingDistributionPoint):
            return NotImplemented

        return (
            self.full_name == other.full_name
            and self.relative_name == other.relative_name
            and self.only_contains_user_certs == other.only_contains_user_certs
            and self.only_contains_ca_certs == other.only_contains_ca_certs
            and self.only_some_reasons == other.only_some_reasons
            and self.indirect_crl == other.indirect_crl
            and self.only_contains_attribute_certs
            == other.only_contains_attribute_certs
        )

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(
            (
                self.full_name,
                self.relative_name,
                self.only_contains_user_certs,
                self.only_contains_ca_certs,
                self.only_some_reasons,
                self.indirect_crl,
                self.only_contains_attribute_certs,
            )
        )

    full_name = utils.read_only_property("_full_name")
    relative_name = utils.read_only_property("_relative_name")
    only_contains_user_certs = utils.read_only_property(
        "_only_contains_user_certs"
    )
    only_contains_ca_certs = utils.read_only_property(
        "_only_contains_ca_certs"
    )
    only_some_reasons = utils.read_only_property("_only_some_reasons")
    indirect_crl = utils.read_only_property("_indirect_crl")
    only_contains_attribute_certs = utils.read_only_property(
        "_only_contains_attribute_certs"
    )


@utils.register_interface(ExtensionType)
class UnrecognizedExtension(object):
    def __init__(self, oid, value):
        if not isinstance(oid, ObjectIdentifier):
            raise TypeError("oid must be an ObjectIdentifier")
        self._oid = oid
        self._value = value

    oid = utils.read_only_property("_oid")
    value = utils.read_only_property("_value")

    def __repr__(self):
        return (
            "<UnrecognizedExtension(oid={0.oid}, "
            "value={0.value!r})>".format(self)
        )

    def __eq__(self, other):
        if not isinstance(other, UnrecognizedExtension):
            return NotImplemented

        return self.oid == other.oid and self.value == other.value

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash((self.oid, self.value))
