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


async def aiter_lines(filename):
    if filename == '-':
        infile = sys.stdin
        line = await read_line_async(infile)
        while line:
            yield line
            line = await read_line_async(infile)
    else:
        # XXX Open using async?
        with open(filename) as infile:
            line = await read_line_async(infile)
            while line:
                yield line
                line = await read_line_async(infile)


async def read_line_async(infile):
    # XXX Do this async!
    # maybe make use of asyncio.to_thread() or loop.run_in_executor()?
    return infile.readline()
