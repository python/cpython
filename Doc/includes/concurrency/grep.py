from collections import namedtuple
import contextlib
import os
import os.path
import re
import sys
import types

# For concurrency:
import asyncio
import concurrent.futures
import test.support.interpreters as interpreters
import test.support.interpreters.queues
import multiprocessing
import queue
import threading


class Match(namedtuple('Match', 'filename line match')):

    @classmethod
    def from_re_match(cls, m, line, filename):
        if line.endswith(os.linesep):
            line = line[:-len(os.linesep)]
        return cls(filename, line, m.group(0))

    @classmethod
    def from_inverted_match(cls, line, filename):
        if line.endswith(os.linesep):
            line = line[:-len(os.linesep)]
        return cls(filename, line, None)

    def __new__(cls, filename, line=None, match=None):
        return super().__new__(cls, filename or '???', line, match)


# [start-search-lines]
def search_lines(lines, regex, opts, filename):
    try:
        if opts.filesonly == 'invert':
            for line in lines:
                m = regex.search(line)
                if m:
                    break
            else:
                yield Match(filename)
        elif opts.filesonly:
            for line in lines:
                m = regex.search(line)
                if m:
                    yield Match(filename)
                    break
        elif opts.invertmatch:
            for line in lines:
                m = regex.search(line)
                if m:
                    continue
                yield Match.from_inverted_match(line, filename)
        else:
            for line in lines:
                m = regex.search(line)
                if not m:
                    continue
                yield Match.from_re_match(m, line, filename)
    except UnicodeDecodeError:
        # It must be a binary file.
        return
# [end-search-lines]


def render_matches(matches, opts):
    matches = iter(matches)

    # Handle the first match.
    blank = False
    for filename, line, match in matches:
        if opts.quiet:
            blank = True
        elif opts.filesonly:
            yield filename
        elif opts.showonlymatch:
            if opts.invertmatch:
                blank = True
            elif opts.showfilename is False:
                yield match
            elif opts.showfilename:
                yield f'{filename}: {match}'
            else:
                try:
                    second = next(matches)
                except StopIteration:
                    yield match
                else:
                    yield f'{filename}: {match}'
                    filename, _, match = second
                    yield f'{filename}: {match}'
        else:
            if opts.showfilename is False:
                yield line
            elif opts.showfilename:
                yield f'{filename}: {line}'
            else:
                try:
                    second = next(matches)
                except StopIteration:
                    yield line
                else:
                    yield f'{filename}: {line}'
                    filename, line, _ = second
                    yield f'{filename}: {line}'
        break

    if blank:
        return

    # Handle the remaining matches.
    if opts.filesonly:
        for filename, _, _ in matches:
            yield filename
    elif opts.showonlymatch:
        if opts.showfilename is False:
            for filename, _, match in matches:
                yield match
        else:
            for filename, _, match in matches:
                yield f'{filename}: {match}'
    else:
        if opts.showfilename is False:
            for filename, line, _ in matches:
                yield line
        else:
            for filename, line, _ in matches:
                yield f'{filename}: {line}'


#######################################
# files

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


#######################################
# options

class Options(types.SimpleNamespace):

    CLI_BY_ATTR = {
        # file selection
        'recursive': [('-r', '--recursive', True)],
        # matching control
        'ignorecase': [('-i', '--ignore-case', True)],
        'invertmatch': [('-v', '--invert-match', True)],
        # output control
        'showfilename': [('-H', '--with-filename', True),
                          ('-h', '--no-filename', False)],
        'filesonly': [('-L', '--files-without-match', 'invert'),
                      ('-l', '--files-with-matches', 'match')],
        'showonlymatch': [('-o', '--only-matching', True)],
        'quiet': [('-q', '--quiet', '--silent', True)],
        'hideerrors': [('-s', '--no-messages', True)],
    }

    @classmethod
    def add_cli(cls, parser, dest='grepopts'):
        assert dest

        import argparse
        class GrepOptsAction(argparse.Action):
            DEST = dest
            def __init__(self,
                         option_strings,
                         dest,
                         default=cls(),
                         nargs=0,
                         **kwargs):
                kwargs['default'] = default
                assert nargs == 0, nargs
                kwargs['nargs'] = nargs
                super().__init__(option_strings, self.DEST, **kwargs)
                self.grepoptsattr = dest
            def __call__(self, parser, namespace, values, option_string=None):
                setattr(self.default, self.grepoptsattr, self.const)

        for attr, opts in cls.CLI_BY_ATTR.items():
            for opt in opts:
                parser.add_argument(*opt[:-1], dest=attr,
                                    action=GrepOptsAction, const=opt[-1])

    def __init__(self, *,
                 recursive=False,
                 ignorecase=False,
                 invertmatch=False,
                 showfilename=None,
                 filesonly=None,
                 showonlymatch=False,
                 quiet=False,
                 hideerrors=False,
                 ):
        self.__dict__.update(locals())
        del self.self

    def __str__(self):
        import shlex
        return shlex.join(self.as_argv())

    def as_argv(self, *, short=True):
        argv = []
        for attr, opts in self.CLI_BY_ATTR.items():
            value = getattr(self, attr)
            for opt in opts:
                if value == opt[-1]:
                    argv.append(opt[0] if short else opt[1])
                    break
        return argv


