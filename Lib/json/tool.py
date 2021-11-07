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
import json
import sys

def _read(infile, json_lines):
    try:
        if json_lines:
            return (json.loads(line) for line in infile)
        else:
            return (json.load(infile), )
    except ValueError as e:
        raise SystemExit(e)

def _open_outfile(outfile, parser):
    try:
        if outfile == '-':
            return sys.stdout
        else:
            return open(outfile, 'w', encoding='utf-8')
    except IOError as e:
        parser.error(f"can't open '{outfile}': {str(e)}")

def _write(outfile, objs, **kwargs):
    for obj in objs:
        json.dump(obj, outfile, **kwargs)
        outfile.write('\n')

def main():
    prog = 'python -m json.tool'
    description = ('A simple command line interface for json module '
                   'to validate and pretty-print JSON objects.')
    parser = argparse.ArgumentParser(prog=prog, description=description)
    parser.add_argument('infile', nargs='?',
                        type=argparse.FileType(encoding="utf-8"),
                        help='a JSON file to be validated or pretty-printed',
                        default=sys.stdin)
    parser.add_argument('outfile', nargs='?', default='-',
                        help='write the output of infile to outfile')
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
    parser.add_argument('-i', '--in-place', action='store_true', default=False,
                        help='edit the file in-place')
    options = parser.parse_args()


    if options.in_place:
        if options.outfile != '-':
            parser.error('outfile cannot be set when -i / --in-place is used')
        if options.infile is sys.stdin:
            parser.error('infile must be set when -i / --in-place is used')
        options.outfile = options.infile.name

    dump_args = {
        'sort_keys': options.sort_keys,
        'indent': options.indent,
        'ensure_ascii': options.ensure_ascii,
    }
    if options.compact:
        dump_args['indent'] = None
        dump_args['separators'] = ',', ':'

    if options.in_place:
        with options.infile as infile:
            objs = tuple(_read(infile, options.json_lines))

        with _open_outfile(options.outfile, parser) as outfile:
            _write(outfile, objs, **dump_args)

    else:
        outfile = _open_outfile(options.outfile, parser)
        with options.infile as infile, outfile:
            objs = _read(infile, options.json_lines)
            _write(outfile, objs, **dump_args)

if __name__ == '__main__':
    try:
        main()
    except BrokenPipeError as exc:
        sys.exit(exc.errno)
