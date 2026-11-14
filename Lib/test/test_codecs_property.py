"""Property tests for the codec machinery.

These run every registered text codec through the incremental, stream and
error-handler interfaces on generated input and compare against the stateless
one-shot codec functions, which the other codec tests treat as the reference.
With Hypothesis installed the input is generated; without it, only the
explicit examples run.

Inputs that are known to fail are pinned as expected-failure examples on
the test they belong to, with the issue number as the reason, and are kept
out of the generated input until the bug is fixed.
"""

import codecs
import encodings
import html
import io
import unittest

from test.support.hypothesis_helper import hypothesis
from test.test_codecs import all_unicode_encodings, bytes_transform_encodings

try:
    import _multibytecodec
except ImportError:
    # Then none of the CJK codecs is available either.
    _multibytecodec = None

st = hypothesis.strategies


def _available(encoding):
    try:
        codecs.lookup(encoding)
    except LookupError:
        return False
    return True


# The Windows-only codecs only support a few error handlers.
UNICODE_ENCODINGS = tuple(e for e in all_unicode_encodings
                          if e not in ("mbcs", "oem") and _available(e))

# These transform a whole label at a time, so their incremental and stream
# variants do not accept arbitrary chunking, and idna supports 'strict' only.
WHOLE_STRING_ENCODINGS = frozenset({"idna", "punycode"})

# The stateless decoder assumes native byte order when there is no byte
# order mark; the incremental decoder refuses the input instead.
BOM_ENCODINGS = frozenset({"utf_16", "utf_32"})

# Implemented by Modules/cjkcodecs; they share one decoder loop and have
# their own stream reader and writer.
MULTIBYTE_ENCODINGS = frozenset(
    e for e in UNICODE_ENCODINGS
    if _multibytecodec is not None
    and issubclass(codecs.lookup(e).incrementaldecoder,
                   _multibytecodec.MultibyteIncrementalDecoder))

ISO2022_ENCODINGS = frozenset(e for e in UNICODE_ENCODINGS
                              if e.startswith("iso2022"))

# The multibyte codecs with sequences longer than two bytes; see the
# expected-failure examples on test_decode_in_chunks (gh-160307).
LONG_SEQUENCE_ENCODINGS = frozenset(
    {"euc_jp", "euc_jis_2004", "euc_jisx0213", "gb18030"})

INCREMENTAL_ENCODINGS = tuple(e for e in UNICODE_ENCODINGS
                              if e not in WHOLE_STRING_ENCODINGS)

# The codecs whose StreamReader is the pure Python one from the codecs
# module, as opposed to the multibyte codecs' own.
PYTHON_READER_ENCODINGS = tuple(e for e in INCREMENTAL_ENCODINGS
                                if e not in MULTIBYTE_ENCODINGS)

DECODE_ERRORS = ("strict", "ignore", "replace", "backslashreplace",
                 "surrogateescape")
ENCODE_ERRORS = DECODE_ERRORS + ("xmlcharrefreplace", "namereplace",
                                 "surrogatepass")

# Encoders that are many-to-one by design, so decode(encode(s)) need not
# give s back: idna applies nameprep, raw_unicode_escape leaves existing
# backslashes alone (it exists for pickle, not for round trips), and the
# Japanese and Big5 codecs fold a few compatibility characters (U+00A5 and
# U+203E onto ASCII bytes, U+00A2 onto U+FFE0 and so on).
NON_INJECTIVE_ENCODINGS = frozenset(
    {"idna", "raw_unicode_escape", "cp932", "cp950", "euc_jp", "shift_jis"})

# Characters an encoder accepts although its decoder cannot give them back;
# each is pinned as an expected-failure example on test_encode_decode and
# kept out of the generated text until fixed.
ENCODE_ONLY_CHARACTERS = {
    # A bare ESC, which the decoders take as the start of a sequence, and
    # for iso2022_kr the SO and SI shift characters as well (gh-160302).
    **dict.fromkeys(ISO2022_ENCODINGS, "\x1b"),
    "iso2022_kr": "\x1b\x0e\x0f",
    # The Hangul filler, which the decoder only accepts inside a sequence
    # (gh-101863).
    "euc_kr": "\u3164",
    # Encoded with the JIS X 0213:2000 table but decoded with the 2004 one
    # (gh-160306).
    "iso2022_jp_3": "\u9b1d",
}


def _lossless_bytes(encoding):
    # Whether every single byte survives decode(surrogateescape) followed
    # by encode(surrogateescape).  That rules out codecs with undefined
    # bytes below 0x80 (surrogateescape refuses to escape those), tables
    # mapping two bytes to one character, and the byte-order-mark codecs.
    for b in range(256):
        data = bytes([b])
        try:
            if data.decode(encoding, "surrogateescape").encode(
                    encoding, "surrogateescape") != data:
                return False
        except UnicodeError:
            return False
    return True


# surrogateescape round trips wherever decoding is injective on bytes and
# the codec is a superset of ASCII.  Multibyte codecs can have duplicate
# two-byte sequences, and raw_unicode_escape raises on a truncated \u
# escape, whose bytes are ASCII and so cannot be escaped either.
SURROGATEESCAPE_ENCODINGS = tuple(
    e for e in UNICODE_ENCODINGS
    if e not in MULTIBYTE_ENCODINGS | WHOLE_STRING_ENCODINGS
    and e != "raw_unicode_escape" and _lossless_bytes(e))


