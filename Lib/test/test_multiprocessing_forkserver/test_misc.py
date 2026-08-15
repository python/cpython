import os
import re
import unittest
from test.support import os_helper
from multiprocessing.util import _SUN_PATH_MAX
from test._test_multiprocessing import install_tests_in_module_dict
from test.support import script_helper

install_tests_in_module_dict(globals(), 'forkserver', exclude_types=True)


class TestForkServerConfiguration(unittest.TestCase):
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

        temp_pymp = res[0]
        temp_pymp_regex = os.path.join(re.escape(root), r"pymp-\w{8}")
        self.assertRegex(temp_pymp, temp_pymp_regex)

        temp_sock = res[1]
        temp_sock_regex = os.path.join(temp_pymp_regex, r"sock-[0-9a-fA-F]{12}")
        self.assertRegex(temp_sock, temp_sock_regex)


if __name__ == '__main__':
    unittest.main()
