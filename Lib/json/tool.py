r"""Command-line tool to validate and pretty-print JSON

Usage::

    $ echo '{"json":"obj"}' | python -m json.tool
    {
        "json": "obj"
    }
    $ echo '{ 1.2:3.4}' | python -m json.tool
    Expecting property name enclosed in double quotes: line 1 column 3 (char 2)

"""
import argparse
import contextlib
import json
import os.path
import sys
import tempfile


@contextlib.contextmanager
def _staged_outfile(path):
    # Use tempfile to support outfile == infile case.
    # See https://discuss.python.org/t/adding-atomicwrite-in-stdlib/11899
    dir, base = os.path.split(path)
    fd, tempname = tempfile.mkstemp(prefix=base + '.', dir=dir)
    file = os.fdopen(fd, "w", encoding="utf-8")
    try:
        with file:
            yield file
    except BaseException as e:
        os.remove(tempname)
        raise
    else:
        os.replace(tempname, path)


def main():
    prog = 'python -m json.tool'
    description = ('A simple command line interface for json module '
                   'to validate and pretty-print JSON objects.')
    parser = argparse.ArgumentParser(prog=prog, description=description)
    parser.add_argument('infile', nargs='?',
                        type=argparse.FileType(encoding="utf-8"),
                        help='a JSON file to be validated or pretty-printed',
                        default=sys.stdin)
    parser.add_argument('outfile', nargs='?',
                        type=str,
                        help='write the output of infile to outfile',
                        default=None)
    parser.add_argument('--sort-keys', action='store_true', default=False,
                        help='sort the output of dictionaries alphabetically by key')
    parser.add_argument('--no-ensure-ascii', dest='ensure_ascii', action='store_false',
                        help='disable escaping of non-ASCII characters')
    parser.add_argument('--json-lines', action='store_true', default=False,
                        help='parse input using the JSON Lines format. '
                        'Use with --no-indent or --compact to produce valid JSON Lines output.')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--indent', default=4, type=int,
                       help='separate items with newlines and use this number '
                       'of spaces for indentation')
    group.add_argument('--tab', action='store_const', dest='indent',
                       const='\t', help='separate items with newlines and use '
                       'tabs for indentation')
    group.add_argument('--no-indent', action='store_const', dest='indent',
                       const=None,
                       help='separate items with spaces rather than newlines')
    group.add_argument('--compact', action='store_true',
                       help='suppress all whitespace separation (most compact)')
    options = parser.parse_args()

    dump_args = {
        'sort_keys': options.sort_keys,
        'indent': options.indent,
        'ensure_ascii': options.ensure_ascii,
    }
    if options.compact:
        dump_args['indent'] = None
        dump_args['separators'] = ',', ':'

    with options.infile as infile:
        try:
            if options.json_lines:
                objs = (json.loads(line) for line in infile)
            else:
                objs = (json.load(infile),)

            if options.outfile is None or options.outfile == '-':
                # infile (argparse.FileType) supports '-' so outfile
                # supports '-' for consistency.
                out = sys.stdout
            else:
                out = _staged_outfile(options.outfile)
            with out as outfile:
                for obj in objs:
                    json.dump(obj, outfile, **dump_args)
                    outfile.write('\n')
        except ValueError as e:
            raise SystemExit(e)


if __name__ == '__main__':
    try:
        main()
    except BrokenPipeError as exc:
        sys.exit(exc.errno)
