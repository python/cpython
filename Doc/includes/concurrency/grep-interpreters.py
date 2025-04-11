import os
import os.path
import re
import sys

import test.support.interpreters as interpreters
import test.support.interpreters.queues as interp_queues
import types
import queue
import threading


def search(filenames, regex, opts):
    matches_by_file = queue.Queue()

    def do_background():
        MAX_FILES = 10
        MAX_MATCHES = 100
        new_queue = interpreters.queues.create

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
            interp.prepare_main(matches=matches)
            interp.exec(
                    f'search_file({filename!r}, matches)')
            # Let a new thread start.
            ready_workers.put(interp)

        for filename in filenames:
            # Prepare for the file.
            matches = interp_queues.create(MAX_MATCHES)
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
        for match in search_lines(
                            lines, regex, opts, filename):
            matches.put(match)  # blocking
        matches.put(None)  # blocking
    return search_file


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
