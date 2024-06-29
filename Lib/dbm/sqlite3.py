import os
import sqlite3
from pathlib import Path
from contextlib import suppress, closing
from collections.abc import MutableMapping

BUILD_TABLE = """
  CREATE TABLE IF NOT EXISTS Dict (
    key BLOB UNIQUE NOT NULL,
    value BLOB NOT NULL
  )
"""
GET_SIZE = "SELECT COUNT (key) FROM Dict"
LOOKUP_KEY = "SELECT value FROM Dict WHERE key = CAST(? AS BLOB)"
STORE_KV = "REPLACE INTO Dict (key, value) VALUES (CAST(? AS BLOB), CAST(? AS BLOB))"
DELETE_KEY = "DELETE FROM Dict WHERE key = CAST(? AS BLOB)"
ITER_KEYS = "SELECT key FROM Dict"


class error(OSError):
    pass


_ERR_CLOSED = "DBM object has already been closed"
_ERR_REINIT = "DBM object does not support reinitialization"


def _normalize_uri(path):
    path = Path(path)
    uri = path.absolute().as_uri()
    while "//" in uri:
        uri = uri.replace("//", "/")
    return uri


class _Database(MutableMapping):

    def __init__(self, path, /, *, flag, mode):
        if hasattr(self, "_cx"):
            raise error(_ERR_REINIT)

        path = os.fsdecode(path)
        match flag:
            case "r":
                flag = "ro"
            case "w":
                flag = "rw"
            case "c":
                flag = "rwc"
                Path(path).touch(mode=mode, exist_ok=True)
            case "n":
                flag = "rwc"
                Path(path).unlink(missing_ok=True)
                Path(path).touch(mode=mode)
            case _:
                raise ValueError("Flag must be one of 'r', 'w', 'c', or 'n', "
                                 f"not {flag!r}")

        # We use the URI format when opening the database.
        uri = _normalize_uri(path)
        uri = f"{uri}?mode={flag}"

        try:
            self._cx = sqlite3.connect(uri, autocommit=True, uri=True)
        except sqlite3.Error as exc:
            raise error(str(exc))

        # This is an optimization only; it's ok if it fails.
        with suppress(sqlite3.OperationalError):
            self._cx.execute("PRAGMA journal_mode = wal")

        if flag == "rwc":
            self._execute(BUILD_TABLE)

    def _execute(self, *args, **kwargs):
        if not self._cx:
            raise error(_ERR_CLOSED)
        try:
            return closing(self._cx.execute(*args, **kwargs))
        except sqlite3.Error as exc:
            raise error(str(exc))

    def __len__(self):
        with self._execute(GET_SIZE) as cu:
            row = cu.fetchone()
        return row[0]

    def __getitem__(self, key):
        with self._execute(LOOKUP_KEY, (key,)) as cu:
            row = cu.fetchone()
        if not row:
            raise KeyError(key)
        return row[0]

    def __setitem__(self, key, value):
        self._execute(STORE_KV, (key, value))

    def __delitem__(self, key):
        with self._execute(DELETE_KEY, (key,)) as cu:
            if not cu.rowcount:
                raise KeyError(key)

    def __iter__(self):
        try:
            with self._execute(ITER_KEYS) as cu:
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


def open(filename, /, flag="r", mode=0o666):
    """Open a dbm.sqlite3 database and return the dbm object.

    The 'filename' parameter is the name of the database file.

    The optional 'flag' parameter can be one of ...:
        'r' (default): open an existing database for read only access
        'w': open an existing database for read/write access
        'c': create a database if it does not exist; open for read/write access
        'n': always create a new, empty database; open for read/write access

    The optional 'mode' parameter is the Unix file access mode of the database;
    only used when creating a new database. Default: 0o666.
    """
    return _Database(filename, flag=flag, mode=mode)
