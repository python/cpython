"""Unit tests for contextlib.py, and other context managers."""

from __future__ import with_statement

import os
import decimal
import tempfile
import unittest
import threading
from contextlib import *  # Tests __all__

class ContextManagerTestCase(unittest.TestCase):

    def test_contextmanager_plain(self):
        state = []
        @contextmanager
        def woohoo():
            state.append(1)
            yield 42
            state.append(999)
        with woohoo() as x:
            self.assertEqual(state, [1])
            self.assertEqual(x, 42)
            state.append(x)
        self.assertEqual(state, [1, 42, 999])

    def test_contextmanager_finally(self):
        state = []
        @contextmanager
        def woohoo():
            state.append(1)
            try:
                yield 42
            finally:
                state.append(999)
        try:
            with woohoo() as x:
                self.assertEqual(state, [1])
                self.assertEqual(x, 42)
                state.append(x)
                raise ZeroDivisionError()
        except ZeroDivisionError:
            pass
        else:
            self.fail("Expected ZeroDivisionError")
        self.assertEqual(state, [1, 42, 999])

    def test_contextmanager_except(self):
        state = []
        @contextmanager
        def woohoo():
            state.append(1)
            try:
                yield 42
            except ZeroDivisionError, e:
                state.append(e.args[0])
                self.assertEqual(state, [1, 42, 999])
        with woohoo() as x:
            self.assertEqual(state, [1])
            self.assertEqual(x, 42)
            state.append(x)
            raise ZeroDivisionError(999)
        self.assertEqual(state, [1, 42, 999])

class NestedTestCase(unittest.TestCase):

    # XXX This needs more work

    def test_nested(self):
        @contextmanager
        def a():
            yield 1
        @contextmanager
        def b():
            yield 2
        @contextmanager
        def c():
            yield 3
        with nested(a(), b(), c()) as (x, y, z):
            self.assertEqual(x, 1)
            self.assertEqual(y, 2)
            self.assertEqual(z, 3)

    def test_nested_cleanup(self):
        state = []
        @contextmanager
        def a():
            state.append(1)
            try:
                yield 2
            finally:
                state.append(3)
        @contextmanager
        def b():
            state.append(4)
            try:
                yield 5
            finally:
                state.append(6)
        try:
            with nested(a(), b()) as (x, y):
                state.append(x)
                state.append(y)
                1/0
        except ZeroDivisionError:
            self.assertEqual(state, [1, 4, 2, 5, 6, 3])
        else:
            self.fail("Didn't raise ZeroDivisionError")

class ClosingTestCase(unittest.TestCase):

    # XXX This needs more work

    def test_closing(self):
        state = []
        class C:
            def close(self):
                state.append(1)
        x = C()
        self.assertEqual(state, [])
        with closing(x) as y:
            self.assertEqual(x, y)
        self.assertEqual(state, [1])

    def test_closing_error(self):
        state = []
        class C:
            def close(self):
                state.append(1)
        x = C()
        self.assertEqual(state, [])
        try:
            with closing(x) as y:
                self.assertEqual(x, y)
                1/0
        except ZeroDivisionError:
            self.assertEqual(state, [1])
        else:
            self.fail("Didn't raise ZeroDivisionError")

class FileContextTestCase(unittest.TestCase):

    def testWithOpen(self):
        tfn = tempfile.mktemp()
        try:
            f = None
            with open(tfn, "w") as f:
                self.failIf(f.closed)
                f.write("Booh\n")
            self.failUnless(f.closed)
            f = None
            try:
                with open(tfn, "r") as f:
                    self.failIf(f.closed)
                    self.assertEqual(f.read(), "Booh\n")
                    1/0
            except ZeroDivisionError:
                self.failUnless(f.closed)
            else:
                self.fail("Didn't raise ZeroDivisionError")
        finally:
            try:
                os.remove(tfn)
            except os.error:
                pass

class LockContextTestCase(unittest.TestCase):

    def boilerPlate(self, lock, locked):
        self.failIf(locked())
        with lock:
            self.failUnless(locked())
        self.failIf(locked())
        try:
            with lock:
                self.failUnless(locked())
                1/0
        except ZeroDivisionError:
            self.failIf(locked())
        else:
            self.fail("Didn't raise ZeroDivisionError")

    def testWithLock(self):
        lock = threading.Lock()
        self.boilerPlate(lock, lock.locked)

    def testWithRLock(self):
        lock = threading.RLock()
        self.boilerPlate(lock, lock._is_owned)

    def testWithCondition(self):
        lock = threading.Condition()
        def locked():
            return lock._is_owned()
        self.boilerPlate(lock, locked)

    def testWithSemaphore(self):
        lock = threading.Semaphore()
        def locked():
            if lock.acquire(False):
                lock.release()
                return False
            else:
                return True
        self.boilerPlate(lock, locked)

    def testWithBoundedSemaphore(self):
        lock = threading.BoundedSemaphore()
        def locked():
            if lock.acquire(False):
                lock.release()
                return False
            else:
                return True
        self.boilerPlate(lock, locked)

class DecimalContextTestCase(unittest.TestCase):

    # XXX Somebody should write more thorough tests for this

    def testBasic(self):
        ctx = decimal.getcontext()
        ctx.prec = save_prec = decimal.ExtendedContext.prec + 5
        with decimal.ExtendedContext:
            self.assertEqual(decimal.getcontext().prec,
                             decimal.ExtendedContext.prec)
        self.assertEqual(decimal.getcontext().prec, save_prec)
        try:
            with decimal.ExtendedContext:
                self.assertEqual(decimal.getcontext().prec,
                                 decimal.ExtendedContext.prec)
                1/0
        except ZeroDivisionError:
            self.assertEqual(decimal.getcontext().prec, save_prec)
        else:
            self.fail("Didn't raise ZeroDivisionError")


if __name__ == "__main__":
    unittest.main()
