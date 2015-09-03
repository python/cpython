import unittest
import sys
from test.test_support import check_py3k_warnings, CleanImport, run_unittest
import warnings
from test import test_support

if not sys.py3kwarning:
    raise unittest.SkipTest('%s must be run with the -3 flag' % __name__)

try:
    from test.test_support import __warningregistry__ as _registry
except ImportError:
    def check_deprecated_module(module_name):
        return False
else:
    past_warnings = _registry.keys()
    del _registry
    def check_deprecated_module(module_name):
        """Lookup the past warnings for module already loaded using
        test_support.import_module(..., deprecated=True)
        """
        return any(module_name in msg and ' removed' in msg
                   and issubclass(cls, DeprecationWarning)
                   and (' module' in msg or ' package' in msg)
                   for (msg, cls, line) in past_warnings)

def reset_module_registry(module):
    try:
        registry = module.__warningregistry__
    except AttributeError:
        pass
    else:
        registry.clear()

class TestPy3KWarnings(unittest.TestCase):

    def assertWarning(self, _, warning, expected_message):
        self.assertEqual(str(warning.message), expected_message)

    def assertNoWarning(self, _, recorder):
        self.assertEqual(len(recorder.warnings), 0)

    def test_backquote(self):
        expected = 'backquote not supported in 3.x; use repr()'
        with check_py3k_warnings((expected, SyntaxWarning)):
            exec "`2`" in {}

    def test_paren_arg_names(self):
        expected = 'parenthesized argument names are invalid in 3.x'
        def check(s):
            with check_py3k_warnings((expected, SyntaxWarning)):
                exec s in {}
        check("def f((x)): pass")
        check("def f((((x))), (y)): pass")
        check("def f((x), (((y))), m=32): pass")
        # Something like def f((a, (b))): pass will raise the tuple
        # unpacking warning.

    def test_forbidden_names(self):
        # So we don't screw up our globals
        def safe_exec(expr):
            def f(**kwargs): pass
            exec expr in {'f' : f}

        tests = [("True", "assignment to True or False is forbidden in 3.x"),
                 ("False", "assignment to True or False is forbidden in 3.x"),
                 ("nonlocal", "nonlocal is a keyword in 3.x")]
        with check_py3k_warnings(('', SyntaxWarning)) as w:
            for keyword, expected in tests:
                safe_exec("{0} = False".format(keyword))
                self.assertWarning(None, w, expected)
                w.reset()
                try:
                    safe_exec("obj.{0} = True".format(keyword))
                except NameError:
                    pass
                self.assertWarning(None, w, expected)
                w.reset()
                safe_exec("def {0}(): pass".format(keyword))
                self.assertWarning(None, w, expected)
                w.reset()
                safe_exec("class {0}: pass".format(keyword))
                self.assertWarning(None, w, expected)
                w.reset()
                safe_exec("def f({0}=43): pass".format(keyword))
                self.assertWarning(None, w, expected)
                w.reset()


    def test_type_inequality_comparisons(self):
        expected = 'type inequality comparisons not supported in 3.x'
        with check_py3k_warnings() as w:
            self.assertWarning(int < str, w, expected)
            w.reset()
            self.assertWarning(type < object, w, expected)

    def test_object_inequality_comparisons(self):
        expected = 'comparing unequal types not supported in 3.x'
        with check_py3k_warnings() as w:
            self.assertWarning(str < [], w, expected)
            w.reset()
            self.assertWarning(object() < (1, 2), w, expected)

    def test_dict_inequality_comparisons(self):
        expected = 'dict inequality comparisons not supported in 3.x'
        with check_py3k_warnings() as w:
            self.assertWarning({} < {2:3}, w, expected)
            w.reset()
            self.assertWarning({} <= {}, w, expected)
            w.reset()
            self.assertWarning({} > {2:3}, w, expected)
            w.reset()
            self.assertWarning({2:3} >= {}, w, expected)

    def test_cell_inequality_comparisons(self):
        expected = 'cell comparisons not supported in 3.x'
        def f(x):
            def g():
                return x
            return g
        cell0, = f(0).func_closure
        cell1, = f(1).func_closure
        with check_py3k_warnings() as w:
            self.assertWarning(cell0 == cell1, w, expected)
            w.reset()
            self.assertWarning(cell0 < cell1, w, expected)

    def test_code_inequality_comparisons(self):
        expected = 'code inequality comparisons not supported in 3.x'
        def f(x):
            pass
        def g(x):
            pass
        with check_py3k_warnings() as w:
            self.assertWarning(f.func_code < g.func_code, w, expected)
            w.reset()
            self.assertWarning(f.func_code <= g.func_code, w, expected)
            w.reset()
            self.assertWarning(f.func_code >= g.func_code, w, expected)
            w.reset()
            self.assertWarning(f.func_code > g.func_code, w, expected)

    def test_builtin_function_or_method_comparisons(self):
        expected = ('builtin_function_or_method '
                    'order comparisons not supported in 3.x')
        func = eval
        meth = {}.get
        with check_py3k_warnings() as w:
            self.assertWarning(func < meth, w, expected)
            w.reset()
            self.assertWarning(func > meth, w, expected)
            w.reset()
            self.assertWarning(meth <= func, w, expected)
            w.reset()
            self.assertWarning(meth >= func, w, expected)
            w.reset()
            self.assertNoWarning(meth == func, w)
            self.assertNoWarning(meth != func, w)
            lam = lambda x: x
            self.assertNoWarning(lam == func, w)
            self.assertNoWarning(lam != func, w)

    def test_frame_attributes(self):
        template = "%s has been removed in 3.x"
        f = sys._getframe(0)
        for attr in ("f_exc_traceback", "f_exc_value", "f_exc_type"):
            expected = template % attr
            with check_py3k_warnings() as w:
                self.assertWarning(getattr(f, attr), w, expected)
                w.reset()
                self.assertWarning(setattr(f, attr, None), w, expected)

    def test_sort_cmp_arg(self):
        expected = "the cmp argument is not supported in 3.x"
        lst = range(5)
        cmp = lambda x,y: -1

        with check_py3k_warnings() as w:
            self.assertWarning(lst.sort(cmp=cmp), w, expected)
            w.reset()
            self.assertWarning(sorted(lst, cmp=cmp), w, expected)
            w.reset()
            self.assertWarning(lst.sort(cmp), w, expected)
            w.reset()
            self.assertWarning(sorted(lst, cmp), w, expected)

    def test_sys_exc_clear(self):
        expected = 'sys.exc_clear() not supported in 3.x; use except clauses'
        with check_py3k_warnings() as w:
            self.assertWarning(sys.exc_clear(), w, expected)

    def test_methods_members(self):
        expected = '__members__ and __methods__ not supported in 3.x'
        class C:
            __methods__ = ['a']
            __members__ = ['b']
        c = C()
        with check_py3k_warnings() as w:
            self.assertWarning(dir(c), w, expected)

    def test_softspace(self):
        expected = 'file.softspace not supported in 3.x'
        with file(__file__) as f:
            with check_py3k_warnings() as w:
                self.assertWarning(f.softspace, w, expected)
            def set():
                f.softspace = 0
            with check_py3k_warnings() as w:
                self.assertWarning(set(), w, expected)

    def test_slice_methods(self):
        class Spam(object):
            def __getslice__(self, i, j): pass
            def __setslice__(self, i, j, what): pass
            def __delslice__(self, i, j): pass
        class Egg:
            def __getslice__(self, i, h): pass
            def __setslice__(self, i, j, what): pass
            def __delslice__(self, i, j): pass

        expected = "in 3.x, __{0}slice__ has been removed; use __{0}item__"

        for obj in (Spam(), Egg()):
            with check_py3k_warnings() as w:
                self.assertWarning(obj[1:2], w, expected.format('get'))
                w.reset()
                del obj[3:4]
                self.assertWarning(None, w, expected.format('del'))
                w.reset()
                obj[4:5] = "eggs"
                self.assertWarning(None, w, expected.format('set'))

    def test_tuple_parameter_unpacking(self):
        expected = "tuple parameter unpacking has been removed in 3.x"
        with check_py3k_warnings((expected, SyntaxWarning)):
            exec "def f((a, b)): pass"

    def test_buffer(self):
        expected = 'buffer() not supported in 3.x'
        with check_py3k_warnings() as w:
            self.assertWarning(buffer('a'), w, expected)

    def test_file_xreadlines(self):
        expected = ("f.xreadlines() not supported in 3.x, "
                    "try 'for line in f' instead")
        with file(__file__) as f:
            with check_py3k_warnings() as w:
                self.assertWarning(f.xreadlines(), w, expected)

    def test_hash_inheritance(self):
        with check_py3k_warnings() as w:
            # With object as the base class
            class WarnOnlyCmp(object):
                def __cmp__(self, other): pass
            self.assertEqual(len(w.warnings), 0)
            w.reset()
            class WarnOnlyEq(object):
                def __eq__(self, other): pass
            self.assertEqual(len(w.warnings), 1)
            self.assertWarning(None, w,
                 "Overriding __eq__ blocks inheritance of __hash__ in 3.x")
            w.reset()
            class WarnCmpAndEq(object):
                def __cmp__(self, other): pass
                def __eq__(self, other): pass
            self.assertEqual(len(w.warnings), 1)
            self.assertWarning(None, w,
                 "Overriding __eq__ blocks inheritance of __hash__ in 3.x")
            w.reset()
            class NoWarningOnlyHash(object):
                def __hash__(self): pass
            self.assertEqual(len(w.warnings), 0)
            # With an intermediate class in the heirarchy
            class DefinesAllThree(object):
                def __cmp__(self, other): pass
                def __eq__(self, other): pass
                def __hash__(self): pass
            class WarnOnlyCmp(DefinesAllThree):
                def __cmp__(self, other): pass
            self.assertEqual(len(w.warnings), 0)
            w.reset()
            class WarnOnlyEq(DefinesAllThree):
                def __eq__(self, other): pass
            self.assertEqual(len(w.warnings), 1)
            self.assertWarning(None, w,
                 "Overriding __eq__ blocks inheritance of __hash__ in 3.x")
            w.reset()
            class WarnCmpAndEq(DefinesAllThree):
                def __cmp__(self, other): pass
                def __eq__(self, other): pass
            self.assertEqual(len(w.warnings), 1)
            self.assertWarning(None, w,
                 "Overriding __eq__ blocks inheritance of __hash__ in 3.x")
            w.reset()
            class NoWarningOnlyHash(DefinesAllThree):
                def __hash__(self): pass
            self.assertEqual(len(w.warnings), 0)

    def test_operator(self):
        from operator import isCallable, sequenceIncludes

        callable_warn = ("operator.isCallable() is not supported in 3.x. "
                         "Use hasattr(obj, '__call__').")
        seq_warn = ("operator.sequenceIncludes() is not supported "
                    "in 3.x. Use operator.contains().")
        with check_py3k_warnings() as w:
            self.assertWarning(isCallable(self), w, callable_warn)
            w.reset()
            self.assertWarning(sequenceIncludes(range(3), 2), w, seq_warn)

    def test_nonascii_bytes_literals(self):
        expected = "non-ascii bytes literals not supported in 3.x"
        with check_py3k_warnings((expected, SyntaxWarning)):
            exec "b'\xbd'"


