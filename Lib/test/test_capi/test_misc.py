# Run the _testcapi module tests (tests for the Python/C API):  by defn,
# these are all functions _testcapi exports whose name begins with 'test_'.

import _thread
from collections import deque
import contextlib
import importlib.machinery
import importlib.util
import json
import os
import pickle
import queue
import random
import sys
import textwrap
import threading
import time
import types
import unittest
import warnings
import weakref
import operator
from test import support
from test.support import MISSING_C_DOCSTRINGS
from test.support import import_helper
from test.support import threading_helper
from test.support import warnings_helper
from test.support import requires_limited_api
from test.support.script_helper import assert_python_failure, assert_python_ok, run_python_until_end
try:
    import _posixsubprocess
except ImportError:
    _posixsubprocess = None
try:
    import _testmultiphase
except ImportError:
    _testmultiphase = None
try:
    import _testsinglephase
except ImportError:
    _testsinglephase = None
try:
    import _xxsubinterpreters as _interpreters
except ModuleNotFoundError:
    _interpreters = None

# Skip this test if the _testcapi module isn't available.
_testcapi = import_helper.import_module('_testcapi')

import _testinternalcapi


NULL = None

def decode_stderr(err):
    return err.decode('utf-8', 'replace').replace('\r', '')


def requires_subinterpreters(meth):
    """Decorator to skip a test if subinterpreters are not supported."""
    return unittest.skipIf(_interpreters is None,
                           'subinterpreters required')(meth)


def testfunction(self):
    """some doc"""
    return self


class InstanceMethod:
    id = _testcapi.instancemethod(id)
    testfunction = _testcapi.instancemethod(testfunction)

