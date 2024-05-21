import unittest

from _pyrepl.keymap import parse_keys, compile_keymap


class TestParseKeys(unittest.TestCase):
    def test_single_character(self):
        self.assertEqual(parse_keys("a"), ["a"])
        self.assertEqual(parse_keys("b"), ["b"])
        self.assertEqual(parse_keys("1"), ["1"])

    def test_escape_sequences(self):
        self.assertEqual(parse_keys("\\n"), ["\n"])
        self.assertEqual(parse_keys("\\t"), ["\t"])
        self.assertEqual(parse_keys("\\\\"), ["\\"])
        self.assertEqual(parse_keys("\\'"), ["'"])
        self.assertEqual(parse_keys('\\"'), ['"'])

    def test_control_sequences(self):
        self.assertEqual(parse_keys("\\C-a"), ["\x01"])
        self.assertEqual(parse_keys("\\C-b"), ["\x02"])
        self.assertEqual(parse_keys("\\C-c"), ["\x03"])

    def test_meta_sequences(self):
        self.assertEqual(parse_keys("\\M-a"), ["\033", "a"])
        self.assertEqual(parse_keys("\\M-b"), ["\033", "b"])
        self.assertEqual(parse_keys("\\M-c"), ["\033", "c"])

    def test_keynames(self):
        self.assertEqual(parse_keys("\\<up>"), ["up"])
        self.assertEqual(parse_keys("\\<down>"), ["down"])
        self.assertEqual(parse_keys("\\<left>"), ["left"])
        self.assertEqual(parse_keys("\\<right>"), ["right"])

    def test_combinations(self):
        self.assertEqual(parse_keys("\\C-a\\n\\<up>"), ["\x01", "\n", "up"])
        self.assertEqual(parse_keys("\\M-a\\t\\<down>"), ["\033", "a", "\t", "down"])


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
