import contextlib
import os
import os.path
import sys
import tempfile
import test.support
import unittest
import unittest.mock

import ensurepip
import ensurepip._uninstall


class TestPackages(unittest.TestCase):
    def touch(self, directory, filename):
        fullname = os.path.join(directory, filename)
        open(fullname, "wb").close()

    def test_version(self):
        # Test version()
        with tempfile.TemporaryDirectory() as tmpdir:
            self.touch(tmpdir, "pip-1.2.3b1-py2.py3-none-any.whl")
            self.touch(tmpdir, "setuptools-49.1.3-py3-none-any.whl")
            with (unittest.mock.patch.object(ensurepip, '_PACKAGES', None),
                  unittest.mock.patch.object(ensurepip, '_WHEEL_PKG_DIR', tmpdir)):
                self.assertEqual(ensurepip.version(), '1.2.3b1')

    def test_get_packages_no_dir(self):
        # Test _get_packages() without a wheel package directory
        with (unittest.mock.patch.object(ensurepip, '_PACKAGES', None),
              unittest.mock.patch.object(ensurepip, '_WHEEL_PKG_DIR', None)):
            packages = ensurepip._get_packages()

            # when bundled wheel packages are used, we get _PIP_VERSION
            self.assertEqual(ensurepip._PIP_VERSION, ensurepip.version())

        # use bundled wheel packages
        self.assertIsNotNone(packages['pip'].wheel_name)
        self.assertIsNotNone(packages['setuptools'].wheel_name)

    def test_get_packages_with_dir(self):
        # Test _get_packages() with a wheel package directory
        setuptools_filename = "setuptools-49.1.3-py3-none-any.whl"
        pip_filename = "pip-20.2.2-py2.py3-none-any.whl"

        with tempfile.TemporaryDirectory() as tmpdir:
            self.touch(tmpdir, setuptools_filename)
            self.touch(tmpdir, pip_filename)
            # not used, make sure that it's ignored
            self.touch(tmpdir, "wheel-0.34.2-py2.py3-none-any.whl")

            with (unittest.mock.patch.object(ensurepip, '_PACKAGES', None),
                  unittest.mock.patch.object(ensurepip, '_WHEEL_PKG_DIR', tmpdir)):
                packages = ensurepip._get_packages()

            self.assertEqual(packages['setuptools'].version, '49.1.3')
            self.assertEqual(packages['setuptools'].wheel_path,
                             os.path.join(tmpdir, setuptools_filename))
            self.assertEqual(packages['pip'].version, '20.2.2')
            self.assertEqual(packages['pip'].wheel_path,
                             os.path.join(tmpdir, pip_filename))

            # wheel package is ignored
            self.assertEqual(sorted(packages), ['pip', 'setuptools'])


class EnsurepipMixin:

    def setUp(self):
        run_pip_patch = unittest.mock.patch("ensurepip._run_pip")
        self.run_pip = run_pip_patch.start()
        self.run_pip.return_value = 0
        self.addCleanup(run_pip_patch.stop)

        # Avoid side effects on the actual os module
        real_devnull = os.devnull
        os_patch = unittest.mock.patch("ensurepip.os")
        patched_os = os_patch.start()
        # But expose os.listdir() used by _find_packages()
        patched_os.listdir = os.listdir
        self.addCleanup(os_patch.stop)
        patched_os.devnull = real_devnull
        patched_os.path = os.path
        self.os_environ = patched_os.environ = os.environ.copy()


class TestBootstrap(EnsurepipMixin, unittest.TestCase):

    def test_basic_bootstrapping(self):
        ensurepip.bootstrap()

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

        additional_paths = self.run_pip.call_args[0][1]
        self.assertEqual(len(additional_paths), 2)

    def test_bootstrapping_with_root(self):
        ensurepip.bootstrap(root="/foo/bar/")

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "--root", "/foo/bar/",
                "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_user(self):
        ensurepip.bootstrap(user=True)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "--user", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_upgrade(self):
        ensurepip.bootstrap(upgrade=True)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "--upgrade", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_verbosity_1(self):
        ensurepip.bootstrap(verbosity=1)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "-v", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_verbosity_2(self):
        ensurepip.bootstrap(verbosity=2)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "-vv", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_verbosity_3(self):
        ensurepip.bootstrap(verbosity=3)

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "-vvv", "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

    def test_bootstrapping_with_regular_install(self):
        ensurepip.bootstrap()
        self.assertEqual(self.os_environ["ENSUREPIP_OPTIONS"], "install")

    def test_bootstrapping_with_alt_install(self):
        ensurepip.bootstrap(altinstall=True)
        self.assertEqual(self.os_environ["ENSUREPIP_OPTIONS"], "altinstall")

    def test_bootstrapping_with_default_pip(self):
        ensurepip.bootstrap(default_pip=True)
        self.assertNotIn("ENSUREPIP_OPTIONS", self.os_environ)

    def test_altinstall_default_pip_conflict(self):
        with self.assertRaises(ValueError):
            ensurepip.bootstrap(altinstall=True, default_pip=True)
        self.assertFalse(self.run_pip.called)

    def test_pip_environment_variables_removed(self):
        # ensurepip deliberately ignores all pip environment variables
        # See http://bugs.python.org/issue19734 for details
        self.os_environ["PIP_THIS_SHOULD_GO_AWAY"] = "test fodder"
        ensurepip.bootstrap()
        self.assertNotIn("PIP_THIS_SHOULD_GO_AWAY", self.os_environ)

    def test_pip_config_file_disabled(self):
        # ensurepip deliberately ignores the pip config file
        # See http://bugs.python.org/issue20053 for details
        ensurepip.bootstrap()
        self.assertEqual(self.os_environ["PIP_CONFIG_FILE"], os.devnull)

