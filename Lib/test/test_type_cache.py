""" Tests for the internal type cache in CPython. """
import unittest
from collections import Counter
from test import support
from test.support import import_helper
try:
    from sys import _clear_type_cache
except ImportError:
    _clear_type_cache = None

# Skip this test if the _testcapi module isn't available.
type_get_version = import_helper.import_module('_testcapi').type_get_version

msg = "requires sys._clear_type_cache and sys._get_type_version_tag"
@support.cpython_only
@unittest.skipIf(_clear_type_cache is None, msg)
class TypeCacheTests(unittest.TestCase):
    def test_tp_version_tag_unique(self):
        """tp_version_tag should be unique assuming no overflow, even after
        clearing type cache.
        """
        # Check if global version tag has already overflowed.
        Y = type('Y', (), {})
        Y.x = 1
        Y.x  # Force a _PyType_Lookup, populating version tag
        y_ver = type_get_version(Y)
        # Overflow, or not enough left to conduct the test.
        if y_ver == 0 or y_ver > 0xFFFFF000:
            self.skipTest("Out of type version tags")
        # Note: try to avoid any method lookups within this loop,
        # It will affect global version tag.
        all_version_tags = []
        append_result = all_version_tags.append
        assertNotEqual = self.assertNotEqual
        for _ in range(30):
            _clear_type_cache()
            X = type('Y', (), {})
            X.x = 1
            X.x
            tp_version_tag_after = type_get_version(X)
            assertNotEqual(tp_version_tag_after, 0, msg="Version overflowed")
            append_result(tp_version_tag_after)
        tp_version_tag, count = Counter(all_version_tags).most_common(1)[0]
        self.assertEqual(count, 1,
                         msg=f"tp_version_tag {tp_version_tag} was not unique")


if __name__ == "__main__":
    support.run_unittest(TypeCacheTests)
