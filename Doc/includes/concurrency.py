"""Example code for howto/concurrency.rst.

The examples take advantage of the literalinclude directive's
:start-after: and :end-before: options.
"""

import contextlib
import os
import tempfile
import sys


@contextlib.contextmanager
def dummy_files(*filenames):
    """A context manager that creates empty files in a temp directory."""
    with tempfile.TemporaryDirectory() as tempdir:
        orig = os.getcwd()
        os.chdir(tempdir)
        try:
            for filename in filenames:
                with open(filename, 'w') as outfile:
                    outfile.write(f'# {filename}\n')
            yield tempdir
        finally:
            os.chdir(orig)


try:
    zip((), (), strict=True)
except TypeError:
    def zip(*args, strict=False, _zip=zip):
        return _zip(*args)


class example(staticmethod):
    """A function containing example code.

    The function will be called when this file is run as a script.
    """

    registry = []

    def __init__(self, func):
        super().__init__(func)
        self.func = func

    def __set_name__(self, cls, name):
        assert name == self.func.__name__, (name, self.func.__name__)
        type(self).registry.append((self.func, cls))


class Examples:
    """Code examples for docs using "literalinclude"."""


class WorkloadExamples(Examples):
    """Examples of a single concurrency workload."""


#######################################
# concurrent.futures examples
#######################################

