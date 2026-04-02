# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2021 Taneli Hukkinen
# Licensed to PSF under a Contributor Agreement.

import copy
import datetime
from decimal import Decimal as D
import importlib
from pathlib import Path
import sys
import tempfile
import textwrap
import unittest
from test import support
from test.support import os_helper
from test.support.script_helper import assert_python_ok

from . import tomllib


class TestMiscellaneous(unittest.TestCase):
    def test_load(self):
        content = "one=1 \n two='two' \n arr=[]"
        expected = {"one": 1, "two": "two", "arr": []}
        with tempfile.TemporaryDirectory() as tmp_dir_path:
            file_path = Path(tmp_dir_path) / "test.toml"
            file_path.write_text(content)

            with open(file_path, "rb") as bin_f:
                actual = tomllib.load(bin_f)
        self.assertEqual(actual, expected)

    def test_incorrect_load(self):
        content = "one=1"
        with tempfile.TemporaryDirectory() as tmp_dir_path:
            file_path = Path(tmp_dir_path) / "test.toml"
            file_path.write_text(content)

            with open(file_path, "r") as txt_f:
                with self.assertRaises(TypeError):
                    tomllib.load(txt_f)  # type: ignore[arg-type]

    def test_parse_float(self):
        doc = """
              val=0.1
              biggest1=inf
              biggest2=+inf
              smallest=-inf
              notnum1=nan
              notnum2=-nan
              notnum3=+nan
              """
        obj = tomllib.loads(doc, parse_float=D)
        expected = {
            "val": D("0.1"),
            "biggest1": D("inf"),
            "biggest2": D("inf"),
            "smallest": D("-inf"),
            "notnum1": D("nan"),
            "notnum2": D("-nan"),
            "notnum3": D("nan"),
        }
        for k, expected_val in expected.items():
            actual_val = obj[k]
            self.assertIsInstance(actual_val, D)
            if actual_val.is_nan():
                self.assertTrue(expected_val.is_nan())
            else:
                self.assertEqual(actual_val, expected_val)

    def test_deepcopy(self):
        doc = """
              [bliibaa.diibaa]
              offsettime=[1979-05-27T00:32:00.999999-07:00]
              """
        obj = tomllib.loads(doc)
        obj_copy = copy.deepcopy(obj)
        self.assertEqual(obj_copy, obj)
        expected_obj = {
            "bliibaa": {
                "diibaa": {
                    "offsettime": [
                        datetime.datetime(
                            1979,
                            5,
                            27,
                            0,
                            32,
                            0,
                            999999,
                            tzinfo=datetime.timezone(datetime.timedelta(hours=-7)),
                        )
                    ]
                }
            }
        }
        self.assertEqual(obj_copy, expected_obj)

    @support.skip_if_unlimited_stack_size
    def test_inline_array_recursion_limit(self):
        with support.infinite_recursion(max_depth=100):
            available = support.get_recursion_available()
            nest_count = (available // 2) - 2
            # Add details if the test fails
            with self.subTest(limit=sys.getrecursionlimit(),
                              available=available,
                              nest_count=nest_count):
                recursive_array_toml = "arr = " + nest_count * "[" + nest_count * "]"
                tomllib.loads(recursive_array_toml)

    @support.skip_if_unlimited_stack_size
    def test_inline_table_recursion_limit(self):
        with support.infinite_recursion(max_depth=100):
            available = support.get_recursion_available()
            nest_count = (available // 3) - 1
            # Add details if the test fails
            with self.subTest(limit=sys.getrecursionlimit(),
                              available=available,
                              nest_count=nest_count):
                recursive_table_toml = nest_count * "key = {" + nest_count * "}"
                tomllib.loads(recursive_table_toml)

    def test_types_import(self):
        """Test that `_types` module runs.

        The module is for type annotations only, so it is otherwise
        never imported by tests.
        """
        importlib.import_module(f"{tomllib.__name__}._types")

    def test_parse_simple_number(self):
        parse_simple_number = tomllib._parser._parse_simple_number
        self.assertEqual(parse_simple_number("123", 0), (3, 123))
        self.assertEqual(parse_simple_number("123\n", 0), (3, 123))
        self.assertEqual(parse_simple_number("0\n", 0), (1, 0))

        self.assertIsNone(parse_simple_number("0123\n", 0))
        self.assertIsNone(parse_simple_number("123-456\n", 0))
        self.assertIsNone(parse_simple_number("123:456\n", 0))
        self.assertIsNone(parse_simple_number("1.0\n", 0))
        self.assertIsNone(parse_simple_number("1_000\n", 0))
        self.assertIsNone(parse_simple_number("x123\n", 0))
        self.assertIsNone(parse_simple_number("o123\n", 0))
        self.assertIsNone(parse_simple_number("b100\n", 0))

    def test_lazy_import(self):
        # Test that _parse_simple_number() can parse the TOML file without
        # importing regular expressions (tomllib._re)
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)
        toml = textwrap.dedent("""
            [metadata]
            int = 123
            list = [+1, -2, 3]
            table = {x=1, y=2}
        """)
        with open(filename, "w") as fp:
            fp.write(toml)

        code = textwrap.dedent(f"""
            import sys, tomllib
            with open({filename!a}, "rb") as fp:
                tomllib.load(fp)
            print("lazy import?", 'tomllib._re' not in sys.modules)
        """)
        proc = assert_python_ok('-c', code)
        self.assertIn(b'lazy import? True', proc.out)
