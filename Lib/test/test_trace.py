import os
from pickle import dump
import sys
from test.support import captured_stdout, requires_resource
from test.support.os_helper import (TESTFN, rmtree, unlink)
from test.support.script_helper import assert_python_ok, assert_python_failure
import textwrap
import unittest
from types import FunctionType

import trace
from trace import Trace

from test.tracedmodules import testmod

##
## See also test_sys_settrace.py, which contains tests that cover
## tracing of many more code blocks.
##

#------------------------------- Utilities -----------------------------------#

def fix_ext_py(filename):
    """Given a .pyc filename converts it to the appropriate .py"""
    if filename.endswith('.pyc'):
        filename = filename[:-1]
    return filename

def my_file_and_modname():
    """The .py file and module name of this file (__file__)"""
    modname = os.path.splitext(os.path.basename(__file__))[0]
    return fix_ext_py(__file__), modname

def get_firstlineno(func):
    return func.__code__.co_firstlineno

#-------------------- Target functions for tracing ---------------------------#
#
# The relative line numbers of lines in these functions matter for verifying
# tracing. Please modify the appropriate tests if you change one of the
# functions. Absolute line numbers don't matter.
#

def traced_func_linear(x, y):
    a = x
    b = y
    c = a + b
    return c

def traced_func_loop(x, y):
    c = x
    for i in range(5):
        c += y
    return c

def traced_func_importing(x, y):
    return x + y + testmod.func(1)

def traced_func_simple_caller(x):
    c = traced_func_linear(x, x)
    return c + x

def traced_func_importing_caller(x):
    k = traced_func_simple_caller(x)
    k += traced_func_importing(k, x)
    return k

def traced_func_generator(num):
    c = 5       # executed once
    for i in range(num):
        yield i + c

def traced_func_calling_generator():
    k = 0
    for i in traced_func_generator(10):
        k += i

def traced_doubler(num):
    return num * 2

def traced_capturer(*args, **kwargs):
    return args, kwargs

def traced_caller_list_comprehension():
    k = 10
    mylist = [traced_doubler(i) for i in range(k)]
    return mylist

def traced_decorated_function():
    def decorator1(f):
        return f
    def decorator_fabric():
        def decorator2(f):
            return f
        return decorator2
    @decorator1
    @decorator_fabric()
    def func():
        pass
    func()


class TracedClass(object):
    def __init__(self, x):
        self.a = x

    def inst_method_linear(self, y):
        return self.a + y

    def inst_method_calling(self, x):
        c = self.inst_method_linear(x)
        return c + traced_func_linear(x, c)

    @classmethod
    def class_method_linear(cls, y):
        return y * 2

    @staticmethod
    def static_method_linear(y):
        return y * 2


#------------------------------ Test cases -----------------------------------#


