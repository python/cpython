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

    def test_module_name(self):
        self.assertEqual(tomllib.TOMLDecodeError().__module__, tomllib.__name__)

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