#######################################
# basic search implementations

# [start-impl-sequential]
def search_sequential(filenames, regex, opts):
    for filename in filenames:
        # iter_lines() opens the file too.
        lines = iter_lines(filename)
        yield from search_lines(lines, regex, opts, filename)
# [end-impl-sequential]


# [start-impl-threads]
def search_threads(filenames, regex, opts):
    MAX_FILES = 10
    MAX_MATCHES = 100

    matches_by_file = queue.Queue()

    def do_background():
        # Make sure we don't have too many threads at once,
        # i.e. too many files open at once.
        counter = threading.Semaphore(MAX_FILES)

        def task(filename, matches):
            lines = iter_lines(filename)
            for match in search_lines(lines, regex, opts, filename):
                matches.put(match)  # blocking
            matches.put(None)  # blocking
            # Let a new thread start.
            counter.release()

        for filename in filenames:
            # Prepare for the file.
            matches = queue.Queue(MAX_MATCHES)
            matches_by_file.put((filename, matches))

            # Start a thread to process the file.
            t = threading.Thread(target=task, args=(filename, matches))
            counter.acquire()
            t.start()
        matches_by_file.put(None)

    background = threading.Thread(target=do_background)
    background.start()

    # Yield the results as they are received, in order.
    next_matches = matches_by_file.get()  # blocking
    while next_matches is not None:
        filename, matches = next_matches
        match = matches.get()  # blocking
        while match is not None:
            yield match
            match = matches.get()  # blocking
        next_matches = matches_by_file.get()  # blocking

    background.join()
# [end-impl-threads]


# [start-impl-cf-threads]
def search_cf_threads(filenames, regex, opts):
    MAX_FILES = 10
    MAX_MATCHES = 100

    matches_by_file = queue.Queue()

    def do_background():
        def task(filename, matches):
            lines = iter_lines(filename)
            for match in search_lines(lines, regex, opts, filename):
                matches.put(match)  # blocking
            matches.put(None)  # blocking

        with concurrent.futures.ThreadPoolExecutor(MAX_FILES) as workers:
            for filename in filenames:
                # Prepare for the file.
                matches = queue.Queue(MAX_MATCHES)
                matches_by_file.put((filename, matches))

                # Start a thread to process the file.
                workers.submit(task, filename, matches)
            matches_by_file.put(None)

    background = threading.Thread(target=do_background)
    background.start()

    # Yield the results as they are received, in order.
    next_matches = matches_by_file.get()  # blocking
    while next_matches is not None:
        filename, matches = next_matches
        match = matches.get()  # blocking
        while match is not None:
            yield match
            match = matches.get()  # blocking
        next_matches = matches_by_file.get()  # blocking

    background.join()
# [end-impl-cf-threads]


# [start-impl-interpreters]
def search_interpreters(filenames, regex, opts):
    raise NotImplementedError
# [end-impl-interpreters]


# [start-impl-cf-interpreters]
def search_cf_interpreters(filenames, regex, opts):
    raise NotImplementedError
# [end-impl-cf-interpreters]


# [start-impl-asyncio]
def search_asyncio(filenames, regex, opts):
    raise NotImplementedError
# [end-impl-asyncio]


# [start-impl-multiprocessing]
def search_multiprocessing(filenames, regex, opts):
    raise NotImplementedError
