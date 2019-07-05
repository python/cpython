
def basic(statics, *,
          _print=print):
    """Print each row simply."""
    for static in statics:
        if static.funcname:
            line = f'{static.filename}:{static.funcname}():{static.name}'
        else:
            line = f'{static.filename}:{static.name}'
        _print(line)
