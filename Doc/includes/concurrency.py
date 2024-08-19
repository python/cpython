"""Example code for howto/concurrency.rst.

The examples take advantage of the literalinclude directive's
:start-after: and :end-before: options.
"""


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