# [end-impl-multiprocessing]


# [start-impl-cf-multiprocessing]
def search_cf_multiprocessing(filenames, regex, opts):
    raise NotImplementedError
# [end-impl-cf-multiprocessing]


#######################################
# common implementation code

#class FilesSearch:
#
#    def __call__(self, filenames, regex, opts):
#        ...


class Search:

#    def __enter

    def __call__(self, filenames, regex, opts):
        ...

#    def _start


#class Results:
#    ...





class Grep:

    def __init__(self, regex, opts):
        if isinstance(regex, str):
            regex = re.compile(regex)
        self.regex = regex
        self.opts = opts

    def search(self, *sources):
        sources = Source.resolve_all(sources, self.opts)
        sources = self._resize_sources(sources)
        return self._search_sources(sources)

    def _resize_sources(self, sources):
        # Some subclasses may want to split sources up.
        yield from sources

    def _search_sources(self, sources):
        # This is re-implemented by each concurrency model subclass.
        # 1. split the problem into chunks (in self._resize_sources())
        # 2. fan out the chunks
        # 3. merge the results, in correct order
        return Search.from_sources(sources, self.regex, self.opts)


class Search:

    @classmethod
    def from_sources(cls, sources, regex, opts):
        self = cls(regex, opts)
        self._start(sources)
        return self

    def __init__(self, regex, opts):
        self.regex = regex
        self.opts = opts
        self._results = None
        self._stopping = False
        self._done = False

    def __iter__(self):
        if self._done:
            return
        if self._results is None:
            raise RuntimeError('not started')
        yield from self._results

    @property
    def matched(self):
        if self._results is None:
            raise RuntimeError('not started')
        return self._results.matched

    @property
    def done(self):
        return self._results.done

    @property
    def results(self):
        if self._results is None:
            raise RuntimeError('not started')
        return self._results

    def __enter__(self):
        if self._results is None:
            raise RuntimeError('not started')
        return self._results

    def __exit__(self, *args, **kwargs):
        self.stop()
        self.join()

    def stop(self):
        if self._results is None:
            raise RuntimeError('not started')
        if self._done:
            return
        self._stop()

    def join(self):
        if self._results is None:
            raise RuntimeError('not started')
        self._join()

    def _start(self, sources):
        assert self._results is None
        matches = self._run(sources)
        self._results = Results(matches)

    def _run(self, sources):
        for source in sources:
            if self._stopping:
                break
            for match in self._search_source(source):
                if self._stopping:
                    break
                yield match
        self._done = True

    def _stop(self):
        self._stopping = True

    def _join(self):
        if not self._done:
            raise RuntimeError('not done (or stopped)')

    def _search_source(self, source):
        filename = source.filename
        lines = source.iter_lines()
        def iter_blocking(lines=lines):
            for line in lines:
                if self._stopping:
                    return
                yield line
        lines = iter_blocking()
        try:
            if self.opts.filesonly == 'invert':
                for line in lines:
                    m = self.regex.search(line)
                    if m:
                        break
                else:
                    yield Match.from_file(filename)
            elif self.opts.filesonly:
                for line in lines:
                    m = self.regex.search(line)
                    if m:
                        yield Match.from_file(filename)
                        break
            elif self.opts.invertmatch:
                for line in lines:
                    m = self.regex.search(line)
                    if m:
                        continue
                    yield Match.from_inverted_match(line, filename)
            else:
                for line in lines:
                    m = self.regex.search(line)
                    if not m:
                        continue
                    yield Match.from_re_match(m, line, filename)
        except UnicodeDecodeError:
            # It must be a binary file.
            return


class ConcurrentSearch(Search):

    def __init__(self, regex, opts):
        super().__init__(regex, opts)
        self._running = threading.Lock()

    def _run(self, sources):
        self._running.acquire()
        yield from self._search_sources(sources)
        self._running.release()
        self._done = True

    def _join(self):
        self._running.acquire()
        self._running.release()

    def _search_sources(self, sources):
        raise NotImplementedError




class ConcurrentImpl:
    ...




#######################################
# implementations

