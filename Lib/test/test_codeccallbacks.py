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
        return (u"<?>", oldpos)

# A UnicodeEncodeError object with a bad start attribute
class BadStartUnicodeEncodeError(UnicodeEncodeError):
    def __init__(self):
        UnicodeEncodeError.__init__(self, "ascii", u"", 0, 1, "bad")
        self.start = []

# A UnicodeEncodeError object with a bad object attribute
class BadObjectUnicodeEncodeError(UnicodeEncodeError):
    def __init__(self):
        UnicodeEncodeError.__init__(self, "ascii", u"", 0, 1, "bad")
        self.object = []

# A UnicodeDecodeError object without an end attribute
class NoEndUnicodeDecodeError(UnicodeDecodeError):
    def __init__(self):
        UnicodeDecodeError.__init__(self, "ascii", "", 0, 1, "bad")
        del self.end

# A UnicodeDecodeError object with a bad object attribute
class BadObjectUnicodeDecodeError(UnicodeDecodeError):
    def __init__(self):
        UnicodeDecodeError.__init__(self, "ascii", "", 0, 1, "bad")
        self.object = []

# A UnicodeTranslateError object without a start attribute
class NoStartUnicodeTranslateError(UnicodeTranslateError):
    def __init__(self):
        UnicodeTranslateError.__init__(self, u"", 0, 1, "bad")
        del self.start

# A UnicodeTranslateError object without an end attribute
class NoEndUnicodeTranslateError(UnicodeTranslateError):
    def __init__(self):
        UnicodeTranslateError.__init__(self,  u"", 0, 1, "bad")
        del self.end

# A UnicodeTranslateError object without an object attribute
class NoObjectUnicodeTranslateError(UnicodeTranslateError):
    def __init__(self):
        UnicodeTranslateError.__init__(self, u"", 0, 1, "bad")
        del self.object

