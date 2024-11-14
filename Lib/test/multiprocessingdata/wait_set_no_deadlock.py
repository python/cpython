import multiprocessing
import signal
import concurrent.futures
import time
import os


# Shows that https://github.com/python/cpython/issues/85772 is fixed

def send_sigint(pid):
    time.sleep(1) # Make sure shutdown_event.wait() is called
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
    run_signal_handler_wait_set_test()
