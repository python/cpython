# This is a script equivalent of running "python -m test.test_c_globals.cg".

from cpython.__main__ import parse_args, main


# This is effectively copied from cg/__main__.py:
if __name__ == '__main__':
    cmd, cmdkwargs = parse_args()
    main(cmd, cmdkwargs)
