import weakref
import unittest
from test.support import import_helper

_testcapi = import_helper.import_module('_testcapi')
_testlimitedcapi = import_helper.import_module('_testlimitedcapi')
NULL = None

class Object:
    pass

class Ref(weakref.ReferenceType):
    pass


class CAPIWeakrefTest(unittest.TestCase):
    def test_pyweakref_check(self):
        # Test PyWeakref_Check()
        check = _testlimitedcapi.pyweakref_check
        obj = Object()
        self.assertEqual(check(obj), 0)
        self.assertEqual(check(weakref.ref(obj)), 1)
        self.assertEqual(check(Ref(obj)), 1)
        self.assertEqual(check(weakref.proxy(obj)), 1)

        # CRASHES check(NULL)

    def test_pyweakref_checkref(self):
        # Test PyWeakref_CheckRef()
        checkref = _testlimitedcapi.pyweakref_checkref
        obj = Object()
        self.assertEqual(checkref(obj), 0)
        self.assertEqual(checkref(weakref.ref(obj)), 1)
        self.assertEqual(checkref(Ref(obj)), 1)
        self.assertEqual(checkref(weakref.proxy(obj)), 0)

        # CRASHES checkref(NULL)

    def test_pyweakref_checkrefexact(self):
        # Test PyWeakref_CheckRefExact()
        checkrefexact = _testlimitedcapi.pyweakref_checkrefexact
        obj = Object()
        self.assertEqual(checkrefexact(obj), 0)
        self.assertEqual(checkrefexact(weakref.ref(obj)), 1)
        self.assertEqual(checkrefexact(Ref(obj)), 0)
        self.assertEqual(checkrefexact(weakref.proxy(obj)), 0)

        # CRASHES checkrefexact(NULL)

    def test_pyweakref_checkproxy(self):
        # Test PyWeakref_CheckProxy()
        checkproxy = _testlimitedcapi.pyweakref_checkproxy
        obj = Object()
        self.assertEqual(checkproxy(obj), 0)
        self.assertEqual(checkproxy(weakref.ref(obj)), 0)
        self.assertEqual(checkproxy(Ref(obj)), 0)
        self.assertEqual(checkproxy(weakref.proxy(obj)), 1)

        # CRASHES checkproxy(NULL)

    def test_pyweakref_getref(self):
        # Test PyWeakref_GetRef()
        getref = _testcapi.pyweakref_getref
        obj = Object()
        wr = weakref.ref(obj)
        wp = weakref.proxy(obj)
        self.assertEqual(getref(wr), (1, obj))
        self.assertEqual(getref(wp), (1, obj))
        del obj
        self.assertEqual(getref(wr), 0)
        self.assertEqual(getref(wp), 0)

        self.assertRaises(TypeError, getref, 42)
        self.assertRaises(SystemError, getref, NULL)

    def test_pyweakref_isdead(self):
        # Test PyWeakref_IsDead()
        isdead = _testcapi.pyweakref_isdead
        obj = Object()
        wr = weakref.ref(obj)
        wp = weakref.proxy(obj)
        self.assertEqual(isdead(wr), 0)
        self.assertEqual(isdead(wp), 0)
        del obj
        self.assertEqual(isdead(wr), 1)
        self.assertEqual(isdead(wp), 1)

        self.assertRaises(TypeError, isdead, 42)
        self.assertRaises(SystemError, isdead, NULL)

    def test_pyweakref_newref(self):
        # Test PyWeakref_NewRef()
        newref = _testlimitedcapi.pyweakref_newref
        obj = Object()
        wr = newref(obj)
        self.assertIs(type(wr), weakref.ReferenceType)
        # PyWeakref_NewRef() handles None callback as NULL callback
        wr = newref(obj, None)
        self.assertIs(type(wr), weakref.ReferenceType)
        self.assertRaises(TypeError, newref, obj, 42)
        log = []
        wr = newref(obj, log.append)
        self.assertIs(type(wr), weakref.ReferenceType)
        self.assertEqual(log, [])
        del obj
        self.assertEqual(log, [wr])

        self.assertRaises(TypeError, newref, [])
        # CRASHES newref(NULL)

    def test_pyweakref_newproxy(self):
        # Test PyWeakref_NewProxy()
        newproxy = _testlimitedcapi.pyweakref_newproxy
        obj = Object()
        wp = newproxy(obj)
        self.assertIs(type(wp), weakref.ProxyType)
        # PyWeakref_NewProxy() handles None callback as NULL callback
        wp = newproxy(obj, None)
        self.assertIs(type(wp), weakref.ProxyType)
        self.assertRaises(TypeError, newproxy, obj, 42)
        log = []
        wp = newproxy(obj, log.append)
        self.assertIs(type(wp), weakref.ProxyType)
        self.assertEqual(log, [])
        del obj
        self.assertEqual(log, [wp])

        def func():
            pass
        wp = newproxy(func)
        self.assertIs(type(wp), weakref.CallableProxyType)

        self.assertRaises(TypeError, newproxy, [])
        # CRASHES newproxy(NULL)


if __name__ == "__main__":
    unittest.main()
