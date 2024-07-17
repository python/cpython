import runpy
import signal
import sys

# Some tests use SIGUSR1, but that's blocked by default in an Android app in
# order to make it available to `sigwait` in the "Signal Catcher" thread. That
# thread's functionality is only relevant to the JVM ("forcing GC (no HPROF) and
# profile save"), so disabling it should not weaken the tests.
signal.pthread_sigmask(signal.SIG_UNBLOCK, [signal.SIGUSR1])

# To run specific tests, or pass any other arguments to the test suite, edit
# this command line.
sys.argv[1:] = [
    "--use", "all,-cpu",
    "--verbose3",
]
runpy.run_module("test")
