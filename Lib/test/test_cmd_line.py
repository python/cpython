# Tests invocation of the interpreter with various command line arguments
# All tests are executed with environment variables ignored
# See test_cmd_line_script.py for testing of script execution

import test.test_support, unittest
import sys
import subprocess

def _spawn_python(*args):
    cmd_line = [sys.executable, '-E']
    cmd_line.extend(args)
    return subprocess.Popen(cmd_line, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

def _kill_python(p):
    p.stdin.close()
    data = p.stdout.read()
    p.stdout.close()
    # try to cleanup the child so we don't appear to leak when running
    # with regrtest -R.  This should be a no-op on Windows.
    subprocess._cleanup()
    return data

class CmdLineTest(unittest.TestCase):
    def start_python(self, *args):
        p = _spawn_python(*args)
        return _kill_python(p)

    def exit_code(self, *args):
        cmd_line = [sys.executable, '-E']
        cmd_line.extend(args)
        return subprocess.call(cmd_line, stdout=subprocess.PIPE,
                                         stderr=subprocess.PIPE)

    def test_directories(self):
        self.assertNotEqual(self.exit_code('.'), 0)
        self.assertNotEqual(self.exit_code('< .'), 0)

    def verify_valid_flag(self, cmd_line):
        data = self.start_python(cmd_line)
        self.assertTrue(data == '' or data.endswith('\n'))
        self.assertTrue('Traceback' not in data)

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
        self.assertTrue('usage' in self.start_python('-h'))

    def test_version(self):
        version = 'Python %d.%d' % sys.version_info[:2]
        self.assertTrue(self.start_python('-V').startswith(version))

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
        p = _spawn_python('-i', '-m', 'timeit', '-n', '1')
        p.stdin.write('Timer\n')
        p.stdin.write('exit()\n')
        data = _kill_python(p)
        self.assertTrue(data.startswith('1 loop'))
        self.assertTrue('__main__.Timer' in data)

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


def test_main():
    test.test_support.run_unittest(CmdLineTest)
    test.test_support.reap_children()

if __name__ == "__main__":
    test_main()
