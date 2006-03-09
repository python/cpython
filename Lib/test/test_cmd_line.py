
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
        return data

    def exit_code(self, cmd_line):
        return subprocess.call([sys.executable, cmd_line], stderr=subprocess.PIPE)

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

def test_main():
    test.test_support.run_unittest(CmdLineTest)

if __name__ == "__main__":
    test_main()
