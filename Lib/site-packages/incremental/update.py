# Copyright (c) Twisted Matrix Laboratories.
# See LICENSE for details.

from __future__ import absolute_import, division, print_function

import click
import os
import datetime

from incremental import Version
from twisted.python.filepath import FilePath

_VERSIONPY_TEMPLATE = '''"""
Provides %s version information.
"""

# This file is auto-generated! Do not edit!
# Use `python -m incremental.update %s` to change this file.

from incremental import Version

__version__ = %s
__all__ = ["__version__"]
'''

_YEAR_START = 2000


def _findPath(path, package):

    cwd = FilePath(path)

    src_dir = cwd.child("src").child(package.lower())
    current_dir = cwd.child(package.lower())

    if src_dir.isdir():
        return src_dir
    elif current_dir.isdir():
        return current_dir
    else:
        raise ValueError(("Can't find under `./src` or `./`. Check the "
                          "package name is right (note that we expect your "
                          "package name to be lower cased), or pass it using "
                          "'--path'."))


def _existing_version(path):
    version_info = {}

    with path.child("_version.py").open('r') as f:
        exec(f.read(), version_info)

    return version_info["__version__"]


def _run(package, path, newversion, patch, rc, dev, create,
         _date=None, _getcwd=None, _print=print):

    if not _getcwd:
        _getcwd = os.getcwd

    if not _date:
        _date = datetime.date.today()

    if type(package) != str:
        package = package.encode('utf8')

    if not path:
        path = _findPath(_getcwd(), package)
    else:
        path = FilePath(path)

    if newversion and patch or newversion and dev or newversion and rc:
        raise ValueError("Only give --newversion")

    if dev and patch or dev and rc:
        raise ValueError("Only give --dev")

    if create and dev or create and patch or create and rc or \
       create and newversion:
        raise ValueError("Only give --create")

    if newversion:
        from pkg_resources import parse_version
        existing = _existing_version(path)
        st_version = parse_version(newversion)._version

        release = list(st_version.release)

        if len(release) == 1:
            release.append(0)
        if len(release) == 2:
            release.append(0)

        v = Version(
            package, *release,
            release_candidate=st_version.pre[1] if st_version.pre else None,
            dev=st_version.dev[1] if st_version.dev else None)

    elif create:
        v = Version(package, _date.year - _YEAR_START, _date.month, 0)
        existing = v

    elif rc and not patch:
        existing = _existing_version(path)

        if existing.release_candidate:
            v = Version(package, existing.major, existing.minor,
                        existing.micro, existing.release_candidate + 1)
        else:
            v = Version(package, _date.year - _YEAR_START, _date.month, 0, 1)

    elif patch:
        if rc:
            rc = 1
        else:
            rc = None

        existing = _existing_version(path)
        v = Version(package, existing.major, existing.minor,
                    existing.micro + 1, rc)

    elif dev:
        existing = _existing_version(path)

        if existing.dev is None:
            _dev = 0
        else:
            _dev = existing.dev + 1

        v = Version(package, existing.major, existing.minor,
                    existing.micro, existing.release_candidate, dev=_dev)

    else:
        existing = _existing_version(path)

        if existing.release_candidate:
            v = Version(package,
                        existing.major, existing.minor, existing.micro)
        else:
            raise ValueError(
                "You need to issue a rc before updating the major/minor")

    NEXT_repr = repr(Version(package, "NEXT", 0, 0)).split("#")[0]
    NEXT_repr_bytes = NEXT_repr.encode('utf8')

    version_repr = repr(v).split("#")[0]
    version_repr_bytes = version_repr.encode('utf8')

    existing_version_repr = repr(existing).split("#")[0]
    existing_version_repr_bytes = existing_version_repr.encode('utf8')

    _print("Updating codebase to %s" % (v.public()))

    for x in path.walk():

        if not x.isfile():
            continue

        original_content = x.getContent()
        content = original_content

        # Replace previous release_candidate calls to the new one
        if existing.release_candidate:
            content = content.replace(existing_version_repr_bytes,
                                      version_repr_bytes)
            content = content.replace(
                (package.encode('utf8') + b" " +
                 existing.public().encode('utf8')),
                (package.encode('utf8') + b" " +
                 v.public().encode('utf8')))

        # Replace NEXT Version calls with the new one
        content = content.replace(NEXT_repr_bytes,
                                  version_repr_bytes)
        content = content.replace(NEXT_repr_bytes.replace(b"'", b'"'),
                                  version_repr_bytes)

        # Replace <package> NEXT with <package> <public>
        content = content.replace(package.encode('utf8') + b" NEXT",
                                  (package.encode('utf8') + b" " +
                                   v.public().encode('utf8')))

        if content != original_content:
            _print("Updating %s" % (x.path,))
            with x.open('w') as f:
                f.write(content)

    _print("Updating %s/_version.py" % (path.path))
    with path.child("_version.py").open('w') as f:
        f.write(
            (_VERSIONPY_TEMPLATE % (
                package, package, version_repr)).encode('utf8'))


@click.command()
@click.argument('package')
@click.option('--path', default=None)
@click.option('--newversion', default=None)
@click.option('--patch', is_flag=True)
@click.option('--rc', is_flag=True)
@click.option('--dev', is_flag=True)
@click.option('--create', is_flag=True)
def run(*args, **kwargs):
    return _run(*args, **kwargs)


if __name__ == '__main__':  # pragma: no cover
    run()
