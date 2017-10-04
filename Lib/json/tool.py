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

def parse_json_from_filehandle(infile, sort_keys):
    try:
        if sort_keys:
            return json.load(infile)
        else:
            return json.load(infile,
                            object_pairs_hook=collections.OrderedDict)
    except ValueError as e:
        raise SystemExit(e)

def parse_json_from_string(input, sort_keys):
    try:
        if sort_keys:
            return json.loads(input)
        else:
            return json.loads(input,
                            object_pairs_hook=collections.OrderedDict)
    except ValueError as e:
        raise SystemExit(e)

def main():
    prog = 'python -m json.tool'
    description = ('A simple command line interface for json module '
                   'to validate and pretty-print JSON objects.')
    parser = argparse.ArgumentParser(prog=prog, description=description)
    parser.add_argument('infile', nargs='?', type=argparse.FileType(),
                        help='a JSON file to be validated or pretty-printed')
    parser.add_argument('outfile', nargs='?', type=argparse.FileType('w'),
                        help='write the output of infile to outfile')
    parser.add_argument('--sort-keys', action='store_true', default=False,
                        help='sort the output of dictionaries alphabetically by key')
    parser.add_argument('--jsonlines', action='store_true', default=False,
                        help='parse input using the jsonlines format')
    options = parser.parse_args()

    infile = options.infile or sys.stdin
    outfile = options.outfile or sys.stdout
    sort_keys = options.sort_keys
    jsonlines = options.jsonlines
    objs = []

    if jsonlines:
        with infile:
            for line in infile:
                objs.append(parse_json_from_string(line, sort_keys))
    else:
        with infile:
            objs = [parse_json_from_filehandle(infile, sort_keys)]

    with outfile:
        for obj in objs:
            json.dump(obj, outfile, sort_keys=sort_keys, indent=4)
            outfile.write('\n')


if __name__ == '__main__':
    main()
