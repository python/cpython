import unittest
import sys
from test.test_support import (catch_warning, CleanImport,
                               TestSkipped, run_unittest)
import warnings

if not sys.py3kwarning:
    raise TestSkipped('%s must be run with the -3 flag' % __name__)


class TestPy3KWarnings(unittest.TestCase):

    def test_type_inequality_comparisons(self):
        expected = 'type inequality comparisons not supported in 3.x'
        with catch_warning() as w:
            self.assertWarning(int < str, w, expected)
        with catch_warning() as w:
            self.assertWarning(type < object, w, expected)

    def test_object_inequality_comparisons(self):
        expected = 'comparing unequal types not supported in 3.x'
        with catch_warning() as w:
            self.assertWarning(str < [], w, expected)
        with catch_warning() as w:
            self.assertWarning(object() < (1, 2), w, expected)

    def test_dict_inequality_comparisons(self):
        expected = 'dict inequality comparisons not supported in 3.x'
        with catch_warning() as w:
            self.assertWarning({} < {2:3}, w, expected)
        with catch_warning() as w:
            self.assertWarning({} <= {}, w, expected)
        with catch_warning() as w:
            self.assertWarning({} > {2:3}, w, expected)
        with catch_warning() as w:
            self.assertWarning({2:3} >= {}, w, expected)

    def test_cell_inequality_comparisons(self):
        expected = 'cell comparisons not supported in 3.x'
        def f(x):
            def g():
                return x
            return g
        cell0, = f(0).func_closure
        cell1, = f(1).func_closure
        with catch_warning() as w:
            self.assertWarning(cell0 == cell1, w, expected)
        with catch_warning() as w:
            self.assertWarning(cell0 < cell1, w, expected)

    def test_code_inequality_comparisons(self):
        expected = 'code inequality comparisons not supported in 3.x'
        def f(x):
            pass
        def g(x):
            pass
        with catch_warning() as w:
            self.assertWarning(f.func_code < g.func_code, w, expected)
        with catch_warning() as w:
            self.assertWarning(f.func_code <= g.func_code, w, expected)
        with catch_warning() as w:
            self.assertWarning(f.func_code >= g.func_code, w, expected)
        with catch_warning() as w:
            self.assertWarning(f.func_code > g.func_code, w, expected)

    def test_builtin_function_or_method_comparisons(self):
        expected = ('builtin_function_or_method '
                    'inequality comparisons not supported in 3.x')
        func = eval
        meth = {}.get
        with catch_warning() as w:
            self.assertWarning(func < meth, w, expected)
        with catch_warning() as w:
            self.assertWarning(func > meth, w, expected)
        with catch_warning() as w:
            self.assertWarning(meth <= func, w, expected)
        with catch_warning() as w:
            self.assertWarning(meth >= func, w, expected)

    def assertWarning(self, _, warning, expected_message):
        self.assertEqual(str(warning.message), expected_message)

    def test_sort_cmp_arg(self):
        expected = "the cmp argument is not supported in 3.x"
        lst = range(5)
        cmp = lambda x,y: -1

        with catch_warning() as w:
            self.assertWarning(lst.sort(cmp=cmp), w, expected)
        with catch_warning() as w:
            self.assertWarning(sorted(lst, cmp=cmp), w, expected)
        with catch_warning() as w:
            self.assertWarning(lst.sort(cmp), w, expected)
        with catch_warning() as w:
            self.assertWarning(sorted(lst, cmp), w, expected)

    def test_sys_exc_clear(self):
        expected = 'sys.exc_clear() not supported in 3.x; use except clauses'
        with catch_warning() as w:
            self.assertWarning(sys.exc_clear(), w, expected)

    def test_methods_members(self):
        expected = '__members__ and __methods__ not supported in 3.x'
        class C:
            __methods__ = ['a']
            __members__ = ['b']
        c = C()
        with catch_warning() as w:
            self.assertWarning(dir(c), w, expected)

    def test_softspace(self):
        expected = 'file.softspace not supported in 3.x'
        with file(__file__) as f:
            with catch_warning() as w:
                self.assertWarning(f.softspace, w, expected)
            def set():
                f.softspace = 0
            with catch_warning() as w:
                self.assertWarning(set(), w, expected)

    def test_buffer(self):
        expected = 'buffer() not supported in 3.x; use memoryview()'
        with catch_warning() as w:
            self.assertWarning(buffer('a'), w, expected)


