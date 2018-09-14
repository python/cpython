#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Tests for `frozen_module` package."""
import os
import ctypes
import subprocess
import tempfile
import unittest
import random
import math
import inspect
import shutil

from frozen_module import frozen_module


def build_so(c_file_name):
    output_file_name = os.path.splitext(c_file_name)[0]
    so_file_name = "".join((output_file_name, ".so"))
    sp = subprocess.run("python3-config --includes --libs --cflags".split(" "),
                        stdout=subprocess.PIPE, check=True)
    assert sp.returncode == 0
    extra_args = sp.stdout.decode().strip().split(" ")
    extra_args = [item for subitem in extra_args
                  for item in subitem.split("\n")]
    compiler_args = ["cc", *extra_args,
                     "-shared", "-undefined", "dynamic_lookup",
                     "-o", so_file_name, c_file_name]
    subprocess.run(compiler_args, check=True)
    return so_file_name


CCODE_TEMPLATE = """#include <Python.h>
{fwd_declarations}

PyObject* getObject() {{
    intitialize_hashes(_ToHash);
    PyObject *obj = (PyObject*){obj_ref};
    Py_INCREF(obj);
    return obj;
}}"""


def compile_and_run_str(fwd_declarations, obj_ref):
    tmpdir = tempfile.gettempdir()
    rnd = random.randint(1, 10000)
    cfile_path = os.path.join(tmpdir,
                              f"test_{rnd}_ext.c")
    if os.path.isfile(cfile_path):
        os.unlink(cfile_path)
    with open(cfile_path, 'w') as f:
        f.write(CCODE_TEMPLATE.format(
            fwd_declarations=fwd_declarations,
            obj_ref=obj_ref))
    so_file_name = build_so(cfile_path)
    shutil.copyfile(so_file_name, '/tmp/test_ext.so')
    try:
        o = execute_shared_object(so_file_name)
    finally:
        os.unlink(so_file_name)
    return cfile_path, o


def execute_shared_object(so_file_name):
    lib = ctypes.cdll.LoadLibrary(so_file_name)
    getObject = lib.getObject
    getObject.argtypes = None
    getObject.restype = ctypes.py_object
    return getObject()


def test_fn(a, b, msg="Hello World"):
    print(msg)
    return a + b


