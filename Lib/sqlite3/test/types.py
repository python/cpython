# pysqlite2/test/types.py: tests for type conversion and detection
#
# Copyright (C) 2005 Gerhard Häring <gh@ghaering.de>
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

import datetime
import unittest
import sqlite3 as sqlite
try:
    import zlib
except ImportError:
    zlib = None


class SqliteTypeTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:")
        self.cur = self.con.cursor()
        self.cur.execute("create table test(i integer, s varchar, f number, b blob)")

    def tearDown(self):
        self.cur.close()
        self.con.close()

    def test_string(self):
        self.cur.execute("insert into test(s) values (?)", ("Österreich",))
        self.cur.execute("select s from test")
        row = self.cur.fetchone()
        self.assertEqual(row[0], "Österreich")

    def test_small_int(self):
        self.cur.execute("insert into test(i) values (?)", (42,))
        self.cur.execute("select i from test")
        row = self.cur.fetchone()
        self.assertEqual(row[0], 42)

    def test_large_int(self):
        num = 2**40
        self.cur.execute("insert into test(i) values (?)", (num,))
        self.cur.execute("select i from test")
        row = self.cur.fetchone()
        self.assertEqual(row[0], num)

    def test_float(self):
        val = 3.14
        self.cur.execute("insert into test(f) values (?)", (val,))
        self.cur.execute("select f from test")
        row = self.cur.fetchone()
        self.assertEqual(row[0], val)

    def test_blob(self):
        sample = b"Guglhupf"
        val = memoryview(sample)
        self.cur.execute("insert into test(b) values (?)", (val,))
        self.cur.execute("select b from test")
        row = self.cur.fetchone()
        self.assertEqual(row[0], sample)

    def test_unicode_execute(self):
        self.cur.execute("select 'Österreich'")
        row = self.cur.fetchone()
        self.assertEqual(row[0], "Österreich")