class CAPITest(unittest.TestCase):

    def test_instancemethod(self):
        inst = InstanceMethod()
        self.assertEqual(id(inst), inst.id())
        self.assertTrue(inst.testfunction() is inst)
        self.assertEqual(inst.testfunction.__doc__, testfunction.__doc__)
        self.assertEqual(InstanceMethod.testfunction.__doc__, testfunction.__doc__)

        InstanceMethod.testfunction.attribute = "test"
        self.assertEqual(testfunction.attribute, "test")
        self.assertRaises(AttributeError, setattr, inst.testfunction, "attribute", "test")

    @support.requires_subprocess()
    def test_no_FatalError_infinite_loop(self):
        code = textwrap.dedent("""
            import _testcapi
            from test import support

            with support.SuppressCrashReport():
                _testcapi.crash_no_current_thread()
        """)

        run_result, _cmd_line = run_python_until_end('-c', code)
        _rc, out, err = run_result
        self.assertEqual(out, b'')
        # This used to cause an infinite loop.
        msg = ("Fatal Python error: PyThreadState_Get: "
               "the function must be called with the GIL held, "
               "after Python initialization and before Python finalization, "
               "but the GIL is released "
               "(the current Python thread state is NULL)").encode()
        self.assertTrue(err.rstrip().startswith(msg),
                        err)

    def test_memoryview_from_NULL_pointer(self):
        self.assertRaises(ValueError, _testcapi.make_memoryview_from_NULL_pointer)

    @unittest.skipUnless(_posixsubprocess, '_posixsubprocess required for this test.')
    def test_seq_bytes_to_charp_array(self):
        # Issue #15732: crash in _PySequence_BytesToCharpArray()
        class Z(object):
            def __len__(self):
                return 1
        with self.assertRaisesRegex(TypeError, 'indexing'):
            _posixsubprocess.fork_exec(
                          1,Z(),True,(1, 2),5,6,7,8,9,10,11,12,13,14,True,True,17,False,19,20,21,22,False)
        # Issue #15736: overflow in _PySequence_BytesToCharpArray()
        class Z(object):
            def __len__(self):
                return sys.maxsize
            def __getitem__(self, i):
                return b'x'
        self.assertRaises(MemoryError, _posixsubprocess.fork_exec,
                          1,Z(),True,(1, 2),5,6,7,8,9,10,11,12,13,14,True,True,17,False,19,20,21,22,False)

    @unittest.skipUnless(_posixsubprocess, '_posixsubprocess required for this test.')
    def test_subprocess_fork_exec(self):
        class Z(object):
            def __len__(self):
                return 1

        # Issue #15738: crash in subprocess_fork_exec()
        self.assertRaises(TypeError, _posixsubprocess.fork_exec,
                          Z(),[b'1'],True,(1, 2),5,6,7,8,9,10,11,12,13,14,True,True,17,False,19,20,21,22,False)

    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_docstring_signature_parsing(self):

        self.assertEqual(_testcapi.no_docstring.__doc__, None)
        self.assertEqual(_testcapi.no_docstring.__text_signature__, None)

        self.assertEqual(_testcapi.docstring_empty.__doc__, None)
        self.assertEqual(_testcapi.docstring_empty.__text_signature__, None)

        self.assertEqual(_testcapi.docstring_no_signature.__doc__,
            "This docstring has no signature.")
        self.assertEqual(_testcapi.docstring_no_signature.__text_signature__, None)

        self.assertEqual(_testcapi.docstring_with_invalid_signature.__doc__,
            "docstring_with_invalid_signature($module, /, boo)\n"
            "\n"
            "This docstring has an invalid signature."
            )
        self.assertEqual(_testcapi.docstring_with_invalid_signature.__text_signature__, None)

        self.assertEqual(_testcapi.docstring_with_invalid_signature2.__doc__,
            "docstring_with_invalid_signature2($module, /, boo)\n"
            "\n"
            "--\n"
            "\n"
            "This docstring also has an invalid signature."
            )
        self.assertEqual(_testcapi.docstring_with_invalid_signature2.__text_signature__, None)

        self.assertEqual(_testcapi.docstring_with_signature.__doc__,
            "This docstring has a valid signature.")
        self.assertEqual(_testcapi.docstring_with_signature.__text_signature__, "($module, /, sig)")

        self.assertEqual(_testcapi.docstring_with_signature_but_no_doc.__doc__, None)
        self.assertEqual(_testcapi.docstring_with_signature_but_no_doc.__text_signature__,
            "($module, /, sig)")

        self.assertEqual(_testcapi.docstring_with_signature_and_extra_newlines.__doc__,
            "\nThis docstring has a valid signature and some extra newlines.")
        self.assertEqual(_testcapi.docstring_with_signature_and_extra_newlines.__text_signature__,
            "($module, /, parameter)")

    def test_c_type_with_matrix_multiplication(self):
        M = _testcapi.matmulType
        m1 = M()
        m2 = M()
        self.assertEqual(m1 @ m2, ("matmul", m1, m2))
        self.assertEqual(m1 @ 42, ("matmul", m1, 42))
        self.assertEqual(42 @ m1, ("matmul", 42, m1))
        o = m1
        o @= m2
        self.assertEqual(o, ("imatmul", m1, m2))
        o = m1
        o @= 42
        self.assertEqual(o, ("imatmul", m1, 42))
        o = 42
        o @= m1
        self.assertEqual(o, ("matmul", 42, m1))

    def test_c_type_with_ipow(self):
        # When the __ipow__ method of a type was implemented in C, using the
        # modulo param would cause segfaults.
        o = _testcapi.ipowType()
        self.assertEqual(o.__ipow__(1), (1, None))
        self.assertEqual(o.__ipow__(2, 2), (2, 2))

    def test_return_null_without_error(self):
        # Issue #23571: A function must not return NULL without setting an
        # error
        if support.Py_DEBUG:
            code = textwrap.dedent("""
                import _testcapi
                from test import support

                with support.SuppressCrashReport():
                    _testcapi.return_null_without_error()
            """)
            rc, out, err = assert_python_failure('-c', code)
            err = decode_stderr(err)
            self.assertRegex(err,
                r'Fatal Python error: _Py_CheckFunctionResult: '
                    r'a function returned NULL without setting an exception\n'
                r'Python runtime state: initialized\n'
                r'SystemError: <built-in function return_null_without_error> '
                    r'returned NULL without setting an exception\n'
                r'\n'
                r'Current thread.*:\n'
                r'  File .*", line 6 in <module>\n')
        else:
            with self.assertRaises(SystemError) as cm:
                _testcapi.return_null_without_error()
            self.assertRegex(str(cm.exception),
                             'return_null_without_error.* '
                             'returned NULL without setting an exception')

    def test_return_result_with_error(self):
        # Issue #23571: A function must not return a result with an error set
        if support.Py_DEBUG:
            code = textwrap.dedent("""
                import _testcapi
                from test import support

                with support.SuppressCrashReport():
                    _testcapi.return_result_with_error()
            """)
            rc, out, err = assert_python_failure('-c', code)
            err = decode_stderr(err)
            self.assertRegex(err,
                    r'Fatal Python error: _Py_CheckFunctionResult: '
                        r'a function returned a result with an exception set\n'
                    r'Python runtime state: initialized\n'
                    r'ValueError\n'
                    r'\n'
                    r'The above exception was the direct cause '
                        r'of the following exception:\n'
                    r'\n'
                    r'SystemError: <built-in '
                        r'function return_result_with_error> '
                        r'returned a result with an exception set\n'
                    r'\n'
                    r'Current thread.*:\n'
                    r'  File .*, line 6 in <module>\n')
        else:
            with self.assertRaises(SystemError) as cm:
                _testcapi.return_result_with_error()
            self.assertRegex(str(cm.exception),
                             'return_result_with_error.* '
                             'returned a result with an exception set')

    def test_getitem_with_error(self):
        # Test _Py_CheckSlotResult(). Raise an exception and then calls
        # PyObject_GetItem(): check that the assertion catches the bug.
        # PyObject_GetItem() must not be called with an exception set.
        code = textwrap.dedent("""
            import _testcapi
            from test import support

            with support.SuppressCrashReport():
                _testcapi.getitem_with_error({1: 2}, 1)
        """)
        rc, out, err = assert_python_failure('-c', code)
        err = decode_stderr(err)
        if 'SystemError: ' not in err:
            self.assertRegex(err,
                    r'Fatal Python error: _Py_CheckSlotResult: '
                        r'Slot __getitem__ of type dict succeeded '
                        r'with an exception set\n'
                    r'Python runtime state: initialized\n'
                    r'ValueError: bug\n'
                    r'\n'
                    r'Current thread .* \(most recent call first\):\n'
                    r'  File .*, line 6 in <module>\n'
                    r'\n'
                    r'Extension modules: _testcapi \(total: 1\)\n')
        else:
            # Python built with NDEBUG macro defined:
            # test _Py_CheckFunctionResult() instead.
            self.assertIn('returned a result with an exception set', err)

    def test_buildvalue(self):
        # Test Py_BuildValue() with object arguments
        buildvalue = _testcapi.py_buildvalue
        self.assertEqual(buildvalue(''), None)
        self.assertEqual(buildvalue('()'), ())
        self.assertEqual(buildvalue('[]'), [])
        self.assertEqual(buildvalue('{}'), {})
        self.assertEqual(buildvalue('()[]{}'), ((), [], {}))
        self.assertEqual(buildvalue('O', 1), 1)
        self.assertEqual(buildvalue('(O)', 1), (1,))
        self.assertEqual(buildvalue('[O]', 1), [1])
        self.assertRaises(SystemError, buildvalue, '{O}', 1)
        self.assertEqual(buildvalue('OO', 1, 2), (1, 2))
        self.assertEqual(buildvalue('(OO)', 1, 2), (1, 2))
        self.assertEqual(buildvalue('[OO]', 1, 2), [1, 2])
        self.assertEqual(buildvalue('{OO}', 1, 2), {1: 2})
        self.assertEqual(buildvalue('{OOOO}', 1, 2, 3, 4), {1: 2, 3: 4})
        self.assertEqual(buildvalue('((O))', 1), ((1,),))
        self.assertEqual(buildvalue('((OO))', 1, 2), ((1, 2),))

        self.assertEqual(buildvalue(' \t,:'), None)
        self.assertEqual(buildvalue('O,', 1), 1)
        self.assertEqual(buildvalue('   O   ', 1), 1)
        self.assertEqual(buildvalue('\tO\t', 1), 1)
        self.assertEqual(buildvalue('O,O', 1, 2), (1, 2))
        self.assertEqual(buildvalue('O, O', 1, 2), (1, 2))
        self.assertEqual(buildvalue('O,\tO', 1, 2), (1, 2))
        self.assertEqual(buildvalue('O O', 1, 2), (1, 2))
        self.assertEqual(buildvalue('O\tO', 1, 2), (1, 2))
        self.assertEqual(buildvalue('(O,O)', 1, 2), (1, 2))
        self.assertEqual(buildvalue('(O, O,)', 1, 2), (1, 2))
        self.assertEqual(buildvalue(' ( O O ) ', 1, 2), (1, 2))
        self.assertEqual(buildvalue('\t(\tO\tO\t)\t', 1, 2), (1, 2))
        self.assertEqual(buildvalue('[O,O]', 1, 2), [1, 2])
        self.assertEqual(buildvalue('[O, O,]', 1, 2), [1, 2])
        self.assertEqual(buildvalue(' [ O O ] ', 1, 2), [1, 2])
        self.assertEqual(buildvalue(' [\tO\tO\t] ', 1, 2), [1, 2])
        self.assertEqual(buildvalue('{O:O}', 1, 2), {1: 2})
        self.assertEqual(buildvalue('{O:O,O:O}', 1, 2, 3, 4), {1: 2, 3: 4})
        self.assertEqual(buildvalue('{O: O, O: O,}', 1, 2, 3, 4), {1: 2, 3: 4})
        self.assertEqual(buildvalue(' { O O O O } ', 1, 2, 3, 4), {1: 2, 3: 4})
        self.assertEqual(buildvalue('\t{\tO\tO\tO\tO\t}\t', 1, 2, 3, 4), {1: 2, 3: 4})

        self.assertRaises(SystemError, buildvalue, 'O', NULL)
        self.assertRaises(SystemError, buildvalue, '(O)', NULL)
        self.assertRaises(SystemError, buildvalue, '[O]', NULL)
        self.assertRaises(SystemError, buildvalue, '{O}', NULL)
        self.assertRaises(SystemError, buildvalue, 'OO', 1, NULL)
        self.assertRaises(SystemError, buildvalue, 'OO', NULL, 2)
        self.assertRaises(SystemError, buildvalue, '(OO)', 1, NULL)
        self.assertRaises(SystemError, buildvalue, '(OO)', NULL, 2)
        self.assertRaises(SystemError, buildvalue, '[OO]', 1, NULL)
        self.assertRaises(SystemError, buildvalue, '[OO]', NULL, 2)
        self.assertRaises(SystemError, buildvalue, '{OO}', 1, NULL)
        self.assertRaises(SystemError, buildvalue, '{OO}', NULL, 2)

    def test_buildvalue_ints(self):
        # Test Py_BuildValue() with integer arguments
        buildvalue = _testcapi.py_buildvalue_ints
        from _testcapi import SHRT_MIN, SHRT_MAX, USHRT_MAX, INT_MIN, INT_MAX, UINT_MAX
        self.assertEqual(buildvalue('i', INT_MAX), INT_MAX)
        self.assertEqual(buildvalue('i', INT_MIN), INT_MIN)
        self.assertEqual(buildvalue('I', UINT_MAX), UINT_MAX)

        self.assertEqual(buildvalue('h', SHRT_MAX), SHRT_MAX)
        self.assertEqual(buildvalue('h', SHRT_MIN), SHRT_MIN)
        self.assertEqual(buildvalue('H', USHRT_MAX), USHRT_MAX)

        self.assertEqual(buildvalue('b', 127), 127)
        self.assertEqual(buildvalue('b', -128), -128)
        self.assertEqual(buildvalue('B', 255), 255)

        self.assertEqual(buildvalue('c', ord('A')), b'A')
        self.assertEqual(buildvalue('c', 255), b'\xff')
        self.assertEqual(buildvalue('c', 256), b'\x00')
        self.assertEqual(buildvalue('c', -1), b'\xff')

        self.assertEqual(buildvalue('C', 255), chr(255))
        self.assertEqual(buildvalue('C', 256), chr(256))
        self.assertEqual(buildvalue('C', sys.maxunicode), chr(sys.maxunicode))
        self.assertRaises(ValueError, buildvalue, 'C', -1)
        self.assertRaises(ValueError, buildvalue, 'C', sys.maxunicode+1)

        # gh-84489
        self.assertRaises(ValueError, buildvalue, '(C )i', -1, 2)
        self.assertRaises(ValueError, buildvalue, '[C ]i', -1, 2)
        self.assertRaises(ValueError, buildvalue, '{Ci }i', -1, 2, 3)

    def test_buildvalue_N(self):
        _testcapi.test_buildvalue_N()

    def check_negative_refcount(self, code):
        # bpo-35059: Check that Py_DECREF() reports the correct filename
        # when calling _Py_NegativeRefcount() to abort Python.
        code = textwrap.dedent(code)
        rc, out, err = assert_python_failure('-c', code)
        self.assertRegex(err,
                         br'_testcapimodule\.c:[0-9]+: '
                         br'_Py_NegativeRefcount: Assertion failed: '
                         br'object has negative ref count')

    @unittest.skipUnless(hasattr(_testcapi, 'negative_refcount'),
                         'need _testcapi.negative_refcount()')
    def test_negative_refcount(self):
        code = """
            import _testcapi
            from test import support

            with support.SuppressCrashReport():
                _testcapi.negative_refcount()
        """
        self.check_negative_refcount(code)

    @unittest.skipUnless(hasattr(_testcapi, 'decref_freed_object'),
                         'need _testcapi.decref_freed_object()')
    @support.skip_if_sanitizer("use after free on purpose",
                               address=True, memory=True, ub=True)
    def test_decref_freed_object(self):
        code = """
            import _testcapi
            from test import support

            with support.SuppressCrashReport():
                _testcapi.decref_freed_object()
        """
        self.check_negative_refcount(code)

    def test_trashcan_subclass(self):
        # bpo-35983: Check that the trashcan mechanism for "list" is NOT
        # activated when its tp_dealloc is being called by a subclass
        from _testcapi import MyList
        L = None
        for i in range(1000):
            L = MyList((L,))

    @support.requires_resource('cpu')
    def test_trashcan_python_class1(self):
        self.do_test_trashcan_python_class(list)

    @support.requires_resource('cpu')
    def test_trashcan_python_class2(self):
        from _testcapi import MyList
        self.do_test_trashcan_python_class(MyList)

    def do_test_trashcan_python_class(self, base):
        # Check that the trashcan mechanism works properly for a Python
        # subclass of a class using the trashcan (this specific test assumes
        # that the base class "base" behaves like list)
        class PyList(base):
            # Count the number of PyList instances to verify that there is
            # no memory leak
            num = 0
            def __init__(self, *args):
                __class__.num += 1
                super().__init__(*args)
            def __del__(self):
                __class__.num -= 1

        for parity in (0, 1):
            L = None
            # We need in the order of 2**20 iterations here such that a
            # typical 8MB stack would overflow without the trashcan.
            for i in range(2**20):
                L = PyList((L,))
                L.attr = i
            if parity:
                # Add one additional nesting layer
                L = (L,)
            self.assertGreater(PyList.num, 0)
            del L
            self.assertEqual(PyList.num, 0)

    @unittest.skipIf(MISSING_C_DOCSTRINGS,
                     "Signature information for builtins requires docstrings")
    def test_heap_ctype_doc_and_text_signature(self):
        self.assertEqual(_testcapi.HeapDocCType.__doc__, "somedoc")
        self.assertEqual(_testcapi.HeapDocCType.__text_signature__, "(arg1, arg2)")

    def test_null_type_doc(self):
        self.assertEqual(_testcapi.NullTpDocType.__doc__, None)

    def test_subclass_of_heap_gc_ctype_with_tpdealloc_decrefs_once(self):
        class HeapGcCTypeSubclass(_testcapi.HeapGcCType):
            def __init__(self):
                self.value2 = 20
                super().__init__()

        subclass_instance = HeapGcCTypeSubclass()
        type_refcnt = sys.getrefcount(HeapGcCTypeSubclass)

        # Test that subclass instance was fully created
        self.assertEqual(subclass_instance.value, 10)
        self.assertEqual(subclass_instance.value2, 20)

        # Test that the type reference count is only decremented once
        del subclass_instance
        self.assertEqual(type_refcnt - 1, sys.getrefcount(HeapGcCTypeSubclass))

    def test_subclass_of_heap_gc_ctype_with_del_modifying_dunder_class_only_decrefs_once(self):
        class A(_testcapi.HeapGcCType):
            def __init__(self):
                self.value2 = 20
                super().__init__()

        class B(A):
            def __init__(self):
                super().__init__()

            def __del__(self):
                self.__class__ = A
                A.refcnt_in_del = sys.getrefcount(A)
                B.refcnt_in_del = sys.getrefcount(B)

        subclass_instance = B()
        type_refcnt = sys.getrefcount(B)
        new_type_refcnt = sys.getrefcount(A)

        # Test that subclass instance was fully created
        self.assertEqual(subclass_instance.value, 10)
        self.assertEqual(subclass_instance.value2, 20)

        del subclass_instance

        # Test that setting __class__ modified the reference counts of the types
        if support.Py_DEBUG:
            # gh-89373: In debug mode, _Py_Dealloc() keeps a strong reference
            # to the type while calling tp_dealloc()
            self.assertEqual(type_refcnt, B.refcnt_in_del)
        else:
            self.assertEqual(type_refcnt - 1, B.refcnt_in_del)
        self.assertEqual(new_type_refcnt + 1, A.refcnt_in_del)

        # Test that the original type already has decreased its refcnt
        self.assertEqual(type_refcnt - 1, sys.getrefcount(B))

        # Test that subtype_dealloc decref the newly assigned __class__ only once
        self.assertEqual(new_type_refcnt, sys.getrefcount(A))

    def test_heaptype_with_dict(self):
        inst = _testcapi.HeapCTypeWithDict()
        inst.foo = 42
        self.assertEqual(inst.foo, 42)
        self.assertEqual(inst.dictobj, inst.__dict__)
        self.assertEqual(inst.dictobj, {"foo": 42})

        inst = _testcapi.HeapCTypeWithDict()
        self.assertEqual({}, inst.__dict__)

    def test_heaptype_with_managed_dict(self):
        inst = _testcapi.HeapCTypeWithManagedDict()
        inst.foo = 42
        self.assertEqual(inst.foo, 42)
        self.assertEqual(inst.__dict__, {"foo": 42})

        inst = _testcapi.HeapCTypeWithManagedDict()
        self.assertEqual({}, inst.__dict__)

        a = _testcapi.HeapCTypeWithManagedDict()
        b = _testcapi.HeapCTypeWithManagedDict()
        a.b = b
        b.a = a
        del a, b

    def test_sublclassing_managed_dict(self):

        class C(_testcapi.HeapCTypeWithManagedDict):
            pass

        i = C()
        i.spam = i
        del i

    def test_heaptype_with_negative_dict(self):
        inst = _testcapi.HeapCTypeWithNegativeDict()
        inst.foo = 42
        self.assertEqual(inst.foo, 42)
        self.assertEqual(inst.dictobj, inst.__dict__)
        self.assertEqual(inst.dictobj, {"foo": 42})

        inst = _testcapi.HeapCTypeWithNegativeDict()
        self.assertEqual({}, inst.__dict__)

    def test_heaptype_with_weakref(self):
        inst = _testcapi.HeapCTypeWithWeakref()
        ref = weakref.ref(inst)
        self.assertEqual(ref(), inst)
        self.assertEqual(inst.weakreflist, ref)

    def test_heaptype_with_managed_weakref(self):
        inst = _testcapi.HeapCTypeWithManagedWeakref()
        ref = weakref.ref(inst)
        self.assertEqual(ref(), inst)

    def test_sublclassing_managed_weakref(self):

        class C(_testcapi.HeapCTypeWithManagedWeakref):
            pass

        inst = C()
        ref = weakref.ref(inst)
        self.assertEqual(ref(), inst)

    def test_sublclassing_managed_both(self):

        class C1(_testcapi.HeapCTypeWithManagedWeakref, _testcapi.HeapCTypeWithManagedDict):
            pass

        class C2(_testcapi.HeapCTypeWithManagedDict, _testcapi.HeapCTypeWithManagedWeakref):
            pass

        for cls in (C1, C2):
            inst = cls()
            ref = weakref.ref(inst)
            self.assertEqual(ref(), inst)
            inst.spam = inst
            del inst
            ref = weakref.ref(cls())
            self.assertIs(ref(), None)

    def test_heaptype_with_buffer(self):
        inst = _testcapi.HeapCTypeWithBuffer()
        b = bytes(inst)
        self.assertEqual(b, b"1234")

    def test_c_subclass_of_heap_ctype_with_tpdealloc_decrefs_once(self):
        subclass_instance = _testcapi.HeapCTypeSubclass()
        type_refcnt = sys.getrefcount(_testcapi.HeapCTypeSubclass)

        # Test that subclass instance was fully created
        self.assertEqual(subclass_instance.value, 10)
        self.assertEqual(subclass_instance.value2, 20)

        # Test that the type reference count is only decremented once
        del subclass_instance
        self.assertEqual(type_refcnt - 1, sys.getrefcount(_testcapi.HeapCTypeSubclass))

    def test_c_subclass_of_heap_ctype_with_del_modifying_dunder_class_only_decrefs_once(self):
        subclass_instance = _testcapi.HeapCTypeSubclassWithFinalizer()
        type_refcnt = sys.getrefcount(_testcapi.HeapCTypeSubclassWithFinalizer)
        new_type_refcnt = sys.getrefcount(_testcapi.HeapCTypeSubclass)

        # Test that subclass instance was fully created
        self.assertEqual(subclass_instance.value, 10)
        self.assertEqual(subclass_instance.value2, 20)

        # The tp_finalize slot will set __class__ to HeapCTypeSubclass
        del subclass_instance

        # Test that setting __class__ modified the reference counts of the types
        if support.Py_DEBUG:
            # gh-89373: In debug mode, _Py_Dealloc() keeps a strong reference
            # to the type while calling tp_dealloc()
            self.assertEqual(type_refcnt, _testcapi.HeapCTypeSubclassWithFinalizer.refcnt_in_del)
        else:
            self.assertEqual(type_refcnt - 1, _testcapi.HeapCTypeSubclassWithFinalizer.refcnt_in_del)
        self.assertEqual(new_type_refcnt + 1, _testcapi.HeapCTypeSubclass.refcnt_in_del)

        # Test that the original type already has decreased its refcnt
        self.assertEqual(type_refcnt - 1, sys.getrefcount(_testcapi.HeapCTypeSubclassWithFinalizer))

        # Test that subtype_dealloc decref the newly assigned __class__ only once
        self.assertEqual(new_type_refcnt, sys.getrefcount(_testcapi.HeapCTypeSubclass))

    def test_heaptype_with_setattro(self):
        obj = _testcapi.HeapCTypeSetattr()
        self.assertEqual(obj.pvalue, 10)
        obj.value = 12
        self.assertEqual(obj.pvalue, 12)
        del obj.value
        self.assertEqual(obj.pvalue, 0)

    def test_heaptype_with_custom_metaclass(self):
        metaclass = _testcapi.HeapCTypeMetaclass
        self.assertTrue(issubclass(metaclass, type))

        # Class creation from C
        t = _testcapi.pytype_fromspec_meta(metaclass)
        self.assertIsInstance(t, type)
        self.assertEqual(t.__name__, "HeapCTypeViaMetaclass")
        self.assertIs(type(t), metaclass)

        # Class creation from Python
        t = metaclass("PyClassViaMetaclass", (), {})
        self.assertIsInstance(t, type)
        self.assertEqual(t.__name__, "PyClassViaMetaclass")

    def test_heaptype_with_custom_metaclass_null_new(self):
        metaclass = _testcapi.HeapCTypeMetaclassNullNew

        self.assertTrue(issubclass(metaclass, type))

        # Class creation from C
        t = _testcapi.pytype_fromspec_meta(metaclass)
        self.assertIsInstance(t, type)
        self.assertEqual(t.__name__, "HeapCTypeViaMetaclass")
        self.assertIs(type(t), metaclass)

        # Class creation from Python
        with self.assertRaisesRegex(TypeError, "cannot create .* instances"):
            metaclass("PyClassViaMetaclass", (), {})

    def test_heaptype_with_custom_metaclass_custom_new(self):
        metaclass = _testcapi.HeapCTypeMetaclassCustomNew

        self.assertTrue(issubclass(_testcapi.HeapCTypeMetaclassCustomNew, type))

        msg = "Metaclasses with custom tp_new are not supported."
        with self.assertRaisesRegex(TypeError, msg):
            t = _testcapi.pytype_fromspec_meta(metaclass)

    def test_heaptype_with_custom_metaclass_deprecation(self):
        metaclass = _testcapi.HeapCTypeMetaclassCustomNew

        # gh-103968: a metaclass with custom tp_new is deprecated, but still
        # allowed for functions that existed in 3.11
        # (PyType_FromSpecWithBases is used here).
        class Base(metaclass=metaclass):
            pass

        # Class creation from C
        with warnings_helper.check_warnings(
                ('.* _testcapi.Subclass .* custom tp_new.*in Python 3.14.*', DeprecationWarning),
                ):
            sub = _testcapi.make_type_with_base(Base)
        self.assertTrue(issubclass(sub, Base))
        self.assertIsInstance(sub, metaclass)

    def test_multiple_inheritance_ctypes_with_weakref_or_dict(self):

        with self.assertRaises(TypeError):
            class Both1(_testcapi.HeapCTypeWithWeakref, _testcapi.HeapCTypeWithDict):
                pass
        with self.assertRaises(TypeError):
            class Both2(_testcapi.HeapCTypeWithDict, _testcapi.HeapCTypeWithWeakref):
                pass

    def test_multiple_inheritance_ctypes_with_weakref_or_dict_and_other_builtin(self):

        with self.assertRaises(TypeError):
            class C1(_testcapi.HeapCTypeWithDict, list):
                pass

        with self.assertRaises(TypeError):
            class C2(_testcapi.HeapCTypeWithWeakref, list):
                pass

        class C3(_testcapi.HeapCTypeWithManagedDict, list):
            pass
        class C4(_testcapi.HeapCTypeWithManagedWeakref, list):
            pass

        inst = C3()
        inst.append(0)
        str(inst.__dict__)

        inst = C4()
        inst.append(0)
        str(inst.__weakref__)

        for cls in (_testcapi.HeapCTypeWithManagedDict, _testcapi.HeapCTypeWithManagedWeakref):
            for cls2 in (_testcapi.HeapCTypeWithDict, _testcapi.HeapCTypeWithWeakref):
                class S(cls, cls2):
                    pass
            class B1(C3, cls):
                pass
            class B2(C4, cls):
                pass

    def test_pytype_fromspec_with_repeated_slots(self):
        for variant in range(2):
            with self.subTest(variant=variant):
                with self.assertRaises(SystemError):
                    _testcapi.create_type_from_repeated_slots(variant)

    @warnings_helper.ignore_warnings(category=DeprecationWarning)
    def test_immutable_type_with_mutable_base(self):
        # Add deprecation warning here so it's removed in 3.14
        warnings._deprecated(
            'creating immutable classes with mutable bases', remove=(3, 14))

        class MutableBase:
            def meth(self):
                return 'original'

        with self.assertWarns(DeprecationWarning):
            ImmutableSubclass = _testcapi.make_immutable_type_with_base(
                MutableBase)
        instance = ImmutableSubclass()

        self.assertEqual(instance.meth(), 'original')

        # Cannot override the static type's method
        with self.assertRaisesRegex(
                TypeError,
                "cannot set 'meth' attribute of immutable type"):
            ImmutableSubclass.meth = lambda self: 'overridden'
        self.assertEqual(instance.meth(), 'original')

        # Can change the method on the mutable base
        MutableBase.meth = lambda self: 'changed'
        self.assertEqual(instance.meth(), 'changed')

    def test_pynumber_tobase(self):
        from _testcapi import pynumber_tobase
        small_number = 123
        large_number = 2**64
        class IDX:
            def __init__(self, val):
                self.val = val
            def __index__(self):
                return self.val

        test_cases = ((2, '0b1111011', '0b10000000000000000000000000000000000000000000000000000000000000000'),
                      (8, '0o173', '0o2000000000000000000000'),
                      (10, '123', '18446744073709551616'),
                      (16, '0x7b', '0x10000000000000000'))
        for base, small_target, large_target in test_cases:
            with self.subTest(base=base, st=small_target, lt=large_target):
                # Test for small number
                self.assertEqual(pynumber_tobase(small_number, base), small_target)
                self.assertEqual(pynumber_tobase(-small_number, base), '-' + small_target)
                self.assertEqual(pynumber_tobase(IDX(small_number), base), small_target)
                # Test for large number(out of range of a longlong,i.e.[-2**63, 2**63-1])
                self.assertEqual(pynumber_tobase(large_number, base), large_target)
                self.assertEqual(pynumber_tobase(-large_number, base), '-' + large_target)
                self.assertEqual(pynumber_tobase(IDX(large_number), base), large_target)
        self.assertRaises(TypeError, pynumber_tobase, IDX(123.0), 10)
        self.assertRaises(TypeError, pynumber_tobase, IDX('123'), 10)
        self.assertRaises(TypeError, pynumber_tobase, 123.0, 10)
        self.assertRaises(TypeError, pynumber_tobase, '123', 10)
        self.assertRaises(SystemError, pynumber_tobase, 123, 0)

    def test_pyobject_repr_from_null(self):
        s = _testcapi.pyobject_repr_from_null()
        self.assertEqual(s, '<NULL>')

    def test_pyobject_str_from_null(self):
        s = _testcapi.pyobject_str_from_null()
        self.assertEqual(s, '<NULL>')

    def test_pyobject_bytes_from_null(self):
        s = _testcapi.pyobject_bytes_from_null()
        self.assertEqual(s, b'<NULL>')

    def test_Py_CompileString(self):
        # Check that Py_CompileString respects the coding cookie
        _compile = _testcapi.Py_CompileString
        code = b"# -*- coding: latin1 -*-\nprint('\xc2\xa4')\n"
        result = _compile(code)
        expected = compile(code, "<string>", "exec")
        self.assertEqual(result.co_consts, expected.co_consts)

    def test_export_symbols(self):
        # bpo-44133: Ensure that the "Py_FrozenMain" and
        # "PyThread_get_thread_native_id" symbols are exported by the Python
        # (directly by the binary, or via by the Python dynamic library).
        ctypes = import_helper.import_module('ctypes')
        names = []

        # Test if the PY_HAVE_THREAD_NATIVE_ID macro is defined
        if hasattr(_thread, 'get_native_id'):
            names.append('PyThread_get_thread_native_id')

        # Python/frozenmain.c fails to build on Windows when the symbols are
        # missing:
        # - PyWinFreeze_ExeInit
        # - PyWinFreeze_ExeTerm
        # - PyInitFrozenExtensions
        if os.name != 'nt':
            names.append('Py_FrozenMain')

        for name in names:
            with self.subTest(name=name):
                self.assertTrue(hasattr(ctypes.pythonapi, name))

    def test_clear_managed_dict(self):

        class C:
            def __init__(self):
                self.a = 1

        c = C()
        _testcapi.clear_managed_dict(c)
        self.assertEqual(c.__dict__, {})
        c = C()
        self.assertEqual(c.__dict__, {'a':1})
        _testcapi.clear_managed_dict(c)
        self.assertEqual(c.__dict__, {})

    def test_eval_get_func_name(self):
        def function_example(): ...

        class A:
            def method_example(self): ...

        self.assertEqual(_testcapi.eval_get_func_name(function_example),
                         "function_example")
        self.assertEqual(_testcapi.eval_get_func_name(A.method_example),
                         "method_example")
        self.assertEqual(_testcapi.eval_get_func_name(A().method_example),
                         "method_example")
        self.assertEqual(_testcapi.eval_get_func_name(sum), "sum")  # c function
        self.assertEqual(_testcapi.eval_get_func_name(A), "type")

    def test_eval_get_func_desc(self):
        def function_example(): ...

        class A:
            def method_example(self): ...

        self.assertEqual(_testcapi.eval_get_func_desc(function_example),
                         "()")
        self.assertEqual(_testcapi.eval_get_func_desc(A.method_example),
                         "()")
        self.assertEqual(_testcapi.eval_get_func_desc(A().method_example),
                         "()")
        self.assertEqual(_testcapi.eval_get_func_desc(sum), "()")  # c function
        self.assertEqual(_testcapi.eval_get_func_desc(A), " object")

    def test_function_get_code(self):
        import types

        def some():
            pass

        code = _testcapi.function_get_code(some)
        self.assertIsInstance(code, types.CodeType)
        self.assertEqual(code, some.__code__)

        with self.assertRaises(SystemError):
            _testcapi.function_get_code(None)  # not a function

    def test_function_get_globals(self):
        def some():
            pass

        globals_ = _testcapi.function_get_globals(some)
        self.assertIsInstance(globals_, dict)
        self.assertEqual(globals_, some.__globals__)

        with self.assertRaises(SystemError):
            _testcapi.function_get_globals(None)  # not a function

    def test_function_get_module(self):
        def some():
            pass

        module = _testcapi.function_get_module(some)
        self.assertIsInstance(module, str)
        self.assertEqual(module, some.__module__)

        with self.assertRaises(SystemError):
            _testcapi.function_get_module(None)  # not a function

    def test_function_get_defaults(self):
        def some(
            pos_only1, pos_only2='p',
            /,
            zero=0, optional=None,
            *,
            kw1,
            kw2=True,
        ):
            pass

        defaults = _testcapi.function_get_defaults(some)
        self.assertEqual(defaults, ('p', 0, None))
        self.assertEqual(defaults, some.__defaults__)

        with self.assertRaises(SystemError):
            _testcapi.function_get_defaults(None)  # not a function

    def test_function_set_defaults(self):
        def some(
            pos_only1, pos_only2='p',
            /,
            zero=0, optional=None,
            *,
            kw1,
            kw2=True,
        ):
            pass

        old_defaults = ('p', 0, None)
        self.assertEqual(_testcapi.function_get_defaults(some), old_defaults)
        self.assertEqual(some.__defaults__, old_defaults)

        with self.assertRaises(SystemError):
            _testcapi.function_set_defaults(some, 1)  # not tuple or None
        self.assertEqual(_testcapi.function_get_defaults(some), old_defaults)
        self.assertEqual(some.__defaults__, old_defaults)

        with self.assertRaises(SystemError):
            _testcapi.function_set_defaults(1, ())    # not a function
        self.assertEqual(_testcapi.function_get_defaults(some), old_defaults)
        self.assertEqual(some.__defaults__, old_defaults)

        new_defaults = ('q', 1, None)
        _testcapi.function_set_defaults(some, new_defaults)
        self.assertEqual(_testcapi.function_get_defaults(some), new_defaults)
        self.assertEqual(some.__defaults__, new_defaults)

        # Empty tuple is fine:
        new_defaults = ()
        _testcapi.function_set_defaults(some, new_defaults)
        self.assertEqual(_testcapi.function_get_defaults(some), new_defaults)
        self.assertEqual(some.__defaults__, new_defaults)

        class tuplesub(tuple): ...  # tuple subclasses must work

        new_defaults = tuplesub(((1, 2), ['a', 'b'], None))
        _testcapi.function_set_defaults(some, new_defaults)
        self.assertEqual(_testcapi.function_get_defaults(some), new_defaults)
        self.assertEqual(some.__defaults__, new_defaults)

        # `None` is special, it sets `defaults` to `NULL`,
        # it needs special handling in `_testcapi`:
        _testcapi.function_set_defaults(some, None)
        self.assertEqual(_testcapi.function_get_defaults(some), None)
        self.assertEqual(some.__defaults__, None)

    def test_function_get_kw_defaults(self):
        def some(
            pos_only1, pos_only2='p',
            /,
            zero=0, optional=None,
            *,
            kw1,
            kw2=True,
        ):
            pass

        defaults = _testcapi.function_get_kw_defaults(some)
        self.assertEqual(defaults, {'kw2': True})
        self.assertEqual(defaults, some.__kwdefaults__)

        with self.assertRaises(SystemError):
            _testcapi.function_get_kw_defaults(None)  # not a function

    def test_function_set_kw_defaults(self):
        def some(
            pos_only1, pos_only2='p',
            /,
            zero=0, optional=None,
            *,
            kw1,
            kw2=True,
        ):
            pass

        old_defaults = {'kw2': True}
        self.assertEqual(_testcapi.function_get_kw_defaults(some), old_defaults)
        self.assertEqual(some.__kwdefaults__, old_defaults)

        with self.assertRaises(SystemError):
            _testcapi.function_set_kw_defaults(some, 1)  # not dict or None
        self.assertEqual(_testcapi.function_get_kw_defaults(some), old_defaults)
        self.assertEqual(some.__kwdefaults__, old_defaults)

        with self.assertRaises(SystemError):
            _testcapi.function_set_kw_defaults(1, {})    # not a function
        self.assertEqual(_testcapi.function_get_kw_defaults(some), old_defaults)
        self.assertEqual(some.__kwdefaults__, old_defaults)

        new_defaults = {'kw2': (1, 2, 3)}
        _testcapi.function_set_kw_defaults(some, new_defaults)
        self.assertEqual(_testcapi.function_get_kw_defaults(some), new_defaults)
        self.assertEqual(some.__kwdefaults__, new_defaults)

        # Empty dict is fine:
        new_defaults = {}
        _testcapi.function_set_kw_defaults(some, new_defaults)
        self.assertEqual(_testcapi.function_get_kw_defaults(some), new_defaults)
        self.assertEqual(some.__kwdefaults__, new_defaults)

        class dictsub(dict): ...  # dict subclasses must work

        new_defaults = dictsub({'kw2': None})
        _testcapi.function_set_kw_defaults(some, new_defaults)
        self.assertEqual(_testcapi.function_get_kw_defaults(some), new_defaults)
        self.assertEqual(some.__kwdefaults__, new_defaults)

        # `None` is special, it sets `kwdefaults` to `NULL`,
        # it needs special handling in `_testcapi`:
        _testcapi.function_set_kw_defaults(some, None)
        self.assertEqual(_testcapi.function_get_kw_defaults(some), None)
        self.assertEqual(some.__kwdefaults__, None)

    def test_unstable_gc_new_with_extra_data(self):
        class Data(_testcapi.ObjExtraData):
            __slots__ = ('x', 'y')

        d = Data()
        d.x = 10
        d.y = 20
        d.extra = 30
        self.assertEqual(d.x, 10)
        self.assertEqual(d.y, 20)
        self.assertEqual(d.extra, 30)
        del d.extra
        self.assertIsNone(d.extra)

    def test_get_type_module_name(self):
        from collections import OrderedDict
        ht = _testcapi.get_heaptype_for_name()
        for cls, expected in {
            int: 'builtins',
            OrderedDict: 'collections',
            ht: '_testcapi',
        }.items():
            with self.subTest(repr(cls)):
                modname = _testinternalcapi.get_type_module_name(cls)
                self.assertEqual(modname, expected)

        ht.__module__ = 'test_module'
        modname = _testinternalcapi.get_type_module_name(ht)
        self.assertEqual(modname, 'test_module')

