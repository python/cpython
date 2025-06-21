"""Test suite for the profile module."""

import sys
import pstats
import unittest
import os
import subprocess
import tempfile
import shutil
from difflib import unified_diff
from io import StringIO
from test.support.os_helper import TESTFN, unlink, temp_dir, change_cwd
from contextlib import contextmanager, redirect_stdout

import profile
from test.profilee import testfunc, timer
from test.support.script_helper import assert_python_failure, assert_python_ok


class ProfileTest(unittest.TestCase):

    profilerclass = profile.Profile
    profilermodule = profile
    methodnames = ['print_stats', 'print_callers', 'print_callees']
    expected_max_output = ':0(max)'

    def tearDown(self):
        unlink(TESTFN)

    def get_expected_output(self):
        return _ProfileOutput

    @classmethod
    def do_profiling(cls):
        results = []
        prof = cls.profilerclass(timer, 0.001)
        start_timer = timer()
        prof.runctx("testfunc()", globals(), locals())
        results.append(timer() - start_timer)
        for methodname in cls.methodnames:
            s = StringIO()
            stats = pstats.Stats(prof, stream=s)
            stats.strip_dirs().sort_stats("stdname")
            getattr(stats, methodname)()
            output = s.getvalue().splitlines()
            mod_name = testfunc.__module__.rsplit('.', 1)[1]
            # Only compare against stats originating from the test file.
            # Prevents outside code (e.g., the io module) from causing
            # unexpected output.
            output = [line.rstrip() for line in output if mod_name in line]
            results.append('\n'.join(output))
        return results

    def test_cprofile(self):
        results = self.do_profiling()
        expected = self.get_expected_output()
        self.assertEqual(results[0], 1000)
        fail = []
        for i, method in enumerate(self.methodnames):
            a = expected[method]
            b = results[i+1]
            if a != b:
                fail.append(f"\nStats.{method} output for "
                            f"{self.profilerclass.__name__} "
                             "does not fit expectation:")
                fail.extend(unified_diff(a.split('\n'), b.split('\n'),
                            lineterm=""))
        if fail:
            self.fail("\n".join(fail))

    def test_calling_conventions(self):
        # Issue #5330: profile and cProfile wouldn't report C functions called
        # with keyword arguments. We test all calling conventions.
        stmts = [
            "max([0])",
            "max([0], key=int)",
            "max([0], **dict(key=int))",
            "max(*([0],))",
            "max(*([0],), key=int)",
            "max(*([0],), **dict(key=int))",
        ]
        for stmt in stmts:
            s = StringIO()
            prof = self.profilerclass(timer, 0.001)
            prof.runctx(stmt, globals(), locals())
            stats = pstats.Stats(prof, stream=s)
            stats.print_stats()
            res = s.getvalue()
            self.assertIn(self.expected_max_output, res,
                "Profiling {0!r} didn't report max:\n{1}".format(stmt, res))

    def test_run(self):
        with silent():
            self.profilermodule.run("int('1')")
        self.profilermodule.run("int('1')", filename=TESTFN)
        self.assertTrue(os.path.exists(TESTFN))

    def test_run_with_sort_by_values(self):
        with redirect_stdout(StringIO()) as f:
            self.profilermodule.run("int('1')", sort=('tottime', 'stdname'))
        self.assertIn("Ordered by: internal time, standard name", f.getvalue())

    def test_runctx(self):
        with silent():
            self.profilermodule.runctx("testfunc()", globals(), locals())
        self.profilermodule.runctx("testfunc()", globals(), locals(),
                                  filename=TESTFN)
        self.assertTrue(os.path.exists(TESTFN))

    def test_run_profile_as_module(self):
        # Test that -m switch needs an argument
        assert_python_failure('-m', self.profilermodule.__name__, '-m')

        # Test failure for not-existent module
        assert_python_failure('-m', self.profilermodule.__name__,
                              '-m', 'random_module_xyz')

        # Test successful run
        assert_python_ok('-m', self.profilermodule.__name__,
                         '-m', 'timeit', '-n', '1')

    def test_output_file_when_changing_directory(self):
        with temp_dir() as tmpdir, change_cwd(tmpdir):
            os.mkdir('dest')
            with open('demo.py', 'w', encoding="utf-8") as f:
                f.write('import os; os.chdir("dest")')

            assert_python_ok(
                '-m', self.profilermodule.__name__,
                '-o', 'out.pstats',
                'demo.py',
            )

            self.assertTrue(os.path.exists('out.pstats'))