text = st.text(max_size=64)
# st.text() leaves surrogates out; the error handlers are mostly about them.
surrogates = st.characters(categories=["Cs"])
text_with_surrogates = st.text(
    alphabet=st.characters() | surrogates, max_size=64)
data = st.binary(max_size=64)
sizes = st.lists(st.integers(0, 8), max_size=16)
read_sizes = st.lists(st.integers(-1, 8), max_size=16)
incremental_encoding = st.sampled_from(INCREMENTAL_ENCODINGS)


def _chunks(buf, sizes):
    # Split buf at the given sizes; the remainder is the last chunk.
    out = []
    pos = 0
    for n in sizes:
        out.append(buf[pos:pos + n])
        pos += n
    out.append(buf[pos:])
    return out


def _outcome(func):
    # Normalize a call to something comparable: its result, or the type
    # of the UnicodeError it raised.
    try:
        return ("ok", func())
    except UnicodeError as exc:
        return ("error", type(exc))


def _with_bom(encoding, data, big_endian):
    # Give the byte-order-mark codecs a mark, so that the stateless and
    # incremental decoders agree on the byte order (see
    # test_bom_codecs_reject_bare_input for what happens without one).
    if encoding in BOM_ENCODINGS:
        suffix = "_be" if big_endian else "_le"
        return "\ufeff".encode(encoding + suffix) + data
    return data


def _decode_errors(encoding):
    # These decoders drop input after an incomplete trailing sequence
    # under surrogateescape (gh-160307); pinned on test_decode_in_chunks.
    if encoding in LONG_SEQUENCE_ENCODINGS:
        return tuple(e for e in DECODE_ERRORS if e != "surrogateescape")
    return DECODE_ERRORS


def _encode_text(encoding):
    # utf-7 encodes a lone surrogate instead of rejecting it, and joins
    # a surrogate pair into one code point, so a pair split across chunks
    # encodes differently from the pair in one piece.
    if encoding == "utf_7":
        return text
    return text_with_surrogates


def _round_trip_text(encoding, strategy=text):
    # Text that the codec can encode and decode again, see
    # ENCODE_ONLY_CHARACTERS.
    bad = ENCODE_ONLY_CHARACTERS.get(encoding)
    if bad:
        table = str.maketrans("", "", bad)
        return strategy.map(lambda s: s.translate(table))
    return strategy


decode_case = incremental_encoding.flatmap(
    lambda e: st.tuples(st.just(e), st.sampled_from(_decode_errors(e))))
encode_case = incremental_encoding.flatmap(
    lambda e: st.tuples(st.just(e), st.sampled_from(ENCODE_ERRORS),
                        _encode_text(e)))
round_trip_case = st.sampled_from(UNICODE_ENCODINGS).flatmap(
    lambda e: st.tuples(st.just(e), _round_trip_text(e)))
# For the tests that run the error handlers' output back through the
# codec: idna accepts no handler but 'strict', and the many-to-one codecs
# do not give the input back.
handler_encoding = st.sampled_from(tuple(
    e for e in UNICODE_ENCODINGS
    if e != "idna" and e not in NON_INJECTIVE_ENCODINGS))


