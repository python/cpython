# pysqlite2/test/userfunctions.py: tests for user-defined functions and
#                                  aggregates.
#
# Copyright (C) 2005-2007 Gerhard Häring <gh@ghaering.de>
#
# This file is part of pysqlite.
#
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.

import sys
import unittest
import sqlite3 as sqlite

from unittest.mock import Mock, patch
from test.support import bigmemtest, gc_collect

from .util import cx_limit, memory_database
from .util import with_tracebacks


def func_returntext():
    return "foo"
def func_returntextwithnull():
    return "1\x002"
def func_returnunicode():
    return "bar"
def func_returnint():
    return 42
def func_returnfloat():
    return 3.14
def func_returnnull():
    return None
def func_returnblob():
    return b"blob"
def func_returnlonglong():
    return 1<<31
def func_raiseexception():
    5/0
def func_memoryerror():
    raise MemoryError
def func_overflowerror():
    raise OverflowError

class AggrNoStep:
    def __init__(self):
        pass

    def finalize(self):
        return 1

class AggrNoFinalize:
    def __init__(self):
        pass

    def step(self, x):
        pass

class AggrExceptionInInit:
    def __init__(self):
        5/0

    def step(self, x):
        pass

    def finalize(self):
        pass

class AggrExceptionInStep:
    def __init__(self):
        pass

    def step(self, x):
        5/0

    def finalize(self):
        return 42

class AggrExceptionInFinalize:
    def __init__(self):
        pass

    def step(self, x):
        pass

    def finalize(self):
        5/0

class AggrCheckType:
    def __init__(self):
        self.val = None

    def step(self, whichType, val):
        theType = {"str": str, "int": int, "float": float, "None": type(None),
                   "blob": bytes}
        self.val = int(theType[whichType] is type(val))

    def finalize(self):
        return self.val

class AggrCheckTypes:
    def __init__(self):
        self.val = 0

    def step(self, whichType, *vals):
        theType = {"str": str, "int": int, "float": float, "None": type(None),
                   "blob": bytes}
        for val in vals:
            self.val += int(theType[whichType] is type(val))

    def finalize(self):
        return self.val

class AggrSum:
    def __init__(self):
        self.val = 0.0

    def step(self, val):
        self.val += val

    def finalize(self):
        return self.val

class AggrText:
    def __init__(self):
        self.txt = ""
    def step(self, txt):
        self.txt = self.txt + txt
    def finalize(self):
        return self.txt


class FunctionTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")

        self.con.create_function("returntext", 0, func_returntext)
        self.con.create_function("returntextwithnull", 0, func_returntextwithnull)
        self.con.create_function("returnunicode", 0, func_returnunicode)
        self.con.create_function("returnint", 0, func_returnint)
        self.con.create_function("returnfloat", 0, func_returnfloat)
        self.con.create_function("returnnull", 0, func_returnnull)
        self.con.create_function("returnblob", 0, func_returnblob)
        self.con.create_function("returnlonglong", 0, func_returnlonglong)
        self.con.create_function("returnnan", 0, lambda: float("nan"))
        self.con.create_function("return_noncont_blob", 0,
                                 lambda: memoryview(b"blob")[::2])
        self.con.create_function("raiseexception", 0, func_raiseexception)
        self.con.create_function("memoryerror", 0, func_memoryerror)
        self.con.create_function("overflowerror", 0, func_overflowerror)

        self.con.create_function("isblob", 1, lambda x: isinstance(x, bytes))
        self.con.create_function("isnone", 1, lambda x: x is None)
        self.con.create_function("spam", -1, lambda *x: len(x))
        self.con.execute("create table test(t text)")

    def tearDown(self):
        self.con.close()

    def test_func_error_on_create(self):
        with self.assertRaisesRegex(sqlite.ProgrammingError, "not -100"):
            self.con.create_function("bla", -100, lambda x: 2*x)

    def test_func_too_many_args(self):
        category = sqlite.SQLITE_LIMIT_FUNCTION_ARG
        msg = "too many arguments on function"
        with cx_limit(self.con, category=category, limit=1):
            self.con.execute("select abs(-1)");
            with self.assertRaisesRegex(sqlite.OperationalError, msg):
                self.con.execute("select max(1, 2)");

    def test_func_ref_count(self):
        def getfunc():
            def f():
                return 1
            return f
        f = getfunc()
        globals()["foo"] = f
        # self.con.create_function("reftest", 0, getfunc())
        self.con.create_function("reftest", 0, f)
        cur = self.con.cursor()
        cur.execute("select reftest()")

    def test_func_return_text(self):
        cur = self.con.cursor()
        cur.execute("select returntext()")
        val = cur.fetchone()[0]
        self.assertEqual(type(val), str)
        self.assertEqual(val, "foo")

    def test_func_return_text_with_null_char(self):
        cur = self.con.cursor()
        res = cur.execute("select returntextwithnull()").fetchone()[0]
        self.assertEqual(type(res), str)
        self.assertEqual(res, "1\x002")

    def test_func_return_unicode(self):
        cur = self.con.cursor()
        cur.execute("select returnunicode()")
        val = cur.fetchone()[0]
        self.assertEqual(type(val), str)
        self.assertEqual(val, "bar")

    def test_func_return_int(self):
        cur = self.con.cursor()
        cur.execute("select returnint()")
        val = cur.fetchone()[0]
        self.assertEqual(type(val), int)
        self.assertEqual(val, 42)

    def test_func_return_float(self):
        cur = self.con.cursor()
        cur.execute("select returnfloat()")
        val = cur.fetchone()[0]
        self.assertEqual(type(val), float)
        if val < 3.139 or val > 3.141:
            self.fail("wrong value")

    def test_func_return_null(self):
        cur = self.con.cursor()
        cur.execute("select returnnull()")
        val = cur.fetchone()[0]
        self.assertEqual(type(val), type(None))
        self.assertEqual(val, None)

    def test_func_return_blob(self):
        cur = self.con.cursor()
        cur.execute("select returnblob()")
        val = cur.fetchone()[0]
        self.assertEqual(type(val), bytes)
        self.assertEqual(val, b"blob")

    def test_func_return_long_long(self):
        cur = self.con.cursor()
        cur.execute("select returnlonglong()")
        val = cur.fetchone()[0]
        self.assertEqual(val, 1<<31)

    def test_func_return_nan(self):
        cur = self.con.cursor()
        cur.execute("select returnnan()")
        self.assertIsNone(cur.fetchone()[0])

    @with_tracebacks(ZeroDivisionError, msg_regex="func_raiseexception")
    def test_func_exception(self):
        cur = self.con.cursor()
        with self.assertRaises(sqlite.OperationalError) as cm:
            cur.execute("select raiseexception()")
            cur.fetchone()
        self.assertEqual(str(cm.exception), 'user-defined function raised exception')

    @with_tracebacks(MemoryError, msg_regex="func_memoryerror")
    def test_func_memory_error(self):
        cur = self.con.cursor()
        with self.assertRaises(MemoryError):
            cur.execute("select memoryerror()")
            cur.fetchone()

    @with_tracebacks(OverflowError, msg_regex="func_overflowerror")
    def test_func_overflow_error(self):
        cur = self.con.cursor()
        with self.assertRaises(sqlite.DataError):
            cur.execute("select overflowerror()")
            cur.fetchone()

    def test_any_arguments(self):
        cur = self.con.cursor()
        cur.execute("select spam(?, ?)", (1, 2))
        val = cur.fetchone()[0]
        self.assertEqual(val, 2)

    def test_empty_blob(self):
        cur = self.con.execute("select isblob(x'')")
        self.assertTrue(cur.fetchone()[0])

    def test_nan_float(self):
        cur = self.con.execute("select isnone(?)", (float("nan"),))
        # SQLite has no concept of nan; it is converted to NULL
        self.assertTrue(cur.fetchone()[0])

    def test_too_large_int(self):
        err = "Python int too large to convert to SQLite INTEGER"
        self.assertRaisesRegex(OverflowError, err, self.con.execute,
                               "select spam(?)", (1 << 65,))

    def test_non_contiguous_blob(self):
        self.assertRaisesRegex(BufferError,
                               "underlying buffer is not C-contiguous",
                               self.con.execute, "select spam(?)",
                               (memoryview(b"blob")[::2],))

    @with_tracebacks(BufferError, regex="buffer.*contiguous")
    def test_return_non_contiguous_blob(self):
        with self.assertRaises(sqlite.OperationalError):
            cur = self.con.execute("select return_noncont_blob()")
            cur.fetchone()

    def test_param_surrogates(self):
        self.assertRaisesRegex(UnicodeEncodeError, "surrogates not allowed",
                               self.con.execute, "select spam(?)",
                               ("\ud803\ude6d",))

    def test_func_params(self):
        results = []
        def append_result(arg):
            results.append((arg, type(arg)))
        self.con.create_function("test_params", 1, append_result)

        dataset = [
            (42, int),
            (-1, int),
            (1234567890123456789, int),
            (4611686018427387905, int),  # 63-bit int with non-zero low bits
            (3.14, float),
            (float('inf'), float),
            ("text", str),
            ("1\x002", str),
            ("\u02e2q\u02e1\u2071\u1d57\u1d49", str),
            (b"blob", bytes),
            (bytearray(range(2)), bytes),
            (memoryview(b"blob"), bytes),
            (None, type(None)),
        ]
        for val, _ in dataset:
            cur = self.con.execute("select test_params(?)", (val,))
            cur.fetchone()
        self.assertEqual(dataset, results)

    # Regarding deterministic functions:
    #
    # Between 3.8.3 and 3.15.0, deterministic functions were only used to
    # optimize inner loops. From 3.15.0 and onward, deterministic functions
    # were permitted in WHERE clauses of partial indices, which allows testing
    # based on syntax, iso. the query optimizer.
    def test_func_non_deterministic(self):
        mock = Mock(return_value=None)
        self.con.create_function("nondeterministic", 0, mock, deterministic=False)
        with self.assertRaises(sqlite.OperationalError):
            self.con.execute("create index t on test(t) where nondeterministic() is not null")

    def test_func_deterministic(self):
        mock = Mock(return_value=None)
        self.con.create_function("deterministic", 0, mock, deterministic=True)
        try:
            self.con.execute("create index t on test(t) where deterministic() is not null")
        except sqlite.OperationalError:
            self.fail("Unexpected failure while creating partial index")

    def test_func_deterministic_keyword_only(self):
        with self.assertRaises(TypeError):
            self.con.create_function("deterministic", 0, int, True)

    def test_function_destructor_via_gc(self):
        # See bpo-44304: The destructor of the user function can
        # crash if is called without the GIL from the gc functions
        def md5sum(t):
            return

        with memory_database() as dest:
            dest.create_function("md5", 1, md5sum)
            x = dest("create table lang (name, first_appeared)")
            del md5sum, dest

            y = [x]
            y.append(y)

            del x,y
            gc_collect()

    @with_tracebacks(OverflowError)
    def test_func_return_too_large_int(self):
        cur = self.con.cursor()
        msg = "string or blob too big"
        for value in 2**63, -2**63-1, 2**64:
            self.con.create_function("largeint", 0, lambda value=value: value)
            with self.assertRaisesRegex(sqlite.DataError, msg):
                cur.execute("select largeint()")

    @with_tracebacks(UnicodeEncodeError, "surrogates not allowed")
    def test_func_return_text_with_surrogates(self):
        cur = self.con.cursor()
        self.con.create_function("pychr", 1, chr)
        for value in 0xd8ff, 0xdcff:
            with self.assertRaises(sqlite.OperationalError):
                cur.execute("select pychr(?)", (value,))

    @unittest.skipUnless(sys.maxsize > 2**32, 'requires 64bit platform')
    @bigmemtest(size=2**31, memuse=3, dry_run=False)
    def test_func_return_too_large_text(self, size):
        cur = self.con.cursor()
        for size in 2**31-1, 2**31:
            self.con.create_function("largetext", 0, lambda size=size: "b" * size)
            with self.assertRaises(sqlite.DataError):
                cur.execute("select largetext()")

    @unittest.skipUnless(sys.maxsize > 2**32, 'requires 64bit platform')
    @bigmemtest(size=2**31, memuse=2, dry_run=False)
    def test_func_return_too_large_blob(self, size):
        cur = self.con.cursor()
        for size in 2**31-1, 2**31:
            self.con.create_function("largeblob", 0, lambda size=size: b"b" * size)
            with self.assertRaises(sqlite.DataError):
                cur.execute("select largeblob()")

    def test_func_return_illegal_value(self):
        self.con.create_function("badreturn", 0, lambda: self)
        msg = "user-defined function raised exception"
        self.assertRaisesRegex(sqlite.OperationalError, msg,
                               self.con.execute, "select badreturn()")

    def test_func_keyword_args(self):
        regex = (
            r"Passing keyword arguments 'name', 'narg' and 'func' to "
            r"_sqlite3.Connection.create_function\(\) is deprecated. "
            r"Parameters 'name', 'narg' and 'func' will become "
            r"positional-only in Python 3.15."
        )

        def noop():
            return None

        with self.assertWarnsRegex(DeprecationWarning, regex) as cm:
            self.con.create_function("noop", 0, func=noop)
        self.assertEqual(cm.filename, __file__)

        with self.assertWarnsRegex(DeprecationWarning, regex) as cm:
            self.con.create_function("noop", narg=0, func=noop)
        self.assertEqual(cm.filename, __file__)

        with self.assertWarnsRegex(DeprecationWarning, regex) as cm:
            self.con.create_function(name="noop", narg=0, func=noop)
        self.assertEqual(cm.filename, __file__)