class ConcurrentFutures(Examples):

    @example
    def example_basic():
        with dummy_files('src1.txt', 'src2.txt', 'src3.txt', 'src4.txt'):
            # [start-cf-basic]
            import shutil
            from concurrent.futures import ThreadPoolExecutor as Executor

            with Executor() as e:
                # Copy 4 files concurrently.
                e.submit(shutil.copy, 'src1.txt', 'dest1.txt')
                e.submit(shutil.copy, 'src2.txt', 'dest2.txt')
                e.submit(shutil.copy, 'src3.txt', 'dest3.txt')
                e.submit(shutil.copy, 'src4.txt', 'dest4.txt')

                # Run a function asynchronously and check the result.
                fut = e.submit(pow, 323, 1235)
                res = fut.result()
                assert res == 323**1235
            # [end-cf-basic]

    @example
    def example_map():
        # [start-cf-map-1]
        from concurrent.futures import ThreadPoolExecutor as Executor

        pow_args = {
            323: 1235,
            100: 10,
            -1: 3,
        }
        for i in range(100):
            pow_args[i] = i

        with Executor() as e:
            # Run a function asynchronously and check the results.
            results = e.map(pow, pow_args.keys(), pow_args.values())
            for (a, n), res in zip(pow_args.items(), results):
                assert res == a**n
        # [end-cf-map-1]

        with dummy_files('src1.txt', 'src2.txt', 'src3.txt', 'src4.txt'):
            # [start-cf-map-2]
            import shutil
            from concurrent.futures import ThreadPoolExecutor as Executor

            # Copy files concurrently.

            files = {
                'src1.txt': 'dest1.txt',
                'src2.txt': 'dest2.txt',
                'src3.txt': 'dest3.txt',
                'src4.txt': 'dest4.txt',
            }

            with Executor() as e:
                copied = {}
                results = e.map(shutil.copy, files.keys(), files.values())
                for src, dest in zip(files, results, strict=True):
                    print(f'copied {src} to {dest}')
                    copied[src] = dest
                assert list(copied.values()) == list(files.values())
            # [end-cf-map-2]

    @example
    def example_wait():
        with dummy_files('src1.txt', 'src2.txt', 'src3.txt', 'src4.txt'):
            # [start-cf-wait]
            import shutil
            import concurrent.futures
            from concurrent.futures import ThreadPoolExecutor as Executor

            # Copy 4 files concurrently and wait for them all to finish.

            files = {
                'src1.txt': 'dest1.txt',
                'src2.txt': 'dest2.txt',
                'src3.txt': 'dest3.txt',
                'src4.txt': 'dest4.txt',
            }

            with Executor() as e:
                # Using wait():
                futures = [e.submit(shutil.copy, src, tgt)
                           for src, tgt in files.items()]
                concurrent.futures.wait(futures)

                # Using as_completed():
                futures = (e.submit(shutil.copy, src, tgt)
                           for src, tgt in files.items())
                list(concurrent.futures.as_completed(futures))

                # Using Executor.map():
                list(e.map(shutil.copy, files.keys(), files.values()))
            # [end-cf-wait]

    @example
    def example_as_completed():
        with dummy_files('src1.txt', 'src2.txt', 'src3.txt', 'src4.txt'):
            # [start-cf-as-completed]
            import shutil
            import concurrent.futures
            from concurrent.futures import ThreadPoolExecutor as Executor

            # Copy 4 files concurrently and handle each completion.

            files = {
                'src1.txt': 'dest1.txt',
                'src2.txt': 'dest2.txt',
                'src3.txt': 'dest3.txt',
                'src4.txt': 'dest4.txt',
            }

            with Executor() as e:
                copied = {}
                futures = (e.submit(shutil.copy, src, tgt)
                           for src, tgt in files.items())
                futures = dict(zip(futures, enumerate(files, 1)))
                for fut in concurrent.futures.as_completed(futures):
                    i, src = futures[fut]
                    res = fut.result()
                    print(f'({i}) {src} copied')
                    copied[src] = res
                assert set(copied.values()) == set(files.values()), (copied, files)
            # [end-cf-as-completed]

    @example
    def example_error_result():
        # [start-cf-error-result-1]
        import shutil
        import concurrent.futures
        from concurrent.futures import ThreadPoolExecutor as Executor

        # Run a function asynchronously and catch the error.
        def fail():
            raise Exception('spam!')
        with Executor() as e:
            fut = e.submit(fail)
            try:
                fut.result()
            except Exception as exc:
                arg, = exc.args
                assert arg == 'spam!'
        # [end-cf-error-result-1]

        with dummy_files('src1.txt', 'src2.txt', 'src3.txt', 'src4.txt'):
            # [start-cf-error-result-2]
            import shutil
            import concurrent.futures
            from concurrent.futures import ThreadPoolExecutor as Executor

            # Copy files concurrently, tracking missing files.

            files = {
                'src1.txt': 'dest1.txt',
                'src2.txt': 'dest2.txt',
                'src3.txt': 'dest3.txt',
                'src4.txt': 'dest4.txt',
                'missing.txt': 'dest5.txt',
            }

            with Executor() as e:
                # using executor.map():
                results = e.map(shutil.copy, files.keys(), files.values())
                for src in files:
                    try:
                        next(results)
                    except FileNotFoundError:
                        print(f'missing {src}')
                assert not list(results)

                # using wait():
                futures = [e.submit(shutil.copy, src, tgt)
                           for src, tgt in files.items()]
                futures = dict(zip(futures, files))
                completed, _ = concurrent.futures.wait(futures)
                for fut in completed:
                    src = futures[fut]
                    try:
                        fut.result()
                    except FileNotFoundError:
                        print(f'missing {src}')

                # using as_completed():
                futures = (e.submit(shutil.copy, src, tgt)
                           for src, tgt in files.items())
                futures = dict(zip(futures, files))
                for fut in concurrent.futures.as_completed(futures):
                    src = futures[fut]
                    try:
                        fut.result()
                    except FileNotFoundError:
                        print(f'missing {src}')
            # [end-cf-error-result-2]


#######################################
# workload: grep
#######################################

