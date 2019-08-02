
def is_supported(static, ignored=None, known=None):
    """Return True if the given static variable is okay in CPython."""
    # XXX finish!
    raise NotImplementedError
    if static in ignored:
        return True
    if static in known:
        return True
    for part in static.vartype.split():
        # XXX const is automatic True?
        if part == 'PyObject' or part.startswith('PyObject['):
            return False
    return True


#############################
# ignored

def ignored_from_file(infile, fmt=None):
    """Yield StaticVar for each ignored var in the file."""
    raise NotImplementedError


#############################
# known

def known_from_file(infile, fmt=None):
    """Yield StaticVar for each ignored var in the file."""
    raise NotImplementedError