class IncrementalCodecTests(unittest.TestCase):
    """The incremental codecs agree with the stateless ones for any
    chunking of the input, including under the error handlers."""

    @hypothesis.given(case=decode_case, data=data, sizes=sizes,
                      big_endian=st.booleans())
    @hypothesis.example(case=("utf_8", "strict"), data=b"", sizes=[],
                        big_endian=False)
    @hypothesis.example(case=("utf_8", "replace"), data=b"a\xe2\x82\xac\xe2",
                        sizes=[2, 1, 1], big_endian=False)
    @hypothesis.example(case=("utf_16", "strict"), data=b"a\x00\xd8",
                        sizes=[1, 1], big_endian=True)
    @hypothesis.example(case=("utf_7", "strict"), data=b"+AOk-+", sizes=[3],
                        big_endian=False)
    @hypothesis.example(case=("euc_jp", "replace"), data=b"\xa4\xa2\xa4",
                        sizes=[1, 1], big_endian=False)
    @hypothesis.example(case=("iso2022_jp", "ignore"),
                        data=b"\x1b$B$\"\x1b(B", sizes=[1, 2, 1],
                        big_endian=False)
    @hypothesis.example(case=("cp1252", "backslashreplace"), data=b"\x81a",
                        sizes=[1], big_endian=False)
    @hypothesis.example(case=("unicode_escape", "strict"), data=b"\\u00e9\\",
                        sizes=[3, 2], big_endian=False)
    # gh-160307: after escaping the first byte of an incomplete three- or
    # four-byte sequence at the end of the input, these decoders drop the
    # bytes that follow it.  The EUC codecs do this for 0x8f followed by
    # any ASCII byte, gb18030 for any byte above 0x7f followed by a digit.
    @hypothesis.example(case=("gb18030", "surrogateescape"), data=b"\x806",
                        sizes=[], big_endian=False).xfail(
                            reason="gh-160307", raises=AssertionError)
    @hypothesis.example(case=("gb18030", "surrogateescape"), data=b"a\x819",
                        sizes=[1], big_endian=False).xfail(
                            reason="gh-160307", raises=AssertionError)
    @hypothesis.example(case=("euc_jp", "surrogateescape"), data=b"a\x8f\x00",
                        sizes=[1], big_endian=False).xfail(
                            reason="gh-160307", raises=AssertionError)
    @hypothesis.example(case=("euc_jis_2004", "surrogateescape"),
                        data=b"\x8fl", sizes=[], big_endian=False).xfail(
                            reason="gh-160307", raises=AssertionError)
    @hypothesis.example(case=("euc_jisx0213", "surrogateescape"),
                        data=b"\x8fA", sizes=[1], big_endian=False).xfail(
                            reason="gh-160307", raises=AssertionError)
    # gh-160308: the punycode incremental decoder decodes each chunk on
    # its own.
    @hypothesis.example(case=("punycode", "strict"), data=b"ab-", sizes=[1],
                        big_endian=False).xfail(
                            reason="gh-160308", raises=AssertionError)
    def test_decode_in_chunks(self, case, data, sizes, big_endian):
        encoding, errors = case
        data = _with_bom(encoding, data, big_endian)
        expected = _outcome(lambda: data.decode(encoding, errors))

        def incremental():
            decoder = codecs.getincrementaldecoder(encoding)(errors)
            parts = [decoder.decode(chunk) for chunk in _chunks(data, sizes)]
            parts.append(decoder.decode(b"", final=True))
            return "".join(parts)

        self.assertEqual(_outcome(incremental), expected)

    @hypothesis.given(case=encode_case, sizes=sizes)
    @hypothesis.example(case=("utf_8", "strict", ""), sizes=[])
    @hypothesis.example(case=("utf_8", "surrogatepass", "a\ud800b"),
                        sizes=[1, 1])
    @hypothesis.example(case=("utf_16", "strict", "ab"), sizes=[1])
    @hypothesis.example(case=("utf_7", "strict", "a\xe9b\xe9"), sizes=[2, 1])
    @hypothesis.example(case=("iso2022_jp", "replace", "a\u3042\xe9b"),
                        sizes=[1, 1, 1])
    @hypothesis.example(case=("hz", "xmlcharrefreplace", "~\u4e2d\u20ac"),
                        sizes=[1, 1])
    @hypothesis.example(case=("ascii", "namereplace", "\xe9"), sizes=[])
    @hypothesis.example(case=("cp437", "ignore", "a\u4e2db"), sizes=[2])
    # gh-160308: the punycode incremental encoder encodes each chunk on
    # its own, so two chunks give two "-" terminated labels.
    @hypothesis.example(case=("punycode", "strict", "ab"), sizes=[1]).xfail(
        reason="gh-160308", raises=AssertionError)
    @hypothesis.example(case=("punycode", "strict", "b\xfccher"),
                        sizes=[2]).xfail(
                            reason="gh-160308", raises=AssertionError)
    def test_encode_in_chunks(self, case, sizes):
        encoding, errors, text = case
        expected = _outcome(lambda: text.encode(encoding, errors))

        def incremental():
            encoder = codecs.getincrementalencoder(encoding)(errors)
            parts = [encoder.encode(chunk) for chunk in _chunks(text, sizes)]
            parts.append(encoder.encode("", final=True))
            return b"".join(parts)

        actual = _outcome(incremental)
        if encoding == "utf_7" and actual[0] == expected[0] == "ok":
            # The encoder closes its shift sequence at each chunk end, so
            # the bytes differ but must decode to the same text.
            actual = actual[1].decode(encoding)
            expected = expected[1].decode(encoding)
        self.assertEqual(actual, expected)

    @hypothesis.given(encoding=incremental_encoding, text=text, sizes=sizes)
    @hypothesis.example(encoding="utf_8", text="", sizes=[])
    @hypothesis.example(encoding="utf_16", text="a\u20acb", sizes=[1, 1])
    @hypothesis.example(encoding="iso2022_kr", text="\uac00a\uac01",
                        sizes=[1, 1])
    @hypothesis.example(encoding="cp037", text="a\xe9", sizes=[1])
    def test_iterencode_iterdecode(self, encoding, text, sizes):
        try:
            expected = text.encode(encoding).decode(encoding)
        except UnicodeError:
            return
        chunks = _chunks(text, sizes)
        encoded = list(codecs.iterencode(chunks, encoding))
        self.assertTrue(all(isinstance(e, bytes) for e in encoded))
        decoded = "".join(codecs.iterdecode(encoded, encoding))
        self.assertEqual(decoded, expected)

    @hypothesis.given(case=decode_case, data=data, split=st.integers(0, 64),
                      big_endian=st.booleans())
    @hypothesis.example(case=("utf_8", "strict"), data=b"", split=0,
                        big_endian=False)
    @hypothesis.example(case=("utf_8", "strict"), data=b"a\xe2\x82\xac",
                        split=2, big_endian=False)
    @hypothesis.example(case=("utf_16", "replace"), data=b"a\x00\x00\xd8b",
                        split=3, big_endian=False)
    @hypothesis.example(case=("utf_7", "strict"), data=b"a+AOk-b", split=3,
                        big_endian=False)
    @hypothesis.example(case=("gb18030", "replace"),
                        data=b"\x81\x30\x81\x30a", split=2, big_endian=False)
    @hypothesis.example(case=("iso2022_jp", "strict"),
                        data=b"\x1b$B$\"\x1b(Ba", split=4, big_endian=False)
    @hypothesis.example(case=("unicode_escape", "strict"), data=b"a\\x41",
                        split=2, big_endian=False)
    def test_decoder_state_round_trip(self, case, data, split, big_endian):
        # The conditions documented for IncrementalDecoder.getstate().
        encoding, errors = case
        data = _with_bom(encoding, data, big_endian)
        split = min(split, len(data))
        decoder = codecs.getincrementaldecoder(encoding)(errors)
        try:
            head = decoder.decode(data[:split])
        except UnicodeError:
            return
        state = decoder.getstate()
        self.assertIsInstance(state[0], bytes)
        self.assertIsInstance(state[1], int)
        if not state[1]:
            # Nothing but buffered bytes: after a reset, feeding them back
            # gives no output and reproduces the state.
            decoder.setstate((b"", 0))
            self.assertEqual(decoder.decode(state[0]), "")
            self.assertEqual(decoder.getstate(), state)

        resumed = codecs.getincrementaldecoder(encoding)(errors)
        resumed.setstate(state)
        actual = _outcome(lambda: head + resumed.decode(data[split:], True))
        expected = _outcome(lambda: data.decode(encoding, errors))
        self.assertEqual(actual, expected)

    @hypothesis.given(case=encode_case, split=st.integers(0, 64))
    @hypothesis.example(case=("utf_8", "strict", ""), split=0)
    @hypothesis.example(case=("utf_16", "strict", "ab"), split=1)
    @hypothesis.example(case=("utf_7", "strict", "a\xe9\xe9b"), split=2)
    @hypothesis.example(case=("iso2022_jp", "strict", "\u3042\u3044a"),
                        split=1)
    @hypothesis.example(case=("hz", "replace", "\u4e2d\u20ac\u4e2d"),
                        split=2)
    @hypothesis.example(case=("latin_1", "surrogateescape", "a\udc80b"),
                        split=1)
    def test_encoder_state_round_trip(self, case, split):
        encoding, errors, text = case
        split = min(split, len(text))
        encoder = codecs.getincrementalencoder(encoding)(errors)
        try:
            head = encoder.encode(text[:split])
        except UnicodeError:
            return
        state = encoder.getstate()
        resumed = codecs.getincrementalencoder(encoding)(errors)
        resumed.setstate(state)
        actual = _outcome(lambda: head + resumed.encode(text[split:], True))
        expected = _outcome(lambda: text.encode(encoding, errors))
        if encoding == "utf_7" and actual[0] == expected[0] == "ok":
            actual = actual[1].decode(encoding)
            expected = expected[1].decode(encoding)
        self.assertEqual(actual, expected)

    @hypothesis.given(encoding=st.sampled_from(sorted(BOM_ENCODINGS)),
                      data=data)
    @hypothesis.example(encoding="utf_16", data=b"")
    @hypothesis.example(encoding="utf_16", data=b"a\x00")
    @hypothesis.example(encoding="utf_16", data=b"\xff\xfea\x00")
    @hypothesis.example(encoding="utf_32", data=b"a\x00\x00")
    @hypothesis.example(encoding="utf_32", data=b"\x00\x00\xfe\xffa")
    def test_bom_codecs_reject_bare_input(self, encoding, data):
        # Without a byte order mark the incremental decoder raises, where
        # the stateless decoder would assume native order.
        decoder = codecs.getincrementaldecoder(encoding)()
        boms = tuple("\ufeff".encode(encoding + suffix)
                     for suffix in ("_le", "_be"))
        if data.startswith(boms):
            self.assertEqual(_outcome(lambda: decoder.decode(data, True)),
                             _outcome(lambda: data.decode(encoding)))
        elif not data:
            self.assertEqual(decoder.decode(data, True), "")
        else:
            with self.assertRaises(UnicodeDecodeError):
                decoder.decode(data, final=True)


