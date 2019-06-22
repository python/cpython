import argparse
import sys

from . import KNOWN_FILE, IGNORED_FILE, SOURCE_DIRS


def check(cmd, dirs=SOURCE_DIRS, *, ignored=IGNORED_FILE, known=KNOWN_FILE):
    """
    Fail if there are unsupported statics variables.

    In the failure case, the list of unsupported variables
    will be printed out.
    """
    raise NotImplementedError


def show(cmd, dirs=SOURCE_DIRS, *, ignored=IGNORED_FILE, known=KNOWN_FILE):
    """
    print out the list of found static variables.

    The variables will be distinguished as "supported" or "unsupported".
    """


#############################
# the script

COMMANDS = {
        'check': check,
        'show': show,
        }

PROG = sys.argv[0]
PROG = 'c-statics.py'


def parse_args(prog=PROG, argv=sys.argv[1:]):
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

    # Now parse the args.
    args = parser.parse_args(argv)
    ns = vars(args)

    cmd = ns.pop('cmd')
    if not cmd:
        parser.error('missing command')

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
