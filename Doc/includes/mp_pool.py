#
# A test of `multiprocessing.Pool` class
#
# Copyright (c) 2006-2008, R Oudkerk
# All rights reserved.
#

import multiprocessing
import time
import random
import sys

#
# Functions used by test code
#

def calculate(func, args):
    result = func(*args)
    return '%s says that %s%s = %s' % (
        multiprocessing.current_process().name,
        func.__name__, args, result
        )

def calculatestar(args):
    return calculate(*args)

def mul(a, b):
    time.sleep(0.5 * random.random())
    return a * b

def plus(a, b):
    time.sleep(0.5 * random.random())
    return a + b

def f(x):
    return 1.0 / (x - 5.0)

def pow3(x):
    return x ** 3

def noop(x):
    pass

#
# Test code
#

def test():
    print('cpu_count() = %d\n' % multiprocessing.cpu_count())

    #
    # Create pool
    #

    PROCESSES = 4
    print('Creating pool with %d processes\n' % PROCESSES)
    pool = multiprocessing.Pool(PROCESSES)
    print('pool = %s' % pool)
    print()

    #
    # Tests
    #

    TASKS = [(mul, (i, 7)) for i in range(10)] + \
            [(plus, (i, 8)) for i in range(10)]

    results = [pool.apply_async(calculate, t) for t in TASKS]
    imap_it = pool.imap(calculatestar, TASKS)
    imap_unordered_it = pool.imap_unordered(calculatestar, TASKS)

    print('Ordered results using pool.apply_async():')
    for r in results:
        print('\t', r.get())
    print()

    print('Ordered results using pool.imap():')
    for x in imap_it:
        print('\t', x)
    print()

    print('Unordered results using pool.imap_unordered():')
    for x in imap_unordered_it:
        print('\t', x)
    print()

    print('Ordered results using pool.map() --- will block till complete:')
    for x in pool.map(calculatestar, TASKS):
        print('\t', x)
    print()

    #
    # Simple benchmarks
    #

    N = 100000
    print('def pow3(x): return x**3')

    t = time.time()
    A = list(map(pow3, range(N)))
    print('\tmap(pow3, range(%d)):\n\t\t%s seconds' % \
          (N, time.time() - t))

    t = time.time()
    B = pool.map(pow3, range(N))
    print('\tpool.map(pow3, range(%d)):\n\t\t%s seconds' % \
          (N, time.time() - t))

    t = time.time()
    C = list(pool.imap(pow3, range(N), chunksize=N//8))
    print('\tlist(pool.imap(pow3, range(%d), chunksize=%d)):\n\t\t%s' \
          ' seconds' % (N, N//8, time.time() - t))

    assert A == B == C, (len(A), len(B), len(C))
    print()

    L = [None] * 1000000
    print('def noop(x): pass')
    print('L = [None] * 1000000')

    t = time.time()
    A = list(map(noop, L))
    print('\tmap(noop, L):\n\t\t%s seconds' % \
          (time.time() - t))

    t = time.time()
    B = pool.map(noop, L)
    print('\tpool.map(noop, L):\n\t\t%s seconds' % \
          (time.time() - t))

    t = time.time()
    C = list(pool.imap(noop, L, chunksize=len(L)//8))
    print('\tlist(pool.imap(noop, L, chunksize=%d)):\n\t\t%s seconds' % \
          (len(L)//8, time.time() - t))

    assert A == B == C, (len(A), len(B), len(C))
    print()

    del A, B, C, L

    #
    # Test error handling
    #

    print('Testing error handling:')

    try:
        print(pool.apply(f, (5,)))
    except ZeroDivisionError:
        print('\tGot ZeroDivisionError as expected from pool.apply()')
    else:
        raise AssertionError('expected ZeroDivisionError')

    try:
        print(pool.map(f, list(range(10))))
    except ZeroDivisionError:
        print('\tGot ZeroDivisionError as expected from pool.map()')
    else:
        raise AssertionError('expected ZeroDivisionError')

    try:
        print(list(pool.imap(f, list(range(10)))))
    except ZeroDivisionError:
        print('\tGot ZeroDivisionError as expected from list(pool.imap())')
    else:
        raise AssertionError('expected ZeroDivisionError')

    it = pool.imap(f, list(range(10)))
    for i in range(10):
        try:
            x = next(it)
        except ZeroDivisionError:
            if i == 5:
                pass
        except StopIteration:
            break
        else:
            if i == 5:
                raise AssertionError('expected ZeroDivisionError')

    assert i == 9
    print('\tGot ZeroDivisionError as expected from IMapIterator.next()')
    print()

    #
    # Testing timeouts
    #

    print('Testing ApplyResult.get() with timeout:', end=' ')
    res = pool.apply_async(calculate, TASKS[0])
    while 1:
        sys.stdout.flush()
        try:
            sys.stdout.write('\n\t%s' % res.get(0.02))
            break
        except multiprocessing.TimeoutError:
            sys.stdout.write('.')
    print()
    print()

    print('Testing IMapIterator.next() with timeout:', end=' ')
    it = pool.imap(calculatestar, TASKS)
    while 1:
        sys.stdout.flush()
        try:
            sys.stdout.write('\n\t%s' % it.next(0.02))
        except StopIteration:
            break
        except multiprocessing.TimeoutError:
            sys.stdout.write('.')
    print()
    print()

    #
    # Testing callback
    #

    print('Testing callback:')

    A = []
    B = [56, 0, 1, 8, 27, 64, 125, 216, 343, 512, 729]

    r = pool.apply_async(mul, (7, 8), callback=A.append)
    r.wait()

    r = pool.map_async(pow3, list(range(10)), callback=A.extend)
    r.wait()

    if A == B:
        print('\tcallbacks succeeded\n')
    else:
        print('\t*** callbacks failed\n\t\t%s != %s\n' % (A, B))

    #
    # Check there are no outstanding tasks
    #

    assert not pool._cache, 'cache = %r' % pool._cache

    #
    # Check close() methods
    #

    print('Testing close():')

    for worker in pool._pool:
        assert worker.is_alive()

    result = pool.apply_async(time.sleep, [0.5])
    pool.close()
    pool.join()

    assert result.get() is None

    for worker in pool._pool:
        assert not worker.is_alive()

    print('\tclose() succeeded\n')

    #
    # Check terminate() method
    #

    print('Testing terminate():')

    pool = multiprocessing.Pool(2)
    DELTA = 0.1
    ignore = pool.apply(pow3, [2])
    results = [pool.apply_async(time.sleep, [DELTA]) for i in range(100)]
    pool.terminate()
    pool.join()

    for worker in pool._pool:
        assert not worker.is_alive()

    print('\tterminate() succeeded\n')

    #
    # Check garbage collection
    #

    print('Testing garbage collection:')

    pool = multiprocessing.Pool(2)
    DELTA = 0.1
    processes = pool._pool
    ignore = pool.apply(pow3, [2])
    results = [pool.apply_async(time.sleep, [DELTA]) for i in range(100)]

    results = pool = None

    time.sleep(DELTA * 2)

    for worker in processes:
        assert not worker.is_alive()

    print('\tgarbage collection succeeded\n')


if __name__ == '__main__':
    multiprocessing.freeze_support()

    assert len(sys.argv) in (1, 2)

    if len(sys.argv) == 1 or sys.argv[1] == 'processes':
        print(' Using processes '.center(79, '-'))
    elif sys.argv[1] == 'threads':
        print(' Using threads '.center(79, '-'))
        import multiprocessing.dummy as multiprocessing
    else:
        print('Usage:\n\t%s [processes | threads]' % sys.argv[0])
        raise SystemExit(2)

    test()