# html.unescape resolves "&" sequences, reads references to the C1 range
# as cp1252 and drops references to the other control characters and to
# noncharacters, so none of those can be in text that is to come back
# from xmlcharrefreplace.
xml_safe_text = st.text(
    alphabet=st.characters(exclude_categories=["Cc", "Cn", "Cs"],
                           exclude_characters="&"),
    max_size=64)


class RoundTripTests(unittest.TestCase):

    @hypothesis.given(case=round_trip_case)
    @hypothesis.example(case=("utf_8", ""))
    @hypothesis.example(case=("utf_8", "a\u20ac\U0001f600"))
    @hypothesis.example(case=("utf_7", "a+b\u20ac"))
    @hypothesis.example(case=("shift_jis", "\xa5~"))
    @hypothesis.example(case=("iso2022_jp_2", "\u3042\xe9\uac00a"))
    @hypothesis.example(case=("big5hkscs", "\u00ca\u0304"))
    @hypothesis.example(case=("punycode", "b\xfccher"))
    @hypothesis.example(case=("idna", "b\xfccher.example"))
    @hypothesis.example(case=("unicode_escape", "\\n\n\u20ac"))
    # gh-160302: the ISO-2022 encoders write U+001B as a bare ESC byte,
    # which their decoders reject or read as the start of a sequence;
    # iso2022_kr does the same with the shift characters U+000E and U+000F.
    @hypothesis.example(case=("iso2022_jp", "\x1b")).xfail(
        reason="gh-160302", raises=(AssertionError, UnicodeDecodeError))
    @hypothesis.example(case=("iso2022_jp_2004", "\x1b\xa0")).xfail(
        reason="gh-160302", raises=(AssertionError, UnicodeDecodeError))
    @hypothesis.example(case=("iso2022_kr", "\x0e")).xfail(
        reason="gh-160302", raises=(AssertionError, UnicodeDecodeError))
    @hypothesis.example(case=("iso2022_kr", "a\x0f")).xfail(
        reason="gh-160302", raises=(AssertionError, UnicodeDecodeError))
    # gh-101863 (closed as not a bug): euc_kr encodes the Hangul filler
    # U+3164 as the two bytes that start an eight-byte composed syllable,
    # and will not decode them on their own.
    @hypothesis.example(case=("euc_kr", "\u3164")).xfail(
        reason="gh-101863", raises=UnicodeDecodeError)
    @hypothesis.example(case=("euc_kr", "a\u3164b")).xfail(
        reason="gh-101863", raises=UnicodeDecodeError)
    # gh-160306: iso2022_jp_3 encodes with the JIS X 0213:2000 table but
    # decodes with the 2004 one, where the same code point means U+9B1C,
    # which the 2000 table then cannot encode again.
    @hypothesis.example(case=("iso2022_jp_3", "\u9b1d")).xfail(
        reason="gh-160306", raises=(AssertionError, UnicodeEncodeError))
    def test_encode_decode(self, case):
        encoding, text = case
        try:
            encoded = text.encode(encoding)
        except UnicodeEncodeError:
            return
        self.assertIsInstance(encoded, bytes)
        decoded = encoded.decode(encoding)
        # encode is always a left inverse of decode on its own output...
        self.assertEqual(decoded.encode(encoding), encoded)
        # ...and a full inverse unless it is many-to-one by design.
        if encoding not in NON_INJECTIVE_ENCODINGS:
            self.assertEqual(decoded, text)

    @hypothesis.given(encoding=st.sampled_from(SURROGATEESCAPE_ENCODINGS),
                      data=data)
    @hypothesis.example(encoding="utf_8", data=b"")
    @hypothesis.example(encoding="utf_8", data=b"a\xff\xe2\x82")
    @hypothesis.example(encoding="ascii", data=b"\x80\xff")
    @hypothesis.example(encoding="cp1252", data=b"\x81\x8d")
    def test_surrogateescape_round_trip(self, encoding, data):
        decoded = data.decode(encoding, "surrogateescape")
        self.assertEqual(decoded.encode(encoding, "surrogateescape"), data)
        # The escapes are exactly the undecodable bytes.
        escaped = sum(1 for c in decoded if "\udc80" <= c <= "\udcff")
        try:
            data.decode(encoding)
        except UnicodeDecodeError:
            self.assertGreater(escaped, 0)
        else:
            self.assertEqual(escaped, 0)

    @hypothesis.given(case=handler_encoding.flatmap(
        lambda e: st.tuples(st.just(e), _round_trip_text(
            e, text_with_surrogates.map(lambda s: s.replace("\\", ""))))))
    @hypothesis.example(case=("ascii", ""))
    @hypothesis.example(case=("ascii", "a\xe9\u20ac\U0001f600\udc80"))
    @hypothesis.example(case=("cp1252", "\u20ac\u0100"))
    @hypothesis.example(case=("euc_kr", "\u3042\u20ac"))
    @hypothesis.example(case=("iso2022_jp_2004", "\xa0\u20ac"))
    @hypothesis.example(case=("punycode", "\xfc\udc80"))
    def test_backslashreplace_decodes_back(self, case):
        # backslashreplace writes unencodable characters as escapes that
        # unicode_escape understands, so as long as the text has no
        # backslashes of its own the original comes back.
        encoding, text = case
        encoded = text.encode(encoding, "backslashreplace")
        decoded = encoded.decode(encoding)
        as_ascii = decoded.encode("ascii", "backslashreplace")
        self.assertEqual(as_ascii.decode("unicode_escape"), text)

    @hypothesis.given(case=handler_encoding.flatmap(
        lambda e: st.tuples(st.just(e), _round_trip_text(e, xml_safe_text))))
    @hypothesis.example(case=("ascii", ""))
    @hypothesis.example(case=("ascii", "a<\xe9>\u20ac\U0001f600"))
    @hypothesis.example(case=("ascii", "\ufdcf\ufdf0\ufffd"))
    @hypothesis.example(case=("utf_7", "+\u20ac-"))
    @hypothesis.example(case=("shift_jis_2004", "\u3042\u20ac"))
    @hypothesis.example(case=("iso2022_kr", "\uac00\u20ac"))
    def test_xmlcharrefreplace_unescapes_back(self, case):
        encoding, text = case
        encoded = text.encode(encoding, "xmlcharrefreplace")
        self.assertEqual(html.unescape(encoded.decode(encoding)), text)

    @hypothesis.given(data=data)
    @hypothesis.example(data=b"")
    @hypothesis.example(data=b"\\\x00\n'\"\xff")
    def test_escape_codec(self, data):
        escaped, consumed = codecs.escape_encode(data)
        self.assertEqual(consumed, len(data))
        self.assertTrue(escaped.isascii())
        self.assertEqual(codecs.escape_decode(escaped), (data, len(escaped)))

    @hypothesis.given(text=text_with_surrogates)
    @hypothesis.example(text="")
    @hypothesis.example(text="\\\n\x00\xe9\u20ac\ud800\U0001f600")
    def test_unicode_escape_with_surrogates(self, text):
        # Both escape codecs accept lone surrogates.  unicode_escape writes
        # ASCII and gives anything back; raw_unicode_escape only escapes
        # what is not Latin-1, so it gives back text without backslashes.
        encoded = text.encode("unicode_escape")
        self.assertTrue(encoded.isascii())
        self.assertEqual(encoded.decode("unicode_escape"), text)
        raw_text = text.replace("\\", "")
        encoded = raw_text.encode("raw_unicode_escape")
        self.assertEqual(encoded.decode("raw_unicode_escape"), raw_text)

    @hypothesis.given(encoding=st.sampled_from(bytes_transform_encodings),
                      data=data)
    @hypothesis.example(encoding="base64_codec", data=b"")
    @hypothesis.example(encoding="quopri_codec", data=b"=\r\n \xff")
    @hypothesis.example(encoding="uu_codec", data=b"\x00" * 45 + b"a")
    def test_bytes_transform(self, encoding, data):
        encoded = codecs.encode(data, encoding)
        self.assertIsInstance(encoded, bytes)
        self.assertEqual(codecs.decode(encoded, encoding), data)

    @hypothesis.given(text=text)
    @hypothesis.example(text="")
    @hypothesis.example(text="Hello, \u20ac!")
    def test_rot13(self, text):
        encoded = codecs.encode(text, "rot_13")
        self.assertEqual(len(encoded), len(text))
        self.assertEqual(codecs.decode(encoded, "rot_13"), text)
        self.assertEqual(codecs.encode(encoded, "rot_13"), text)


