import copy
import os
import pickle
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
        self.assertTrue(rep.startswith("os.stat_result"))
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

        s = "123456789"
        self.assertEqual("".join(t(s)), s)

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
