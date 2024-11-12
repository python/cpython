import multiprocessing
import os
import signal
import concurrent.futures


def send_sigint(pid):
    print('send_sigint running')
    os.kill(pid, signal.SIGINT)


def run_signal_handler_test():
    shutdown_event = multiprocessing.Event()

    def sigterm_handler(_signo, _stack_frame):
        try:
            print('sigterm_handler running')
            shutdown_event.set()
        finally:
            print('sigterm_handler done')

    signal.signal(signal.SIGINT, sigterm_handler)

    with concurrent.futures.ProcessPoolExecutor() as executor:
        f = executor.submit(send_sigint, os.getpid())
        while not shutdown_event.is_set():
            pass
        print('Received shutdown event')
        f.result()


if __name__ == '__main__':
    run_signal_handler_test()
