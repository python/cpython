import os
import os.path
import re
import sys

import asyncio


async def search(filenames, regex, opts):
    matches_by_file = asyncio.Queue()

    async def do_background():
        MAX_FILES = 10
        MAX_MATCHES = 100

        # Make sure we don't have too many coros at once,
        # i.e. too many files open at once.
        counter = asyncio.Semaphore(MAX_FILES)

        async def search_file(filename, matches):
            # aiter_lines() opens the file too.
            lines = iter_lines(filename)
            async for m in search_lines(
                            lines, regex, opts, filename):
                await matches.put(match)
            await matches.put(None)
            # Let a new coroutine start.
            counter.release()

        async with asyncio.TaskGroup() as tg:
            for filename in filenames:
                # Prepare for the file.
                matches = asyncio.Queue(MAX_MATCHES)
                await matches_by_file.put(matches)

                # Start a coroutine to process the file.
                tg.create_task(
                    search_file(filename, matches),
                )
                await counter.acquire()
            await matches_by_file.put(None)

    background = asyncio.create_task(do_background())

    # Yield the results as they are received, in order.
    matches = await matches_by_file.get()  # blocking
    while matches is not None:
        match = await matches.get()  # blocking
        while match is not None:
            yield match
            match = await matches.get()  # blocking
        matches = await matches_by_file.get()  # blocking

    await asyncio.wait([background])


async def iter_lines(filename):
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
    # maybe make use of asyncio.to_thread()
    # or loop.run_in_executor()?
    return infile.readline()


async def search_lines(lines, regex, opts, filename):
    try:
        if opts.filesonly:
            if opts.invert:
                async for line in lines:
                    m = regex.search(line)
                    if m:
                        break
                else:
                    yield (filename, None)
            else:
                async for line in lines:
                    m = regex.search(line)
                    if m:
                        yield (filename, None)
                        break
        else:
            assert not opts.invert, opts
            async for line in lines:
                m = regex.search(line)
                if not m:
                    continue
                if line.endswith(os.linesep):
                    line = line[:-len(os.linesep)]
                yield (filename, line)
    except UnicodeDecodeError:
        # It must be a binary file.
        return


def resolve_filenames(filenames, recursive=False):
    for filename in filenames:
        assert isinstance(filename, str), repr(filename)
        if filename == '-':
            yield '-'
        elif not os.path.isdir(filename):
            yield filename
        elif recursive:
            for d, _, files in os.walk(filename):
                for base in files:
                    yield os.path.join(d, base)


if __name__ == '__main__':
    # Parse the args.
    import argparse
    ap = argparse.ArgumentParser(prog='grep')

    ap.add_argument('-r', '--recursive',
                    action='store_true')
    ap.add_argument('-L', '--files-without-match',
                    dest='filesonly',
                    action='store_const', const='invert')
    ap.add_argument('-l', '--files-with-matches',
                    dest='filesonly',
                    action='store_const', const='match')
    ap.add_argument('-q', '--quiet', action='store_true')
    ap.set_defaults(invert=False)

    reopts = ap.add_mutually_exclusive_group(required=True)
    reopts.add_argument('-e', '--regexp', dest='regex',
                        metavar='REGEX')
    reopts.add_argument('regex', nargs='?',
                        metavar='REGEX')

    ap.add_argument('files', nargs='+', metavar='FILE')

    opts = ap.parse_args()
    ns = vars(opts)

    regex = ns.pop('regex')
    filenames = ns.pop('files')
    recursive = ns.pop('recursive')
    if opts.filesonly:
        if opts.filesonly == 'invert':
            opts.invert = True
        else:
            assert opts.filesonly == 'match', opts
            opts.invert = False
    opts.filesonly = bool(opts.filesonly)

    async def main(regex=regex, filenames=filenames):
        # step 1
        regex = re.compile(regex)
        # step 2
        filenames = resolve_filenames(filenames, recursive)
        # step 3
        matches = search(filenames, regex, opts)
        matches = type(matches).__aiter__(matches)

        # step 4

        # Handle the first match.
        async for filename, line in matches:
            if opts.quiet:
                return 0
            elif opts.filesonly:
                print(filename)
            else:
                async for second in matches:
                    print(f'{filename}: {line}')
                    filename, line = second
                    print(f'{filename}: {line}')
                    break
                else:
                    print(line)
            break
        else:
            return 1

        # Handle the remaining matches.
        if opts.filesonly:
            async for filename, _ in matches:
                print(filename)
        else:
            async for filename, line in matches:
                print(f'{filename}: {line}')

        return 0
    rc = asyncio.run(main())

    # step 5
    sys.exit(rc)
