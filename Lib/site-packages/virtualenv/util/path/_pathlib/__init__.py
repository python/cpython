from __future__ import absolute_import, unicode_literals

import sys

import six

if six.PY3:
    from pathlib import Path

    if sys.version_info[0:2] == (3, 4):
        # no read/write text on python3.4
        BuiltinPath = Path

        class Path(type(BuiltinPath())):
            def read_text(self, encoding=None, errors=None):
                """
                Open the file in text mode, read it, and close the file.
                """
                with self.open(mode="r", encoding=encoding, errors=errors) as f:
                    return f.read()

            def read_bytes(self):
                """
                Open the file in bytes mode, read it, and close the file.
                """
                with self.open(mode="rb") as f:
                    return f.read()

            def write_text(self, data, encoding=None, errors=None):
                """
                Open the file in text mode, write to it, and close the file.
                """
                if not isinstance(data, str):
                    raise TypeError("data must be str, not %s" % data.__class__.__name__)
                with self.open(mode="w", encoding=encoding, errors=errors) as f:
                    return f.write(data)

            def write_bytes(self, data):
                """
                Open the file in bytes mode, write to it, and close the file.
                """
                # type-check for the buffer interface before truncating the file
                view = memoryview(data)
                with self.open(mode="wb") as f:
                    return f.write(view)

            def mkdir(self, mode=0o777, parents=False, exist_ok=False):
                try:
                    super(type(BuiltinPath()), self).mkdir(mode, parents)
                except FileExistsError as exception:
                    if not exist_ok:
                        raise exception


else:
    if sys.platform == "win32":
        # workaround for https://github.com/mcmtroffaes/pathlib2/issues/56
        from .via_os_path import Path
    else:
        from pathlib2 import Path


__all__ = ("Path",)
