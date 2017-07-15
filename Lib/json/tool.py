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
import collections
import json
import sys


def main():
    prog = 'python -m json.tool'
    description = ('A simple command line interface for json module '
                   'to validate and pretty-print JSON objects.')
    parser = argparse.ArgumentParser(prog=prog, description=description)
    parser.add_argument('infile', nargs='?', type=argparse.FileType(),
                        help='a JSON file to be validated or pretty-printed')
    parser.add_argument('outfile', nargs='?', type=argparse.FileType('w'),
                        help='write the output of infile to outfile')
    parser.add_argument('--sort-keys', action='store_true',
                        help='sort dictionary outputs alphabetically by key')
    options = parser.parse_args()

    # Read input JSON
    infile = options.infile or sys.stdin
    sort_keys = options.sort_keys
    hook = collections.OrderedDict if sort_keys else None
    with infile:
        try:
            obj = json.load(infile, object_pairs_hook=hook)
        except ValueError as e:
            raise SystemExit(e)

    # Output JSON
    outfile = options.outfile or sys.stdout
    with outfile:
        json.dump(obj, outfile, sort_keys=sort_keys, indent=4)
        outfile.write('\n')


if __name__ == '__main__':
    main()