class DeclTypesTests(unittest.TestCase):
    class Foo:
        def __init__(self, _val):
            if isinstance(_val, bytes):
                # sqlite3 always calls __init__ with a bytes created from a
                # UTF-8 string when __conform__ was used to store the object.
                _val = _val.decode('utf-8')
            self.val = _val

        def __eq__(self, other):
            if not isinstance(other, DeclTypesTests.Foo):
                return NotImplemented
            return self.val == other.val

        def __conform__(self, protocol):
            if protocol is sqlite.PrepareProtocol:
                return self.val
            else:
                return None

        def __str__(self):
            return "<%s>" % self.val

    class BadConform:
        def __init__(self, exc):
            self.exc = exc
        def __conform__(self, protocol):
            raise self.exc

    def setUp(self):
        self.con = sqlite.connect(":memory:", detect_types=sqlite.PARSE_DECLTYPES)
        self.cur = self.con.cursor()
        self.cur.execute("""
            create table test(
                i int,
                s str,
                f float,
                b bool,
                u unicode,
                foo foo,
                bin blob,
                n1 number,
                n2 number(5),
                bad bad,
                cbin cblob)
        """)

        # override float, make them always return the same number
        sqlite.converters["FLOAT"] = lambda x: 47.2

        # and implement two custom ones
        sqlite.converters["BOOL"] = lambda x: bool(int(x))
        sqlite.converters["FOO"] = DeclTypesTests.Foo
        sqlite.converters["BAD"] = DeclTypesTests.BadConform
        sqlite.converters["WRONG"] = lambda x: "WRONG"
        sqlite.converters["NUMBER"] = float
        sqlite.converters["CBLOB"] = lambda x: b"blobish"

    def tearDown(self):
        del sqlite.converters["FLOAT"]
        del sqlite.converters["BOOL"]
        del sqlite.converters["FOO"]
        del sqlite.converters["BAD"]
        del sqlite.converters["WRONG"]
        del sqlite.converters["NUMBER"]
        del sqlite.converters["CBLOB"]
        self.cur.close()
        self.con.close()

    def test_string(self):
        # default
        self.cur.execute("insert into test(s) values (?)", ("foo",))
        self.cur.execute('select s as "s [WRONG]" from test')
        row = self.cur.fetchone()
        self.assertEqual(row[0], "foo")

    def test_small_int(self):
        # default
        self.cur.execute("insert into test(i) values (?)", (42,))
        self.cur.execute("select i from test")
        row = self.cur.fetchone()
        self.assertEqual(row[0], 42)

    def test_large_int(self):
        # default
        num = 2**40
        self.cur.execute("insert into test(i) values (?)", (num,))
        self.cur.execute("select i from test")
        row = self.cur.fetchone()
        self.assertEqual(row[0], num)

    def test_float(self):
        # custom
        val = 3.14
        self.cur.execute("insert into test(f) values (?)", (val,))
        self.cur.execute("select f from test")
        row = self.cur.fetchone()
        self.assertEqual(row[0], 47.2)

    def test_bool(self):
        # custom
        self.cur.execute("insert into test(b) values (?)", (False,))
        self.cur.execute("select b from test")
        row = self.cur.fetchone()
        self.assertIs(row[0], False)

        self.cur.execute("delete from test")
        self.cur.execute("insert into test(b) values (?)", (True,))
        self.cur.execute("select b from test")
        row = self.cur.fetchone()
        self.assertIs(row[0], True)

    def test_unicode(self):
        # default
        val = "\xd6sterreich"
        self.cur.execute("insert into test(u) values (?)", (val,))
        self.cur.execute("select u from test")
        row = self.cur.fetchone()
        self.assertEqual(row[0], val)

    def test_foo(self):
        val = DeclTypesTests.Foo("bla")
        self.cur.execute("insert into test(foo) values (?)", (val,))
        self.cur.execute("select foo from test")
        row = self.cur.fetchone()
        self.assertEqual(row[0], val)

    def test_error_in_conform(self):
        val = DeclTypesTests.BadConform(TypeError)
        with self.assertRaises(sqlite.InterfaceError):
            self.cur.execute("insert into test(bad) values (?)", (val,))
        with self.assertRaises(sqlite.InterfaceError):
            self.cur.execute("insert into test(bad) values (:val)", {"val": val})

        val = DeclTypesTests.BadConform(KeyboardInterrupt)
        with self.assertRaises(KeyboardInterrupt):
            self.cur.execute("insert into test(bad) values (?)", (val,))
        with self.assertRaises(KeyboardInterrupt):
            self.cur.execute("insert into test(bad) values (:val)", {"val": val})

    def test_unsupported_seq(self):
        class Bar: pass
        val = Bar()
        with self.assertRaises(sqlite.InterfaceError):
            self.cur.execute("insert into test(f) values (?)", (val,))

    def test_unsupported_dict(self):
        class Bar: pass
        val = Bar()
        with self.assertRaises(sqlite.InterfaceError):
            self.cur.execute("insert into test(f) values (:val)", {"val": val})

    def test_blob(self):
        # default
        sample = b"Guglhupf"
        val = memoryview(sample)
        self.cur.execute("insert into test(bin) values (?)", (val,))
        self.cur.execute("select bin from test")
        row = self.cur.fetchone()
        self.assertEqual(row[0], sample)

    def test_number1(self):
        self.cur.execute("insert into test(n1) values (5)")
        value = self.cur.execute("select n1 from test").fetchone()[0]
        # if the converter is not used, it's an int instead of a float
        self.assertEqual(type(value), float)

    def test_number2(self):
        """Checks whether converter names are cut off at '(' characters"""
        self.cur.execute("insert into test(n2) values (5)")
        value = self.cur.execute("select n2 from test").fetchone()[0]
        # if the converter is not used, it's an int instead of a float
        self.assertEqual(type(value), float)

    def test_convert_zero_sized_blob(self):
        self.con.execute("insert into test(cbin) values (?)", (b"",))
        cur = self.con.execute("select cbin from test")
        # Zero-sized blobs with converters returns None.  This differs from
        # blobs without a converter, where b"" is returned.
        self.assertIsNone(cur.fetchone()[0])


class ColNamesTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:", detect_types=sqlite.PARSE_COLNAMES)
        self.cur = self.con.cursor()
        self.cur.execute("create table test(x foo)")

        sqlite.converters["FOO"] = lambda x: "[%s]" % x.decode("ascii")
        sqlite.converters["BAR"] = lambda x: "<%s>" % x.decode("ascii")
        sqlite.converters["EXC"] = lambda x: 5/0
        sqlite.converters["B1B1"] = lambda x: "MARKER"

    def tearDown(self):
        del sqlite.converters["FOO"]
        del sqlite.converters["BAR"]
        del sqlite.converters["EXC"]
        del sqlite.converters["B1B1"]
        self.cur.close()
        self.con.close()

    def test_decl_type_not_used(self):
        """
        Assures that the declared type is not used when PARSE_DECLTYPES
        is not set.
        """
        self.cur.execute("insert into test(x) values (?)", ("xxx",))
        self.cur.execute("select x from test")
        val = self.cur.fetchone()[0]
        self.assertEqual(val, "xxx")

    def test_none(self):
        self.cur.execute("insert into test(x) values (?)", (None,))
        self.cur.execute("select x from test")
        val = self.cur.fetchone()[0]
        self.assertEqual(val, None)

    def test_col_name(self):
        self.cur.execute("insert into test(x) values (?)", ("xxx",))
        self.cur.execute('select x as "x y [bar]" from test')
        val = self.cur.fetchone()[0]
        self.assertEqual(val, "<xxx>")

        # Check if the stripping of colnames works. Everything after the first
        # '[' (and the preceeding space) should be stripped.
        self.assertEqual(self.cur.description[0][0], "x y")

    def test_case_in_converter_name(self):
        self.cur.execute("select 'other' as \"x [b1b1]\"")
        val = self.cur.fetchone()[0]
        self.assertEqual(val, "MARKER")

    def test_cursor_description_no_row(self):
        """
        cursor.description should at least provide the column name(s), even if
        no row returned.
        """
        self.cur.execute("select * from test where 0 = 1")
        self.assertEqual(self.cur.description[0][0], "x")

    def test_cursor_description_insert(self):
        self.cur.execute("insert into test values (1)")
        self.assertIsNone(self.cur.description)


@unittest.skipIf(sqlite.sqlite_version_info < (3, 8, 3), "CTEs not supported")
class CommonTableExpressionTests(unittest.TestCase):

    def setUp(self):
        self.con = sqlite.connect(":memory:")
        self.cur = self.con.cursor()
        self.cur.execute("create table test(x foo)")

    def tearDown(self):
        self.cur.close()
        self.con.close()

    def test_cursor_description_cte_simple(self):
        self.cur.execute("with one as (select 1) select * from one")
        self.assertIsNotNone(self.cur.description)
        self.assertEqual(self.cur.description[0][0], "1")

    def test_cursor_description_cte_multiple_columns(self):
        self.cur.execute("insert into test values(1)")
        self.cur.execute("insert into test values(2)")
        self.cur.execute("with testCTE as (select * from test) select * from testCTE")
        self.assertIsNotNone(self.cur.description)
        self.assertEqual(self.cur.description[0][0], "x")

    def test_cursor_description_cte(self):
        self.cur.execute("insert into test values (1)")
        self.cur.execute("with bar as (select * from test) select * from test where x = 1")
        self.assertIsNotNone(self.cur.description)
        self.assertEqual(self.cur.description[0][0], "x")
        self.cur.execute("with bar as (select * from test) select * from test where x = 2")
        self.assertIsNotNone(self.cur.description)
        self.assertEqual(self.cur.description[0][0], "x")