class WindowSumInt:
    def __init__(self):
        self.count = 0

    def step(self, value):
        self.count += value

    def value(self):
        return self.count

    def inverse(self, value):
        self.count -= value

    def finalize(self):
        return self.count

class BadWindow(Exception):
    pass


@unittest.skipIf(sqlite.sqlite_version_info < (3, 25, 0),
                 "Requires SQLite 3.25.0 or newer")
class WindowFunctionTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")
        self.cur = self.con.cursor()

        # Test case taken from https://www.sqlite.org/windowfunctions.html#udfwinfunc
        values = [
            ("a", 4),
            ("b", 5),
            ("c", 3),
            ("d", 8),
            ("e", 1),
        ]
        with self.con:
            self.con.execute("create table test(x, y)")
            self.con.executemany("insert into test values(?, ?)", values)
        self.expected = [
            ("a", 9),
            ("b", 12),
            ("c", 16),
            ("d", 12),
            ("e", 9),
        ]
        self.query = """
            select x, %s(y) over (
                order by x rows between 1 preceding and 1 following
            ) as sum_y
            from test order by x
        """
        self.con.create_window_function("sumint", 1, WindowSumInt)

    def tearDown(self):
        self.cur.close()
        self.con.close()

    def test_win_sum_int(self):
        self.cur.execute(self.query % "sumint")
        self.assertEqual(self.cur.fetchall(), self.expected)

    def test_win_error_on_create(self):
        with self.assertRaisesRegex(sqlite.ProgrammingError, "not -100"):
            self.con.create_window_function("shouldfail", -100, WindowSumInt)

    @with_tracebacks(BadWindow)
    def test_win_exception_in_method(self):
        for meth in "__init__", "step", "value", "inverse":
            with self.subTest(meth=meth):
                with patch.object(WindowSumInt, meth, side_effect=BadWindow):
                    name = f"exc_{meth}"
                    self.con.create_window_function(name, 1, WindowSumInt)
                    msg = f"'{meth}' method raised error"
                    with self.assertRaisesRegex(sqlite.OperationalError, msg):
                        self.cur.execute(self.query % name)
                        self.cur.fetchall()

    @with_tracebacks(BadWindow)
    def test_win_exception_in_finalize(self):
        # Note: SQLite does not (as of version 3.38.0) propagate finalize
        # callback errors to sqlite3_step(); this implies that OperationalError
        # is _not_ raised.
        with patch.object(WindowSumInt, "finalize", side_effect=BadWindow):
            name = "exception_in_finalize"
            self.con.create_window_function(name, 1, WindowSumInt)
            self.cur.execute(self.query % name)
            self.cur.fetchall()

    @with_tracebacks(AttributeError)
    def test_win_missing_method(self):
        class MissingValue:
            def step(self, x): pass
            def inverse(self, x): pass
            def finalize(self): return 42

        class MissingInverse:
            def step(self, x): pass
            def value(self): return 42
            def finalize(self): return 42

        class MissingStep:
            def value(self): return 42
            def inverse(self, x): pass
            def finalize(self): return 42

        dataset = (
            ("step", MissingStep),
            ("value", MissingValue),
            ("inverse", MissingInverse),
        )
        for meth, cls in dataset:
            with self.subTest(meth=meth, cls=cls):
                name = f"exc_{meth}"
                self.con.create_window_function(name, 1, cls)
                with self.assertRaisesRegex(sqlite.OperationalError,
                                            f"'{meth}' method not defined"):
                    self.cur.execute(self.query % name)
                    self.cur.fetchall()

    @with_tracebacks(AttributeError)
    def test_win_missing_finalize(self):
        # Note: SQLite does not (as of version 3.38.0) propagate finalize
        # callback errors to sqlite3_step(); this implies that OperationalError
        # is _not_ raised.
        class MissingFinalize:
            def step(self, x): pass
            def value(self): return 42
            def inverse(self, x): pass

        name = "missing_finalize"
        self.con.create_window_function(name, 1, MissingFinalize)
        self.cur.execute(self.query % name)
        self.cur.fetchall()

    def test_win_clear_function(self):
        self.con.create_window_function("sumint", 1, None)
        self.assertRaises(sqlite.OperationalError, self.cur.execute,
                          self.query % "sumint")

    def test_win_redefine_function(self):
        # Redefine WindowSumInt; adjust the expected results accordingly.
        class Redefined(WindowSumInt):
            def step(self, value): self.count += value * 2
            def inverse(self, value): self.count -= value * 2
        expected = [(v[0], v[1]*2) for v in self.expected]

        self.con.create_window_function("sumint", 1, Redefined)
        self.cur.execute(self.query % "sumint")
        self.assertEqual(self.cur.fetchall(), expected)

    def test_win_error_value_return(self):
        class ErrorValueReturn:
            def __init__(self): pass
            def step(self, x): pass
            def value(self): return 1 << 65

        self.con.create_window_function("err_val_ret", 1, ErrorValueReturn)
        self.assertRaisesRegex(sqlite.DataError, "string or blob too big",
                               self.cur.execute, self.query % "err_val_ret")


class AggregateTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")
        cur = self.con.cursor()
        cur.execute("""
            create table test(
                t text,
                i integer,
                f float,
                n,
                b blob
                )
            """)
        cur.execute("insert into test(t, i, f, n, b) values (?, ?, ?, ?, ?)",
            ("foo", 5, 3.14, None, memoryview(b"blob"),))
        cur.close()

        self.con.create_aggregate("nostep", 1, AggrNoStep)
        self.con.create_aggregate("nofinalize", 1, AggrNoFinalize)
        self.con.create_aggregate("excInit", 1, AggrExceptionInInit)
        self.con.create_aggregate("excStep", 1, AggrExceptionInStep)
        self.con.create_aggregate("excFinalize", 1, AggrExceptionInFinalize)
        self.con.create_aggregate("checkType", 2, AggrCheckType)
        self.con.create_aggregate("checkTypes", -1, AggrCheckTypes)
        self.con.create_aggregate("mysum", 1, AggrSum)
        self.con.create_aggregate("aggtxt", 1, AggrText)

    def tearDown(self):
        self.con.close()

    def test_aggr_error_on_create(self):
        with self.assertRaisesRegex(sqlite.ProgrammingError, "not -100"):
            self.con.create_function("bla", -100, AggrSum)

    @with_tracebacks(AttributeError, msg_regex="AggrNoStep")
    def test_aggr_no_step(self):
        cur = self.con.cursor()
        with self.assertRaises(sqlite.OperationalError) as cm:
            cur.execute("select nostep(t) from test")
        self.assertEqual(str(cm.exception),
                         "user-defined aggregate's 'step' method not defined")

    def test_aggr_no_finalize(self):
        cur = self.con.cursor()
        msg = "user-defined aggregate's 'finalize' method not defined"
        with self.assertRaisesRegex(sqlite.OperationalError, msg):
            cur.execute("select nofinalize(t) from test")
            val = cur.fetchone()[0]

    @with_tracebacks(ZeroDivisionError, msg_regex="AggrExceptionInInit")
    def test_aggr_exception_in_init(self):
        cur = self.con.cursor()
        with self.assertRaises(sqlite.OperationalError) as cm:
            cur.execute("select excInit(t) from test")
            val = cur.fetchone()[0]
        self.assertEqual(str(cm.exception), "user-defined aggregate's '__init__' method raised error")

    @with_tracebacks(ZeroDivisionError, msg_regex="AggrExceptionInStep")
    def test_aggr_exception_in_step(self):
        cur = self.con.cursor()
        with self.assertRaises(sqlite.OperationalError) as cm:
            cur.execute("select excStep(t) from test")
            val = cur.fetchone()[0]
        self.assertEqual(str(cm.exception), "user-defined aggregate's 'step' method raised error")

    @with_tracebacks(ZeroDivisionError, msg_regex="AggrExceptionInFinalize")
    def test_aggr_exception_in_finalize(self):
        cur = self.con.cursor()
        with self.assertRaises(sqlite.OperationalError) as cm:
            cur.execute("select excFinalize(t) from test")
            val = cur.fetchone()[0]
        self.assertEqual(str(cm.exception), "user-defined aggregate's 'finalize' method raised error")

    def test_aggr_check_param_str(self):
        cur = self.con.cursor()
        cur.execute("select checkTypes('str', ?, ?)", ("foo", str()))
        val = cur.fetchone()[0]
        self.assertEqual(val, 2)

    def test_aggr_check_param_int(self):
        cur = self.con.cursor()
        cur.execute("select checkType('int', ?)", (42,))
        val = cur.fetchone()[0]
        self.assertEqual(val, 1)

    def test_aggr_check_params_int(self):
        cur = self.con.cursor()
        cur.execute("select checkTypes('int', ?, ?)", (42, 24))
        val = cur.fetchone()[0]
        self.assertEqual(val, 2)

    def test_aggr_check_param_float(self):
        cur = self.con.cursor()
        cur.execute("select checkType('float', ?)", (3.14,))
        val = cur.fetchone()[0]
        self.assertEqual(val, 1)

    def test_aggr_check_param_none(self):
        cur = self.con.cursor()
        cur.execute("select checkType('None', ?)", (None,))
        val = cur.fetchone()[0]
        self.assertEqual(val, 1)

    def test_aggr_check_param_blob(self):
        cur = self.con.cursor()
        cur.execute("select checkType('blob', ?)", (memoryview(b"blob"),))
        val = cur.fetchone()[0]
        self.assertEqual(val, 1)

    def test_aggr_check_aggr_sum(self):
        cur = self.con.cursor()
        cur.execute("delete from test")
        cur.executemany("insert into test(i) values (?)", [(10,), (20,), (30,)])
        cur.execute("select mysum(i) from test")
        val = cur.fetchone()[0]
        self.assertEqual(val, 60)

    def test_aggr_no_match(self):
        cur = self.con.execute("select mysum(i) from (select 1 as i) where i == 0")
        val = cur.fetchone()[0]
        self.assertIsNone(val)

    def test_aggr_text(self):
        cur = self.con.cursor()
        for txt in ["foo", "1\x002"]:
            with self.subTest(txt=txt):
                cur.execute("select aggtxt(?) from test", (txt,))
                val = cur.fetchone()[0]
                self.assertEqual(val, txt)

    def test_agg_keyword_args(self):
        regex = (
            r"Passing keyword arguments 'name', 'n_arg' and 'aggregate_class' to "
            r"_sqlite3.Connection.create_aggregate\(\) is deprecated. "
            r"Parameters 'name', 'n_arg' and 'aggregate_class' will become "
            r"positional-only in Python 3.15."
        )

        with self.assertWarnsRegex(DeprecationWarning, regex) as cm:
            self.con.create_aggregate("test", 1, aggregate_class=AggrText)
        self.assertEqual(cm.filename, __file__)

        with self.assertWarnsRegex(DeprecationWarning, regex) as cm:
            self.con.create_aggregate("test", n_arg=1, aggregate_class=AggrText)
        self.assertEqual(cm.filename, __file__)

        with self.assertWarnsRegex(DeprecationWarning, regex) as cm:
            self.con.create_aggregate(name="test", n_arg=0,
                                      aggregate_class=AggrText)
        self.assertEqual(cm.filename, __file__)


