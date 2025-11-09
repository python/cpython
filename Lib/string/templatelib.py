"""Support for template string literals (t-strings)."""

t = t"{0}"
Template = type(t)
Interpolation = type(t.interpolations[0])
del t

def convert(obj, /, conversion):
    """Convert *obj* using formatted string literal semantics."""
    if conversion is None:
        return obj
    if conversion == 'r':
        return repr(obj)
    if conversion == 's':
        return str(obj)
    if conversion == 'a':
        return ascii(obj)
    raise ValueError(f'invalid conversion specifier: {conversion}')

def _template_unpickle(*args):
    import itertools

    if len(args) != 2:
        raise ValueError('Template expects tuple of length 2 to unpickle')

    strings, interpolations = args
    parts = []
    for string, interpolation in itertools.zip_longest(strings, interpolations):
        if string is not None:
            parts.append(string)
        if interpolation is not None:
            parts.append(interpolation)
    return Template(*parts)
