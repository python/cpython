"""Tests for helper functions used by import.c ."""

from importlib import _bootstrap_external, machinery
import os.path
import unittest

from .. import util


class FixUpModuleTests:

    def test_no_loader_but_spec(self):
        loader = object()
        name = "hello"
        path = "hello.py"
        spec = machinery.ModuleSpec(name, loader)
        ns = {"__spec__": spec}
        _bootstrap_external._fix_up_module(ns, name, path)

        expected = {"__spec__": spec, "__loader__": loader, "__file__": path,
                    "__cached__": None}
        self.assertEqual(ns, expected)

    def test_no_loader_no_spec_but_sourceless(self):
        name = "hello"
        path = "hello.py"
        ns = {}
        _bootstrap_external._fix_up_module(ns, name, path, path)

        expected = {"__file__": path, "__cached__": path}

        for key, val in expected.items():
            with self.subTest(f"{key}: {val}"):
                self.assertEqual(ns[key], val)

        spec = ns["__spec__"]
        self.assertIsInstance(spec, machinery.ModuleSpec)
        self.assertEqual(spec.name, name)
        self.assertEqual(spec.origin, os.path.abspath(path))
        self.assertEqual(spec.cached, os.path.abspath(path))
        self.assertIsInstance(spec.loader, machinery.SourcelessFileLoader)
        self.assertEqual(spec.loader.name, name)
        self.assertEqual(spec.loader.path, path)
        self.assertEqual(spec.loader, ns["__loader__"])

    def test_no_loader_no_spec_but_source(self):
        name = "hello"
        path = "hello.py"
        ns = {}
        _bootstrap_external._fix_up_module(ns, name, path)

        expected = {"__file__": path, "__cached__": None}

        for key, val in expected.items():
            with self.subTest(f"{key}: {val}"):
                self.assertEqual(ns[key], val)

        spec = ns["__spec__"]
        self.assertIsInstance(spec, machinery.ModuleSpec)
        self.assertEqual(spec.name, name)
        self.assertEqual(spec.origin, os.path.abspath(path))
        self.assertIsInstance(spec.loader, machinery.SourceFileLoader)
        self.assertEqual(spec.loader.name, name)
        self.assertEqual(spec.loader.path, path)
        self.assertEqual(spec.loader, ns["__loader__"])


FrozenFixUpModuleTests, SourceFixUpModuleTests = util.test_both(FixUpModuleTests)

if __name__ == "__main__":
    unittest.main()
