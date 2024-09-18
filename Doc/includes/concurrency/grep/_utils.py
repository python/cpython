import os.path
import re
import sys


def resolve_regex(regex, opts):
    if isinstance(regex, str):
        regex = re.compile(regex)
    return regex


def resolve_filenames(filenames, opts):
    for filename in filenames:
        assert isinstance(filename, str), repr(filename)
        if filename == '-':
            yield '-'
        elif not opts.recursive:
            assert not os.path.isdir(filename)
            yield filename
        elif not os.path.isdir(filename):
            yield filename
        else:
            for d, _, files in os.walk(filename):
                for base in files:
                    yield os.path.join(d, base)


def iter_lines(filename):
    if filename == '-':
        yield from sys.stdin
    else:
        with open(filename) as infile:
            yield from infile
