import argparse
import sys


#############################
# the script

def parse_args(prog=sys.argv[0], argv=sys.argv[1:]):
    parser = argparse.ArgumentParser(
            prog=prog,
            )
    parser.set_defaults(cmd=None)

    args = parser.parse_args(argv)
    ns = vars(args)

    cmd = ns.pop('cmd')

    return cmd, ns


def main(cmd, cmdkwargs):
    return


if __name__ == '__main__':
    cmd, cmdkwargs = parse_args()
    main(cmd, cmdkwargs)
