import os
import runpy
import shlex
import signal
import sys

# Some tests use SIGUSR1, but that's blocked by default in an Android app in
# order to make it available to `sigwait` in the Signal Catcher thread. That
# thread's functionality is only useful for debugging the JVM, so disabling it
# should not weaken the tests.
#
# Simply unblocking SIGUSR1 is enough to fix most tests that use it. But in
# tests that generate multiple different signals in quick succession, it's
# possible for SIGUSR1 to arrive while the main thread is busy running the
# C-level handler for a different signal, in which case the SIGUSR1 may be sent
# to the Signal Catcher thread instead. When this happens, there will be a log
# message containing the text "reacting to signal".
#
# In such cases, the tests may need to be changed. Possible workarounds include:
#   * Use a signal other than SIGUSR1 (e.g. test_stress_delivery_simultaneous in
#     test_signal.py).
#   * Send the signal to a specific thread rather than the whole process (e.g.
#     test_signals in test_threadsignals.py.
signal.pthread_sigmask(signal.SIG_UNBLOCK, [signal.SIGUSR1])

sys.argv[1:] = shlex.split(os.environ["PYTHON_ARGS"])

# The test module will call sys.exit to indicate whether the tests passed.
runpy.run_module("test")
