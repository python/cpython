# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
"""
Benchmark script for recursive async tree workloads. This script includes the
following microbenchmark scenarios:

1) "no_suspension": No suspension in the async tree.
2) "suspense_all": Suspension (simulating IO) at all leaf nodes in the async tree.
3) "memoization": Simulated IO calls at all leaf nodes, but with memoization. Only
                  un-memoized IO calls will result in suspensions.
4) "cpu_io_mixed": A mix of CPU-bound workload and IO-bound workload (with
                   memoization) at the leaf nodes.

Use the commandline flag or choose the corresponding <Scenario>AsyncTree class
to run the desired microbenchmark scenario.
"""


import asyncio
import math
import random
import time
from argparse import ArgumentParser


NUM_RECURSE_LEVELS = 6
NUM_RECURSE_BRANCHES = 6
IO_SLEEP_TIME = 0.05
DEFAULT_MEMOIZABLE_PERCENTAGE = 90
DEFAULT_CPU_PROBABILITY = 0.5
FACTORIAL_N = 500


def parse_args():
    parser = ArgumentParser(
        description="""\
Benchmark script for recursive async tree workloads. It can be run as a standalone
script, in which case you can specify the microbenchmark scenario to run and whether
to print the results.
"""
    )
    parser.add_argument(
        "-s",
        "--scenario",
        choices=["no_suspension", "suspense_all", "memoization", "cpu_io_mixed"],
        default="no_suspension",
        help="""\
Determines which microbenchmark scenario to run. Defaults to no_suspension. Options:
1) "no_suspension": No suspension in the async tree.
2) "suspense_all": Suspension (simulating IO) at all leaf nodes in the async tree.
3) "memoization": Simulated IO calls at all leaf nodes, but with memoization. Only
                  un-memoized IO calls will result in suspensions.
4) "cpu_io_mixed": A mix of CPU-bound workload and IO-bound workload (with
                   memoization) at the leaf nodes.
""",
    )
    parser.add_argument(
        "-m",
        "--memoizable-percentage",
        type=int,
        default=DEFAULT_MEMOIZABLE_PERCENTAGE,
        help="""\
Sets the percentage (0-100) of the data that should be memoized, defaults to 90. For
example, at the default 90 percent, data 1-90 will be memoized and data 91-100 will not.
""",
    )
    parser.add_argument(
        "-c",
        "--cpu-probability",
        type=float,
        default=DEFAULT_CPU_PROBABILITY,
        help="""\
Sets the probability (0-1) that a leaf node will execute a cpu-bound workload instead
of an io-bound workload. Defaults to 0.5. Only applies to the "cpu_io_mixed"
microbenchmark scenario.
""",
    )
    parser.add_argument(
        "-p",
        "--print",
        action="store_true",
        default=False,
        help="Print the results (runtime and number of Tasks created).",
    )
    parser.add_argument(
        "-e",
        "--eager",
        action="store_true",
        default=False,
        help="Use the eager task factory.",
    )
    return parser.parse_args()


class AsyncTree:
    def __init__(
        self,
        memoizable_percentage=DEFAULT_MEMOIZABLE_PERCENTAGE,
        cpu_probability=DEFAULT_CPU_PROBABILITY,
    ):
        self.suspense_count = 0
        self.task_count = 0
        self.memoizable_percentage = memoizable_percentage
        self.cpu_probability = cpu_probability
        self.cache = {}
        # set to deterministic random, so that the results are reproducible
        random.seed(0)

    async def mock_io_call(self):
        self.suspense_count += 1
        await asyncio.sleep(IO_SLEEP_TIME)

    async def suspense_func(self):
        raise NotImplementedError(
            "To be implemented by each microbenchmark's derived class."
        )

    async def recurse(self, recurse_level):
        if recurse_level == 0:
            await self.suspense_func()
            return

        await asyncio.gather(
            *[self.recurse(recurse_level - 1) for _ in range(NUM_RECURSE_BRANCHES)]
        )

    async def run_benchmark(self):
        await self.recurse(NUM_RECURSE_LEVELS)

    def run(self, use_eager_factory):

        def counting_task_constructor(coro, *, loop=None, name=None, context=None, yield_result=None):
            self.task_count += 1
            return asyncio.Task(coro, loop=loop, name=name, context=context, yield_result=yield_result)

        def counting_task_factory(loop, coro, *, name=None, context=None, yield_result=None):
            return counting_task_constructor(coro, loop=loop, name=name, context=context, yield_result=yield_result)

        asyncio.run(
            self.run_benchmark(),
            task_factory=(
                asyncio.create_eager_task_factory(counting_task_constructor)
                if use_eager_factory else counting_task_factory
            ),
        )


class NoSuspensionAsyncTree(AsyncTree):
    async def suspense_func(self):
        return


class SuspenseAllAsyncTree(AsyncTree):
    async def suspense_func(self):
        await self.mock_io_call()


class MemoizationAsyncTree(AsyncTree):
    async def suspense_func(self):
        # deterministic random (seed preset)
        data = random.randint(1, 100)

        if data <= self.memoizable_percentage:
            if self.cache.get(data):
                return data

            self.cache[data] = True

        await self.mock_io_call()
        return data


class CpuIoMixedAsyncTree(MemoizationAsyncTree):
    async def suspense_func(self):
        if random.random() < self.cpu_probability:
            # mock cpu-bound call
            return math.factorial(FACTORIAL_N)
        else:
            return await MemoizationAsyncTree.suspense_func(self)


if __name__ == "__main__":
    args = parse_args()
    scenario = args.scenario

    trees = {
        "no_suspension": NoSuspensionAsyncTree,
        "suspense_all": SuspenseAllAsyncTree,
        "memoization": MemoizationAsyncTree,
        "cpu_io_mixed": CpuIoMixedAsyncTree,
    }
    async_tree_class = trees[scenario]
    async_tree = async_tree_class(args.memoizable_percentage, args.cpu_probability)

    start_time = time.perf_counter()
    async_tree.run(args.eager)
    end_time = time.perf_counter()

    if args.print:
        print(f"Scenario: {scenario}")
        print(f"Time: {end_time - start_time} s")
        print(f"Tasks created: {async_tree.task_count}")
        print(f"Suspense called: {async_tree.suspense_count}")
