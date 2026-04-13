import signal, subprocess, sys, time
# On Linux this causes os.waitpid to fail with OSError as the OS has already
# reaped our child process.  The wait() passing the OSError on to the caller
# and causing us to exit with an error is what we are testing against.
signal.signal(signal.SIGCHLD, signal.SIG_IGN)
subprocess.Popen([sys.executable, '-c', 'print("albatross")']).wait()
# Also ensure poll() handles an errno.ECHILD appropriately.
p = subprocess.Popen([sys.executable, '-c', 'print("albatross")'])
num_polls = 0
while p.poll() is None:
    # Waiting for the process to finish.
    time.sleep(0.01)  # Avoid being a CPU busy loop.
    num_polls += 1
    if num_polls > 3000:
        raise RuntimeError('poll should have returned 0 within 30 seconds')
