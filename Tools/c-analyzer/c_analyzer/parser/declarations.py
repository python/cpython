import re
import shlex
import subprocess

from ..common.info import UNKNOWN

from . import source


IDENTIFIER = r'(?:[a-zA-z]|_+[a-zA-Z0-9]\w*)'

TYPE_QUAL = r'(?:const|volatile)'

VAR_TYPE_SPEC = r'''(?:
        void |
        (?:
         (?:(?:un)?signed\s+)?
         (?:
          char |
          short |
          int |
          long |
          long\s+int |
          long\s+long
          ) |
         ) |
        float |
        double |
        {IDENTIFIER} |
        (?:struct|union)\s+{IDENTIFIER}
        )'''

POINTER = rf'''(?:
        (?:\s+const)?\s*[*]
        )'''

#STRUCT = r'''(?:
#        (?:struct|(struct\s+%s))\s*[{]
#            [^}]*
#        [}]
#        )''' % (IDENTIFIER)
#UNION = r'''(?:
#        (?:union|(union\s+%s))\s*[{]
#            [^}]*
#        [}]
#        )''' % (IDENTIFIER)
#DECL_SPEC = rf'''(?:
#        ({VAR_TYPE_SPEC}) |
#        ({STRUCT}) |
#        ({UNION})
#        )'''

FUNC_START = rf'''(?:
        (?:
          (?:
            extern |
            static |
            static\s+inline
           )\s+
         )?
        #(?:const\s+)?
        {VAR_TYPE_SPEC}
        )'''
#GLOBAL_VAR_START = rf'''(?:
#        (?:
#          (?:
#            extern |
#            static
#           )\s+
#         )?
#        (?:
#           {TYPE_QUAL}
#           (?:\s+{TYPE_QUAL})?
#         )?\s+
#        {VAR_TYPE_SPEC}
#        )'''
GLOBAL_DECL_START_RE = re.compile(rf'''
        ^
        (?:
            ({FUNC_START})
         )
        ''', re.VERBOSE)

LOCAL_VAR_START = rf'''(?:
        (?:
          (?:
            register |
            static
           )\s+
         )?
        (?:
          (?:
            {TYPE_QUAL}
            (?:\s+{TYPE_QUAL})?
           )\s+
         )?
        {VAR_TYPE_SPEC}
        {POINTER}?
        )'''
LOCAL_STMT_START_RE = re.compile(rf'''
        ^
        (?:
            ({LOCAL_VAR_START})
         )
        ''', re.VERBOSE)


def iter_global_declarations(lines):
    """Yield (decl, body) for each global declaration in the given lines.

    For function definitions the header is reduced to one line and
    the body is provided as-is.  For other compound declarations (e.g.
    struct) the entire declaration is reduced to one line and "body"
    is None.  Likewise for simple declarations (e.g. variables).

    Declarations inside function bodies are ignored, though their text
    is provided in the function body.
    """
    # XXX Bail out upon bogus syntax.
    lines = source.iter_clean_lines(lines)
    for line in lines:
        if not GLOBAL_DECL_START_RE.match(line):
            continue
        # We only need functions here, since we only need locals for now.
        if line.endswith(';'):
            continue
        if line.endswith('{') and '(' not in line:
            continue

        # Capture the function.
        # (assume no func is a one-liner)
        decl = line
        while '{' not in line:  # assume no inline structs, etc.
            try:
                line = next(lines)
            except StopIteration:
                return
            decl += ' ' + line

        body, end = _extract_block(lines)
        if end is None:
            return
        assert end == '}'
        yield (f'{decl}\n{body}\n{end}', body)


def iter_local_statements(lines):
    """Yield (lines, blocks) for each statement in the given lines.

    For simple statements, "blocks" is None and the statement is reduced
    to a single line.  For compound statements, "blocks" is a pair of
    (header, body) for each block in the statement.  The headers are
    reduced to a single line each, but the bpdies are provided as-is.
    """
    # XXX Bail out upon bogus syntax.
    lines = source.iter_clean_lines(lines)
    for line in lines:
        if not LOCAL_STMT_START_RE.match(line):
            continue

        stmt = line
        blocks = None
        if not line.endswith(';'):
            # XXX Support compound & multiline simple statements.
            #blocks = []
            continue

        yield (stmt, blocks)


def _extract_block(lines):
    end = None
    depth = 1
    body = []
    for line in lines:
        depth += line.count('{') - line.count('}')
        if depth == 0:
            end = line
            break
        body.append(line)
    return '\n'.join(body), end


