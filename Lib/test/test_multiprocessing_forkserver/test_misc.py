import os
import re
import unittest
from multiprocessing.util import _SUN_PATH_MAX
from test._test_multiprocessing import install_tests_in_module_dict
from test import support
from test.support import os_helper, script_helper

install_tests_in_module_dict(globals(), 'forkserver', exclude_types=True)


class TestForkServerConfiguration(unittest.TestCase):
    def test_client_hanging_up_during_handshake(self):
        # A client that goes away without reading the forkserver's challenge
        # resets the connection on Linux. The forkserver must survive it, as
        # it survives a client that hangs up cleanly.
        cmd = f'''if 1:
            import multiprocessing, os, socket, time
            from multiprocessing import forkserver

            if __name__ == "__main__":
                forkserver.ensure_running()
                pid = forkserver._forkserver._forkserver_pid
                address = forkserver._forkserver._forkserver_address
                with socket.socket(socket.AF_UNIX) as client:
                    client.connect(address)
                    client.settimeout({support.SHORT_TIMEOUT})
                    client.recv(1, socket.MSG_PEEK)  # the challenge is here
                time.sleep(0.5)  # let the forkserver read the reset
                if os.waitpid(pid, os.WNOHANG)[0]:
                    print("forkserver died")
                    raise SystemExit
                ctx = multiprocessing.get_context("forkserver")
                p = ctx.Process(target=time.sleep, args=(0,))
                p.start()
                p.join()
                assert forkserver._forkserver._forkserver_pid == pid
                print("exitcode", p.exitcode)
        '''
        _, out, _ = script_helper.assert_python_ok('-c', cmd)
        self.assertEqual(out.decode().strip(), "exitcode 0")

    def test_respect_sun_path_max(self):
        # Ensure that the calculation for temporary filepath lengths is correct.
        # See https://github.com/python/cpython/issues/149527.

        cmd = '''if 1:
            from multiprocessing.connection import arbitrary_address
            from multiprocessing.util import get_temp_dir
            if __name__ == "__main__":
                print(get_temp_dir())
                print(arbitrary_address("AF_UNIX"))
        '''
        with os_helper.temp_dir() as root:
            self.assertLess(len(root), _SUN_PATH_MAX)
            _, out, _ = script_helper.assert_python_ok('-c', cmd, TMPDIR=root)
        res = out.decode().strip().splitlines()
        self.assertEqual(len(res), 2)

        # get_temp_dir() with a TMPDIR that has a too large length ends up
        # NOT being used as it is not possible to use it. In particular,
        # the corresponding path will fallback to one of the "built-in"
        # ones such as '/tmp', '/var/tmp', or '/usr/tmp'.
        root_regex = "|".join(map(re.escape, [root, "/tmp", "/var/tmp", "/usr/tmp"]))

        temp_pymp = res[0]
        self.assertLess(len(temp_pymp), _SUN_PATH_MAX)
        temp_pymp_regex = os.path.join(root_regex, r"pymp-\w{8}")
        self.assertRegex(temp_pymp, temp_pymp_regex)

        temp_sock = res[1]
        self.assertLess(len(temp_sock), _SUN_PATH_MAX)
        temp_sock_regex = os.path.join(temp_pymp_regex, r"sock-[0-9a-fA-F]{12}")
        self.assertRegex(temp_sock, temp_sock_regex)


if __name__ == '__main__':
    unittest.main()