@requires_limited_api
class TestHeapTypeRelative(unittest.TestCase):
    """Test API for extending opaque types (PEP 697)"""

    @requires_limited_api
    def test_heaptype_relative_sizes(self):
        # Test subclassing using "relative" basicsize, see PEP 697
        def check(extra_base_size, extra_size):
            Base, Sub, instance, data_ptr, data_offset, data_size = (
                _testcapi.make_sized_heaptypes(
                    extra_base_size, -extra_size))

            # no alignment shenanigans when inheriting directly
            if extra_size == 0:
                self.assertEqual(Base.__basicsize__, Sub.__basicsize__)
                self.assertEqual(data_size, 0)

            else:
                # The following offsets should be in increasing order:
                offsets = [
                    (0, 'start of object'),
                    (Base.__basicsize__, 'end of base data'),
                    (data_offset, 'subclass data'),
                    (data_offset + extra_size, 'end of requested subcls data'),
                    (data_offset + data_size, 'end of reserved subcls data'),
                    (Sub.__basicsize__, 'end of object'),
                ]
                ordered_offsets = sorted(offsets, key=operator.itemgetter(0))
                self.assertEqual(
                    offsets, ordered_offsets,
                    msg=f'Offsets not in expected order, got: {ordered_offsets}')

                # end of reserved subcls data == end of object
                self.assertEqual(Sub.__basicsize__, data_offset + data_size)

                # we don't reserve (requested + alignment) or more data
                self.assertLess(data_size - extra_size,
                                _testcapi.ALIGNOF_MAX_ALIGN_T)

            # The offsets/sizes we calculated should be aligned.
            self.assertEqual(data_offset % _testcapi.ALIGNOF_MAX_ALIGN_T, 0)
            self.assertEqual(data_size % _testcapi.ALIGNOF_MAX_ALIGN_T, 0)

        sizes = sorted({0, 1, 2, 3, 4, 7, 8, 123,
                        object.__basicsize__,
                        object.__basicsize__-1,
                        object.__basicsize__+1})
        for extra_base_size in sizes:
            for extra_size in sizes:
                args = dict(extra_base_size=extra_base_size,
                            extra_size=extra_size)
                with self.subTest(**args):
                    check(**args)

    def test_HeapCCollection(self):
        """Make sure HeapCCollection works properly by itself"""
        collection = _testcapi.HeapCCollection(1, 2, 3)
        self.assertEqual(list(collection), [1, 2, 3])

    def test_heaptype_inherit_itemsize(self):
        """Test HeapCCollection subclasses work properly"""
        sizes = sorted({0, 1, 2, 3, 4, 7, 8, 123,
                        object.__basicsize__,
                        object.__basicsize__-1,
                        object.__basicsize__+1})
        for extra_size in sizes:
            with self.subTest(extra_size=extra_size):
                Sub = _testcapi.subclass_var_heaptype(
                    _testcapi.HeapCCollection, -extra_size, 0, 0)
                collection = Sub(1, 2, 3)
                collection.set_data_to_3s()

                self.assertEqual(list(collection), [1, 2, 3])
                mem = collection.get_data()
                self.assertGreaterEqual(len(mem), extra_size)
                self.assertTrue(set(mem) <= {3}, f'got {mem!r}')

    def test_heaptype_invalid_inheritance(self):
        with self.assertRaises(SystemError,
                               msg="Cannot extend variable-size class without "
                               + "Py_TPFLAGS_ITEMS_AT_END"):
            _testcapi.subclass_heaptype(int, -8, 0)

    def test_heaptype_relative_members(self):
        """Test HeapCCollection subclasses work properly"""
        sizes = sorted({0, 1, 2, 3, 4, 7, 8, 123,
                        object.__basicsize__,
                        object.__basicsize__-1,
                        object.__basicsize__+1})
        for extra_base_size in sizes:
            for extra_size in sizes:
                for offset in sizes:
                    with self.subTest(extra_base_size=extra_base_size, extra_size=extra_size, offset=offset):
                        if offset < extra_size:
                            Sub = _testcapi.make_heaptype_with_member(
                                extra_base_size, -extra_size, offset, True)
                            Base = Sub.mro()[1]
                            instance = Sub()
                            self.assertEqual(instance.memb, instance.get_memb())
                            instance.set_memb(13)
                            self.assertEqual(instance.memb, instance.get_memb())
                            self.assertEqual(instance.get_memb(), 13)
                            instance.memb = 14
                            self.assertEqual(instance.memb, instance.get_memb())
                            self.assertEqual(instance.get_memb(), 14)
                            self.assertGreaterEqual(instance.get_memb_offset(), Base.__basicsize__)
                            self.assertLess(instance.get_memb_offset(), Sub.__basicsize__)
                            with self.assertRaises(SystemError):
                                instance.get_memb_relative()
                            with self.assertRaises(SystemError):
                                instance.set_memb_relative(0)
                        else:
                            with self.assertRaises(SystemError):
                                Sub = _testcapi.make_heaptype_with_member(
                                    extra_base_size, -extra_size, offset, True)
                        with self.assertRaises(SystemError):
                            Sub = _testcapi.make_heaptype_with_member(
                                extra_base_size, extra_size, offset, True)
                with self.subTest(extra_base_size=extra_base_size, extra_size=extra_size):
                    with self.assertRaises(SystemError):
                        Sub = _testcapi.make_heaptype_with_member(
                            extra_base_size, -extra_size, -1, True)

    def test_heaptype_relative_members_errors(self):
        with self.assertRaisesRegex(
                SystemError,
                r"With Py_RELATIVE_OFFSET, basicsize must be negative"):
            _testcapi.make_heaptype_with_member(0, 1234, 0, True)
        with self.assertRaisesRegex(
                SystemError, r"Member offset out of range \(0\.\.-basicsize\)"):
            _testcapi.make_heaptype_with_member(0, -8, 1234, True)
        with self.assertRaisesRegex(
                SystemError, r"Member offset out of range \(0\.\.-basicsize\)"):
            _testcapi.make_heaptype_with_member(0, -8, -1, True)

        Sub = _testcapi.make_heaptype_with_member(0, -8, 0, True)
        instance = Sub()
        with self.assertRaisesRegex(
                SystemError, r"PyMember_GetOne used with Py_RELATIVE_OFFSET"):
            instance.get_memb_relative()
        with self.assertRaisesRegex(
                SystemError, r"PyMember_SetOne used with Py_RELATIVE_OFFSET"):
            instance.set_memb_relative(0)

    def test_pyobject_getitemdata_error(self):
        """Test PyObject_GetItemData fails on unsupported types"""
        with self.assertRaises(TypeError):
            # None is not variable-length
            _testcapi.pyobject_getitemdata(None)
        with self.assertRaises(TypeError):
            # int is variable-length, but doesn't have the
            # Py_TPFLAGS_ITEMS_AT_END layout (and flag)
            _testcapi.pyobject_getitemdata(0)


