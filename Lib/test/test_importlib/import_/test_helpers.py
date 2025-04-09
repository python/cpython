"""Tests for helper functions used by import.c ."""

from importlib import _bootstrap_external, machinery
import os.path
from types import ModuleType, SimpleNamespace
import unittest
import warnings

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


class TestBlessMyLoader(unittest.TestCase):
    # GH#86298 is part of the migration away from module attributes and toward
    # __spec__ attributes.  There are several cases to test here.  This will
    # have to change in Python 3.14 when we actually remove/ignore __loader__
    # in favor of requiring __spec__.loader.

    def test_gh86298_no_loader_and_no_spec(self):
        bar = ModuleType('bar')
        del bar.__loader__
        del bar.__spec__
        # 2022-10-06(warsaw): For backward compatibility with the
        # implementation in _warnings.c, this can't raise an
        # AttributeError.  See _bless_my_loader() in _bootstrap_external.py
        # If working with a module:
        ## self.assertRaises(
        ##     AttributeError, _bootstrap_external._bless_my_loader,
        ##     bar.__dict__)
        self.assertIsNone(_bootstrap_external._bless_my_loader(bar.__dict__))

    def test_gh86298_loader_is_none_and_no_spec(self):
        bar = ModuleType('bar')
        bar.__loader__ = None
        del bar.__spec__
        # 2022-10-06(warsaw): For backward compatibility with the
        # implementation in _warnings.c, this can't raise an
        # AttributeError.  See _bless_my_loader() in _bootstrap_external.py
        # If working with a module:
        ## self.assertRaises(
        ##     AttributeError, _bootstrap_external._bless_my_loader,
        ##     bar.__dict__)
        self.assertIsNone(_bootstrap_external._bless_my_loader(bar.__dict__))

    def test_gh86298_no_loader_and_spec_is_none(self):
        bar = ModuleType('bar')
        del bar.__loader__
        bar.__spec__ = None
        self.assertRaises(
            ValueError,
            _bootstrap_external._bless_my_loader, bar.__dict__)

    def test_gh86298_loader_is_none_and_spec_is_none(self):
        bar = ModuleType('bar')
        bar.__loader__ = None
        bar.__spec__ = None
        self.assertRaises(
            ValueError,
            _bootstrap_external._bless_my_loader, bar.__dict__)

    def test_gh86298_loader_is_none_and_spec_loader_is_none(self):
        bar = ModuleType('bar')
        bar.__loader__ = None
        bar.__spec__ = SimpleNamespace(loader=None)
        self.assertRaises(
            ValueError,
            _bootstrap_external._bless_my_loader, bar.__dict__)

    def test_gh86298_no_spec(self):
        bar = ModuleType('bar')
        bar.__loader__ = object()
        del bar.__spec__
        with warnings.catch_warnings():
            self.assertWarns(
                DeprecationWarning,
                _bootstrap_external._bless_my_loader, bar.__dict__)

    def test_gh86298_spec_is_none(self):
        bar = ModuleType('bar')
        bar.__loader__ = object()
        bar.__spec__ = None
        with warnings.catch_warnings():
            self.assertWarns(
                DeprecationWarning,
                _bootstrap_external._bless_my_loader, bar.__dict__)

    def test_gh86298_no_spec_loader(self):
        bar = ModuleType('bar')
        bar.__loader__ = object()
        bar.__spec__ = SimpleNamespace()
        with warnings.catch_warnings():
            self.assertWarns(
                DeprecationWarning,
                _bootstrap_external._bless_my_loader, bar.__dict__)

    def test_gh86298_loader_and_spec_loader_disagree(self):
        bar = ModuleType('bar')
        bar.__loader__ = object()
        bar.__spec__ = SimpleNamespace(loader=object())
        with warnings.catch_warnings():
            self.assertWarns(
                DeprecationWarning,
                _bootstrap_external._bless_my_loader, bar.__dict__)

    def test_gh86298_no_loader_and_no_spec_loader(self):
        bar = ModuleType('bar')
        del bar.__loader__
        bar.__spec__ = SimpleNamespace()
        self.assertRaises(
            AttributeError,
            _bootstrap_external._bless_my_loader, bar.__dict__)

    def test_gh86298_no_loader_with_spec_loader_okay(self):
        bar = ModuleType('bar')
        del bar.__loader__
        loader = object()
        bar.__spec__ = SimpleNamespace(loader=loader)
        self.assertEqual(
            _bootstrap_external._bless_my_loader(bar.__dict__),
            loader)


if __name__ == "__main__":
    unittest.main()
