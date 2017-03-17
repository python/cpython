[Security] Issue #27278: Fix os.urandom() implementation using getrandom() on
Linux.  Truncate size to INT_MAX and loop until we collected enough random
bytes, instead of casting a directly Py_ssize_t to int.