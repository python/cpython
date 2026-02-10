from pathlib import Path

CPYTHON_ROOT = Path(
    __file__,  # cpython/Doc/tools/check-epub.py
    '..',  # cpython/Doc/tools
    '..',  # cpython/Doc
    '..',  # cpython
).resolve()
EPUBCHECK_PATH = CPYTHON_ROOT / 'Doc' / 'epubcheck.txt'


def main() -> int:
    lines = EPUBCHECK_PATH.read_text(encoding='utf-8').splitlines()
    fatal_errors = [line for line in lines if line.startswith('FATAL')]

    if fatal_errors:
        err_count = len(fatal_errors)
        s = 's' * (err_count != 1)
        print()
        print(f'Error: epubcheck reported {err_count} fatal error{s}:')
        print()
        print('\n'.join(fatal_errors))
        return 1

    print('Success: no fatal errors found.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
