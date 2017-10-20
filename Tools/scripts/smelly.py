#!/usr/bin/env python
# Script checking if all symbols exported by libpython start with "Py" or "_Py"

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
    for line in stdout.splitlines():
        # Split line '0000000000001b80 D PyTextIOWrapper_Type'
        if not line:
            continue
        parts = line.split(maxsplit=2)
        if len(parts) < 3:
            continue
        symtype = parts[1].strip()
        if symtype not in 'TDB':
            continue
        symbol = parts[-1]
        if symbol.startswith(('Py', '_Py')):
            continue
        symbols.append(symbol)
    return symbols


def main():
    stdout = get_exported_symbols()
    symbols = get_smelly_symbols(stdout)

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
