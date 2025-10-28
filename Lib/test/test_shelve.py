import array
import unittest
import dbm
import shelve
import pickle
import os

from test.support import import_helper, os_helper
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

    def test_custom_serializer_and_deserializer(self):
        os.mkdir(self.dirname)
        self.addCleanup(os_helper.rmtree, self.dirname)

        def serializer(obj, protocol):
            if isinstance(obj, (bytes, bytearray, str)):
                if protocol == 5:
                    return obj
                return type(obj).__name__
            elif isinstance(obj, array.array):
                return obj.tobytes()
            raise TypeError(f"Unsupported type for serialization: {type(obj)}")

        def deserializer(data):
            if isinstance(data, (bytes, bytearray, str)):
                return data.decode("utf-8")
            elif isinstance(data, array.array):
                return array.array("b", data)
            raise TypeError(
                f"Unsupported type for deserialization: {type(data)}"
            )

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto), shelve.open(
                self.fn,
                protocol=proto,
                serializer=serializer,
                deserializer=deserializer
            ) as s:
                bar = "bar"
                bytes_data = b"Hello, world!"
                bytearray_data = bytearray(b"\x00\x01\x02\x03\x04")
                array_data = array.array("i", [1, 2, 3, 4, 5])

                s["foo"] = bar
                s["bytes_data"] = bytes_data
                s["bytearray_data"] = bytearray_data
                s["array_data"] = array_data

                if proto == 5:
                    self.assertEqual(s["foo"], str(bar))
                    self.assertEqual(s["bytes_data"], "Hello, world!")
                    self.assertEqual(
                        s["bytearray_data"], bytearray_data.decode()
                    )
                    self.assertEqual(
                        s["array_data"], array_data.tobytes().decode()
                    )
                else:
                    self.assertEqual(s["foo"], "str")
                    self.assertEqual(s["bytes_data"], "bytes")
                    self.assertEqual(s["bytearray_data"], "bytearray")
                    self.assertEqual(
                        s["array_data"], array_data.tobytes().decode()
                    )

    def test_custom_incomplete_serializer_and_deserializer(self):
        dbm_sqlite3 = import_helper.import_module("dbm.sqlite3")
        os.mkdir(self.dirname)
        self.addCleanup(os_helper.rmtree, self.dirname)

        with self.assertRaises(dbm_sqlite3.error):
            def serializer(obj, protocol=None):
                pass

            def deserializer(data):
                return data.decode("utf-8")

            with shelve.open(self.fn, serializer=serializer,
                             deserializer=deserializer) as s:
                s["foo"] = "bar"

        def serializer(obj, protocol=None):
            return type(obj).__name__.encode("utf-8")

        def deserializer(data):
            pass

        with shelve.open(self.fn, serializer=serializer,
                         deserializer=deserializer) as s:
            s["foo"] = "bar"
            self.assertNotEqual(s["foo"], "bar")
            self.assertIsNone(s["foo"])

    def test_custom_serializer_and_deserializer_bsd_db_shelf(self):
        berkeleydb = import_helper.import_module("berkeleydb")
        os.mkdir(self.dirname)
        self.addCleanup(os_helper.rmtree, self.dirname)

        def serializer(obj, protocol=None):
            data = obj.__class__.__name__
            if protocol == 5:
                data = str(len(data))
            return data.encode("utf-8")

        def deserializer(data):
            return data.decode("utf-8")

        def type_name_len(obj):
            return f"{(len(type(obj).__name__))}"

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto), shelve.BsdDbShelf(
                berkeleydb.btopen(self.fn),
                protocol=proto,
                serializer=serializer,
                deserializer=deserializer,
            ) as s:
                bar = "bar"
                bytes_obj = b"Hello, world!"
                bytearray_obj = bytearray(b"\x00\x01\x02\x03\x04")
                arr_obj = array.array("i", [1, 2, 3, 4, 5])

                s["foo"] = bar
                s["bytes_data"] = bytes_obj
                s["bytearray_data"] = bytearray_obj
                s["array_data"] = arr_obj

                if proto == 5:
                    self.assertEqual(s["foo"], type_name_len(bar))
                    self.assertEqual(s["bytes_data"], type_name_len(bytes_obj))
                    self.assertEqual(s["bytearray_data"],
                                     type_name_len(bytearray_obj))
                    self.assertEqual(s["array_data"], type_name_len(arr_obj))

                    k, v = s.set_location(b"foo")
                    self.assertEqual(k, "foo")
                    self.assertEqual(v, type_name_len(bar))

                    k, v = s.previous()
                    self.assertEqual(k, "bytes_data")
                    self.assertEqual(v, type_name_len(bytes_obj))

                    k, v = s.previous()
                    self.assertEqual(k, "bytearray_data")
                    self.assertEqual(v, type_name_len(bytearray_obj))

                    k, v = s.previous()
                    self.assertEqual(k, "array_data")
                    self.assertEqual(v, type_name_len(arr_obj))

                    k, v = s.next()
                    self.assertEqual(k, "bytearray_data")
                    self.assertEqual(v, type_name_len(bytearray_obj))

                    k, v = s.next()
                    self.assertEqual(k, "bytes_data")
                    self.assertEqual(v, type_name_len(bytes_obj))

                    k, v = s.first()
                    self.assertEqual(k, "array_data")
                    self.assertEqual(v, type_name_len(arr_obj))
                else:
                    k, v = s.set_location(b"foo")
                    self.assertEqual(k, "foo")
                    self.assertEqual(v, "str")

                    k, v = s.previous()
                    self.assertEqual(k, "bytes_data")
                    self.assertEqual(v, "bytes")

                    k, v = s.previous()
                    self.assertEqual(k, "bytearray_data")
                    self.assertEqual(v, "bytearray")

                    k, v = s.previous()
                    self.assertEqual(k, "array_data")
                    self.assertEqual(v, "array")

                    k, v = s.next()
                    self.assertEqual(k, "bytearray_data")
                    self.assertEqual(v, "bytearray")

                    k, v = s.next()
                    self.assertEqual(k, "bytes_data")
                    self.assertEqual(v, "bytes")

                    k, v = s.first()
                    self.assertEqual(k, "array_data")
                    self.assertEqual(v, "array")

                    self.assertEqual(s["foo"], "str")
                    self.assertEqual(s["bytes_data"], "bytes")
                    self.assertEqual(s["bytearray_data"], "bytearray")
                    self.assertEqual(s["array_data"], "array")

    def test_custom_incomplete_serializer_and_deserializer_bsd_db_shelf(self):
        berkeleydb = import_helper.import_module("berkeleydb")
        os.mkdir(self.dirname)
        self.addCleanup(os_helper.rmtree, self.dirname)

        def serializer(obj, protocol=None):
            return type(obj).__name__.encode("utf-8")

        def deserializer(data):
            pass

        with shelve.BsdDbShelf(berkeleydb.btopen(self.fn),
                               serializer=serializer,
                               deserializer=deserializer) as s:
            s["foo"] = "bar"
            self.assertIsNone(s["foo"])
            self.assertNotEqual(s["foo"], "bar")

        def serializer(obj, protocol=None):
            pass

        def deserializer(data):
            return data.decode("utf-8")

        with shelve.BsdDbShelf(berkeleydb.btopen(self.fn),
                               serializer=serializer,
                               deserializer=deserializer) as s:
            s["foo"] = "bar"
            self.assertEqual(s["foo"], "")
            self.assertNotEqual(s["foo"], "bar")

    def test_missing_custom_deserializer(self):
        def serializer(obj, protocol=None):
            pass

        kwargs = dict(protocol=2, writeback=False, serializer=serializer)
        self.assertRaises(shelve.ShelveError, shelve.Shelf, {}, **kwargs)
        self.assertRaises(shelve.ShelveError, shelve.BsdDbShelf, {}, **kwargs)

    def test_missing_custom_serializer(self):
        def deserializer(data):
            pass

        kwargs = dict(protocol=2, writeback=False, deserializer=deserializer)
        self.assertRaises(shelve.ShelveError, shelve.Shelf, {}, **kwargs)
        self.assertRaises(shelve.ShelveError, shelve.BsdDbShelf, {}, **kwargs)


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
