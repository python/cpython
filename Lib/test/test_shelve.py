import unittest
import shelve
import glob
from test import test_support
from UserDict import DictMixin
from test.test_anydbm import dbm_iterator

def L1(s):
    return s.decode("latin-1")

class byteskeydict(DictMixin):
    "Mapping that supports bytes keys"

    def __init__(self):
        self.d = {}

    def __getitem__(self, key):
        return self.d[L1(key)]

    def __setitem__(self, key, value):
        self.d[L1(key)] = value

    def __delitem__(self, key):
        del self.d[L1(key)]

    def iterkeys(self):
        for k in self.d.keys():
            yield k.decode("latin-1")

    def keys(self):
        return list(self.iterkeys())

    def copy(self):
        return byteskeydict(self.d)


class TestCase(unittest.TestCase):

    fn = "shelftemp.db"

    def tearDown(self):
        for f in glob.glob(self.fn+"*"):
            test_support.unlink(f)

    def test_ascii_file_shelf(self):
        s = shelve.open(self.fn, protocol=0)
        try:
            s['key1'] = (1,2,3,4)
            self.assertEqual(s['key1'], (1,2,3,4))
        finally:
            s.close()

    def test_binary_file_shelf(self):
        s = shelve.open(self.fn, protocol=1)
        try:
            s['key1'] = (1,2,3,4)
            self.assertEqual(s['key1'], (1,2,3,4))
        finally:
            s.close()

    def test_proto2_file_shelf(self):
        s = shelve.open(self.fn, protocol=2)
        try:
            s['key1'] = (1,2,3,4)
            self.assertEqual(s['key1'], (1,2,3,4))
        finally:
            s.close()

    def test_in_memory_shelf(self):
        d1 = byteskeydict()
        s = shelve.Shelf(d1, protocol=0)
        s['key1'] = (1,2,3,4)
        self.assertEqual(s['key1'], (1,2,3,4))
        s.close()
        d2 = byteskeydict()
        s = shelve.Shelf(d2, protocol=1)
        s['key1'] = (1,2,3,4)
        self.assertEqual(s['key1'], (1,2,3,4))
        s.close()

        self.assertEqual(len(d1), 1)
        self.assertNotEqual(d1, d2)

    def test_mutable_entry(self):
        d1 = byteskeydict()
        s = shelve.Shelf(d1, protocol=2, writeback=False)
        s['key1'] = [1,2,3,4]
        self.assertEqual(s['key1'], [1,2,3,4])
        s['key1'].append(5)
        self.assertEqual(s['key1'], [1,2,3,4])
        s.close()

        d2 = byteskeydict()
        s = shelve.Shelf(d2, protocol=2, writeback=True)
        s['key1'] = [1,2,3,4]
        self.assertEqual(s['key1'], [1,2,3,4])
        s['key1'].append(5)
        self.assertEqual(s['key1'], [1,2,3,4,5])
        s.close()

        self.assertEqual(len(d1), 1)
        self.assertEqual(len(d2), 1)


from test import mapping_tests

class TestShelveBase(mapping_tests.BasicTestMappingProtocol):
    fn = "shelftemp.db"
    counter = 0
    def __init__(self, *args, **kw):
        self._db = []
        mapping_tests.BasicTestMappingProtocol.__init__(self, *args, **kw)
    type2test = shelve.Shelf
    def _reference(self):
        return {"key1":"value1", "key2":2, "key3":(1,2,3)}
    def _empty_mapping(self):
        if self._in_mem:
            x= shelve.Shelf(byteskeydict(), **self._args)
        else:
            self.counter+=1
            x= shelve.open(self.fn+str(self.counter), **self._args)
        self._db.append(x)
        return x
    def tearDown(self):
        for db in self._db:
            db.close()
        self._db = []
        if not self._in_mem:
            for f in glob.glob(self.fn+"*"):
                test_support.unlink(f)

class TestAsciiFileShelve(TestShelveBase):
    _args={'protocol':0}
    _in_mem = False
class TestBinaryFileShelve(TestShelveBase):
    _args={'protocol':1}
    _in_mem = False
class TestProto2FileShelve(TestShelveBase):
    _args={'protocol':2}
    _in_mem = False
class TestAsciiMemShelve(TestShelveBase):
    _args={'protocol':0}
    _in_mem = True
class TestBinaryMemShelve(TestShelveBase):
    _args={'protocol':1}
    _in_mem = True
class TestProto2MemShelve(TestShelveBase):
    _args={'protocol':2}
    _in_mem = True

def test_main():
    for module in dbm_iterator():
        test_support.run_unittest(
            TestAsciiFileShelve,
            TestBinaryFileShelve,
            TestProto2FileShelve,
            TestAsciiMemShelve,
            TestBinaryMemShelve,
            TestProto2MemShelve,
            TestCase
        )

if __name__ == "__main__":
    test_main()
