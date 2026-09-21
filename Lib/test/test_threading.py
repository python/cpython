"""
Tests for the threading module.
"""

import test.support
from test.support import threading_helper, requires_subprocess, requires_gil_enabled
from test.support import verbose, cpython_only, os_helper
from test.support.import_helper import ensure_lazy_imports, import_module
from test.support.script_helper import assert_python_ok, assert_python_failure, spawn_python
from test.support import force_not_colorized

import random
import sys
import _thread
import threading
import time
import unittest
import weakref
import os
import subprocess
import signal
import textwrap
import traceback
import warnings

from unittest import mock
from test import lock_tests
from test import support

try:
    from concurrent import interpreters
except ImportError:
    interpreters = None

threading_helper.requires_working_threading(module=True)

# Between fork() and exec(), only async-safe functions are allowed (issues
# #12316 and #11870), and fork() from a worker thread is known to trigger
# problems with some operating systems (issue #3863): skip problematic tests
# on platforms known to behave badly.
platforms_to_skip = ('netbsd5', 'hp-ux11')


def skip_unless_reliable_fork(test):
    if not support.has_fork_support:
        return unittest.skip("requires working os.fork()")(test)
    if sys.platform in platforms_to_skip:
        return unittest.skip("due to known OS bug related to thread+fork")(test)
    if support.HAVE_ASAN_FORK_BUG:
        return unittest.skip("libasan has a pthread_create() dead lock related to thread+fork")(test)
    if support.check_sanitizer(thread=True):
        return unittest.skip("TSAN doesn't support threads after fork")(test)
    return test


def requires_subinterpreters(meth):
    """Decorator to skip a test if subinterpreters are not supported."""
    return unittest.skipIf(interpreters is None,
                           'subinterpreters required')(meth)


def restore_default_excepthook(testcase):
    testcase.addCleanup(setattr, threading, 'excepthook', threading.excepthook)
    threading.excepthook = threading.__excepthook__


# A trivial mutable counter.
class Counter(object):
    def __init__(self):
        self.value = 0
    def inc(self):
        self.value += 1
    def dec(self):
        self.value -= 1
    def get(self):
        return self.value

class TestThread(threading.Thread):
    def __init__(self, name, testcase, sema, mutex, nrunning):
        threading.Thread.__init__(self, name=name)
        self.testcase = testcase
        self.sema = sema
        self.mutex = mutex
        self.nrunning = nrunning

    def run(self):
        delay = random.random() / 10000.0
        if verbose:
            print('task %s will run for %.1f usec' %
                  (self.name, delay * 1e6))

        with self.sema:
            with self.mutex:
                self.nrunning.inc()
                if verbose:
                    print(self.nrunning.get(), 'tasks are running')
                self.testcase.assertLessEqual(self.nrunning.get(), 3)

            time.sleep(delay)
            if verbose:
                print('task', self.name, 'done')

            with self.mutex:
                self.nrunning.dec()
                self.testcase.assertGreaterEqual(self.nrunning.get(), 0)
                if verbose:
                    print('%s is finished. %d tasks are running' %
                          (self.name, self.nrunning.get()))


class BaseTestCase(unittest.TestCase):
    def setUp(self):
        self._threads = threading_helper.threading_setup()

    def tearDown(self):
        threading_helper.threading_cleanup(*self._threads)
        test.support.reap_children()


class ThreadTests(BaseTestCase):
    maxDiff = 9999

    @cpython_only
    def test_lazy_import(self):
        ensure_lazy_imports("threading", {"functools", "warnings"})

    @cpython_only
    def test_name(self):
        def func(): pass

        thread = threading.Thread(name="myname1")
        self.assertEqual(thread.name, "myname1")

        # Convert int name to str
        thread = threading.Thread(name=123)
        self.assertEqual(thread.name, "123")

        # target name is ignored if name is specified
        thread = threading.Thread(target=func, name="myname2")
        self.assertEqual(thread.name, "myname2")

        with mock.patch.object(threading, '_counter', return_value=2):
            thread = threading.Thread(name="")
            self.assertEqual(thread.name, "Thread-2")

        with mock.patch.object(threading, '_counter', return_value=3):
            thread = threading.Thread()
            self.assertEqual(thread.name, "Thread-3")

        with mock.patch.object(threading, '_counter', return_value=5):
            thread = threading.Thread(target=func)
            self.assertEqual(thread.name, "Thread-5 (func)")

    def test_args_argument(self):
        # bpo-45735: Using list or tuple as *args* in constructor could
        # achieve the same effect.
        num_list = [1]