class TestFrozen_module(unittest.TestCase):
    """Tests for `frozen_module` package."""
    def _serialize_and_run(self, o):
        fwd_declarations, obj_ref, = frozen_module.Serializer.serialize(o)
        return compile_and_run_str(fwd_declarations, obj_ref)

    def test_None(self):
        """Test serializing None objects."""
        fname, o = self._serialize_and_run(None)
        self.assertIs(o, None, f"{o} != None, check {fname}")

    def test_bool(self):
        """Test serializing bool objects."""
        fname, o = self._serialize_and_run(True)
        self.assertIs(o, True, f"{o} != True, check {fname}")
        fname, o = self._serialize_and_run(False)
        self.assertIs(o, False, f"{o} != False, check {fname}")

    def test_long(self):
        """Test serializing long objects."""
        fname, o = self._serialize_and_run(0)
        self.assertEqual(o, 0, f"{o} != 0, check {fname}")
        fname, o = self._serialize_and_run(1)
        self.assertEqual(o, 1, f"{o} != 0, check {fname}")
        for num in [-1000000000000,
                    +1000000000000]:
            fname, o = self._serialize_and_run(num)
            self.assertEqual(o, num, f"{o} != {num}, check {fname}")

    def test_float(self):
        """Test serializing floats."""
        fname, o = self._serialize_and_run(1.1)
        self.assertEqual(o, 1.1, f"{o} != 0, check {fname}")
        fname, o = self._serialize_and_run(math.nan)
        self.assertTrue(math.isnan(o), f"{o} is not nan, check {fname}")
        fname, o = self._serialize_and_run(float('-nan'))
        self.assertTrue(math.isnan(o), f"{o} is not nan, check {fname}")
        fname, o = self._serialize_and_run(math.inf)
        self.assertEqual(o, math.inf, f"{o} is not inf, check {fname}")
        fname, o = self._serialize_and_run(-math.inf)
        self.assertEqual(o, -math.inf, f"{o} is not -inf, check {fname}")

    def test_bytes(self):
        """Test serializing bytes."""
        fname, o = self._serialize_and_run(b'')
        self.assertEqual(o, b'', f"{o} != b'', check {fname}")
        fname, o = self._serialize_and_run(b' ')
        self.assertEqual(o, b' ', f"{o} != b'', check {fname}")
        fname, o = self._serialize_and_run(b'\xff')
        self.assertEqual(o, b'\xff', f"{o} != b'', check {fname}")
        fname, o = self._serialize_and_run(b'\x00')
        self.assertEqual(o, b'\x00', f"{o} != b'', check {fname}")
        fname, o = self._serialize_and_run(b'\xca\xfe\xba\xbe')
        self.assertEqual(o, b'\xca\xfe\xba\xbe', f"{o} != b'\xca\xfe\xba\xbe',"
                                                 f" check {fname}")

    def test_tuple(self):
        """Test serializing tuples."""
        t = (1, 2, 3, (4, 5, (4, 5, 6, 7)), None, True, False, 1.1, (), (1,))
        fname, o = self._serialize_and_run(t)
        self.assertEqual(o, t, f"{o} != {t}, check {fname}")

    def test_code(self):
        """Test serializing code."""
        fname, o = self._serialize_and_run(test_fn.__code__)
        self.assertTrue(inspect.iscode(o))

    def test_ellipsis(self):
        """Test serializing the Ellipsis object."""
        fname, o = self._serialize_and_run(Ellipsis)
        self.assertIs(o, Ellipsis, f"{o} is not Ellipsis, check {fname}")

    def test_complex(self):
        """Test serializing the complex object."""
        for x in [complex(), complex('1+2j')]:
            fname, o = self._serialize_and_run(x)
            self.assertEqual(o, x, f"{o} != {x}, check {fname}")

    def test_str(self):
        """Test serializing strs."""
        for s in ['', 'hello world', '\xfe', '∂∆', 'देवनागरी', '\U00010000']:
            fname, o = self._serialize_and_run(s)
            self.assertEqual(o, s, f"{o} != {s!r}, check {fname}")

    def test_bool_int_confusion(self):
        """
        The integer 0 and False hash to the same value.
        Test to ensure that this is handled correctly
        """
        t = (0, False)
        fname, o = self._serialize_and_run(t)
        self.assertEqual(o, t, f"{o} != {t!r}, check {fname}")
        self.assertIs(type(o[0]), int)
        self.assertIs(type(o[1]), bool)

    def test_frozenset(self):
        """Test serializing frozensets."""
        for x in range(10):
            value = frozenset(range(x))
            fname, o = self._serialize_and_run(value)
            self.assertIsInstance(o, frozenset)
            self.assertEqual(o, value)
            self.assertEqual(hash(o), hash(value))
        a = frozenset({'Foo Bar', None})
        b = frozenset({None, 'Hello World', None})
        c = frozenset({'a', 'b', 'c', 'd',
                       'e', 'f', 'g', 'h', None})
        value = frozenset((a, b, c))
        fname, o = self._serialize_and_run(value)
        self.assertIsInstance(o, frozenset)
        self.assertEqual(o, value)
        self.assertEqual(hash(o), hash(value))

    def test_can_statically_hash(self):
        test_vec = [
            ((lambda x: x).__code__, False),
            (1, True),
            (1.1, True),
            (None, True),
            (True, True),
            (False, True),
            (set([1, 2, 3]), False),
            (frozenset([1, 2, 3]), True),
            (frozenset([1, 2, 3, frozenset({''})]), False),
            ('', False),
            (b'', False),
            ([], False),
        ]
        for candidate, expected in test_vec:
            self.assertEqual(
                frozen_module.Serializer.can_statically_hash(candidate),
                expected,
                f"can_statically_hash({candidate!r}) returned: "
                f"{(not expected)!r} expected: {expected!r}")
