import codecs

from test.test_json import PyTest, CTest


class TestDetectEncoding:
    def test_utf32(self):
        # Arrange
        for test_input, expected_encoding in [
            (codecs.BOM_UTF32_BE + "abc".encode("utf-32-be"), "utf-32"),
            (codecs.BOM_UTF32_LE + "abc".encode("utf-32-le"), "utf-32"),
            (codecs.BOM_UTF16_BE + "abc".encode("utf-16-be"), "utf-16"),
            (codecs.BOM_UTF16_LE + "abc".encode("utf-16-le"), "utf-16"),
            (codecs.BOM_UTF8 + "abc".encode("utf-8-sig"), "utf-8-sig"),
            (b"\x00\x00\x00a\x00\x00\x00b\x00\x00\x00c", "utf-32-be"),
            (b"\x00a\x00b\x00c", "utf-16-be"),
            (b"a\x00\x00\x00b\x00\x00\x00c\x00\x00\x00", "utf-32-le"),
            (b"a\x00\x00b\x00c\x00", "utf-16-le"),
            (b"a\x00b\x00c\x00", "utf-16-le"),
            (b"\x00a", "utf-16-be"),
            (b"a\x00", "utf-16-le"),
            (b"abcd", "utf-8"),
            (b"abc", "utf-8"),
            (b"ab", "utf-8"),
        ]:
            # Act
            result = self.json.detect_encoding(test_input)

            # Assert
            self.assertEqual(result, expected_encoding)


class TestPyTestDetectEncoding(TestDetectEncoding, PyTest):
    pass


class TestCTestDetectEncoding(TestDetectEncoding, CTest):
    pass
