import unittest
from threading import Thread, Barrier
from itertools import (
    accumulate,
    chain,
    combinations_with_replacement,
    cycle,
    permutations,
    tee,
    zip_longest,
)
from test.support import threading_helper


threading_helper.requires_working_threading(module=True)


def work_iterator(it):
    while True:
        try:
            next(it)
        except StopIteration:
            break


class ItertoolsThreading(unittest.TestCase):

    @threading_helper.reap_threads
    def test_cycle(self):
        number_of_threads = 6
        number_of_iterations = 10
        number_of_cycles = 400

        barrier = Barrier(number_of_threads)
        def work(it):
            barrier.wait()
            for _ in range(number_of_cycles):
                _ = next(it)

        data = (1, 2, 3, 4)
        for it in range(number_of_iterations):
            cycle_iterator = cycle(data)
            worker_threads = []
            for ii in range(number_of_threads):
                worker_threads.append(
                    Thread(target=work, args=[cycle_iterator]))

            with threading_helper.start_threads(worker_threads):
                pass

            barrier.reset()

    @threading_helper.reap_threads
    def test_chain(self):
        number_of_threads = 6
        number_of_iterations = 20

        barrier = Barrier(number_of_threads)
        def work(it):
            barrier.wait()
            while True:
                try:
                    next(it)
                except StopIteration:
                    break

        data = [(1, )] * 200
        for it in range(number_of_iterations):
            chain_iterator = chain(*data)
            worker_threads = []
            for ii in range(number_of_threads):
                worker_threads.append(
                    Thread(target=work, args=[chain_iterator]))

            with threading_helper.start_threads(worker_threads):
                pass

            barrier.reset()

    @threading_helper.reap_threads
    def test_combinations_with_replacement(self):
        number_of_iterations = 6
        for _ in range(number_of_iterations):
            it = combinations_with_replacement(tuple(range(2)), 2)
            threading_helper.run_concurrently(work_iterator, nthreads=6, args=[it])

    @threading_helper.reap_threads
    def test_permutations(self):
        number_of_iterations = 6
        for _ in range(number_of_iterations):
            it = permutations(tuple(range(2)), 2)
            threading_helper.run_concurrently(work_iterator, nthreads=6, args=[it])

    @threading_helper.reap_threads
    def test_accumulate(self):
        number_of_iterations = 10
        for _ in range(number_of_iterations):
            it = accumulate(tuple(range(40)))
            threading_helper.run_concurrently(work_iterator, nthreads=10, args=[it])

    @threading_helper.reap_threads
    def test_zip_longest(self):
        number_of_iterations = 10
        for _ in range(number_of_iterations):
            it = zip_longest(list(range(4)), list(range(8)), fillvalue=0)
            threading_helper.run_concurrently(work_iterator, nthreads=10, args=[it])



class TestTeeConcurrent(unittest.TestCase):
    # itertools.tee branches share a linked list of internal data cells.
    # Concurrent iteration must not corrupt that shared state or crash the
    # free-threaded build.  A crash shows up as the interpreter dying (not as a
    # caught exception); tee is documented as not thread-safe, so a
    # ``RuntimeError`` from the re-entrancy guard is an allowed outcome and is
    # tolerated here.

    def test_same_branch(self):
        # Many threads consume the same tee branch.
        errors = []

        def consume(it):
            try:
                for _ in it:
                    pass
            except RuntimeError:
                pass
            except Exception as e:
                errors.append(e)

        for _ in range(100):
            a, _ = tee(iter(range(2000)), 2)
            threading_helper.run_concurrently(consume, nthreads=8, args=(a,))

        self.assertEqual(errors, [], msg=f"unexpected errors: {errors}")

    def test_sibling_branches(self):
        # Each thread consumes a different sibling branch of the same tee.
        errors = []

        def make_worker(it):
            def consume():
                try:
                    for _ in it:
                        pass
                except RuntimeError:
                    pass
                except Exception as e:
                    errors.append(e)

            return consume

        for _ in range(100):
            branches = tee(iter(range(4000)), 8)
            threading_helper.run_concurrently(
                [make_worker(it) for it in branches]
            )

        self.assertEqual(errors, [], msg=f"unexpected errors: {errors}")


if __name__ == "__main__":
    unittest.main()
