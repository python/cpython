
def basic(statics, *,
          _print=print):
    """Print each row simply."""
    for static in statics:
        if static.funcname:
            line = f'{static.filename}:{static.funcname}():{static.name}'
        else:
            line = f'{static.filename}:{static.name}'
        vartype = static.vartype
        if vartype.startswith('static '):
            vartype = vartype.partition(' ')[2]
        else:
            vartype = '=' + vartype
        line = f'{line:<64} {vartype}'
        _print(line)