class CodecCallbackTest(unittest.TestCase):

    def test_xmlcharrefreplace(self):
        # replace unencodable characters which numeric character entities.
        # For ascii, latin-1 and charmaps this is completely implemented
        # in C and should be reasonably fast.
        s = u"\u30b9\u30d1\u30e2 \xe4nd eggs"
        self.assertEqual(
            s.encode("ascii", "xmlcharrefreplace"),
            "&#12473;&#12497;&#12514; &#228;nd eggs"
        )
        self.assertEqual(
            s.encode("latin-1", "xmlcharrefreplace"),
            "&#12473;&#12497;&#12514; \xe4nd eggs"
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
                    l.append(u"&%s;" % htmlentitydefs.codepoint2name[ord(c)])
                except KeyError:
                    l.append(u"&#%d;" % ord(c))
            return (u"".join(l), exc.end)

        codecs.register_error(
            "test.xmlcharnamereplace", xmlcharnamereplace)

        sin = u"\xab\u211c\xbb = \u2329\u1234\u20ac\u232a"
        sout = "&laquo;&real;&raquo; = &lang;&#4660;&euro;&rang;"
        self.assertEqual(sin.encode("ascii", "test.xmlcharnamereplace"), sout)
        sout = "\xab&real;\xbb = &lang;&#4660;&euro;&rang;"
        self.assertEqual(sin.encode("latin-1", "test.xmlcharnamereplace"), sout)
        sout = "\xab&real;\xbb = &lang;&#4660;\xa4&rang;"
        self.assertEqual(sin.encode("iso-8859-15", "test.xmlcharnamereplace"), sout)

    def test_uninamereplace(self):
        # We're using the names from the unicode database this time,
        # and we're doing "syntax highlighting" here, i.e. we include
        # the replaced text in ANSI escape sequences. For this it is
        # useful that the error handler is not called for every single
        # unencodable character, but for a complete sequence of
        # unencodable characters, otherwise we would output many
        # unnecessary escape sequences.

        def uninamereplace(exc):
            if not isinstance(exc, UnicodeEncodeError):
                raise TypeError("don't know how to handle %r" % exc)
            l = []
            for c in exc.object[exc.start:exc.end]:
                l.append(unicodedata.name(c, u"0x%x" % ord(c)))
            return (u"\033[1m%s\033[0m" % u", ".join(l), exc.end)

        codecs.register_error(
            "test.uninamereplace", uninamereplace)

        sin = u"\xac\u1234\u20ac\u8000"
        sout = "\033[1mNOT SIGN, ETHIOPIC SYLLABLE SEE, EURO SIGN, CJK UNIFIED IDEOGRAPH-8000\033[0m"
        self.assertEqual(sin.encode("ascii", "test.uninamereplace"), sout)

        sout = "\xac\033[1mETHIOPIC SYLLABLE SEE, EURO SIGN, CJK UNIFIED IDEOGRAPH-8000\033[0m"
        self.assertEqual(sin.encode("latin-1", "test.uninamereplace"), sout)

        sout = "\xac\033[1mETHIOPIC SYLLABLE SEE\033[0m\xa4\033[1mCJK UNIFIED IDEOGRAPH-8000\033[0m"
        self.assertEqual(sin.encode("iso-8859-15", "test.uninamereplace"), sout)

    def test_backslashescape(self):
        # Does the same as the "unicode-escape" encoding, but with different
        # base encodings.
        sin = u"a\xac\u1234\u20ac\u8000"
        if sys.maxunicode > 0xffff:
            sin += unichr(sys.maxunicode)
        sout = "a\\xac\\u1234\\u20ac\\u8000"
        if sys.maxunicode > 0xffff:
            sout += "\\U%08x" % sys.maxunicode
        self.assertEqual(sin.encode("ascii", "backslashreplace"), sout)

        sout = "a\xac\\u1234\\u20ac\\u8000"
        if sys.maxunicode > 0xffff:
            sout += "\\U%08x" % sys.maxunicode
        self.assertEqual(sin.encode("latin-1", "backslashreplace"), sout)

        sout = "a\xac\\u1234\xa4\\u8000"
        if sys.maxunicode > 0xffff:
            sout += "\\U%08x" % sys.maxunicode
        self.assertEqual(sin.encode("iso-8859-15", "backslashreplace"), sout)

    def test_decoding_callbacks(self):
        # This is a test for a decoding callback handler
        # that allows the decoding of the invalid sequence
        # "\xc0\x80" and returns "\x00" instead of raising an error.
        # All other illegal sequences will be handled strictly.
        def relaxedutf8(exc):
            if not isinstance(exc, UnicodeDecodeError):
                raise TypeError("don't know how to handle %r" % exc)
            if exc.object[exc.start:exc.start+2] == "\xc0\x80":
                return (u"\x00", exc.start+2) # retry after two bytes
            else:
                raise exc

        codecs.register_error("test.relaxedutf8", relaxedutf8)

        # all the "\xc0\x80" will be decoded to "\x00"
        sin = "a\x00b\xc0\x80c\xc3\xbc\xc0\x80\xc0\x80"
        sout = u"a\x00b\x00c\xfc\x00\x00"
        self.assertEqual(sin.decode("utf-8", "test.relaxedutf8"), sout)

        # "\xc0\x81" is not valid and a UnicodeDecodeError will be raised
        sin = "\xc0\x80\xc0\x81"
        self.assertRaises(UnicodeDecodeError, sin.decode,
                          "utf-8", "test.relaxedutf8")

    def test_charmapencode(self):
        # For charmap encodings the replacement string will be
        # mapped through the encoding again. This means, that
        # to be able to use e.g. the "replace" handler, the
        # charmap has to have a mapping for "?".
        charmap = dict([ (ord(c), 2*c.upper()) for c in "abcdefgh"])
        sin = u"abc"
        sout = "AABBCC"
        self.assertEqual(codecs.charmap_encode(sin, "strict", charmap)[0], sout)

        sin = u"abcA"
        self.assertRaises(UnicodeError, codecs.charmap_encode, sin, "strict", charmap)

        charmap[ord("?")] = "XYZ"
        sin = u"abcDEF"
        sout = "AABBCCXYZXYZXYZ"
        self.assertEqual(codecs.charmap_encode(sin, "replace", charmap)[0], sout)

        charmap[ord("?")] = u"XYZ"
        self.assertRaises(TypeError, codecs.charmap_encode, sin, "replace", charmap)

        charmap[ord("?")] = u"XYZ"
        self.assertRaises(TypeError, codecs.charmap_encode, sin, "replace", charmap)

    def test_decodeunicodeinternal(self):
        self.assertRaises(
            UnicodeDecodeError,
            "\x00\x00\x00\x00\x00".decode,
            "unicode-internal",
        )
        if sys.maxunicode > 0xffff:
            def handler_unicodeinternal(exc):
                if not isinstance(exc, UnicodeDecodeError):
                    raise TypeError("don't know how to handle %r" % exc)
                return (u"\x01", 1)

            self.assertEqual(
                "\x00\x00\x00\x00\x00".decode("unicode-internal", "ignore"),
                u"\u0000"
            )

            self.assertEqual(
                "\x00\x00\x00\x00\x00".decode("unicode-internal", "replace"),
                u"\u0000\ufffd"
            )

            codecs.register_error("test.hui", handler_unicodeinternal)

            self.assertEqual(
                "\x00\x00\x00\x00\x00".decode("unicode-internal", "test.hui"),
                u"\u0000\u0001\u0000"
            )

    def test_callbacks(self):
        def handler1(exc):
            if not isinstance(exc, UnicodeEncodeError) \
               and not isinstance(exc, UnicodeDecodeError):
                raise TypeError("don't know how to handle %r" % exc)
            l = [u"<%d>" % ord(exc.object[pos]) for pos in xrange(exc.start, exc.end)]
            return (u"[%s]" % u"".join(l), exc.end)

        codecs.register_error("test.handler1", handler1)

        def handler2(exc):
            if not isinstance(exc, UnicodeDecodeError):
                raise TypeError("don't know how to handle %r" % exc)
            l = [u"<%d>" % ord(exc.object[pos]) for pos in xrange(exc.start, exc.end)]
            return (u"[%s]" % u"".join(l), exc.end+1) # skip one character

        codecs.register_error("test.handler2", handler2)

        s = "\x00\x81\x7f\x80\xff"

        self.assertEqual(
            s.decode("ascii", "test.handler1"),
            u"\x00[<129>]\x7f[<128>][<255>]"
        )
        self.assertEqual(
            s.decode("ascii", "test.handler2"),
            u"\x00[<129>][<128>]"
        )

        self.assertEqual(
            "\\u3042\u3xxx".decode("unicode-escape", "test.handler1"),
            u"\u3042[<92><117><51><120>]xx"
        )

        self.assertEqual(
            "\\u3042\u3xx".decode("unicode-escape", "test.handler1"),
            u"\u3042[<92><117><51><120><120>]"
        )

        self.assertEqual(
            codecs.charmap_decode("abc", "test.handler1", {ord("a"): u"z"})[0],
            u"z[<98>][<99>]"
        )

        self.assertEqual(
            u"g\xfc\xdfrk".encode("ascii", "test.handler1"),
            u"g[<252><223>]rk"
        )

        self.assertEqual(
            u"g\xfc\xdf".encode("ascii", "test.handler1"),
            u"g[<252><223>]"
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
        for uni in [ s*l for s in (u"x", u"\u3042", u"a\xe4") ]:
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
        wrongargs = [ "spam", u"eggs", 42, 1.0, None ]
        for i in xrange(len(args)):
            for wrongarg in wrongargs:
                if type(wrongarg) is type(args[i]):
                    continue
                # build argument array
                callargs = []
                for j in xrange(len(args)):
                    if i==j:
                        callargs.append(wrongarg)
                    else:
                        callargs.append(args[i])
                self.assertRaises(TypeError, exctype, *callargs)

        # check with the correct number and type of arguments
        exc = exctype(*args)
        self.assertEqual(str(exc), msg)

    def test_unicodeencodeerror(self):
        self.check_exceptionobjectargs(
            UnicodeEncodeError,
            ["ascii", u"g\xfcrk", 1, 2, "ouch"],
            "'ascii' codec can't encode character u'\\xfc' in position 1: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeEncodeError,
            ["ascii", u"g\xfcrk", 1, 4, "ouch"],
            "'ascii' codec can't encode characters in position 1-3: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeEncodeError,
            ["ascii", u"\xfcx", 0, 1, "ouch"],
            "'ascii' codec can't encode character u'\\xfc' in position 0: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeEncodeError,
            ["ascii", u"\u0100x", 0, 1, "ouch"],
            "'ascii' codec can't encode character u'\\u0100' in position 0: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeEncodeError,
            ["ascii", u"\uffffx", 0, 1, "ouch"],
            "'ascii' codec can't encode character u'\\uffff' in position 0: ouch"
        )
        if sys.maxunicode > 0xffff:
            self.check_exceptionobjectargs(
                UnicodeEncodeError,
                ["ascii", u"\U00010000x", 0, 1, "ouch"],
                "'ascii' codec can't encode character u'\\U00010000' in position 0: ouch"
            )

    def test_unicodedecodeerror(self):
        self.check_exceptionobjectargs(
            UnicodeDecodeError,
            ["ascii", "g\xfcrk", 1, 2, "ouch"],
            "'ascii' codec can't decode byte 0xfc in position 1: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeDecodeError,
            ["ascii", "g\xfcrk", 1, 3, "ouch"],
            "'ascii' codec can't decode bytes in position 1-2: ouch"
        )

    def test_unicodetranslateerror(self):
        self.check_exceptionobjectargs(
            UnicodeTranslateError,
            [u"g\xfcrk", 1, 2, "ouch"],
            "can't translate character u'\\xfc' in position 1: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeTranslateError,
            [u"g\u0100rk", 1, 2, "ouch"],
            "can't translate character u'\\u0100' in position 1: ouch"
        )
        self.check_exceptionobjectargs(
            UnicodeTranslateError,
            [u"g\uffffrk", 1, 2, "ouch"],
            "can't translate character u'\\uffff' in position 1: ouch"
        )
        if sys.maxunicode > 0xffff:
            self.check_exceptionobjectargs(
                UnicodeTranslateError,
                [u"g\U00010000rk", 1, 2, "ouch"],
                "can't translate character u'\\U00010000' in position 1: ouch"
            )
        self.check_exceptionobjectargs(
            UnicodeTranslateError,
            [u"g\xfcrk", 1, 3, "ouch"],
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
            UnicodeEncodeError("ascii", u"\u3042", 0, 1, "ouch")
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
        self.assertEqual(
            codecs.ignore_errors(UnicodeEncodeError("ascii", u"\u3042", 0, 1, "ouch")),
            (u"", 1)
        )
        self.assertEqual(
            codecs.ignore_errors(UnicodeDecodeError("ascii", "\xff", 0, 1, "ouch")),
            (u"", 1)
        )
        self.assertEqual(
            codecs.ignore_errors(UnicodeTranslateError(u"\u3042", 0, 1, "ouch")),
            (u"", 1)
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
        # With the correct exception, "replace" returns an "?" or u"\ufffd" replacement
        self.assertEqual(
            codecs.replace_errors(UnicodeEncodeError("ascii", u"\u3042", 0, 1, "ouch")),
            (u"?", 1)
        )
        self.assertEqual(
            codecs.replace_errors(UnicodeDecodeError("ascii", "\xff", 0, 1, "ouch")),
            (u"\ufffd", 1)
        )
        self.assertEqual(
            codecs.replace_errors(UnicodeTranslateError(u"\u3042", 0, 1, "ouch")),
            (u"\ufffd", 1)
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
            UnicodeDecodeError("ascii", "\xff", 0, 1, "ouch")
        )
        self.assertRaises(
            TypeError,
            codecs.xmlcharrefreplace_errors,
            UnicodeTranslateError(u"\u3042", 0, 1, "ouch")
        )
        # Use the correct exception
        cs = (0, 1, 9, 10, 99, 100, 999, 1000, 9999, 10000, 0x3042)
        s = "".join(unichr(c) for c in cs)
        self.assertEqual(
            codecs.xmlcharrefreplace_errors(
                UnicodeEncodeError("ascii", s, 0, len(s), "ouch")
            ),
            (u"".join(u"&#%d;" % ord(c) for c in s), len(s))
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
            UnicodeDecodeError("ascii", "\xff", 0, 1, "ouch")
        )
        self.assertRaises(
            TypeError,
            codecs.backslashreplace_errors,
            UnicodeTranslateError(u"\u3042", 0, 1, "ouch")
        )
        # Use the correct exception
        self.assertEqual(
            codecs.backslashreplace_errors(UnicodeEncodeError("ascii", u"\u3042", 0, 1, "ouch")),
            (u"\\u3042", 1)
        )
        self.assertEqual(
            codecs.backslashreplace_errors(UnicodeEncodeError("ascii", u"\x00", 0, 1, "ouch")),
            (u"\\x00", 1)
        )
        self.assertEqual(
            codecs.backslashreplace_errors(UnicodeEncodeError("ascii", u"\xff", 0, 1, "ouch")),
            (u"\\xff", 1)
        )
        self.assertEqual(
            codecs.backslashreplace_errors(UnicodeEncodeError("ascii", u"\u0100", 0, 1, "ouch")),
            (u"\\u0100", 1)
        )
        self.assertEqual(
            codecs.backslashreplace_errors(UnicodeEncodeError("ascii", u"\uffff", 0, 1, "ouch")),
            (u"\\uffff", 1)
        )
        if sys.maxunicode>0xffff:
            self.assertEqual(
                codecs.backslashreplace_errors(UnicodeEncodeError("ascii", u"\U00010000", 0, 1, "ouch")),
                (u"\\U00010000", 1)
            )
            self.assertEqual(
                codecs.backslashreplace_errors(UnicodeEncodeError("ascii", u"\U0010ffff", 0, 1, "ouch")),
                (u"\\U0010ffff", 1)
            )

    def test_badhandlerresults(self):
        results = ( 42, u"foo", (1,2,3), (u"foo", 1, 3), (u"foo", None), (u"foo",), ("foo", 1, 3), ("foo", None), ("foo",) )
        encs = ("ascii", "latin-1", "iso-8859-1", "iso-8859-15")

        for res in results:
            codecs.register_error("test.badhandler", lambda x: res)
            for enc in encs:
                self.assertRaises(
                    TypeError,
                    u"\u3042".encode,
                    enc,
                    "test.badhandler"
                )
            for (enc, bytes) in (
                ("ascii", "\xff"),
                ("utf-8", "\xff"),
                ("utf-7", "+x-"),
                ("unicode-internal", "\x00"),
            ):
                self.assertRaises(
                    TypeError,
                    bytes.decode,
                    enc,
                    "test.badhandler"
                )

    def test_lookup(self):
        self.assertEqual(codecs.strict_errors, codecs.lookup_error("strict"))
        self.assertEqual(codecs.ignore_errors, codecs.lookup_error("ignore"))
        self.assertEqual(codecs.strict_errors, codecs.lookup_error("strict"))
        self.assertEqual(
            codecs.xmlcharrefreplace_errors,
            codecs.lookup_error("xmlcharrefreplace")
        )
        self.assertEqual(
            codecs.backslashreplace_errors,
            codecs.lookup_error("backslashreplace")
        )

    def test_unencodablereplacement(self):
        def unencrepl(exc):
            if isinstance(exc, UnicodeEncodeError):
                return (u"\u4242", exc.end)
            else:
                raise TypeError("don't know how to handle %r" % exc)
        codecs.register_error("test.unencreplhandler", unencrepl)
        for enc in ("ascii", "iso-8859-1", "iso-8859-15"):
            self.assertRaises(
                UnicodeEncodeError,
                u"\u4242".encode,
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
        s = u"".join([unichr(x) for x in v])
        codecs.register_error("test.xmlcharrefreplace", codecs.xmlcharrefreplace_errors)
        for enc in ("ascii", "iso-8859-15"):
            for err in ("xmlcharrefreplace", "test.xmlcharrefreplace"):
                s.encode(enc, err)

    def test_decodehelper(self):
        # enhance coverage of:
        # Objects/unicodeobject.c::unicode_decode_call_errorhandler()
        # and callers
        self.assertRaises(LookupError, "\xff".decode, "ascii", "test.unknown")

        def baddecodereturn1(exc):
            return 42
        codecs.register_error("test.baddecodereturn1", baddecodereturn1)
        self.assertRaises(TypeError, "\xff".decode, "ascii", "test.baddecodereturn1")
        self.assertRaises(TypeError, "\\".decode, "unicode-escape", "test.baddecodereturn1")
        self.assertRaises(TypeError, "\\x0".decode, "unicode-escape", "test.baddecodereturn1")
        self.assertRaises(TypeError, "\\x0y".decode, "unicode-escape", "test.baddecodereturn1")
        self.assertRaises(TypeError, "\\Uffffeeee".decode, "unicode-escape", "test.baddecodereturn1")
        self.assertRaises(TypeError, "\\uyyyy".decode, "raw-unicode-escape", "test.baddecodereturn1")

        def baddecodereturn2(exc):
            return (u"?", None)
        codecs.register_error("test.baddecodereturn2", baddecodereturn2)
        self.assertRaises(TypeError, "\xff".decode, "ascii", "test.baddecodereturn2")

        handler = PosReturn()
        codecs.register_error("test.posreturn", handler.handle)

        # Valid negative position
        handler.pos = -1
        self.assertEqual("\xff0".decode("ascii", "test.posreturn"), u"<?>0")

        # Valid negative position
        handler.pos = -2
        self.assertEqual("\xff0".decode("ascii", "test.posreturn"), u"<?><?>")

        # Negative position out of bounds
        handler.pos = -3
        self.assertRaises(IndexError, "\xff0".decode, "ascii", "test.posreturn")

        # Valid positive position
        handler.pos = 1
        self.assertEqual("\xff0".decode("ascii", "test.posreturn"), u"<?>0")

        # Largest valid positive position (one beyond end of input)
        handler.pos = 2
        self.assertEqual("\xff0".decode("ascii", "test.posreturn"), u"<?>")

        # Invalid positive position
        handler.pos = 3
        self.assertRaises(IndexError, "\xff0".decode, "ascii", "test.posreturn")

        # Restart at the "0"
        handler.pos = 6
        self.assertEqual("\\uyyyy0".decode("raw-unicode-escape", "test.posreturn"), u"<?>0")

        class D(dict):
            def __getitem__(self, key):
                raise ValueError
        self.assertRaises(UnicodeError, codecs.charmap_decode, "\xff", "strict", {0xff: None})
        self.assertRaises(ValueError, codecs.charmap_decode, "\xff", "strict", D())
        self.assertRaises(TypeError, codecs.charmap_decode, "\xff", "strict", {0xff: sys.maxunicode+1})

    def test_encodehelper(self):
        # enhance coverage of:
        # Objects/unicodeobject.c::unicode_encode_call_errorhandler()
        # and callers
        self.assertRaises(LookupError, u"\xff".encode, "ascii", "test.unknown")

        def badencodereturn1(exc):
            return 42
        codecs.register_error("test.badencodereturn1", badencodereturn1)
        self.assertRaises(TypeError, u"\xff".encode, "ascii", "test.badencodereturn1")

        def badencodereturn2(exc):
            return (u"?", None)
        codecs.register_error("test.badencodereturn2", badencodereturn2)
        self.assertRaises(TypeError, u"\xff".encode, "ascii", "test.badencodereturn2")

        handler = PosReturn()
        codecs.register_error("test.posreturn", handler.handle)

        # Valid negative position
        handler.pos = -1
        self.assertEqual(u"\xff0".encode("ascii", "test.posreturn"), "<?>0")

        # Valid negative position
        handler.pos = -2
        self.assertEqual(u"\xff0".encode("ascii", "test.posreturn"), "<?><?>")

        # Negative position out of bounds
        handler.pos = -3
        self.assertRaises(IndexError, u"\xff0".encode, "ascii", "test.posreturn")

        # Valid positive position
        handler.pos = 1
        self.assertEqual(u"\xff0".encode("ascii", "test.posreturn"), "<?>0")

        # Largest valid positive position (one beyond end of input
        handler.pos = 2
        self.assertEqual(u"\xff0".encode("ascii", "test.posreturn"), "<?>")

        # Invalid positive position
        handler.pos = 3
        self.assertRaises(IndexError, u"\xff0".encode, "ascii", "test.posreturn")

        handler.pos = 0

        class D(dict):
            def __getitem__(self, key):
                raise ValueError
        for err in ("strict", "replace", "xmlcharrefreplace", "backslashreplace", "test.posreturn"):
            self.assertRaises(UnicodeError, codecs.charmap_encode, u"\xff", err, {0xff: None})
            self.assertRaises(ValueError, codecs.charmap_encode, u"\xff", err, D())
            self.assertRaises(TypeError, codecs.charmap_encode, u"\xff", err, {0xff: 300})

    def test_translatehelper(self):
        # enhance coverage of:
        # Objects/unicodeobject.c::unicode_encode_call_errorhandler()
        # and callers
        # (Unfortunately the errors argument is not directly accessible
        # from Python, so we can't test that much)
        class D(dict):
            def __getitem__(self, key):
                raise ValueError
        self.assertRaises(ValueError, u"\xff".translate, D())
        self.assertRaises(TypeError, u"\xff".translate, {0xff: sys.maxunicode+1})
        self.assertRaises(TypeError, u"\xff".translate, {0xff: ()})

    def test_bug828737(self):
        charmap = {
            ord("&"): u"&amp;",
            ord("<"): u"&lt;",
            ord(">"): u"&gt;",
            ord('"'): u"&quot;",
        }

        for n in (1, 10, 100, 1000):
            text = u'abc<def>ghi'*n
            text.translate(charmap)

def test_main():
    test.test_support.run_unittest(CodecCallbackTest)

if __name__ == "__main__":
    test_main()
