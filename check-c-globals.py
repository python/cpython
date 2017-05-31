
from collections import namedtuple
import os.path
import shutil
import sys
import subprocess


ROOT_DIR = os.path.dirname(__file__) or '.'

VERBOSITY = 0

# These variables are shared between all interpreters in the process.
with open('globals-runtime.txt') as file:
    RUNTIME_VARS = {line.strip()
                    for line in file
                    if line.strip() and not line.startswith('#')}
del file

IGNORED_VARS = {
        '_DYNAMIC',
        '_GLOBAL_OFFSET_TABLE_',
        '__JCR_LIST__',
        '__JCR_END__',
        '__TMC_END__',
        '__bss_start',
        '__data_start',
        '__dso_handle',
        '_edata',
        '_end',
        }


class Var(namedtuple('Var', 'name kind filename')):

    @property
    def is_global(self):
        return self.kind.isupper()


def find_vars(root, *, publiconly=False):
    python = os.path.join(root, 'python')
    nm = shutil.which('nm')
    if nm is None or not os.path.exists(python):
        raise NotImplementedError
    else:
        yield from (var
                    for var in _find_var_symbols(python, nm, publiconly=publiconly)
                    if var.name not in IGNORED_VARS)


NM_FUNCS = set('Tt')
NM_PUBLIC_VARS = set('BD')
NM_PRIVATE_VARS = set('bd')
NM_VARS = NM_PUBLIC_VARS | NM_PRIVATE_VARS
NM_DATA = set('Rr')
NM_OTHER = set('ACGgiINpSsuUVvWw-?')
NM_IGNORED = NM_FUNCS | NM_DATA | NM_OTHER | NM_PRIVATE_VARS


def _find_var_symbols(python, nm, *, publiconly=False):
    if publiconly:
        kinds = NM_PUBLIC_VARS
    else:
        kinds = NM_VARS
    args = [nm,
            '--line-numbers',
            python]
    out = subprocess.check_output(args)
    for line in out.decode('utf-8').splitlines():
        _, _, line = line.partition(' ')  # strip off the address
        line = line.strip()
        kind, _, line = line.partition(' ')
        name, _, filename = line.partition('\t')
        if filename:
            filename = os.path.relpath(filename.partition(':')[0])
        if kind in kinds:
            yield Var(name.strip(), kind, filename or '~???~')
        elif kind not in NM_IGNORED:
            raise RuntimeError('unsupported NM type {!r}'.format(kind))


def filter_var(var):
    name = var.name
    if (
        '.' in name or
        # Objects/typeobject.c
        name.startswith('op_id.') or
        name.startswith('rop_id.') or
        # Python/graminit.c
        name.startswith('arcs_') or
        name.startswith('states_')
        ):
        return False

    if (
        name.endswith('_doc') or
        name.endswith('__doc__') or
        name.endswith('_docstring') or
        name.startswith('doc_') or
        name.endswith('_methods') or
        name.endswith('_functions') or
        name.endswith('_fields') or
        name.endswith('_memberlist') or
        name.endswith('_members') or
        name.endswith('_slots') or
        name.endswith('_getset') or
        name.endswith('_getsets') or
        name.endswith('_getsetlist') or
        name.endswith('_as_mapping') or
        name.endswith('_as_number') or
        name.endswith('_as_sequence')
        ):
        return False

    if (
        name.endswith('_Type') or
        name.endswith('_type') or
        name.startswith('PyId_')
        ):
        return False

    return name not in RUNTIME_VARS


def format_var(var, fmt=None):
    if fmt is None:
        fmt = '{:40} {}'
    if var is None:
        return fmt.format('  NAME', '  FILE')
    if var == '-':
        return fmt.format('-'*40, '-'*20)
    filename = var.filename if var.is_global else '({})'.format(var.filename)
    return fmt.format(var.name, filename)


#######################################

def parse_args(argv=None):
    if argv is None:
        argv = sys.argv[1:]

    import argparse
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument('-v', '--verbose', action='count', default=0)
    common.add_argument('-q', '--quiet', action='count', default=0)

    filtered = argparse.ArgumentParser(add_help=False)
    filtered.add_argument('--publiconly', action='store_true')

    parser = argparse.ArgumentParser(parents=[common])
    subs = parser.add_subparsers()

    show = subs.add_parser('show',
                           description='print out all supported globals',
                           parents=[common, filtered])
    show.add_argument('--by-name', dest='sort_by_name', action='store_true')
    show.set_defaults(cmd='show')

    check = subs.add_parser('check',
                            description='print out all unsupported globals',
                            parents=[common, filtered])
    check.set_defaults(cmd='check')

    args = parser.parse_args(argv)

    try:
        cmd = vars(args).pop('cmd')
    except KeyError:
        # Fall back to the default.
        cmd = 'check'
        argv.insert(0, cmd)
        args = parser.parse_args(argv)
        del args.cmd

    verbose = vars(args).pop('verbose', 0)
    quiet = vars(args).pop('quiet', 0)
    args.verbosity = max(0, VERBOSITY + verbose - quiet)

    return cmd, args


def main_show(root=ROOT_DIR, *, sort_by_name=False, publiconly=False):
    allvars = (var
               for var in find_vars(root, publiconly=publiconly)
               if not filter_var(var))

    found = 0
    if sort_by_name:
        print(format_var(None))
        print(format_var('-'))
        for var in sorted(allvars, key=lambda v: v.name.strip('_')):
            found += 1
            print(format_var(var))

    else:
        current = None
        for var in sorted(allvars,
                          key=lambda v: (v.filename, v.name.strip('_'))):
            found += 1
            if var.filename != current:
                if current is None:
                    print('ERROR: found globals', file=sys.stderr)
                current = var.filename
                print('\n  # ' + current, file=sys.stderr)
            print(var.name if var.is_global else '({})'.format(var.name))
    print('\ntotal: {}'.format(found))


def main_check(root=ROOT_DIR, *, publiconly=False, quiet=False):
    allvars = (var
               for var in find_vars(root, publiconly=publiconly)
               if filter_var(var))

    if quiet:
        found = sum(1 for _ in allvars)
    else:
        found = 0
        current = None
        for var in sorted(allvars,
                          key=lambda v: (v.filename, v.name.strip('_'))):
            found += 1
            if var.filename != current:
                if current is None:
                    print('ERROR: found globals', file=sys.stderr)
                current = var.filename
                print('\n  # ' + current, file=sys.stderr)
            print(var.name if var.is_global else '({})'.format(var.name),
                  file=sys.stderr)
        
    print('\ntotal: {}'.format(found))
    rc = 1 if found else 0
    return rc


if __name__ == '__main__':
    sub, args = parse_args()

    verbosity = vars(args).pop('verbosity')
    if verbosity == 0:
        log = (lambda *args: None)

    main = main_check
    if sub == 'show':
        main = main_show
    sys.exit(
            main(**vars(args)))
