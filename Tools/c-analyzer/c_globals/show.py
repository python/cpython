
def basic(globals, *,
          _print=print):
    """Print each row simply."""
    for variable in globals:
        if variable.funcname:
            line = f'{variable.filename}:{variable.funcname}():{variable.name}'
        else:
            line = f'{variable.filename}:{variable.name}'
        vartype = variable.vartype
        #if vartype.startswith('static '):
        #    vartype = vartype.partition(' ')[2]
        #else:
        #    vartype = '=' + vartype
        line = f'{line:<64} {vartype}'
        _print(line)
