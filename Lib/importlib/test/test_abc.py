from importlib import abc
from importlib import machinery
import inspect
import unittest


class InheritanceTests:

    """Test that the specified class is a subclass/superclass of the expected
    classes."""

    subclasses = []
    superclasses = []

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        assert self.subclasses or self.superclasses, self.__class__
        self.__test = getattr(abc, self.__class__.__name__)

    def test_subclasses(self):
        # Test that the expected subclasses inherit.
        for subclass in self.subclasses:
            self.assertTrue(issubclass(subclass, self.__test),
                "{0} is not a subclass of {1}".format(subclass, self.__test))

    def test_superclasses(self):
        # Test that the class inherits from the expected superclasses.
        for superclass in self.superclasses:
            self.assertTrue(issubclass(self.__test, superclass),
               "{0} is not a superclass of {1}".format(superclass, self.__test))


class Finder(InheritanceTests, unittest.TestCase):

    subclasses = [machinery.BuiltinImporter, machinery.FrozenImporter,
                    machinery.PathFinder]


class Loader(InheritanceTests, unittest.TestCase):

    subclasses = [abc.PyLoader]


class ResourceLoader(InheritanceTests, unittest.TestCase):

    superclasses = [abc.Loader]


class InspectLoader(InheritanceTests, unittest.TestCase):

    superclasses = [abc.Loader]
    subclasses = [abc.PyLoader, machinery.BuiltinImporter,
                    machinery.FrozenImporter]


class ExecutionLoader(InheritanceTests, unittest.TestCase):

    superclasses = [abc.InspectLoader]
    subclasses = [abc.PyLoader]


class SourceLoader(InheritanceTests, unittest.TestCase):

    superclasses = [abc.ResourceLoader, abc.ExecutionLoader]


class PyLoader(InheritanceTests, unittest.TestCase):

    superclasses = [abc.Loader, abc.ResourceLoader, abc.ExecutionLoader]


class PyPycLoader(InheritanceTests, unittest.TestCase):

    superclasses = [abc.PyLoader]


def test_main():
    from test.support import run_unittest
    classes = []
    for class_ in globals().values():
        if (inspect.isclass(class_) and
                issubclass(class_, unittest.TestCase) and
                issubclass(class_, InheritanceTests)):
            classes.append(class_)
    run_unittest(*classes)


if __name__ == '__main__':
    test_main()