class TestStdlibRemovals(unittest.TestCase):

    # test.testall not tested as it executes all unit tests as an
    # import side-effect.
    all_platforms = ('audiodev', 'imputil', 'mutex', 'user', 'new', 'rexec',
                        'Bastion', 'compiler', 'dircache', 'fpformat',
                        'ihooks', 'mhlib')
    inclusive_platforms = {'irix' : ('pure', 'AL', 'al', 'CD', 'cd', 'cddb',
                                     'cdplayer', 'CL', 'cl', 'DEVICE', 'GL',
                                     'gl', 'ERRNO', 'FILE', 'FL', 'flp', 'fl'),
                          'darwin' : ('autoGIL', 'Carbon', 'OSATerminology',
                                      'icglue', 'Nav', 'MacOS', 'aepack',
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
                                      'ColorPicker')}
    optional_modules = ('bsddb185', 'Canvas', 'dl', 'linuxaudiodev', 'imageop',
                        'sv')

    def check_removal(self, module_name, optional=False):
        """Make sure the specified module, when imported, raises a
        DeprecationWarning and specifies itself in the message."""
        with CleanImport(module_name):
            with catch_warning(record=False) as w:
                warnings.filterwarnings("error", ".+ removed",
                                        DeprecationWarning)
                try:
                    __import__(module_name, level=0)
                except DeprecationWarning as exc:
                    self.assert_(module_name in exc.args[0],
                                 "%s warning didn't contain module name"
                                 % module_name)
                except ImportError:
                    if not optional:
                        self.fail("Non-optional module {0} raised an "
                                  "ImportError.".format(module_name))
                else:
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
            with catch_warning() as w:
                # Since os3exmpath just imports it from ntpath
                warnings.simplefilter("always")
                mod.walk(".", dumbo, None)
            self.assertEquals(str(w.message), msg)


class TestStdlibRenames(unittest.TestCase):

    renames = {'copy_reg': 'copyreg', 'Queue': 'queue',
               'SocketServer': 'socketserver',
               'ConfigParser': 'configparser'}

    def check_rename(self, module_name, new_module_name):
        """Make sure that:
        - A DeprecationWarning is raised when importing using the
          old 2.x module name.
        - The module can be imported using the new 3.x name.
        - The warning message specify both names.
        """
        with CleanImport(module_name):
            with catch_warning(record=False) as w:
                warnings.filterwarnings("error", ".+ renamed to",
                                        DeprecationWarning)
                try:
                    __import__(module_name, level=0)
                except DeprecationWarning as exc:
                    self.assert_(module_name in exc.args[0])
                    self.assert_(new_module_name in exc.args[0])
                else:
                    self.fail("DeprecationWarning not raised for %s" %
                              module_name)
        with CleanImport(new_module_name):
            try:
                __import__(new_module_name, level=0)
            except ImportError:
                self.fail("cannot import %s with its 3.x name, %s" %
                          module_name, new_module_name)
            except DeprecationWarning:
                self.fail("unexpected DeprecationWarning raised for %s" %
                          module_name)

    def test_module_renames(self):
        for module_name, new_module_name in self.renames.items():
            self.check_rename(module_name, new_module_name)


def test_main():
    run_unittest(TestPy3KWarnings,
                 TestStdlibRemovals,
                 TestStdlibRenames)

if __name__ == '__main__':
    test_main()
