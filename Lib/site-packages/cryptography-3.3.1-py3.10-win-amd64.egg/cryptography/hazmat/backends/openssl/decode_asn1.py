# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import datetime
import ipaddress

import six

from cryptography import x509
from cryptography.hazmat._der import DERReader, INTEGER, NULL, SEQUENCE
from cryptography.x509.extensions import _TLS_FEATURE_TYPE_TO_ENUM
from cryptography.x509.name import _ASN1_TYPE_TO_ENUM
from cryptography.x509.oid import (
    CRLEntryExtensionOID,
    CertificatePoliciesOID,
    ExtensionOID,
    OCSPExtensionOID,
)


def _obj2txt(backend, obj):
    # Set to 80 on the recommendation of
    # https://www.openssl.org/docs/crypto/OBJ_nid2ln.html#return_values
    #
    # But OIDs longer than this occur in real life (e.g. Active
    # Directory makes some very long OIDs).  So we need to detect
    # and properly handle the case where the default buffer is not
    # big enough.
    #
    buf_len = 80
    buf = backend._ffi.new("char[]", buf_len)

    # 'res' is the number of bytes that *would* be written if the
    # buffer is large enough.  If 'res' > buf_len - 1, we need to
    # alloc a big-enough buffer and go again.
    res = backend._lib.OBJ_obj2txt(buf, buf_len, obj, 1)
    if res > buf_len - 1:  # account for terminating null byte
        buf_len = res + 1
        buf = backend._ffi.new("char[]", buf_len)
        res = backend._lib.OBJ_obj2txt(buf, buf_len, obj, 1)
    backend.openssl_assert(res > 0)
    return backend._ffi.buffer(buf, res)[:].decode()


def _decode_x509_name_entry(backend, x509_name_entry):
    obj = backend._lib.X509_NAME_ENTRY_get_object(x509_name_entry)
    backend.openssl_assert(obj != backend._ffi.NULL)
    data = backend._lib.X509_NAME_ENTRY_get_data(x509_name_entry)
    backend.openssl_assert(data != backend._ffi.NULL)
    value = _asn1_string_to_utf8(backend, data)
    oid = _obj2txt(backend, obj)
    type = _ASN1_TYPE_TO_ENUM[data.type]

    return x509.NameAttribute(x509.ObjectIdentifier(oid), value, type)


def _decode_x509_name(backend, x509_name):
    count = backend._lib.X509_NAME_entry_count(x509_name)
    attributes = []
    prev_set_id = -1
    for x in range(count):
        entry = backend._lib.X509_NAME_get_entry(x509_name, x)
        attribute = _decode_x509_name_entry(backend, entry)
        set_id = backend._lib.X509_NAME_ENTRY_set(entry)
        if set_id != prev_set_id:
            attributes.append({attribute})
        else:
            # is in the same RDN a previous entry
            attributes[-1].add(attribute)
        prev_set_id = set_id

    return x509.Name(x509.RelativeDistinguishedName(rdn) for rdn in attributes)


def _decode_general_names(backend, gns):
    num = backend._lib.sk_GENERAL_NAME_num(gns)
    names = []
    for i in range(num):
        gn = backend._lib.sk_GENERAL_NAME_value(gns, i)
        backend.openssl_assert(gn != backend._ffi.NULL)
        names.append(_decode_general_name(backend, gn))

    return names


