import os
import signal
import threading
import time
import multiprocessing
import sys

def sigint_self():
    time.sleep(1)
    print(f'{threading.current_thread().name}: Stopping PID {os.getpid()} with SIGINT')
    os.kill(os.getpid(), signal.SIGINT)

def sigkill_self():
    time.sleep(5)
    print(f'{threading.current_thread().name}: Killing PID {os.getpid()} with SIGKILL')
    os.kill(os.getpid(), signal.SIGKILL)

def run_signal_handler_dedicated_thread():
    event = multiprocessing.Event()
    def sigint_handler(_signo, _stack_frame):
        try:
            # print(f'{threading.current_thread().name}: sigint_handler raising SIGUSR1 ...')
            # signal.raise_signal(signal.SIGUSR1)
            # print(f'{threading.current_thread().name}: sigint_handler raising SIGUSR1 ... OK')
            sys.exit()
        finally:
            print(f'{threading.current_thread().name}: sigint_handler exiting ...')

    def sigusr1_handler(_signo, _):
        print(f'{threading.current_thread().name}: USR1 running')

    def sigterm_handler(_signo, _stack_frame):
        print(f'{threading.current_thread().name}: sigterm_handler is running')
        pass

    signal.signal(signal.SIGINT, sigint_handler, True)
    signal.signal(signal.SIGUSR1, sigusr1_handler, True)

    # signal.raise_signal(signal.SIGINT)

    threading.Thread(target=sigint_self, daemon=True).start()
    threading.Thread(target=sigkill_self, daemon=True).start() # Used for debugging only.

    print(f'{threading.current_thread().name}: Waiting on event. PID = {os.getpid()}')
    while True:
        if event.is_set():
            break
        else:
            time.sleep(0.1)
    print(f'{threading.current_thread().name}: Waiting is done')

if __name__ == '__main__':
    try:
        run_signal_handler_dedicated_thread()
    finally:
        print(f'{threading.current_thread().name}: Exiting')