class ProfileCLITests(unittest.TestCase):
    """Tests for the profile module's command line interface."""

    def setUp(self):
        self.temp_dir = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, self.temp_dir)

        self.script_content = """\
def factorial(n):
    if n <= 1:
        return 1
    return n * factorial(n-1)

if __name__ == "__main__":
    factorial(10)
"""
        self.script_file = os.path.join(self.temp_dir, "factorial_script.py")
        with open(self.script_file, "w") as f:
            f.write(self.script_content)

    def _run_profile_cli(self, *args):
        cmd = [sys.executable, '-m', 'profile', *args]
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True
        )
        stdout, stderr = proc.communicate()
        return proc.returncode, stdout, stderr

    def test_basic_profile(self):
        returncode, stdout, stderr = self._run_profile_cli(self.script_file)
        self.assertEqual(returncode, 0)
        self.assertIn("function calls", stdout)
        self.assertIn("factorial", stdout)
        self.assertIn("ncalls", stdout)

    def test_sort_options(self):
        sort_options = ['calls', 'cumulative', 'cumtime', 'file',
                        'filename', 'module', 'ncalls', 'pcalls',
                        'line', 'stdname', 'time', 'tottime']

        for option in sort_options:
            with self.subTest(sort_option=option):
                returncode, stdout, stderr = self._run_profile_cli(
                    '-s', option, self.script_file
                )
                self.assertEqual(returncode, 0)
                self.assertIn("function calls", stdout)

    def test_output_file(self):
        output_file = os.path.join(self.temp_dir, "profile_output.prof")

        returncode, stdout, stderr = self._run_profile_cli(
            '-o', output_file, self.script_file
        )
        self.assertEqual(returncode, 0)

        self.assertTrue(os.path.exists(output_file))
        stats = pstats.Stats(output_file)
        self.assertGreater(stats.total_calls, 0)

    def test_invalid_option(self):
        returncode, stdout, stderr = self._run_profile_cli(
            '--invalid-option', self.script_file
        )
        self.assertNotEqual(returncode, 0)
        self.assertIn("error", stderr.lower())

    def test_no_arguments(self):
        returncode, stdout, stderr = self._run_profile_cli()
        self.assertNotEqual(returncode, 0)

        combined_output = stdout.lower() + stderr.lower()
        self.assertTrue(
            "usage:" in combined_output or
            "error:" in combined_output or
            "no script filename specified" in combined_output,
            "Expected usage information or error message not found"
        )

    def test_run_module(self):
        module_name = "profilemod"
        module_file = os.path.join(self.temp_dir, f"{module_name}.py")

        with open(module_file, "w") as f:
            f.write("print('Module executed')\n")

        env = os.environ.copy()

        python_path = self.temp_dir
        if 'PYTHONPATH' in env:
            python_path = os.pathsep.join([python_path, env['PYTHONPATH']])
        env['PYTHONPATH'] = python_path

        cmd = [sys.executable, '-m', 'profile', '-m', module_name]
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            env=env
        )
        stdout, stderr = proc.communicate()
        returncode = proc.returncode

        self.assertEqual(returncode, 0)
        self.assertIn("Module executed", stdout)
        self.assertIn("function calls", stdout)


def regenerate_expected_output(filename, cls):
    filename = filename.rstrip('co')
    print('Regenerating %s...' % filename)
    results = cls.do_profiling()

    newfile = []
    with open(filename, 'r') as f:
        for line in f:
            newfile.append(line)
            if line.startswith('#--cut'):
                break

    with open(filename, 'w') as f:
        f.writelines(newfile)
        f.write("_ProfileOutput = {}\n")
        for i, method in enumerate(cls.methodnames):
            f.write('_ProfileOutput[%r] = """\\\n%s"""\n' % (
                    method, results[i+1]))
        f.write('\nif __name__ == "__main__":\n    main()\n')