class TestLineCounts(unittest.TestCase):
    """White-box testing of line-counting, via runfunc"""
    def setUp(self):
        self.addCleanup(sys.settrace, sys.gettrace())
        self.tracer = Trace(count=1, trace=0, countfuncs=0, countcallers=0)
        self.my_py_filename = fix_ext_py(__file__)

    def test_traced_func_linear(self):
        result = self.tracer.runfunc(traced_func_linear, 2, 5)
        self.assertEqual(result, 7)

        # all lines are executed once
        expected = {}
        firstlineno = get_firstlineno(traced_func_linear)
        for i in range(1, 5):
            expected[(self.my_py_filename, firstlineno +  i)] = 1

        self.assertEqual(self.tracer.results().counts, expected)

    def test_traced_func_loop(self):
        self.tracer.runfunc(traced_func_loop, 2, 3)

        firstlineno = get_firstlineno(traced_func_loop)
        expected = {
            (self.my_py_filename, firstlineno + 1): 1,
            (self.my_py_filename, firstlineno + 2): 6,
            (self.my_py_filename, firstlineno + 3): 5,
            (self.my_py_filename, firstlineno + 4): 1,
        }
        self.assertEqual(self.tracer.results().counts, expected)

    def test_traced_func_importing(self):
        self.tracer.runfunc(traced_func_importing, 2, 5)

        firstlineno = get_firstlineno(traced_func_importing)
        expected = {
            (self.my_py_filename, firstlineno + 1): 1,
            (fix_ext_py(testmod.__file__), 2): 1,
            (fix_ext_py(testmod.__file__), 3): 1,
        }

        self.assertEqual(self.tracer.results().counts, expected)

    def test_trace_func_generator(self):
        self.tracer.runfunc(traced_func_calling_generator)

        firstlineno_calling = get_firstlineno(traced_func_calling_generator)
        firstlineno_gen = get_firstlineno(traced_func_generator)
        expected = {
            (self.my_py_filename, firstlineno_calling + 1): 1,
            (self.my_py_filename, firstlineno_calling + 2): 11,
            (self.my_py_filename, firstlineno_calling + 3): 10,
            (self.my_py_filename, firstlineno_gen + 1): 1,
            (self.my_py_filename, firstlineno_gen + 2): 11,
            (self.my_py_filename, firstlineno_gen + 3): 10,
        }
        self.assertEqual(self.tracer.results().counts, expected)

    def test_trace_list_comprehension(self):
        self.tracer.runfunc(traced_caller_list_comprehension)

        firstlineno_calling = get_firstlineno(traced_caller_list_comprehension)
        firstlineno_called = get_firstlineno(traced_doubler)
        expected = {
            (self.my_py_filename, firstlineno_calling + 1): 1,
            (self.my_py_filename, firstlineno_calling + 2): 11,
            (self.my_py_filename, firstlineno_calling + 3): 1,
            (self.my_py_filename, firstlineno_called + 1): 10,
        }
        self.assertEqual(self.tracer.results().counts, expected)

    def test_traced_decorated_function(self):
        self.tracer.runfunc(traced_decorated_function)

        firstlineno = get_firstlineno(traced_decorated_function)
        expected = {
            (self.my_py_filename, firstlineno + 1): 1,
            (self.my_py_filename, firstlineno + 2): 1,
            (self.my_py_filename, firstlineno + 3): 1,
            (self.my_py_filename, firstlineno + 4): 1,
            (self.my_py_filename, firstlineno + 5): 1,
            (self.my_py_filename, firstlineno + 6): 1,
            (self.my_py_filename, firstlineno + 7): 2,
            (self.my_py_filename, firstlineno + 8): 2,
            (self.my_py_filename, firstlineno + 9): 2,
            (self.my_py_filename, firstlineno + 10): 1,
            (self.my_py_filename, firstlineno + 11): 1,
        }
        self.assertEqual(self.tracer.results().counts, expected)

    def test_linear_methods(self):
        # XXX todo: later add 'static_method_linear' and 'class_method_linear'
        # here, once issue1764286 is resolved
        #
        for methname in ['inst_method_linear',]:
            tracer = Trace(count=1, trace=0, countfuncs=0, countcallers=0)
            traced_obj = TracedClass(25)
            method = getattr(traced_obj, methname)
            tracer.runfunc(method, 20)

            firstlineno = get_firstlineno(method)
            expected = {
                (self.my_py_filename, firstlineno + 1): 1,
            }
            self.assertEqual(tracer.results().counts, expected)


class TestRunExecCounts(unittest.TestCase):
    """A simple sanity test of line-counting, via runctx (exec)"""
    def setUp(self):
        self.my_py_filename = fix_ext_py(__file__)
        self.addCleanup(sys.settrace, sys.gettrace())

    def test_exec_counts(self):
        self.tracer = Trace(count=1, trace=0, countfuncs=0, countcallers=0)
        code = r'''traced_func_loop(2, 5)'''
        code = compile(code, __file__, 'exec')
        self.tracer.runctx(code, globals(), vars())

        firstlineno = get_firstlineno(traced_func_loop)
        expected = {
            (self.my_py_filename, firstlineno + 1): 1,
            (self.my_py_filename, firstlineno + 2): 6,
            (self.my_py_filename, firstlineno + 3): 5,
            (self.my_py_filename, firstlineno + 4): 1,
        }

        # When used through 'run', some other spurious counts are produced, like
        # the settrace of threading, which we ignore, just making sure that the
        # counts fo traced_func_loop were right.
        #
        for k in expected.keys():
            self.assertEqual(self.tracer.results().counts[k], expected[k])


