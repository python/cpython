#!/usr/bin/env python3
import sys
import os
import marshal


DIR = os.path.dirname(sys.argv[0])
# source code for module to freeze
FILE = os.path.join(DIR, 'flag.py')
# C symbol to use for array holding frozen bytes
SYMBOL = 'M___hello__'


def get_module_code(filename):
    """Compile 'filename' and return the module code as a marshalled byte
    string.
    """
    with open(filename, 'r') as fp:
        src = fp.read()
    co = compile(src, 'none', 'exec')
    co_bytes = marshal.dumps(co)
    return co_bytes


def gen_c_code(fp, co_bytes):
    """Generate C code for the module code in 'co_bytes', write it to 'fp'.
    """
    def write(*args, **kwargs):
        print(*args, **kwargs, file=fp)
    write('/* Generated with Tools/freeze/regen_frozen.py */')
    write('static unsigned char %s[] = {' % SYMBOL, end='')
    bytes_per_row = 13
    for i, opcode in enumerate(co_bytes):
        if (i % bytes_per_row) == 0:
            # start a new row
            write()
            write('    ', end='')
        write('%d,' % opcode, end='')
    write()
    write('};')


def main():
    out_filename = sys.argv[1]
    co_bytes = get_module_code(FILE)
    with open(out_filename, 'w') as fp:
        gen_c_code(fp, co_bytes)


if __name__ == '__main__':
    main()
