# This file is dual licensed under the terms of the Apache License, Version
# 2.0, and the BSD License. See the LICENSE file in the root of this repository
# for complete details.

from __future__ import absolute_import, division, print_function

import six

from cryptography.utils import int_from_bytes, int_to_bytes


# This module contains a lightweight DER encoder and decoder. See X.690 for the
# specification. This module intentionally does not implement the more complex
# BER encoding, only DER.
#
# Note this implementation treats an element's constructed bit as part of the
# tag. This is fine for DER, where the bit is always computable from the type.


CONSTRUCTED = 0x20
CONTEXT_SPECIFIC = 0x80

INTEGER = 0x02
BIT_STRING = 0x03
OCTET_STRING = 0x04
NULL = 0x05
OBJECT_IDENTIFIER = 0x06
SEQUENCE = 0x10 | CONSTRUCTED
SET = 0x11 | CONSTRUCTED
PRINTABLE_STRING = 0x13
UTC_TIME = 0x17
GENERALIZED_TIME = 0x18


class DERReader(object):
    def __init__(self, data):
        self.data = memoryview(data)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, tb):
        if exc_value is None:
            self.check_empty()

    def is_empty(self):
        return len(self.data) == 0

    def check_empty(self):
        if not self.is_empty():
            raise ValueError("Invalid DER input: trailing data")

    def read_byte(self):
        if len(self.data) < 1:
            raise ValueError("Invalid DER input: insufficient data")
        ret = six.indexbytes(self.data, 0)
        self.data = self.data[1:]
        return ret

    def read_bytes(self, n):
        if len(self.data) < n:
            raise ValueError("Invalid DER input: insufficient data")
        ret = self.data[:n]
        self.data = self.data[n:]
        return ret

    def read_any_element(self):
        tag = self.read_byte()
        # Tag numbers 31 or higher are stored in multiple bytes. No supported
        # ASN.1 types use such tags, so reject these.
        if tag & 0x1F == 0x1F:
            raise ValueError("Invalid DER input: unexpected high tag number")
        length_byte = self.read_byte()
        if length_byte & 0x80 == 0:
            # If the high bit is clear, the first length byte is the length.
            length = length_byte
        else:
            # If the high bit is set, the first length byte encodes the length
            # of the length.
            length_byte &= 0x7F
            if length_byte == 0:
                raise ValueError(
                    "Invalid DER input: indefinite length form is not allowed "
                    "in DER"
                )
            length = 0
            for i in range(length_byte):
                length <<= 8
                length |= self.read_byte()
                if length == 0:
                    raise ValueError(
                        "Invalid DER input: length was not minimally-encoded"
                    )
            if length < 0x80:
                # If the length could have been encoded in short form, it must
                # not use long form.
                raise ValueError(
                    "Invalid DER input: length was not minimally-encoded"
                )
        body = self.read_bytes(length)
        return tag, DERReader(body)

    def read_element(self, expected_tag):
        tag, body = self.read_any_element()
        if tag != expected_tag:
            raise ValueError("Invalid DER input: unexpected tag")
        return body

    def read_single_element(self, expected_tag):
        with self:
            return self.read_element(expected_tag)

    def read_optional_element(self, expected_tag):
        if len(self.data) > 0 and six.indexbytes(self.data, 0) == expected_tag:
            return self.read_element(expected_tag)
        return None

    def as_integer(self):
        if len(self.data) == 0:
            raise ValueError("Invalid DER input: empty integer contents")
        first = six.indexbytes(self.data, 0)
        if first & 0x80 == 0x80:
            raise ValueError("Negative DER integers are not supported")
        # The first 9 bits must not all be zero or all be ones. Otherwise, the
        # encoding should have been one byte shorter.
        if len(self.data) > 1:
            second = six.indexbytes(self.data, 1)
            if first == 0 and second & 0x80 == 0:
                raise ValueError(
                    "Invalid DER input: integer not minimally-encoded"
                )
        return int_from_bytes(self.data, "big")


def encode_der_integer(x):
    if not isinstance(x, six.integer_types):
        raise ValueError("Value must be an integer")
    if x < 0:
        raise ValueError("Negative integers are not supported")
    n = x.bit_length() // 8 + 1
    return int_to_bytes(x, n)


def encode_der(tag, *children):
    length = 0
    for child in children:
        length += len(child)
    chunks = [six.int2byte(tag)]
    if length < 0x80:
        chunks.append(six.int2byte(length))
    else:
        length_bytes = int_to_bytes(length)
        chunks.append(six.int2byte(0x80 | len(length_bytes)))
        chunks.append(length_bytes)
    chunks.extend(children)
    return b"".join(chunks)
