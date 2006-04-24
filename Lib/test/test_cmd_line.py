
import test.test_support, unittest
import sys
import popen2
import subprocess

class CmdLineTest(unittest.TestCase):
    def start_python(self, cmd_line):
        outfp, infp = popen2.popen4('%s %s' % (sys.executable, cmd_line))
        infp.close()
        data = outfp.read()
        outfp.close()
        # try to cleanup the child so we don't appear to leak when running
        # with regrtest -R.  This should be a no-op on Windows.
        popen2._cleanup()
        return data

    def exit_code(self, cmd_line):
        return subprocess.call([sys.executable, cmd_line], stderr=subprocess.PIPE)

    def popen_python(self, *args):
        cmd_line = [sys.executable]
        cmd_line.extend(args)
        return subprocess.Popen(cmd_line, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    def test_directories(self):
        self.assertNotEqual(self.exit_code('.'), 0)
        self.assertNotEqual(self.exit_code('< .'), 0)

    def verify_valid_flag(self, cmd_line):
        data = self.start_python(cmd_line)
        self.assertTrue(data == '' or data.endswith('\n'))
        self.assertTrue('Traceback' not in data)

    def test_environment(self):
        self.verify_valid_flag('-E')

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
        result = self.popen_python('-m')
        exit_code = result.wait()
        self.assertNotEqual(exit_code, 0)
        err_details = result.stderr.read()
        self.assertTrue(err_details.startswith('Argument expected'))
        # Check we get an import error for a nonexistent module
        result = self.popen_python('-m', 'fnord43520xyz')
        exit_code = result.wait()
        self.assertNotEqual(exit_code, 0)
        err_details = result.stderr.read()
        self.assertTrue('ImportError' in err_details)
        # Traceback shown if the requested module is located for execution
        # and subsequently fails (even if that module is runpy)
        result = self.popen_python('-m', 'runpy', 'fnord')
        exit_code = result.wait()
        self.assertNotEqual(exit_code, 0)
        err_details = result.stderr.read()
        self.assertTrue(err_details.startswith('Traceback'))
        # Silence if module is located and run successfully
        result = self.popen_python('-m', 'timeit', '-n', '1')
        exit_code = result.wait()
        self.assertEqual(exit_code, 0)
        err_details = result.stderr.read()
        self.assertTrue(err_details in ('', '\n'))

    def test_run_code(self):
        # Test expected operation of the '-c' switch
        # Switch needs an argument
        result = self.popen_python('-c')
        exit_code = result.wait()
        self.assertNotEqual(exit_code, 0)
        err_details = result.stderr.read()
        self.assertTrue(err_details.startswith('Argument expected'))
        # Traceback shown for uncaught exceptions
        result = self.popen_python('-c', 'raise Exception')
        exit_code = result.wait()
        self.assertNotEqual(exit_code, 0)
        err_details = result.stderr.read()
        self.assertTrue(err_details.startswith('Traceback'))
        # Silence if execution is successful
        result = self.popen_python('-c', '""')
        exit_code = result.wait()
        self.assertEqual(exit_code, 0)
        err_details = result.stderr.read()
        self.assertTrue(err_details in ('', '\n'))


def test_main():
    test.test_support.run_unittest(CmdLineTest)

if __name__ == "__main__":
    test_main()