class TestFuncs(unittest.TestCase):
    """White-box testing of funcs tracing"""
    def setUp(self):
        self.addCleanup(sys.settrace, sys.gettrace())
        self.tracer = Trace(count=0, trace=0, countfuncs=1)
        self.filemod = my_file_and_modname()
        self._saved_tracefunc = sys.gettrace()

    def tearDown(self):
        if self._saved_tracefunc is not None:
            sys.settrace(self._saved_tracefunc)

    def test_simple_caller(self):
        self.tracer.runfunc(traced_func_simple_caller, 1)

        expected = {
            self.filemod + ('traced_func_simple_caller',): 1,
            self.filemod + ('traced_func_linear',): 1,
        }
        self.assertEqual(self.tracer.results().calledfuncs, expected)

    def test_arg_errors(self):
        res = self.tracer.runfunc(traced_capturer, 1, 2, self=3, func=4)
        self.assertEqual(res, ((1, 2), {'self': 3, 'func': 4}))
        with self.assertRaises(TypeError):
            self.tracer.runfunc(func=traced_capturer, arg=1)
        with self.assertRaises(TypeError):
            self.tracer.runfunc()

    def test_loop_caller_importing(self):
        self.tracer.runfunc(traced_func_importing_caller, 1)

        expected = {
            self.filemod + ('traced_func_simple_caller',): 1,
            self.filemod + ('traced_func_linear',): 1,
            self.filemod + ('traced_func_importing_caller',): 1,
            self.filemod + ('traced_func_importing',): 1,
            (fix_ext_py(testmod.__file__), 'testmod', 'func'): 1,
        }
        self.assertEqual(self.tracer.results().calledfuncs, expected)

    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'pre-existing trace function throws off measurements')
    def test_inst_method_calling(self):
        obj = TracedClass(20)
        self.tracer.runfunc(obj.inst_method_calling, 1)

        expected = {
            self.filemod + ('TracedClass.inst_method_calling',): 1,
            self.filemod + ('TracedClass.inst_method_linear',): 1,
            self.filemod + ('traced_func_linear',): 1,
        }
        self.assertEqual(self.tracer.results().calledfuncs, expected)

    def test_traced_decorated_function(self):
        self.tracer.runfunc(traced_decorated_function)

        expected = {
            self.filemod + ('traced_decorated_function',): 1,
            self.filemod + ('decorator_fabric',): 1,
            self.filemod + ('decorator2',): 1,
            self.filemod + ('decorator1',): 1,
            self.filemod + ('func',): 1,
        }
        self.assertEqual(self.tracer.results().calledfuncs, expected)


class TestCallers(unittest.TestCase):
    """White-box testing of callers tracing"""
    def setUp(self):
        self.addCleanup(sys.settrace, sys.gettrace())
        self.tracer = Trace(count=0, trace=0, countcallers=1)
        self.filemod = my_file_and_modname()

    @unittest.skipIf(hasattr(sys, 'gettrace') and sys.gettrace(),
                     'pre-existing trace function throws off measurements')
    def test_loop_caller_importing(self):
        self.tracer.runfunc(traced_func_importing_caller, 1)

        expected = {
            ((os.path.splitext(trace.__file__)[0] + '.py', 'trace', 'Trace.runfunc'),
                (self.filemod + ('traced_func_importing_caller',))): 1,
            ((self.filemod + ('traced_func_simple_caller',)),
                (self.filemod + ('traced_func_linear',))): 1,
            ((self.filemod + ('traced_func_importing_caller',)),
                (self.filemod + ('traced_func_simple_caller',))): 1,
            ((self.filemod + ('traced_func_importing_caller',)),
                (self.filemod + ('traced_func_importing',))): 1,
            ((self.filemod + ('traced_func_importing',)),
                (fix_ext_py(testmod.__file__), 'testmod', 'func')): 1,
        }
        self.assertEqual(self.tracer.results().callers, expected)


