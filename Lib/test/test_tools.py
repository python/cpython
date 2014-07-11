"""Tests for scripts in the Tools directory.

This file contains regression tests for some of the scripts found in the
Tools directory of a Python checkout or tarball, such as reindent.py.
"""

import os
import sys
import importlib._bootstrap
import importlib.machinery
import unittest
from unittest import mock
import shutil
import subprocess
import sysconfig
import tempfile
import textwrap
from test import support
from test.script_helper import assert_python_ok, assert_python_failure

if not sysconfig.is_python_build():
    # XXX some installers do contain the tools, should we detect that
    # and run the tests in that case too?
    raise unittest.SkipTest('test irrelevant for an installed Python')

basepath = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(__file__))),
                        'Tools')
scriptsdir = os.path.join(basepath, 'scripts')


class ReindentTests(unittest.TestCase):
    script = os.path.join(scriptsdir, 'reindent.py')

    def test_noargs(self):
        assert_python_ok(self.script)

    def test_help(self):
        rc, out, err = assert_python_ok(self.script, '-h')
        self.assertEqual(out, b'')
        self.assertGreater(err, b'')


class PindentTests(unittest.TestCase):
    script = os.path.join(scriptsdir, 'pindent.py')

    def assertFileEqual(self, fn1, fn2):
        with open(fn1) as f1, open(fn2) as f2:
            self.assertEqual(f1.readlines(), f2.readlines())

    def pindent(self, source, *args):
        with subprocess.Popen(
                (sys.executable, self.script) + args,
                stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                universal_newlines=True) as proc:
            out, err = proc.communicate(source)
        self.assertIsNone(err)
        return out

    def lstriplines(self, data):
        return '\n'.join(line.lstrip() for line in data.splitlines()) + '\n'

    def test_selftest(self):
        self.maxDiff = None
        with support.temp_dir() as directory:
            data_path = os.path.join(directory, '_test.py')
            with open(self.script) as f:
                closed = f.read()
            with open(data_path, 'w') as f:
                f.write(closed)

            rc, out, err = assert_python_ok(self.script, '-d', data_path)
            self.assertEqual(out, b'')
            self.assertEqual(err, b'')
            backup = data_path + '~'
            self.assertTrue(os.path.exists(backup))
            with open(backup) as f:
                self.assertEqual(f.read(), closed)
            with open(data_path) as f:
                clean = f.read()
            compile(clean, '_test.py', 'exec')
            self.assertEqual(self.pindent(clean, '-c'), closed)
            self.assertEqual(self.pindent(closed, '-d'), clean)

            rc, out, err = assert_python_ok(self.script, '-c', data_path)
            self.assertEqual(out, b'')
            self.assertEqual(err, b'')
            with open(backup) as f:
                self.assertEqual(f.read(), clean)
            with open(data_path) as f:
                self.assertEqual(f.read(), closed)

            broken = self.lstriplines(closed)
            with open(data_path, 'w') as f:
                f.write(broken)
            rc, out, err = assert_python_ok(self.script, '-r', data_path)
            self.assertEqual(out, b'')
            self.assertEqual(err, b'')
            with open(backup) as f:
                self.assertEqual(f.read(), broken)
            with open(data_path) as f:
                indented = f.read()
            compile(indented, '_test.py', 'exec')
            self.assertEqual(self.pindent(broken, '-r'), indented)

    def pindent_test(self, clean, closed):
        self.assertEqual(self.pindent(clean, '-c'), closed)
        self.assertEqual(self.pindent(closed, '-d'), clean)
        broken = self.lstriplines(closed)
        self.assertEqual(self.pindent(broken, '-r', '-e', '-s', '4'), closed)

    def test_statements(self):
        clean = textwrap.dedent("""\
            if a:
                pass

            if a:
                pass
            else:
                pass

            if a:
                pass
            elif:
                pass
            else:
                pass

            while a:
                break

            while a:
                break
            else:
                pass

            for i in a:
                break

            for i in a:
                break
            else:
                pass

            try:
                pass
            finally:
                pass

            try:
                pass
            except TypeError:
                pass
            except ValueError:
                pass
            else:
                pass

            try:
                pass
            except TypeError:
                pass
            except ValueError:
                pass
            finally:
                pass

            with a:
                pass

            class A:
                pass

            def f():
                pass
            """)

        closed = textwrap.dedent("""\
            if a:
                pass
            # end if

            if a:
                pass
            else:
                pass
            # end if

            if a:
                pass
            elif:
                pass
            else:
                pass
            # end if

            while a:
                break
            # end while

            while a:
                break
            else:
                pass
            # end while

            for i in a:
                break
            # end for

            for i in a:
                break
            else:
                pass
            # end for

            try:
                pass
            finally:
                pass
            # end try

            try:
                pass
            except TypeError:
                pass
            except ValueError:
                pass
            else:
                pass
            # end try

            try:
                pass
            except TypeError:
                pass
            except ValueError:
                pass
            finally:
                pass
            # end try

            with a:
                pass
            # end with

            class A:
                pass
            # end class A

            def f():
                pass
            # end def f
            """)
        self.pindent_test(clean, closed)

    def test_multilevel(self):
        clean = textwrap.dedent("""\
            def foobar(a, b):
                if a == b:
                    a = a+1
                elif a < b:
                    b = b-1
                    if b > a: a = a-1
                else:
                    print 'oops!'
            """)
        closed = textwrap.dedent("""\
            def foobar(a, b):
                if a == b:
                    a = a+1
                elif a < b:
                    b = b-1
                    if b > a: a = a-1
                    # end if
                else:
                    print 'oops!'
                # end if
            # end def foobar
            """)
        self.pindent_test(clean, closed)

    def test_preserve_indents(self):
        clean = textwrap.dedent("""\
            if a:
                     if b:
                              pass
            """)
        closed = textwrap.dedent("""\
            if a:
                     if b:
                              pass
                     # end if
            # end if
            """)
        self.assertEqual(self.pindent(clean, '-c'), closed)
        self.assertEqual(self.pindent(closed, '-d'), clean)
        broken = self.lstriplines(closed)
        self.assertEqual(self.pindent(broken, '-r', '-e', '-s', '9'), closed)
        clean = textwrap.dedent("""\
            if a:
            \tif b:
            \t\tpass
            """)
        closed = textwrap.dedent("""\
            if a:
            \tif b:
            \t\tpass
            \t# end if
            # end if
            """)
        self.assertEqual(self.pindent(clean, '-c'), closed)
        self.assertEqual(self.pindent(closed, '-d'), clean)
        broken = self.lstriplines(closed)
        self.assertEqual(self.pindent(broken, '-r'), closed)

    def test_escaped_newline(self):
        clean = textwrap.dedent("""\
            class\\
            \\
             A:
               def\
            \\
            f:
                  pass
            """)
        closed = textwrap.dedent("""\
            class\\
            \\
             A:
               def\
            \\
            f:
                  pass
               # end def f
            # end class A
            """)
        self.assertEqual(self.pindent(clean, '-c'), closed)
        self.assertEqual(self.pindent(closed, '-d'), clean)

    def test_empty_line(self):
        clean = textwrap.dedent("""\
            if a:

                pass
            """)
        closed = textwrap.dedent("""\
            if a:

                pass
            # end if
            """)
        self.pindent_test(clean, closed)

    def test_oneline(self):
        clean = textwrap.dedent("""\
            if a: pass
            """)
        closed = textwrap.dedent("""\
            if a: pass
            # end if
            """)
        self.pindent_test(clean, closed)


