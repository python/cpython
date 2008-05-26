"""Provide a (g)dbm-compatible interface to bsddb.hashopen."""

import bsddb

__all__ = ["error", "open"]

error = bsddb.error

def open(file, flag = 'r', mode=0o666):
    return bsddb.hashopen(file, flag, mode)
