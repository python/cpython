#!/usr/bin/env python
# Script checking that all symbols exported by libpython start with Py or _Py

import subprocess
import sys
import sysconfig


def get_exported_symbols():
    LIBRARY = sysconfig.get_config_var('LIBRARY')
    if not LIBRARY:
        raise Exception("failed to get LIBRARY")

    args = ('nm', '-p', LIBRARY)
    print("+ %s" % ' '.join(args))
    proc = subprocess.run(args, stdout=subprocess.PIPE, universal_newlines=True)
    if proc.returncode:
        sys.stdout.write(proc.stdout)
        sys.exit(proc.returncode)

    stdout = proc.stdout.rstrip()
    if not stdout:
        raise Exception("command output is empty")
    return stdout


def get_smelly_symbols(stdout):
    symbols = []
    ignored_symtypes = set()

    allowed_prefixes = ('Py', '_Py')
    if sys.platform == 'darwin':
        allowed_prefixes += ('__Py',)

    for line in stdout.splitlines():
        # Split line '0000000000001b80 D PyTextIOWrapper_Type'
        if not line:
            continue

        parts = line.split(maxsplit=2)
        if len(parts) < 3:
            continue

        symtype = parts[1].strip()
        # Ignore private symbols.
        #
        # If lowercase, the symbol is usually local; if uppercase, the symbol
        # is global (external).  There are however a few lowercase symbols that
        # are shown for special global symbols ("u", "v" and "w").
        if symtype.islower() and symtype not in "uvw":
            ignored_symtypes.add(symtype)
            continue

        symbol = parts[-1]
        if symbol.startswith(allowed_prefixes):
            continue
        symbol = '%s (type: %s)' % (symbol, symtype)
        symbols.append(symbol)

    if ignored_symtypes:
        print("Ignored symbol types: %s" % ', '.join(sorted(ignored_symtypes)))
        print()
    return symbols


def main():
    nm_output = get_exported_symbols()
    symbols = get_smelly_symbols(nm_output)

    if not symbols:
        print("OK: no smelly symbol found")
        sys.exit(0)

    symbols.sort()
    for symbol in symbols:
        print("Smelly symbol: %s" % symbol)
    print()
    print("ERROR: Found %s smelly symbols!" % len(symbols))
    sys.exit(1)


if __name__ == "__main__":
    main()
