import copy
import os
import pickle
import re
import textwrap
import time
import unittest
from test.support import script_helper


class StructSeqTest(unittest.TestCase):

    def test_tuple(self):
        t = time.gmtime()
        self.assertIsInstance(t, tuple)
        astuple = tuple(t)
        self.assertEqual(len(t), len(astuple))
        self.assertEqual(t, astuple)

        # Check that slicing works the same way; at one point, slicing t[i:j] with
        # 0 < i < j could produce NULLs in the result.
        for i in range(-len(t), len(t)):
            self.assertEqual(t[i:], astuple[i:])
            for j in range(-len(t), len(t)):
                self.assertEqual(t[i:j], astuple[i:j])

        for j in range(-len(t), len(t)):
            self.assertEqual(t[:j], astuple[:j])

        self.assertRaises(IndexError, t.__getitem__, -len(t)-1)
        self.assertRaises(IndexError, t.__getitem__, len(t))
        for i in range(-len(t), len(t)-1):
            self.assertEqual(t[i], astuple[i])

    def test_repr(self):
        t = time.gmtime()
        self.assertTrue(repr(t))
        t = time.gmtime(0)
        self.assertEqual(repr(t),
            "time.struct_time(tm_year=1970, tm_mon=1, tm_mday=1, tm_hour=0, "
            "tm_min=0, tm_sec=0, tm_wday=3, tm_yday=1, tm_isdst=0)")
        # os.stat() gives a complicated struct sequence.
        st = os.stat(__file__)
        rep = repr(st)
        self.assertStartsWith(rep, "os.stat_result")
        self.assertIn("st_mode=", rep)
        self.assertIn("st_ino=", rep)
        self.assertIn("st_dev=", rep)

    def test_concat(self):
        t1 = time.gmtime()
        t2 = t1 + tuple(t1)
        for i in range(len(t1)):
            self.assertEqual(t2[i], t2[i+len(t1)])

    def test_repeat(self):
        t1 = time.gmtime()
        t2 = 3 * t1
        for i in range(len(t1)):
            self.assertEqual(t2[i], t2[i+len(t1)])
            self.assertEqual(t2[i], t2[i+2*len(t1)])

    def test_contains(self):
        t1 = time.gmtime()
        for item in t1:
            self.assertIn(item, t1)
        self.assertNotIn(-42, t1)

    def test_hash(self):
        t1 = time.gmtime()
        self.assertEqual(hash(t1), hash(tuple(t1)))

    def test_cmp(self):
        t1 = time.gmtime()
        t2 = type(t1)(t1)
        self.assertEqual(t1, t2)
        self.assertTrue(not (t1 < t2))
        self.assertTrue(t1 <= t2)
        self.assertTrue(not (t1 > t2))
        self.assertTrue(t1 >= t2)
        self.assertTrue(not (t1 != t2))

    def test_fields(self):
        t = time.gmtime()
        self.assertEqual(len(t), t.n_sequence_fields)
        self.assertEqual(t.n_unnamed_fields, 0)
        self.assertEqual(t.n_fields, time._STRUCT_TM_ITEMS)

    def test_constructor(self):
        t = time.struct_time

        self.assertRaises(TypeError, t)
        self.assertRaises(TypeError, t, None)
        self.assertRaises(TypeError, t, "123")
        self.assertRaises(TypeError, t, "123", dict={})
        self.assertRaises(TypeError, t, "123456789", dict=None)
        self.assertRaises(TypeError, t, seq="123456789", dict={})

        self.assertEqual(t("123456789"), tuple("123456789"))
        self.assertEqual(t("123456789", {}), tuple("123456789"))
        self.assertEqual(t("123456789", dict={}), tuple("123456789"))
        self.assertEqual(t(sequence="123456789", dict={}), tuple("123456789"))

        self.assertEqual(t("1234567890"), tuple("123456789"))
        self.assertEqual(t("1234567890").tm_zone, "0")
        self.assertEqual(t("123456789", {"tm_zone": "some zone"}), tuple("123456789"))
        self.assertEqual(t("123456789", {"tm_zone": "some zone"}).tm_zone, "some zone")

        s = "123456789"
        self.assertEqual("".join(t(s)), s)

    def test_constructor_with_duplicate_fields(self):
        t = time.struct_time

        error_message = re.escape("got duplicate or unexpected field name(s)")
        with self.assertRaisesRegex(TypeError, error_message):
            t("1234567890", dict={"tm_zone": "some zone"})
        with self.assertRaisesRegex(TypeError, error_message):
            t("1234567890", dict={"tm_zone": "some zone", "tm_mon": 1})
        with self.assertRaisesRegex(TypeError, error_message):
            t("1234567890", dict={"error": 0, "tm_zone": "some zone"})
        with self.assertRaisesRegex(TypeError, error_message):
            t("1234567890", dict={"error": 0, "tm_zone": "some zone", "tm_mon": 1})

    def test_constructor_with_duplicate_unnamed_fields(self):
        assert os.stat_result.n_unnamed_fields > 0
        n_visible_fields = os.stat_result.n_sequence_fields

        r = os.stat_result(range(n_visible_fields), {'st_atime': -1.0})
        self.assertEqual(r.st_atime, -1.0)
        self.assertEqual(r, tuple(range(n_visible_fields)))

        r = os.stat_result((*range(n_visible_fields), -1.0))
        self.assertEqual(r.st_atime, -1.0)
        self.assertEqual(r, tuple(range(n_visible_fields)))

        with self.assertRaisesRegex(TypeError,
                                    re.escape("got duplicate or unexpected field name(s)")):
            os.stat_result((*range(n_visible_fields), -1.0), {'st_atime': -1.0})

    def test_constructor_with_unknown_fields(self):
        t = time.struct_time

        error_message = re.escape("got duplicate or unexpected field name(s)")
        with self.assertRaisesRegex(TypeError, error_message):
            t("123456789", dict={"tm_year": 0})
        with self.assertRaisesRegex(TypeError, error_message):
            t("123456789", dict={"tm_year": 0, "tm_mon": 1})
        with self.assertRaisesRegex(TypeError, error_message):
            t("123456789", dict={"tm_zone": "some zone", "tm_mon": 1})
        with self.assertRaisesRegex(TypeError, error_message):
            t("123456789", dict={"tm_zone": "some zone", "error": 0})
        with self.assertRaisesRegex(TypeError, error_message):
            t("123456789", dict={"error": 0, "tm_zone": "some zone", "tm_mon": 1})
        with self.assertRaisesRegex(TypeError, error_message):
            t("123456789", dict={"error": 0})
        with self.assertRaisesRegex(TypeError, error_message):
            t("123456789", dict={"tm_zone": "some zone", "error": 0})

    def test_eviltuple(self):
        class Exc(Exception):
            pass

        # Devious code could crash structseqs' constructors
        class C:
            def __getitem__(self, i):
                raise Exc
            def __len__(self):
                return 9

        self.assertRaises(Exc, time.struct_time, C())

    def test_pickling(self):
        t = time.gmtime()
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            p = pickle.dumps(t, proto)
            t2 = pickle.loads(p)
            self.assertEqual(t2.__class__, t.__class__)
            self.assertEqual(t2, t)
            self.assertEqual(t2.tm_year, t.tm_year)
            self.assertEqual(t2.tm_zone, t.tm_zone)

    def test_pickling_with_unnamed_fields(self):
        assert os.stat_result.n_unnamed_fields > 0

        r = os.stat_result(range(os.stat_result.n_sequence_fields),
                           {'st_atime': 1.0, 'st_atime_ns': 2.0})
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            p = pickle.dumps(r, proto)
            r2 = pickle.loads(p)
            self.assertEqual(r2.__class__, r.__class__)
            self.assertEqual(r2, r)
            self.assertEqual(r2.st_mode, r.st_mode)
            self.assertEqual(r2.st_atime, r.st_atime)
            self.assertEqual(r2.st_atime_ns, r.st_atime_ns)

    def test_copying(self):
        n_fields = time.struct_time.n_fields
        t = time.struct_time([[i] for i in range(n_fields)])

        t2 = copy.copy(t)
        self.assertEqual(t2.__class__, t.__class__)
        self.assertEqual(t2, t)
        self.assertEqual(t2.tm_year, t.tm_year)
        self.assertEqual(t2.tm_zone, t.tm_zone)
        self.assertIs(t2[0], t[0])
        self.assertIs(t2.tm_year, t.tm_year)

        t3 = copy.deepcopy(t)
        self.assertEqual(t3.__class__, t.__class__)
        self.assertEqual(t3, t)
        self.assertEqual(t3.tm_year, t.tm_year)
        self.assertEqual(t3.tm_zone, t.tm_zone)
        self.assertIsNot(t3[0], t[0])
        self.assertIsNot(t3.tm_year, t.tm_year)

    def test_copying_with_unnamed_fields(self):
        assert os.stat_result.n_unnamed_fields > 0

        n_sequence_fields = os.stat_result.n_sequence_fields
        r = os.stat_result([[i] for i in range(n_sequence_fields)],
                           {'st_atime': [1.0], 'st_atime_ns': [2.0]})

        r2 = copy.copy(r)
        self.assertEqual(r2.__class__, r.__class__)
        self.assertEqual(r2, r)
        self.assertEqual(r2.st_mode, r.st_mode)
        self.assertEqual(r2.st_atime, r.st_atime)
        self.assertEqual(r2.st_atime_ns, r.st_atime_ns)
        self.assertIs(r2[0], r[0])
        self.assertIs(r2.st_mode, r.st_mode)
        self.assertIs(r2.st_atime, r.st_atime)
        self.assertIs(r2.st_atime_ns, r.st_atime_ns)

        r3 = copy.deepcopy(r)
        self.assertEqual(r3.__class__, r.__class__)
        self.assertEqual(r3, r)
        self.assertEqual(r3.st_mode, r.st_mode)
        self.assertEqual(r3.st_atime, r.st_atime)
        self.assertEqual(r3.st_atime_ns, r.st_atime_ns)
        self.assertIsNot(r3[0], r[0])
        self.assertIsNot(r3.st_mode, r.st_mode)
        self.assertIsNot(r3.st_atime, r.st_atime)
        self.assertIsNot(r3.st_atime_ns, r.st_atime_ns)

    def test_extended_getslice(self):
        # Test extended slicing by comparing with list slicing.
        t = time.gmtime()
        L = list(t)
        indices = (0, None, 1, 3, 19, 300, -1, -2, -31, -300)
        for start in indices:
            for stop in indices:
                # Skip step 0 (invalid)
                for step in indices[1:]:
                    self.assertEqual(list(t[start:stop:step]),
                                     L[start:stop:step])

    def test_match_args(self):
        expected_args = ('tm_year', 'tm_mon', 'tm_mday', 'tm_hour', 'tm_min',
                         'tm_sec', 'tm_wday', 'tm_yday', 'tm_isdst')
        self.assertEqual(time.struct_time.__match_args__, expected_args)

    def test_match_args_with_unnamed_fields(self):
        expected_args = ('st_mode', 'st_ino', 'st_dev', 'st_nlink', 'st_uid',
                         'st_gid', 'st_size')
        self.assertEqual(os.stat_result.n_unnamed_fields, 3)
        self.assertEqual(os.stat_result.__match_args__, expected_args)

    def test_copy_replace_all_fields_visible(self):
        assert os.times_result.n_unnamed_fields == 0
        assert os.times_result.n_sequence_fields == os.times_result.n_fields

        t = os.times()

        # visible fields
        self.assertEqual(copy.replace(t), t)
        self.assertIsInstance(copy.replace(t), os.times_result)
        self.assertEqual(copy.replace(t, user=1.5), (1.5, *t[1:]))
        self.assertEqual(copy.replace(t, system=2.5), (t[0], 2.5, *t[2:]))
        self.assertEqual(copy.replace(t, user=1.5, system=2.5), (1.5, 2.5, *t[2:]))

        # unknown fields
        with self.assertRaisesRegex(TypeError, 'unexpected field name'):
            copy.replace(t, error=-1)
        with self.assertRaisesRegex(TypeError, 'unexpected field name'):
            copy.replace(t, user=1, error=-1)

    def test_copy_replace_with_invisible_fields(self):
        assert time.struct_time.n_unnamed_fields == 0
        assert time.struct_time.n_sequence_fields < time.struct_time.n_fields

        t = time.gmtime(0)

        # visible fields
        t2 = copy.replace(t)
        self.assertEqual(t2, (1970, 1, 1, 0, 0, 0, 3, 1, 0))
        self.assertIsInstance(t2, time.struct_time)
        t3 = copy.replace(t, tm_year=2000)
        self.assertEqual(t3, (2000, 1, 1, 0, 0, 0, 3, 1, 0))
        self.assertEqual(t3.tm_year, 2000)
        t4 = copy.replace(t, tm_mon=2)
        self.assertEqual(t4, (1970, 2, 1, 0, 0, 0, 3, 1, 0))
        self.assertEqual(t4.tm_mon, 2)
        t5 = copy.replace(t, tm_year=2000, tm_mon=2)
        self.assertEqual(t5, (2000, 2, 1, 0, 0, 0, 3, 1, 0))
        self.assertEqual(t5.tm_year, 2000)
        self.assertEqual(t5.tm_mon, 2)

        # named invisible fields
        self.assertHasAttr(t, 'tm_zone')
        with self.assertRaisesRegex(AttributeError, 'readonly attribute'):
            t.tm_zone = 'some other zone'
        self.assertEqual(t2.tm_zone, t.tm_zone)
        self.assertEqual(t3.tm_zone, t.tm_zone)
        self.assertEqual(t4.tm_zone, t.tm_zone)
        t6 = copy.replace(t, tm_zone='some other zone')
        self.assertEqual(t, t6)
        self.assertEqual(t6.tm_zone, 'some other zone')
        t7 = copy.replace(t, tm_year=2000, tm_zone='some other zone')
        self.assertEqual(t7, (2000, 1, 1, 0, 0, 0, 3, 1, 0))
        self.assertEqual(t7.tm_year, 2000)
        self.assertEqual(t7.tm_zone, 'some other zone')

        # unknown fields
        with self.assertRaisesRegex(TypeError, 'unexpected field name'):
            copy.replace(t, error=2)
        with self.assertRaisesRegex(TypeError, 'unexpected field name'):
            copy.replace(t, tm_year=2000, error=2)
        with self.assertRaisesRegex(TypeError, 'unexpected field name'):
            copy.replace(t, tm_zone='some other zone', error=2)

    def test_copy_replace_with_unnamed_fields(self):
        assert os.stat_result.n_unnamed_fields > 0

        r = os.stat_result(range(os.stat_result.n_sequence_fields))

        error_message = re.escape('__replace__() is not supported')
        with self.assertRaisesRegex(TypeError, error_message):
            copy.replace(r)
        with self.assertRaisesRegex(TypeError, error_message):
            copy.replace(r, st_mode=1)
        with self.assertRaisesRegex(TypeError, error_message):
            copy.replace(r, error=2)
        with self.assertRaisesRegex(TypeError, error_message):
            copy.replace(r, st_mode=1, error=2)

    def test_reference_cycle(self):
        # gh-122527: Check that a structseq that's part of a reference cycle
        # with its own type doesn't crash. Previously, if the type's dictionary
        # was cleared first, the structseq instance would crash in the
        # destructor.
        script_helper.assert_python_ok("-c", textwrap.dedent(r"""
            import time
            t = time.gmtime()
            type(t).refcyle = t
        """))


if __name__ == "__main__":
    unittest.main()
