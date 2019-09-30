import argparse
import os.path
import re
import sys

from c_analyzer_common import SOURCE_DIRS, REPO_ROOT, show
from c_analyzer_common.find import vars_from_binary
from c_analyzer_common.info import UNKNOWN
from c_analyzer_common.known import (
    from_file as known_from_file,
    DATA_FILE as KNOWN_FILE,
    )
from .supported import is_supported, ignored_from_file, IGNORED_FILE, _is_object


def _check_results(unknown, knownvars, used):
    def _match_unused_global(variable):
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

    badknown = set()
    for variable in sorted(unknown):
        msg = None
        if variable.funcname != UNKNOWN:
            msg = f'could not find global symbol {variable.id}'
        elif m := _match_unused_global(variable):
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
    # For now we only use known variables (no source lookup).
    dirnames = None

    knownvars = (known or {}).get('variables')
    for var in vars_from_binary(known=known, dirnames=dirnames):
        if not var.isglobal:
            continue
        elif var.vartype == UNKNOWN:
            yield var, None
        else:
            yield var, is_supported(var, ignored, known)


def cmd_check(cmd, dirs=SOURCE_DIRS, *,
              known=KNOWN_FILE,
              ignored=IGNORED_FILE,
              checkfound=False,
              _known_from_file=known_from_file,
              _ignored_from_file=ignored_from_file,
              _find=_find_globals,
              _show=show.basic,
              _print=print,
              ):
    """
    Fail if there are unsupported globals variables.

    In the failure case, the list of unsupported variables
    will be printed out.
    """
    known = _known_from_file(known)
    ignored = _ignored_from_file(ignored)

    used = set()
    unknown = set()
    unsupported = []
    for var, supported in _find(dirs, known, ignored):
        if supported is None:
            unknown.add(var)
            continue
        used.add(var.id)
        if not supported:
            unsupported.append(var)

    if checkfound:
        # XXX Move this check to its own command.
        _check_results(unknown, known['variables'], used)

    if not unsupported:
        #_print('okay')
        return

    _print('ERROR: found unsupported global variables')
    _print()
    _show(sorted(unsupported))
    _print(f' ({len(unsupported)} total)')
    sys.exit(1)


def cmd_show(cmd, dirs=SOURCE_DIRS, *,
             known=KNOWN_FILE,
             ignored=IGNORED_FILE,
             skip_objects=False,
             _known_from_file=known_from_file,
             _ignored_from_file=ignored_from_file,
             _find=_find_globals,
             _show=show.basic,
             _print=print,
             ):
    """
    Print out the list of found global variables.

    The variables will be distinguished as "supported" or "unsupported".
    """
    known = _known_from_file(known)
    ignored = _ignored_from_file(ignored)

    allsupported = []
    allunsupported = []
    for found, supported in _find(dirs, known, ignored):
        if supported is None:
            continue
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