class TestPendingCalls(unittest.TestCase):

    # See the comment in ceval.c (at the "handle_eval_breaker" label)
    # about when pending calls get run.  This is especially relevant
    # here for creating deterministic tests.

    def pendingcalls_submit(self, l, n):
        def callback():
            #this function can be interrupted by thread switching so let's
            #use an atomic operation
            l.append(None)

        for i in range(n):
            time.sleep(random.random()*0.02) #0.01 secs on average
            #try submitting callback until successful.
            #rely on regular interrupt to flush queue if we are
            #unsuccessful.
            while True:
                if _testcapi._pending_threadfunc(callback):
                    break

    def pendingcalls_wait(self, l, n, context = None):
        #now, stick around until l[0] has grown to 10
        count = 0
        while len(l) != n:
            #this busy loop is where we expect to be interrupted to
            #run our callbacks.  Note that callbacks are only run on the
            #main thread
            if False and support.verbose:
                print("(%i)"%(len(l),),)
            for i in range(1000):
                a = i*i
            if context and not context.event.is_set():
                continue
            count += 1
            self.assertTrue(count < 10000,
                "timeout waiting for %i callbacks, got %i"%(n, len(l)))
        if False and support.verbose:
            print("(%i)"%(len(l),))

    @threading_helper.requires_working_threading()
    def test_pendingcalls_threaded(self):

        #do every callback on a separate thread
        n = 32 #total callbacks
        threads = []
        class foo(object):pass
        context = foo()
        context.l = []
        context.n = 2 #submits per thread
        context.nThreads = n // context.n
        context.nFinished = 0
        context.lock = threading.Lock()
        context.event = threading.Event()

        threads = [threading.Thread(target=self.pendingcalls_thread,
                                    args=(context,))
                   for i in range(context.nThreads)]
        with threading_helper.start_threads(threads):
            self.pendingcalls_wait(context.l, n, context)

    def pendingcalls_thread(self, context):
        try:
            self.pendingcalls_submit(context.l, context.n)
        finally:
            with context.lock:
                context.nFinished += 1
                nFinished = context.nFinished
                if False and support.verbose:
                    print("finished threads: ", nFinished)
            if nFinished == context.nThreads:
                context.event.set()

    def test_pendingcalls_non_threaded(self):
        #again, just using the main thread, likely they will all be dispatched at
        #once.  It is ok to ask for too many, because we loop until we find a slot.
        #the loop can be interrupted to dispatch.
        #there are only 32 dispatch slots, so we go for twice that!
        l = []
        n = 64
        self.pendingcalls_submit(l, n)
        self.pendingcalls_wait(l, n)

    def test_gen_get_code(self):
        def genf(): yield
        gen = genf()
        self.assertEqual(_testcapi.gen_get_code(gen), gen.gi_code)

    class PendingTask(types.SimpleNamespace):

        _add_pending = _testinternalcapi.pending_threadfunc

        def __init__(self, req, taskid=None, notify_done=None):
            self.id = taskid
            self.req = req
            self.notify_done = notify_done

            self.creator_tid = threading.get_ident()
            self.requester_tid = None
            self.runner_tid = None
            self.result = None

        def run(self):
            assert self.result is None
            self.runner_tid = threading.get_ident()
            self._run()
            if self.notify_done is not None:
                self.notify_done()

        def _run(self):
            self.result = self.req

        def run_in_pending_call(self, worker_tids):
            assert self._add_pending is _testinternalcapi.pending_threadfunc
            self.requester_tid = threading.get_ident()
            def callback():
                assert self.result is None
                # It can be tricky to control which thread handles
                # the eval breaker, so we take a naive approach to
                # make sure.
                if threading.get_ident() not in worker_tids:
                    self._add_pending(callback, ensure_added=True)
                    return
                self.run()
            self._add_pending(callback, ensure_added=True)

        def create_thread(self, worker_tids):
            return threading.Thread(
                target=self.run_in_pending_call,
                args=(worker_tids,),
            )

        def wait_for_result(self):
            while self.result is None:
                time.sleep(0.01)

    @threading_helper.requires_working_threading()
    def test_subthreads_can_handle_pending_calls(self):
        payload = 'Spam spam spam spam. Lovely spam! Wonderful spam!'

        task = self.PendingTask(payload)
        def do_the_work():
            tid = threading.get_ident()
            t = task.create_thread({tid})
            with threading_helper.start_threads([t]):
                task.wait_for_result()
        t = threading.Thread(target=do_the_work)
        with threading_helper.start_threads([t]):
            pass

        self.assertEqual(task.result, payload)

    @threading_helper.requires_working_threading()
    def test_many_subthreads_can_handle_pending_calls(self):
        main_tid = threading.get_ident()
        self.assertEqual(threading.main_thread().ident, main_tid)

        # We can't use queue.Queue since it isn't reentrant relative
        # to pending calls.
        _queue = deque()
        _active = deque()
        _done_lock = threading.Lock()
        def queue_put(task):
            _queue.append(task)
            _active.append(True)
        def queue_get():
            try:
                task = _queue.popleft()
            except IndexError:
                raise queue.Empty
            return task
        def queue_task_done():
            _active.pop()
            if not _active:
                try:
                    _done_lock.release()
                except RuntimeError:
                    assert not _done_lock.locked()
        def queue_empty():
            return not _queue
        def queue_join():
            _done_lock.acquire()
            _done_lock.release()

        tasks = []
        for i in range(20):
            task = self.PendingTask(
                req=f'request {i}',
                taskid=i,
                notify_done=queue_task_done,
            )
            tasks.append(task)
            queue_put(task)
        # This will be released once all the tasks have finished.
        _done_lock.acquire()

        def add_tasks(worker_tids):
            while True:
                if done:
                    return
                try:
                    task = queue_get()
                except queue.Empty:
                    break
                task.run_in_pending_call(worker_tids)

        done = False
        def run_tasks():
            while not queue_empty():
                if done:
                    return
                time.sleep(0.01)
            # Give the worker a chance to handle any remaining pending calls.
            while not done:
                time.sleep(0.01)

        # Start the workers and wait for them to finish.
        worker_threads = [threading.Thread(target=run_tasks)
                          for _ in range(3)]
        with threading_helper.start_threads(worker_threads):
            try:
                # Add a pending call for each task.
                worker_tids = [t.ident for t in worker_threads]
                threads = [threading.Thread(target=add_tasks, args=(worker_tids,))
                           for _ in range(3)]
                with threading_helper.start_threads(threads):
                    try:
                        pass
                    except BaseException:
                        done = True
                        raise  # re-raise
                # Wait for the pending calls to finish.
                queue_join()
                # Notify the workers that they can stop.
                done = True
            except BaseException:
                done = True
                raise  # re-raise
        runner_tids = [t.runner_tid for t in tasks]

        self.assertNotIn(main_tid, runner_tids)
        for task in tasks:
            with self.subTest(f'task {task.id}'):
                self.assertNotEqual(task.requester_tid, main_tid)
                self.assertNotEqual(task.requester_tid, task.runner_tid)
                self.assertNotIn(task.requester_tid, runner_tids)

    @requires_subinterpreters
    def test_isolated_subinterpreter(self):
        # We exercise the most important permutations.

        # This test relies on pending calls getting called
        # (eval breaker tripped) at each loop iteration
        # and at each call.

        maxtext = 250
        main_interpid = 0
        interpid = _interpreters.create()
        self.addCleanup(lambda: _interpreters.destroy(interpid))
        _interpreters.run_string(interpid, f"""if True:
            import json
            import os
            import threading
            import time
            import _testinternalcapi
            from test.support import threading_helper
            """)

        def create_pipe():
            r, w = os.pipe()
            self.addCleanup(lambda: os.close(r))
            self.addCleanup(lambda: os.close(w))
            return r, w

        with self.subTest('add in main, run in subinterpreter'):
            r_ready, w_ready = create_pipe()
            r_done, w_done= create_pipe()
            timeout = time.time() + 30  # seconds

            def do_work():
                _interpreters.run_string(interpid, f"""if True:
                    # Wait until this interp has handled the pending call.
                    waiting = False
                    done = False
                    def wait(os_read=os.read):
                        global done, waiting
                        waiting = True
                        os_read({r_done}, 1)
                        done = True
                    t = threading.Thread(target=wait)
                    with threading_helper.start_threads([t]):
                        while not waiting:
                            pass
                        os.write({w_ready}, b'\\0')
                        # Loop to trigger the eval breaker.
                        while not done:
                            time.sleep(0.01)
                            if time.time() > {timeout}:
                                raise Exception('timed out!')
                    """)
            t = threading.Thread(target=do_work)
            with threading_helper.start_threads([t]):
                os.read(r_ready, 1)
                # Add the pending call and wait for it to finish.
                actual = _testinternalcapi.pending_identify(interpid)
                # Signal the subinterpreter to stop.
                os.write(w_done, b'\0')

            self.assertEqual(actual, int(interpid))

        with self.subTest('add in main, run in subinterpreter sub-thread'):
            r_ready, w_ready = create_pipe()
            r_done, w_done= create_pipe()
            timeout = time.time() + 30  # seconds

            def do_work():
                _interpreters.run_string(interpid, f"""if True:
                    waiting = False
                    done = False
                    def subthread():
                        while not waiting:
                            pass
                        os.write({w_ready}, b'\\0')
                        # Loop to trigger the eval breaker.
                        while not done:
                            time.sleep(0.01)
                            if time.time() > {timeout}:
                                raise Exception('timed out!')
                    t = threading.Thread(target=subthread)
                    with threading_helper.start_threads([t]):
                        # Wait until this interp has handled the pending call.
                        waiting = True
                        os.read({r_done}, 1)
                        done = True
                    """)
            t = threading.Thread(target=do_work)
            with threading_helper.start_threads([t]):
                os.read(r_ready, 1)
                # Add the pending call and wait for it to finish.
                actual = _testinternalcapi.pending_identify(interpid)
                # Signal the subinterpreter to stop.
                os.write(w_done, b'\0')

            self.assertEqual(actual, int(interpid))

        with self.subTest('add in subinterpreter, run in main'):
            r_ready, w_ready = create_pipe()
            r_done, w_done= create_pipe()
            r_data, w_data= create_pipe()
            timeout = time.time() + 30  # seconds

            def add_job():
                os.read(r_ready, 1)
                _interpreters.run_string(interpid, f"""if True:
                    # Add the pending call and wait for it to finish.
                    actual = _testinternalcapi.pending_identify({main_interpid})
                    # Signal the subinterpreter to stop.
                    os.write({w_done}, b'\\0')
                    os.write({w_data}, actual.to_bytes(1, 'little'))
                    """)
            # Wait until this interp has handled the pending call.
            waiting = False
            done = False
            def wait(os_read=os.read):
                nonlocal done, waiting
                waiting = True
                os_read(r_done, 1)
                done = True
            t1 = threading.Thread(target=add_job)
            t2 = threading.Thread(target=wait)
            with threading_helper.start_threads([t1, t2]):
                while not waiting:
                    pass
                os.write(w_ready, b'\0')
                # Loop to trigger the eval breaker.
                while not done:
                    time.sleep(0.01)
                    if time.time() > timeout:
                        raise Exception('timed out!')
                text = os.read(r_data, 1)
            actual = int.from_bytes(text, 'little')

            self.assertEqual(actual, int(main_interpid))

        with self.subTest('add in subinterpreter, run in sub-thread'):
            r_ready, w_ready = create_pipe()
            r_done, w_done= create_pipe()
            r_data, w_data= create_pipe()
            timeout = time.time() + 30  # seconds

            def add_job():
                os.read(r_ready, 1)
                _interpreters.run_string(interpid, f"""if True:
                    # Add the pending call and wait for it to finish.
                    actual = _testinternalcapi.pending_identify({main_interpid})
                    # Signal the subinterpreter to stop.
                    os.write({w_done}, b'\\0')
                    os.write({w_data}, actual.to_bytes(1, 'little'))
                    """)
            # Wait until this interp has handled the pending call.
            waiting = False
            done = False
            def wait(os_read=os.read):
                nonlocal done, waiting
                waiting = True
                os_read(r_done, 1)
                done = True
            def subthread():
                while not waiting:
                    pass
                os.write(w_ready, b'\0')
                # Loop to trigger the eval breaker.
                while not done:
                    time.sleep(0.01)
                    if time.time() > timeout:
                        raise Exception('timed out!')
            t1 = threading.Thread(target=add_job)
            t2 = threading.Thread(target=wait)
            t3 = threading.Thread(target=subthread)
            with threading_helper.start_threads([t1, t2, t3]):
                pass
            text = os.read(r_data, 1)
            actual = int.from_bytes(text, 'little')

            self.assertEqual(actual, int(main_interpid))

        # XXX We can't use the rest until gh-105716 is fixed.
        return

        with self.subTest('add in subinterpreter, run in subinterpreter sub-thread'):
            r_ready, w_ready = create_pipe()
            r_done, w_done= create_pipe()
            r_data, w_data= create_pipe()
            timeout = time.time() + 30  # seconds

            def do_work():
                _interpreters.run_string(interpid, f"""if True:
                    waiting = False
                    done = False
                    def subthread():
                        while not waiting:
                            pass
                        os.write({w_ready}, b'\\0')
                        # Loop to trigger the eval breaker.
                        while not done:
                            time.sleep(0.01)
                            if time.time() > {timeout}:
                                raise Exception('timed out!')
                    t = threading.Thread(target=subthread)
                    with threading_helper.start_threads([t]):
                        # Wait until this interp has handled the pending call.
                        waiting = True
                        os.read({r_done}, 1)
                        done = True
                    """)
            t = threading.Thread(target=do_work)
            #with threading_helper.start_threads([t]):
            t.start()
            if True:
                os.read(r_ready, 1)
                _interpreters.run_string(interpid, f"""if True:
                    # Add the pending call and wait for it to finish.
                    actual = _testinternalcapi.pending_identify({interpid})
                    # Signal the subinterpreter to stop.
                    os.write({w_done}, b'\\0')
                    os.write({w_data}, actual.to_bytes(1, 'little'))
                    """)
            t.join()
            text = os.read(r_data, 1)
            actual = int.from_bytes(text, 'little')

            self.assertEqual(actual, int(interpid))


