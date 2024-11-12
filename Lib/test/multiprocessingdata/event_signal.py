import multiprocessing
import os
import signal
import concurrent.futures


def send_sigint(pid):
    os.kill(pid, signal.SIGINT)


def run_signal_handler_test():
    shutdown_event = multiprocessing.Event()

    def sigterm_handler(_signo, _stack_frame):
        shutdown_event.set()

    signal.signal(signal.SIGINT, sigterm_handler)

    with concurrent.futures.ProcessPoolExecutor() as executor:
        f = executor.submit(send_sigint, os.getpid())
        while not shutdown_event.is_set():
            pass
        f.result()


if __name__ == '__main__':
    run_signal_handler_test()