# U+FFFE marks an undefined byte in a decoding table.
UNDEFINED = "\ufffe"
decoding_table = st.lists(
    st.characters() | st.just(UNDEFINED), min_size=256, max_size=256,
).map("".join)
# An injective table, so that encoding inverts decoding.  charmap_build
# skips the undefined marker, so it cannot be an entry here.
encoding_table = st.lists(
    st.characters(exclude_characters=UNDEFINED), min_size=256, max_size=256,
    unique=True,
).map("".join)


def _charmap_oracle(data, errors, table):
    # What charmap_decode should produce, from the documented behavior of
    # the error handlers on a one-byte-at-a-time codec.
    out = []
    for i, b in enumerate(data):
        c = table[b]
        if c != UNDEFINED:
            out.append(c)
        elif errors == "ignore":
            pass
        elif errors == "replace":
            out.append("\ufffd")
        elif errors == "backslashreplace":
            out.append(f"\\x{b:02x}")
        elif errors == "surrogateescape" and b >= 0x80:
            out.append(chr(0xdc00 + b))
        else:
            raise UnicodeDecodeError("charmap", data, i, i + 1, "undefined")
    return "".join(out)


class CharmapTests(unittest.TestCase):

    @hypothesis.given(table=decoding_table, data=data,
                      errors=st.sampled_from(DECODE_ERRORS))
    @hypothesis.example(table="".join(map(chr, range(256))), data=b"",
                        errors="strict")
    @hypothesis.example(table=UNDEFINED * 128 + "".join(map(chr, range(128))),
                        data=b"\x00\x80\xff", errors="replace")
    @hypothesis.example(table=UNDEFINED * 128 + "".join(map(chr, range(128))),
                        data=b"\x00\x80\xff", errors="surrogateescape")
    @hypothesis.example(table=UNDEFINED * 256, data=b"\x00", errors="strict")
    def test_decode_matches_table(self, table, data, errors):
        expected = _outcome(lambda: _charmap_oracle(data, errors, table))
        actual = _outcome(lambda: codecs.charmap_decode(data, errors, table))
        if actual[0] == "ok":
            actual = ("ok", actual[1][0])
            self.assertEqual(
                codecs.charmap_decode(data, errors, table)[1], len(data))
        self.assertEqual(actual, expected)
        # A dict mapping is accepted too and must agree.
        mapping = {i: c for i, c in enumerate(table) if c != UNDEFINED}
        self.assertEqual(
            _outcome(lambda: codecs.charmap_decode(data, errors, mapping)),
            _outcome(lambda: codecs.charmap_decode(data, errors, table)))

    @hypothesis.given(table=encoding_table,
                      indices=st.lists(st.integers(0, 255), max_size=32),
                      errors=st.sampled_from(ENCODE_ERRORS))
    @hypothesis.example(table="".join(map(chr, range(256))), indices=[],
                        errors="strict")
    @hypothesis.example(table="".join(map(chr, range(256, 512))),
                        indices=[0, 255, 1], errors="strict")
    def test_encode_inverts_table(self, table, indices, errors):
        # Encoding characters from the table gives back their positions,
        # through both kinds of encoding map.
        text = "".join(table[i] for i in indices)
        expected = bytes(indices)
        built = codecs.charmap_build(table)
        mapping = {ord(c): i for i, c in enumerate(table)}
        self.assertEqual(codecs.charmap_encode(text, errors, built),
                         (expected, len(text)))
        self.assertEqual(codecs.charmap_encode(text, errors, mapping),
                         (expected, len(text)))
        self.assertEqual(codecs.charmap_decode(expected, "strict", table),
                         (text, len(expected)))

    @hypothesis.given(table=encoding_table, text=text_with_surrogates,
                      errors=st.sampled_from(ENCODE_ERRORS))
    @hypothesis.example(table="".join(map(chr, range(256))), text="\u20ac",
                        errors="strict")
    @hypothesis.example(table="".join(map(chr, range(256))),
                        text="a\u20ac\udc80", errors="xmlcharrefreplace")
    @hypothesis.example(table="".join(map(chr, range(256))),
                        text="a\u20ac\udc80", errors="surrogateescape")
    def test_encode_maps_agree(self, table, text, errors):
        # The EncodingMap from charmap_build and an equivalent dict give
        # the same result or raise the same error.
        built = codecs.charmap_build(table)
        mapping = {ord(c): i for i, c in enumerate(table)}
        self.assertEqual(
            _outcome(lambda: codecs.charmap_encode(text, errors, built)),
            _outcome(lambda: codecs.charmap_encode(text, errors, mapping)))