class TestSundryScripts(unittest.TestCase):
    # At least make sure the rest don't have syntax errors.  When tests are
    # added for a script it should be added to the whitelist below.

    # scripts that have independent tests.
    whitelist = ['reindent.py', 'pdeps.py', 'gprof2html', 'md5sum.py']
    # scripts that can't be imported without running
    blacklist = ['make_ctype.py']
    # scripts that use windows-only modules
    windows_only = ['win_add2path.py']
    # blacklisted for other reasons
    other = ['analyze_dxp.py']

    skiplist = blacklist + whitelist + windows_only + other

    def setUp(self):
        cm = support.DirsOnSysPath(scriptsdir)
        cm.__enter__()
        self.addCleanup(cm.__exit__)

    def test_sundry(self):
        for fn in os.listdir(scriptsdir):
            if fn.endswith('.py') and fn not in self.skiplist:
                __import__(fn[:-3])

    @unittest.skipIf(sys.platform != "win32", "Windows-only test")
    def test_sundry_windows(self):
        for fn in self.windows_only:
            __import__(fn[:-3])

    @unittest.skipIf(not support.threading, "test requires _thread module")
    def test_analyze_dxp_import(self):
        if hasattr(sys, 'getdxp'):
            import analyze_dxp
        else:
            with self.assertRaises(RuntimeError):
                import analyze_dxp


