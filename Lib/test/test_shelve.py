import unittest
import dbm
import shelve
import pickle
import os

from test.support import os_helper
from collections.abc import MutableMapping
from test.test_dbm import dbm_iterator

def L1(s):
    return s.decode("latin-1")

class byteskeydict(MutableMapping):
    "Mapping that supports bytes keys"

    def __init__(self):
        self.d = {}

    def __getitem__(self, key):
        return self.d[L1(key)]

    def __setitem__(self, key, value):
        self.d[L1(key)] = value

    def __delitem__(self, key):
        del self.d[L1(key)]

    def __len__(self):
        return len(self.d)

    def iterkeys(self):
        for k in self.d.keys():
            yield k.encode("latin-1")

    __iter__ = iterkeys

    def keys(self):
        return list(self.iterkeys())

    def copy(self):
        return byteskeydict(self.d)


class TestCase(unittest.TestCase):
    dirname = os_helper.TESTFN
    fn = os.path.join(os_helper.TESTFN, "shelftemp.db")

    def test_close(self):
        d1 = {}
        s = shelve.Shelf(d1, protocol=2, writeback=False)
        s['key1'] = [1,2,3,4]
        self.assertEqual(s['key1'], [1,2,3,4])
        self.assertEqual(len(s), 1)
        s.close()
        self.assertRaises(ValueError, len, s)
        try:
            s['key1']
        except ValueError:
            pass
        else:
            self.fail('Closed shelf should not find a key')

    def test_open_template(self, filename=None, protocol=None):
        os.mkdir(self.dirname)
        self.addCleanup(os_helper.rmtree, self.dirname)
        s = shelve.open(filename=filename if filename is not None else self.fn,
                        protocol=protocol)
        try:
            s['key1'] = (1,2,3,4)
            self.assertEqual(s['key1'], (1,2,3,4))
        finally:
            s.close()

    def test_ascii_file_shelf(self):
        self.test_open_template(protocol=0)

    def test_binary_file_shelf(self):
        self.test_open_template(protocol=1)

    def test_proto2_file_shelf(self):
        self.test_open_template(protocol=2)

    def test_pathlib_path_file_shelf(self):
        self.test_open_template(filename=os_helper.FakePath(self.fn))

    def test_bytes_path_file_shelf(self):
        self.test_open_template(filename=os.fsencode(self.fn))

    def test_pathlib_bytes_path_file_shelf(self):
        self.test_open_template(filename=os_helper.FakePath(os.fsencode(self.fn)))

    def test_in_memory_shelf(self):
        d1 = byteskeydict()
        with shelve.Shelf(d1, protocol=0) as s:
            s['key1'] = (1,2,3,4)
            self.assertEqual(s['key1'], (1,2,3,4))
        d2 = byteskeydict()
        with shelve.Shelf(d2, protocol=1) as s:
            s['key1'] = (1,2,3,4)
            self.assertEqual(s['key1'], (1,2,3,4))

        self.assertEqual(len(d1), 1)
        self.assertEqual(len(d2), 1)
        self.assertNotEqual(d1.items(), d2.items())

    def test_mutable_entry(self):
        d1 = byteskeydict()
        with shelve.Shelf(d1, protocol=2, writeback=False) as s:
            s['key1'] = [1,2,3,4]
            self.assertEqual(s['key1'], [1,2,3,4])
            s['key1'].append(5)
            self.assertEqual(s['key1'], [1,2,3,4])

        d2 = byteskeydict()
        with shelve.Shelf(d2, protocol=2, writeback=True) as s:
            s['key1'] = [1,2,3,4]
            self.assertEqual(s['key1'], [1,2,3,4])
            s['key1'].append(5)
            self.assertEqual(s['key1'], [1,2,3,4,5])

        self.assertEqual(len(d1), 1)
        self.assertEqual(len(d2), 1)

    def test_keyencoding(self):
        d = {}
        key = 'PÃ¶p'
        # the default keyencoding is utf-8
        shelve.Shelf(d)[key] = [1]
        self.assertIn(key.encode('utf-8'), d)
        # but a different one can be given
        shelve.Shelf(d, keyencoding='latin-1')[key] = [1]
        self.assertIn(key.encode('latin-1'), d)
        # with all consequences
        s = shelve.Shelf(d, keyencoding='ascii')
        self.assertRaises(UnicodeEncodeError, s.__setitem__, key, [1])

    def test_writeback_also_writes_immediately(self):
        # Issue 5754
        d = {}
        key = 'key'
        encodedkey = key.encode('utf-8')
        with shelve.Shelf(d, writeback=True) as s:
            s[key] = [1]
            p1 = d[encodedkey]  # Will give a KeyError if backing store not updated
            s['key'].append(2)
        p2 = d[encodedkey]
        self.assertNotEqual(p1, p2)  # Write creates new object in store

    def test_with(self):
        d1 = {}
        with shelve.Shelf(d1, protocol=2, writeback=False) as s:
            s['key1'] = [1,2,3,4]
            self.assertEqual(s['key1'], [1,2,3,4])
            self.assertEqual(len(s), 1)
        self.assertRaises(ValueError, len, s)
        try:
            s['key1']
        except ValueError:
            pass
        else:
            self.fail('Closed shelf should not find a key')

    def test_default_protocol(self):
        with shelve.Shelf({}) as s:
            self.assertEqual(s._protocol, pickle.DEFAULT_PROTOCOL)


class TestShelveBase:
    type2test = shelve.Shelf

    def _reference(self):
        return {"key1":"value1", "key2":2, "key3":(1,2,3)}


class TestShelveInMemBase(TestShelveBase):
    def _empty_mapping(self):
        return shelve.Shelf(byteskeydict(), **self._args)


class TestShelveFileBase(TestShelveBase):
    counter = 0

    def _empty_mapping(self):
        self.counter += 1
        x = shelve.open(self.base_path + str(self.counter), **self._args)
        self.addCleanup(x.close)
        return x

    def setUp(self):
        dirname = os_helper.TESTFN
        os.mkdir(dirname)
        self.addCleanup(os_helper.rmtree, dirname)
        self.base_path = os.path.join(dirname, "shelftemp.db")
        self.addCleanup(setattr, dbm, '_defaultmod', dbm._defaultmod)
        dbm._defaultmod = self.dbm_mod


from test import mapping_tests

for proto in range(pickle.HIGHEST_PROTOCOL + 1):
    bases = (TestShelveInMemBase, mapping_tests.BasicTestMappingProtocol)
    name = f'TestProto{proto}MemShelve'
    globals()[name] = type(name, bases,
                           {'_args': {'protocol': proto}})
    bases = (TestShelveFileBase, mapping_tests.BasicTestMappingProtocol)
    for dbm_mod in dbm_iterator():
        assert dbm_mod.__name__.startswith('dbm.')
        suffix = dbm_mod.__name__[4:]
        name = f'TestProto{proto}File_{suffix}Shelve'
        globals()[name] = type(name, bases,
                               {'dbm_mod': dbm_mod, '_args': {'protocol': proto}})


if __name__ == "__main__":
    unittest.main()
