"""Example code for howto/concurrency.rst.

The examples take advantage of the literalinclude directive's
:start-after: and :end-before: options.
"""

import os.path
import sys


IMPLS_DIR = os.path.join(os.path.dirname(__file__), 'concurrency')
if IMPLS_DIR not in sys.path:
    sys.path.insert(0, IMPLS_DIR)


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
# workload: grep
#######################################

class GrepExamples(WorkloadExamples):

    @staticmethod
    def app(name, kind='basic', cf=False):
        import shlex
        from grep import Options, resolve_impl, grep
        from grep.__main__ import render_matches
        opts = Options(
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
        #opts = Options(recursive=True, filesonly='match')
        impl = resolve_impl(name, kind, cf)
        pat = 'help'
        #filenames = '.'
        filenames = ['make.bat', 'Makefile']
        print(f'# grep {opts} {shlex.quote(pat)} {shlex.join(filenames)}')
        print()
        matches = grep(pat, opts, *filenames, impl=impl)
        if name == 'asyncio':
            assert hasattr(type(matches), '__aiter__'), (name,)
            async def search():
                async for line in render_matches(matches, opts):
                    print(line)
            import asyncio
            asyncio.run(search())
        else:
            assert not hasattr(type(matches), '__aiter__'), (name,)
            for line in render_matches(matches, opts):
                print(line)

    @example
    def run_sequentially():
        GrepExamples.app('sequential')

    @example
    def run_using_threads():
        GrepExamples.app('threads')

    @example
    def run_using_cf_threads():
        GrepExamples.app('threads', cf=True)

    @example
    def run_using_subinterpreters():
        GrepExamples.app('interpreters')

    @example
    def run_using_cf_subinterpreters():
        GrepExamples.app('interpreters', cf=True)

    @example
    def run_using_async():
        GrepExamples.app('asyncio')

    @example
    def run_using_multiprocessing():
        GrepExamples.app('multiprocessing')

    @example
    def run_using_cf_multiprocessing():
        GrepExamples.app('multiprocessing', cf=True)


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


#######################################
# workload 2: ...
#######################################

class Workload2(WorkloadExamples):

    @example
    def run_sequentially():
        # [start-w2-sequential]
        # sequential 3
        ...
        # [end-w2-sequential]

    @example
    def run_using_threads():
        # [start-w2-threads]
        import threading

        def task():
            ...

        t = threading.Thread(target=task)
        t.start()

        ...
        # [end-w2-threads]

    @example
    def run_using_cf_thread():
        # [start-w2-cf-thread]
        # concurrent.futures 3
        ...
        # [end-w2-cf-thread]

    @example
    def run_using_subinterpreters():
        # [start-w2-subinterpreters]
        # subinterpreters 3
        ...
        # [end-w2-subinterpreters]

    @example
    def run_using_cf_subinterpreters():
        # [start-w2-cf-subinterpreters]
        # concurrent.futures 3
        ...
        # [end-w2-cf-subinterpreters]

    @example
    def run_using_async():
        # [start-w2-async]
        # async 3
        ...
        # [end-w2-async]

    @example
    def run_using_multiprocessing():
        # [start-w2-multiprocessing]
        import multiprocessing

        def task():
            ...

        ...
        # [end-w2-multiprocessing]

    @example
    def run_using_cf_multiprocessing():
        # [start-w2-cf-multiprocessing]
        # concurrent.futures 3
        ...
        # [end-w2-cf-multiprocessing]


# [start-w2-cf]
# concurrent.futures 2
...
# [end-w2-cf]


#######################################
# workload 3: ...
#######################################

class Workload3(WorkloadExamples):

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


# [start-w3-cf]
# concurrent.futures 3
...
# [end-w3-cf]


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
