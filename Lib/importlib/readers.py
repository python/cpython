import zipfile
import pathlib
from . import abc


class FileReader(abc.TraversableResources):
    def __init__(self, loader):
        self.path = pathlib.Path(loader.path).parent

    def files(self):
        return self.path


class ZipReader(FileReader):
    def __init__(self, loader, module):
        _, _, name = module.rpartition('.')
        prefix = loader.prefix.replace('\\', '/') + name + '/'
        self.path = zipfile.Path(loader.archive, prefix)

    def open_resource(self, resource):
        try:
            return super().open_resource(resource)
        except KeyError as exc:
            raise FileNotFoundError(exc.args[0])

    def is_resource(self, path):
        # workaround for `zipfile.Path.is_file` returning true
        # for non-existent paths.
        target = self.files().joinpath(path)
        return target.is_file() and target.exists()
