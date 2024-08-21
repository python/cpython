"""Example code for howto/concurrency.rst.

The examples take advantage of the literalinclude directive's
:start-after: and :end-before: options.
"""


class ConcurrentFutures:

    def example_basic(self):
        # [start-cf-basic]
        from concurrent.futures import ThreadPoolExecutor as Executor

        with Executor() as e:
            # Copy 4 files concurrently.
            e.submit(shutil.copy, 'src1.txt', 'dest1.txt')
            e.submit(shutil.copy, 'src2.txt', 'dest2.txt')
            e.submit(shutil.copy, 'src3.txt', 'dest3.txt')
            e.submit(shutil.copy, 'src4.txt', 'dest4.txt')

            # Run a function asynchronously and check the result.
            fut = executor.submit(pow, 323, 1235)
            res = fut.result()
            assert res == 323**1235
        # [end-cf-basic]

    def example_map_1(self):
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

    def example_map_2(self):
        # [start-cf-map-2]
        from concurrent.futures import ThreadPoolExecutor as Executor

        files = {
            'src1.txt': 'dest1.txt',
            'src2.txt': 'dest2.txt',
            'src3.txt': 'dest3.txt',
            'src4.txt': 'dest4.txt',
        }

        with Executor() as e:
            # Copy files concurrently, tracking missing files.
            copied = {}
            results = e.map(shutil.copy, files.keys(), files.values())
            for src in files:
                 copied[src] = next(results)
            assert list(copied.values()) == list(files.values())

            # An alternate spelling:
            copied = {}
            results = e.map(shutil.copy, files.keys(), files.values())
            for src, res in zip(files, results, strict=True):
                copied[src] = res
            assert list(copied.values()) == list(files.values())
        # [end-cf-map-2]

    def example_wait(self):
        # [start-cf-wait]
        import concurrent.futures
        from concurrent.futures import ThreadPoolExecutor as Executor

        files = {
            'src1.txt': 'dest1.txt',
            'src2.txt': 'dest2.txt',
            'src3.txt': 'dest3.txt',
            'src4.txt': 'dest4.txt',
        }

        with Executor() as e:
            # Copy 4 files concurrently and wait for them all to finish.
            futures = (e.submit(shutil.copy, src, tgt)
                       for src, tgt in files.items())
            concurrent.futures.wait(futures)

            # Using as_completed():
            futures = (e.submit(shutil.copy, src, tgt)
                       for src, tgt in files.items())
            list(concurrent.futures.as_completed(futures))

            # Using Executor.map():
            list(e.map(shutil.copy, files.keys(), files.values()))
        # [end-cf-wait]

    def example_as_completed(self):
        # [start-cf-as-completed]
        import concurrent.futures
        from concurrent.futures import ThreadPoolExecutor as Executor

        files = {
            'src1.txt': 'dest1.txt',
            'src2.txt': 'dest2.txt',
            'src3.txt': 'dest3.txt',
            'src4.txt': 'dest4.txt',
        }

        with Executor() as e:
            # Copy 4 files concurrently and handle each completion.
            copied = {}
            missing = []
            futures = (e.submit(shutil.copy, src, tgt)
                       for src, tgt in files.items())
            futures = dict(zip(futures, enumerate(files, 1)))
            for fut in concurrent.futures.as_completed(futures):
                i, src = futures[fut]
                res = fut.result()
                print(f'({i}) {src} copied')
                copied[src] = res
            assert list(copied.values()) == list(files.values())
        # [end-cf-as-completed]

    def example_error_result(self):
        # [start-cf-error-result]
        import concurrent.futures
        from concurrent.futures import ThreadPoolExecutor as Executor

        # Run a function asynchronously and catch the error.
        def fail():
            raise Exception('spam!')
        with Executor() as e:
            fut = executor.submit(fail)
            try:
                fut.result()
            except Exception as exc:
                arg, = exc.args
                assert arg == 'spam!'


        # Copy files concurrently, tracking missing files.
        files = {
            'src1.txt': 'dest1.txt',
            'src2.txt': 'dest2.txt',
            'src3.txt': 'dest3.txt',
            'src4.txt': 'dest4.txt',
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
            futures = (e.submit(shutil.copy, src, tgt)
                       for src, tgt in files.items())
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
        # [end-cf-error-result]


class Workload1:

    def run_using_threads(self):
        # [start-w1-threads]
        import threading

        def task():
            ...

        t = threading.Thread(target=task)
        t.start()

        ...
        # [end-w1-threads]

    def run_using_multiprocessing(self):
        # [start-w1-multiprocessing]
        import multiprocessing

        def task():
            ...

        ...
        # [end-w1-multiprocessing]

    def run_using_async(self):
        # [start-w1-async]
        # async 1
        ...
        # [end-w1-async]

    def run_using_subinterpreters(self):
        # [start-w1-subinterpreters]
        # subinterpreters 1
        ...
        # [end-w1-subinterpreters]

    def run_using_smp(self):
        # [start-w1-smp]
        # smp 1
        ...
        # [end-w1-smp]

    def run_using_concurrent_futures_thread(self):
        # [start-w1-concurrent-futures-thread]
        # concurrent.futures 1
        ...
        # [end-w1-concurrent-futures-thread]


#######################################
# workload 2: ...
#######################################

class Workload2:

    def run_using_threads(self):
        # [start-w2-threads]
        import threading

        def task():
            ...

        t = threading.Thread(target=task)
        t.start()

        ...
        # [end-w2-threads]

    def run_using_multiprocessing(self):
        # [start-w2-multiprocessing]
        import multiprocessing

        def task():
            ...

        ...
        # [end-w2-multiprocessing]

    def run_using_async(self):
        # [start-w2-async]
        # async 2
        ...
        # [end-w2-async]

    def run_using_subinterpreters(self):
        # [start-w2-subinterpreters]
        # subinterpreters 2
        ...
        # [end-w2-subinterpreters]

    def run_using_smp(self):
        # [start-w2-smp]
        # smp 2
        ...
        # [end-w2-smp]

    def run_using_concurrent_futures_thread(self):
        # [start-w2-concurrent-futures-thread]
        # concurrent.futures 2
        ...
        # [end-w2-concurrent-futures-thread]


#######################################
# workload 3: ...
#######################################

class Workload3:

    def run_using_threads(self):
        # [start-w3-threads]
        import threading

        def task():
            ...

        t = threading.Thread(target=task)
        t.start()

        ...
        # [end-w3-threads]

    def run_using_multiprocessing(self):
        # [start-w3-multiprocessing]
        import multiprocessing

        def task():
            ...

        ...
        # [end-w3-multiprocessing]

    def run_using_async(self):
        # [start-w3-async]
        # async 3
        ...
        # [end-w3-async]

    def run_using_subinterpreters(self):
        # [start-w3-subinterpreters]
        # subinterpreters 3
        ...
        # [end-w3-subinterpreters]

    def run_using_smp(self):
        # [start-w3-smp]
        # smp 3
        ...
        # [end-w3-smp]

    def run_using_concurrent_futures_thread(self):
        # [start-w3-concurrent-futures-thread]
        # concurrent.futures 3
        ...
        # [end-w3-concurrent-futures-thread]
