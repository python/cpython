# Measure the performance of PyMutex and PyThread_type_lock locks
# with short critical sections.
#
# Usage: python Tools/lockbench/lockbench.py [CRITICAL_SECTION_LENGTH]
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

from _testinternalcapi import benchmark_locks
import sys

# Max number of threads to test
MAX_THREADS = 10

# How much "work" to do while holding the lock
CRITICAL_SECTION_LENGTH = 1


def jains_fairness(values):
    # Jain's fairness index
    # See https://en.wikipedia.org/wiki/Fairness_measure
    return (sum(values) ** 2) / (len(values) * sum(x ** 2 for x in values))

def main():
    print("Lock Type           Threads           Acquisitions (kHz)   Fairness")
    for lock_type in ["PyMutex", "PyThread_type_lock"]:
        use_pymutex = (lock_type == "PyMutex")
        for num_threads in range(1, MAX_THREADS + 1):
            acquisitions, thread_iters = benchmark_locks(
                num_threads, use_pymutex, CRITICAL_SECTION_LENGTH)

            acquisitions /= 1000  # report in kHz for readability
            fairness = jains_fairness(thread_iters)

            print(f"{lock_type: <20}{num_threads: <18}{acquisitions: >5.0f}{fairness: >20.2f}")


if __name__ == "__main__":
    if len(sys.argv) > 1:
        CRITICAL_SECTION_LENGTH = int(sys.argv[1])
    main()