class ObjectAdaptationTests(unittest.TestCase):
    def cast(obj):
        return float(obj)
    cast = staticmethod(cast)

    def setUp(self):
        self.con = sqlite.connect(":memory:")
        try:
            del sqlite.adapters[int]
        except:
            pass
        sqlite.register_adapter(int, ObjectAdaptationTests.cast)
        self.cur = self.con.cursor()

    def tearDown(self):
        del sqlite.adapters[(int, sqlite.PrepareProtocol)]
        self.cur.close()
        self.con.close()

    def test_caster_is_used(self):
        self.cur.execute("select ?", (4,))
        val = self.cur.fetchone()[0]
        self.assertEqual(type(val), float)

    def test_missing_adapter(self):
        with self.assertRaises(sqlite.ProgrammingError):
            sqlite.adapt(1.)  # No float adapter registered

    def test_missing_protocol(self):
        with self.assertRaises(sqlite.ProgrammingError):
            sqlite.adapt(1, None)

    def test_defect_proto(self):
        class DefectProto():
            def __adapt__(self):
                return None
        with self.assertRaises(sqlite.ProgrammingError):
            sqlite.adapt(1., DefectProto)

    def test_defect_self_adapt(self):
        class DefectSelfAdapt(float):
            def __conform__(self, _):
                return None
        with self.assertRaises(sqlite.ProgrammingError):
            sqlite.adapt(DefectSelfAdapt(1.))

    def test_custom_proto(self):
        class CustomProto():
            def __adapt__(self):
                return "adapted"
        self.assertEqual(sqlite.adapt(1., CustomProto), "adapted")

    def test_adapt(self):
        val = 42
        self.assertEqual(float(val), sqlite.adapt(val))

    def test_adapt_alt(self):
        alt = "other"
        self.assertEqual(alt, sqlite.adapt(1., None, alt))


@unittest.skipUnless(zlib, "requires zlib")
class BinaryConverterTests(unittest.TestCase):
    def convert(s):
        return zlib.decompress(s)
    convert = staticmethod(convert)

    def setUp(self):
        self.con = sqlite.connect(":memory:", detect_types=sqlite.PARSE_COLNAMES)
        sqlite.register_converter("bin", BinaryConverterTests.convert)

    def tearDown(self):
        self.con.close()

    def test_binary_input_for_converter(self):
        testdata = b"abcdefg" * 10
        result = self.con.execute('select ? as "x [bin]"', (memoryview(zlib.compress(testdata)),)).fetchone()[0]
        self.assertEqual(testdata, result)

class DateTimeTests(unittest.TestCase):
    def setUp(self):
        self.con = sqlite.connect(":memory:", detect_types=sqlite.PARSE_DECLTYPES)
        self.cur = self.con.cursor()
        self.cur.execute("create table test(d date, ts timestamp)")

    def tearDown(self):
        self.cur.close()
        self.con.close()

    def test_sqlite_date(self):
        d = sqlite.Date(2004, 2, 14)
        self.cur.execute("insert into test(d) values (?)", (d,))
        self.cur.execute("select d from test")
        d2 = self.cur.fetchone()[0]
        self.assertEqual(d, d2)

    def test_sqlite_timestamp(self):
        ts = sqlite.Timestamp(2004, 2, 14, 7, 15, 0)
        self.cur.execute("insert into test(ts) values (?)", (ts,))
        self.cur.execute("select ts from test")
        ts2 = self.cur.fetchone()[0]
        self.assertEqual(ts, ts2)

    def test_sql_timestamp(self):
        now = datetime.datetime.utcnow()
        self.cur.execute("insert into test(ts) values (current_timestamp)")
        self.cur.execute("select ts from test")
        ts = self.cur.fetchone()[0]
        self.assertEqual(type(ts), datetime.datetime)
        self.assertEqual(ts.year, now.year)

    def test_date_time_sub_seconds(self):
        ts = sqlite.Timestamp(2004, 2, 14, 7, 15, 0, 500000)
        self.cur.execute("insert into test(ts) values (?)", (ts,))
        self.cur.execute("select ts from test")
        ts2 = self.cur.fetchone()[0]
        self.assertEqual(ts, ts2)

    def test_date_time_sub_seconds_floating_point(self):
        ts = sqlite.Timestamp(2004, 2, 14, 7, 15, 0, 510241)
        self.cur.execute("insert into test(ts) values (?)", (ts,))
        self.cur.execute("select ts from test")
        ts2 = self.cur.fetchone()[0]
        self.assertEqual(ts, ts2)

def suite():
    tests = [
        BinaryConverterTests,
        ColNamesTests,
        CommonTableExpressionTests,
        DateTimeTests,
        DeclTypesTests,
        ObjectAdaptationTests,
        SqliteTypeTests,
    ]
    return unittest.TestSuite(
        [unittest.TestLoader().loadTestsFromTestCase(t) for t in tests]
    )

def test():
    runner = unittest.TextTestRunner()
    runner.run(suite())

if __name__ == "__main__":
    test()
