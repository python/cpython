from c_parser import files, declarations


def find_symbol(name, dirnames, *,
                _perfilecache,
                _get_local_symbols=(lambda *a, **k: _get_symbols(*a, **k)),
                _iter_files=files.iter_files,
                ):
    for filename in _iter_files(dirnames, ('.c', '.h')):
        try:
            symbols = _perfilecache[filename]
        except KeyError:
            symbols = _perfilecache[filename] = _get_local_symbols(filename)

        try:
            instances = symbols[name]
        except KeyError:
            continue

        funcname, vartype = instances.pop(0)
        if not instances:
            symbols.pop(name)
        return filename, funcname, vartype


def _get_symbols(filename, *,
                       _iter_variables=declarations.iter_variables,
                       ):
    symbols = {}
    for funcname, name, vartype in _iter_variables(filename):
        if not funcname:
            continue
        try:
            instances = symbols[name]
        except KeyError:
            instances = symbols[name] = []
        instances.append((funcname, vartype))
    return symbols