#--------------------------------------
# [start-sequential]
class SequentialSearch(Search):

    @classmethod
    def from_sources(cls, sources, regex, opts):
        self = cls(regex, opts)
        self._start(sources)
        return self

    def _start(self, sources):
        assert self._results is None
        matches = self._run(sources)
        self._results = Results(matches)

    def _run(self, sources):
        for source in sources:
            if self._stopping:
                break
            for match in self._search_source(source):
                if self._stopping:
                    break
                yield match
        self._done = True

    def _join(self):
        if not self._done:
            raise RuntimeError('not done (or stopped)')

class GrepSequential(Grep):

    def search(self, *sources):
        sources = Source.resolve_all(sources, self.opts)
        sources = self._resize_sources(sources)
        return self._search_sources(sources)

    def _resize_sources(self, sources):
        yield from sources

    def _search_sources(self, sources):
        return SequentialSearch.from_sources(sources, self.regex, self.opts)
# [end-sequential]


#--------------------------------------
# [start-threads]
class ThreadedSearch(ConcurrentSearch):

    MAX_FILES = 10
    MAX_MATCHES = 100

    @classmethod
    def from_sources(cls, sources, regex, opts):
        self = cls(regex, opts)
        self._start(sources)
        return self

    def _start(self, sources):
        assert self._results is None
        # wraps self._search_sources():
        matches = self._run(sources)
        self._results = Results(matches)

    def _search_sources(self, sources):
        matches_by_file = queue.Queue()

        t = threading.Thread(
            target=self._manage_tasks,
            args=(sources, matches_by_file),
        )
        t.start()

        def next_blocking(items):
            while True:
                if self._stopping:
                    return None
                try:
                    return items.get(timeout=0.1)
                except queue.Empty:
                    pass

        # Yield the results as they are received, in order.
        next_matches = next_blocking(matches_by_file)
        while next_matches is not None:
            filename, matches = next_matches
            match = next_blocking(matches)
            while match is not None:
                yield match
                match = next_blocking(matches)
            next_matches = next_blocking(matches_by_file)

        t.join()

    def _manage_tasks(self, sources, matches_by_file):
        add_file_matches = matches_by_file.put
        counter = threading.Semaphore(self.MAX_FILES)

        for source in sources:
            if self._stopping:
                break

            # Prepare for the file.
            matches = queue.Queue(self.MAX_MATCHES)
            add_file_matches((source.filename, matches))

            # Start a thread to process the file.
            t = threading.Thread(
                target=self._task,
                args=(source, matches, counter),
            )
            while not self._stopping:
                if counter.acquire(timeout=0.1):
                    t.start()
                    break
        add_file_matches(None)

    def _task(self, source, matches, counter):
        def send_blocking(match):
            while not self._stopping:
                try:
                    return matches.put(match, timeout=0.1)
                except queue.Empty:
                    pass
        for match in self._search_source(source):
            send_blocking(match)
        send_blocking(None)
        # Let a new thread start.
        counter.release()

class GrepWithThreads(Grep):

    def search(self, *sources):
        sources = Source.resolve_all(sources, self.opts)
        sources = self._resize_sources(sources)
        return self._search_sources(sources)

    def _resize_sources(self, sources):
        yield from sources

    def _search_sources(self, sources):
        return ThreadedSearch.from_sources(sources, self.regex, self.opts)
# [end-threads]


#--------------------------------------
# [start-cf-threads]
class CFThreadedSearch(ConcurrentSearch):

    MAX_FILES = 10
    MAX_MATCHES = 100

    @classmethod
    def from_sources(cls, sources, regex, opts):
        self = cls(regex, opts)
        self._start(sources)
        return self

    def _start(self, sources):
        assert self._results is None
        # wraps self._search_sources():
        matches = self._run(sources)
        self._results = Results(matches)

    def _search_sources(self, sources):
        matches_by_file = queue.Queue()

        t = threading.Thread(
            target=self._manage_tasks,
            args=(sources, matches_by_file),
        )
        t.start()

        def next_blocking(items):
            while True:
                if self._stopping:
                    return None
                try:
                    return items.get(timeout=0.1)
                except queue.Empty:
                    pass

        # Yield the results as they are received, in order.
        next_matches = next_blocking(matches_by_file)
        while next_matches is not None:
            filename, matches = next_matches
            match = next_blocking(matches)
            while match is not None:
                yield match
                match = next_blocking(matches)
            next_matches = next_blocking(matches_by_file)

        t.join()

    def _manage_tasks(self, sources, matches_by_file):
        add_file_matches = matches_by_file.put
        with concurrent.futures.ThreadPoolExecutor(self.MAX_FILES) as workers:
            for source in sources:
                if self._stopping:
                    break

                # Prepare for the file.
                matches = queue.Queue(self.MAX_MATCHES)
                add_file_matches((source.filename, matches))

                # Start a thread to process the file.
                workers.submit(self._task, source, matches)
            add_file_matches(None)

    def _task(self, source, matches):
        def send_blocking(match):
            while not self._stopping:
                try:
                    return matches.put(match, timeout=0.1)
                except queue.Empty:
                    pass
        for match in self._search_source(source):
            send_blocking(match)
        send_blocking(None)

