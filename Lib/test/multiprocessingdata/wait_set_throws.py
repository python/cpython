import multiprocessing
import signal
import concurrent.futures
import time
import sys
import os


def send_sigint(pid):
    time.sleep(1)
    os.kill(pid, signal.SIGINT)


def run_signal_handler_wait_set_test():
    shutdown_event = multiprocessing.Event()

    def sigterm_handler(_signo, _stack_frame):
        shutdown_event.set()

    signal.signal(signal.SIGINT, sigterm_handler)

    with concurrent.futures.ProcessPoolExecutor() as executor:
        f = executor.submit(send_sigint, os.getpid())
        shutdown_event.wait()
        f.result()


if __name__ == '__main__':
    try:
        run_signal_handler_wait_set_test()
        sys.exit(1)
    except AssertionError as e:
        assert 'multiprocessing.Event.set() cannot be called from a thread that is already wait()-ing' in str(e)
        sys.exit(0)