# Characters that str.splitlines() treats as line boundaries, plus filler.
line_text = st.text(
    alphabet=st.sampled_from("ab \n\r\x0b\x0c\x1c\x1d\x1e\x85\u2028\u2029"),
    max_size=64)

# Complete encoded text, with the error handler the reader will be given.
# Nothing else is generated: StreamReader never tells the decoder that the
# stream has ended, so an incomplete trailing sequence is dropped instead
# of reported (gh-160309, the same mechanism as gh-56717); the
# expected-failure examples on test_reader_matches_decode pin that.
reader_case = incremental_encoding.flatmap(
    lambda e: st.tuples(st.just(e), st.sampled_from(DECODE_ERRORS),
                        _round_trip_text(e).map(
                            lambda s: s.encode(e, "replace"))))


class StreamTests(unittest.TestCase):

    @hypothesis.given(encoding=incremental_encoding, text=text,
                      write_sizes=sizes, read_sizes=read_sizes)
    @hypothesis.example(encoding="utf_8", text="", write_sizes=[],
                        read_sizes=[])
    @hypothesis.example(encoding="utf_8", text="a\u20ac\U0001f600",
                        write_sizes=[1, 1], read_sizes=[1, 2, -1])
    @hypothesis.example(encoding="utf_16", text="a\u20acb", write_sizes=[1],
                        read_sizes=[2, 0, 1])
    @hypothesis.example(encoding="utf_7", text="a\xe9\xe9b", write_sizes=[2],
                        read_sizes=[3])
    @hypothesis.example(encoding="iso2022_jp", text="\u3042a\u3044",
                        write_sizes=[1, 1], read_sizes=[1, 1, 1])
    @hypothesis.example(encoding="big5hkscs", text="\u00ca\u0304",
                        write_sizes=[], read_sizes=[1, 1])
    def test_writer_then_reader(self, encoding, text, write_sizes,
                                read_sizes):
        # The size passed to read() is a number of bytes, so a read may
        # well return more characters than that.
        try:
            expected = text.encode(encoding).decode(encoding)
        except UnicodeError:
            return
        stream = io.BytesIO()
        writer = codecs.getwriter(encoding)(stream)
        for chunk in _chunks(text, write_sizes):
            writer.write(chunk)
        self.assertEqual(stream.getvalue().decode(encoding), expected)

        stream.seek(0)
        reader = codecs.getreader(encoding)(stream)
        parts = [reader.read(n) for n in read_sizes]
        parts.append(reader.read())
        self.assertEqual("".join(parts), expected)
        self.assertEqual(reader.read(), "")

    @hypothesis.given(case=reader_case, read_sizes=read_sizes)
    @hypothesis.example(case=("utf_8", "strict", b""), read_sizes=[])
    @hypothesis.example(case=("utf_8", "replace", b"a\xe2\x82\xac"),
                        read_sizes=[1, 2])
    @hypothesis.example(case=("utf_16", "strict", b"\xff\xfea\x00"),
                        read_sizes=[1])
    @hypothesis.example(case=("euc_jp", "ignore", b"\xa4\xa2a"),
                        read_sizes=[1, 1])
    @hypothesis.example(case=("unicode_escape", "strict", b"a\\u00e9"),
                        read_sizes=[2])
    # gh-160309: a chopped trailing character is dropped rather than
    # reported, whatever the error handler.
    @hypothesis.example(case=("utf_8", "strict", b"ab\xe2"),
                        read_sizes=[]).xfail(
                            reason="gh-160309", raises=AssertionError)
    @hypothesis.example(case=("utf_8", "replace", b"a\xe2\x82"),
                        read_sizes=[1]).xfail(
                            reason="gh-160309", raises=AssertionError)
    @hypothesis.example(case=("utf_16", "strict", b"\xff\xfeB"),
                        read_sizes=[]).xfail(
                            reason="gh-160309", raises=AssertionError)
    @hypothesis.example(case=("utf_32_be", "replace", b"\x00\x000"),
                        read_sizes=[2]).xfail(
                            reason="gh-160309", raises=AssertionError)
    @hypothesis.example(case=("raw_unicode_escape", "strict", b"a\\u30"),
                        read_sizes=[]).xfail(
                            reason="gh-160309", raises=AssertionError)
    def test_reader_matches_decode(self, case, read_sizes):
        encoding, errors, data = case
        expected = _outcome(lambda: data.decode(encoding, errors))

        def read_all():
            reader = codecs.getreader(encoding)(io.BytesIO(data), errors)
            parts = [reader.read(n) for n in read_sizes]
            parts.append(reader.read())
            return "".join(parts)

        self.assertEqual(_outcome(read_all), expected)

    @hypothesis.given(
        encoding=st.sampled_from(PYTHON_READER_ENCODINGS), text=text,
        reads=st.lists(st.tuples(st.sampled_from([-1, 1, 2, 3, 8]),
                                 st.integers(0, 8)), max_size=16))
    @hypothesis.example(encoding="utf_8", text="", reads=[])
    @hypothesis.example(encoding="utf_8", text="a\u20ac\U0001f600b",
                        reads=[(1, 2), (-1, 0), (8, 1), (1, 8)])
    @hypothesis.example(encoding="utf_16", text="a\u20acb",
                        reads=[(1, 1), (3, 2)])
    @hypothesis.example(encoding="utf_7", text="a\xe9\xe9b",
                        reads=[(1, 3), (2, 1)])
    @hypothesis.example(encoding="unicode_escape", text="\\a\u20ac",
                        reads=[(1, 1), (1, 1)])
    def test_reader_read_chars(self, encoding, text, reads):
        # Asked for a number of characters, the Python StreamReader keeps
        # reading until it has them, however many bytes it takes at a time
        # (the multibyte codecs' readers only take a byte count).
        try:
            expected = text.encode(encoding).decode(encoding)
        except UnicodeError:
            return
        reader = codecs.getreader(encoding)(io.BytesIO(text.encode(encoding)))
        remaining = expected
        for size, chars in reads:
            part = reader.read(size, chars)
            self.assertEqual(part, remaining[:chars])
            remaining = remaining[len(part):]
        self.assertEqual(reader.read(), remaining)

    @hypothesis.given(text=line_text, keepends=st.booleans())
    @hypothesis.example(text="", keepends=True)
    @hypothesis.example(text="a\r\nb\rc\nd", keepends=True)
    @hypothesis.example(text="a\r\nb\rc\nd", keepends=False)
    @hypothesis.example(text="\u2028\x85\n\r", keepends=True)
    def test_reader_readline(self, text, keepends):
        expected = text.splitlines(keepends)
        reader = codecs.getreader("utf_8")(io.BytesIO(text.encode()))
        lines = []
        while line := reader.readline(keepends=keepends):
            lines.append(line)
        if not keepends:
            # An empty line without its ending is indistinguishable from
            # the end of the stream, so the loop above stops early there.
            n = len(lines)
            self.assertEqual(lines, expected[:n])
            self.assertTrue(n == len(expected) or expected[n] == "")
        else:
            self.assertEqual(lines, expected)

        reader = codecs.getreader("utf_8")(io.BytesIO(text.encode()))
        self.assertEqual(reader.readlines(keepends=keepends), expected)
        reader = codecs.getreader("utf_8")(io.BytesIO(text.encode()))
        self.assertEqual(list(reader), text.splitlines(True))

    @hypothesis.given(
        text=line_text,
        ops=st.lists(st.integers(-1, 6) | st.just("line") | st.just("next"),
                     max_size=16))
    @hypothesis.example(text="", ops=[])
    @hypothesis.example(text="a\r\nb\rc\nd\n", ops=["line", 1, "line", -1])
    @hypothesis.example(text="a\nb\nc\nd\n",
                        ops=["line", "line", 0, 2, "next"])
    @hypothesis.example(text="ab\r\n", ops=[3, "line", "line"])
    def test_reader_mixed_reads(self, text, ops):
        # Interleaving read() and readline() exercises the reader's line
        # buffer; the result must match reading from the decoded text.
        reader = codecs.getreader("utf_8")(io.BytesIO(text.encode()))
        remaining = text
        for op in ops:
            if op == "line" or op == "next":
                lines = remaining.splitlines(True)
                expected = lines[0] if lines else ""
                if op == "next" and not expected:
                    with self.assertRaises(StopIteration):
                        next(reader)
                    continue
                actual = next(reader) if op == "next" else reader.readline()
            elif op < 0:
                expected = remaining
                actual = reader.read()
            else:
                expected = remaining[:op]
                actual = reader.read(op)
            self.assertEqual(actual, expected)
            remaining = remaining[len(expected):]
        self.assertEqual(reader.read(), remaining)


