r"""Command-line tool to validate and pretty-print JSON

Usage::

    $ echo '{"json":"obj"}' | python -mjson.tool
    {
        "json": "obj"
    }
    $ echo '{ 1.2:3.4}' | python -mjson.tool
    Expecting property name: line 1 column 2 (char 2)
"""
import sys
import json

def main():
    if len(sys.argv) == 1:
        infile = sys.stdin
        outfile = sys.stdout
    elif len(sys.argv) == 2:
        infile = open(sys.argv[1], 'rb')
        outfile = sys.stdout
    elif len(sys.argv) == 3:
        infile = open(sys.argv[1], 'rb')
        outfile = open(sys.argv[2], 'wb')
    else:
        raise SystemExit("{0} [infile [outfile]]".format(sys.argv[0]))
    try:
        obj = json.load(infile)
    except ValueError, e:
        raise SystemExit(e)
    json.dump(obj, outfile, sort_keys=True, indent=4)
    outfile.write('\n')


if __name__ == '__main__':
    main()
