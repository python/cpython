"""Provide a (g)dbm-compatible interface to bsdhash.hashopen."""

import bsddb

__all__ = ["error","open"]

error = bsddb.error                     # Exported for anydbm

def open(file, flag, mode=0666):
    return bsddb.hashopen(file, flag, mode)
