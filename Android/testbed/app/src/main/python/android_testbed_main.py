import os
import runpy
import shlex
import signal
import sys

# Some tests use SIGUSR1, but that's blocked by default in an Android app in
# order to make it available to `sigwait` in the Signal Catcher thread.
# (https://cs.android.com/android/platform/superproject/+/android14-qpr3-release:art/runtime/signal_catcher.cc).
# That thread's functionality is only useful for debugging the JVM, so disabling
# it should not weaken the tests.
#
# There's no safe way of stopping the thread completely (#123982), but simply
# unblocking SIGUSR1 is enough to fix most tests.
#
# However, in tests that generate multiple different signals in quick
# succession, it's possible for SIGUSR1 to arrive while the main thread is busy
# running the C-level handler for a different signal. In that case, the SIGUSR1
# may be sent to the Signal Catcher thread instead, which will generate a log
# message containing the text "reacting to signal".
#
# Such tests may need to be changed in one of the following ways:
#   * Use a signal other than SIGUSR1 (e.g. test_stress_delivery_simultaneous in
#     test_signal.py).
#   * Send the signal to a specific thread rather than the whole process (e.g.
#     test_signals in test_threadsignals.py.
signal.pthread_sigmask(signal.SIG_UNBLOCK, [signal.SIGUSR1])

mode = os.environ["PYTHON_MODE"]
module = os.environ["PYTHON_MODULE"]
sys.argv[1:] = shlex.split(os.environ["PYTHON_ARGS"])

cwd = f"{sys.prefix}/cwd"
if not os.path.exists(cwd):
    # Empty directories are lost in the asset packing/unpacking process.
    os.mkdir(cwd)
os.chdir(cwd)

if mode == "-c":
    # In -c mode, sys.path starts with an empty string, which means whatever the current
    # working directory is at the moment of each import.
    sys.path.insert(0, "")
    exec(module, {})
elif mode == "-m":
    sys.path.insert(0, os.getcwd())
    runpy.run_module(module, run_name="__main__", alter_sys=True)
else:
    raise ValueError(f"unknown mode: {mode}")