class GrepWithCFThreads(Grep):

    def search(self, *sources):
        sources = Source.resolve_all(sources, self.opts)
        sources = self._resize_sources(sources)
        return self._search_sources(sources)

    def _resize_sources(self, sources):
        yield from sources

    def _search_sources(self, sources):
        return ThreadedSearch.from_sources(sources, self.regex, self.opts)
# [end-cf-threads]


#--------------------------------------
# [start-subinterpreters]
class InterpretersSearch(ConcurrentSearch):

    MAX_FILES = 10
    MAX_MATCHES = 100

    @classmethod
    def from_sources(cls, sources, regex, opts):
        self = cls(regex, opts)
        self._start(sources)
        return self

    def _start(self, sources):
        assert self._results is None
        # wraps self._search_sources():
        matches = self._run(sources)
        self._results = Results(matches)

    def _search_sources(self, sources):
        matches_by_file = interpreters.queues.create()

        t = threading.Thread(
            target=self._manage_tasks,
            args=(sources, matches_by_file),
        )
        t.start()

        def next_blocking(items):
            while True:
                if self._stopping:
                    return None
                try:
                    return items.get(timeout=0.1)
                except queue.Empty:
                    pass

        # Yield the results as they are received, in order.
        next_matches = next_blocking(matches_by_file)
        while next_matches is not None:
            filename, matches = next_matches
            match = next_blocking(matches)
            while match is not None:
                yield match
                match = next_blocking(matches)
            next_matches = next_blocking(matches_by_file)

        t.join()

    def _manage_tasks(self, sources, matches_by_file):
        add_file_matches = matches_by_file.put
        ready_workers = queue.Queue(self.MAX_FILES)
        workers = []

        def next_worker():
            if len(workers) < self.MAX_FILES:
                interp = interpreters.create()
                interp.exec(f"""if True:
                    import re, types
                    import test.support.interpreters.queues
                    from grep import Source, Search
                    regex = re.compile({self.regex.pattern!r}, re.VERBOSE)
                    opts = {tuple(vars(self.opts).items())}
                    opts = types.SimpleNamespace(opts)
                    search = Search(regex, opts)
                    """)
#                interp.prepare_main(ready_workers=ready_workers)
                workers.append(interp)
                ready_workers.put(interp)
            while not self._stopping:
                try:
                    return ready_workers.get(timeout=0.1)
                except queue.Empty:
                    pass

        for source in sources:
            if self._stopping:
                break

            # Prepare for the file.
            matches = interpreters.queues.create(self.MAX_MATCHES)
            add_file_matches((source.filename, matches))
            interp = next_worker()

            # Start a thread to process the file.
            t = threading.Thread(
                target=self._task,
                args=(source, matches, interp, ready_workers),
            )
            if not self._stopping:
                t.start()
        add_file_matches(None)

    def _task(self, source, matches, interp, ready_workers):
        #interp.call(self._search_source, (matches, source))
        interp.prepare_main(matches=matches)
        interp.exec(f"""if True:
            source = Source({source.filename!r})
            for match in search._search_source(source):
                matches.put(match)
            matches.put(None)
            """)
        # Let a new thread start.
        ready_workers.put(interp)

#        def send_blocking(match):
#            while not self._stopping:
#                try:
#                    return matches.put(match, timeout=0.1)
#                except queue.Empty:
#                    pass
#        for match in self._search_source(source):
#            send_blocking(match)
#        send_blocking(None)

