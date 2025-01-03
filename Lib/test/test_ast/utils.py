def to_tuple(t):
    if t is None or isinstance(t, (str, int, complex, float, bytes)) or t is Ellipsis:
        return t
    elif isinstance(t, list):
        return [to_tuple(e) for e in t]
    result = [t.__class__.__name__]
    if hasattr(t, 'lineno') and hasattr(t, 'col_offset'):
        result.append((t.lineno, t.col_offset))
        if hasattr(t, 'end_lineno') and hasattr(t, 'end_col_offset'):
            result[-1] += (t.end_lineno, t.end_col_offset)
    if t._fields is None:
        return tuple(result)
    for f in t._fields:
        result.append(to_tuple(getattr(t, f)))
    return tuple(result)
