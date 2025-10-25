"""
Benchmark for async tree workload, which calls asyncio.gather() on a tree
(6 levels deep, 6 branches per level) with the leaf nodes simulating some
(potentially) async work (depending on the benchmark variant). Benchmark
variants include:

1) "none": No actual async work in the async tree.
2) "io": All leaf nodes simulate async IO workload (async sleep 50ms).
3) "memoization": All leaf nodes simulate async IO workload with 90% of
                  the data memoized
4) "cpu_io_mixed": Half of the leaf nodes simulate CPU-bound workload and
                   the other half simulate the same workload as the
                   "memoization" variant.

All variants also have an "eager" flavor that uses the asyncio eager task
factory (if available), and a "tg" variant that uses TaskGroups.
"""


import asyncio
import math
import random

NUM_RECURSE_LEVELS = 5
NUM_RECURSE_BRANCHES = 5
RANDOM_SEED = 0
IO_SLEEP_TIME = 0.02
MEMOIZABLE_PERCENTAGE = 90
CPU_PROBABILITY = 0.5
FACTORIAL_N = 500


class AsyncTree:
    def __init__(self, use_task_groups=False):
        self.cache = {}
        self.use_task_groups = use_task_groups
        # set to deterministic random, so that the results are reproducible
        random.seed(RANDOM_SEED)

    async def mock_io_call(self):
        await asyncio.sleep(IO_SLEEP_TIME)

    async def workload_func(self):
        raise NotImplementedError(
            "To be implemented by each variant's derived class."
        )

    async def recurse_with_gather(self, recurse_level):
        if recurse_level == 0:
            await self.workload_func()
            return

        await asyncio.gather(
            *[self.recurse_with_gather(recurse_level - 1)
              for _ in range(NUM_RECURSE_BRANCHES)]
        )

    async def recurse_with_task_group(self, recurse_level):
        if recurse_level == 0:
            await self.workload_func()
            return

        async with asyncio.TaskGroup() as tg:
            for _ in range(NUM_RECURSE_BRANCHES):
                tg.create_task(
                    self.recurse_with_task_group(recurse_level - 1))

    async def run(self):
        if self.use_task_groups:
            await self.recurse_with_task_group(NUM_RECURSE_LEVELS)
        else:
            await self.recurse_with_gather(NUM_RECURSE_LEVELS)


class EagerMixin:
    async def run(self):
        loop = asyncio.get_running_loop()
        if hasattr(asyncio, 'eager_task_factory'):
            loop.set_task_factory(asyncio.eager_task_factory)
        return await super().run()


class NoneAsyncTree(AsyncTree):
    async def workload_func(self):
        return


class EagerAsyncTree(EagerMixin, NoneAsyncTree):
    pass


class IOAsyncTree(AsyncTree):
    async def workload_func(self):
        await self.mock_io_call()


class EagerIOAsyncTree(EagerMixin, IOAsyncTree):
    pass


class MemoizationAsyncTree(AsyncTree):
    async def workload_func(self):
        # deterministic random, seed set in AsyncTree.__init__()
        data = random.randint(1, 100)

        if data <= MEMOIZABLE_PERCENTAGE:
            if self.cache.get(data):
                return data

            self.cache[data] = True

        await self.mock_io_call()
        return data


class EagerMemoizationAsyncTree(EagerMixin, MemoizationAsyncTree):
    pass


class CpuIoMixedAsyncTree(MemoizationAsyncTree):
    async def workload_func(self):
        # deterministic random, seed set in AsyncTree.__init__()
        if random.random() < CPU_PROBABILITY:
            # mock cpu-bound call
            return math.factorial(FACTORIAL_N)
        else:
            return await MemoizationAsyncTree.workload_func(self)


class EagerCpuIoMixedAsyncTree(EagerMixin, CpuIoMixedAsyncTree):
    pass


BENCHMARKS = {
    "none": NoneAsyncTree,
    "eager": EagerAsyncTree,
    "io": IOAsyncTree,
    "eager_io": EagerIOAsyncTree,
    "memoization": MemoizationAsyncTree,
    "eager_memoization": EagerMemoizationAsyncTree,
    "cpu_io_mixed": CpuIoMixedAsyncTree,
    "eager_cpu_io_mixed": EagerCpuIoMixedAsyncTree,
}


def run_pgo():
    for benchmark in BENCHMARKS:
        async_tree_class = BENCHMARKS[benchmark]
        for use_task_groups in [True, False]:
            async_tree = async_tree_class(use_task_groups=use_task_groups)
            asyncio.run(async_tree.run())
