import argparse
import sys

from . import (
        KNOWN_FILE, IGNORED_FILE, SOURCE_DIRS,
        find, show,
        )


def cmd_check(cmd, dirs=SOURCE_DIRS, *,
              ignored=IGNORED_FILE, known=KNOWN_FILE,
              _find=find.statics,
              _show=show.basic,
              _print=print,
              ):
    """
    Fail if there are unsupported statics variables.

    In the failure case, the list of unsupported variables
    will be printed out.
    """
    unsupported = [v for v, s in _find(dirs, ignored, known) if not s]
    if not unsupported:
        #_print('okay')
        return

    _print('ERROR: found unsupported static variables')
    _print()
    _show(unsupported)
    # XXX totals?
    sys.exit(1)


def cmd_show(cmd, dirs=SOURCE_DIRS, *,
             ignored=IGNORED_FILE, known=KNOWN_FILE,
             _find=find.statics,
             _show=show.basic,
             _print=print,
             ):
    """
    print out the list of found static variables.

    The variables will be distinguished as "supported" or "unsupported".
    """
    allsupported = []
    allunsupported = []
    for found, supported in _find(dirs, ignored, known):
        (allsupported if supported else allunsupported
         ).append(found)

    _print('supported:')
    _print('----------')
    _show(allsupported)
    # XXX totals?
    _print()
    _print('unsupported:')
    _print('------------')
    _show(allunsupported)
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
