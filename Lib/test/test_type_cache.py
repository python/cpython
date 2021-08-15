""" Tests for the internal type cache in CPython. """
import unittest
from collections import Counter
from test import support
try:
    from sys import _clear_type_cache, _get_type_version_tag
except ImportError:
    _clear_type_cache = None

msg = "requires sys._clear_type_cache and sys._get_type_version_tag"
@unittest.skipIf(_clear_type_cache is None, msg)
@support.cpython_only
class TypeCacheTests(unittest.TestCase):
    def test_tp_version_tag_unique(self):
        """tp_version_tag should be unique assuming no overflow, even after
        clearing type cache.
        """
        all_version_tags = []
        append_result = all_version_tags.append
        # Note: try to avoid any method lookups within this loop,
        # It will affect global version tag.
        for _ in range(30):
            _clear_type_cache()
            class C:
                # __init__ is only here to force
                # C.tp_version_tag to update when initializing
                # object C().
                def __init__(self):
                    pass
            x = C()
            tp_version_tag_after = _get_type_version_tag(C)
            # Try to avoid self.assertNotEqual to not overflow type cache.
            assert tp_version_tag_after != 0
            append_result(tp_version_tag_after)
        tp_version_tag, count = Counter(all_version_tags).most_common(1)[0]
        self.assertEqual(count, 1,
                         msg=f"tp_version_tag {tp_version_tag} was not unique")


if __name__ == "__main__":
    support.run_unittest(TypeCacheTests)
