# Copyright 2007 Google, Inc. All Rights Reserved.
# Licensed to PSF under a Contributor Agreement.

"""Tests for the raise statement."""

from test import test_support
import sys
import types
import unittest


def get_tb():
    try:
        raise OSError()
    except:
        return sys.exc_info()[2]


class TestRaise(unittest.TestCase):
    def test_invalid_reraise(self):
        try:
            raise
        except RuntimeError as e:
            self.failUnless("No active exception" in str(e))
        else:
            self.fail("No exception raised")

    def test_reraise(self):
        try:
            try:
                raise IndexError()
            except IndexError as e:
                exc1 = e
                raise
        except IndexError as exc2:
            self.failUnless(exc1 is exc2)
        else:
            self.fail("No exception raised")

    def test_erroneous_exception(self):
        class MyException(Exception):
            def __init__(self):
                raise RuntimeError()

        try:
            raise MyException
        except RuntimeError:
            pass
        else:
            self.fail("No exception raised")


class TestCause(unittest.TestCase):
    def test_invalid_cause(self):
        try:
            raise IndexError from 5
        except TypeError as e:
            self.failUnless("exception cause" in str(e))
        else:
            self.fail("No exception raised")

    def test_class_cause(self):
        try:
            raise IndexError from KeyError
        except IndexError as e:
            self.failUnless(isinstance(e.__cause__, KeyError))
        else:
            self.fail("No exception raised")

    def test_instance_cause(self):
        cause = KeyError()
        try:
            raise IndexError from cause
        except IndexError as e:
            self.failUnless(e.__cause__ is cause)
        else:
            self.fail("No exception raised")

    def test_erroneous_cause(self):
        class MyException(Exception):
            def __init__(self):
                raise RuntimeError()

        try:
            raise IndexError from MyException
        except RuntimeError:
            pass
        else:
            self.fail("No exception raised")


class TestTraceback(unittest.TestCase):
    def test_sets_traceback(self):
        try:
            raise IndexError()
        except IndexError as e:
            self.failUnless(isinstance(e.__traceback__, types.TracebackType))
        else:
            self.fail("No exception raised")

    def test_accepts_traceback(self):
        tb = get_tb()
        try:
            raise IndexError().with_traceback(tb)
        except IndexError as e:
            self.assertNotEqual(e.__traceback__, tb)
            self.assertEqual(e.__traceback__.tb_next, tb)
        else:
            self.fail("No exception raised")


# Disabled until context is implemented
# class TestContext(object):
#     def test_instance_context_bare_raise(self):
#         context = IndexError()
#         try:
#             try:
#                 raise context
#             except:
#                 raise OSError()
#         except OSError as e:
#             self.assertEqual(e.__context__, context)
#         else:
#             self.fail("No exception raised")
#
#     def test_class_context_bare_raise(self):
#         context = IndexError
#         try:
#             try:
#                 raise context
#             except:
#                 raise OSError()
#         except OSError as e:
#             self.assertNotEqual(e.__context__, context)
#             self.failUnless(isinstance(e.__context__, context))
#         else:
#             self.fail("No exception raised")


class TestRemovedFunctionality(unittest.TestCase):
    def test_tuples(self):
        try:
            raise (IndexError, KeyError) # This should be a tuple!
        except TypeError:
            pass
        else:
            self.fail("No exception raised")

    def test_strings(self):
        try:
            raise "foo"
        except TypeError:
            pass
        else:
            self.fail("No exception raised")


def test_main():
    test_support.run_unittest(__name__)


if __name__ == "__main__":
    unittest.main()
