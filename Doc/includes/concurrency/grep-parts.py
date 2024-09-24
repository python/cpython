import os
import os.path
import re
import sys

import asyncio
from concurrent.futures import ThreadPoolExecutor, ProcessPoolExecutor
import test.support.interpreters as interpreters
import test.support.interpreters.queues
import multiprocessing
import queue
import threading
import types


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


# [start-search-lines]
def search_lines(lines, regex, opts, filename):
    try:
        if opts.filesonly:
            if opts.invert:
                for line in lines:
                    m = regex.search(line)
                    if m:
                        break
                else:
                    yield (filename, None)
            else:
                for line in lines:
                    m = regex.search(line)
                    if m:
                        yield (filename, None)
                        break
        else:
            assert not opts.invert, opts
            for line in lines:
                m = regex.search(line)
                if not m:
                    continue
                if line.endswith(os.linesep):
                    line = line[:-len(os.linesep)]
                yield (filename, line)
    except UnicodeDecodeError:
        # It must be a binary file.
        return
# [end-search-lines]


async def asearch_lines(lines, regex, opts, filename):
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


#######################################
# implemetations

# [start-impl-sequential]
def search_sequential(filenames, regex, opts):
    for filename in filenames:
        lines = iter_lines(filename)
        yield from search_lines(lines, regex, opts, filename)
# [end-impl-sequential]


# [start-impl-threads]
def search_using_threads(filenames, regex, opts):
    matches_by_file = queue.Queue()

    def do_background():
        MAX_FILES = 10
        MAX_MATCHES = 100

        # Make sure we don't have too many threads at once,
        # i.e. too many files open at once.
        counter = threading.Semaphore(MAX_FILES)

        def search_file(filename, matches):
            lines = iter_lines(filename)
            for match in search_lines(lines, regex, opts, filename):
                matches.put(match)  # blocking
            matches.put(None)  # blocking
            # Let a new thread start.
            counter.release()

        for filename in filenames:
            # Prepare for the file.
            matches = queue.Queue(MAX_MATCHES)
            matches_by_file.put(matches)

            # Start a thread to process the file.
            t = threading.Thread(target=search_file, args=(filename, matches))
            counter.acquire()
            t.start()
        matches_by_file.put(None)

    background = threading.Thread(target=do_background)
    background.start()

    # Yield the results as they are received, in order.
    matches = matches_by_file.get()  # blocking
    while matches is not None:
        match = matches.get()  # blocking
        while match is not None:
            yield match
            match = matches.get()  # blocking
        matches = matches_by_file.get()  # blocking

    background.join()
# [end-impl-threads]


# [start-impl-cf-threads]
def search_using_threads_cf(filenames, regex, opts):
    matches_by_file = queue.Queue()

    def do_background():
        MAX_FILES = 10
        MAX_MATCHES = 100

        def search_file(filename, matches):
            lines = iter_lines(filename)
            for match in search_lines(lines, regex, opts, filename):
                matches.put(match)  # blocking
            matches.put(None)  # blocking

        with ThreadPoolExecutor(MAX_FILES) as workers:
            for filename in filenames:
                # Prepare for the file.
                matches = queue.Queue(MAX_MATCHES)
                matches_by_file.put(matches)

                # Start a thread to process the file.
                workers.submit(search_file, filename, matches)
            matches_by_file.put(None)

    background = threading.Thread(target=do_background)
    background.start()

    # Yield the results as they are received, in order.
    matches = matches_by_file.get()  # blocking
    while matches is not None:
        match = matches.get()  # blocking
        while match is not None:
            yield match
            match = matches.get()  # blocking
        matches = matches_by_file.get()  # blocking

    background.join()
# [end-impl-cf-threads]


def search_using_interpreters(filenames, regex, opts):
    matches_by_file = queue.Queue()

    def do_background():
        MAX_FILES = 10
        MAX_MATCHES = 100

        def new_interpreter():
            interp = interpreters.create()
            interp.exec(f"""if True:
                with open({__file__!r}) as infile:
                    text = infile.read()
                ns = dict()
                exec(text, ns, ns)
                prep_interpreter = ns['prep_interpreter']
                del ns, text

                search_file = prep_interpreter(
                    {regex.pattern!r},
                    {regex.flags},
                    {tuple(vars(opts).items())},
                )
                """)
            return interp

        ready_workers = queue.Queue(MAX_FILES)
        workers = []

        def next_worker():
            if len(workers) < MAX_FILES:
                interp = new_interpreter()
                workers.append(interp)
                ready_workers.put(interp)
            return ready_workers.get()  # blocking

        def do_work(filename, matches, interp):
            #interp.call(search_file, (regex, opts, filename, matches))
            interp.prepare_main(matches=matches)
            interp.exec(f'search_file({filename!r}, matches)')
            # Let a new thread start.
            ready_workers.put(interp)

        for filename in filenames:
            # Prepare for the file.
            matches = interpreters.queues.create(MAX_MATCHES)
            matches_by_file.put(matches)
            interp = next_worker()

            # Start a thread to process the file.
            t = threading.Thread(
                target=do_work,
                args=(filename, matches, interp),
            )
            t.start()
        matches_by_file.put(None)

    background = threading.Thread(target=do_background)
    background.start()

    # Yield the results as they are received, in order.
    matches = matches_by_file.get()  # blocking
    while matches is not None:
        match = matches.get()  # blocking
        while match is not None:
            yield match
            match = matches.get()  # blocking
        matches = matches_by_file.get()  # blocking

    background.join()