class GrepWithInterpreters(Grep):

    def search(self, *sources):
        sources = Source.resolve_all(sources, self.opts)
        sources = self._resize_sources(sources)
        return self._search_sources(sources)

    def _resize_sources(self, sources):
        yield from sources

    def _search_sources(self, sources):
        return InterpretersSearch.from_sources(sources, self.regex, self.opts)
# [end-subinterpreters]


#--------------------------------------
# [start-cf-subinterpreters]
class CFInterpretersSearch(ConcurrentSearch):

    MAX_FILES = 10
    MAX_MATCHES = 100

    @classmethod
    def from_sources(cls, sources, regex, opts):
        self = cls(regex, opts)
        self._start(sources)
        return self

    def _start(self, sources):
        assert self._results is None
        # wraps self._search_sources():
        matches = self._run(sources)
        self._results = Results(matches)

    def _search_sources(self, sources):
        raise NotImplementedError

class GrepWithCFInterpreters(Grep):

    def search(self, *sources):
        sources = Source.resolve_all(sources, self.opts)
        sources = self._resize_sources(sources)
        return self._search_sources(sources)

    def _resize_sources(self, sources):
        yield from sources

    def _search_sources(self, sources):
        return CFInterpretersSearch.from_sources(sources, self.regex, self.opts)
# [end-cf-subinterpreters]


#--------------------------------------
# [start-async]
class GrepWithAsyncio(Grep):

    MAX_FILES = 10
    MAX_MATCHES = 100

    def _search_sources(self, sources):
        raise NotImplementedError
# [end-async]


#--------------------------------------
# [start-multiprocessing]
class MultiprocessingSearch(ConcurrentSearch):

    MAX_FILES = 10
    MAX_MATCHES = 100

    @classmethod
    def from_sources(cls, sources, regex, opts):
        self = cls(regex, opts)
        self._start(sources)
        return self

    def _start(self, sources):
        assert self._results is None
        # wraps self._search_sources():
        matches = self._run(sources)
        self._results = Results(matches)

    def _search_sources(self, sources):
        matches_by_file = queue.Queue()

        t = threading.Thread(
            target=self._manage_tasks,
            args=(sources, matches_by_file),
        )
        t.start()

        def next_blocking(items):
            while True:
                if self._stopping:
                    return None
                try:
                    return items.get(timeout=0.1)
                except queue.Empty:
                    pass

        # Yield the results as they are received, in order.
        next_matches = next_blocking(matches_by_file)
        while next_matches is not None:
            filename, matches = next_matches
            match = next_blocking(matches)
            while match is not None:
                yield match
                match = next_blocking(matches)
            next_matches = next_blocking(matches_by_file)

        t.join()

    def _manage_tasks(self, sources, matches_by_file):
        add_file_matches = matches_by_file.put
        counter = threading.Semaphore(self.MAX_FILES)
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

        for source in sources:
            if self._stopping:
                break

            # Prepare for the file.
            matches = queue.Queue(self.MAX_MATCHES)
            add_file_matches((source.filename, matches))

            # Start a thread to process the file.
            t = threading.Thread(
                target=self._task,
                args=(source, matches, counter),
            )
            while not self._stopping:
                if counter.acquire(timeout=0.1):
                    t.start()
                    break
        add_file_matches(None)

    def _task(self, source, matches, finished):
        def send_blocking(match):
            while not self._stopping:
                try:
                    return matches.put(match, timeout=0.1)
                except queue.Empty:
                    pass
        for match in self._search_source(source):
            send_blocking(match)
        send_blocking(None)
        # Let a new thread start.
        counter.release()

class GrepWithThreads(Grep):

    def search(self, *sources):
        sources = Source.resolve_all(sources, self.opts)
        sources = self._resize_sources(sources)
        return self._search_sources(sources)

    def _resize_sources(self, sources):
        yield from sources

    def _search_sources(self, sources):
        return ThreadedSearch.from_sources(sources, self.regex, self.opts)

class GrepWithMultiprocessing(Grep):

    MAX_FILES = 10
    MAX_MATCHES = 100

    def _search_sources(self, sources):
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
                #

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
# [end-multiprocessing]


