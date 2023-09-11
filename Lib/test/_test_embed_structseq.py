import sys
import types

# Note: This test file can't import `unittest` since the runtime can't
# currently guarantee that it will not leak memory. Doing so will mark
# the test as passing but with reference leaks. This can safely import
# the `unittest` library once there's a strict guarantee of no leaks
# during runtime shutdown.

# bpo-46417: Test that structseq types used by the sys module are still
# valid when Py_Finalize()/Py_Initialize() are called multiple times.
class TestStructSeq:
    # test PyTypeObject members
    def _check_structseq(self, obj_type):
        # ob_refcnt
        assert sys.getrefcount(obj_type) > 1
        # tp_base
        assert issubclass(obj_type, tuple)
        # tp_bases
        assert obj_type.__bases__ == (tuple,)
        # tp_dict
        assert isinstance(obj_type.__dict__, types.MappingProxyType)
        # tp_mro
        assert obj_type.__mro__ == (obj_type, tuple, object)
        # tp_name
        assert isinstance(type.__name__, str)
        # tp_subclasses
        assert obj_type.__subclasses__() == []

    def test_sys_attrs(self):
        for attr_name in (
            'flags',          # FlagsType
            'float_info',     # FloatInfoType
            'hash_info',      # Hash_InfoType
            'int_info',       # Int_InfoType
            'thread_info',    # ThreadInfoType
            'version_info',   # VersionInfoType
        ):
            attr = getattr(sys, attr_name)
            self._check_structseq(type(attr))

    def test_sys_funcs(self):
        func_names = ['get_asyncgen_hooks']  # AsyncGenHooksType
        if hasattr(sys, 'getwindowsversion'):
            func_names.append('getwindowsversion')  # WindowsVersionType
        for func_name in func_names:
            func = getattr(sys, func_name)
            obj = func()
            self._check_structseq(type(obj))


try:
    tests = TestStructSeq()
    tests.test_sys_attrs()
    tests.test_sys_funcs()
except SystemExit as exc:
    if exc.args[0] != 0:
        raise
print("Tests passed")
