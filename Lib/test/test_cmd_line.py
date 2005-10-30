
import test.test_support, unittest
import sys
import popen2

class CmdLineTest(unittest.TestCase):
    def start_python(self, cmd_line):
        outfp, infp = popen2.popen4('%s %s' % (sys.executable, cmd_line))
        infp.close()
        data = outfp.read()
        outfp.close()
        return data

    def test_directories(self):
        # Does this test make sense?  The message for "< ."  may depend on
        # the command shell, and the message for "." depends on the OS.
        if sys.platform.startswith("win"):
            # On WinXP w/ cmd.exe,
            #    "< ." gives "Access is denied.\n"
            #    "."   gives "C:\\Code\\python\\PCbuild\\python.exe: " +
            #                "can't open file '.':" +
            #                "[Errno 13] Permission denied\n"
            lookfor = " denied"  # common to both cases
        else:
            # This is what the test looked for originally, on all platforms.
            lookfor = "is a directory"
        self.assertTrue(lookfor in self.start_python('.'))
        self.assertTrue(lookfor in self.start_python('< .'))

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
