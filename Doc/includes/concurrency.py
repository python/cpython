"""Example code for howto/concurrency.rst.

The examples take advantage of the literalinclude directive's
:start-after: and :end-before: options.
"""

import contextlib
import os
import tempfile


@contextlib.contextmanager
def dummy_files(*filenames):
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

    registry = []

    def __init__(self, func):
        super().__init__(func)
        self.func = func

    def __set_name__(self, cls, name):
        assert name == self.func.__name__, (name, self.func.__name__)
        type(self).registry.append((self.func, cls))


class ConcurrentFutures:

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


class Workload1:

    @example
    def run_using_threads():
        # [start-w1-threads]
        import threading

        def task():
            ...

        t = threading.Thread(target=task)
        t.start()

        ...
        # [end-w1-threads]

    @example
    def run_using_multiprocessing():
        # [start-w1-multiprocessing]
        import multiprocessing

        def task():
            ...

        ...
        # [end-w1-multiprocessing]

    @example
    def run_using_async():
        # [start-w1-async]
        # async 1
        ...
        # [end-w1-async]

    @example
    def run_using_subinterpreters():
        # [start-w1-subinterpreters]
        # subinterpreters 1
        ...
        # [end-w1-subinterpreters]

    @example
    def run_using_smp():
        # [start-w1-smp]
        # smp 1
        ...
        # [end-w1-smp]

    @example
    def run_using_concurrent_futures_thread():
        # [start-w1-concurrent-futures-thread]
        # concurrent.futures 1
        ...
        # [end-w1-concurrent-futures-thread]


#######################################
# workload 2: ...
#######################################

class Workload2:

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
    def run_using_multiprocessing():
        # [start-w2-multiprocessing]
        import multiprocessing

        def task():
            ...

        ...
        # [end-w2-multiprocessing]

    @example
    def run_using_async():
        # [start-w2-async]
        # async 2
        ...
        # [end-w2-async]

    @example
    def run_using_subinterpreters():
        # [start-w2-subinterpreters]
        # subinterpreters 2
        ...
        # [end-w2-subinterpreters]

    @example
    def run_using_smp():
        # [start-w2-smp]
        # smp 2
        ...
        # [end-w2-smp]

    @example
    def run_using_concurrent_futures_thread():
        # [start-w2-concurrent-futures-thread]
        # concurrent.futures 2
        ...
        # [end-w2-concurrent-futures-thread]


#######################################
# workload 3: ...
#######################################

class Workload3:

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
    def run_using_multiprocessing():
        # [start-w3-multiprocessing]
        import multiprocessing

        def task():
            ...

        ...
        # [end-w3-multiprocessing]

    @example
    def run_using_async():
        # [start-w3-async]
        # async 3
        ...
        # [end-w3-async]

    @example
    def run_using_subinterpreters():
        # [start-w3-subinterpreters]
        # subinterpreters 3
        ...
        # [end-w3-subinterpreters]

    @example
    def run_using_smp():
        # [start-w3-smp]
        # smp 3
        ...
        # [end-w3-smp]

    @example
    def run_using_concurrent_futures_thread():
        # [start-w3-concurrent-futures-thread]
        # concurrent.futures 3
        ...
        # [end-w3-concurrent-futures-thread]


if __name__ == '__main__':
    # Run all the examples.
    div1 = '#' * 40
    div2 = '#' + '-' * 39
    last = None
    for func, cls in example.registry:
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
