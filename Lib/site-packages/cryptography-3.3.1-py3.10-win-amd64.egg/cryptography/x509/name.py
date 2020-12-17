# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

from enum import Enum

import six

from cryptography import utils
from cryptography.hazmat.backends import _get_backend
from cryptography.x509.oid import NameOID, ObjectIdentifier


class _ASN1Type(Enum):
    UTF8String = 12
    NumericString = 18
    PrintableString = 19
    T61String = 20
    IA5String = 22
    UTCTime = 23
    GeneralizedTime = 24
    VisibleString = 26
    UniversalString = 28
    BMPString = 30


_ASN1_TYPE_TO_ENUM = {i.value: i for i in _ASN1Type}
_SENTINEL = object()
_NAMEOID_DEFAULT_TYPE = {
    NameOID.COUNTRY_NAME: _ASN1Type.PrintableString,
    NameOID.JURISDICTION_COUNTRY_NAME: _ASN1Type.PrintableString,
    NameOID.SERIAL_NUMBER: _ASN1Type.PrintableString,
    NameOID.DN_QUALIFIER: _ASN1Type.PrintableString,
    NameOID.EMAIL_ADDRESS: _ASN1Type.IA5String,
    NameOID.DOMAIN_COMPONENT: _ASN1Type.IA5String,
}

#: Short attribute names from RFC 4514:
#: https://tools.ietf.org/html/rfc4514#page-7
_NAMEOID_TO_NAME = {
    NameOID.COMMON_NAME: "CN",
    NameOID.LOCALITY_NAME: "L",
    NameOID.STATE_OR_PROVINCE_NAME: "ST",
    NameOID.ORGANIZATION_NAME: "O",
    NameOID.ORGANIZATIONAL_UNIT_NAME: "OU",
    NameOID.COUNTRY_NAME: "C",
    NameOID.STREET_ADDRESS: "STREET",
    NameOID.DOMAIN_COMPONENT: "DC",
    NameOID.USER_ID: "UID",
}


def _escape_dn_value(val):
    """Escape special characters in RFC4514 Distinguished Name value."""

    if not val:
        return ""

    # See https://tools.ietf.org/html/rfc4514#section-2.4
    val = val.replace("\\", "\\\\")
    val = val.replace('"', '\\"')
    val = val.replace("+", "\\+")
    val = val.replace(",", "\\,")
    val = val.replace(";", "\\;")
    val = val.replace("<", "\\<")
    val = val.replace(">", "\\>")
    val = val.replace("\0", "\\00")

    if val[0] in ("#", " "):
        val = "\\" + val
    if val[-1] == " ":
        val = val[:-1] + "\\ "

    return val


class NameAttribute(object):
    def __init__(self, oid, value, _type=_SENTINEL):
        if not isinstance(oid, ObjectIdentifier):
            raise TypeError(
                "oid argument must be an ObjectIdentifier instance."
            )

        if not isinstance(value, six.text_type):
            raise TypeError("value argument must be a text type.")

        if (
            oid == NameOID.COUNTRY_NAME
            or oid == NameOID.JURISDICTION_COUNTRY_NAME
        ):
            if len(value.encode("utf8")) != 2:
                raise ValueError(
                    "Country name must be a 2 character country code"
                )

        # The appropriate ASN1 string type varies by OID and is defined across
        # multiple RFCs including 2459, 3280, and 5280. In general UTF8String
        # is preferred (2459), but 3280 and 5280 specify several OIDs with
        # alternate types. This means when we see the sentinel value we need
        # to look up whether the OID has a non-UTF8 type. If it does, set it
        # to that. Otherwise, UTF8!
        if _type == _SENTINEL:
            _type = _NAMEOID_DEFAULT_TYPE.get(oid, _ASN1Type.UTF8String)

        if not isinstance(_type, _ASN1Type):
            raise TypeError("_type must be from the _ASN1Type enum")

        self._oid = oid
        self._value = value
        self._type = _type

    oid = utils.read_only_property("_oid")
    value = utils.read_only_property("_value")

    def rfc4514_string(self):
        """
        Format as RFC4514 Distinguished Name string.

        Use short attribute name if available, otherwise fall back to OID
        dotted string.
        """
        key = _NAMEOID_TO_NAME.get(self.oid, self.oid.dotted_string)
        return "%s=%s" % (key, _escape_dn_value(self.value))

    def __eq__(self, other):
        if not isinstance(other, NameAttribute):
            return NotImplemented

        return self.oid == other.oid and self.value == other.value

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash((self.oid, self.value))

    def __repr__(self):
        return "<NameAttribute(oid={0.oid}, value={0.value!r})>".format(self)


