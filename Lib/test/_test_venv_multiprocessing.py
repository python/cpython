import multiprocessing
import random
import sys
import time

def fill_queue(queue, code):
    queue.put(code)


def drain_queue(queue, code):
    if code != queue.get():
        sys.exit(1)


def test_func():
    code = random.randrange(0, 1000)
    queue = multiprocessing.Queue()
    fill_pool = multiprocessing.Process(
        target=fill_queue,
        args=(queue, code)
    )
    drain_pool = multiprocessing.Process(
        target=drain_queue,
        args=(queue, code)
    )
    drain_pool.start()
    fill_pool.start()
    fill_pool.join()
    drain_pool.join()


def main():
    test_pool = multiprocessing.Process(target=test_func)
    test_pool.start()
    test_pool.join()
    sys.exit(test_pool.exitcode)


if __name__ == "__main__":
    main()
