"""Tests for the annotations module."""

import textwrap
import annotationlib
import builtins
import collections
import functools
import itertools
import pickle
from string.templatelib import Template, Interpolation
import types
import typing
import sys
import unittest
from annotationlib import (
    Format,
    ForwardRef,
    get_annotations,
    annotations_to_string,
    type_repr,
)
from typing import Unpack, get_type_hints, List, Union

from test import support
from test.support import import_helper
from test.test_inspect import inspect_stock_annotations
from test.test_inspect import inspect_stringized_annotations
from test.test_inspect import inspect_stringized_annotations_2
from test.test_inspect import inspect_stringized_annotations_pep695


def times_three(fn):
    @functools.wraps(fn)
    def wrapper(a, b):
        return fn(a * 3, b * 3)

    return wrapper


class MyClass:
    def __repr__(self):
        return "my repr"


class TestFormat(unittest.TestCase):
    def test_enum(self):
        self.assertEqual(Format.VALUE.value, 1)
        self.assertEqual(Format.VALUE, 1)

        self.assertEqual(Format.VALUE_WITH_FAKE_GLOBALS.value, 2)
        self.assertEqual(Format.VALUE_WITH_FAKE_GLOBALS, 2)

        self.assertEqual(Format.FORWARDREF.value, 3)
        self.assertEqual(Format.FORWARDREF, 3)

        self.assertEqual(Format.STRING.value, 4)
        self.assertEqual(Format.STRING, 4)
