import re

from ..common.info import UNKNOWN, ID

from .preprocessor import _iter_clean_lines


_NOT_SET = object()


def get_srclines(filename, *,
                 cache=None,
                 _open=open,
                 _iter_lines=_iter_clean_lines,
                 ):
    """Return the file's lines as a list.

    Each line will have trailing whitespace removed (including newline).

    If a cache is given the it is used.
    """
    if cache is not None:
        try:
            return cache[filename]
        except KeyError:
            pass

    with _open(filename) as srcfile:
        srclines = [line
                    for _, line in _iter_lines(srcfile)
                    if not line.startswith('#')]
    for i, line in enumerate(srclines):
        srclines[i] = line.rstrip()

    if cache is not None:
        cache[filename] = srclines
    return srclines


def parse_variable_declaration(srcline):
    """Return (name, decl) for the given declaration line."""
    # XXX possible false negatives...
    decl, sep, _ = srcline.partition('=')
    if not sep:
        if not srcline.endswith(';'):
            return None, None
        decl = decl.strip(';')
    decl = decl.strip()
    m = re.match(r'.*\b(\w+)\s*(?:\[[^\]]*\])?$', decl)
    if not m:
        return None, None
    name = m.group(1)
    return name, decl


def parse_variable(srcline, funcname=None):
    """Return (varid, decl) for the variable declared on the line (or None)."""
    line = srcline.strip()

    # XXX Handle more than just static variables.
    if line.startswith('static '):
        if '(' in line and '[' not in line:
            # a function
            return None, None
        return parse_variable_declaration(line)
    else:
        return None, None


def iter_variables(filename, *,
                   srccache=None,
                   parse_variable=None,
                   _get_srclines=get_srclines,
                   _default_parse_variable=parse_variable,
                   ):
    """Yield (varid, decl) for each variable in the given source file."""
    if parse_variable is None:
        parse_variable = _default_parse_variable

    indent = ''
    prev = ''
    funcname = None
    for line in _get_srclines(filename, cache=srccache):
        # remember current funcname
        if funcname:
            if line == indent + '}':
                funcname = None
                continue
        else:
            if '(' in prev and line == indent + '{':
                if not prev.startswith('__attribute__'):
                    funcname = prev.split('(')[0].split()[-1]
                    prev = ''
                    continue
            indent = line[:-len(line.lstrip())]
            prev = line

        info = parse_variable(line, funcname)
        if isinstance(info, list):
            for name, _funcname, decl in info:
                yield ID(filename, _funcname, name), decl
            continue
        name, decl = info

        if name is None:
            continue
        yield ID(filename, funcname, name), decl


def _match_varid(variable, name, funcname, ignored=None):
    if ignored and variable in ignored:
        return False

    if variable.name != name:
        return False

    if funcname == UNKNOWN:
        if not variable.funcname:
            return False
    elif variable.funcname != funcname:
        return False

    return True


def find_variable(filename, funcname, name, *,
                  ignored=None,
                  srccache=None,  # {filename: lines}
                  parse_variable=None,
                  _iter_variables=iter_variables,
                  ):
    """Return the matching variable.

    Return None if the variable is not found.
    """
    for varid, decl in _iter_variables(filename,
                                    srccache=srccache,
                                    parse_variable=parse_variable,
                                    ):
        if _match_varid(varid, name, funcname, ignored):
            return varid, decl
    else:
        return None


def find_variables(varids, filenames=None, *,
                   srccache=_NOT_SET,
                   parse_variable=None,
                   _find_symbol=find_variable,
                   ):
    """Yield (varid, decl) for each ID.

    If the variable is not found then its decl will be UNKNOWN.  That
    way there will be one resulting variable per given ID.
    """
    if srccache is _NOT_SET:
        srccache = {}

    used = set()
    for varid in varids:
        if varid.filename and varid.filename != UNKNOWN:
            srcfiles = [varid.filename]
        else:
            if not filenames:
                yield varid, UNKNOWN
                continue
            srcfiles = filenames
        for filename in srcfiles:
            varid, decl = _find_varid(filename, varid.funcname, varid.name,
                                      ignored=used,
                                      srccache=srccache,
                                      parse_variable=parse_variable,
                                      )
            if varid:
                yield varid, decl
                used.add(varid)
                break
        else:
            yield varid, UNKNOWN
