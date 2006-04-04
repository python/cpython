import unittest
from test import test_support
import platform

class PlatformTest(unittest.TestCase):
    def test_architecture(self):
        res = platform.architecture()

    def test_machine(self):
        res = platform.machine()

    def test_node(self):
        res = platform.node()

    def test_platform(self):
        for aliased in (False, True):
            for terse in (False, True):
                res = platform.platform(aliased, terse)

    def test_processor(self):
        res = platform.processor()

    def test_python_build(self):
        res = platform.python_build()

    def test_python_compiler(self):
        res = platform.python_compiler()

    def test_version(self):
        res1 = platform.version()
        res2 = platform.version_tuple()
        self.assertEqual(res1, ".".join(res2))

    def test_release(self):
        res = platform.release()

    def test_system(self):
        res = platform.system()

    def test_version(self):
        res = platform.version()

    def test_system_alias(self):
        res = platform.system_alias(
            platform.system(),
            platform.release(),
            platform.version(),
        )

    def test_uname(self):
        res = platform.uname()

    def test_java_ver(self):
        res = platform.java_ver()

    def test_win32_ver(self):
        res = platform.win32_ver()

    def test_mac_ver(self):
        res = platform.mac_ver()

    def test_dist(self):
        res = platform.dist()

    def test_libc_ver(self):
        from sys import executable
        import os
        if os.path.isdir(executable) and os.path.exists(executable+'.exe'):
            # Cygwin horror
            executable = executable + '.exe'
        res = platform.libc_ver(executable)

def test_main():
    test_support.run_unittest(
        PlatformTest
    )

if __name__ == '__main__':
    test_main()