def parse_func(stmt, body):
    """Return (name, signature) for the given function definition."""
    header, _, end = stmt.partition(body)
    assert end.strip() == '}'
    assert header.strip().endswith('{')
    header, _, _= header.rpartition('{')

    signature = ' '.join(header.strip().splitlines())

    _, _, name = signature.split('(')[0].strip().rpartition(' ')
    assert name

    return name, signature


#TYPE_SPEC = rf'''(?:
#        )'''
#VAR_DECLARATOR = rf'''(?:
#        )'''
#VAR_DECL = rf'''(?:
#            {TYPE_SPEC}+
#            {VAR_DECLARATOR}
#            \s*
#        )'''
#VAR_DECLARATION = rf'''(?:
#            {VAR_DECL}
#            (?: = [^=] [^;]* )?
#            ;
#        )'''
#
#
#def parse_variable(decl, *, inFunc=False):
#    """Return [(name, storage, vartype)] for the given variable declaration."""
#    ...


def _parse_var(stmt):
    """Return (name, vartype) for the given variable declaration."""
    stmt = stmt.rstrip(';')
    m = LOCAL_STMT_START_RE.match(stmt)
    assert m
    vartype = m.group(0)
    name = stmt[len(vartype):].partition('=')[0].strip()

    if name.startswith('('):
        name, _, after = name[1:].partition(')')
        assert after
        name = name.replace('*', '* ')
        inside, _, name = name.strip().rpartition(' ')
        vartype = f'{vartype} ({inside.strip()}){after}'
    else:
        name = name.replace('*', '* ')
        before, _, name = name.rpartition(' ')
        vartype = f'{vartype} {before}'

    vartype = vartype.strip()
    while '  ' in vartype:
        vartype = vartype.replace('  ', ' ')

    return name, vartype


def extract_storage(decl, *, infunc=None):
    """Return (storage, vartype) based on the given declaration.

    The default storage is "implicit" (or "local" if infunc is True).
    """
    if decl == UNKNOWN:
        return decl
    if decl.startswith('static '):
        return 'static'
        #return 'static', decl.partition(' ')[2].strip()
    elif decl.startswith('extern '):
        return 'extern'
        #return 'extern', decl.partition(' ')[2].strip()
    elif re.match('.*\b(static|extern)\b', decl):
        raise NotImplementedError
    elif infunc:
        return 'local'
    else:
        return 'implicit'


def parse_compound(stmt, blocks):
    """Return (headers, bodies) for the given compound statement."""
    # XXX Identify declarations inside compound statements
    # (if/switch/for/while).
    raise NotImplementedError


def iter_variables(filename, *,
                   preprocessed=False,
                   _iter_source_lines=source.iter_lines,
                   _iter_global=iter_global_declarations,
                   _iter_local=iter_local_statements,
                   _parse_func=parse_func,
                   _parse_var=_parse_var,
                   _parse_compound=parse_compound,
                   ):
    """Yield (funcname, name, vartype) for every variable in the given file."""
    if preprocessed:
        raise NotImplementedError
    lines = _iter_source_lines(filename)
    for stmt, body in _iter_global(lines):
        # At the file top-level we only have to worry about vars & funcs.
        if not body:
            name, vartype = _parse_var(stmt)
            if name:
                yield (None, name, vartype)
        else:
            funcname, _ = _parse_func(stmt, body)
            localvars = _iter_locals(body,
                                     _iter_statements=_iter_local,
                                     _parse_var=_parse_var,
                                     _parse_compound=_parse_compound,
                                     )
            for name, vartype in localvars:
                yield (funcname, name, vartype)


def _iter_locals(lines, *,
                 _iter_statements=iter_local_statements,
                 _parse_var=_parse_var,
                 _parse_compound=parse_compound,
                 ):
    compound = [lines]
    while compound:
        body = compound.pop(0)
        bodylines = body.splitlines()
        for stmt, blocks in _iter_statements(bodylines):
            if not blocks:
                name, vartype = _parse_var(stmt)
                if name:
                    yield (name, vartype)
            else:
                headers, bodies = _parse_compound(stmt, blocks)
                for header in headers:
                    for line in header:
                        name, vartype = _parse_var(line)
                        if name:
                            yield (name, vartype)
                compound.extend(bodies)


def iter_all(filename, *,
             preprocessed=False,
             ):
    """Yield a Declaration for each one found.

    If there are duplicates, due to preprocessor conditionals, then
    they are checked to make sure they are the same.
    """
    # XXX For the moment we cheat.
    for funcname, name, decl in iter_variables(filename,
                                               preprocessed=preprocessed):
        yield 'variable', funcname, name, decl