def prep_interpreter(regex_pat, regex_flags, opts):
    regex = re.compile(regex_pat, regex_flags)
    opts = types.SimpleNamespace(**dict(opts))

    def search_file(filename, matches):
        lines = iter_lines(filename)
        for match in search_lines(lines, regex, opts, filename):
            matches.put(match)  # blocking
        matches.put(None)  # blocking
    return search_file


async def search_using_asyncio(filenames, regex, opts):
    matches_by_file = asyncio.Queue()

    async def do_background():
        MAX_FILES = 10
        MAX_MATCHES = 100

        # Make sure we don't have too many coroutines at once,
        # i.e. too many files open at once.
        counter = asyncio.Semaphore(MAX_FILES)

        async def search_file(filename, matches):
            # aiter_lines() opens the file too.
            lines = aiter_lines(filename)
            async for match in asearch_lines(lines, regex, opts, filename):
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


def search_using_multiprocessing(filenames, regex, opts):
    matches_by_file = queue.Queue()

    def do_background():
        MAX_FILES = 10
        MAX_MATCHES = 100

        # Make sure we don't have too many processes at once,
        # i.e. too many files open at once.
        counter = threading.Semaphore(MAX_FILES)
        finished = multiprocessing.Queue()
        active = {}
        done = False

        def monitor_tasks():
            while not done:
                try:
                    index = finished.get(timeout=0.1)
                except queue.Empty:
                    continue
                proc = active.pop(index)
                proc.join(0.1)
                if proc.is_alive():
                    # It's taking too long to terminate.
                    # We can wait for it at the end.
                    active[index] = proc
                # Let a new process start.
                counter.release()
        monitor = threading.Thread(target=monitor_tasks)
        monitor.start()

        for index, filename in enumerate(filenames):
            # Prepare for the file.
            matches = multiprocessing.Queue(MAX_MATCHES)
            matches_by_file.put(matches)

            # Start a subprocess to process the file.
            proc = multiprocessing.Process(
                target=search_file,
                args=(filename, matches, regex, opts, index, finished),
            )
            counter.acquire(blocking=True)
            active[index] = proc
            proc.start()
        matches_by_file.put(None)
        # Wait for all remaining tasks to finish.
        done = True
        monitor.join()
        for proc in active.values():
            proc.join()

    background = threading.Thread(target=do_background)
    background.start()

    # Yield the results as they are received, in order.
    matches = matches_by_file.get()  # blocking
    while matches is not None:
        match = matches.get()  # blocking
        while match is not None:
            yield match
            match = matches.get()  # blocking
        matches = matches_by_file.get()  # blocking

    background.join()


def search_file(filename, matches, regex, opts, index, finished):
    lines = iter_lines(filename)
    for match in search_lines(lines, regex, opts, filename):
        matches.put(match)  # blocking
    matches.put(None)  # blocking
    # Let a new process start.
    finished.put(index)


IMPLS = {
    'sequential': search_sequential,
    'threads': search_using_threads,
    'threads-cf': search_using_threads_cf,
    'interpreters': search_using_interpreters,
    'asyncio': search_using_asyncio,
    'multiprocessing': search_using_multiprocessing,
}


#######################################
# the script

if __name__ == '__main__':
    multiprocessing.set_start_method('spawn')

    # Parse the args.
    import argparse
    parser = argparse.ArgumentParser(prog='grep')

    parser.add_argument('--impl', choices=sorted(IMPLS), default='threads')

    parser.add_argument('-r', '--recursive', action='store_true')
    parser.add_argument('-L', '--files-without-match', dest='filesonly',
                        action='store_const', const='invert')
    parser.add_argument('-l', '--files-with-matches', dest='filesonly',
                        action='store_const', const='match')
    parser.add_argument('-q', '--quiet', action='store_true')
    parser.set_defaults(invert=False)

    regexopts = parser.add_mutually_exclusive_group(required=True)
    regexopts.add_argument('-e', '--regexp', dest='regex', metavar='REGEX')
    regexopts.add_argument('regex', nargs='?', metavar='REGEX')

    parser.add_argument('files', nargs='+', metavar='FILE')

    opts = parser.parse_args()
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

    search = IMPLS[ns.pop('impl')]

# [start-high-level]
    def main(regex=regex, filenames=filenames):
        # step 1
        regex = re.compile(regex)
        # step 2
        filenames = resolve_filenames(filenames, recursive)
        # step 3
        matches = search(filenames, regex, opts)

        # step 4

        if hasattr(type(matches), '__aiter__'):
            async def iter_and_show(matches=matches):
                matches = type(matches).__aiter__(matches)

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
            return asyncio.run(search_and_show())
        else:
            matches = iter(matches)

            # Handle the first match.
            for filename, line in matches:
                if opts.quiet:
                    return 0
                elif opts.filesonly:
                    print(filename)
                else:
                    for second in matches:
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
                for filename, _ in matches:
                    print(filename)
            else:
                for filename, line in matches:
                    print(f'{filename}: {line}')

            return 0
    rc = main()

    # step 5
    sys.exit(rc)
# [end-high-level]
