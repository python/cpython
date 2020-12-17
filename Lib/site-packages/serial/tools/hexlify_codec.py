#! python
#
# This is a codec to create and decode hexdumps with spaces between characters. used by miniterm.
#
# This file is part of pySerial. https://github.com/pyserial/pyserial
# (C) 2015-2016 Chris Liechti <cliechti@gmx.net>
#
# SPDX-License-Identifier:    BSD-3-Clause
"""\
Python 'hex' Codec - 2-digit hex with spaces content transfer encoding.

Encode and decode may be a bit missleading at first sight...

The textual representation is a hex dump: e.g. "40 41"
The "encoded" data of this is the binary form, e.g. b"@A"

Therefore decoding is binary to text and thus converting binary data to hex dump.

"""

from __future__ import absolute_import

import codecs
import serial


try:
    unicode
except (NameError, AttributeError):
    unicode = str       # for Python 3, pylint: disable=redefined-builtin,invalid-name


HEXDIGITS = '0123456789ABCDEF'


# Codec APIs

def hex_encode(data, errors='strict'):
    """'40 41 42' -> b'@ab'"""
    return (serial.to_bytes([int(h, 16) for h in data.split()]), len(data))


def hex_decode(data, errors='strict'):
    """b'@ab' -> '40 41 42'"""
    return (unicode(''.join('{:02X} '.format(ord(b)) for b in serial.iterbytes(data))), len(data))


class Codec(codecs.Codec):
    def encode(self, data, errors='strict'):
        """'40 41 42' -> b'@ab'"""
        return serial.to_bytes([int(h, 16) for h in data.split()])

    def decode(self, data, errors='strict'):
        """b'@ab' -> '40 41 42'"""
        return unicode(''.join('{:02X} '.format(ord(b)) for b in serial.iterbytes(data)))


class IncrementalEncoder(codecs.IncrementalEncoder):
    """Incremental hex encoder"""

    def __init__(self, errors='strict'):
        self.errors = errors
        self.state = 0

    def reset(self):
        self.state = 0

    def getstate(self):
        return self.state

    def setstate(self, state):
        self.state = state

    def encode(self, data, final=False):
        """\
        Incremental encode, keep track of digits and emit a byte when a pair
        of hex digits is found. The space is optional unless the error
        handling is defined to be 'strict'.
        """
        state = self.state
        encoded = []
        for c in data.upper():
            if c in HEXDIGITS:
                z = HEXDIGITS.index(c)
                if state:
                    encoded.append(z + (state & 0xf0))
                    state = 0
                else:
                    state = 0x100 + (z << 4)
            elif c == ' ':      # allow spaces to separate values
                if state and self.errors == 'strict':
                    raise UnicodeError('odd number of hex digits')
                state = 0
            else:
                if self.errors == 'strict':
                    raise UnicodeError('non-hex digit found: {!r}'.format(c))
        self.state = state
        return serial.to_bytes(encoded)


class IncrementalDecoder(codecs.IncrementalDecoder):
    """Incremental decoder"""
    def decode(self, data, final=False):
        return unicode(''.join('{:02X} '.format(ord(b)) for b in serial.iterbytes(data)))


class StreamWriter(Codec, codecs.StreamWriter):
    """Combination of hexlify codec and StreamWriter"""


class StreamReader(Codec, codecs.StreamReader):
    """Combination of hexlify codec and StreamReader"""


def getregentry():
    """encodings module API"""
    return codecs.CodecInfo(
        name='hexlify',
        encode=hex_encode,
        decode=hex_decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamwriter=StreamWriter,
        streamreader=StreamReader,
        #~ _is_text_encoding=True,
    )
