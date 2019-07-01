import shlex
import subprocess


def iter_statements(lines, *, local=False):
    """Yield (lines, issimple) for each statement in the given lines.

    Only the (relative) top-level statements are yielded, hence
    "issimple".  For compound statements (including functions), the ends
    and block(s) of the statement are provided as-is.  For simple
    statements a single line will be provided.
    """
    # XXX Bail out upon bogus syntax.
    comment = False
    for line in lines:
        # Deal with comments.
        if comment:
            _, sep, line = line.partition('*/')
            if sep:
                comment = False
        line, _, _ = line.partition('//')
        line, sep, _ = line.partition('/*')
        if sep:
            comment = True

        if not line.strip():
            continue

        raise NotImplementedError
        yield('', True)


def parse_func(lines):
    """Return (name, signature, body) for the given function definition."""
    raise NotImplementedError


def parse_var(lines):
    """Return (name, vartype) for the given variable declaration."""
    raise NotImplementedError


def parse_compound(lines):
    """Return (allsimple, blocks) for the given compound statement."""
    # XXX Identify declarations inside compound statements
    # (if/switch/for/while).
    raise NotImplementedError


def _iter_source_lines(filename):
    gcc, cflags = _get_build()
    content = _preprocess(filename, gcc, cflags)
    return iter(content.splitlines())


def iter_variables(filename, *,
                   _iter_source_lines=_iter_source_lines,
                   _iter_statements=iter_statements,
                   _parse_func=parse_func,
                   _parse_var=parse_var,
                   _parse_compound=parse_compound,
                   ):
    """Yield (funcname, name, vartype) for every variable in the given file."""
    lines = _iter_source_lines(filename)
    for stmt, issimple in _iter_statements(lines, local=False):
        # At the file top-level we only have to worry about vars & funcs.
        if issimple:
            name, vartype = _parse_var(stmt)
            if name:
                yield (None, name, vartype)
        else:
            funcname, _, body = _parse_func(stmt)
            localvars = _iter_locals(body,
                                     _iter_statements=_iter_statements,
                                     _parse_func=_parse_func,
                                     _parse_var=_parse_var,
                                     _parse_compound=_parse_compound,
                                     )
            for name, vartype in localvars:
                yield (funcname, name, vartype)


def _iter_locals(lines, *,
                 _iter_statements=iter_statements,
                 _parse_func=parse_func,
                 _parse_var=parse_var,
                 _parse_compound=parse_compound,
                 ):
    compound = [lines]
    while compound:
        block = compound.pop(0)
        blocklines = block.splitlines()
        for stmt, issimple in _iter_statements(blocklines, local=True):
            if issimple:
                name, vartype = _parse_var(stmt)
                if name:
                    yield (name, vartype)
            else:
                simple, blocks = _parse_compound(stmt)
                for line in simple:
                    name, vartype = _parse_var(line)
                    if name:
                        yield (name, vartype)
                compound.extend(blocks)


#############################
# GCC preprocessor (platform-specific)

def _get_build(*,
               _open=open,
               _run=subprocess.run,
               ):
    with _open('/tmp/print.mk', 'w') as tmpfile:
        tmpfile.write('print-%:\n')
        #tmpfile.write('\t@echo $* = $($*)\n')
        tmpfile.write('\t@echo $($*)\n')
    argv = ['/usr/bin/make',
            '-f', 'Makefile',
            '-f', '/tmp/print.mk',
            'print-CC',
            'print-PY_CORE_CFLAGS',
            ]
    proc = _run(argv,
                capture_output=True,
                text=True,
                check=True,
                )
    gcc, cflags = proc.stdout.strip().splitlines()
    cflags = shlex.split(cflags)
    return gcc, cflags


def _preprocess(filename, gcc, cflags, *,
                _run=subprocess.run,
                ):
    argv = gcc.split() + cflags
    argv.extend([
            '-E', filename,
            ])
    proc = _run(argv,
                capture_output=True,
                text=True,
                check=True,
                )
    return proc.stdout
