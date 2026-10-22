""" Codec for the modified UTF-7 encoding used for IMAP4 mailbox names,
as specified in RFC 3501, section 5.1.3.

It differs from UTF-7 (RFC 2152) as follows:

* "&" (not "+") is the shift character introducing a Base64 sequence,
  and "&-" encodes a literal "&";
* "," is used instead of "/" in the modified Base64 alphabet;
* only printable US-ASCII characters (except "&") may be represented
  directly, so all other characters, including other controls, are
  Base64-encoded.
"""

import binascii
import codecs

# The modified Base64 alphabet of RFC 3501: standard Base64 but with "," in
# place of "/".
_alphabet = binascii.BASE64_ALPHABET[:-1] + b','

### Codec APIs

def utf_7_imap_encode(input, errors='strict'):
    if errors != 'strict':
        raise UnicodeError(f"Unsupported error handling: {errors}")
    res = bytearray()
    start = 0                   # start of the current run
    b64run = False              # is input[start:i] a Base64 run?
    def flush(end):
        if start < end:
            if b64run:
                b64 = binascii.b2a_base64(input[start:end].encode('utf-16-be'),
                                          alphabet=_alphabet, padded=False,
                                          newline=False)
                res.extend(b'&' + b64 + b'-')
            else:
                res.extend(input[start:end].encode('ascii'))
    for i, ch in enumerate(input):
        if ch == '&':
            flush(i)
            res.extend(b'&-')
            start = i + 1
            b64run = False
        elif ' ' <= ch <= '~':      # printable ASCII, represented directly
            if b64run:
                flush(i)
                start = i
                b64run = False
        else:                        # everything else is Base64-encoded
            if not b64run:
                flush(i)
                start = i
                b64run = True
    flush(len(input))
    return res.take_bytes(), len(input)

def utf_7_imap_decode(input, errors='strict'):
    if errors != 'strict':
        raise UnicodeError(f"Unsupported error handling: {errors}")
    input = bytes(input)
    res = []
    start = 0                   # start of the current direct ASCII run
    i = 0
    n = len(input)
    def flush(end):
        if start < end:
            res.append(input[start:end].decode('ascii'))
    while i < n:
        c = input[i]
        if c == b'&'[0]:
            flush(i)
            j = input.find(b'-', i + 1)
            if j < 0:
                raise UnicodeDecodeError('utf-7-imap', input, i, n,
                                         'unterminated shift sequence')
            if j == i + 1:          # '&-'
                res.append('&')
            else:
                b64 = input[i + 1:j]
                try:
                    data = binascii.a2b_base64(b64, alphabet=_alphabet,
                                               strict_mode=True, padded=False)
                except binascii.Error:
                    data = b''
                if not data or len(data) % 2:
                    raise UnicodeDecodeError('utf-7-imap', input, i, j + 1,
                                             'invalid shift sequence')
                res.append(data.decode('utf-16-be'))
            i = j + 1
            start = i
        elif b' '[0] <= c <= b'~'[0]:
            i += 1
        else:
            raise UnicodeDecodeError('utf-7-imap', input, i, i + 1,
                                     'unexpected byte')
    flush(n)
    return ''.join(res), len(input)

class Codec(codecs.Codec):
    def encode(self, input, errors='strict'):
        return utf_7_imap_encode(input, errors)
    def decode(self, input, errors='strict'):
        return utf_7_imap_decode(input, errors)

class IncrementalEncoder(codecs.IncrementalEncoder):
    def encode(self, input, final=False):
        return utf_7_imap_encode(input, self.errors)[0]

class IncrementalDecoder(codecs.IncrementalDecoder):
    def decode(self, input, final=False):
        return utf_7_imap_decode(input, self.errors)[0]

class StreamWriter(Codec, codecs.StreamWriter):
    pass

class StreamReader(Codec, codecs.StreamReader):
    pass

### encodings module API

def getregentry():
    return codecs.CodecInfo(
        name='utf-7-imap',
        encode=Codec().encode,
        decode=Codec().decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamwriter=StreamWriter,
        streamreader=StreamReader,
    )
