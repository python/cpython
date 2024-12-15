# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021 Taneli Hukkinen
# Licensed to PSF under a Contributor Agreement.

import unittest

from . import tomllib


class TestError(unittest.TestCase):
    def test_line_and_col(self):
        with self.assertRaises(tomllib.TOMLDecodeError) as exc_info:
            tomllib.loads("val=.")
        self.assertEqual(str(exc_info.exception), "Invalid value (at line 1, column 5)")

        with self.assertRaises(tomllib.TOMLDecodeError) as exc_info:
            tomllib.loads(".")
        self.assertEqual(
            str(exc_info.exception), "Invalid statement (at line 1, column 1)"
        )

        with self.assertRaises(tomllib.TOMLDecodeError) as exc_info:
            tomllib.loads("\n\nval=.")
        self.assertEqual(str(exc_info.exception), "Invalid value (at line 3, column 5)")

        with self.assertRaises(tomllib.TOMLDecodeError) as exc_info:
            tomllib.loads("\n\n.")
        self.assertEqual(
            str(exc_info.exception), "Invalid statement (at line 3, column 1)"
        )

    def test_missing_value(self):
        with self.assertRaises(tomllib.TOMLDecodeError) as exc_info:
            tomllib.loads("\n\nfwfw=")
        self.assertEqual(str(exc_info.exception), "Invalid value (at end of document)")

    def test_invalid_char_quotes(self):
        with self.assertRaises(tomllib.TOMLDecodeError) as exc_info:
            tomllib.loads("v = '\n'")
        self.assertTrue(" '\\n' " in str(exc_info.exception))

    def test_type_error(self):
        with self.assertRaises(TypeError) as exc_info:
            tomllib.loads(b"v = 1")  # type: ignore[arg-type]
        self.assertEqual(str(exc_info.exception), "Expected str object, not 'bytes'")

        with self.assertRaises(TypeError) as exc_info:
            tomllib.loads(False)  # type: ignore[arg-type]
        self.assertEqual(str(exc_info.exception), "Expected str object, not 'bool'")

    def test_module_name(self):
        self.assertEqual(
            tomllib.TOMLDecodeError("", "", 0).__module__, tomllib.__name__
        )

    def test_invalid_parse_float(self):
        def dict_returner(s: str) -> dict:
            return {}

        def list_returner(s: str) -> list:
            return []

        for invalid_parse_float in (dict_returner, list_returner):
            with self.assertRaises(ValueError) as exc_info:
                tomllib.loads("f=0.1", parse_float=invalid_parse_float)
            self.assertEqual(
                str(exc_info.exception), "parse_float must not return dicts or lists"
            )

    def test_deprecated_tomldecodeerror(self):
        for args in [
            (),
            ("err msg",),
            (None,),
            (None, "doc"),
            ("err msg", None),
            (None, "doc", None),
            ("err msg", "doc", None),
            ("one", "two", "three", "four"),
            ("one", "two", 3, "four", "five"),
        ]:
            with self.assertWarns(DeprecationWarning):
                e = tomllib.TOMLDecodeError(*args)  # type: ignore[arg-type]
            self.assertEqual(e.args, args)

    def test_tomldecodeerror(self):
        msg = "error parsing"
        doc = "v=1\n[table]\nv='val'"
        pos = 13
        formatted_msg = "error parsing (at line 3, column 2)"
        e = tomllib.TOMLDecodeError(msg, doc, pos)
        self.assertEqual(e.args, (formatted_msg,))
        self.assertEqual(str(e), formatted_msg)
        self.assertEqual(e.msg, msg)
        self.assertEqual(e.doc, doc)
        self.assertEqual(e.pos, pos)
        self.assertEqual(e.lineno, 3)
        self.assertEqual(e.colno, 2)