class SubinterpreterTest(unittest.TestCase):

    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_subinterps(self):
        import builtins
        r, w = os.pipe()
        code = """if 1:
            import sys, builtins, pickle
            with open({:d}, "wb") as f:
                pickle.dump(id(sys.modules), f)
                pickle.dump(id(builtins), f)
            """.format(w)
        with open(r, "rb") as f:
            ret = support.run_in_subinterp(code)
            self.assertEqual(ret, 0)
            self.assertNotEqual(pickle.load(f), id(sys.modules))
            self.assertNotEqual(pickle.load(f), id(builtins))

    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_subinterps_recent_language_features(self):
        r, w = os.pipe()
        code = """if 1:
            import pickle
            with open({:d}, "wb") as f:

                @(lambda x:x)  # Py 3.9
                def noop(x): return x

                a = (b := f'1{{2}}3') + noop('x')  # Py 3.8 (:=) / 3.6 (f'')

                async def foo(arg): return await arg  # Py 3.5

                pickle.dump(dict(a=a, b=b), f)
            """.format(w)

        with open(r, "rb") as f:
            ret = support.run_in_subinterp(code)
            self.assertEqual(ret, 0)
            self.assertEqual(pickle.load(f), {'a': '123x', 'b': '123'})

    def test_py_config_isoloated_per_interpreter(self):
        # A config change in one interpreter must not leak to out to others.
        #
        # This test could verify ANY config value, it just happens to have been
        # written around the time of int_max_str_digits. Refactoring is okay.
        code = """if 1:
        import sys, _testinternalcapi

        # Any config value would do, this happens to be the one being
        # double checked at the time this test was written.
        config = _testinternalcapi.get_config()
        config['int_max_str_digits'] = 55555
        _testinternalcapi.set_config(config)
        sub_value = _testinternalcapi.get_config()['int_max_str_digits']
        assert sub_value == 55555, sub_value
        """
        before_config = _testinternalcapi.get_config()
        assert before_config['int_max_str_digits'] != 55555
        self.assertEqual(support.run_in_subinterp(code), 0,
                         'subinterp code failure, check stderr.')
        after_config = _testinternalcapi.get_config()
        self.assertIsNot(
                before_config, after_config,
                "Expected get_config() to return a new dict on each call")
        self.assertEqual(before_config, after_config,
                         "CAUTION: Tests executed after this may be "
                         "running under an altered config.")
        # try:...finally: calling set_config(before_config) not done
        # as that results in sys.argv, sys.path, and sys.warnoptions
        # "being modified by test_capi" per test.regrtest.  So if this
        # test fails, assume that the environment in this process may
        # be altered and suspect.

    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_configured_settings(self):
        """
        The config with which an interpreter is created corresponds
        1-to-1 with the new interpreter's settings.  This test verifies
        that they match.
        """

        OBMALLOC = 1<<5
        EXTENSIONS = 1<<8
        THREADS = 1<<10
        DAEMON_THREADS = 1<<11
        FORK = 1<<15
        EXEC = 1<<16
        ALL_FLAGS = (OBMALLOC | FORK | EXEC | THREADS | DAEMON_THREADS
                     | EXTENSIONS);

        features = [
            'obmalloc',
            'fork',
            'exec',
            'threads',
            'daemon_threads',
            'extensions',
            'own_gil',
        ]
        kwlist = [f'allow_{n}' for n in features]
        kwlist[0] = 'use_main_obmalloc'
        kwlist[-2] = 'check_multi_interp_extensions'
        kwlist[-1] = 'own_gil'

        # expected to work
        for config, expected in {
            (True, True, True, True, True, True, True):
                (ALL_FLAGS, True),
            (True, False, False, False, False, False, False):
                (OBMALLOC, False),
            (False, False, False, True, False, True, False):
                (THREADS | EXTENSIONS, False),
        }.items():
            kwargs = dict(zip(kwlist, config))
            exp_flags, exp_gil = expected
            expected = {
                'feature_flags': exp_flags,
                'own_gil': exp_gil,
            }
            with self.subTest(config):
                r, w = os.pipe()
                script = textwrap.dedent(f'''
                    import _testinternalcapi, json, os
                    settings = _testinternalcapi.get_interp_settings()
                    with os.fdopen({w}, "w") as stdin:
                        json.dump(settings, stdin)
                    ''')
                with os.fdopen(r) as stdout:
                    ret = support.run_in_subinterp_with_config(script, **kwargs)
                    self.assertEqual(ret, 0)
                    out = stdout.read()
                settings = json.loads(out)

                self.assertEqual(settings, expected)

        # expected to fail
        for config in [
            (False, False, False, False, False, False, False),
        ]:
            kwargs = dict(zip(kwlist, config))
            with self.subTest(config):
                script = textwrap.dedent(f'''
                    import _testinternalcapi
                    _testinternalcapi.get_interp_settings()
                    raise NotImplementedError('unreachable')
                    ''')
                with self.assertRaises(RuntimeError):
                    support.run_in_subinterp_with_config(script, **kwargs)

    @unittest.skipIf(_testsinglephase is None, "test requires _testsinglephase module")
    @unittest.skipUnless(hasattr(os, "pipe"), "requires os.pipe()")
    def test_overridden_setting_extensions_subinterp_check(self):
        """
        PyInterpreterConfig.check_multi_interp_extensions can be overridden
        with PyInterpreterState.override_multi_interp_extensions_check.
        This verifies that the override works but does not modify
        the underlying setting.
        """

        OBMALLOC = 1<<5
        EXTENSIONS = 1<<8
        THREADS = 1<<10
        DAEMON_THREADS = 1<<11
        FORK = 1<<15
        EXEC = 1<<16
        BASE_FLAGS = OBMALLOC | FORK | EXEC | THREADS | DAEMON_THREADS
        base_kwargs = {
            'use_main_obmalloc': True,
            'allow_fork': True,
            'allow_exec': True,
            'allow_threads': True,
            'allow_daemon_threads': True,
            'own_gil': False,
        }

        def check(enabled, override):
            kwargs = dict(
                base_kwargs,
                check_multi_interp_extensions=enabled,
            )
            flags = BASE_FLAGS | EXTENSIONS if enabled else BASE_FLAGS
            settings = {
                'feature_flags': flags,
                'own_gil': False,
            }

            expected = {
                'requested': override,
                'override__initial': 0,
                'override_after': override,
                'override_restored': 0,
                # The override should not affect the config or settings.
                'settings__initial': settings,
                'settings_after': settings,
                'settings_restored': settings,
                # These are the most likely values to be wrong.
                'allowed__initial': not enabled,
                'allowed_after': not ((override > 0) if override else enabled),
                'allowed_restored': not enabled,
            }

            r, w = os.pipe()
            script = textwrap.dedent(f'''
                from test.test_capi.check_config import run_singlephase_check
                run_singlephase_check({override}, {w})
                ''')
            with os.fdopen(r) as stdout:
                ret = support.run_in_subinterp_with_config(script, **kwargs)
                self.assertEqual(ret, 0)
                out = stdout.read()
            results = json.loads(out)

            self.assertEqual(results, expected)

        self.maxDiff = None

        # setting: check disabled
        with self.subTest('config: check disabled; override: disabled'):
            check(False, -1)
        with self.subTest('config: check disabled; override: use config'):
            check(False, 0)
        with self.subTest('config: check disabled; override: enabled'):
            check(False, 1)

        # setting: check enabled
        with self.subTest('config: check enabled; override: disabled'):
            check(True, -1)
        with self.subTest('config: check enabled; override: use config'):
            check(True, 0)
        with self.subTest('config: check enabled; override: enabled'):
            check(True, 1)

    def test_mutate_exception(self):
        """
        Exceptions saved in global module state get shared between
        individual module instances. This test checks whether or not
        a change in one interpreter's module gets reflected into the
        other ones.
        """
        import binascii

        support.run_in_subinterp("import binascii; binascii.Error.foobar = 'foobar'")

        self.assertFalse(hasattr(binascii.Error, "foobar"))

    @unittest.skipIf(_testmultiphase is None, "test requires _testmultiphase module")
    def test_module_state_shared_in_global(self):
        """
        bpo-44050: Extension module state should be shared between interpreters
        when it doesn't support sub-interpreters.
        """
        r, w = os.pipe()
        self.addCleanup(os.close, r)
        self.addCleanup(os.close, w)

        script = textwrap.dedent(f"""
            import importlib.machinery
            import importlib.util
            import os

            fullname = '_test_module_state_shared'
            origin = importlib.util.find_spec('_testmultiphase').origin
            loader = importlib.machinery.ExtensionFileLoader(fullname, origin)
            spec = importlib.util.spec_from_loader(fullname, loader)
            module = importlib.util.module_from_spec(spec)
            attr_id = str(id(module.Error)).encode()

            os.write({w}, attr_id)
            """)
        exec(script)
        main_attr_id = os.read(r, 100)

        ret = support.run_in_subinterp(script)
        self.assertEqual(ret, 0)
        subinterp_attr_id = os.read(r, 100)
        self.assertEqual(main_attr_id, subinterp_attr_id)