class AuthorizerTests(unittest.TestCase):
    @staticmethod
    def authorizer_cb(action, arg1, arg2, dbname, source):
        if action != sqlite.SQLITE_SELECT:
            return sqlite.SQLITE_DENY
        if arg2 == 'c2' or arg1 == 't2':
            return sqlite.SQLITE_DENY
        return sqlite.SQLITE_OK

    def setUp(self):
        self.con = sqlite.connect(":memory:")
        self.con.executescript("""
            create table t1 (c1, c2);
            create table t2 (c1, c2);
            insert into t1 (c1, c2) values (1, 2);
            insert into t2 (c1, c2) values (4, 5);
            """)

        # For our security test:
        self.con.execute("select c2 from t2")

        self.con.set_authorizer(self.authorizer_cb)

    def tearDown(self):
        self.con.close()

    def test_table_access(self):
        with self.assertRaises(sqlite.DatabaseError) as cm:
            self.con.execute("select * from t2")
        self.assertIn('prohibited', str(cm.exception))

    def test_column_access(self):
        with self.assertRaises(sqlite.DatabaseError) as cm:
            self.con.execute("select c2 from t1")
        self.assertIn('prohibited', str(cm.exception))

    def test_clear_authorizer(self):
        self.con.set_authorizer(None)
        self.con.execute("select * from t2")
        self.con.execute("select c2 from t1")

    def test_authorizer_keyword_args(self):
        regex = (
            r"Passing keyword argument 'authorizer_callback' to "
            r"_sqlite3.Connection.set_authorizer\(\) is deprecated. "
            r"Parameter 'authorizer_callback' will become positional-only in "
            r"Python 3.15."
        )

        with self.assertWarnsRegex(DeprecationWarning, regex) as cm:
            self.con.set_authorizer(authorizer_callback=lambda: None)
        self.assertEqual(cm.filename, __file__)


