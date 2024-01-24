import os
import sqlite3
import sys
from pathlib import Path
from contextlib import suppress, closing
from collections.abc import MutableMapping

BUILD_TABLE = """
  CREATE TABLE IF NOT EXISTS Dict (
    key TEXT UNIQUE NOT NULL,
    value BLOB NOT NULL
  )
"""
GET_SIZE = "SELECT COUNT (key) FROM Dict"
LOOKUP_KEY = "SELECT value FROM Dict WHERE key = ?"
STORE_KV = "REPLACE INTO Dict (key, value) VALUES (?, ?)"
DELETE_KEY = "DELETE FROM Dict WHERE key = ?"
ITER_KEYS = "SELECT key FROM Dict"


class error(OSError):
    pass


_ERR_CLOSED = "DBM object has already been closed"
_ERR_REINIT = "DBM object does not support reinitialization"


def _normalize_uri_path(path):
    path = os.fsdecode(path)
    for char in "%", "?", "#":
        path = path.replace(char, f"%{ord(char):02x}")
    path = Path(path)
    if sys.platform == "win32":
        if path.drive:
            if not path.is_absolute():
                path = Path(path).absolute()
            return "/" + Path(path).as_posix()
    return path.as_posix()


class _Database(MutableMapping):

    def __init__(self, path, /, flag):
        if hasattr(self, "_cx"):
            raise error(_ERR_REINIT)

        match flag:
            case "r":
                flag = "ro"
            case "w":
                flag = "rw"
            case "c":
                flag = "rwc"
            case "n":
                flag = "rwc"
                try:
                    os.remove(path)
                except FileNotFoundError:
                    pass
            case _:
                raise ValueError("Flag must be one of 'r', 'w', 'c', or 'n', "
                                 f"not {flag!r}")

        # We use the URI format when opening the database.
        path = _normalize_uri_path(path)
        uri = f"file:{path}?mode={flag}"

        try:
            self._cx = sqlite3.connect(uri, autocommit=True, uri=True)
        except sqlite3.Error as exc:
            raise error(str(exc))

        # This is an optimization only; it's ok if it fails.
        with suppress(sqlite3.OperationalError):
            self._cx.execute("PRAGMA journal_mode = wal")

        if flag == "rwc":
            with closing(self._execute(BUILD_TABLE)):
                pass

    def _execute(self, *args, **kwargs):
        if not self._cx:
            raise error(_ERR_CLOSED)
        try:
            ret = self._cx.execute(*args, **kwargs)
        except sqlite3.Error as exc:
            raise error(str(exc))
        else:
            return ret

    def __len__(self):
        with closing(self._execute(GET_SIZE)) as cu:
            row = cu.fetchone()
        return row[0]

    def __getitem__(self, key):
        with closing(self._execute(LOOKUP_KEY, (key,))) as cu:
            row = cu.fetchone()
        if not row:
            raise KeyError(key)
        return row[0]

    def __setitem__(self, key, value):
        with closing(self._execute(STORE_KV, (key, value))):
            pass

    def __delitem__(self, key):
        with closing(self._execute(DELETE_KEY, (key,))) as cu:
            if not cu.rowcount:
                raise KeyError(key)

    def __iter__(self):
        try:
            with closing(self._execute(ITER_KEYS)) as cu:
                for row in cu:
                    yield row[0]
        except sqlite3.Error as exc:
            raise error(str(exc))

    def close(self):
        if self._cx:
            self._cx.close()
            self._cx = None

    def keys(self):
        return list(super().keys())

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()


def open(path, /, flag="r", mode=None):
    return _Database(path, flag)