class Grep(WorkloadExamples):

    @staticmethod
    def common():
        # [start-grep-common]
        import os
        import os.path
        import re
        import sys
        import types

        class GrepOptions(types.SimpleNamespace):
            # file selection
            recursive = False      # -r --recursive
            # matching control
            ignorecase = False     # -i --ignore-case
            invertmatch = False    # -v --invert-match
            # output control
            showfilename = None    # -H --with-filename
                                   # -h --no-filename
            filesonly = None       # -L --files-without-match
                                   # -l --files-with-matches
            showonlymatch = False  # -o --only-matching
            quiet = False          # -q --quiet, --silent
            hideerrors = False     # -s --no-messages

        def normalize_file(unresolved, opts):
            if not isinstance(unresolved, str):
                infile, filename = unresolved
                if not filename:
                    filename = infile.name
                    yield infile, filename
                else:
                    assert not os.path.isdir(filename)
                    yield unresolved
            elif unresolved == '-':
                yield sys.stdin, '-'
            else:
                filename = unresolved
                if not opts.recursive:
                    assert not os.path.isdir(filename)
                    yield None, filename
                elif not os.path.isdir(filename):
                    yield None, filename
                else:
                    for d, _, files in os.walk(filename):
                        for base in files:
                            yield None, os.path.join(d, base)

        def iter_files(files, opts):
            for unresolved in files:
                yield from normalize_file(unresolved, opts)

        def _grep_file(regex, opts, infile, filename):
            invert = not opts.filesonly and opts.invertmatch
            if invert:
                for line in infile:
                    m = regex.search(line)
                    if m:
                        continue
                    if line.endswith(os.linesep):
                        line = line[:-len(os.linesep)]
                    yield filename, line, None
            else:
                for line in infile:
                    m = regex.search(line)
                    if not m:
                        continue
                    if line.endswith(os.linesep):
                        line = line[:-len(os.linesep)]
                    yield filename, line, m.group(0)

        def _grep(regex, opts, infile):
            infile, filename = infile
            if infile is None:
                with open(filename) as infile:
                    infile = (infile, filename)
                    yield from _grep(regex, opts, infile)
                return
            matches = _grep_file(regex, opts, infile, filename)
            try:
                if opts.filesonly == 'invert':
                    for _ in matches:
                        break
                    else:
                        yield filename, None, None
                elif opts.filesonly:
                    for filename, _, _ in matches:
                        yield filename, None, None
                        break
                else:
                    yield from matches
            except UnicodeDecodeError:
                # It must be a binary file.
                return

        def run_all(regex, opts, files, grep=_grep):
            for infile in files:
                yield from grep(regex, opts, infile)

        def main(pat, opts, *filenames, run_all=run_all):  # -e --regexp
            # Create the regex object.
            regex = re.compile(pat)

            # Resolve the files.
            files = iter_files(filenames, opts)

            # Process the files.
            matches = run_all(regex, opts, files, _grep)

            # Handle the first match.
            for filename, line, match in matches:
                if opts.quiet:
                    return 0
                elif opts.filesonly:
                    print(filename)
                elif opts.showonlymatch:
                    if opts.invertmatch:
                        return 0
                    elif opts.showfilename is False:
                        print(match)
                    elif opts.showfilename:
                        print(f'{filename}: {match}')
                    else:
                        try:
                            second = next(matches)
                        except StopIteration:
                            print(match)
                        else:
                            print(f'{filename}: {match}')
                            filename, _, match = second
                            print(f'{filename}: {match}')
                else:
                    if opts.showfilename is False:
                        print(line)
                    elif opts.showfilename:
                        print(f'{filename}: {line}')
                    else:
                        try:
                            second = next(matches)
                        except StopIteration:
                            print(line)
                        else:
                            print(f'{filename}: {line}')
                            filename, line, _ = second
                            print(f'{filename}: {line}')
                break
            else:
                return 1

            # Handle the remaining matches.
            if opts.filesonly:
                for filename, _, _ in matches:
                    print(filename)
            elif opts.showonlymatch:
                if opts.showfilename is False:
                    for filename, _, match in matches:
                        print(match)
                else:
                    for filename, _, match in matches:
                        print(f'{filename}: {match}')
            else:
                if opts.showfilename is False:
                    for filename, line, _ in matches:
                        print(line)
                else:
                    for filename, line, _ in matches:
                        print(f'{filename}: {line}')
            return 0
        # [end-grep-common]

        return main, GrepOptions

    @staticmethod
    def app(run_all):
        main, GrepOptions = Grep.common()
        opts = GrepOptions(
            #recursive=True,
            #ignorecase = True,
            #invertmatch = True,
            #showfilename = True,
            #showfilename = False,
            #filesonly = 'invert',
            #filesonly = 'match',
            #showonlymatch = True,
            #quiet = True,
            #hideerrors = True,
        )
        #main('help', opts, 'make.bat', 'Makefile', run_all=run_all)
        opts = GrepOptions(recursive=True, filesonly='match')
        main('help', opts, '.', run_all=run_all)

    @example
    def run_sequentially():
        # [start-grep-sequential]
        #
        #
        #

        #
        #

        #
        #
        #

        def run_all(regex, opts, files, grep):
            #

            #def manage_tasks():
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #

                #
                #
                #
                #
                #
                #

            for infile in files:
                yield from grep(regex, opts, infile)
        # [end-grep-sequential]
        Grep.app(run_all)


    @example
    def run_using_threads():
        # [start-grep-threads]
        import queue
        import threading
        #

        MAX_FILES = 10
        MAX_MATCHES = 100

        #
        #
        #

        def run_all(regex, opts, files, grep):
            matches_by_file = queue.Queue()

            def manage_tasks():
                #
                counter = threading.Semaphore(MAX_FILES)
                #
                #
                #

                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #

                def task(infile, matches):
                    for match in grep(regex, opts, infile):
                        matches.put(match)
                    matches.put(None)
                    # Let a new thread start.
                    counter.release()

                for infile in files:
                    _, filename = infile

                    # Prepare for the file.
                    matches = queue.Queue(MAX_MATCHES)
                    matches_by_file.put((filename, matches))

                    # Start a thread to process the file.
                    t = threading.Thread(
                        target=task,
                        args=(infile, matches),
                    )
                    counter.acquire(blocking=True)
                    #
                    t.start()
                matches_by_file.put(None)
                #
                #
                #
                #
                #
            t = threading.Thread(target=manage_tasks)
            t.start()

            # Yield the results as they are received, in order.
            next_matches = matches_by_file.get(block=True)
            while next_matches is not None:
                filename, matches = next_matches
                match = matches.get(block=True)
                while match is not None:
                    yield match
                    match = matches.get(block=True)
                next_matches = matches_by_file.get(block=True)

            t.join()
        # [end-grep-threads]
        Grep.app(run_all)

    @example
    def run_using_cf_threads():
        # [start-grep-cf-threads]
        import concurrent.futures
        import queue
        import threading

        MAX_FILES = 10
        MAX_MATCHES = 100

        # Alternately, swap in ProcessPoolExecutor
        # or InterpreterPoolExecutor.
        c_f_Executor = concurrent.futures.ThreadPoolExecutor

        def run_all(regex, opts, files, grep):
            matches_by_file = queue.Queue()

            def manage_tasks():
                threads = c_f_Executor(MAX_FILES)
                #
                #
                #
                #

                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #

                def task(infile, matches):
                    for match in grep(regex, opts, infile):
                        matches.put(match)
                    matches.put(None)
                    #
                    #

                for infile in files:
                    _, filename = infile

                    # Prepare for the file.
                    matches = queue.Queue(MAX_MATCHES)
                    matches_by_file.put((filename, matches))

                    # Start a thread to process the file.
                    threads.submit(task, infile, matches)
                    #
                    #
                    #
                    #
                    #
                    #
                matches_by_file.put(None)
                #
                #
                #
                #
                #
            t = threading.Thread(target=manage_tasks)
            t.start()

            # Yield the results as they are received, in order.
            next_matches = matches_by_file.get(block=True)
            while next_matches is not None:
                filename, matches = next_matches
                match = matches.get(block=True)
                while match is not None:
                    yield match
                    match = matches.get(block=True)
                next_matches = matches_by_file.get(block=True)

            t.join()
        # [end-grep-cf-threads]
        Grep.app(run_all)

    @example
    def run_using_subinterpreters():
        # [start-grep-subinterpreters]
        # subinterpreters 1
        ...
        # [end-grep-subinterpreters]

    @example
    def run_using_cf_subinterpreters():
        # [start-grep-cf-subinterpreters]
        # concurrent.futures 1
        ...
        # [end-grep-cf-subinterpreters]

    @example
    def run_using_async():
        # [start-grep-async]
        # async 1
        ...
        # [end-grep-async]

    @example
    def run_using_multiprocessing():
        # [start-grep-multiprocessing]
        import multiprocessing
        import queue
        import threading

        MAX_FILES = 10
        MAX_MATCHES = 100

        #
        #
        #

        def run_all(regex, opts, files, grep):
            matches_by_file = queue.Queue()

            def manage_tasks():
                #
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
                        # Let a new thread start.
                        counter.release()
                monitor = threading.Thread(target=monitor_tasks)
                monitor.start()

                def task(infile, index, matches, finished):
                    for match in grep(regex, opts, infile):
                        matches.put(match)
                    matches.put(None)
                    #
                    finished.put(index)

                for index, infile in enumerate(files):
                    _, filename = infile

                    # Prepare for the file.
                    matches = multiprocessing.Queue(MAX_MATCHES)
                    matches_by_file.put((filename, matches))

                    # Start a subprocess to process the file.
                    proc = multiprocessing.Process(
                        target=task,
                        args=(infile, index, matches, finished),
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
            t = threading.Thread(target=manage_tasks)
            t.start()

            # Yield the results as they are received, in order.
            next_matches = matches_by_file.get(block=True)
            while next_matches is not None:
                filename, matches = next_matches
                match = matches.get(block=True)
                while match is not None:
                    yield match
                    match = matches.get(block=True)
                next_matches = matches_by_file.get(block=True)

            t.join()
        # [end-grep-multiprocessing]
        Grep.app(run_all)

    @example
    def run_using_cf_multiprocessing():
        # [start-grep-cf-multiprocessing]
        import concurrent.futures
        import queue, multiprocessing
        import threading

        MAX_FILES = 10
        MAX_MATCHES = 100

        # Alternately, swap in ThreadPoolExecutor
        # or InterpreterPoolExecutor.
        c_f_Executor = concurrent.futures.ThreadPoolExecutor

        def run_all(regex, opts, files, grep):
            matches_by_file = queue.Queue()

            def manage_tasks():
                threads = c_f_Executor(MAX_FILES)
                #
                #
                #
                #

                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #
                #

                def task(infile, matches):
                    for match in grep(regex, opts, infile):
                        matches.put(match)
                    matches.put(None)
                    #
                    #

                for infile in files:
                    _, filename = infile

                    # Prepare for the file.
                    matches = multiprocessing.Queue(MAX_MATCHES)
                    matches_by_file.put((filename, matches))

                    # Start a thread to process the file.
                    threads.submit(task, infile, matches)
                    #
                    #
                    #
                    #
                    #
                    #
                matches_by_file.put(None)
                #
                #
                #
                #
                #
            t = threading.Thread(target=manage_tasks)
            t.start()

            # Yield the results as they are received, in order.
            next_matches = matches_by_file.get(block=True)
            while next_matches is not None:
                filename, matches = next_matches
                match = matches.get(block=True)
                while match is not None:
                    yield match
                    match = matches.get(block=True)
                next_matches = matches_by_file.get(block=True)

            t.join()
        # [end-grep-cf-multiprocessing]
        Grep.app(run_all)

    @example
    def run_using_dask():
        # [start-grep-dask]
        # dask 1
        ...
        # [end-grep-dask]


#######################################
# workload: image resizer
#######################################

class ImageResizer(WorkloadExamples):

    @example
    def run_sequentially():
        # [start-image-resizer-sequential]
        # sequential 2
        ...
        # [end-image-resizer-sequential]

    @example
    def run_using_threads():
        # [start-image-resizer-threads]
        import threading

        def task():
            ...

        t = threading.Thread(target=task)
        t.start()

        ...
        # [end-image-resizer-threads]

    @example
    def run_using_cf_thread():
        # [start-image-resizer-cf-thread]
        # concurrent.futures 2
        ...
        # [end-image-resizer-cf-thread]

    @example
    def run_using_subinterpreters():
        # [start-image-resizer-subinterpreters]
        # subinterpreters 2
        ...
        # [end-image-resizer-subinterpreters]

    @example
    def run_using_cf_subinterpreters():
        # [start-image-resizer-cf-subinterpreters]
        # concurrent.futures 2
        ...
        # [end-image-resizer-cf-subinterpreters]

    @example
    def run_using_async():
        # [start-image-resizer-async]
        # async 2
        ...
        # [end-image-resizer-async]

    @example
    def run_using_multiprocessing():
        # [start-image-resizer-multiprocessing]
        import multiprocessing

        def task():
            ...

        ...
        # [end-image-resizer-multiprocessing]

    @example
    def run_using_cf_multiprocessing():
        # [start-image-resizer-cf-multiprocessing]
        # concurrent.futures 2
        ...
        # [end-image-resizer-cf-multiprocessing]

    @example
    def run_using_dask():
        # [start-image-resizer-dask]
        # dask 2
        ...
        # [end-image-resizer-dask]


#######################################
# workload: ...
#######################################

class WorkloadX(WorkloadExamples):

    @example
    def run_sequentially():
        # [start-w3-sequential]
        # sequential 3
        ...
        # [end-w3-sequential]

    @example
    def run_using_threads():
        # [start-w3-threads]
        import threading

        def task():
            ...

        t = threading.Thread(target=task)
        t.start()

        ...
        # [end-w3-threads]

    @example
    def run_using_cf_thread():
        # [start-w3-cf-thread]
        # concurrent.futures 3
        ...
        # [end-w3-cf-thread]

    @example
    def run_using_subinterpreters():
        # [start-w3-subinterpreters]
        # subinterpreters 3
        ...
        # [end-w3-subinterpreters]

    @example
    def run_using_cf_subinterpreters():
        # [start-w3-cf-subinterpreters]
        # concurrent.futures 3
        ...
        # [end-w3-cf-subinterpreters]

    @example
    def run_using_async():
        # [start-w3-async]
        # async 3
        ...
        # [end-w3-async]

    @example
    def run_using_multiprocessing():
        # [start-w3-multiprocessing]
        import multiprocessing

        def task():
            ...

        ...
        # [end-w3-multiprocessing]

    @example
    def run_using_cf_multiprocessing():
        # [start-w3-cf-multiprocessing]
        # concurrent.futures 3
        ...
        # [end-w3-cf-multiprocessing]

    @example
    def run_using_dask():
        # [start-w3-dask]
        # dask 3
        ...
        # [end-w3-dask]


#######################################
# A script to run the examples
#######################################

if __name__ == '__main__':
    # Run (all) the examples.
    argv = sys.argv[1:]
    if argv:
        classname, _, funcname = argv[0].rpartition('.')
        requested = (classname, funcname)
    else:
        requested = None

    div1 = '#' * 40
    div2 = '#' + '-' * 39
    last = None
    for func, cls in example.registry:
        if requested:
            classname, funcname = requested
            if classname:
                if cls.__name__ != classname:
                    continue
                if func.__name__ != funcname:
                    continue
            else:
                if func.__name__ != funcname:
                    if cls.__name__ != funcname:
                        continue
        print()
        if cls is not last:
            last = cls
            print(div1)
            print(f'# {cls.__name__}')
            print(div1)
            print()
        print(div2)
        print(f'# {func.__name__}')
        print(div2)
        print()
        try:
            func()
        except Exception:
            import traceback
            traceback.print_exc()