# Created separately for issue #3821
class TestCoverage(unittest.TestCase):
    def setUp(self):
        self.addCleanup(sys.settrace, sys.gettrace())

    def tearDown(self):
        rmtree(TESTFN)
        unlink(TESTFN)

    DEFAULT_SCRIPT = '''if True:
        import unittest
        from test.test_pprint import QueryTestCase
        loader = unittest.TestLoader()
        tests = loader.loadTestsFromTestCase(QueryTestCase)
        tests(unittest.TestResult())
        '''
    def _coverage(self, tracer, cmd=DEFAULT_SCRIPT):
        tracer.run(cmd)
        r = tracer.results()
        r.write_results(show_missing=True, summary=True, coverdir=TESTFN)

    @requires_resource('cpu')
    def test_coverage(self):
        tracer = trace.Trace(trace=0, count=1)
        with captured_stdout() as stdout:
            self._coverage(tracer)
        stdout = stdout.getvalue()
        self.assertIn("pprint.py", stdout)
        self.assertIn("case.py", stdout)   # from unittest
        files = os.listdir(TESTFN)
        self.assertIn("pprint.cover", files)
        self.assertIn("unittest.case.cover", files)

    def test_coverage_ignore(self):
        # Ignore all files, nothing should be traced nor printed
        libpath = os.path.normpath(os.path.dirname(os.path.dirname(__file__)))
        # sys.prefix does not work when running from a checkout
        tracer = trace.Trace(ignoredirs=[sys.base_prefix, sys.base_exec_prefix,
                             libpath] + sys.path, trace=0, count=1)
        with captured_stdout() as stdout:
            self._coverage(tracer)
        if os.path.exists(TESTFN):
            files = os.listdir(TESTFN)
            self.assertEqual(files, ['_importlib.cover'])  # Ignore __import__

    def test_issue9936(self):
        tracer = trace.Trace(trace=0, count=1)
        modname = 'test.tracedmodules.testmod'
        # Ensure that the module is executed in import
        if modname in sys.modules:
            del sys.modules[modname]
        cmd = ("import test.tracedmodules.testmod as t;"
               "t.func(0); t.func2();")
        with captured_stdout() as stdout:
            self._coverage(tracer, cmd)
        stdout.seek(0)
        stdout.readline()
        coverage = {}
        for line in stdout:
            lines, cov, module = line.split()[:3]
            coverage[module] = (float(lines), float(cov[:-1]))
        # XXX This is needed to run regrtest.py as a script
        modname = trace._fullmodname(sys.modules[modname].__file__)
        self.assertIn(modname, coverage)
        self.assertEqual(coverage[modname], (5, 100))

    def test_coverageresults_update(self):
        # Update empty CoverageResults with a non-empty infile.
        infile = TESTFN + '-infile'
        with open(infile, 'wb') as f:
            dump(({}, {}, {'caller': 1}), f, protocol=1)
        self.addCleanup(unlink, infile)
        results = trace.CoverageResults({}, {}, infile, {})
        self.assertEqual(results.callers, {'caller': 1})

### Tests that don't mess with sys.settrace and can be traced
### themselves TODO: Skip tests that do mess with sys.settrace when
### regrtest is invoked with -T option.
class Test_Ignore(unittest.TestCase):
    def test_ignored(self):
        jn = os.path.join
        ignore = trace._Ignore(['x', 'y.z'], [jn('foo', 'bar')])
        self.assertTrue(ignore.names('x.py', 'x'))
        self.assertFalse(ignore.names('xy.py', 'xy'))
        self.assertFalse(ignore.names('y.py', 'y'))
        self.assertTrue(ignore.names(jn('foo', 'bar', 'baz.py'), 'baz'))
        self.assertFalse(ignore.names(jn('bar', 'z.py'), 'z'))
        # Matched before.
        self.assertTrue(ignore.names(jn('bar', 'baz.py'), 'baz'))

# Created for Issue 31908 -- CLI utility not writing cover files
class TestCoverageCommandLineOutput(unittest.TestCase):

    codefile = 'tmp.py'
    coverfile = 'tmp.cover'

    def setUp(self):
        with open(self.codefile, 'w', encoding='iso-8859-15') as f:
            f.write(textwrap.dedent('''\
                # coding: iso-8859-15
                x = 'spœm'
                if []:
                    print('unreachable')
            '''))

    def tearDown(self):
        unlink(self.codefile)
        unlink(self.coverfile)

    def test_cover_files_written_no_highlight(self):
        # Test also that the cover file for the trace module is not created
        # (issue #34171).
        tracedir = os.path.dirname(os.path.abspath(trace.__file__))
        tracecoverpath = os.path.join(tracedir, 'trace.cover')
        unlink(tracecoverpath)

        argv = '-m trace --count'.split() + [self.codefile]
        status, stdout, stderr = assert_python_ok(*argv)
        self.assertEqual(stderr, b'')
        self.assertFalse(os.path.exists(tracecoverpath))
        self.assertTrue(os.path.exists(self.coverfile))
        with open(self.coverfile, encoding='iso-8859-15') as f:
            self.assertEqual(f.read(),
                "       # coding: iso-8859-15\n"
                "    1: x = 'spœm'\n"
                "    1: if []:\n"
                "           print('unreachable')\n"
            )

    def test_cover_files_written_with_highlight(self):
        argv = '-m trace --count --missing'.split() + [self.codefile]
        status, stdout, stderr = assert_python_ok(*argv)
        self.assertTrue(os.path.exists(self.coverfile))
        with open(self.coverfile, encoding='iso-8859-15') as f:
            self.assertEqual(f.read(), textwrap.dedent('''\
                       # coding: iso-8859-15
                    1: x = 'spœm'
                    1: if []:
                >>>>>>     print('unreachable')
            '''))

