import test.test_support, unittest
import sys, codecs, htmlentitydefs, unicodedata

class PosReturn:
    # this can be used for configurable callbacks

    def __init__(self):
        self.pos = 0

    def handle(self, exc):
        oldpos = self.pos
        realpos = oldpos
        if realpos<0:
            realpos = len(exc.object) + realpos
        # if we don't advance this time, terminate on the next call
        # otherwise we'd get an endless loop
        if realpos <= exc.start:
            self.pos = len(exc.object)
        return ("<?>", oldpos)

# A UnicodeEncodeError object with a bad start attribute
class BadStartUnicodeEncodeError(UnicodeEncodeError):
    def __init__(self):
        UnicodeEncodeError.__init__(self, "ascii", "", 0, 1, "bad")
        self.start = []

# A UnicodeEncodeError object with a bad object attribute
class BadObjectUnicodeEncodeError(UnicodeEncodeError):
    def __init__(self):
        UnicodeEncodeError.__init__(self, "ascii", "", 0, 1, "bad")
        self.object = []

# A UnicodeDecodeError object without an end attribute
class NoEndUnicodeDecodeError(UnicodeDecodeError):
    def __init__(self):
        UnicodeDecodeError.__init__(self, "ascii", b"", 0, 1, "bad")
        del self.end

# A UnicodeDecodeError object with a bad object attribute
class BadObjectUnicodeDecodeError(UnicodeDecodeError):
    def __init__(self):
        UnicodeDecodeError.__init__(self, "ascii", b"", 0, 1, "bad")
        self.object = []

# A UnicodeTranslateError object without a start attribute
class NoStartUnicodeTranslateError(UnicodeTranslateError):
    def __init__(self):
        UnicodeTranslateError.__init__(self, "", 0, 1, "bad")
        del self.start

# A UnicodeTranslateError object without an end attribute
class NoEndUnicodeTranslateError(UnicodeTranslateError):
    def __init__(self):
        UnicodeTranslateError.__init__(self,  "", 0, 1, "bad")
        del self.end

# A UnicodeTranslateError object without an object attribute
class NoObjectUnicodeTranslateError(UnicodeTranslateError):
    def __init__(self):
        UnicodeTranslateError.__init__(self, "", 0, 1, "bad")
        del self.object

