def cstring(s, width=70):
    """Return C string representation of a Python string.

    width specifies the maximum width of any line of the C string.
    """
    L = []
    for l in s.split("\n"):
        if len(l) < width:
            L.append(r'"%s\n"' % l)

    return "\n".join(L)

def unindent(s, skipfirst=True):
    """Return an unindented version of a docstring.

    Removes indentation on lines following the first one, using the
    leading whitespace of the first indented line that is not blank
    to determine the indentation.
    """

    lines = s.split("\n")
    if skipfirst:
        first = lines.pop(0)
        L = [first]
    else:
        L = []
    indent = None
    for l in lines:
        ls = l.strip()
        if ls:
            indent = len(l) - len(ls)
            break
    L += [l[indent:] for l in lines]

    return "\n".join(L)