@requires_subinterpreters
class InterpreterIDTests(unittest.TestCase):

    InterpreterID = _testcapi.get_interpreterid_type()

    def new_interpreter(self):
        def ensure_destroyed(interpid):
            try:
                _interpreters.destroy(interpid)
            except _interpreters.InterpreterNotFoundError:
                pass
        id = _interpreters.create()
        self.addCleanup(lambda: ensure_destroyed(id))
        return id

    def test_with_int(self):
        id = self.InterpreterID(10, force=True)

        self.assertEqual(int(id), 10)

    def test_coerce_id(self):
        class Int(str):
            def __index__(self):
                return 10

        id = self.InterpreterID(Int(), force=True)
        self.assertEqual(int(id), 10)

    def test_bad_id(self):
        for badid in [
            object(),
            10.0,
            '10',
            b'10',
        ]:
            with self.subTest(badid):
                with self.assertRaises(TypeError):
                    self.InterpreterID(badid)

        badid = -1
        with self.subTest(badid):
            with self.assertRaises(ValueError):
                self.InterpreterID(badid)

        badid = 2**64
        with self.subTest(badid):
            with self.assertRaises(OverflowError):
                self.InterpreterID(badid)

    def test_exists(self):
        id = self.new_interpreter()
        with self.assertRaises(_interpreters.InterpreterNotFoundError):
            self.InterpreterID(int(id) + 1)  # unforced

    def test_does_not_exist(self):
        id = self.new_interpreter()
        with self.assertRaises(_interpreters.InterpreterNotFoundError):
            self.InterpreterID(int(id) + 1)  # unforced

    def test_destroyed(self):
        id = _interpreters.create()
        _interpreters.destroy(id)
        with self.assertRaises(_interpreters.InterpreterNotFoundError):
            self.InterpreterID(id)  # unforced

    def test_str(self):
        id = self.InterpreterID(10, force=True)
        self.assertEqual(str(id), '10')

    def test_repr(self):
        id = self.InterpreterID(10, force=True)
        self.assertEqual(repr(id), 'InterpreterID(10)')

    def test_equality(self):
        id1 = self.new_interpreter()
        id2 = self.InterpreterID(id1)
        id3 = self.InterpreterID(
                self.new_interpreter())

        self.assertTrue(id2 == id2)  # identity
        self.assertTrue(id2 == id1)  # int-equivalent
        self.assertTrue(id1 == id2)  # reversed
        self.assertTrue(id2 == int(id2))
        self.assertTrue(id2 == float(int(id2)))
        self.assertTrue(float(int(id2)) == id2)
        self.assertFalse(id2 == float(int(id2)) + 0.1)
        self.assertFalse(id2 == str(int(id2)))
        self.assertFalse(id2 == 2**1000)
        self.assertFalse(id2 == float('inf'))
        self.assertFalse(id2 == 'spam')
        self.assertFalse(id2 == id3)

        self.assertFalse(id2 != id2)
        self.assertFalse(id2 != id1)
        self.assertFalse(id1 != id2)
        self.assertTrue(id2 != id3)

    def test_linked_lifecycle(self):
        id1 = _interpreters.create()
        _testcapi.unlink_interpreter_refcount(id1)
        self.assertEqual(
            _testinternalcapi.get_interpreter_refcount(id1),
            0)

        id2 = self.InterpreterID(id1)
        self.assertEqual(
            _testinternalcapi.get_interpreter_refcount(id1),
            1)

        # The interpreter isn't linked to ID objects, so it isn't destroyed.
        del id2
        self.assertEqual(
            _testinternalcapi.get_interpreter_refcount(id1),
            0)

        _testcapi.link_interpreter_refcount(id1)
        self.assertEqual(
            _testinternalcapi.get_interpreter_refcount(id1),
            0)

        id3 = self.InterpreterID(id1)
        self.assertEqual(
            _testinternalcapi.get_interpreter_refcount(id1),
            1)

        # The interpreter is linked now so is destroyed.
        del id3
        with self.assertRaises(_interpreters.InterpreterNotFoundError):
            _testinternalcapi.get_interpreter_refcount(id1)


