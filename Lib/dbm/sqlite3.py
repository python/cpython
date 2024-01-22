import os
import sqlite3
from contextlib import suppress, closing
from collections.abc import MutableMapping

BUILD_TABLE = """
  CREATE TABLE Dict (
    key TEXT NOT NULL,
    value BLOB NOT NULL,
    PRIMARY KEY (key)
  )
"""
GET_SIZE = "SELECT COUNT (key) FROM Dict"
LOOKUP_KEY = "SELECT value FROM Dict WHERE key = ? LIMIT 1"
STORE_KV = "REPLACE INTO Dict (key, value) VALUES (?, ?)"
DELETE_KEY = "DELETE FROM Dict WHERE key = ?"
ITER_KEYS = "SELECT key FROM Dict"


def _normalize_uri_path(path):
    path = os.fsdecode(path)
    path = path.replace("?", "%3f")
    path = path.replace("#", "%23")
    while "//" in path:
        path = path.replace("//", "/")
    return path


class _Database(MutableMapping):

    def __init__(self, path, flag):
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
                raise ValueError("Flag must be one of 'r', 'w', 'c', or 'n', not",
                                 repr(flag))

        # We use the URI format when opening the database.
        if not path or path == ":memory:":
            uri = "file:?mode=memory"
        else:
            path = _normalize_uri_path(path)
            uri = f"file:{path}?mode={flag}"

        self.cx = sqlite3.connect(uri, autocommit=True, uri=True)
        self.cx.execute("PRAGMA journal_mode = wal")
        with suppress(sqlite3.OperationalError):
            self.cx.execute(BUILD_TABLE)

    def __len__(self):
        return self.cx.execute(GET_SIZE).fetchone()[0]

    def __getitem__(self, key):
        rows = [row for row in self.cx.execute(LOOKUP_KEY, (key,))]
        if not rows:
            raise KeyError(key)
        assert len(rows) == 1
        row = rows[0]
        return row[0]

    def __setitem__(self, key, value):
        self.cx.execute(STORE_KV, (key, value));

    def __delitem__(self, key):
        if key not in self:
            raise KeyError(key)
        self.cx.execute(DELETE_KEY, (key,));

    def __iter__(self):
        with closing(self.cx.execute(ITER_KEYS)) as cu:
            for row in cu:
                yield row[0]

    def close(self):
        self.cx.close()
        self.cx = None

    def keys(self):
        return list(super().keys())

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()


class error(OSError, sqlite3.Error):
    pass


def open(path, flag="r", mode=None):
    return _Database(path, flag)
