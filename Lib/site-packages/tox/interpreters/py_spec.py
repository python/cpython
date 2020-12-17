from __future__ import unicode_literals

import os
import re
import sys

import six

import tox


class PythonSpec(object):
    def __init__(self, name, major, minor, architecture, path, args=None):
        self.name = name
        self.major = major
        self.minor = minor
        self.architecture = architecture
        self.path = path
        self.args = args

    def __repr__(self):
        return (
            "{0.__class__.__name__}(name={0.name!r}, major={0.major!r}, minor={0.minor!r}, "
            "architecture={0.architecture!r}, path={0.path!r}, args={0.args!r})"
        ).format(self)

    def __str__(self):
        msg = repr(self)
        return msg.encode("utf-8") if six.PY2 else msg

    def satisfies(self, req):
        if req.is_abs and self.is_abs and self.path != req.path:
            return False
        if req.name is not None and req.name != self.name:
            return False
        if req.architecture is not None and req.architecture != self.architecture:
            return False
        if req.major is not None and req.major != self.major:
            return False
        if req.minor is not None and req.minor != self.minor:
            return False
        if req.major is None and req.minor is not None:
            return False
        return True

    @property
    def is_abs(self):
        return self.path is not None and os.path.isabs(self.path)

    @classmethod
    def from_name(cls, base_python):
        name, major, minor, architecture, path = None, None, None, None, None
        if os.path.isabs(base_python):
            path = base_python
        else:
            match = re.match(r"(python|pypy|jython)(\d)?(?:\.(\d+))?(?:-(32|64))?$", base_python)
            if match:
                groups = match.groups()
                name = groups[0]
                major = int(groups[1]) if len(groups) >= 2 and groups[1] is not None else None
                minor = int(groups[2]) if len(groups) >= 3 and groups[2] is not None else None
                architecture = (
                    int(groups[3]) if len(groups) >= 4 and groups[3] is not None else None
                )
            else:
                path = base_python
        return cls(name, major, minor, architecture, path)


CURRENT = PythonSpec(
    "pypy" if tox.constants.INFO.IS_PYPY else "python",
    sys.version_info[0],
    sys.version_info[1],
    64 if sys.maxsize > 2 ** 32 else 32,
    sys.executable,
)