def _decode_general_name(backend, gn):
    if gn.type == backend._lib.GEN_DNS:
        # Convert to bytes and then decode to utf8. We don't use
        # asn1_string_to_utf8 here because it doesn't properly convert
        # utf8 from ia5strings.
        data = _asn1_string_to_bytes(backend, gn.d.dNSName).decode("utf8")
        # We don't use the constructor for DNSName so we can bypass validation
        # This allows us to create DNSName objects that have unicode chars
        # when a certificate (against the RFC) contains them.
        return x509.DNSName._init_without_validation(data)
    elif gn.type == backend._lib.GEN_URI:
        # Convert to bytes and then decode to utf8. We don't use
        # asn1_string_to_utf8 here because it doesn't properly convert
        # utf8 from ia5strings.
        data = _asn1_string_to_bytes(
            backend, gn.d.uniformResourceIdentifier
        ).decode("utf8")
        # We don't use the constructor for URI so we can bypass validation
        # This allows us to create URI objects that have unicode chars
        # when a certificate (against the RFC) contains them.
        return x509.UniformResourceIdentifier._init_without_validation(data)
    elif gn.type == backend._lib.GEN_RID:
        oid = _obj2txt(backend, gn.d.registeredID)
        return x509.RegisteredID(x509.ObjectIdentifier(oid))
    elif gn.type == backend._lib.GEN_IPADD:
        data = _asn1_string_to_bytes(backend, gn.d.iPAddress)
        data_len = len(data)
        if data_len == 8 or data_len == 32:
            # This is an IPv4 or IPv6 Network and not a single IP. This
            # type of data appears in Name Constraints. Unfortunately,
            # ipaddress doesn't support packed bytes + netmask. Additionally,
            # IPv6Network can only handle CIDR rather than the full 16 byte
            # netmask. To handle this we convert the netmask to integer, then
            # find the first 0 bit, which will be the prefix. If another 1
            # bit is present after that the netmask is invalid.
            base = ipaddress.ip_address(data[: data_len // 2])
            netmask = ipaddress.ip_address(data[data_len // 2 :])
            bits = bin(int(netmask))[2:]
            prefix = bits.find("0")
            # If no 0 bits are found it is a /32 or /128
            if prefix == -1:
                prefix = len(bits)

            if "1" in bits[prefix:]:
                raise ValueError("Invalid netmask")

            ip = ipaddress.ip_network(base.exploded + u"/{}".format(prefix))
        else:
            ip = ipaddress.ip_address(data)

        return x509.IPAddress(ip)
    elif gn.type == backend._lib.GEN_DIRNAME:
        return x509.DirectoryName(
            _decode_x509_name(backend, gn.d.directoryName)
        )
    elif gn.type == backend._lib.GEN_EMAIL:
        # Convert to bytes and then decode to utf8. We don't use
        # asn1_string_to_utf8 here because it doesn't properly convert
        # utf8 from ia5strings.
        data = _asn1_string_to_bytes(backend, gn.d.rfc822Name).decode("utf8")
        # We don't use the constructor for RFC822Name so we can bypass
        # validation. This allows us to create RFC822Name objects that have
        # unicode chars when a certificate (against the RFC) contains them.
        return x509.RFC822Name._init_without_validation(data)
    elif gn.type == backend._lib.GEN_OTHERNAME:
        type_id = _obj2txt(backend, gn.d.otherName.type_id)
        value = _asn1_to_der(backend, gn.d.otherName.value)
        return x509.OtherName(x509.ObjectIdentifier(type_id), value)
    else:
        # x400Address or ediPartyName
        raise x509.UnsupportedGeneralNameType(
            "{} is not a supported type".format(
                x509._GENERAL_NAMES.get(gn.type, gn.type)
            ),
            gn.type,
        )


def _decode_ocsp_no_check(backend, ext):
    return x509.OCSPNoCheck()


def _decode_crl_number(backend, ext):
    asn1_int = backend._ffi.cast("ASN1_INTEGER *", ext)
    asn1_int = backend._ffi.gc(asn1_int, backend._lib.ASN1_INTEGER_free)
    return x509.CRLNumber(_asn1_integer_to_int(backend, asn1_int))


def _decode_delta_crl_indicator(backend, ext):
    asn1_int = backend._ffi.cast("ASN1_INTEGER *", ext)
    asn1_int = backend._ffi.gc(asn1_int, backend._lib.ASN1_INTEGER_free)
    return x509.DeltaCRLIndicator(_asn1_integer_to_int(backend, asn1_int))


class _X509ExtensionParser(object):
    def __init__(self, backend, ext_count, get_ext, handlers):
        self.ext_count = ext_count
        self.get_ext = get_ext
        self.handlers = handlers
        self._backend = backend

    def parse(self, x509_obj):
        extensions = []
        seen_oids = set()
        for i in range(self.ext_count(x509_obj)):
            ext = self.get_ext(x509_obj, i)
            self._backend.openssl_assert(ext != self._backend._ffi.NULL)
            crit = self._backend._lib.X509_EXTENSION_get_critical(ext)
            critical = crit == 1
            oid = x509.ObjectIdentifier(
                _obj2txt(
                    self._backend,
                    self._backend._lib.X509_EXTENSION_get_object(ext),
                )
            )
            if oid in seen_oids:
                raise x509.DuplicateExtension(
                    "Duplicate {} extension found".format(oid), oid
                )

            # These OIDs are only supported in OpenSSL 1.1.0+ but we want
            # to support them in all versions of OpenSSL so we decode them
            # ourselves.
            if oid == ExtensionOID.TLS_FEATURE:
                # The extension contents are a SEQUENCE OF INTEGERs.
                data = self._backend._lib.X509_EXTENSION_get_data(ext)
                data_bytes = _asn1_string_to_bytes(self._backend, data)
                features = DERReader(data_bytes).read_single_element(SEQUENCE)
                parsed = []
                while not features.is_empty():
                    parsed.append(features.read_element(INTEGER).as_integer())
                # Map the features to their enum value.
                value = x509.TLSFeature(
                    [_TLS_FEATURE_TYPE_TO_ENUM[x] for x in parsed]
                )
                extensions.append(x509.Extension(oid, critical, value))
                seen_oids.add(oid)
                continue
            elif oid == ExtensionOID.PRECERT_POISON:
                data = self._backend._lib.X509_EXTENSION_get_data(ext)
                # The contents of the extension must be an ASN.1 NULL.
                reader = DERReader(_asn1_string_to_bytes(self._backend, data))
                reader.read_single_element(NULL).check_empty()
                extensions.append(
                    x509.Extension(oid, critical, x509.PrecertPoison())
                )
                seen_oids.add(oid)
                continue

            try:
                handler = self.handlers[oid]
            except KeyError:
                # Dump the DER payload into an UnrecognizedExtension object
                data = self._backend._lib.X509_EXTENSION_get_data(ext)
                self._backend.openssl_assert(data != self._backend._ffi.NULL)
                der = self._backend._ffi.buffer(data.data, data.length)[:]
                unrecognized = x509.UnrecognizedExtension(oid, der)
                extensions.append(x509.Extension(oid, critical, unrecognized))
            else:
                ext_data = self._backend._lib.X509V3_EXT_d2i(ext)
                if ext_data == self._backend._ffi.NULL:
                    self._backend._consume_errors()
                    raise ValueError(
                        "The {} extension is invalid and can't be "
                        "parsed".format(oid)
                    )

                value = handler(self._backend, ext_data)
                extensions.append(x509.Extension(oid, critical, value))

            seen_oids.add(oid)

        return x509.Extensions(extensions)


def _decode_certificate_policies(backend, cp):
    cp = backend._ffi.cast("Cryptography_STACK_OF_POLICYINFO *", cp)
    cp = backend._ffi.gc(cp, backend._lib.CERTIFICATEPOLICIES_free)

    num = backend._lib.sk_POLICYINFO_num(cp)
    certificate_policies = []
    for i in range(num):
        qualifiers = None
        pi = backend._lib.sk_POLICYINFO_value(cp, i)
        oid = x509.ObjectIdentifier(_obj2txt(backend, pi.policyid))
        if pi.qualifiers != backend._ffi.NULL:
            qnum = backend._lib.sk_POLICYQUALINFO_num(pi.qualifiers)
            qualifiers = []
            for j in range(qnum):
                pqi = backend._lib.sk_POLICYQUALINFO_value(pi.qualifiers, j)
                pqualid = x509.ObjectIdentifier(_obj2txt(backend, pqi.pqualid))
                if pqualid == CertificatePoliciesOID.CPS_QUALIFIER:
                    cpsuri = backend._ffi.buffer(
                        pqi.d.cpsuri.data, pqi.d.cpsuri.length
                    )[:].decode("ascii")
                    qualifiers.append(cpsuri)
                else:
                    assert pqualid == CertificatePoliciesOID.CPS_USER_NOTICE
                    user_notice = _decode_user_notice(
                        backend, pqi.d.usernotice
                    )
                    qualifiers.append(user_notice)

        certificate_policies.append(x509.PolicyInformation(oid, qualifiers))

    return x509.CertificatePolicies(certificate_policies)


def _decode_user_notice(backend, un):
    explicit_text = None
    notice_reference = None

    if un.exptext != backend._ffi.NULL:
        explicit_text = _asn1_string_to_utf8(backend, un.exptext)

    if un.noticeref != backend._ffi.NULL:
        organization = _asn1_string_to_utf8(backend, un.noticeref.organization)

        num = backend._lib.sk_ASN1_INTEGER_num(un.noticeref.noticenos)
        notice_numbers = []
        for i in range(num):
            asn1_int = backend._lib.sk_ASN1_INTEGER_value(
                un.noticeref.noticenos, i
            )
            notice_num = _asn1_integer_to_int(backend, asn1_int)
            notice_numbers.append(notice_num)

        notice_reference = x509.NoticeReference(organization, notice_numbers)

    return x509.UserNotice(notice_reference, explicit_text)


def _decode_basic_constraints(backend, bc_st):
    basic_constraints = backend._ffi.cast("BASIC_CONSTRAINTS *", bc_st)
    basic_constraints = backend._ffi.gc(
        basic_constraints, backend._lib.BASIC_CONSTRAINTS_free
    )
    # The byte representation of an ASN.1 boolean true is \xff. OpenSSL
    # chooses to just map this to its ordinal value, so true is 255 and
    # false is 0.
    ca = basic_constraints.ca == 255
    path_length = _asn1_integer_to_int_or_none(
        backend, basic_constraints.pathlen
    )

    return x509.BasicConstraints(ca, path_length)


def _decode_subject_key_identifier(backend, asn1_string):
    asn1_string = backend._ffi.cast("ASN1_OCTET_STRING *", asn1_string)
    asn1_string = backend._ffi.gc(
        asn1_string, backend._lib.ASN1_OCTET_STRING_free
    )
    return x509.SubjectKeyIdentifier(
        backend._ffi.buffer(asn1_string.data, asn1_string.length)[:]
    )


def _decode_authority_key_identifier(backend, akid):
    akid = backend._ffi.cast("AUTHORITY_KEYID *", akid)
    akid = backend._ffi.gc(akid, backend._lib.AUTHORITY_KEYID_free)
    key_identifier = None
    authority_cert_issuer = None

    if akid.keyid != backend._ffi.NULL:
        key_identifier = backend._ffi.buffer(
            akid.keyid.data, akid.keyid.length
        )[:]

    if akid.issuer != backend._ffi.NULL:
        authority_cert_issuer = _decode_general_names(backend, akid.issuer)

    authority_cert_serial_number = _asn1_integer_to_int_or_none(
        backend, akid.serial
    )

    return x509.AuthorityKeyIdentifier(
        key_identifier, authority_cert_issuer, authority_cert_serial_number
    )


def _decode_information_access(backend, ia):
    ia = backend._ffi.cast("Cryptography_STACK_OF_ACCESS_DESCRIPTION *", ia)
    ia = backend._ffi.gc(
        ia,
        lambda x: backend._lib.sk_ACCESS_DESCRIPTION_pop_free(
            x,
            backend._ffi.addressof(
                backend._lib._original_lib, "ACCESS_DESCRIPTION_free"
            ),
        ),
    )
    num = backend._lib.sk_ACCESS_DESCRIPTION_num(ia)
    access_descriptions = []
    for i in range(num):
        ad = backend._lib.sk_ACCESS_DESCRIPTION_value(ia, i)
        backend.openssl_assert(ad.method != backend._ffi.NULL)
        oid = x509.ObjectIdentifier(_obj2txt(backend, ad.method))
        backend.openssl_assert(ad.location != backend._ffi.NULL)
        gn = _decode_general_name(backend, ad.location)
        access_descriptions.append(x509.AccessDescription(oid, gn))

    return access_descriptions


def _decode_authority_information_access(backend, aia):
    access_descriptions = _decode_information_access(backend, aia)
    return x509.AuthorityInformationAccess(access_descriptions)


def _decode_subject_information_access(backend, aia):
    access_descriptions = _decode_information_access(backend, aia)
    return x509.SubjectInformationAccess(access_descriptions)


def _decode_key_usage(backend, bit_string):
    bit_string = backend._ffi.cast("ASN1_BIT_STRING *", bit_string)
    bit_string = backend._ffi.gc(bit_string, backend._lib.ASN1_BIT_STRING_free)
    get_bit = backend._lib.ASN1_BIT_STRING_get_bit
    digital_signature = get_bit(bit_string, 0) == 1
    content_commitment = get_bit(bit_string, 1) == 1
    key_encipherment = get_bit(bit_string, 2) == 1
    data_encipherment = get_bit(bit_string, 3) == 1
    key_agreement = get_bit(bit_string, 4) == 1
    key_cert_sign = get_bit(bit_string, 5) == 1
    crl_sign = get_bit(bit_string, 6) == 1
    encipher_only = get_bit(bit_string, 7) == 1
    decipher_only = get_bit(bit_string, 8) == 1
    return x509.KeyUsage(
        digital_signature,
        content_commitment,
        key_encipherment,
        data_encipherment,
        key_agreement,
        key_cert_sign,
        crl_sign,
        encipher_only,
        decipher_only,
    )


def _decode_general_names_extension(backend, gns):
    gns = backend._ffi.cast("GENERAL_NAMES *", gns)
    gns = backend._ffi.gc(gns, backend._lib.GENERAL_NAMES_free)
    general_names = _decode_general_names(backend, gns)
    return general_names


def _decode_subject_alt_name(backend, ext):
    return x509.SubjectAlternativeName(
        _decode_general_names_extension(backend, ext)
    )


def _decode_issuer_alt_name(backend, ext):
    return x509.IssuerAlternativeName(
        _decode_general_names_extension(backend, ext)
    )


def _decode_name_constraints(backend, nc):
    nc = backend._ffi.cast("NAME_CONSTRAINTS *", nc)
    nc = backend._ffi.gc(nc, backend._lib.NAME_CONSTRAINTS_free)
    permitted = _decode_general_subtrees(backend, nc.permittedSubtrees)
    excluded = _decode_general_subtrees(backend, nc.excludedSubtrees)
    return x509.NameConstraints(
        permitted_subtrees=permitted, excluded_subtrees=excluded
    )


def _decode_general_subtrees(backend, stack_subtrees):
    if stack_subtrees == backend._ffi.NULL:
        return None

    num = backend._lib.sk_GENERAL_SUBTREE_num(stack_subtrees)
    subtrees = []

    for i in range(num):
        obj = backend._lib.sk_GENERAL_SUBTREE_value(stack_subtrees, i)
        backend.openssl_assert(obj != backend._ffi.NULL)
        name = _decode_general_name(backend, obj.base)
        subtrees.append(name)

    return subtrees


def _decode_issuing_dist_point(backend, idp):
    idp = backend._ffi.cast("ISSUING_DIST_POINT *", idp)
    idp = backend._ffi.gc(idp, backend._lib.ISSUING_DIST_POINT_free)
    if idp.distpoint != backend._ffi.NULL:
        full_name, relative_name = _decode_distpoint(backend, idp.distpoint)
    else:
        full_name = None
        relative_name = None

    only_user = idp.onlyuser == 255
    only_ca = idp.onlyCA == 255
    indirect_crl = idp.indirectCRL == 255
    only_attr = idp.onlyattr == 255
    if idp.onlysomereasons != backend._ffi.NULL:
        only_some_reasons = _decode_reasons(backend, idp.onlysomereasons)
    else:
        only_some_reasons = None

    return x509.IssuingDistributionPoint(
        full_name,
        relative_name,
        only_user,
        only_ca,
        only_some_reasons,
        indirect_crl,
        only_attr,
    )


def _decode_policy_constraints(backend, pc):
    pc = backend._ffi.cast("POLICY_CONSTRAINTS *", pc)
    pc = backend._ffi.gc(pc, backend._lib.POLICY_CONSTRAINTS_free)

    require_explicit_policy = _asn1_integer_to_int_or_none(
        backend, pc.requireExplicitPolicy
    )
    inhibit_policy_mapping = _asn1_integer_to_int_or_none(
        backend, pc.inhibitPolicyMapping
    )

    return x509.PolicyConstraints(
        require_explicit_policy, inhibit_policy_mapping
    )


def _decode_extended_key_usage(backend, sk):
    sk = backend._ffi.cast("Cryptography_STACK_OF_ASN1_OBJECT *", sk)
    sk = backend._ffi.gc(sk, backend._lib.sk_ASN1_OBJECT_free)
    num = backend._lib.sk_ASN1_OBJECT_num(sk)
    ekus = []

    for i in range(num):
        obj = backend._lib.sk_ASN1_OBJECT_value(sk, i)
        backend.openssl_assert(obj != backend._ffi.NULL)
        oid = x509.ObjectIdentifier(_obj2txt(backend, obj))
        ekus.append(oid)

    return x509.ExtendedKeyUsage(ekus)


_DISTPOINT_TYPE_FULLNAME = 0
_DISTPOINT_TYPE_RELATIVENAME = 1


def _decode_dist_points(backend, cdps):
    cdps = backend._ffi.cast("Cryptography_STACK_OF_DIST_POINT *", cdps)
    cdps = backend._ffi.gc(cdps, backend._lib.CRL_DIST_POINTS_free)

    num = backend._lib.sk_DIST_POINT_num(cdps)
    dist_points = []
    for i in range(num):
        full_name = None
        relative_name = None
        crl_issuer = None
        reasons = None
        cdp = backend._lib.sk_DIST_POINT_value(cdps, i)
        if cdp.reasons != backend._ffi.NULL:
            reasons = _decode_reasons(backend, cdp.reasons)

        if cdp.CRLissuer != backend._ffi.NULL:
            crl_issuer = _decode_general_names(backend, cdp.CRLissuer)

        # Certificates may have a crl_issuer/reasons and no distribution
        # point so make sure it's not null.
        if cdp.distpoint != backend._ffi.NULL:
            full_name, relative_name = _decode_distpoint(
                backend, cdp.distpoint
            )

        dist_points.append(
            x509.DistributionPoint(
                full_name, relative_name, reasons, crl_issuer
            )
        )

    return dist_points


# ReasonFlags ::= BIT STRING {
#      unused                  (0),
#      keyCompromise           (1),
#      cACompromise            (2),
#      affiliationChanged      (3),
#      superseded              (4),
#      cessationOfOperation    (5),
#      certificateHold         (6),
#      privilegeWithdrawn      (7),
#      aACompromise            (8) }
_REASON_BIT_MAPPING = {
    1: x509.ReasonFlags.key_compromise,
    2: x509.ReasonFlags.ca_compromise,
    3: x509.ReasonFlags.affiliation_changed,
    4: x509.ReasonFlags.superseded,
    5: x509.ReasonFlags.cessation_of_operation,
    6: x509.ReasonFlags.certificate_hold,
    7: x509.ReasonFlags.privilege_withdrawn,
    8: x509.ReasonFlags.aa_compromise,
}


def _decode_reasons(backend, reasons):
    # We will check each bit from RFC 5280
    enum_reasons = []
    for bit_position, reason in six.iteritems(_REASON_BIT_MAPPING):
        if backend._lib.ASN1_BIT_STRING_get_bit(reasons, bit_position):
            enum_reasons.append(reason)

    return frozenset(enum_reasons)


def _decode_distpoint(backend, distpoint):
    if distpoint.type == _DISTPOINT_TYPE_FULLNAME:
        full_name = _decode_general_names(backend, distpoint.name.fullname)
        return full_name, None

    # OpenSSL code doesn't test for a specific type for
    # relativename, everything that isn't fullname is considered
    # relativename.  Per RFC 5280:
    #
    # DistributionPointName ::= CHOICE {
    #      fullName                [0]      GeneralNames,
    #      nameRelativeToCRLIssuer [1]      RelativeDistinguishedName }
    rns = distpoint.name.relativename
    rnum = backend._lib.sk_X509_NAME_ENTRY_num(rns)
    attributes = set()
    for i in range(rnum):
        rn = backend._lib.sk_X509_NAME_ENTRY_value(rns, i)
        backend.openssl_assert(rn != backend._ffi.NULL)
        attributes.add(_decode_x509_name_entry(backend, rn))

    relative_name = x509.RelativeDistinguishedName(attributes)

    return None, relative_name


def _decode_crl_distribution_points(backend, cdps):
    dist_points = _decode_dist_points(backend, cdps)
    return x509.CRLDistributionPoints(dist_points)


def _decode_freshest_crl(backend, cdps):
    dist_points = _decode_dist_points(backend, cdps)
    return x509.FreshestCRL(dist_points)


def _decode_inhibit_any_policy(backend, asn1_int):
    asn1_int = backend._ffi.cast("ASN1_INTEGER *", asn1_int)
    asn1_int = backend._ffi.gc(asn1_int, backend._lib.ASN1_INTEGER_free)
    skip_certs = _asn1_integer_to_int(backend, asn1_int)
    return x509.InhibitAnyPolicy(skip_certs)


def _decode_scts(backend, asn1_scts):
    from cryptography.hazmat.backends.openssl.x509 import (
        _SignedCertificateTimestamp,
    )

    asn1_scts = backend._ffi.cast("Cryptography_STACK_OF_SCT *", asn1_scts)
    asn1_scts = backend._ffi.gc(asn1_scts, backend._lib.SCT_LIST_free)

    scts = []
    for i in range(backend._lib.sk_SCT_num(asn1_scts)):
        sct = backend._lib.sk_SCT_value(asn1_scts, i)

        scts.append(_SignedCertificateTimestamp(backend, asn1_scts, sct))
    return scts


def _decode_precert_signed_certificate_timestamps(backend, asn1_scts):
    return x509.PrecertificateSignedCertificateTimestamps(
        _decode_scts(backend, asn1_scts)
    )


def _decode_signed_certificate_timestamps(backend, asn1_scts):
    return x509.SignedCertificateTimestamps(_decode_scts(backend, asn1_scts))


#    CRLReason ::= ENUMERATED {
#        unspecified             (0),
#        keyCompromise           (1),
#        cACompromise            (2),
#        affiliationChanged      (3),
#        superseded              (4),
#        cessationOfOperation    (5),
#        certificateHold         (6),
#             -- value 7 is not used
#        removeFromCRL           (8),
#        privilegeWithdrawn      (9),
#        aACompromise           (10) }
_CRL_ENTRY_REASON_CODE_TO_ENUM = {
    0: x509.ReasonFlags.unspecified,
    1: x509.ReasonFlags.key_compromise,
    2: x509.ReasonFlags.ca_compromise,
    3: x509.ReasonFlags.affiliation_changed,
    4: x509.ReasonFlags.superseded,
    5: x509.ReasonFlags.cessation_of_operation,
    6: x509.ReasonFlags.certificate_hold,
    8: x509.ReasonFlags.remove_from_crl,
    9: x509.ReasonFlags.privilege_withdrawn,
    10: x509.ReasonFlags.aa_compromise,
}


_CRL_ENTRY_REASON_ENUM_TO_CODE = {
    x509.ReasonFlags.unspecified: 0,
    x509.ReasonFlags.key_compromise: 1,
    x509.ReasonFlags.ca_compromise: 2,
    x509.ReasonFlags.affiliation_changed: 3,
    x509.ReasonFlags.superseded: 4,
    x509.ReasonFlags.cessation_of_operation: 5,
    x509.ReasonFlags.certificate_hold: 6,
    x509.ReasonFlags.remove_from_crl: 8,
    x509.ReasonFlags.privilege_withdrawn: 9,
    x509.ReasonFlags.aa_compromise: 10,
}


def _decode_crl_reason(backend, enum):
    enum = backend._ffi.cast("ASN1_ENUMERATED *", enum)
    enum = backend._ffi.gc(enum, backend._lib.ASN1_ENUMERATED_free)
    code = backend._lib.ASN1_ENUMERATED_get(enum)

    try:
        return x509.CRLReason(_CRL_ENTRY_REASON_CODE_TO_ENUM[code])
    except KeyError:
        raise ValueError("Unsupported reason code: {}".format(code))


def _decode_invalidity_date(backend, inv_date):
    generalized_time = backend._ffi.cast("ASN1_GENERALIZEDTIME *", inv_date)
    generalized_time = backend._ffi.gc(
        generalized_time, backend._lib.ASN1_GENERALIZEDTIME_free
    )
    return x509.InvalidityDate(
        _parse_asn1_generalized_time(backend, generalized_time)
    )


def _decode_cert_issuer(backend, gns):
    gns = backend._ffi.cast("GENERAL_NAMES *", gns)
    gns = backend._ffi.gc(gns, backend._lib.GENERAL_NAMES_free)
    general_names = _decode_general_names(backend, gns)
    return x509.CertificateIssuer(general_names)


def _asn1_to_der(backend, asn1_type):
    buf = backend._ffi.new("unsigned char **")
    res = backend._lib.i2d_ASN1_TYPE(asn1_type, buf)
    backend.openssl_assert(res >= 0)
    backend.openssl_assert(buf[0] != backend._ffi.NULL)
    buf = backend._ffi.gc(
        buf, lambda buffer: backend._lib.OPENSSL_free(buffer[0])
    )
    return backend._ffi.buffer(buf[0], res)[:]


def _asn1_integer_to_int(backend, asn1_int):
    bn = backend._lib.ASN1_INTEGER_to_BN(asn1_int, backend._ffi.NULL)
    backend.openssl_assert(bn != backend._ffi.NULL)
    bn = backend._ffi.gc(bn, backend._lib.BN_free)
    return backend._bn_to_int(bn)


def _asn1_integer_to_int_or_none(backend, asn1_int):
    if asn1_int == backend._ffi.NULL:
        return None
    else:
        return _asn1_integer_to_int(backend, asn1_int)


def _asn1_string_to_bytes(backend, asn1_string):
    return backend._ffi.buffer(asn1_string.data, asn1_string.length)[:]


def _asn1_string_to_ascii(backend, asn1_string):
    return _asn1_string_to_bytes(backend, asn1_string).decode("ascii")


def _asn1_string_to_utf8(backend, asn1_string):
    buf = backend._ffi.new("unsigned char **")
    res = backend._lib.ASN1_STRING_to_UTF8(buf, asn1_string)
    if res == -1:
        raise ValueError(
            "Unsupported ASN1 string type. Type: {}".format(asn1_string.type)
        )

    backend.openssl_assert(buf[0] != backend._ffi.NULL)
    buf = backend._ffi.gc(
        buf, lambda buffer: backend._lib.OPENSSL_free(buffer[0])
    )
    return backend._ffi.buffer(buf[0], res)[:].decode("utf8")


def _parse_asn1_time(backend, asn1_time):
    backend.openssl_assert(asn1_time != backend._ffi.NULL)
    generalized_time = backend._lib.ASN1_TIME_to_generalizedtime(
        asn1_time, backend._ffi.NULL
    )
    if generalized_time == backend._ffi.NULL:
        raise ValueError(
            "Couldn't parse ASN.1 time as generalizedtime {!r}".format(
                _asn1_string_to_bytes(backend, asn1_time)
            )
        )

    generalized_time = backend._ffi.gc(
        generalized_time, backend._lib.ASN1_GENERALIZEDTIME_free
    )
    return _parse_asn1_generalized_time(backend, generalized_time)


def _parse_asn1_generalized_time(backend, generalized_time):
    time = _asn1_string_to_ascii(
        backend, backend._ffi.cast("ASN1_STRING *", generalized_time)
    )
    return datetime.datetime.strptime(time, "%Y%m%d%H%M%SZ")


def _decode_nonce(backend, nonce):
    nonce = backend._ffi.cast("ASN1_OCTET_STRING *", nonce)
    nonce = backend._ffi.gc(nonce, backend._lib.ASN1_OCTET_STRING_free)
    return x509.OCSPNonce(_asn1_string_to_bytes(backend, nonce))


_EXTENSION_HANDLERS_BASE = {
    ExtensionOID.BASIC_CONSTRAINTS: _decode_basic_constraints,
    ExtensionOID.SUBJECT_KEY_IDENTIFIER: _decode_subject_key_identifier,
    ExtensionOID.KEY_USAGE: _decode_key_usage,
    ExtensionOID.SUBJECT_ALTERNATIVE_NAME: _decode_subject_alt_name,
    ExtensionOID.EXTENDED_KEY_USAGE: _decode_extended_key_usage,
    ExtensionOID.AUTHORITY_KEY_IDENTIFIER: _decode_authority_key_identifier,
    ExtensionOID.AUTHORITY_INFORMATION_ACCESS: (
        _decode_authority_information_access
    ),
    ExtensionOID.SUBJECT_INFORMATION_ACCESS: (
        _decode_subject_information_access
    ),
    ExtensionOID.CERTIFICATE_POLICIES: _decode_certificate_policies,
    ExtensionOID.CRL_DISTRIBUTION_POINTS: _decode_crl_distribution_points,
    ExtensionOID.FRESHEST_CRL: _decode_freshest_crl,
    ExtensionOID.OCSP_NO_CHECK: _decode_ocsp_no_check,
    ExtensionOID.INHIBIT_ANY_POLICY: _decode_inhibit_any_policy,
    ExtensionOID.ISSUER_ALTERNATIVE_NAME: _decode_issuer_alt_name,
    ExtensionOID.NAME_CONSTRAINTS: _decode_name_constraints,
    ExtensionOID.POLICY_CONSTRAINTS: _decode_policy_constraints,
}
_EXTENSION_HANDLERS_SCT = {
    ExtensionOID.PRECERT_SIGNED_CERTIFICATE_TIMESTAMPS: (
        _decode_precert_signed_certificate_timestamps
    )
}

_REVOKED_EXTENSION_HANDLERS = {
    CRLEntryExtensionOID.CRL_REASON: _decode_crl_reason,
    CRLEntryExtensionOID.INVALIDITY_DATE: _decode_invalidity_date,
    CRLEntryExtensionOID.CERTIFICATE_ISSUER: _decode_cert_issuer,
}

_CRL_EXTENSION_HANDLERS = {
    ExtensionOID.CRL_NUMBER: _decode_crl_number,
    ExtensionOID.DELTA_CRL_INDICATOR: _decode_delta_crl_indicator,
    ExtensionOID.AUTHORITY_KEY_IDENTIFIER: _decode_authority_key_identifier,
    ExtensionOID.ISSUER_ALTERNATIVE_NAME: _decode_issuer_alt_name,
    ExtensionOID.AUTHORITY_INFORMATION_ACCESS: (
        _decode_authority_information_access
    ),
    ExtensionOID.ISSUING_DISTRIBUTION_POINT: _decode_issuing_dist_point,
    ExtensionOID.FRESHEST_CRL: _decode_freshest_crl,
}

_OCSP_REQ_EXTENSION_HANDLERS = {
    OCSPExtensionOID.NONCE: _decode_nonce,
}

_OCSP_BASICRESP_EXTENSION_HANDLERS = {
    OCSPExtensionOID.NONCE: _decode_nonce,
}

_OCSP_SINGLERESP_EXTENSION_HANDLERS_SCT = {
    ExtensionOID.SIGNED_CERTIFICATE_TIMESTAMPS: (
        _decode_signed_certificate_timestamps
    )
}
