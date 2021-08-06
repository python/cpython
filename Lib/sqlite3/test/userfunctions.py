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

import unittest
import unittest.mock
import sqlite3 as sqlite

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

def func_isstring(v):
    return type(v) is str
def func_isint(v):
    return type(v) is int
def func_isfloat(v):
    return type(v) is float
def func_isnone(v):
    return type(v) is type(None)
def func_isblob(v):
    return isinstance(v, (bytes, memoryview))
def func_islonglong(v):
    return isinstance(v, int) and v >= 1<<31

def func(*args):
    return len(args)

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
        self.con.create_function("raiseexception", 0, func_raiseexception)

        self.con.create_function("isstring", 1, func_isstring)
        self.con.create_function("isint", 1, func_isint)
        self.con.create_function("isfloat", 1, func_isfloat)
        self.con.create_function("isnone", 1, func_isnone)
        self.con.create_function("isblob", 1, func_isblob)
        self.con.create_function("islonglong", 1, func_islonglong)
        self.con.create_function("spam", -1, func)
        self.con.execute("create table test(t text)")

    def tearDown(self):
        self.con.close()

    def CheckFuncErrorOnCreate(self):
        with self.assertRaises(sqlite.OperationalError):
            self.con.create_function("bla", -100, lambda x: 2*x)

    def CheckFuncRefCount(self):
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

    def CheckFuncReturnText(self):
        cur = self.con.cursor()
        cur.execute("select returntext()")
        val = cur.fetchone()[0]
        self.assertEqual(type(val), str)
        self.assertEqual(val, "foo")

    def CheckFuncReturnTextWithNullChar(self):
        cur = self.con.cursor()
        res = cur.execute("select returntextwithnull()").fetchone()[0]
        self.assertEqual(type(res), str)
        self.assertEqual(res, "1\x002")

    def CheckFuncReturnUnicode(self):
        cur = self.con.cursor()
        cur.execute("select returnunicode()")
        val = cur.fetchone()[0]
        self.assertEqual(type(val), str)
        self.assertEqual(val, "bar")

    def CheckFuncReturnInt(self):
        cur = self.con.cursor()
        cur.execute("select returnint()")
        val = cur.fetchone()[0]
        self.assertEqual(type(val), int)
        self.assertEqual(val, 42)

    def CheckFuncReturnFloat(self):
        cur = self.con.cursor()
        cur.execute("select returnfloat()")
        val = cur.fetchone()[0]
        self.assertEqual(type(val), float)
        if val < 3.139 or val > 3.141:
            self.fail("wrong value")

    def CheckFuncReturnNull(self):
        cur = self.con.cursor()
        cur.execute("select returnnull()")
        val = cur.fetchone()[0]
        self.assertEqual(type(val), type(None))
        self.assertEqual(val, None)

    def CheckFuncReturnBlob(self):
        cur = self.con.cursor()
        cur.execute("select returnblob()")
        val = cur.fetchone()[0]
        self.assertEqual(type(val), bytes)
        self.assertEqual(val, b"blob")

    def CheckFuncReturnLongLong(self):
        cur = self.con.cursor()
        cur.execute("select returnlonglong()")
        val = cur.fetchone()[0]
        self.assertEqual(val, 1<<31)

    def CheckFuncException(self):
        cur = self.con.cursor()
        with self.assertRaises(sqlite.OperationalError) as cm:
            cur.execute("select raiseexception()")
            cur.fetchone()
        self.assertEqual(str(cm.exception), 'user-defined function raised exception')

    def CheckParamString(self):
        cur = self.con.cursor()
        for text in ["foo", str()]:
            with self.subTest(text=text):
                cur.execute("select isstring(?)", (text,))
                val = cur.fetchone()[0]
                self.assertEqual(val, 1)

    def CheckParamInt(self):
        cur = self.con.cursor()
        cur.execute("select isint(?)", (42,))
        val = cur.fetchone()[0]
        self.assertEqual(val, 1)

    def CheckParamFloat(self):
        cur = self.con.cursor()
        cur.execute("select isfloat(?)", (3.14,))
        val = cur.fetchone()[0]
        self.assertEqual(val, 1)

    def CheckParamNone(self):
        cur = self.con.cursor()
        cur.execute("select isnone(?)", (None,))
        val = cur.fetchone()[0]
        self.assertEqual(val, 1)

    def CheckParamBlob(self):
        cur = self.con.cursor()
        cur.execute("select isblob(?)", (memoryview(b"blob"),))
        val = cur.fetchone()[0]
        self.assertEqual(val, 1)

    def CheckParamLongLong(self):
        cur = self.con.cursor()
        cur.execute("select islonglong(?)", (1<<42,))
        val = cur.fetchone()[0]
        self.assertEqual(val, 1)

    def CheckAnyArguments(self):
        cur = self.con.cursor()
        cur.execute("select spam(?, ?)", (1, 2))
        val = cur.fetchone()[0]
        self.assertEqual(val, 2)

    # Regarding deterministic functions:
    #
    # Between 3.8.3 and 3.15.0, deterministic functions were only used to
    # optimize inner loops, so for those versions we can only test if the
    # sqlite machinery has factored out a call or not. From 3.15.0 and onward,
    # deterministic functions were permitted in WHERE clauses of partial
    # indices, which allows testing based on syntax, iso. the query optimizer.
    @unittest.skipIf(sqlite.sqlite_version_info < (3, 8, 3), "Requires SQLite 3.8.3 or higher")
    def CheckFuncNonDeterministic(self):
        mock = unittest.mock.Mock(return_value=None)
        self.con.create_function("nondeterministic", 0, mock, deterministic=False)
        if sqlite.sqlite_version_info < (3, 15, 0):
            self.con.execute("select nondeterministic() = nondeterministic()")
            self.assertEqual(mock.call_count, 2)
        else:
            with self.assertRaises(sqlite.OperationalError):
                self.con.execute("create index t on test(t) where nondeterministic() is not null")

    @unittest.skipIf(sqlite.sqlite_version_info < (3, 8, 3), "Requires SQLite 3.8.3 or higher")
    def CheckFuncDeterministic(self):
        mock = unittest.mock.Mock(return_value=None)
        self.con.create_function("deterministic", 0, mock, deterministic=True)
        if sqlite.sqlite_version_info < (3, 15, 0):
            self.con.execute("select deterministic() = deterministic()")
            self.assertEqual(mock.call_count, 1)
        else:
            try:
                self.con.execute("create index t on test(t) where deterministic() is not null")
            except sqlite.OperationalError:
                self.fail("Unexpected failure while creating partial index")

    @unittest.skipIf(sqlite.sqlite_version_info >= (3, 8, 3), "SQLite < 3.8.3 needed")
    def CheckFuncDeterministicNotSupported(self):
        with self.assertRaises(sqlite.NotSupportedError):
            self.con.create_function("deterministic", 0, int, deterministic=True)

    def CheckFuncDeterministicKeywordOnly(self):
        with self.assertRaises(TypeError):
            self.con.create_function("deterministic", 0, int, True)


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
        #self.cur.close()
        #self.con.close()
        pass

    def CheckAggrErrorOnCreate(self):
        with self.assertRaises(sqlite.OperationalError):
            self.con.create_function("bla", -100, AggrSum)

    def CheckAggrNoStep(self):
        cur = self.con.cursor()
        with self.assertRaises(AttributeError) as cm:
            cur.execute("select nostep(t) from test")
        self.assertEqual(str(cm.exception), "'AggrNoStep' object has no attribute 'step'")

    def CheckAggrNoFinalize(self):
        cur = self.con.cursor()
        with self.assertRaises(sqlite.OperationalError) as cm:
            cur.execute("select nofinalize(t) from test")
            val = cur.fetchone()[0]
        self.assertEqual(str(cm.exception), "user-defined aggregate's 'finalize' method raised error")

    def CheckAggrExceptionInInit(self):
        cur = self.con.cursor()
        with self.assertRaises(sqlite.OperationalError) as cm:
            cur.execute("select excInit(t) from test")
            val = cur.fetchone()[0]
        self.assertEqual(str(cm.exception), "user-defined aggregate's '__init__' method raised error")

    def CheckAggrExceptionInStep(self):
        cur = self.con.cursor()
        with self.assertRaises(sqlite.OperationalError) as cm:
            cur.execute("select excStep(t) from test")
            val = cur.fetchone()[0]
        self.assertEqual(str(cm.exception), "user-defined aggregate's 'step' method raised error")

    def CheckAggrExceptionInFinalize(self):
        cur = self.con.cursor()
        with self.assertRaises(sqlite.OperationalError) as cm:
            cur.execute("select excFinalize(t) from test")
            val = cur.fetchone()[0]
        self.assertEqual(str(cm.exception), "user-defined aggregate's 'finalize' method raised error")

    def CheckAggrCheckParamStr(self):
        cur = self.con.cursor()
        cur.execute("select checkTypes('str', ?, ?)", ("foo", str()))
        val = cur.fetchone()[0]
        self.assertEqual(val, 2)

    def CheckAggrCheckParamInt(self):
        cur = self.con.cursor()
        cur.execute("select checkType('int', ?)", (42,))
        val = cur.fetchone()[0]
        self.assertEqual(val, 1)

    def CheckAggrCheckParamsInt(self):
        cur = self.con.cursor()
        cur.execute("select checkTypes('int', ?, ?)", (42, 24))
        val = cur.fetchone()[0]
        self.assertEqual(val, 2)

    def CheckAggrCheckParamFloat(self):
        cur = self.con.cursor()
        cur.execute("select checkType('float', ?)", (3.14,))
        val = cur.fetchone()[0]
        self.assertEqual(val, 1)

    def CheckAggrCheckParamNone(self):
        cur = self.con.cursor()
        cur.execute("select checkType('None', ?)", (None,))
        val = cur.fetchone()[0]
        self.assertEqual(val, 1)

    def CheckAggrCheckParamBlob(self):
        cur = self.con.cursor()
        cur.execute("select checkType('blob', ?)", (memoryview(b"blob"),))
        val = cur.fetchone()[0]
        self.assertEqual(val, 1)

    def CheckAggrCheckAggrSum(self):
        cur = self.con.cursor()
        cur.execute("delete from test")
        cur.executemany("insert into test(i) values (?)", [(10,), (20,), (30,)])
        cur.execute("select mysum(i) from test")
        val = cur.fetchone()[0]
        self.assertEqual(val, 60)

    def CheckAggrText(self):
        cur = self.con.cursor()
        for txt in ["foo", "1\x002"]:
            with self.subTest(txt=txt):
                cur.execute("select aggtxt(?) from test", (txt,))
                val = cur.fetchone()[0]
                self.assertEqual(val, txt)


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
        pass

    def test_table_access(self):
        with self.assertRaises(sqlite.DatabaseError) as cm:
            self.con.execute("select * from t2")
        self.assertIn('prohibited', str(cm.exception))

    def test_column_access(self):
        with self.assertRaises(sqlite.DatabaseError) as cm:
            self.con.execute("select c2 from t1")
        self.assertIn('prohibited', str(cm.exception))

class AuthorizerRaiseExceptionTests(AuthorizerTests):
    @staticmethod
    def authorizer_cb(action, arg1, arg2, dbname, source):
        if action != sqlite.SQLITE_SELECT:
            raise ValueError
        if arg2 == 'c2' or arg1 == 't2':
            raise ValueError
        return sqlite.SQLITE_OK

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


def suite():
    function_suite = unittest.makeSuite(FunctionTests, "Check")
    aggregate_suite = unittest.makeSuite(AggregateTests, "Check")
    authorizer_suite = unittest.makeSuite(AuthorizerTests)
    return unittest.TestSuite((
            function_suite,
            aggregate_suite,
            authorizer_suite,
            unittest.makeSuite(AuthorizerRaiseExceptionTests),
            unittest.makeSuite(AuthorizerIllegalTypeTests),
            unittest.makeSuite(AuthorizerLargeIntegerTests),
        ))

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
