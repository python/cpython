import sys
import unittest
from unittest import mock

from xml.dom import domreg


class BrokenModule:
    """Fake DOM implementation module whose factory is genuinely buggy."""

    @staticmethod
    def getDOMImplementation():
        raise RuntimeError("bug in implementation factory")


class MissingFactoryModule:
    """Fake DOM implementation module without a factory (AttributeError)."""


class GetDOMImplementationFallbackTests(unittest.TestCase):
    """The nameless getDOMImplementation() call tries each well-known
    implementation and skips the unavailable ones.
    """

    def _patch_implementations(self, module):
        self.enterContext(
            mock.patch.dict(sys.modules, {"test_fake_dom": module})
        )
        self.enterContext(
            mock.patch.dict(
                domreg.well_known_implementations,
                {"test_fake_dom": "test_fake_dom"},
                clear=True,
            )
        )
        self.enterContext(mock.patch.dict(domreg.registered, clear=True))

    def test_fallback_skips_unavailable_implementations(self):
        # A missing factory (AttributeError) means "not available": skipped,
        # and with no other implementation the lookup fails cleanly.
        self._patch_implementations(MissingFactoryModule)
        with self.assertRaises(ImportError):
            domreg.getDOMImplementation()

    def test_fallback_propagates_unexpected_errors(self):
        # gh-155944: a genuine bug in a candidate's factory must propagate,
        # not be silently treated as "implementation not available".
        self._patch_implementations(BrokenModule)
        with self.assertRaises(RuntimeError):
            domreg.getDOMImplementation()


if __name__ == "__main__":
    unittest.main()
