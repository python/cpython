import os
import runpy
import shlex
import signal
import sys

# Some tests use SIGUSR1, but that's blocked by default in an Android app in
# order to make it available to `sigwait` in the "Signal Catcher" thread. That
# thread's functionality is only relevant to the JVM ("forcing GC (no HPROF) and
# profile save"), so disabling it should not weaken the tests.
signal.pthread_sigmask(signal.SIG_UNBLOCK, [signal.SIGUSR1])

sys.argv[1:] = shlex.split(os.environ["PYTHON_ARGS"])

# The test module will call sys.exit to indicate whether the tests passed.
runpy.run_module("test")