#--------------------------------------
# [start-cf-multiprocessing]
class GrepWithCFMultiprocessing(Grep):

    MAX_FILES = 10
    MAX_MATCHES = 100

    def _search_sources(self, sources):
        matches_by_file = queue.Queue()

        def manage_tasks():
            procs = concurrent.futures.ProcessPoolExecutor(MAX_FILES)

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
                #

                # Start a thread to process the file.
                procs.submit(task, infile, matches)
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
# [end-cf-multiprocessing]


#--------------------------------------
# [start-cf]
class CFSearch(ConcurrentSearch):

    # ThreadPoolExecutor, ProcessPoolExecutor,
    # or InterpreterPoolExecutor.
    EXECUTOR = None
    QUEUE = None

    MAX_FILES = 10
    MAX_MATCHES = 100

    @classmethod
    def from_sources(cls, sources, regex, opts):
        self = cls(regex, opts)
        self._start(sources)
        return self

    def _start(self, sources):
        assert self._results is None
        # wraps self._search_sources():
        matches = self._run(sources)
        self._results = Results(matches)

    def _search_sources(self, sources):
        matches_by_file = self.QUEUE()

        t = threading.Thread(
            target=self._manage_tasks,
            args=(sources, matches_by_file),
        )
        t.start()

        def next_blocking(items):
            while True:
                if self._stopping:
                    return None
                try:
                    return items.get(timeout=0.1)
                except queue.Empty:
                    pass

        # Yield the results as they are received, in order.
        next_matches = next_blocking(matches_by_file)
        while next_matches is not None:
            filename, matches = next_matches
            match = next_blocking(matches)
            while match is not None:
                yield match
                match = next_blocking(matches)
            next_matches = next_blocking(matches_by_file)

        t.join()

    def _manage_tasks(self, sources, matches_by_file):
        add_file_matches = matches_by_file.put
        with self.EXECUTOR(self.MAX_FILES) as workers:
            for source in sources:
                if self._stopping:
                    break

                # Prepare for the file.
                matches = self.QUEUE(self.MAX_MATCHES)
                add_file_matches((source.filename, matches))

                # Start a thread to process the file.
                workers.submit(self._task, source, matches)
            add_file_matches(None)

    def _task(self, source, matches):
        def send_blocking(match):
            while not self._stopping:
                try:
                    return matches.put(match, timeout=0.1)
                except queue.Empty:
                    pass
        for match in self._search_source(source):
            send_blocking(match)
        send_blocking(None)


class CFGrep(Grep):
    EXECUTOR = None
    QUEUE = None
    def _search_sources(self, sources):
        class Search(CFSearch):
            EXECUTOR = self.EXECUTOR
            QUEUE = self.QUEUE
        return Search.from_sources(sources, self.regex, self.opts)


class GrepWithCFThreadsAlt(CFGrep):
    EXECUTOR = concurrent.futures.ThreadPoolExecutor
    QUEUE = queue.Queue


class GrepWithCFInterpretersAlt(CFGrep):
    #EXECUTOR = concurrent.futures.InterpreterPoolExecutor
    #QUEUE = interpreters.Queue
    ...


class GrepWithCFMultiprocessingAlt(CFGrep):
    EXECUTOR = concurrent.futures.ProcessPoolExecutor
    QUEUE = multiprocessing.Queue
# [end-cf]


IMPLEMENTATIONS = {
    'sequential': search_sequential,
    'threads': search_threads,
    'threads-common': GrepWithThreads,
    'interpreters': GrepWithInterpreters,
    'interpreters-common': GrepWithInterpreters,
    'asyncio': GrepWithAsyncio,
    'multiprocessing': GrepWithMultiprocessing,
    'multiprocessing-common': GrepWithMultiprocessing,
}
CF_IMPLEMENTATIONS = {
#    'threads': GrepWithCFThreads,
    'threads': search_cf_threads,
    'interpreters': GrepWithCFInterpreters,
    'multiprocessing': GrepWithCFMultiprocessing,
}
CF_IMPLEMENTATIONS_ALT = {
    'threads': GrepWithCFThreadsAlt,
    'interpreters': GrepWithCFInterpretersAlt,
    'multiprocessing': GrepWithCFMultiprocessingAlt,
}


def _resolve_impl(impl, cf):
    if cf == 'alt':
        return CF_IMPLEMENTATIONS_ALT[impl]
    elif cf:
        return CF_IMPLEMENTATIONS[impl]
    else:
        return IMPLEMENTATIONS[impl]