class LookupTests(unittest.TestCase):

    @hypothesis.given(
        encoding=st.sampled_from(UNICODE_ENCODINGS + ("rot_13",)),
        separators=st.lists(st.sampled_from(["_", "-", " ", "--", "_ -"]),
                            min_size=8, max_size=8),
        upper=st.lists(st.booleans(), min_size=16, max_size=16))
    @hypothesis.example(encoding="utf_8", separators=["-"] * 8,
                        upper=[True] * 16)
    @hypothesis.example(encoding="iso8859_1", separators=["_ -"] * 8,
                        upper=[False, True] * 8)
    @hypothesis.example(encoding="shift_jis_2004", separators=[" "] * 8,
                        upper=[True] * 16)
    def test_lookup_name_normalization(self, encoding, separators, upper):
        # Case and the punctuation between name parts do not matter.
        parts = encoding.split("_")
        joined = "".join(
            part + (separators[i] if i < len(parts) - 1 else "")
            for i, part in enumerate(parts))
        variant = "".join(c.upper() if upper[i % len(upper)] else c
                          for i, c in enumerate(joined))
        self.assertEqual(codecs.lookup(variant).name,
                         codecs.lookup(encoding).name)
        self.assertEqual(encodings.normalize_encoding(variant).lower(),
                         encodings.normalize_encoding(encoding))
        if encoding != "rot_13":
            self.assertEqual("x".encode(variant), "x".encode(encoding))


if __name__ == "__main__":
    unittest.main()
