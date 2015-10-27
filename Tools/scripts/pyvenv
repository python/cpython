#!/usr/bin/env python3
if __name__ == '__main__':
    import sys
    import pathlib

    executable = pathlib.Path(sys.executable or 'python3').name
    print('WARNING: the pyenv script is deprecated in favour of '
          f'`{executable} -m venv`', file=sys.stderr)

    rc = 1
    try:
        import venv
        venv.main()
        rc = 0
    except Exception as e:
        print('Error: %s' % e, file=sys.stderr)
    sys.exit(rc)
