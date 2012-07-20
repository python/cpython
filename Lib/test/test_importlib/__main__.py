"""Run importlib's test suite.

Specifying the ``--builtin`` flag will run tests, where applicable, with
builtins.__import__ instead of importlib.__import__.

"""
from . import test_main


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Execute the importlib test '
                                                  'suite')
    parser.add_argument('-b', '--builtin', action='store_true', default=False,
                        help='use builtins.__import__() instead of importlib')
    args = parser.parse_args()
    if args.builtin:
        util.using___import__ = True
    test_main()
