from importlib import abc
from importlib import machinery
import unittest


class SubclassTests(unittest.TestCase):

    """Test that the various classes in importlib are subclasses of the
    expected ABCS."""

    def verify(self, ABC, *classes):
        """Verify the classes are subclasses of the ABC."""
        for cls in classes:
            self.assert_(issubclass(cls, ABC))

    def test_Finder(self):
        self.verify(abc.Finder, machinery.BuiltinImporter,
                    machinery.FrozenImporter, machinery.PathFinder)

    def test_Loader(self):
        self.verify(abc.Loader, machinery.BuiltinImporter,
                    machinery.FrozenImporter)


def test_main():
    from test.support import run_unittest
    run_unittest(SubclassTests)


if __name__ == '__main__':
    test_main()
