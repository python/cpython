"""Support for template string literals (t-strings)."""

__all__ = [
    "Interpolation",
    "Template",
]

t = t"{0}"
Template = type(t)
Interpolation = type(t.interpolations[0])
del t

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