@contextlib.contextmanager
def fake_pip(version=ensurepip.version()):
    if version is None:
        pip = None
    else:
        class FakePip():
            __version__ = version
        pip = FakePip()
    sentinel = object()
    orig_pip = sys.modules.get("pip", sentinel)
    sys.modules["pip"] = pip
    try:
        yield pip
    finally:
        if orig_pip is sentinel:
            del sys.modules["pip"]
        else:
            sys.modules["pip"] = orig_pip

class TestUninstall(EnsurepipMixin, unittest.TestCase):

    def test_uninstall_skipped_when_not_installed(self):
        with fake_pip(None):
            ensurepip._uninstall_helper()
        self.assertFalse(self.run_pip.called)

    def test_uninstall_skipped_with_warning_for_wrong_version(self):
        with fake_pip("not a valid version"):
            with test.support.captured_stderr() as stderr:
                ensurepip._uninstall_helper()
        warning = stderr.getvalue().strip()
        self.assertIn("only uninstall a matching version", warning)
        self.assertFalse(self.run_pip.called)


    def test_uninstall(self):
        with fake_pip():
            ensurepip._uninstall_helper()

        self.run_pip.assert_called_once_with(
            [
                "uninstall", "-y", "--disable-pip-version-check", "pip",
                "setuptools",
            ]
        )

    def test_uninstall_with_verbosity_1(self):
        with fake_pip():
            ensurepip._uninstall_helper(verbosity=1)

        self.run_pip.assert_called_once_with(
            [
                "uninstall", "-y", "--disable-pip-version-check", "-v", "pip",
                "setuptools",
            ]
        )

    def test_uninstall_with_verbosity_2(self):
        with fake_pip():
            ensurepip._uninstall_helper(verbosity=2)

        self.run_pip.assert_called_once_with(
            [
                "uninstall", "-y", "--disable-pip-version-check", "-vv", "pip",
                "setuptools",
            ]
        )

    def test_uninstall_with_verbosity_3(self):
        with fake_pip():
            ensurepip._uninstall_helper(verbosity=3)

        self.run_pip.assert_called_once_with(
            [
                "uninstall", "-y", "--disable-pip-version-check", "-vvv",
                "pip", "setuptools",
            ]
        )

    def test_pip_environment_variables_removed(self):
        # ensurepip deliberately ignores all pip environment variables
        # See http://bugs.python.org/issue19734 for details
        self.os_environ["PIP_THIS_SHOULD_GO_AWAY"] = "test fodder"
        with fake_pip():
            ensurepip._uninstall_helper()
        self.assertNotIn("PIP_THIS_SHOULD_GO_AWAY", self.os_environ)

    def test_pip_config_file_disabled(self):
        # ensurepip deliberately ignores the pip config file
        # See http://bugs.python.org/issue20053 for details
        with fake_pip():
            ensurepip._uninstall_helper()
        self.assertEqual(self.os_environ["PIP_CONFIG_FILE"], os.devnull)


# Basic testing of the main functions and their argument parsing

EXPECTED_VERSION_OUTPUT = "pip " + ensurepip.version()

class TestBootstrappingMainFunction(EnsurepipMixin, unittest.TestCase):

    def test_bootstrap_version(self):
        with test.support.captured_stdout() as stdout:
            with self.assertRaises(SystemExit):
                ensurepip._main(["--version"])
        result = stdout.getvalue().strip()
        self.assertEqual(result, EXPECTED_VERSION_OUTPUT)
        self.assertFalse(self.run_pip.called)

    def test_basic_bootstrapping(self):
        exit_code = ensurepip._main([])

        self.run_pip.assert_called_once_with(
            [
                "install", "--no-cache-dir", "--no-index", "--find-links",
                unittest.mock.ANY, "setuptools", "pip",
            ],
            unittest.mock.ANY,
        )

        additional_paths = self.run_pip.call_args[0][1]
        self.assertEqual(len(additional_paths), 2)
        self.assertEqual(exit_code, 0)

    def test_bootstrapping_error_code(self):
        self.run_pip.return_value = 2
        exit_code = ensurepip._main([])
        self.assertEqual(exit_code, 2)


class TestUninstallationMainFunction(EnsurepipMixin, unittest.TestCase):

    def test_uninstall_version(self):
        with test.support.captured_stdout() as stdout:
            with self.assertRaises(SystemExit):
                ensurepip._uninstall._main(["--version"])
        result = stdout.getvalue().strip()
        self.assertEqual(result, EXPECTED_VERSION_OUTPUT)
        self.assertFalse(self.run_pip.called)

    def test_basic_uninstall(self):
        with fake_pip():
            exit_code = ensurepip._uninstall._main([])

        self.run_pip.assert_called_once_with(
            [
                "uninstall", "-y", "--disable-pip-version-check", "pip",
                "setuptools",
            ]
        )

        self.assertEqual(exit_code, 0)

    def test_uninstall_error_code(self):
        with fake_pip():
            self.run_pip.return_value = 2
            exit_code = ensurepip._uninstall._main([])
        self.assertEqual(exit_code, 2)


if __name__ == "__main__":
    unittest.main()
