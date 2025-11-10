import unittest
from . import load_tests_for_base_classes

class TestModule:
    def test_deprecated__version__(self):
        with self.assertWarnsRegex(
            DeprecationWarning,
            "'__version__' is deprecated and slated for removal in Python 3.20",
        ) as cm:
            getattr(self.decimal, "__version__")
        self.assertEqual(cm.filename, __file__)


def load_tests(loader, tests, pattern):
    return load_tests_for_base_classes(loader, tests, [TestModule])


if __name__ == '__main__':
    unittest.main()
