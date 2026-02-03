import unittest
from itertools import batched, chain, combinations_with_replacement, cycle, permutations
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
    def test_batched(self):
        number_of_iterations = 10
        for _ in range(number_of_iterations):
            it = batched(tuple(range(1000)), 2)
            threading_helper.run_concurrently(work_iterator, nthreads=10, args=[it])

    @threading_helper.reap_threads
    def test_cycle(self):
        def work(it):
            for _ in range(400):
                next(it)

        number_of_iterations = 6
        for _ in range(number_of_iterations):
            it = cycle((1, 2, 3, 4))
            threading_helper.run_concurrently(work, nthreads=6, args=[it])

    @threading_helper.reap_threads
    def test_chain(self):
        number_of_iterations = 10
        for _ in range(number_of_iterations):
            it = chain(*[(1,)] * 200)
            threading_helper.run_concurrently(work_iterator, nthreads=6, args=[it])

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
            it = permutations(tuple(range(4)), 2)
            threading_helper.run_concurrently(work_iterator, nthreads=6, args=[it])


if __name__ == "__main__":
    unittest.main()
