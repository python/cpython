# Measure the performance of PyMutex
# with short critical sections.
#
# How to interpret the results:
#
# Acquisitions (kHz): Reports the total number of lock acquisitions in
# thousands of acquisitions per second. This is the most important metric,
# particularly for the 1 thread case because even in multithreaded programs,
# most locks acquisitions are not contended. Values for 2+ threads are
# only meaningful for `--disable-gil` builds, because the GIL prevents most
# situations where there is lock contention with short critical sections.
#
# Fairness: A measure of how evenly the lock acquisitions are distributed.
# A fairness of 1.0 means that all threads acquired the lock the same number
# of times. A fairness of 1/N means that only one thread ever acquired the
# lock.
# See https://en.wikipedia.org/wiki/Fairness_measure#Jain's_fairness_index

import argparse
from _testinternalcapi import benchmark_locks

def jains_fairness(values):
    # Jain's fairness index
    # See https://en.wikipedia.org/wiki/Fairness_measure
    if not values:
        return 0.0
    return (sum(values) ** 2) / (len(values) * sum(x ** 2 for x in values))

def main():
    parser = argparse.ArgumentParser(description="Measure the performance of PyMutex")
    parser.add_argument("threads", type=int, nargs="?", default=1,
                        help="Number of threads")
    parser.add_argument("--num-locks", type=int, default=1,
                        help="Number of locks")
    parser.add_argument("--critical-section", type=int, default=1,
                        help="Work inside the lock")
    parser.add_argument("--work-outside", type=int, default=0,
                        help="Work outside the lock")
    parser.add_argument("--time", type=int, default=1000,
                        help="Benchmark duration in milliseconds")
    parser.add_argument("--total-iters", type=int, default=0,
                        help="Fixed number of iterations per thread")

    args = parser.parse_args()

    acquisitions, thread_iters = benchmark_locks(
        args.threads,
        num_locks=args.num_locks,
        critical_section_length=args.critical_section,
        work_outside_length=args.work_outside,
        time_ms=args.time,
        iters_limit=args.total_iters
    )

    acquisitions /= 1000  # report in kHz for readability
    fairness = jains_fairness(thread_iters)

    print(f"Threads:            {args.threads}")
    print(f"Locks:              {args.num_locks}")
    print(f"Acquisitions (kHz): {acquisitions: >5.0f}")
    print(f"Fairness:           {fairness: >20.2f}")

if __name__ == "__main__":
    main()