class PdepsTests(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        path = os.path.join(scriptsdir, 'pdeps.py')
        spec = importlib.util.spec_from_file_location('pdeps', path)
        self.pdeps = importlib._bootstrap._load(spec)

    @classmethod
    def tearDownClass(self):
        if 'pdeps' in sys.modules:
            del sys.modules['pdeps']

    def test_process_errors(self):
        # Issue #14492: m_import.match(line) can be None.
        with tempfile.TemporaryDirectory() as tmpdir:
            fn = os.path.join(tmpdir, 'foo')
            with open(fn, 'w') as stream:
                stream.write("#!/this/will/fail")
            self.pdeps.process(fn, {})

    def test_inverse_attribute_error(self):
        # Issue #14492: this used to fail with an AttributeError.
        self.pdeps.inverse({'a': []})


class Gprof2htmlTests(unittest.TestCase):

    def setUp(self):
        path = os.path.join(scriptsdir, 'gprof2html.py')
        spec = importlib.util.spec_from_file_location('gprof2html', path)
        self.gprof = importlib._bootstrap._load(spec)
        oldargv = sys.argv
        def fixup():
            sys.argv = oldargv
        self.addCleanup(fixup)
        sys.argv = []

    def test_gprof(self):
        # Issue #14508: this used to fail with an NameError.
        with mock.patch.object(self.gprof, 'webbrowser') as wmock, \
                tempfile.TemporaryDirectory() as tmpdir:
            fn = os.path.join(tmpdir, 'abc')
            open(fn, 'w').close()
            sys.argv = ['gprof2html', fn]
            self.gprof.main()
        self.assertTrue(wmock.open.called)


class MD5SumTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.script = os.path.join(scriptsdir, 'md5sum.py')
        os.mkdir(support.TESTFN)
        cls.fodder = os.path.join(support.TESTFN, 'md5sum.fodder')
        with open(cls.fodder, 'wb') as f:
            f.write(b'md5sum\r\ntest file\r\n')
        cls.fodder_md5 = b'd38dae2eb1ab346a292ef6850f9e1a0d'
        cls.fodder_textmode_md5 = b'a8b07894e2ca3f2a4c3094065fa6e0a5'

    @classmethod
    def tearDownClass(cls):
        support.rmtree(support.TESTFN)

    def test_noargs(self):
        rc, out, err = assert_python_ok(self.script)
        self.assertEqual(rc, 0)
        self.assertTrue(
            out.startswith(b'd41d8cd98f00b204e9800998ecf8427e <stdin>'))
        self.assertFalse(err)

    def test_checksum_fodder(self):
        rc, out, err = assert_python_ok(self.script, self.fodder)
        self.assertEqual(rc, 0)
        self.assertTrue(out.startswith(self.fodder_md5))
        for part in self.fodder.split(os.path.sep):
            self.assertIn(part.encode(), out)
        self.assertFalse(err)

    def test_dash_l(self):
        rc, out, err = assert_python_ok(self.script, '-l', self.fodder)
        self.assertEqual(rc, 0)
        self.assertIn(self.fodder_md5, out)
        parts = self.fodder.split(os.path.sep)
        self.assertIn(parts[-1].encode(), out)
        self.assertNotIn(parts[-2].encode(), out)

    def test_dash_t(self):
        rc, out, err = assert_python_ok(self.script, '-t', self.fodder)
        self.assertEqual(rc, 0)
        self.assertTrue(out.startswith(self.fodder_textmode_md5))
        self.assertNotIn(self.fodder_md5, out)

    def test_dash_s(self):
        rc, out, err = assert_python_ok(self.script, '-s', '512', self.fodder)
        self.assertEqual(rc, 0)
        self.assertIn(self.fodder_md5, out)

    def test_multiple_files(self):
        rc, out, err = assert_python_ok(self.script, self.fodder, self.fodder)
        self.assertEqual(rc, 0)
        lines = out.splitlines()
        self.assertEqual(len(lines), 2)
        self.assertEqual(*lines)

    def test_usage(self):
        rc, out, err = assert_python_failure(self.script, '-h')
        self.assertEqual(rc, 2)
        self.assertEqual(out, b'')
        self.assertGreater(err, b'')


# Run the tests in Tools/parser/test_unparse.py
with support.DirsOnSysPath(os.path.join(basepath, 'parser')):
    from test_unparse import UnparseTestCase
    from test_unparse import DirectoryTestCase

if __name__ == '__main__':
    unittest.main()