class TestStdlibRemovals(unittest.TestCase):

    # test.testall not tested as it executes all unit tests as an
    # import side-effect.
    all_platforms = ('audiodev', 'imputil', 'mutex', 'user', 'new', 'rexec',
                        'Bastion', 'compiler', 'dircache', 'mimetools',
                        'fpformat', 'ihooks', 'mhlib', 'statvfs', 'htmllib',
                        'sgmllib', 'rfc822', 'sunaudio')
    inclusive_platforms = {'irix' : ('pure', 'AL', 'al', 'CD', 'cd', 'cddb',
                                     'cdplayer', 'CL', 'cl', 'DEVICE', 'GL',
                                     'gl', 'ERRNO', 'FILE', 'FL', 'flp', 'fl',
                                     'fm', 'GET', 'GLWS', 'imgfile', 'IN',
                                     'IOCTL', 'jpeg', 'panel', 'panelparser',
                                     'readcd', 'SV', 'torgb', 'WAIT'),
                          'darwin' : ('autoGIL', 'Carbon', 'OSATerminology',
                                      'icglue', 'Nav',
                                      # MacOS should (and does) give a Py3kWarning, but one of the
                                      # earlier tests already imports the MacOS extension which causes
                                      # this test to fail. Disabling the test for 'MacOS' avoids this
                                      # spurious test failure.
                                      #'MacOS',
                                      'aepack',
                                      'aetools', 'aetypes', 'applesingle',
                                      'appletrawmain', 'appletrunner',
                                      'argvemulator', 'bgenlocations',
                                      'EasyDialogs', 'macerrors', 'macostools',
                                      'findertools', 'FrameWork', 'ic',
                                      'gensuitemodule', 'icopen', 'macresource',
                                      'MiniAEFrame', 'pimp', 'PixMapWrapper',
                                      'terminalcommand', 'videoreader',
                                      '_builtinSuites', 'CodeWarrior',
                                      'Explorer', 'Finder', 'Netscape',
                                      'StdSuites', 'SystemEvents', 'Terminal',
                                      'cfmfile', 'bundlebuilder', 'buildtools',
                                      'ColorPicker', 'Audio_mac'),
                           'sunos5' : ('sunaudiodev', 'SUNAUDIODEV'),
                          }
    optional_modules = ('bsddb185', 'Canvas', 'dl', 'linuxaudiodev', 'imageop',
                        'sv', 'bsddb', 'dbhash')

    def check_removal(self, module_name, optional=False):
        """Make sure the specified module, when imported, raises a
        DeprecationWarning and specifies itself in the message."""
        if module_name in sys.modules:
            mod = sys.modules[module_name]
            filename = getattr(mod, '__file__', '')
            mod = None
            # the module is not implemented in C?
            if not filename.endswith(('.py', '.pyc', '.pyo')):
                # Issue #23375: If the module was already loaded, reimporting
                # the module will not emit again the warning. The warning is
                # emited when the module is loaded, but C modules cannot
                # unloaded.
                if test_support.verbose:
                    print("Cannot test the Python 3 DeprecationWarning of the "
                          "%s module, the C module is already loaded"
                          % module_name)
                return
        with CleanImport(module_name), warnings.catch_warnings():
            warnings.filterwarnings("error", ".+ (module|package) .+ removed",
                                    DeprecationWarning, __name__)
            warnings.filterwarnings("error", ".+ removed .+ (module|package)",
                                    DeprecationWarning, __name__)
            try:
                __import__(module_name, level=0)
            except DeprecationWarning as exc:
                self.assertIn(module_name, exc.args[0],
                              "%s warning didn't contain module name"
                              % module_name)
            except ImportError:
                if not optional:
                    self.fail("Non-optional module {0} raised an "
                              "ImportError.".format(module_name))
            else:
                # For extension modules, check the __warningregistry__.
                # They won't rerun their init code even with CleanImport.
                if not check_deprecated_module(module_name):
                    self.fail("DeprecationWarning not raised for {0}"
                              .format(module_name))

    def test_platform_independent_removals(self):
        # Make sure that the modules that are available on all platforms raise
        # the proper DeprecationWarning.
        for module_name in self.all_platforms:
            self.check_removal(module_name)

    def test_platform_specific_removals(self):
        # Test the removal of platform-specific modules.
        for module_name in self.inclusive_platforms.get(sys.platform, []):
            self.check_removal(module_name, optional=True)

    def test_optional_module_removals(self):
        # Test the removal of modules that may or may not be built.
        for module_name in self.optional_modules:
            self.check_removal(module_name, optional=True)

    def test_os_path_walk(self):
        msg = "In 3.x, os.path.walk is removed in favor of os.walk."
        def dumbo(where, names, args): pass
        for path_mod in ("ntpath", "macpath", "os2emxpath", "posixpath"):
            mod = __import__(path_mod)
            reset_module_registry(mod)
            with check_py3k_warnings() as w:
                mod.walk("crashers", dumbo, None)
            self.assertEqual(str(w.message), msg)

    def test_reduce_move(self):
        from operator import add
        # reduce tests may have already triggered this warning
        reset_module_registry(unittest.case)
        with warnings.catch_warnings():
            warnings.filterwarnings("error", "reduce")
            self.assertRaises(DeprecationWarning, reduce, add, range(10))

    def test_mutablestring_removal(self):
        # UserString.MutableString has been removed in 3.0.
        import UserString
        # UserString tests may have already triggered this warning
        reset_module_registry(UserString)
        with warnings.catch_warnings():
            warnings.filterwarnings("error", ".*MutableString",
                                    DeprecationWarning)
            self.assertRaises(DeprecationWarning, UserString.MutableString)


def test_main():
    run_unittest(TestPy3KWarnings,
                 TestStdlibRemovals)

if __name__ == '__main__':
    test_main()
