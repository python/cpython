import sys
import warnings

from . import __main__


def main():
    warnings.warn('The json.tool module is deprecated',
                  DeprecationWarning, stacklevel=2)
    __main__.main()


if __name__ == '__main__':
    try:
        main()
    except BrokenPipeError as exc:
        sys.exit(exc.errno)
