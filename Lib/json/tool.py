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
        return sys.stdout if outfile == '-' else open(outfile, 'w')
    except IOError as e:
        parser.error(f"can't open '{outfile}': {str(e)}")

def _write(parser, outfile, objs, sort_keys):
    for obj in objs:
        json.dump(obj, outfile, sort_keys=sort_keys, indent=4)
        outfile.write('\n')

def main():
    prog = 'python -m json.tool'
    description = ('A simple command line interface for json module '
                   'to validate and pretty-print JSON objects.')
    parser = argparse.ArgumentParser(prog=prog, description=description)
    parser.add_argument('infile', nargs='?', type=argparse.FileType(), default=sys.stdin,
                        help='a JSON file to be validated or pretty-printed')
    parser.add_argument('outfile', nargs='?', default='-',
                        help='write the output of infile to outfile')
    parser.add_argument('--sort-keys', action='store_true', default=False,
                        help='sort the output of dictionaries alphabetically by key')
    parser.add_argument('--json-lines', action='store_true', default=False,
                        help='parse input using the jsonlines format')
    parser.add_argument('-i', '--in-place', action='store_true', default=False,
                        help='edit the file in-place')
    options = parser.parse_args()

    if options.in_place:
        if options.outfile != '-':
            parser.error('outfile cannot be set when -i / --in-place is used')
        if options.infile is sys.stdin:
            parser.error('infile must be set when -i / --in-place is used')
        options.outfile = options.infile.name

    outfile = options.outfile
    infile = options.infile
    sort_keys = options.sort_keys
    json_lines = options.json_lines

    if options.in_place:
        with infile:
            objs = tuple(_read(infile, json_lines))

        outfile = _open_outfile(outfile, parser)

        with outfile:
            _write(parser, outfile, objs, sort_keys)

    else:
        outfile = _open_outfile(outfile, parser)
        with infile, outfile:
            objs = _read(infile, json_lines)
            _write(parser, outfile, objs, sort_keys)

if __name__ == '__main__':
    main()
