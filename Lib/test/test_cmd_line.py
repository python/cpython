# Tests invocation of the interpreter with various command line arguments
# All tests are executed with environment variables ignored
# See test_cmd_line_script.py for testing of script execution

import test.test_support, unittest
import sys
from test.script_helper import spawn_python, kill_python, python_exit_code


class CmdLineTest(unittest.TestCase):
    def start_python(self, *args):
        p = spawn_python(*args)
        return kill_python(p)

    def exit_code(self, *args):
        return python_exit_code(*args)

    def test_directories(self):
        self.assertNotEqual(self.exit_code('.'), 0)
        self.assertNotEqual(self.exit_code('< .'), 0)

    def verify_valid_flag(self, cmd_line):
        data = self.start_python(cmd_line)
        self.assertTrue(data == '' or data.endswith('\n'))
        self.assertNotIn('Traceback', data)

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
        self.assertIn('usage', self.start_python('-h'))

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
        p = spawn_python('-i', '-m', 'timeit', '-n', '1')
        p.stdin.write('Timer\n')
        p.stdin.write('exit()\n')
        data = kill_python(p)
        self.assertTrue(data.startswith('1 loop'))
        self.assertIn('__main__.Timer', data)

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

    def test_hash_randomization(self):
        # Verify that -R enables hash randomization:
        self.verify_valid_flag('-R')
        hashes = []
        for i in range(2):
            code = 'print(hash("spam"))'
            data = self.start_python('-R', '-c', code)
            hashes.append(data)
        self.assertNotEqual(hashes[0], hashes[1])

        # Verify that sys.flags contains hash_randomization
        code = 'import sys; print sys.flags'
        data = self.start_python('-R', '-c', code)
        self.assertTrue('hash_randomization=1' in data)

def test_main():
    test.test_support.run_unittest(CmdLineTest)
    test.test_support.reap_children()

if __name__ == "__main__":
    test_main()
