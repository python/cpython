"""Provide a (g)dbm-compatible interface to bsddb.hashopen."""

import bsddb

__all__ = ["error", "open"]

class error(bsddb.error, IOError):
    pass

def open(file, flag = 'r', mode=0o666):
    return bsddb.hashopen(file, flag, mode)
