import re
import textwrap
import unittest


from test import support
from test.support import import_helper, requires_subprocess
from test.support.script_helper import assert_python_failure, assert_python_ok


# Skip this test if the _testcapi and _testinternalcapi extensions are not
# available.
_testcapi = import_helper.import_module('_testcapi')
_testinternalcapi = import_helper.import_module('_testinternalcapi')

@requires_subprocess()
class PyMemDebugTests(unittest.TestCase):
    PYTHONMALLOC = 'debug'
    # '0x04c06e0' or '04C06E0'
    PTR_REGEX = r'(?:0x)?[0-9a-fA-F]+'

    def check(self, code):
        with support.SuppressCrashReport():
            out = assert_python_failure(
                '-c', code,
                PYTHONMALLOC=self.PYTHONMALLOC,
                # FreeBSD: instruct jemalloc to not fill freed() memory
                # with junk byte 0x5a, see JEMALLOC(3)
                MALLOC_CONF="junk:false",
            )
        stderr = out.err
        return stderr.decode('ascii', 'replace')

    def test_buffer_overflow(self):
        out = self.check('import _testcapi; _testcapi.pymem_buffer_overflow()')
        regex = (r"Debug memory block at address p={ptr}: API 'm'\n"
                 r"    16 bytes originally requested\n"
                 r"    The [0-9] pad bytes at p-[0-9] are FORBIDDENBYTE, as expected.\n"
                 r"    The [0-9] pad bytes at tail={ptr} are not all FORBIDDENBYTE \(0x[0-9a-f]{{2}}\):\n"
                 r"        at tail\+0: 0x78 \*\*\* OUCH\n"
                 r"        at tail\+1: 0xfd\n"
                 r"        at tail\+2: 0xfd\n"
                 r"        .*\n"
                 r"(    The block was made by call #[0-9]+ to debug malloc/realloc.\n)?"
                 r"    Data at p: cd cd cd .*\n"
                 r"\n"
                 r"Enable tracemalloc to get the memory block allocation traceback\n"
                 r"\n"
                 r"Fatal Python error: _PyMem_DebugRawFree: bad trailing pad byte")
        regex = regex.format(ptr=self.PTR_REGEX)
        regex = re.compile(regex, flags=re.DOTALL)
        self.assertRegex(out, regex)

    def test_api_misuse(self):
        out = self.check('import _testcapi; _testcapi.pymem_api_misuse()')
        regex = (r"Debug memory block at address p={ptr}: API 'm'\n"
                 r"    16 bytes originally requested\n"
                 r"    The [0-9] pad bytes at p-[0-9] are FORBIDDENBYTE, as expected.\n"
                 r"    The [0-9] pad bytes at tail={ptr} are FORBIDDENBYTE, as expected.\n"
                 r"(    The block was made by call #[0-9]+ to debug malloc/realloc.\n)?"
                 r"    Data at p: cd cd cd .*\n"
                 r"\n"
                 r"Enable tracemalloc to get the memory block allocation traceback\n"
                 r"\n"
                 r"Fatal Python error: _PyMem_DebugRawFree: bad ID: Allocated using API 'm', verified using API 'r'\n")
        regex = regex.format(ptr=self.PTR_REGEX)
        self.assertRegex(out, regex)

    def check_malloc_without_gil(self, code):
        out = self.check(code)
        if not support.Py_GIL_DISABLED:
            expected = ('Fatal Python error: _PyMem_DebugMalloc: '
                        'Python memory allocator called without holding the GIL')
        else:
            expected = ('Fatal Python error: _PyMem_DebugMalloc: '
                        'Python memory allocator called without an active thread state. '
                        'Are you trying to call it inside of a Py_BEGIN_ALLOW_THREADS block?')
        self.assertIn(expected, out)

    def test_pymem_malloc_without_gil(self):
        # Debug hooks must raise an error if PyMem_Malloc() is called
        # without holding the GIL
        code = 'import _testcapi; _testcapi.pymem_malloc_without_gil()'
        self.check_malloc_without_gil(code)

    def test_pyobject_malloc_without_gil(self):
        # Debug hooks must raise an error if PyObject_Malloc() is called
        # without holding the GIL
        code = 'import _testcapi; _testcapi.pyobject_malloc_without_gil()'
        self.check_malloc_without_gil(code)

    def check_pyobject_is_freed(self, func_name):
        code = textwrap.dedent(f'''
            import gc, os, sys, _testinternalcapi
            # Disable the GC to avoid crash on GC collection
            gc.disable()
            _testinternalcapi.{func_name}()
            # Exit immediately to avoid a crash while deallocating
            # the invalid object
            os._exit(0)
        ''')
        assert_python_ok(
            '-c', code,
            PYTHONMALLOC=self.PYTHONMALLOC,
            MALLOC_CONF="junk:false",
        )

    def test_pyobject_null_is_freed(self):
        self.check_pyobject_is_freed('check_pyobject_null_is_freed')

    def test_pyobject_uninitialized_is_freed(self):
        self.check_pyobject_is_freed('check_pyobject_uninitialized_is_freed')

    def test_pyobject_forbidden_bytes_is_freed(self):
        self.check_pyobject_is_freed('check_pyobject_forbidden_bytes_is_freed')

    def test_pyobject_freed_is_freed(self):
        self.check_pyobject_is_freed('check_pyobject_freed_is_freed')

    # Python built with Py_TRACE_REFS fail with a fatal error in
    # _PyRefchain_Trace() on memory allocation error.
    @unittest.skipIf(support.Py_TRACE_REFS, 'cannot test Py_TRACE_REFS build')
    def test_set_nomemory(self):
        code = """if 1:
            import _testcapi

            class C(): pass

            # The first loop tests both functions and that remove_mem_hooks()
            # can be called twice in a row. The second loop checks a call to
            # set_nomemory() after a call to remove_mem_hooks(). The third
            # loop checks the start and stop arguments of set_nomemory().
            for outer_cnt in range(1, 4):
                start = 10 * outer_cnt
                for j in range(100):
                    if j == 0:
                        if outer_cnt != 3:
                            _testcapi.set_nomemory(start)
                        else:
                            _testcapi.set_nomemory(start, start + 1)
                    try:
                        C()
                    except MemoryError as e:
                        if outer_cnt != 3:
                            _testcapi.remove_mem_hooks()
                        print('MemoryError', outer_cnt, j)
                        _testcapi.remove_mem_hooks()
                        break
        """
        rc, out, err = assert_python_ok('-c', code)
        lines = out.splitlines()
        for i, line in enumerate(lines, 1):
            self.assertIn(b'MemoryError', out)
            *_, count = line.split(b' ')
            count = int(count)
            self.assertLessEqual(count, i*10)
            self.assertGreaterEqual(count, i*10-4)


# free-threading requires mimalloc (not malloc)
@support.requires_gil_enabled()
class PyMemMallocDebugTests(PyMemDebugTests):
    PYTHONMALLOC = 'malloc_debug'


@unittest.skipUnless(support.with_pymalloc(), 'need pymalloc')
class PyMemPymallocDebugTests(PyMemDebugTests):
    PYTHONMALLOC = 'pymalloc_debug'


@unittest.skipUnless(support.with_mimalloc(), 'need mimaloc')
class PyMemMimallocDebugTests(PyMemDebugTests):
    PYTHONMALLOC = 'mimalloc_debug'


@unittest.skipUnless(support.Py_DEBUG, 'need Py_DEBUG')
class PyMemDefaultTests(PyMemDebugTests):
    # test default allocator of Python compiled in debug mode
    PYTHONMALLOC = ''


if __name__ == "__main__":
    unittest.main()
