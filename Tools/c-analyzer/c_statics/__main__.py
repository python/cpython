import argparse
import sys

from c_analyzer_common import SOURCE_DIRS
from c_analyzer_common.known import (
    from_file as known_from_file,
    DATA_FILE as KNOWN_FILE,
    )
from . import find, show
from .supported import is_supported, ignored_from_file, IGNORED_FILE


def _find_statics(dirnames, known, ignored):
    ignored = ignored_from_file(ignored)
    known = known_from_file(known)

    knownvars = (known or {}).get('variables')
    for static in find.statics_from_binary(knownvars=knownvars,
                                           dirnames=dirnames):
    #for static in find.statics(dirnames, known, kind='platform'):
        yield static, is_supported(static, ignored, known)


def cmd_check(cmd, dirs=SOURCE_DIRS, *,
              ignored=IGNORED_FILE,
              known=KNOWN_FILE,
              _find=_find_statics,
              _show=show.basic,
              _print=print,
              ):
    """
    Fail if there are unsupported statics variables.

    In the failure case, the list of unsupported variables
    will be printed out.
    """
    unsupported = [v for v, s in _find(dirs, known, ignored) if not s]
    if not unsupported:
        #_print('okay')
        return

    _print('ERROR: found unsupported static variables')
    _print()
    _show(sorted(unsupported))
    # XXX totals?
    sys.exit(1)


def cmd_show(cmd, dirs=SOURCE_DIRS, *,
             ignored=IGNORED_FILE,
             known=KNOWN_FILE,
              _find=_find_statics,
             _show=show.basic,
             _print=print,
             ):
    """
    print out the list of found static variables.

    The variables will be distinguished as "supported" or "unsupported".
    """
    allsupported = []
    allunsupported = []
    for found, supported in _find(dirs, known, ignored):
        (allsupported if supported else allunsupported
         ).append(found)

    _print('supported:')
    _print('----------')
    _show(sorted(allsupported))
    # XXX totals?
    _print()
    _print('unsupported:')
    _print('------------')
    _show(sorted(allunsupported))
    # XXX totals?


#############################
# the script

COMMANDS = {
        'check': cmd_check,
        'show': cmd_show,
        }

PROG = sys.argv[0]
PROG = 'c-statics.py'


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

    check = subs.add_parser('show', parents=[common])

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