class CodecCallbackTest(unittest.TestCase):

    def test_xmlcharrefreplace(self):
        # replace unencodable characters which numeric character entities.
        # For ascii, latin-1 and charmaps this is completely implemented
        # in C and should be reasonably fast.
        s = "\u30b9\u30d1\u30e2 \xe4nd eggs"
        self.assertEqual(
            s.encode("ascii", "xmlcharrefreplace"),
            b"&#12473;&#12497;&#12514; &#228;nd eggs"
        )
        self.assertEqual(
            s.encode("latin-1", "xmlcharrefreplace"),
            b"&#12473;&#12497;&#12514; \xe4nd eggs"
        )

    def test_xmlcharnamereplace(self):
        # This time use a named character entity for unencodable
        # characters, if one is available.

        def xmlcharnamereplace(exc):
            if not isinstance(exc, UnicodeEncodeError):
                raise TypeError("don't know how to handle %r" % exc)
            l = []
            for c in exc.object[exc.start:exc.end]:
                try:
                    l.append("&%s;" % htmlentitydefs.codepoint2name[ord(c)])
                except KeyError:
                    l.append("&#%d;" % ord(c))
            return ("".join(l), exc.end)

        codecs.register_error(
            "test.xmlcharnamereplace", xmlcharnamereplace)

        sin = "\xab\u211c\xbb = \u2329\u1234\u20ac\u232a"
        sout = b"&laquo;&real;&raquo; = &lang;&#4660;&euro;&rang;"
        self.assertEqual(sin.encode("ascii", "test.xmlcharnamereplace"), sout)
        sout = b"\xab&real;\xbb = &lang;&#4660;&euro;&rang;"
        self.assertEqual(sin.encode("latin-1", "test.xmlcharnamereplace"), sout)
        sout = b"\xab&real;\xbb = &lang;&#4660;\xa4&rang;"
        self.assertEqual(sin.encode("iso-8859-15", "test.xmlcharnamereplace"), sout)

    def test_uninamereplace(self):
        # We're using the names from the unicode database this time,
        # and we're doing "syntax highlighting" here, i.e. we include
        # the replaced text in ANSI escape sequences. For this it is
        # useful that the error handler is not called for every single
        # unencodable character, but for a complete sequence of
        # unencodable characters, otherwise we would output many
        # unneccessary escape sequences.

        def uninamereplace(exc):
            if not isinstance(exc, UnicodeEncodeError):
                raise TypeError("don't know how to handle %r" % exc)
            l = []
            for c in exc.object[exc.start:exc.end]:
                l.append(unicodedata.name(c, "0x%x" % ord(c)))
            return ("\033[1m%s\033[0m" % ", ".join(l), exc.end)

        codecs.register_error(
            "test.uninamereplace", uninamereplace)

        sin = "\xac\u1234\u20ac\u8000"
        sout = b"\033[1mNOT SIGN, ETHIOPIC SYLLABLE SEE, EURO SIGN, CJK UNIFIED IDEOGRAPH-8000\033[0m"
        self.assertEqual(sin.encode("ascii", "test.uninamereplace"), sout)

        sout = b"\xac\033[1mETHIOPIC SYLLABLE SEE, EURO SIGN, CJK UNIFIED IDEOGRAPH-8000\033[0m"
        self.assertEqual(sin.encode("latin-1", "test.uninamereplace"), sout)

        sout = b"\xac\033[1mETHIOPIC SYLLABLE SEE\033[0m\xa4\033[1mCJK UNIFIED IDEOGRAPH-8000\033[0m"
        self.assertEqual(sin.encode("iso-8859-15", "test.uninamereplace"), sout)

    def test_backslashescape(self):
        # Does the same as the "unicode-escape" encoding, but with different
        # base encodings.
        sin = "a\xac\u1234\u20ac\u8000"
        if sys.maxunicode > 0xffff:
            sin += chr(sys.maxunicode)
        sout = b"a\\xac\\u1234\\u20ac\\u8000"
        if sys.maxunicode > 0xffff:
            sout += bytes("\\U%08x" % sys.maxunicode)
        self.assertEqual(sin.encode("ascii", "backslashreplace"), sout)

        sout = b"a\xac\\u1234\\u20ac\\u8000"
        if sys.maxunicode > 0xffff:
            sout += bytes("\\U%08x" % sys.maxunicode)
        self.assertEqual(sin.encode("latin-1", "backslashreplace"), sout)

        sout = b"a\xac\\u1234\xa4\\u8000"
        if sys.maxunicode > 0xffff:
            sout += bytes("\\U%08x" % sys.maxunicode)
        self.assertEqual(sin.encode("iso-8859-15", "backslashreplace"), sout)

    def test_decoderelaxedutf8(self):
        # This is the test for a decoding callback handler,
        # that relaxes the UTF-8 minimal encoding restriction.
        # A null byte that is encoded as "\xc0\x80" will be
        # decoded as a null byte. All other illegal sequences
        # will be handled strictly.
        def relaxedutf8(exc):
            if not isinstance(exc, UnicodeDecodeError):
                raise TypeError("don't know how to handle %r" % exc)
            if exc.object[exc.start:exc.end].startswith(b"\xc0\x80"):
                return ("\x00", exc.start+2) # retry after two bytes
            else:
                raise exc

        codecs.register_error(
            "test.relaxedutf8", relaxedutf8)

        sin = b"a\x00b\xc0\x80c\xc3\xbc\xc0\x80\xc0\x80"
        sout = "a\x00b\x00c\xfc\x00\x00"
        self.assertEqual(sin.decode("utf-8", "test.relaxedutf8"), sout)
        sin = b"\xc0\x80\xc0\x81"
        self.assertRaises(UnicodeError, sin.decode, "utf-8", "test.relaxedutf8")

    def test_charmapencode(self):
        # For charmap encodings the replacement string will be
        # mapped through the encoding again. This means, that
        # to be able to use e.g. the "replace" handler, the
        # charmap has to have a mapping for "?".
        charmap = dict((ord(c), str8(2*c.upper())) for c in "abcdefgh")
        sin = "abc"
        sout = b"AABBCC"
        self.assertEquals(codecs.charmap_encode(sin, "strict", charmap)[0], sout)

        sin = "abcA"
        self.assertRaises(UnicodeError, codecs.charmap_encode, sin, "strict", charmap)

        charmap[ord("?")] = str8("XYZ")
        sin = "abcDEF"
        sout = b"AABBCCXYZXYZXYZ"
        self.assertEquals(codecs.charmap_encode(sin, "replace", charmap)[0], sout)

        charmap[ord("?")] = "XYZ" # wrong type in mapping
        self.assertRaises(TypeError, codecs.charmap_encode, sin, "replace", charmap)

    def test_decodeunicodeinternal(self):
        self.assertRaises(
            UnicodeDecodeError,
            b"\x00\x00\x00\x00\x00".decode,
            "unicode-internal",
        )
        if sys.maxunicode > 0xffff:
            def handler_unicodeinternal(exc):
                if not isinstance(exc, UnicodeDecodeError):
                    raise TypeError("don't know how to handle %r" % exc)
                return ("\x01", 1)

            self.assertEqual(
                b"\x00\x00\x00\x00\x00".decode("unicode-internal", "ignore"),
                "\u0000"
            )

            self.assertEqual(
                b"\x00\x00\x00\x00\x00".decode("unicode-internal", "replace"),
                "\u0000\ufffd"
            )

            codecs.register_error("test.hui", handler_unicodeinternal)

            self.assertEqual(
                b"\x00\x00\x00\x00\x00".decode("unicode-internal", "test.hui"),
                "\u0000\u0001\u0000"
            )

    def test_callbacks(self):
        def handler1(exc):
            r = range(exc.start, exc.end)
            if isinstance(exc, UnicodeEncodeError):
                l = ["<%d>" % ord(exc.object[pos]) for pos in r]
            elif isinstance(exc, UnicodeDecodeError):
                l = ["<%d>" % exc.object[pos] for pos in r]
            else:
                raise TypeError("don't know how to handle %r" % exc)
            return ("[%s]" % "".join(l), exc.end)

        codecs.register_error("test.handler1", handler1)

        def handler2(exc):
            if not isinstance(exc, UnicodeDecodeError):
                raise TypeError("don't know how to handle %r" % exc)
            l = ["<%d>" % exc.object[pos] for pos in range(exc.start, exc.end)]
            return ("[%s]" % "".join(l), exc.end+1) # skip one character

        codecs.register_error("test.handler2", handler2)

        s = b"\x00\x81\x7f\x80\xff"

        self.assertEqual(
            s.decode("ascii", "test.handler1"),
            "\x00[<129>]\x7f[<128>][<255>]"
        )
        self.assertEqual(
            s.decode("ascii", "test.handler2"),
            "\x00[<129>][<128>]"
        )

        self.assertEqual(
            b"\\u3042\u3xxx".decode("unicode-escape", "test.handler1"),
            "\u3042[<92><117><51><120>]xx"
        )

        self.assertEqual(
            b"\\u3042\u3xx".decode("unicode-escape", "test.handler1"),
            "\u3042[<92><117><51><120><120>]"
        )

        self.assertEqual(
            codecs.charmap_decode(b"abc", "test.handler1", {ord("a"): "z"})[0],
            "z[<98>][<99>]"
        )

        self.assertEqual(
            "g\xfc\xdfrk".encode("ascii", "test.handler1"),
            b"g[<252><223>]rk"
        )

        self.assertEqual(
            "g\xfc\xdf".encode("ascii", "test.handler1"),
            b"g[<252><223>]"
        )

    def test_longstrings(self):
        # test long strings to check for memory overflow problems
        errors = [ "strict", "ignore", "replace", "xmlcharrefreplace",
                   "backslashreplace"]
        # register the handlers under different names,
        # to prevent the codec from recognizing the name
        for err in errors:
            codecs.register_error("test." + err, codecs.lookup_error(err))
        l = 1000
        errors += [ "test." + err for err in errors ]
        for uni in [ s*l for s in ("x", "\u3042", "a\xe4") ]:
            for enc in ("ascii", "latin-1", "iso-8859-1", "iso-8859-15",
                        "utf-8", "utf-7", "utf-16", "utf-32"):
                for err in errors:
                    try:
                        uni.encode(enc, err)
                    except UnicodeError:
                        pass

    def check_exceptionobjectargs(self, exctype, args, msg):
        # Test UnicodeError subclasses: construction, attribute assignment and __str__ conversion
        # check with one missing argument
        self.assertRaises(TypeError, exctype, *args[:-1])
        # check with one argument too much
        self.assertRaises(TypeError, exctype, *(args + ["too much"]))
        # check with one argument of the wrong type
        wrongargs = [ "spam", str8("eggs"), b"spam", 42, 1.0, None ]
        for i in range(len(args)):
            for wrongarg in wrongargs:
                if type(wrongarg) is type(args[i]):
                    continue
                # build argument array
                callargs = []
                for j in range(len(args)):
                    if i==j:
                        callargs.append(wrongarg)
                    else:
                        callargs.append(args[i])
                self.assertRaises(TypeError, exctype, *callargs)

        # check with the correct number and type of arguments
        exc = exctype(*args)
        self.assertEquals(str(exc), msg)

    def test_unicodeencodeerror(self):
        self.check_exceptionobjectargs(
            UnicodeEncodeError,
            ["ascii", "g\xfcrk", 1, 2, "ouch"],
            "'ascii' codec can't encode character '\\xfc' in position 1: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeEncodeError,
            ["ascii", "g\xfcrk", 1, 4, "ouch"],
            "'ascii' codec can't encode characters in position 1-3: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeEncodeError,
            ["ascii", "\xfcx", 0, 1, "ouch"],
            "'ascii' codec can't encode character '\\xfc' in position 0: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeEncodeError,
            ["ascii", "\u0100x", 0, 1, "ouch"],
            "'ascii' codec can't encode character '\\u0100' in position 0: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeEncodeError,
            ["ascii", "\uffffx", 0, 1, "ouch"],
            "'ascii' codec can't encode character '\\uffff' in position 0: ouch"
        )
        if sys.maxunicode > 0xffff:
            self.check_exceptionobjectargs(
                UnicodeEncodeError,
                ["ascii", "\U00010000x", 0, 1, "ouch"],
                "'ascii' codec can't encode character '\\U00010000' in position 0: ouch"
            )

    def test_unicodedecodeerror(self):
        self.check_exceptionobjectargs(
            UnicodeDecodeError,
            ["ascii", b"g\xfcrk", 1, 2, "ouch"],
            "'ascii' codec can't decode byte 0xfc in position 1: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeDecodeError,
            ["ascii", b"g\xfcrk", 1, 3, "ouch"],
            "'ascii' codec can't decode bytes in position 1-2: ouch"
        )

    def test_unicodetranslateerror(self):
        self.check_exceptionobjectargs(
            UnicodeTranslateError,
            ["g\xfcrk", 1, 2, "ouch"],
            "can't translate character '\\xfc' in position 1: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeTranslateError,
            ["g\u0100rk", 1, 2, "ouch"],
            "can't translate character '\\u0100' in position 1: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeTranslateError,
            ["g\uffffrk", 1, 2, "ouch"],
            "can't translate character '\\uffff' in position 1: ouch"
        )
        if sys.maxunicode > 0xffff:
            self.check_exceptionobjectargs(
                UnicodeTranslateError,
                ["g\U00010000rk", 1, 2, "ouch"],
                "can't translate character '\\U00010000' in position 1: ouch"
            )
        self.check_exceptionobjectargs(
            UnicodeTranslateError,
            ["g\xfcrk", 1, 3, "ouch"],
            "can't translate characters in position 1-2: ouch"
        )

    def test_badandgoodstrictexceptions(self):
        # "strict" complains about a non-exception passed in
        self.assertRaises(
            TypeError,
            codecs.strict_errors,
            42
        )
        # "strict" complains about the wrong exception type
        self.assertRaises(
            Exception,
            codecs.strict_errors,
            Exception("ouch")
        )

        # If the correct exception is passed in, "strict" raises it
        self.assertRaises(
            UnicodeEncodeError,
            codecs.strict_errors,
            UnicodeEncodeError("ascii", "\u3042", 0, 1, "ouch")
        )

    def test_badandgoodignoreexceptions(self):
        # "ignore" complains about a non-exception passed in
        self.assertRaises(
           TypeError,
           codecs.ignore_errors,
           42
        )
        # "ignore" complains about the wrong exception type
        self.assertRaises(
           TypeError,
           codecs.ignore_errors,
           UnicodeError("ouch")
        )
        # If the correct exception is passed in, "ignore" returns an empty replacement
        self.assertEquals(
            codecs.ignore_errors(
                UnicodeEncodeError("ascii", "\u3042", 0, 1, "ouch")),
            ("", 1)
        )
        self.assertEquals(
            codecs.ignore_errors(
                UnicodeDecodeError("ascii", b"\xff", 0, 1, "ouch")),
            ("", 1)
        )
        self.assertEquals(
            codecs.ignore_errors(
                UnicodeTranslateError("\u3042", 0, 1, "ouch")),
            ("", 1)
        )

    def test_badandgoodreplaceexceptions(self):
        # "replace" complains about a non-exception passed in
        self.assertRaises(
           TypeError,
           codecs.replace_errors,
           42
        )
        # "replace" complains about the wrong exception type
        self.assertRaises(
           TypeError,
           codecs.replace_errors,
           UnicodeError("ouch")
        )
        self.assertRaises(
            TypeError,
            codecs.replace_errors,
            BadObjectUnicodeEncodeError()
        )
        self.assertRaises(
            TypeError,
            codecs.replace_errors,
            BadObjectUnicodeDecodeError()
        )
        # With the correct exception, "replace" returns an "?" or "\ufffd" replacement
        self.assertEquals(
            codecs.replace_errors(
                UnicodeEncodeError("ascii", "\u3042", 0, 1, "ouch")),
            ("?", 1)
        )
        self.assertEquals(
            codecs.replace_errors(
                UnicodeDecodeError("ascii", b"\xff", 0, 1, "ouch")),
            ("\ufffd", 1)
        )
        self.assertEquals(
            codecs.replace_errors(
                UnicodeTranslateError("\u3042", 0, 1, "ouch")),
            ("\ufffd", 1)
        )

    def test_badandgoodxmlcharrefreplaceexceptions(self):
        # "xmlcharrefreplace" complains about a non-exception passed in
        self.assertRaises(
           TypeError,
           codecs.xmlcharrefreplace_errors,
           42
        )
        # "xmlcharrefreplace" complains about the wrong exception types
        self.assertRaises(
           TypeError,
           codecs.xmlcharrefreplace_errors,
           UnicodeError("ouch")
        )
        # "xmlcharrefreplace" can only be used for encoding
        self.assertRaises(
            TypeError,
            codecs.xmlcharrefreplace_errors,
            UnicodeDecodeError("ascii", b"\xff", 0, 1, "ouch")
        )
        self.assertRaises(
            TypeError,
            codecs.xmlcharrefreplace_errors,
            UnicodeTranslateError("\u3042", 0, 1, "ouch")
        )
        # Use the correct exception
        cs = (0, 1, 9, 10, 99, 100, 999, 1000, 9999, 10000, 0x3042)
        s = "".join(chr(c) for c in cs)
        self.assertEquals(
            codecs.xmlcharrefreplace_errors(
                UnicodeEncodeError("ascii", s, 0, len(s), "ouch")
            ),
            ("".join("&#%d;" % ord(c) for c in s), len(s))
        )

    def test_badandgoodbackslashreplaceexceptions(self):
        # "backslashreplace" complains about a non-exception passed in
        self.assertRaises(
           TypeError,
           codecs.backslashreplace_errors,
           42
        )
        # "backslashreplace" complains about the wrong exception types
        self.assertRaises(
           TypeError,
           codecs.backslashreplace_errors,
           UnicodeError("ouch")
        )
        # "backslashreplace" can only be used for encoding
        self.assertRaises(
            TypeError,
            codecs.backslashreplace_errors,
            UnicodeDecodeError("ascii", b"\xff", 0, 1, "ouch")
        )
        self.assertRaises(
            TypeError,
            codecs.backslashreplace_errors,
            UnicodeTranslateError("\u3042", 0, 1, "ouch")
        )
        # Use the correct exception
        self.assertEquals(
            codecs.backslashreplace_errors(
                UnicodeEncodeError("ascii", "\u3042", 0, 1, "ouch")),
            ("\\u3042", 1)
        )
        self.assertEquals(
            codecs.backslashreplace_errors(
                UnicodeEncodeError("ascii", "\x00", 0, 1, "ouch")),
            ("\\x00", 1)
        )
        self.assertEquals(
            codecs.backslashreplace_errors(
                UnicodeEncodeError("ascii", "\xff", 0, 1, "ouch")),
            ("\\xff", 1)
        )
        self.assertEquals(
            codecs.backslashreplace_errors(
                UnicodeEncodeError("ascii", "\u0100", 0, 1, "ouch")),
            ("\\u0100", 1)
        )
        self.assertEquals(
            codecs.backslashreplace_errors(
                UnicodeEncodeError("ascii", "\uffff", 0, 1, "ouch")),
            ("\\uffff", 1)
        )
        if sys.maxunicode>0xffff:
            self.assertEquals(
                codecs.backslashreplace_errors(
                    UnicodeEncodeError("ascii", "\U00010000", 0, 1, "ouch")),
                ("\\U00010000", 1)
            )
            self.assertEquals(
                codecs.backslashreplace_errors(
                    UnicodeEncodeError("ascii", "\U0010ffff", 0, 1, "ouch")),
                ("\\U0010ffff", 1)
            )

    def test_badhandlerresults(self):
        results = ( 42, "foo", (1,2,3), ("foo", 1, 3), ("foo", None), ("foo",), ("foo", 1, 3), ("foo", None), ("foo",) )
        encs = ("ascii", "latin-1", "iso-8859-1", "iso-8859-15")

        for res in results:
            codecs.register_error("test.badhandler", lambda: res)
            for enc in encs:
                self.assertRaises(
                    TypeError,
                    "\u3042".encode,
                    enc,
                    "test.badhandler"
                )
            for (enc, bytes) in (
                ("ascii", b"\xff"),
                ("utf-8", b"\xff"),
                ("utf-7", b"+x-"),
                ("unicode-internal", b"\x00"),
            ):
                self.assertRaises(
                    TypeError,
                    bytes.decode,
                    enc,
                    "test.badhandler"
                )

    def test_lookup(self):
        self.assertEquals(codecs.strict_errors, codecs.lookup_error("strict"))
        self.assertEquals(codecs.ignore_errors, codecs.lookup_error("ignore"))
        self.assertEquals(codecs.strict_errors, codecs.lookup_error("strict"))
        self.assertEquals(
            codecs.xmlcharrefreplace_errors,
            codecs.lookup_error("xmlcharrefreplace")
        )
        self.assertEquals(
            codecs.backslashreplace_errors,
            codecs.lookup_error("backslashreplace")
        )

    def test_unencodablereplacement(self):
        def unencrepl(exc):
            if isinstance(exc, UnicodeEncodeError):
                return ("\u4242", exc.end)
            else:
                raise TypeError("don't know how to handle %r" % exc)
        codecs.register_error("test.unencreplhandler", unencrepl)
        for enc in ("ascii", "iso-8859-1", "iso-8859-15"):
            self.assertRaises(
                UnicodeEncodeError,
                "\u4242".encode,
                enc,
                "test.unencreplhandler"
            )

    def test_badregistercall(self):
        # enhance coverage of:
        # Modules/_codecsmodule.c::register_error()
        # Python/codecs.c::PyCodec_RegisterError()
        self.assertRaises(TypeError, codecs.register_error, 42)
        self.assertRaises(TypeError, codecs.register_error, "test.dummy", 42)

    def test_badlookupcall(self):
        # enhance coverage of:
        # Modules/_codecsmodule.c::lookup_error()
        self.assertRaises(TypeError, codecs.lookup_error)

    def test_unknownhandler(self):
        # enhance coverage of:
        # Modules/_codecsmodule.c::lookup_error()
        self.assertRaises(LookupError, codecs.lookup_error, "test.unknown")

    def test_xmlcharrefvalues(self):
        # enhance coverage of:
        # Python/codecs.c::PyCodec_XMLCharRefReplaceErrors()
        # and inline implementations
        v = (1, 5, 10, 50, 100, 500, 1000, 5000, 10000, 50000)
        if sys.maxunicode>=100000:
            v += (100000, 500000, 1000000)
        s = "".join([chr(x) for x in v])
        codecs.register_error("test.xmlcharrefreplace", codecs.xmlcharrefreplace_errors)
        for enc in ("ascii", "iso-8859-15"):
            for err in ("xmlcharrefreplace", "test.xmlcharrefreplace"):
                s.encode(enc, err)

    def test_decodehelper(self):
        # enhance coverage of:
        # Objects/unicodeobject.c::unicode_decode_call_errorhandler()
        # and callers
        self.assertRaises(LookupError, b"\xff".decode, "ascii", "test.unknown")

        def baddecodereturn1(exc):
            return 42
        codecs.register_error("test.baddecodereturn1", baddecodereturn1)
        self.assertRaises(TypeError, b"\xff".decode, "ascii", "test.baddecodereturn1")
        self.assertRaises(TypeError, b"\\".decode, "unicode-escape", "test.baddecodereturn1")
        self.assertRaises(TypeError, b"\\x0".decode, "unicode-escape", "test.baddecodereturn1")
        self.assertRaises(TypeError, b"\\x0y".decode, "unicode-escape", "test.baddecodereturn1")
        self.assertRaises(TypeError, b"\\Uffffeeee".decode, "unicode-escape", "test.baddecodereturn1")
        self.assertRaises(TypeError, b"\\uyyyy".decode, "raw-unicode-escape", "test.baddecodereturn1")

        def baddecodereturn2(exc):
            return ("?", None)
        codecs.register_error("test.baddecodereturn2", baddecodereturn2)
        self.assertRaises(TypeError, b"\xff".decode, "ascii", "test.baddecodereturn2")

        handler = PosReturn()
        codecs.register_error("test.posreturn", handler.handle)

        # Valid negative position
        handler.pos = -1
        self.assertEquals(b"\xff0".decode("ascii", "test.posreturn"), "<?>0")

        # Valid negative position
        handler.pos = -2
        self.assertEquals(b"\xff0".decode("ascii", "test.posreturn"), "<?><?>")

        # Negative position out of bounds
        handler.pos = -3
        self.assertRaises(IndexError, b"\xff0".decode, "ascii", "test.posreturn")

        # Valid positive position
        handler.pos = 1
        self.assertEquals(b"\xff0".decode("ascii", "test.posreturn"), "<?>0")

        # Largest valid positive position (one beyond end of input)
        handler.pos = 2
        self.assertEquals(b"\xff0".decode("ascii", "test.posreturn"), "<?>")

        # Invalid positive position
        handler.pos = 3
        self.assertRaises(IndexError, b"\xff0".decode, "ascii", "test.posreturn")

        # Restart at the "0"
        handler.pos = 6
        self.assertEquals(b"\\uyyyy0".decode("raw-unicode-escape", "test.posreturn"), "<?>0")

        class D(dict):
            def __getitem__(self, key):
                raise ValueError
        self.assertRaises(UnicodeError, codecs.charmap_decode, b"\xff", "strict", {0xff: None})
        self.assertRaises(ValueError, codecs.charmap_decode, b"\xff", "strict", D())
        self.assertRaises(TypeError, codecs.charmap_decode, b"\xff", "strict", {0xff: sys.maxunicode+1})

    def test_encodehelper(self):
        # enhance coverage of:
        # Objects/unicodeobject.c::unicode_encode_call_errorhandler()
        # and callers
        self.assertRaises(LookupError, "\xff".encode, "ascii", "test.unknown")

        def badencodereturn1(exc):
            return 42
        codecs.register_error("test.badencodereturn1", badencodereturn1)
        self.assertRaises(TypeError, "\xff".encode, "ascii", "test.badencodereturn1")

        def badencodereturn2(exc):
            return ("?", None)
        codecs.register_error("test.badencodereturn2", badencodereturn2)
        self.assertRaises(TypeError, "\xff".encode, "ascii", "test.badencodereturn2")

        handler = PosReturn()
        codecs.register_error("test.posreturn", handler.handle)

        # Valid negative position
        handler.pos = -1
        self.assertEquals("\xff0".encode("ascii", "test.posreturn"), b"<?>0")

        # Valid negative position
        handler.pos = -2
        self.assertEquals("\xff0".encode("ascii", "test.posreturn"), b"<?><?>")

        # Negative position out of bounds
        handler.pos = -3
        self.assertRaises(IndexError, "\xff0".encode, "ascii", "test.posreturn")

        # Valid positive position
        handler.pos = 1
        self.assertEquals("\xff0".encode("ascii", "test.posreturn"), b"<?>0")

        # Largest valid positive position (one beyond end of input
        handler.pos = 2
        self.assertEquals("\xff0".encode("ascii", "test.posreturn"), b"<?>")

        # Invalid positive position
        handler.pos = 3
        self.assertRaises(IndexError, "\xff0".encode, "ascii", "test.posreturn")

        handler.pos = 0

        class D(dict):
            def __getitem__(self, key):
                raise ValueError
        for err in ("strict", "replace", "xmlcharrefreplace", "backslashreplace", "test.posreturn"):
            self.assertRaises(UnicodeError, codecs.charmap_encode, "\xff", err, {0xff: None})
            self.assertRaises(ValueError, codecs.charmap_encode, "\xff", err, D())
            self.assertRaises(TypeError, codecs.charmap_encode, "\xff", err, {0xff: 300})

    def test_translatehelper(self):
        # enhance coverage of:
        # Objects/unicodeobject.c::unicode_encode_call_errorhandler()
        # and callers
        # (Unfortunately the errors argument is not directly accessible
        # from Python, so we can't test that much)
        class D(dict):
            def __getitem__(self, key):
                raise ValueError
        self.assertRaises(ValueError, "\xff".translate, D())
        self.assertRaises(TypeError, "\xff".translate, {0xff: sys.maxunicode+1})
        self.assertRaises(TypeError, "\xff".translate, {0xff: ()})

    def test_bug828737(self):
        charmap = {
            ord("&"): "&amp;",
            ord("<"): "&lt;",
            ord(">"): "&gt;",
            ord('"'): "&quot;",
        }

        for n in (1, 10, 100, 1000):
            text = 'abc<def>ghi'*n
            text.translate(charmap)

    def test_mutatingdecodehandler(self):
        baddata = [
            ("ascii", b"\xff"),
            ("utf-7", b"++"),
            ("utf-8",  b"\xff"),
            ("utf-16", b"\xff"),
            ("utf-32", b"\xff"),
            ("unicode-escape", b"\\u123g"),
            ("raw-unicode-escape", b"\\u123g"),
            ("unicode-internal", b"\xff"),
        ]

        def replacing(exc):
            if isinstance(exc, UnicodeDecodeError):
                exc.object = 42
                return ("\u4242", 0)
            else:
                raise TypeError("don't know how to handle %r" % exc)
        codecs.register_error("test.replacing", replacing)
        for (encoding, data) in baddata:
            self.assertRaises(TypeError, data.decode, encoding, "test.replacing")

        def mutating(exc):
            if isinstance(exc, UnicodeDecodeError):
                exc.object[:] = b""
                return ("\u4242", 0)
            else:
                raise TypeError("don't know how to handle %r" % exc)
        codecs.register_error("test.mutating", mutating)
        # If the decoder doesn't pick up the modified input the following
        # will lead to an endless loop
        for (encoding, data) in baddata:
            self.assertRaises(TypeError, data.decode, encoding, "test.replacing")

def test_main():
    test.test_support.run_unittest(CodecCallbackTest)

if __name__ == "__main__":
    test_main()