class AuthorizerRaiseExceptionTests(AuthorizerTests):
    @staticmethod
    def authorizer_cb(action, arg1, arg2, dbname, source):
        if action != sqlite.SQLITE_SELECT:
            raise ValueError
        if arg2 == 'c2' or arg1 == 't2':
            raise ValueError
        return sqlite.SQLITE_OK

    @with_tracebacks(ValueError, msg_regex="authorizer_cb")
    def test_table_access(self):
        super().test_table_access()

    @with_tracebacks(ValueError, msg_regex="authorizer_cb")
    def test_column_access(self):
        super().test_table_access()

class AuthorizerIllegalTypeTests(AuthorizerTests):
    @staticmethod
    def authorizer_cb(action, arg1, arg2, dbname, source):
        if action != sqlite.SQLITE_SELECT:
            return 0.0
        if arg2 == 'c2' or arg1 == 't2':
            return 0.0
        return sqlite.SQLITE_OK

class AuthorizerLargeIntegerTests(AuthorizerTests):
    @staticmethod
    def authorizer_cb(action, arg1, arg2, dbname, source):
        if action != sqlite.SQLITE_SELECT:
            return 2**32
        if arg2 == 'c2' or arg1 == 't2':
            return 2**32
        return sqlite.SQLITE_OK


if __name__ == "__main__":
    unittest.main()