class TestCommandLine(unittest.TestCase):

    def test_failures(self):
        _errors = (
            (b'progname is missing: required with the main options', '-l', '-T'),
            (b'cannot specify both --listfuncs and (--trace or --count)', '-lc'),
            (b'argument -R/--no-report: not allowed with argument -r/--report', '-rR'),
            (b'must specify one of --trace, --count, --report, --listfuncs, or --trackcalls', '-g'),
            (b'-r/--report requires -f/--file', '-r'),
            (b'--summary can only be used with --count or --report', '-sT'),
            (b'unrecognized arguments: -y', '-y'))
        for message, *args in _errors:
            *_, stderr = assert_python_failure('-m', 'trace', *args)
            self.assertIn(message, stderr)

    def test_listfuncs_flag_success(self):
        filename = TESTFN + '.py'
        modulename = os.path.basename(TESTFN)
        with open(filename, 'w', encoding='utf-8') as fd:
            self.addCleanup(unlink, filename)
            fd.write("a = 1\n")
            status, stdout, stderr = assert_python_ok('-m', 'trace', '-l', filename,
                                                      PYTHONIOENCODING='utf-8')
            self.assertIn(b'functions called:', stdout)
            expected = f'filename: {filename}, modulename: {modulename}, funcname: <module>'
            self.assertIn(expected.encode(), stdout)

    def test_sys_argv_list(self):
        with open(TESTFN, 'w', encoding='utf-8') as fd:
            self.addCleanup(unlink, TESTFN)
            fd.write("import sys\n")
            fd.write("print(type(sys.argv))\n")

        status, direct_stdout, stderr = assert_python_ok(TESTFN)
        status, trace_stdout, stderr = assert_python_ok('-m', 'trace', '-l', TESTFN,
                                                        PYTHONIOENCODING='utf-8')
        self.assertIn(direct_stdout.strip(), trace_stdout)

    def test_count_and_summary(self):
        filename = f'{TESTFN}.py'
        coverfilename = f'{TESTFN}.cover'
        modulename = os.path.basename(TESTFN)
        with open(filename, 'w', encoding='utf-8') as fd:
            self.addCleanup(unlink, filename)
            self.addCleanup(unlink, coverfilename)
            fd.write(textwrap.dedent("""\
                x = 1
                y = 2

                def f():
                    return x + y

                for i in range(10):
                    f()
            """))
        status, stdout, _ = assert_python_ok('-m', 'trace', '-cs', filename,
                                             PYTHONIOENCODING='utf-8')
        stdout = stdout.decode()
        self.assertEqual(status, 0)
        self.assertIn('lines   cov%   module   (path)', stdout)
        self.assertIn(f'6   100.0%   {modulename}   ({filename})', stdout)

    def test_run_as_module(self):
        assert_python_ok('-m', 'trace', '-l', '--module', 'timeit', '-n', '1')
        assert_python_failure('-m', 'trace', '-l', '--module', 'not_a_module_zzz')


class TestTrace(unittest.TestCase):
    def setUp(self):
        self.addCleanup(sys.settrace, sys.gettrace())
        self.tracer = Trace(count=0, trace=1)
        self.filemod = my_file_and_modname()

    def test_no_source_file(self):
        filename = "<unknown>"
        co = traced_func_linear.__code__
        co = co.replace(co_filename=filename)
        f = FunctionType(co, globals())

        with captured_stdout() as out:
            self.tracer.runfunc(f, 2, 3)

        out = out.getvalue().splitlines()
        firstlineno = get_firstlineno(f)
        self.assertIn(f" --- modulename: {self.filemod[1]}, funcname: {f.__code__.co_name}", out[0])
        self.assertIn(f"{filename}({firstlineno + 1})", out[1])
        self.assertIn(f"{filename}({firstlineno + 2})", out[2])
        self.assertIn(f"{filename}({firstlineno + 3})", out[3])
        self.assertIn(f"{filename}({firstlineno + 4})", out[4])


if __name__ == '__main__':
    unittest.main()