class BuiltinStaticTypesTests(unittest.TestCase):

    TYPES = [
        object,
        type,
        int,
        str,
        dict,
        type(None),
        bool,
        BaseException,
        Exception,
        Warning,
        DeprecationWarning,  # Warning subclass
    ]

    def test_tp_bases_is_set(self):
        # PyTypeObject.tp_bases is documented as public API.
        # See https://github.com/python/cpython/issues/105020.
        for typeobj in self.TYPES:
            with self.subTest(typeobj):
                bases = _testcapi.type_get_tp_bases(typeobj)
                self.assertIsNot(bases, None)

    def test_tp_mro_is_set(self):
        # PyTypeObject.tp_bases is documented as public API.
        # See https://github.com/python/cpython/issues/105020.
        for typeobj in self.TYPES:
            with self.subTest(typeobj):
                mro = _testcapi.type_get_tp_mro(typeobj)
                self.assertIsNot(mro, None)


class TestStaticTypes(unittest.TestCase):

    _has_run = False

    @classmethod
    def setUpClass(cls):
        # The tests here don't play nice with our approach to refleak
        # detection, so we bail out in that case.
        if cls._has_run:
            raise unittest.SkipTest('these tests do not support re-running')
        cls._has_run = True

    @contextlib.contextmanager
    def basic_static_type(self, *args):
        cls = _testcapi.get_basic_static_type(*args)
        yield cls

    def test_pytype_ready_always_sets_tp_type(self):
        # The point of this test is to prevent something like
        # https://github.com/python/cpython/issues/104614
        # from happening again.

        # First check when tp_base/tp_bases is *not* set before PyType_Ready().
        with self.basic_static_type() as cls:
            self.assertIs(cls.__base__, object);
            self.assertEqual(cls.__bases__, (object,));
            self.assertIs(type(cls), type(object));

        # Then check when we *do* set tp_base/tp_bases first.
        with self.basic_static_type(object) as cls:
            self.assertIs(cls.__base__, object);
            self.assertEqual(cls.__bases__, (object,));
            self.assertIs(type(cls), type(object));


