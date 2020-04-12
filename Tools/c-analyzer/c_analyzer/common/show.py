
def basic(variables, *,
          _print=print):
    """Print each row simply."""
    for var in variables:
        if var.funcname:
            line = f'{var.filename}:{var.funcname}():{var.name}'
        else:
            line = f'{var.filename}:{var.name}'
        line = f'{line:<64} {var.vartype}'
        _print(line)
