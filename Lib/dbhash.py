"""Provide a (g)dbm-compatible interface to bsddb.hashopen."""

import sys
try:
    import bsddb
except ImportError:
    # prevent a second import of this module from spuriously succeeding
    del sys.modules[__name__]
    raise

__all__ = ["error","open"]

error = bsddb.error                     # Exported for anydbm

def open(file, flag = 'r', mode=0666):
    return bsddb.hashopen(file, flag, mode)
