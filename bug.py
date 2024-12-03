import os
import signal
import threading
import time
import multiprocessing

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
            x = 1 / 0
            print(f'{threading.current_thread().name}: sigint_handler is setting event')
            event.set()
        finally:
            print(f'{threading.current_thread().name}: sigint_handler is done')

    def sigterm_handler(_signo, _stack_frame):
        print(f'{threading.current_thread().name}: sigterm_handler is running')
        pass

    signal.signal(signal.SIGTERM, sigterm_handler)
    signal.signal(signal.SIGINT, sigint_handler)

    threading.Thread(target=sigint_self, daemon=True).start()
    threading.Thread(target=sigkill_self, daemon=True).start() # Used for debugging only.

    print(f'{threading.current_thread().name}: Waiting on event. PID = {os.getpid()}')
    event.wait()
    print(f'{threading.current_thread().name}: Waiting is done')

if __name__ == '__main__':
    try:
        run_signal_handler_dedicated_thread()
    finally:
        print(f'{threading.current_thread().name}: Exiting')