@contextmanager
def silent():
    stdout = sys.stdout
    try:
        sys.stdout = StringIO()
        yield
    finally:
        sys.stdout = stdout


def main():
    if '-r' not in sys.argv:
        unittest.main()
    else:
        regenerate_expected_output(__file__, ProfileTest)


# Don't remove this comment. Everything below it is auto-generated.
#--cut--------------------------------------------------------------------------
_ProfileOutput = {}
_ProfileOutput['print_stats'] = """\
       28   27.972    0.999   27.972    0.999 profilee.py:110(__getattr__)
        1  269.996  269.996  999.769  999.769 profilee.py:25(testfunc)
     23/3  149.937    6.519  169.917   56.639 profilee.py:35(factorial)
       20   19.980    0.999   19.980    0.999 profilee.py:48(mul)
        2   39.986   19.993  599.830  299.915 profilee.py:55(helper)
        4  115.984   28.996  119.964   29.991 profilee.py:73(helper1)
        2   -0.006   -0.003  139.946   69.973 profilee.py:84(helper2_indirect)
        8  311.976   38.997  399.912   49.989 profilee.py:88(helper2)
        8   63.976    7.997   79.960    9.995 profilee.py:98(subhelper)"""
_ProfileOutput['print_callers'] = """\
:0(append)                        <- profilee.py:73(helper1)(4)  119.964
:0(exception)                     <- profilee.py:73(helper1)(4)  119.964
:0(hasattr)                       <- profilee.py:73(helper1)(4)  119.964
                                     profilee.py:88(helper2)(8)  399.912
profilee.py:110(__getattr__)      <- :0(hasattr)(12)   11.964
                                     profilee.py:98(subhelper)(16)   79.960
profilee.py:25(testfunc)          <- <string>:1(<module>)(1)  999.767
profilee.py:35(factorial)         <- profilee.py:25(testfunc)(1)  999.769
                                     profilee.py:35(factorial)(20)  169.917
                                     profilee.py:84(helper2_indirect)(2)  139.946
profilee.py:48(mul)               <- profilee.py:35(factorial)(20)  169.917
profilee.py:55(helper)            <- profilee.py:25(testfunc)(2)  999.769
profilee.py:73(helper1)           <- profilee.py:55(helper)(4)  599.830
profilee.py:84(helper2_indirect)  <- profilee.py:55(helper)(2)  599.830
profilee.py:88(helper2)           <- profilee.py:55(helper)(6)  599.830
                                     profilee.py:84(helper2_indirect)(2)  139.946
profilee.py:98(subhelper)         <- profilee.py:88(helper2)(8)  399.912"""
_ProfileOutput['print_callees'] = """\
:0(hasattr)                       -> profilee.py:110(__getattr__)(12)   27.972
<string>:1(<module>)              -> profilee.py:25(testfunc)(1)  999.769
profilee.py:110(__getattr__)      ->
profilee.py:25(testfunc)          -> profilee.py:35(factorial)(1)  169.917
                                     profilee.py:55(helper)(2)  599.830
profilee.py:35(factorial)         -> profilee.py:35(factorial)(20)  169.917
                                     profilee.py:48(mul)(20)   19.980
profilee.py:48(mul)               ->
profilee.py:55(helper)            -> profilee.py:73(helper1)(4)  119.964
                                     profilee.py:84(helper2_indirect)(2)  139.946
                                     profilee.py:88(helper2)(6)  399.912
profilee.py:73(helper1)           -> :0(append)(4)   -0.004
profilee.py:84(helper2_indirect)  -> profilee.py:35(factorial)(2)  169.917
                                     profilee.py:88(helper2)(2)  399.912
profilee.py:88(helper2)           -> :0(hasattr)(8)   11.964
                                     profilee.py:98(subhelper)(8)   79.960
profilee.py:98(subhelper)         -> profilee.py:110(__getattr__)(16)   27.972"""

if __name__ == "__main__":
    main()
