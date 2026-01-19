from test.test_json import CTest


class BadBool:
    def __bool__(self):
        1/0


class TestSpeedups(CTest):
    def test_scanstring(self):
        self.assertEqual(self.json.decoder.scanstring.__module__, "_json")
        self.assertIs(self.json.decoder.scanstring, self.json.decoder.c_scanstring)

    def test_encode_basestring_ascii(self):
        self.assertEqual(self.json.encoder.encode_basestring_ascii.__module__,
                         "_json")
        self.assertIs(self.json.encoder.encode_basestring_ascii,
                      self.json.encoder.c_encode_basestring_ascii)


class TestDecode(CTest):
    def test_make_scanner(self):
        self.assertRaises(AttributeError, self.json.scanner.c_make_scanner, 1)

    def test_bad_bool_args(self):
        def test(value):
            self.json.decoder.JSONDecoder(strict=BadBool()).decode(value)
        self.assertRaises(ZeroDivisionError, test, '""')
        self.assertRaises(ZeroDivisionError, test, '{}')


class TestEncode(CTest):
    def test_make_encoder(self):
        # bpo-6986: The interpreter shouldn't crash in case c_make_encoder()
        # receives invalid arguments.
        self.assertRaises(TypeError, self.json.encoder.c_make_encoder,
            (True, False),
            b"\xCD\x7D\x3D\x4E\x12\x4C\xF9\x79\xD7\x52\xBA\x82\xF2\x27\x4A\x7D\xA0\xCA\x75",
            None)

    def test_bad_str_encoder(self):
        # Issue #31505: There shouldn't be an assertion failure in case
        # c_make_encoder() receives a bad encoder() argument.
        def bad_encoder1(*args):
            return None
        enc = self.json.encoder.c_make_encoder(None, lambda obj: str(obj),
                                               bad_encoder1, None, ': ', ', ',
                                               False, False, False)
        with self.assertRaises(TypeError):
            enc('spam', 4)
        with self.assertRaises(TypeError):
            enc({'spam': 42}, 4)

        def bad_encoder2(*args):
            1/0
        enc = self.json.encoder.c_make_encoder(None, lambda obj: str(obj),
                                               bad_encoder2, None, ': ', ', ',
                                               False, False, False)
        with self.assertRaises(ZeroDivisionError):
            enc('spam', 4)

    def test_bad_markers_argument_to_encoder(self):
        # https://bugs.python.org/issue45269
        with self.assertRaisesRegex(
            TypeError,
            r'make_encoder\(\) argument 1 must be dict or None, not int',
        ):
            self.json.encoder.c_make_encoder(1, None, None, None, ': ', ', ',
                                             False, False, False)

    def test_bad_bool_args(self):
        def test(name):
            self.json.encoder.JSONEncoder(**{name: BadBool()}).encode({'a': 1})
        self.assertRaises(ZeroDivisionError, test, 'skipkeys')
        self.assertRaises(ZeroDivisionError, test, 'ensure_ascii')
        self.assertRaises(ZeroDivisionError, test, 'check_circular')
        self.assertRaises(ZeroDivisionError, test, 'allow_nan')
        self.assertRaises(ZeroDivisionError, test, 'sort_keys')

    def test_unsortable_keys(self):
        with self.assertRaises(TypeError):
            self.json.encoder.JSONEncoder(sort_keys=True).encode({'a': 1, 1: 'a'})

    def test_current_indent_level(self):
        enc = self.json.encoder.c_make_encoder(
            markers=None,
            default=str,
            encoder=self.json.encoder.c_encode_basestring,
            indent='\t',
            key_separator=': ',
            item_separator=', ',
            sort_keys=False,
            skipkeys=False,
            allow_nan=False)
        expected = (
            '[\n'
            '\t"spam", \n'
            '\t{\n'
            '\t\t"ham": "eggs"\n'
            '\t}\n'
            ']')
        self.assertEqual(enc(['spam', {'ham': 'eggs'}], 0)[0], expected)
        self.assertEqual(enc(['spam', {'ham': 'eggs'}], -3)[0], expected)
        expected2 = (
            '[\n'
            '\t\t\t\t"spam", \n'
            '\t\t\t\t{\n'
            '\t\t\t\t\t"ham": "eggs"\n'
            '\t\t\t\t}\n'
            '\t\t\t]')
        self.assertEqual(enc(['spam', {'ham': 'eggs'}], 3)[0], expected2)
        self.assertRaises(TypeError, enc, ['spam', {'ham': 'eggs'}], 3.0)
        self.assertRaises(TypeError, enc, ['spam', {'ham': 'eggs'}])
       
    def test_mutate_items_during_encode(self):
        c_make_encoder = getattr(self.json.encoder, 'c_make_encoder', None)
        if c_make_encoder is None:
            self.skipTest("c_make_encoder not available")

        cache = []

        class BadDict(dict):
            def items(self):
                entries = [("boom", object())]
                cache.append(entries)
                return entries

        def encode_str(obj):
            if cache:
                cache.pop().clear()
            return '"x"'

        encoder = c_make_encoder(
            None, lambda o: "null",
            encode_str, None,
            ": ", ", ", False,
            False, True
        )

        try:
            encoder(BadDict(real=1), 0)
        except (ValueError, RuntimeError):
            pass
