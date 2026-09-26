"""Tests for xml.dom.domreg.getDOMImplementation fallback discovery (gh-155944)."""
import sys
import types
import unittest

import xml.dom.domreg as domreg


class GetDOMImplementationTest(unittest.TestCase):
    def setUp(self):
        self.saved_well_known = domreg.well_known_implementations
        self.saved_registered = domreg.registered

    def tearDown(self):
        domreg.well_known_implementations = self.saved_well_known
        domreg.registered = self.saved_registered

    def test_missing_module_is_skipped(self):
        domreg.well_known_implementations = {
            "missing": "no.such.dom.module.xyz"
        }
        domreg.registered = {}
        with self.assertRaises(ImportError):
            domreg.getDOMImplementation()

    def test_missing_factory_is_skipped(self):
        mod = types.ModuleType("fake_no_factory_dom")
        sys.modules["fake_no_factory_dom"] = mod
        self.addCleanup(sys.modules.pop, "fake_no_factory_dom", None)
        domreg.well_known_implementations = {
            "nofactory": "fake_no_factory_dom"
        }
        domreg.registered = {}
        with self.assertRaises(ImportError):
            domreg.getDOMImplementation()

    def test_buggy_candidate_does_not_block_a_later_one(self):
        mod = types.ModuleType("fake_broken_dom_then_good")

        def getDOMImplementation():
            raise RuntimeError("boom")

        mod.getDOMImplementation = getDOMImplementation
        sys.modules["fake_broken_dom_then_good"] = mod
        self.addCleanup(sys.modules.pop, "fake_broken_dom_then_good", None)
        domreg.well_known_implementations = {
            "broken": "fake_broken_dom_then_good",
            "minidom": "xml.dom.minidom",
        }
        domreg.registered = {}
        dom = domreg.getDOMImplementation()
        self.assertTrue(domreg._good_enough(dom, ()))

    def test_genuine_factory_bug_propagates(self):
        mod = types.ModuleType("fake_broken_dom")

        def getDOMImplementation():
            raise RuntimeError("boom")

        mod.getDOMImplementation = getDOMImplementation
        sys.modules["fake_broken_dom"] = mod
        self.addCleanup(sys.modules.pop, "fake_broken_dom", None)
        domreg.well_known_implementations = {"broken": "fake_broken_dom"}
        domreg.registered = {}
        with self.assertRaises(RuntimeError):
            domreg.getDOMImplementation()

    def test_default_discovery_still_works(self):
        dom = domreg.getDOMImplementation()
        self.assertTrue(domreg._good_enough(dom, ()))


if __name__ == "__main__":
    unittest.main()
