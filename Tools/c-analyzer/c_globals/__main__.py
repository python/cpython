import argparse
import os.path
import re
import sys

from c_analyzer_common import SOURCE_DIRS, REPO_ROOT
from c_analyzer_common.info import UNKNOWN
from c_analyzer_common.known import (
    from_file as known_from_file,
    DATA_FILE as KNOWN_FILE,
    )
from . import find, show
from .supported import is_supported, ignored_from_file, IGNORED_FILE, _is_object


def _match_unused_global(variable, knownvars, used):
    found = []
    for varid in knownvars:
        if varid in used:
            continue
        if varid.funcname is not None:
            continue
        if varid.name != variable.name:
            continue
        if variable.filename and variable.filename != UNKNOWN:
            if variable.filename == varid.filename:
                found.append(varid)
        else:
            found.append(varid)
    return found


def _check_results(unknown, knownvars, used):
    badknown = set()
    for variable in sorted(unknown):
        msg = None
        if variable.funcname != UNKNOWN:
            msg = f'could not find global symbol {variable.id}'
        elif m := _match_unused_global(variable, knownvars, used):
            assert isinstance(m, list)
            badknown.update(m)
        elif variable.name in ('completed', 'id'):  # XXX Figure out where these variables are.
            unknown.remove(variable)
        else:
            msg = f'could not find local symbol {variable.id}'
        if msg:
            #raise Exception(msg)
            print(msg)
    if badknown:
        print('---')
        print(f'{len(badknown)} globals in known.tsv, but may actually be local:')
        for varid in sorted(badknown):
            print(f'{varid.filename:30} {varid.name}')
    unused = sorted(varid
                    for varid in set(knownvars) - used
                    if varid.name != 'id')  # XXX Figure out where these variables are.
    if unused:
        print('---')
        print(f'did not use {len(unused)} known vars:')
        for varid in unused:
            print(f'{varid.filename:30} {varid.funcname or "-":20} {varid.name}')
        raise Exception('not all known symbols used')
    if unknown:
        print('---')
        raise Exception('could not find all symbols')


def _find_globals(dirnames, known, ignored):
    if dirnames == SOURCE_DIRS:
        dirnames = [os.path.relpath(d, REPO_ROOT) for d in dirnames]

    ignored = ignored_from_file(ignored)
    known = known_from_file(known)

    used = set()
    unknown = set()
    knownvars = (known or {}).get('variables')
    for variable in find.globals_from_binary(knownvars=knownvars,
                                             dirnames=dirnames):
    #for variable in find.globals(dirnames, known, kind='platform'):
        if variable.vartype == UNKNOWN:
            unknown.add(variable)
            continue
        yield variable, is_supported(variable, ignored, known)
        used.add(variable.id)

    #_check_results(unknown, knownvars, used)


def cmd_check(cmd, dirs=SOURCE_DIRS, *,
              ignored=IGNORED_FILE,
              known=KNOWN_FILE,
              _find=_find_globals,
              _show=show.basic,
              _print=print,
              ):
    """
    Fail if there are unsupported globals variables.

    In the failure case, the list of unsupported variables
    will be printed out.
    """
    unsupported = [v for v, s in _find(dirs, known, ignored) if not s]
    if not unsupported:
        #_print('okay')
        return

    _print('ERROR: found unsupported global variables')
    _print()
    _show(sorted(unsupported))
    _print(f' ({len(unsupported)} total)')
    sys.exit(1)


def cmd_show(cmd, dirs=SOURCE_DIRS, *,
             ignored=IGNORED_FILE,
             known=KNOWN_FILE,
             skip_objects=False,
              _find=_find_globals,
             _show=show.basic,
             _print=print,
             ):
    """
    Print out the list of found global variables.

    The variables will be distinguished as "supported" or "unsupported".
    """
    allsupported = []
    allunsupported = []
    for found, supported in _find(dirs, known, ignored):
        if skip_objects:  # XXX Support proper filters instead.
            if _is_object(found.vartype):
                continue
        (allsupported if supported else allunsupported
         ).append(found)

    _print('supported:')
    _print('----------')
    _show(sorted(allsupported))
    _print(f' ({len(allsupported)} total)')
    _print()
    _print('unsupported:')
    _print('------------')
    _show(sorted(allunsupported))
    _print(f' ({len(allunsupported)} total)')


#############################
# the script

COMMANDS = {
        'check': cmd_check,
        'show': cmd_show,
        }

PROG = sys.argv[0]
PROG = 'c-globals.py'


def parse_args(prog=PROG, argv=sys.argv[1:], *, _fail=None):
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument('--ignored', metavar='FILE',
                        default=IGNORED_FILE,
                        help='path to file that lists ignored vars')
    common.add_argument('--known', metavar='FILE',
                        default=KNOWN_FILE,
                        help='path to file that lists known types')
    common.add_argument('dirs', metavar='DIR', nargs='*',
                        default=SOURCE_DIRS,
                        help='a directory to check')

    parser = argparse.ArgumentParser(
            prog=prog,
            )
    subs = parser.add_subparsers(dest='cmd')

    check = subs.add_parser('check', parents=[common])

    show = subs.add_parser('show', parents=[common])
    show.add_argument('--skip-objects', action='store_true')

    if _fail is None:
        def _fail(msg):
            parser.error(msg)

    # Now parse the args.
    args = parser.parse_args(argv)
    ns = vars(args)

    cmd = ns.pop('cmd')
    if not cmd:
        _fail('missing command')

    return cmd, ns


def main(cmd, cmdkwargs=None, *, _COMMANDS=COMMANDS):
    try:
        cmdfunc = _COMMANDS[cmd]
    except KeyError:
        raise ValueError(
            f'unsupported cmd {cmd!r}' if cmd else 'missing cmd')

    cmdfunc(cmd, **cmdkwargs or {})


if __name__ == '__main__':
    cmd, cmdkwargs = parse_args()
    main(cmd, cmdkwargs)
