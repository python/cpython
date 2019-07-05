
def is_supported(static):
    """Return True if the given static variable is okay in CPython."""
    # XXX finish!
    raise NotImplementedError
    for part in static.vartype.split():
        # XXX const is automatic True?
        if part == 'PyObject' or part.startswith('PyObject['):
            return False
    return True
