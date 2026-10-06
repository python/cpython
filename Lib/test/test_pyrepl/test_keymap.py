import string
import unittest

from _pyrepl.keymap import _keynames, _escapes, parse_keys, compile_keymap, KeySpecError


class TestParseKeys(unittest.TestCase):
    def test_single_character(self):
        """Ensure that single ascii characters or single digits are parsed as single characters."""
        test_cases = [(key, [key]) for key in string.ascii_letters + string.digits]
        for test_key, expected_keys in test_cases:
            with self.subTest(f"{test_key} should be parsed as {expected_keys}"):
                self.assertEqual(parse_keys(test_key), expected_keys)

    def test_keynames(self):
        """Ensure that keynames are parsed to their corresponding mapping.

        A keyname is expected to be of the following form: \\<keyname> such as \\<left>
        which would get parsed as "left".
        """
        test_cases = [(f"\\<{keyname}>", [parsed_keyname]) for keyname, parsed_keyname in _keynames.items()]
        for test_key, expected_keys in test_cases:
            with self.subTest(f"{test_key} should be parsed as {expected_keys}"):
                self.assertEqual(parse_keys(test_key), expected_keys)

    def test_escape_sequences(self):
        """Ensure that escaping sequences are parsed to their corresponding mapping."""
        test_cases = [(f"\\{escape}", [parsed_escape]) for escape, parsed_escape in _escapes.items()]
        for test_key, expected_keys in test_cases:
            with self.subTest(f"{test_key} should be parsed as {expected_keys}"):
                self.assertEqual(parse_keys(test_key), expected_keys)

    def test_control_sequences(self):
        """Ensure that supported control sequences are parsed successfully."""
        keys = ["@", "[", "]", "\\", "^", "_", "\\<space>", "\\<delete>"]
        keys.extend(string.ascii_letters)
        test_cases = [(f"\\C-{key}", chr(ord(key) & 0x1F)) for key in []]
        for test_key, expected_keys in test_cases:
            with self.subTest(f"{test_key} should be parsed as {expected_keys}"):
                self.assertEqual(parse_keys(test_key), expected_keys)

    def test_meta_sequences(self):
        self.assertEqual(parse_keys("\\M-a"), ["\033", "a"])
        self.assertEqual(parse_keys("\\M-b"), ["\033", "b"])
        self.assertEqual(parse_keys("\\M-c"), ["\033", "c"])

    def test_combinations(self):
        self.assertEqual(parse_keys("\\C-a\\n\\<up>"), ["\x01", "\n", "up"])
        self.assertEqual(parse_keys("\\M-a\\t\\<down>"), ["\033", "a", "\t", "down"])

    def test_keyspec_errors(self):
        cases = [
            ("\\Ca", "\\C must be followed by `-'"),
            ("\\ca", "\\C must be followed by `-'"),
            ("\\C-\\C-", "doubled \\C-"),
            ("\\Ma", "\\M must be followed by `-'"),
            ("\\ma", "\\M must be followed by `-'"),
            ("\\M-\\M-", "doubled \\M-"),
            ("\\<left", "unterminated \\<"),
            ("\\<unsupported>", "unrecognised keyname"),
            ("\\大", "unknown backslash escape"),
            ("\\C-\\<backspace>", "\\C- followed by invalid key")
        ]
        for test_keys, expected_err in cases:
            with self.subTest(f"{test_keys} should give error {expected_err}"):
                with self.assertRaises(KeySpecError) as e:
                    parse_keys(test_keys)
                self.assertIn(expected_err, str(e.exception))

    def test_index_errors(self):
        test_cases = ["\\", "\\C", "\\C-\\C"]
        for test_keys in test_cases:
            with self.assertRaises(IndexError):
                parse_keys(test_keys)


class TestCompileKeymap(unittest.TestCase):
    def test_empty_keymap(self):
        keymap = {}
        result = compile_keymap(keymap)
        self.assertEqual(result, {})

    def test_single_keymap(self):
        keymap = {b"a": "action"}
        result = compile_keymap(keymap)
        self.assertEqual(result, {b"a": "action"})

    def test_nested_keymap(self):
        keymap = {b"a": {b"b": "action"}}
        result = compile_keymap(keymap)
        self.assertEqual(result, {b"a": {b"b": "action"}})

    def test_empty_value(self):
        keymap = {b"a": {b"": "action"}}
        result = compile_keymap(keymap)
        self.assertEqual(result, {b"a": {b"": "action"}})

    def test_multiple_empty_values(self):
        keymap = {b"a": {b"": "action1", b"b": "action2"}}
        result = compile_keymap(keymap)
        self.assertEqual(result, {b"a": {b"": "action1", b"b": "action2"}})

    def test_multiple_keymaps(self):
        keymap = {b"a": {b"b": "action1", b"c": "action2"}}
        result = compile_keymap(keymap)
        self.assertEqual(result, {b"a": {b"b": "action1", b"c": "action2"}})

    def test_nested_multiple_keymaps(self):
        keymap = {b"a": {b"b": {b"c": "action"}}}
        result = compile_keymap(keymap)
        self.assertEqual(result, {b"a": {b"b": {b"c": "action"}}})

    def test_clashing_definitions(self):
        km = {b'a': 'c', b'a' + b'b': 'd'}
        with self.assertRaises(KeySpecError):
            compile_keymap(km)

    def test_non_bytes_key(self):
        with self.assertRaises(TypeError):
            compile_keymap({123: 'a'})