class RelativeDistinguishedName(object):
    def __init__(self, attributes):
        attributes = list(attributes)
        if not attributes:
            raise ValueError("a relative distinguished name cannot be empty")
        if not all(isinstance(x, NameAttribute) for x in attributes):
            raise TypeError("attributes must be an iterable of NameAttribute")

        # Keep list and frozenset to preserve attribute order where it matters
        self._attributes = attributes
        self._attribute_set = frozenset(attributes)

        if len(self._attribute_set) != len(attributes):
            raise ValueError("duplicate attributes are not allowed")

    def get_attributes_for_oid(self, oid):
        return [i for i in self if i.oid == oid]

    def rfc4514_string(self):
        """
        Format as RFC4514 Distinguished Name string.

        Within each RDN, attributes are joined by '+', although that is rarely
        used in certificates.
        """
        return "+".join(attr.rfc4514_string() for attr in self._attributes)

    def __eq__(self, other):
        if not isinstance(other, RelativeDistinguishedName):
            return NotImplemented

        return self._attribute_set == other._attribute_set

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        return hash(self._attribute_set)

    def __iter__(self):
        return iter(self._attributes)

    def __len__(self):
        return len(self._attributes)

    def __repr__(self):
        return "<RelativeDistinguishedName({})>".format(self.rfc4514_string())


class Name(object):
    def __init__(self, attributes):
        attributes = list(attributes)
        if all(isinstance(x, NameAttribute) for x in attributes):
            self._attributes = [
                RelativeDistinguishedName([x]) for x in attributes
            ]
        elif all(isinstance(x, RelativeDistinguishedName) for x in attributes):
            self._attributes = attributes
        else:
            raise TypeError(
                "attributes must be a list of NameAttribute"
                " or a list RelativeDistinguishedName"
            )

    def rfc4514_string(self):
        """
        Format as RFC4514 Distinguished Name string.
        For example 'CN=foobar.com,O=Foo Corp,C=US'

        An X.509 name is a two-level structure: a list of sets of attributes.
        Each list element is separated by ',' and within each list element, set
        elements are separated by '+'. The latter is almost never used in
        real world certificates. According to RFC4514 section 2.1 the
        RDNSequence must be reversed when converting to string representation.
        """
        return ",".join(
            attr.rfc4514_string() for attr in reversed(self._attributes)
        )

    def get_attributes_for_oid(self, oid):
        return [i for i in self if i.oid == oid]

    @property
    def rdns(self):
        return self._attributes

    def public_bytes(self, backend=None):
        backend = _get_backend(backend)
        return backend.x509_name_bytes(self)

    def __eq__(self, other):
        if not isinstance(other, Name):
            return NotImplemented

        return self._attributes == other._attributes

    def __ne__(self, other):
        return not self == other

    def __hash__(self):
        # TODO: this is relatively expensive, if this looks like a bottleneck
        # for you, consider optimizing!
        return hash(tuple(self._attributes))

    def __iter__(self):
        for rdn in self._attributes:
            for ava in rdn:
                yield ava

    def __len__(self):
        return sum(len(rdn) for rdn in self._attributes)

    def __repr__(self):
        rdns = ",".join(attr.rfc4514_string() for attr in self._attributes)

        if six.PY2:
            return "<Name({})>".format(rdns.encode("utf8"))
        else:
            return "<Name({})>".format(rdns)
