import sys
import os
import unittest
import platform
import subprocess

from test import test_support

class PlatformTest(unittest.TestCase):
    def test_architecture(self):
        res = platform.architecture()

    if hasattr(os, "symlink"):
        def test_architecture_via_symlink(self): # issue3762
            def get(python):
                cmd = [python, '-c',
                    'import platform; print platform.architecture()']
                p = subprocess.Popen(cmd, stdout=subprocess.PIPE)
                return p.communicate()
            real = os.path.realpath(sys.executable)
            link = os.path.abspath(test_support.TESTFN)
            os.symlink(real, link)
            try:
                self.assertEqual(get(real), get(link))
            finally:
                os.remove(link)

    def test_platform(self):
        for aliased in (False, True):
            for terse in (False, True):
                res = platform.platform(aliased, terse)

    def test_system(self):
        res = platform.system()

    def test_node(self):
        res = platform.node()

    def test_release(self):
        res = platform.release()

    def test_version(self):
        res = platform.version()

    def test_machine(self):
        res = platform.machine()

    def test_processor(self):
        res = platform.processor()

    def test_python_implementation(self):
        res = platform.python_implementation()

    def test_python_version(self):
        res1 = platform.python_version()
        res2 = platform.python_version_tuple()
        self.assertEqual(res1, ".".join(res2))

    def test_python_branch(self):
        res = platform.python_branch()

    def test_python_revision(self):
        res = platform.python_revision()

    def test_python_build(self):
        res = platform.python_build()

    def test_python_compiler(self):
        res = platform.python_compiler()

    def test_system_alias(self):
        res = platform.system_alias(
            platform.system(),
            platform.release(),
            platform.version(),
        )

    def test_uname(self):
        res = platform.uname()
        self.assert_(any(res))

    def test_java_ver(self):
        res = platform.java_ver()
        if sys.platform == 'java':
            self.assert_(all(res))

    def test_win32_ver(self):
        res = platform.win32_ver()

    def test_mac_ver(self):
        res = platform.mac_ver()

        try:
            import gestalt
        except ImportError:
            have_toolbox_glue = False
        else:
            have_toolbox_glue = True

        if have_toolbox_glue and platform.uname()[0] == 'Darwin':
            # We're on a MacOSX system, check that
            # the right version information is returned
            fd = os.popen('sw_vers', 'r')
            real_ver = None
            for ln in fd:
                if ln.startswith('ProductVersion:'):
                    real_ver = ln.strip().split()[-1]
                    break
            fd.close()
            self.failIf(real_ver is None)
            self.assertEquals(res[0], real_ver)

            # res[1] claims to contain
            # (version, dev_stage, non_release_version)
            # That information is no longer available
            self.assertEquals(res[1], ('', '', ''))

            if sys.byteorder == 'little':
                self.assertEquals(res[2], 'i386')
            else:
                self.assertEquals(res[2], 'PowerPC')


    if sys.platform == 'darwin':
        def test_mac_ver_with_fork(self):
            # Issue7895: platform.mac_ver() crashes when using fork without exec
            #
            # This test checks that the fix for that issue works.
            #
            pid = os.fork()
            if pid == 0:
                # child
                info = platform.mac_ver()
                os._exit(0)

            else:
                # parent
                cpid, sts = os.waitpid(pid, 0)
                self.assertEquals(cpid, pid)
                self.assertEquals(sts, 0)

    def test_dist(self):
        res = platform.dist()

    def test_libc_ver(self):
        import os
        if os.path.isdir(sys.executable) and \
           os.path.exists(sys.executable+'.exe'):
            # Cygwin horror
            executable = sys.executable + '.exe'
        else:
            executable = sys.executable
        res = platform.libc_ver(executable)

    def test_parse_release_file(self):

        for input, output in (
            # Examples of release file contents:
            ('SuSE Linux 9.3 (x86-64)', ('SuSE Linux ', '9.3', 'x86-64')),
            ('SUSE LINUX 10.1 (X86-64)', ('SUSE LINUX ', '10.1', 'X86-64')),
            ('SUSE LINUX 10.1 (i586)', ('SUSE LINUX ', '10.1', 'i586')),
            ('Fedora Core release 5 (Bordeaux)', ('Fedora Core', '5', 'Bordeaux')),
            ('Red Hat Linux release 8.0 (Psyche)', ('Red Hat Linux', '8.0', 'Psyche')),
            ('Red Hat Linux release 9 (Shrike)', ('Red Hat Linux', '9', 'Shrike')),
            ('Red Hat Enterprise Linux release 4 (Nahant)', ('Red Hat Enterprise Linux', '4', 'Nahant')),
            ('CentOS release 4', ('CentOS', '4', None)),
            ('Rocks release 4.2.1 (Cydonia)', ('Rocks', '4.2.1', 'Cydonia')),
            ('', ('', '', '')), # If there's nothing there.
            ):
            self.assertEqual(platform._parse_release_file(input), output)


def test_main():
    test_support.run_unittest(
        PlatformTest
    )

if __name__ == '__main__':
    test_main()
