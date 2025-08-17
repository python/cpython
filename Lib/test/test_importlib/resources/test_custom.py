import unittest
import contextlib
import pathlib

from test.support import os_helper

from importlib import resources
from importlib.resources import abc
from importlib.resources.abc import TraversableResources, ResourceReader
from . import util


class SimpleLoader:
    """
    A simple loader that only implements a resource reader.
    """

    def __init__(self, reader: ResourceReader):
        self.reader = reader

    def get_resource_reader(self, package):
        return self.reader


class MagicResources(TraversableResources):
    """
    Magically returns the resources at path.
    """

    def __init__(self, path: pathlib.Path):
        self.path = path

    def files(self):
        return self.path


class CustomTraversableResourcesTests(unittest.TestCase):
    def setUp(self):
        self.fixtures = contextlib.ExitStack()
        self.addCleanup(self.fixtures.close)

    def test_custom_loader(self):
        temp_dir = pathlib.Path(self.fixtures.enter_context(os_helper.temp_dir()))
        loader = SimpleLoader(MagicResources(temp_dir))
        pkg = util.create_package_from_loader(loader)
        files = resources.files(pkg)
        assert isinstance(files, abc.Traversable)
        assert list(files.iterdir()) == []
