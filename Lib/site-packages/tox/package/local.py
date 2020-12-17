import os
import re

import packaging.version
import py

import tox
from tox import reporter
from tox.exception import MissingDependency

_SPEC_2_PACKAGE = {}


def resolve_package(package_spec):
    global _SPEC_2_PACKAGE
    try:
        return _SPEC_2_PACKAGE[package_spec]
    except KeyError:
        _SPEC_2_PACKAGE[package_spec] = x = get_latest_version_of_package(package_spec)
        return x


def get_latest_version_of_package(package_spec):
    if not os.path.isabs(str(package_spec)):
        return package_spec
    p = py.path.local(package_spec)
    if p.check():
        return p
    if not p.dirpath().check(dir=1):
        raise tox.exception.MissingDirectory(p.dirpath())
    reporter.info("determining {}".format(p))
    candidates = p.dirpath().listdir(p.basename)
    if len(candidates) == 0:
        raise MissingDependency(package_spec)
    if len(candidates) > 1:
        version_package = []
        for filename in candidates:
            version = get_version_from_filename(filename.basename)
            if version is not None:
                version_package.append((version, filename))
            else:
                reporter.warning("could not determine version of: {}".format(str(filename)))
        if not version_package:
            raise tox.exception.MissingDependency(package_spec)
        version_package.sort()
        _, package_with_largest_version = version_package[-1]
        return package_with_largest_version
    else:
        return candidates[0]


_REGEX_FILE_NAME_WITH_VERSION = re.compile(r"[\w_\-\+\.]+-(.*)\.(zip|tar\.gz)")


def get_version_from_filename(basename):
    m = _REGEX_FILE_NAME_WITH_VERSION.match(basename)
    if m is None:
        return None
    version = m.group(1)
    try:
        return packaging.version.Version(version)
    except packaging.version.InvalidVersion:
        return None
