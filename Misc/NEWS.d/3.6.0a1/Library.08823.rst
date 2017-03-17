Issue #24879: help() and pydoc can now list named tuple fields in the
order they were defined rather than alphabetically.  The ordering is
determined by the _fields attribute if present.