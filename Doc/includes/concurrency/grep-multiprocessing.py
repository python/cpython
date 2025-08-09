import os
import os.path
import re
import sys

import multiprocessing
import queue
import threading


def search(filenames, regex, opts):
    matches_by_file = queue.Queue()

    def do_background():
        MAX_FILES = 10
        MAX_MATCHES = 100

        # Make sure we don't have too many procs at once,
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
                args=(filename, matches, regex, opts,
                      index, finished),
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


def search_file(filename, matches, regex, opts,
                index, finished):
    lines = iter_lines(filename)
    for match in search_lines(lines, regex, opts, filename):
        matches.put(match)  # blocking
    matches.put(None)  # blocking
    # Let a new process start.
    finished.put(index)


def iter_lines(filename):
    if filename == '-':
        yield from sys.stdin
    else:
        with open(filename) as infile:
            yield from infile


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
    multiprocessing.set_start_method('spawn')

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

    def main(regex=regex, filenames=filenames):
        # step 1
        regex = re.compile(regex)
        # step 2
        filenames = resolve_filenames(filenames, recursive)
        # step 3
        matches = search(filenames, regex, opts)
        matches = iter(matches)

        # step 4

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
