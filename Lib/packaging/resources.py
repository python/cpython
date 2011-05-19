"""Data file path abstraction.

Functions in this module use sysconfig to find the paths to the resource
files registered in project's setup.cfg file.  See the documentation for
more information.
"""
# TODO write that documentation

from packaging.database import get_distribution

__all__ = ['get_file_path', 'get_file']


def get_file_path(distribution_name, relative_path):
    """Return the path to a resource file."""
    dist = get_distribution(distribution_name)
    if dist != None:
        return dist.get_resource_path(relative_path)
    raise LookupError('no distribution named %r found' % distribution_name)


def get_file(distribution_name, relative_path, *args, **kwargs):
    """Open and return a resource file."""
    return open(get_file_path(distribution_name, relative_path),
                *args, **kwargs)
