import pathlib
from .abc import TraversableResources


class FileReader(TraversableResources):
    def __init__(self, loader):
        self.path = pathlib.Path(loader.path).parent

    def files(self):
        return self.path