class TestThreadState(unittest.TestCase):

    @threading_helper.reap_threads
    @threading_helper.requires_working_threading()
    def test_thread_state(self):
        # some extra thread-state tests driven via _testcapi
        def target():
            idents = []

            def callback():
                idents.append(threading.get_ident())

            _testcapi._test_thread_state(callback)
            a = b = callback
            time.sleep(1)
            # Check our main thread is in the list exactly 3 times.
            self.assertEqual(idents.count(threading.get_ident()), 3,
                             "Couldn't find main thread correctly in the list")

        target()
        t = threading.Thread(target=target)
        t.start()
        t.join()

    @threading_helper.reap_threads
    @threading_helper.requires_working_threading()
    def test_gilstate_ensure_no_deadlock(self):
        # See https://github.com/python/cpython/issues/96071
        code = textwrap.dedent("""
            import _testcapi

            def callback():
                print('callback called')

            _testcapi._test_thread_state(callback)
            """)
        ret = assert_python_ok('-X', 'tracemalloc', '-c', code)
        self.assertIn(b'callback called', ret.out)

    def test_gilstate_matches_current(self):
        _testcapi.test_current_tstate_matches()


class Test_testcapi(unittest.TestCase):
    locals().update((name, getattr(_testcapi, name))
                    for name in dir(_testcapi)
                    if name.startswith('test_'))

    # Suppress warning from PyUnicode_FromUnicode().
    @warnings_helper.ignore_warnings(category=DeprecationWarning)
    def test_widechar(self):
        _testcapi.test_widechar()

    def test_version_api_data(self):
        self.assertEqual(_testcapi.Py_Version, sys.hexversion)


class Test_testinternalcapi(unittest.TestCase):
    locals().update((name, getattr(_testinternalcapi, name))
                    for name in dir(_testinternalcapi)
                    if name.startswith('test_')
                    and not name.startswith('test_lock_'))


@threading_helper.requires_working_threading()
class Test_PyLock(unittest.TestCase):
    locals().update((name, getattr(_testinternalcapi, name))
                    for name in dir(_testinternalcapi)
                    if name.startswith('test_lock_'))


@unittest.skipIf(_testmultiphase is None, "test requires _testmultiphase module")
class Test_ModuleStateAccess(unittest.TestCase):
    """Test access to module start (PEP 573)"""

    # The C part of the tests lives in _testmultiphase, in a module called
    # _testmultiphase_meth_state_access.
    # This module has multi-phase initialization, unlike _testcapi.

    def setUp(self):
        fullname = '_testmultiphase_meth_state_access'  # XXX
        origin = importlib.util.find_spec('_testmultiphase').origin
        loader = importlib.machinery.ExtensionFileLoader(fullname, origin)
        spec = importlib.util.spec_from_loader(fullname, loader)
        module = importlib.util.module_from_spec(spec)
        loader.exec_module(module)
        self.module = module

    def test_subclass_get_module(self):
        """PyType_GetModule for defining_class"""
        class StateAccessType_Subclass(self.module.StateAccessType):
            pass

        instance = StateAccessType_Subclass()
        self.assertIs(instance.get_defining_module(), self.module)

    def test_subclass_get_module_with_super(self):
        class StateAccessType_Subclass(self.module.StateAccessType):
            def get_defining_module(self):
                return super().get_defining_module()

        instance = StateAccessType_Subclass()
        self.assertIs(instance.get_defining_module(), self.module)

    def test_state_access(self):
        """Checks methods defined with and without argument clinic

        This tests a no-arg method (get_count) and a method with
        both a positional and keyword argument.
        """

        a = self.module.StateAccessType()
        b = self.module.StateAccessType()

        methods = {
            'clinic': a.increment_count_clinic,
            'noclinic': a.increment_count_noclinic,
        }

        for name, increment_count in methods.items():
            with self.subTest(name):
                self.assertEqual(a.get_count(), b.get_count())
                self.assertEqual(a.get_count(), 0)

                increment_count()
                self.assertEqual(a.get_count(), b.get_count())
                self.assertEqual(a.get_count(), 1)

                increment_count(3)
                self.assertEqual(a.get_count(), b.get_count())
                self.assertEqual(a.get_count(), 4)

                increment_count(-2, twice=True)
                self.assertEqual(a.get_count(), b.get_count())
                self.assertEqual(a.get_count(), 0)

                with self.assertRaises(TypeError):
                    increment_count(thrice=3)

                with self.assertRaises(TypeError):
                    increment_count(1, 2, 3)

    def test_get_module_bad_def(self):
        # PyType_GetModuleByDef fails gracefully if it doesn't
        # find what it's looking for.
        # see bpo-46433
        instance = self.module.StateAccessType()
        with self.assertRaises(TypeError):
            instance.getmodulebydef_bad_def()

    def test_get_module_static_in_mro(self):
        # Here, the class PyType_GetModuleByDef is looking for
        # appears in the MRO after a static type (Exception).
        # see bpo-46433
        class Subclass(BaseException, self.module.StateAccessType):
            pass
        self.assertIs(Subclass().get_defining_module(), self.module)


class TestInternalFrameApi(unittest.TestCase):

    @staticmethod
    def func():
        return sys._getframe()

    def test_code(self):
        frame = self.func()
        code = _testinternalcapi.iframe_getcode(frame)
        self.assertIs(code, self.func.__code__)

    def test_lasti(self):
        frame = self.func()
        lasti = _testinternalcapi.iframe_getlasti(frame)
        self.assertGreater(lasti, 0)
        self.assertLess(lasti, len(self.func.__code__.co_code))

    def test_line(self):
        frame = self.func()
        line = _testinternalcapi.iframe_getline(frame)
        firstline = self.func.__code__.co_firstlineno
        self.assertEqual(line, firstline + 2)


SUFFICIENT_TO_DEOPT_AND_SPECIALIZE = 100

class Test_Pep523API(unittest.TestCase):

    def do_test(self, func, names):
        actual_calls = []
        start = SUFFICIENT_TO_DEOPT_AND_SPECIALIZE
        count = start + SUFFICIENT_TO_DEOPT_AND_SPECIALIZE
        try:
            for i in range(count):
                if i == start:
                    _testinternalcapi.set_eval_frame_record(actual_calls)
                func()
        finally:
            _testinternalcapi.set_eval_frame_default()
        expected_calls = names * SUFFICIENT_TO_DEOPT_AND_SPECIALIZE
        self.assertEqual(len(expected_calls), len(actual_calls))
        for expected, actual in zip(expected_calls, actual_calls, strict=True):
            self.assertEqual(expected, actual)

    def test_inlined_binary_subscr(self):
        class C:
            def __getitem__(self, other):
                return None
        def func():
            C()[42]
        names = ["func", "__getitem__"]
        self.do_test(func, names)

    def test_inlined_call(self):
        def inner(x=42):
            pass
        def func():
            inner()
            inner(42)
        names = ["func", "inner", "inner"]
        self.do_test(func, names)

    def test_inlined_call_function_ex(self):
        def inner(x):
            pass
        def func():
            inner(*[42])
        names = ["func", "inner"]
        self.do_test(func, names)

    def test_inlined_for_iter(self):
        def gen():
            yield 42
        def func():
            for _ in gen():
                pass
        names = ["func", "gen", "gen", "gen"]
        self.do_test(func, names)

    def test_inlined_load_attr(self):
        class C:
            @property
            def a(self):
                return 42
        class D:
            def __getattribute__(self, name):
                return 42
        def func():
            C().a
            D().a
        names = ["func", "a", "__getattribute__"]
        self.do_test(func, names)

    def test_inlined_send(self):
        def inner():
            yield 42
        def outer():
            yield from inner()
        def func():
            list(outer())
        names = ["func", "outer", "outer", "inner", "inner", "outer", "inner"]
        self.do_test(func, names)


@unittest.skipUnless(support.Py_GIL_DISABLED, 'need Py_GIL_DISABLED')
class TestPyThreadId(unittest.TestCase):
    def test_py_thread_id(self):
        # gh-112535: Test _Py_ThreadId(): make sure that thread identifiers
        # in a few threads are unique
        py_thread_id = _testinternalcapi.py_thread_id
        short_sleep = 0.010

        class GetThreadId(threading.Thread):
            def __init__(self):
                super().__init__()
                self.get_lock = threading.Lock()
                self.get_lock.acquire()
                self.started_lock = threading.Event()
                self.py_tid = None

            def run(self):
                self.started_lock.set()
                self.get_lock.acquire()
                self.py_tid = py_thread_id()
                time.sleep(short_sleep)
                self.py_tid2 = py_thread_id()

        nthread = 5
        threads = [GetThreadId() for _ in range(nthread)]

        # first make run sure that all threads are running
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.started_lock.wait()

        # call _Py_ThreadId() in the main thread
        py_thread_ids = [py_thread_id()]

        # now call _Py_ThreadId() in each thread
        for thread in threads:
            thread.get_lock.release()

        # call _Py_ThreadId() in each thread and wait until threads complete
        for thread in threads:
            thread.join()
            py_thread_ids.append(thread.py_tid)
            # _PyThread_Id() should not change for a given thread.
            # For example, it should remain the same after a short sleep.
            self.assertEqual(thread.py_tid2, thread.py_tid)

        # make sure that all _Py_ThreadId() are unique
        for tid in py_thread_ids:
            self.assertIsInstance(tid, int)
            self.assertGreater(tid, 0)
        self.assertEqual(len(set(py_thread_ids)), len(py_thread_ids),
                         py_thread_ids)


if __name__ == "__main__":
    unittest.main()
