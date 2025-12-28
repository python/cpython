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

    def test_indent_argument_to_encoder(self):
        # gh-143196: indent must be str, int, or None
        # int is converted to spaces
        enc = self.json.encoder.c_make_encoder(
            None, lambda obj: obj, lambda obj: obj,
            4, ':', ', ', False, False, False,
        )
        result = enc({'a': 1}, 0)
        self.assertIn('    ', result[0])  # 4 spaces

        # Negative int should raise ValueError
        with self.assertRaisesRegex(
            ValueError,
            r'make_encoder\(\) argument 4 must be a non-negative int',
        ):
            self.json.encoder.c_make_encoder(None, None, None, -1, ': ', ', ',
                                             False, False, False)

        # Other types should raise TypeError
        with self.assertRaisesRegex(
            TypeError,
            r'make_encoder\(\) argument 4 must be str, int, or None, not list',
        ):
            self.json.encoder.c_make_encoder(None, None, None, [' '],
                                             ': ', ', ', False, False, False)

    def test_nonzero_indent_level_with_indent(self):
        # gh-143196: _current_indent_level must be 0 when indent is set
        # This prevents heap-buffer-overflow from uninitialized cache access
        # and also prevents re-entrant __mul__ attacks since PySequence_Repeat
        # is only called when indent_level != 0
        enc = self.json.encoder.c_make_encoder(
            None, lambda obj: obj, lambda obj: obj,
            '  ', ':', ', ', False, False, False,
        )
        # indent_level=0 should work
        enc([None], 0)
        # indent_level!=0 should raise ValueError
        with self.assertRaisesRegex(
            ValueError,
            r'_current_indent_level must be 0 when indent is set',
        ):
            enc([None], 1)

        # Verify that str subclasses with custom __mul__ are safe because
        # __mul__ is never called when indent_level=0
        class CustomIndent(str):
            def __mul__(self, count):
                raise RuntimeError("__mul__ should not be called")

        enc2 = self.json.encoder.c_make_encoder(
            None, lambda obj: obj, lambda obj: obj,
            CustomIndent('  '), ':', ', ', False, False, False,
        )
        # This should work - __mul__ is not called when indent_level=0
        enc2({'a': 1}, 0)