def resolve_concurrency(concurrency, cf=None):
    if concurrency is None:
        impl = _impl_sequential
    elif isinstance(concurrency, str):
        impl = _resolve_impl(concurrency, cf)
    else:
        assert cf is None, cf
        impl = concurrency

    if isinstance(impl, ConcurrentImpl):
        return contextlib.nullcontext(impl)
    elif isinstance(impl, type):
        if not issubclass(impl, ConcurrentImpl):
            raise TypeError(impl)
        return impl()
    elif callable(impl):
        return impl
    else:
        raise TypeError(impl)


#######################################

# [start-do-search]
@contextlib.contextmanager
def _do_search(filenames, regex, opts, concurrency):
    # "impl" is a callable that performs a search for
    # a set of files.  That might be a function or not.
    impl = resolve_concurrency(concurrency)

    # If not a function, tt may involve resources that
    # need to be cleaned up when we're done, in which
    # case it can be used as a context manager.
    if not hasattr(type(impl), '__enter__'):
        impl = contextlib.nullcontext(impl)
    with impl as search:

        # "maches" is an iteraterable of Match objects.
        # Concurrency can only happen here.
        matches = search(filenames, regex, opts)

        # Just like with "impl" this search operation may
        # involve resources that need to be cleaned up
        # once we're done searching.  Thus the returned
        # object might also be a context manager.
        cm = matches
        if not hasattr(type(cm), '__enter__'):
            cm = contextlib.nullcontext(matches)
        with cm as matches:

            # This is what _do_search() actually "returns".
            yield matches
# [end-do-search]


#######################################
# [app]

def grep(regex, opts, filenames, concurrency=None):
    # step 1:
    if isinstance(regex, str):
        regex = re.compile(regex)
    # step 2:
    filenames = resolve_filenames(filenames, opts)

    # needed for step 5:
    matched = False
    def iter_matches(matches):
        nonlocal matched
        matches = iter(matches)
        for match in matches:
            matched = True
            yield match
            break
        yield from matches

    # step 3:
    with _do_search(filenames, regex, opts, concurrency) as matches:
        matches = iter_matches(matches)
        # step 4:
        for line in render_matches(matches, opts):
            print(line)

    # step 5:
    return 0 if matched else 1


def parse_args(argv=sys.argv[1:], prog=sys.argv[0]):
    import argparse
    parser = argparse.ArgumentParser(
        prog=prog,
        usage=('[--help] [--concurrency MODEL] [--cf] [-rivHhLloqs] '
               '[-e] REGEX FILE [FILE ...]'),
        add_help=False,
    )

    parser.add_argument('--help', action='help', default=argparse.SUPPRESS,
                        help='show this help message and exit')

    concurrencyopts = parser.add_argument_group(title='concurrency')
    concurrencyopts.add_argument('--concurrency', dest='impl',
                                 choices=tuple(IMPLEMENTATIONS))
    parser.set_defaults(impl='sequential')
    concurrencyopts.add_argument('--cf', '--concurrent-futures', dest='cf',
                                 action='store_const', const=True)
    concurrencyopts.add_argument('--cf-alt', dest='cf',
                                 action='store_const', const='alt')

    grepopts = parser.add_argument_group(title='grep')
    Options.add_cli(grepopts, 'opts')

    regexopts = grepopts.add_mutually_exclusive_group(required=True)
    regexopts.add_argument('-e', '--regexp', dest='regex', metavar='REGEX')
    regexopts.add_argument('regex', nargs='?', metavar='REGEX')
    grepopts.add_argument('files', nargs='+', metavar='FILE')

    args = parser.parse_args()
    ns = vars(args)

    impl = ns.pop('impl')
    cf = ns.pop('cf')
    opts = ns.pop('opts')
    pat = ns.pop('regex')
    files = ns.pop('files')
    assert not ns, ns

    if impl not in IMPLEMENTATIONS:
        raise NotImplementedError(impl)
    if cf and impl not in CF_IMPLEMENTATIONS:
        parser.error(f'{impl} does not support --cf')

    return impl, cf, opts, pat, files


if __name__ == '__main__':
    impl, cf, opts, pat, files = parse_args()
    impl = _resolve_impl(impl, cf)
    rc = grep(pat, opts, files, impl)
    sys.exit(rc)
