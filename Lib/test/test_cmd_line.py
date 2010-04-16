# Tests invocation of the interpreter with various command line arguments
# All tests are executed with environment variables ignored
# See test_cmd_line_script.py for testing of script execution

import test.support, unittest
import os
import sys
from test.script_helper import spawn_python, kill_python, python_exit_code

# XXX (ncoghlan): there are assorted gratuitous inconsistencies between the
# support code in the Py3k version and the 2.x version that unnecessarily
# complicate test suite merges. See issue 7331

# spawn_python normally enforces use of -E to avoid environmental effects
# but one test checks PYTHONPATH behaviour explicitly
# XXX (ncoghlan): Give script_helper.spawn_python an option to switch
# off the -E flag that is normally inserted automatically
import subprocess
def _spawn_python_with_env(*args):
    cmd_line = [sys.executable]
    cmd_line.extend(args)
    return subprocess.Popen(cmd_line, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


# XXX (ncoghlan): Move to script_helper and make consistent with run_python
def _kill_python_and_exit_code(p):
    data = kill_python(p)
    returncode = p.wait()
    return data, returncode

class CmdLineTest(unittest.TestCase):
    def start_python(self, *args):
        p = spawn_python(*args)
        return kill_python(p)

    def start_python_and_exit_code(self, *args):
        p = spawn_python(*args)
        return _kill_python_and_exit_code(p)

    def exit_code(self, *args):
        return python_exit_code(*args)

    def test_directories(self):
        self.assertNotEqual(self.exit_code('.'), 0)
        self.assertNotEqual(self.exit_code('< .'), 0)

    def verify_valid_flag(self, cmd_line):
        data = self.start_python(cmd_line)
        self.assertTrue(data == b'' or data.endswith(b'\n'))
        self.assertNotIn(b'Traceback', data)

    def test_optimize(self):
        self.verify_valid_flag('-O')
        self.verify_valid_flag('-OO')

    def test_q(self):
        self.verify_valid_flag('-Qold')
        self.verify_valid_flag('-Qnew')
        self.verify_valid_flag('-Qwarn')
        self.verify_valid_flag('-Qwarnall')

    def test_site_flag(self):
        self.verify_valid_flag('-S')

    def test_usage(self):
        self.assertIn(b'usage', self.start_python('-h'))

    def test_version(self):
        version = ('Python %d.%d' % sys.version_info[:2]).encode("ascii")
        self.assertTrue(self.start_python('-V').startswith(version))

    def test_verbose(self):
        # -v causes imports to write to stderr.  If the write to
        # stderr itself causes an import to happen (for the output
        # codec), a recursion loop can occur.
        data, rc = self.start_python_and_exit_code('-v')
        self.assertEqual(rc, 0)
        self.assertNotIn(b'stack overflow', data)
        data, rc = self.start_python_and_exit_code('-vv')
        self.assertEqual(rc, 0)
        self.assertNotIn(b'stack overflow', data)

    def test_run_module(self):
        # Test expected operation of the '-m' switch
        # Switch needs an argument
        self.assertNotEqual(self.exit_code('-m'), 0)
        # Check we get an error for a nonexistent module
        self.assertNotEqual(
            self.exit_code('-m', 'fnord43520xyz'),
            0)
        # Check the runpy module also gives an error for
        # a nonexistent module
        self.assertNotEqual(
            self.exit_code('-m', 'runpy', 'fnord43520xyz'),
            0)
        # All good if module is located and run successfully
        self.assertEqual(
            self.exit_code('-m', 'timeit', '-n', '1'),
            0)

    def test_run_module_bug1764407(self):
        # -m and -i need to play well together
        # Runs the timeit module and checks the __main__
        # namespace has been populated appropriately
        p = spawn_python('-i', '-m', 'timeit', '-n', '1')
        p.stdin.write(b'Timer\n')
        p.stdin.write(b'exit()\n')
        data = kill_python(p)
        self.assertTrue(data.find(b'1 loop') != -1)
        self.assertTrue(data.find(b'__main__.Timer') != -1)

    def test_run_code(self):
        # Test expected operation of the '-c' switch
        # Switch needs an argument
        self.assertNotEqual(self.exit_code('-c'), 0)
        # Check we get an error for an uncaught exception
        self.assertNotEqual(
            self.exit_code('-c', 'raise Exception'),
            0)
        # All good if execution is successful
        self.assertEqual(
            self.exit_code('-c', 'pass'),
            0)

        # Test handling of non-ascii data
        if sys.getfilesystemencoding() != 'ascii':
            command = "assert(ord('\xe9') == 0xe9)"
            self.assertEqual(
                self.exit_code('-c', command),
                0)

    def test_unbuffered_output(self):
        # Test expected operation of the '-u' switch
        for stream in ('stdout', 'stderr'):
            # Binary is unbuffered
            code = ("import os, sys; sys.%s.buffer.write(b'x'); os._exit(0)"
                % stream)
            data, rc = self.start_python_and_exit_code('-u', '-c', code)
            self.assertEqual(rc, 0)
            self.assertEqual(data, b'x', "binary %s not unbuffered" % stream)
            # Text is line-buffered
            code = ("import os, sys; sys.%s.write('x\\n'); os._exit(0)"
                % stream)
            data, rc = self.start_python_and_exit_code('-u', '-c', code)
            self.assertEqual(rc, 0)
            self.assertEqual(data.strip(), b'x',
                "text %s not line-buffered" % stream)

    def test_unbuffered_input(self):
        # sys.stdin still works with '-u'
        code = ("import sys; sys.stdout.write(sys.stdin.read(1))")
        p = spawn_python('-u', '-c', code)
        p.stdin.write(b'x')
        p.stdin.flush()
        data, rc = _kill_python_and_exit_code(p)
        self.assertEqual(rc, 0)
        self.assertTrue(data.startswith(b'x'), data)

    def test_large_PYTHONPATH(self):
        with test.support.EnvironmentVarGuard() as env:
            path1 = "ABCDE" * 100
            path2 = "FGHIJ" * 100
            env['PYTHONPATH'] = path1 + os.pathsep + path2

            code = """
import sys
path = ":".join(sys.path)
path = path.encode("ascii", "backslashreplace")
sys.stdout.buffer.write(path)"""
            code = code.strip().splitlines()
            code = '; '.join(code)
            p = _spawn_python_with_env('-S', '-c', code)
            stdout, _ = p.communicate()
            self.assertIn(path1.encode('ascii'), stdout)
            self.assertIn(path2.encode('ascii'), stdout)


def test_main():
    test.support.run_unittest(CmdLineTest)
    test.support.reap_children()

if __name__ == "__main__":
    test_main()
