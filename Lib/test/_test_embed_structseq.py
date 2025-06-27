import sys
import types
import unittest


# bpo-46417: Test that structseq types used by the sys module are still
# valid when Py_Finalize()/Py_Initialize() are called multiple times.
class TestStructSeq(unittest.TestCase):
    # test PyTypeObject members
    def check_structseq(self, obj_type):
        # ob_refcnt
        self.assertGreaterEqual(sys.getrefcount(obj_type), 1)
        # tp_base
        self.assertIsSubclass(obj_type, tuple)
        # tp_bases
        self.assertEqual(obj_type.__bases__, (tuple,))
        # tp_dict
        self.assertIsInstance(obj_type.__dict__, types.MappingProxyType)
        # tp_mro
        self.assertEqual(obj_type.__mro__, (obj_type, tuple, object))
        # tp_name
        self.assertIsInstance(type.__name__, str)
        # tp_subclasses
        self.assertEqual(obj_type.__subclasses__(), [])

    def test_sys_attrs(self):
        for attr_name in (
            'flags',          # FlagsType
            'float_info',     # FloatInfoType
            'hash_info',      # Hash_InfoType
            'int_info',       # Int_InfoType
            'thread_info',    # ThreadInfoType
            'version_info',   # VersionInfoType
        ):
            with self.subTest(attr=attr_name):
                attr = getattr(sys, attr_name)
                self.check_structseq(type(attr))

    def test_sys_funcs(self):
        func_names = ['get_asyncgen_hooks']  # AsyncGenHooksType
        if hasattr(sys, 'getwindowsversion'):
            func_names.append('getwindowsversion')  # WindowsVersionType
        for func_name in func_names:
            with self.subTest(func=func_name):
                func = getattr(sys, func_name)
                obj = func()
                self.check_structseq(type(obj))


try:
    unittest.main(
        module=(
            '__main__'
            if __name__ == '__main__'
            # Avoiding a circular import:
            else sys.modules['test._test_embed_structseq']
        )
    )
except SystemExit as exc:
    if exc.args[0] != 0:
        raise
print("Tests passed")
